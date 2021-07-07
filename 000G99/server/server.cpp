#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h> // set socket non-block
#include "my_etc.h"

/* args */
#include <stdarg.h>

/* unix sys */
#include <sys/epoll.h>
#include <sys/stat.h> // stat file
#include <sys/sendfile.h> // sendfile
#include <sys/signal.h> // signal
#include <sys/wait.h> // waitpid


/* STL */
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

#include <exception>


/* utils */
#include "utils/json.hpp"
#include "utils/tools.hpp"
#include "utils/rio.hpp"
#include "utils/db.hpp"

/* db connection */
#include <mysql.h>

/* md5 */
#include <openssl/md5.h>

/* rand */
#include <stdlib.h>

/* debug */
#include <iostream>
#include <fstream>
#include "utils/debug.hpp"


using string = std::string;
using json = nlohmann::json;

#include <ctime>

MYSQL* db;

#define debug(...) Server::debugger(__VA_ARGS__)
#define conn(...) Server::LOG("CONN", __VA_ARGS__)
#define logger(...) Server::LOG("LOG", __VA_ARGS__)
#define trans(...) Server::LOG("TRANSACION", __VA_ARGS__)

const int listen_Q = 20;
const int MAX_EVENT_NUMBER = 3;

const int BUFFER_SIZE = 1024;

/* level-triggered */
void add_event(int epoll_fd, int fd, int state) {
	epoll_event event;
	event.data.fd = fd;
	event.events = state;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)<0){
        printf("add event failed\n");
    }
	// ���÷�����
	set_nonblock(fd);
}

void modify_event(int epoll_fd, int fd, int state){
	epoll_event event;
    event.events = state;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event)<0){
        // printf("modify event failed\n");
    }
}

void remove_event(int epoll_fd, int fd){
	epoll_event event;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL)<0){
        // printf("remove event failed\n");
    }
}

struct SOCK_INFO{
    int     sock;               // client fd
    u_short client_port;
    u_int client_ip;
    u_char buffer[BUFFER_SIZE];
    // string type;
    // json header;
    SOCK_INFO(int _sock, u_short _client_port, u_int _client_ip) : sock(_sock), client_port(_client_port), client_ip(_client_ip){};
    int recv_header();          // ��ָ����socket�ж�ȡһ��json��������
    json parse_header(int);        // �ӻ������н�����һ��json
    int send_header(string, json&);          // ��ָ����socket����һ��json����
    string parse_req_type();
    ssize_t sendn(const char*, ssize_t);
    ssize_t recvn(char*, ssize_t);
    // int parse_req(Rdbuf_ptr readbuf);
};

/*
int SOCK_INFO::parse_req(Rdbuf_ptr readbuf)
{
    type.clear();
    header.clear();

    static string msg;
    char buf[1024];
    int n;

    // ���ȶ�ȡ��Ȼ�������type��Ҳ���Ǳ��β���������
    msg = "";
    while((n = readbuf->read_until(buf, '\n')) > 0) {
        printf("receive %d bytes\n", n);
        buf[n] = 0;
        msg += buf;
        if (buf[n - 1] == '\n')
            break;
    }

    printf("read %s", msg.c_str());

    if(n < 0)
        return n;
    if(n == 0)
        return 3;
    
    // last byte is '\n'
    msg.resize(msg.size()-1);
    type = msg;

    // Ȼ���ȡ����������JSON��ʽ��header
    msg = "";

    while((n = readbuf->read_until(buf, '\0')) > 0) {
        buf[n] = 0;
        msg += buf;
        printf("msg.length()=%d", msg.length());
        if (buf[n - 1] == '\0')
            break;
    }

    printf("read %s", msg.c_str());

    if(n < 0)
        return n;
    if(n == 0)
        return 3;

    printf("Parsing header...");
    header = json::parse(msg.c_str());
    printf("Parse header sucessfully");
    // JSON��ʽ��header�������

    return 0;
}
*/

ssize_t SOCK_INFO::sendn(const char* buf, ssize_t len)
{
    char *_buf = (char*)buf;
    ssize_t cnt = 0;
    while(cnt != len)
    {
        bool ready = sock_ready_to_write(sock);
        if(!ready)
        {
            // �����ȴ�ʱ��
            return 0;
        }
        ssize_t w_bytes = write(sock, _buf, len);
        if(w_bytes == 0)
            break;
        if(w_bytes == -1)
            return -1;
        cnt += w_bytes;
        _buf += w_bytes;
        len -= w_bytes;
    }
    return cnt;
}

ssize_t SOCK_INFO::recvn(char* buffer, ssize_t len)
{
    ssize_t cnt = 0;
    while(cnt != len)
    {
        bool ready = sock_ready_to_read(sock);
        if(!ready)
        {
            // �����ȴ�ʱ��
            return 0;
        }
        ssize_t r_bytes = read(sock, buffer, len);
        if(r_bytes == 0)
            break;
        if(r_bytes == -1)
            return -1;
        cnt += r_bytes;
        buffer += r_bytes;
        len -= r_bytes;
    }
    return cnt;
}


/**
 * ������������
 * ���ȶ�ȡ1�ֽڣ������ȡ=0��˵���Զ˹ر�socket����ô����"close"
 * ���򣬶�ȡ����
 * �������󲢷���
 * !�������20�ֽڣ���ô˵���Ƿ�����
 */ 
string SOCK_INFO::parse_req_type()
{
    int i = 0;
    int byte = read(sock, buffer, 1);
    if(byte==0)
    {
        return "close";
    }
    i++;
    while (1)
    {
        byte = read(sock, buffer + i, 1);
        if(byte != 1)
        {
            return "";
        }
        if(buffer[i] == '\n')
        {
            break;
        }
        i++;
        // ����Ƿ�������һֱ��ȡ����Ҫ�ر�
        if(i == 20){
            return "close";
        }
    }
    // if(i == 20)
    //     return "";
    buffer[i] = '\0';
    print_buffer(buffer, i);
    string res = (const char *)buffer;
    memset(buffer, 0, sizeof(buffer));
    return res;
}

/**
 * �������ܣ���ָ����socket�ж�ȡ���ݵ���������ֱ����ȡ��\0����ʾ��ȡ��һ��json
 * ����ֵ�����ض�ȡ���ֽ�����
 *         �����������-1
 * ��ϸ���������ϴ�socket�н������ݣ�ÿ�ζ�ȡ���ַ�ֱ��\0��˵�������ض�ȡ��һ�� json ��ʽ��header���������н���
 */ 
int SOCK_INFO::recv_header()
{
    int i = 0;
    int cnt = 0;
    int res = -1;
    while (1)
    {
        int byte = read(sock, buffer + i, 1);
        if(byte != 1)
            return -1;
        cnt++;
        if(buffer[i]=='\0')
        {
            res = i;
            break;
        }
        if(cnt == BUFFER_SIZE)
            break;
        i++;
    }
    print_buffer(buffer, i);
    // ����forѭ����������json�ĳ���С��buffer
    return res;
}

/**
 * �������ܣ��ӻ�������ָ���ķ�Χ(0~len)������json��ʽ��header����Ϊ����ֵ����
 * ���������len����������'\0'���±�
 * ����ֵ��
 * ��ϸ������������json��ʽ��header����ʣ��������ǰ�ƶ�
 */ 
json SOCK_INFO::parse_header(int len)
{
    if(len == -1){
        printf("buffer: %s\n", buffer);
    }
    // auto header = json::parse(buffer, buffer + len + 1);
    json header = json::parse(buffer);
    memset(buffer, 0, sizeof(buffer));
    return header;
}

/**
 * ����һ��json+β��
 */ 
int SOCK_INFO::send_header(string res_type, json& header)
{
    string msg = res_type + header.dump();
    printf("***����res*****\n");
    printf("%s\n", msg.c_str());
    print_buffer((u_char *)msg.c_str(), msg.size());
    // write�п���û�а�����ȫ������
    sendn(msg.c_str(), msg.size() + 1);
    // write(sock, msg.c_str(), msg.size() + 1); // ������ +1 �෢����β��
    return 1;
}

class Server{
private:
    /* ��־ */
    FILE *log;
    FILE *debug_log;

    /* initial setting */
    sockaddr_in serv_addr;
    
    /* db��� */
    string db_ip;
    string db_name;
    string db_user;
    string db_pwd;
    
    /* �洢·����� */
    string store_path;
    string log_name;
    string debuglog_name;
    string realfile_path;

    /* socket��� */
    int listen_fd;
    int epoll_fd;
    epoll_event events[MAX_EVENT_NUMBER];

    std::unordered_map<int, std::shared_ptr<SOCK_INFO>> sinfos;
    std::unordered_map<string, string> m_type;
    string res_type;

public:
    void Run();

private:
    void read_conf();
    string get_filename_by_md5(const string &);

    /* logs */
    void LOG(const char *logLevel, const char *format, ...);
    void debugger(const char *format, ...);

private:
    void child_loop();
    void loop_once(epoll_event *events, int number);
    void close_release(std::shared_ptr<SOCK_INFO> &sinfo);

private:
    /* APIs */
    int checkSession(json &, json &, std::shared_ptr<SOCK_INFO> &);
    int checkBind(json &, json &, int, std::shared_ptr<SOCK_INFO> &);

    void signup(json &, std::shared_ptr<SOCK_INFO> &);
    void login(json &, std::shared_ptr<SOCK_INFO> &);
    void logout(json &, std::shared_ptr<SOCK_INFO> &);
    void setbind(json &, std::shared_ptr<SOCK_INFO> &);
    void disbind(json &, std::shared_ptr<SOCK_INFO> &);
    void getbindid(json &, std::shared_ptr<SOCK_INFO> &);
    void getdir(json &, std::shared_ptr<SOCK_INFO> &);

    void uploadFile(json &, std::shared_ptr<SOCK_INFO> &);
    void uploadChunk(json &, std::shared_ptr<SOCK_INFO> &);
    void downloadFile(json &, std::shared_ptr<SOCK_INFO> &);
    void deleteFile(json &, std::shared_ptr<SOCK_INFO> &);
    void createDir(json &, std::shared_ptr<SOCK_INFO> &);
    void deleteDir(json &, std::shared_ptr<SOCK_INFO> &);

public:
    Server(int _s_port, const char *_s_ip)
    {
        read_conf();
        log = fopen( log_name.c_str(), "a+");
        if(log==nullptr){
            printf("open log failed!\nlog name %s\n", log_name.c_str());
        }
        /* stdio functions are bufferd IO, so need to close buffer */
        if(setvbuf(log, NULL, _IONBF, 0)!=0){
            printf("close fprintf buffer failed!\n");
        }
        debug_log = fopen(debuglog_name.c_str(), "a+");
        setvbuf(debug_log, NULL, _IONBF, 0);

        listen_fd = create_server("0.0.0.0", _s_port, true);
        // listen to socket
        int res = listen(listen_fd, listen_Q);
        if ( res < 0){
            perror("[listen failed]");
            exit(EXIT_FAILURE);
        }


        // ����db
        if ((db = mysql_init(NULL))==NULL) {
            printf("mysql_init failed\n");
        }

        if (mysql_real_connect(db,db_ip.c_str(),db_user.c_str(), db_pwd.c_str(),db_name.c_str(), 0, NULL, 0)==NULL) {
            printf("mysql_real_connect failed(%s)\n", mysql_error(db));
        }

        mysql_set_character_set(db, "gbk");

        srand(time(NULL));

        m_type["signup"] = "signupRes\n";
        m_type["login"] = "loginRes\n";
        m_type["logout"] = "logoutRes\n";
        m_type["disbind"] = "disbindRes\n";
        m_type["setbind"] = "setbindRes\n";
        m_type["getbindid"] = "getbindidRes\n";
        m_type["getdir"] = "getdirRes\n";
        m_type["uploadFile"] = "uploadFileRes\n";
        m_type["uploadChunk"] = "uploadChunkRes\n";
        m_type["downloadFile"] = "downloadFileRes\n";
        m_type["deleteFile"] = "deleteFileRes\n";
        m_type["createDir"] = "createDirRes\n";
        m_type["deleteDir"] = "deleteDirRes\n";
        

    }
    ~Server(){
        fclose(log);

        mysql_close(db);
    }
};


void Server::read_conf()
{
    string conf_file = "/home/G1752240/u1752240.conf";
    db_ip = "127.0.0.1";
    db_name = "db1752240";
    db_user = "u1752240";
    db_pwd = "u1752240";
    std::fstream in;
    in.open(conf_file, std::ios::in);
    in >> db_ip >> db_name >> db_user >> db_pwd >> store_path;
    in.close();
    log_name = store_path + "/log/u1752240.log";
    debuglog_name = store_path + "/log/debug.log";
    realfile_path = store_path + "/realfile/";
    printf("DB: ip %s\nname %s\nuser %s\npwd %s\nstore_path %s\nlog name %s\nreal file path %s\n", db_ip.c_str(), db_name.c_str(), db_user.c_str(), db_pwd.c_str(), store_path.c_str(), log_name.c_str(), realfile_path.c_str());
}

/**
 * 
 */ 
string Server::get_filename_by_md5(const string& md5)
{
    return realfile_path + md5;
}

void Server::LOG(
			const char *logLevel,
            const char *format ,...)
{
    time_t tt = time(NULL);
    struct tm* t= localtime(&tt);
    char date[50];
    char time[50];
    sprintf(date, "%d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    sprintf(time, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    static char output[1024] = {0};
    va_list arglst;
    va_start(arglst, format);
    vsnprintf(output, sizeof(output), format, arglst);
    fprintf(log, "[%s %s] [%s] %s\n", date, time, logLevel, output);
    va_end(arglst);
}

void Server::debugger(
            const char *format ,...)
{
    static char output[1024] = {0};
    va_list arglst;
    va_start(arglst, format);
    vsnprintf(output, sizeof(output), format, arglst);
    fprintf(debug_log, "%s\n", output);
    printf("%s\n", output);
    va_end(arglst);
}


void Server::close_release(std::shared_ptr<SOCK_INFO> & sinfo){
    debug("[�ر� ]");
    close(sinfo->sock);
    struct sockaddr_in client;
    client.sin_addr.s_addr = sinfo->client_ip;
    
    conn("%s:%d disconnect.", inet_ntoa(client.sin_addr), sinfo->client_port);
    remove_event(epoll_fd, sinfo->sock);
    sinfos.erase(sinfo->sock);
    exit(0);
}

// ��Ҫ��ɵ�
void Server::Run()
{
    fd_set rfds;
    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        int res = select(listen_fd + 1, &rfds, NULL, NULL, NULL);
        if(res > 0){
            if(FD_ISSET(listen_fd, &rfds)){
                struct sockaddr_in client;
                socklen_t addrlen = sizeof(client);
                int client_fd = accept(listen_fd, (struct sockaddr *)&client, &addrlen);

                int pid = fork();
                if( pid < 0){
                    debug("%s:%d, fork failed.", inet_ntoa(client.sin_addr), client.sin_port);
                    break;
                }else if(pid == 0){
                    // �����ں˱������������ ����50��fd
                    epoll_fd = epoll_create(MAX_EVENT_NUMBER);
                    if(epoll_fd == -1){
                        return;
                    }

                    // ���ں˱������fd
                    add_event(epoll_fd, client_fd, EPOLLIN | EPOLLHUP | EPOLLERR );
                    sinfos[client_fd] = std::make_shared<SOCK_INFO>(client_fd, client.sin_port, client.sin_addr.s_addr);

                    child_loop();
                    return;
                }else{

                    conn("%s:%d connect.", inet_ntoa(client.sin_addr), client.sin_port);
                    close(client_fd);
                }

            }
        }
    }
}

void Server::child_loop()
{
    while(1){
        debug("��һ��epoll...");
        printf("[%d %d]\n", epoll_fd, MAX_EVENT_NUMBER);
        int res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        debug("��һ��epoll����...");
        if (res <= 0)
        {
            debug("epoll failed, %d", res);
            break;
        }
        debug("epoll succ, %d", res);
        loop_once(events, res);
        printf("һ��epoll����\n*******************\n");
    }
}


void Server::loop_once(epoll_event* events, int number) {
    printf("����ѭ��epoll\n");
	for(int i = 0; i < number; i++) {
		int sockfd = events[i].data.fd;
        
        fprintf(debug_log, "fd, %d\n", sockfd);
        // printf("checking %d, total %d, sock%d listen_fd%d\n",i,number,sockfd,listen_fd);
		if(events[i].events & EPOLLIN) {
            printf("����EPOLLIN\n");
            std::shared_ptr<SOCK_INFO> sinfo = sinfos[sockfd];

            // auto readbuf = std::shared_ptr<Readbuf_>(new Readbuf_(sockfd));
            // if((sinfo->parse_req(readbuf)) != 0 ){
            //     printf("����Ƿ��������������\n");
            //     continue;
            // }

            // string type = sinfo->type;
            // json header = sinfo->header;

            // TODO ��ȷ���߼�Ӧ���ǣ��ȶ�ȡtype��type�Ϸ������н��������Ϸ���������������(���߹ر�socket)
            // TODO Ȼ�����json��ִ�в�ͬ�Ĳ���

            string type = sinfo->parse_req_type();
            debug("���Խ���type");
            if (type == "")
            {
                debug("typeΪ�� �����������");
                continue;
            }
            else if(type == "close")
            {
                debug("�Է��ر�socket��typeΪclose�������˳�");
                close_release(sinfo);
                continue;
            }
            else if(m_type.count(type) == 0)
            {
                debug("type�Ƿ� �ر��ͷ�");
                close_release(sinfo);
                continue;
            }

            // ����json��ǰ���� type ���ͺϷ�
            // ��ȡ�ͻ��˵�header
            int len = sinfo->recv_header();
            debug("��ȡlen %d", len);
            if(len == -1)
            {
                debug("json�Ƿ� �����������");
                continue;
            }
            print_json(type, sinfo->buffer, len);
            debug("���Խ���json");

            // �����len������parse_header��������崻�
            json header;
            try{
                header = std::move(sinfo->parse_header(len));
            }catch(json::exception e){
                debug("���յ������json������");
                debug("%s", e.what());
                continue;
            }
            debug("json��ȷ");
            debug("�ͻ�������header%s", header.dump().c_str());
            debug("������header.dump()");
            debug("type %s",type.c_str());
            res_type = m_type[type];
            debug("ӳ��mtype");
            try{
                // ���ݲ�ͬ��type��ִ�в�ͬ�Ĳ���
                if(type == "signup"){
                    signup(header, sinfo);
                }
                else if(type == "login"){
                    login(header, sinfo);
                }
                else if(type == "logout"){
                    logout(header, sinfo);
                }
                else if(type == "setbind"){
                    setbind(header, sinfo);
                }
                else if(type == "disbind"){
                    disbind(header, sinfo);
                }
                else if(type == "getbindid"){
                    getbindid(header, sinfo);
                }
                else if(type == "getdir"){
                    getdir(header, sinfo);
                }
                else if(type == "uploadFile"){
                    uploadFile(header, sinfo);
                }
                else if(type == "uploadChunk"){
                    uploadChunk(header, sinfo);
                }
                else if(type == "downloadFile"){
                    downloadFile(header, sinfo);
                }
                else if(type == "deleteFile"){
                    deleteFile(header, sinfo);
                }
                else if(type == "createDir"){
                    createDir(header, sinfo);
                }
                else if(type == "deleteDir"){
                    deleteDir(header, sinfo);
                }
                else{
                    debug("type��Ӧ�����ͷǷ����ҽ���json�ɹ� �ر��ͷ�");
                    close_release(sinfo);
                }
            }catch(json::exception e)
            {
                debug("%s", e.what());
                close_release(sinfos[sockfd]);
            }
        }
        else if(events[i].events & EPOLLHUP){
            // ��⵽�ͻ��˶Ͽ����ӣ��������ͷŶ�Ӧ��Դ
            debug("����EPOLLHUP");
            close_release(sinfos[sockfd]);
        }
        else if(events[i].events & EPOLLERR){
            debug("����EPOLLERR");
            close_release(sinfos[sockfd]);
        }
    }
}


/**
 * �������ܣ�����[ע��]�¼�
 * ���������{username,password}
 * ����ֵ����
 * ��ϸ���������� {username, password} -> ��Ӧ {error, msg}
 */ 
void Server::signup(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // ��֤�û����Ƿ����
    //  -> ��ռ�ã���֪client
    //  -> δ��ռ�ã���������м��ܣ��������û�
    json res;

    string username = header["username"].get<string>();
    string pwd = header["password"].get<string>();

    bool user_exist = if_user_exist(username);

    // select user where username=username
    if(user_exist){
        res["error"] = 1;
        res["msg"] = "username has been registered";
        sinfo->send_header(res_type, res);
        return;
    }

    string pwd_encoded = std::move(encode(pwd));

    bool error_occur = create_user(username, pwd_encoded);

    if(!error_occur){
        res["error"] = 0;
        res["msg"] = "signup success";
        logger("user: %s signup.", username.c_str());
    }
    else{
        res["error"] = 1;
        res["msg"] = "signup fail for db insert";
    }
    sinfo->send_header(res_type, res);
    return;
}

/**
 * �������ܣ�����[��¼]�¼�
 * ���������{username, password}
 * ����ֵ����
 * ��ϸ���������� {username, password} 
 *             -> ��¼�ɹ�����Ӧ {error, msg, session }
 *             -> ��¼ʧ�ܣ���Ӧ {error, msg }
 */ 
void Server::login(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // ���Խ��е�¼����
    // 1. ɸѡusername �� ���ܺ��pwd�Ƿ���ڶ�Ӧ����
    //   ��������ڣ���ô˵���˺����벻ƥ�䣬����
    // 2. �����������session��д���Ӧ�У���֪
    //    sqlִ�г���Ҳ���и�֪

    string username = header["username"].get<string>();
    string pwd = header["password"].get<string>();

    string pwd_encoded = std::move(encode(pwd));

    json res;
    bool user_exist = if_user_exist(username);
    if(!user_exist){
        res["error"] = 1;
        res["msg"] = "username not exist";
        sinfo->send_header(res_type, res);
        return;
    }

    int uid = get_uid_by_name_pwd(username, pwd_encoded); // select(username,pwd)

    if (uid == -1){
        res["error"] = 1;
        res["msg"] = "username password not match";
        sinfo->send_header(res_type, res);
        return;
    }

    string session = generate_session();

    // TODO ������Ը�д��location
    bool error_occur = write_session(uid, session);

    if(error_occur){
        res["error"] = 1;
        res["msg"] = "error occur when generate session";
    }else{
        res["error"] = 0;
        res["msg"] = "login success";
        res["session"] = session;
        logger("user: %s login", username.c_str());
    }
    sinfo->send_header(res_type, res);
    return;
}


/**
 * �������ܣ�����[�˳���¼]�¼�
 * ���������{session}
 * ����ֵ����
 * ��ϸ���������� {session} 
 *             -> �˳��ɹ�����Ӧ {error, msg }
 *             -> �˳�ʧ�ܣ���Ӧ {error, msg }
 */ 
void Server::logout(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    string session = header["session"].get<string>();

    json res;
    int uid = get_uid_by_session(session);
    string username = get_username_by_uid(uid);

    bool error_occur = destroy_session(session);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "destroy session failed";
    }
    else{
        res["error"] = 0;
        res["msg"] = "log out success";
        logger("user: %s logout", username.c_str());
    }
    // sinfo->send_header(res_type, res);
    close_release(sinfo);
    return;
}

/**
 * �������ܣ�����[�󶨷�����Ŀ¼]�¼�
 * ���������{session}
 * ����ֵ����
 * ��ϸ���������� {session, bindid} 
 *             -> �󶨳ɹ�����Ӧ {error, msg, bindid }
 *             -> ��ʧ�ܣ���Ӧ {error, msg, bindid }
 */ 
void Server::setbind(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    int bindid = header["bindid"].get<int>();

    json res;
    res["bindid"] = bindid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;

    bool error_occur = set_bind_dir(uid, bindid);

    if(error_occur){
        res["error"] = 1;
        res["msg"] = "bind failed";
    }
    else{
        res["error"] = 0;
        res["msg"] = "bind success";
        trans("uid: %d setbind %d.", uid, bindid);
    }
    sinfo->send_header(res_type, res);
    return;
}

/**
 * �������ܣ�����[����󶨷�����Ŀ¼]�¼�
 * ���������{session}
 * ����ֵ����
 * ��ϸ���������� {session} 
 *             -> ���ɹ�����Ӧ {error, msg }
 *             -> ���ʧ�ܣ���Ӧ {error, msg }
 */ 
void Server::disbind(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{

    json res;
    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;

    bool error_occur = disbind_dir(uid);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "disbind failed for unknown reason";
    }else{
        res["error"] = 0;
        res["msg"] = "disbind success";
        trans("uid: %d disbind.", uid);
    }
    sinfo->send_header(res_type, res);
    return;
}

void Server::getbindid(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    json res;
    res["bindid"] = -1;
    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;

    int bindid = get_bindid_by_uid(uid);
    if(bindid == -1 || bindid == 0){
        res["error"] = 1;
        res["bindid"] = bindid;
        res["msg"] = "get bindid failed";
    }else{
        res["error"] = 0;
        res["bindid"] = bindid;
        res["msg"] = "get bindid success";
        trans("uid: %d getbindid.", uid);
    }
    sinfo->send_header(res_type, res);
    return;
}

/**
 * �������ܣ�����[��ȡ���������ļ�Ŀ¼]�¼�
 * ���������{session}
 * ����ֵ����
 * ��ϸ���������� {session} 
 *             -> ��ȡ�ɹ�����Ӧ {error, msg, dir_list }
 *             -> ��ȡʧ�ܣ���Ӧ {error, msg }
 */ 
void Server::getdir(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{

    json res;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;
    
    res["error"] = 0;
    res["msg"] = "get server file_dir success";
    // json file_dir = std::move(get_file_dir(uid, bindid));
    // json file_dir = get_file_dir(uid, bindid);
    res["dir_list"] = get_file_dir(uid, bindid);
    trans("uid: %d get dir.", uid);
    sinfo->send_header(res_type, res);
    return;

}

/**
 * �������ܣ�����[�ϴ��ļ�]�¼�������Ԥ���ж��ܷ��봫
 * ���������{session, queueid, filename, path, md5, size, mtime }
 * ����ֵ��
 * ��ϸ���������� {session, queueid, filename, path, md5, size, mtime } -> ��Ӧ {error, msg, queueid }
 *             -> ���سɹ�����Ӧ {error, msg, queueid }
 *             -> ����ʧ�ܣ���Ӧ {error, msg, queueid, vfile_id }
 * TODO �ϴ�ʧ�ܣ�Ӧ�ø�֪md5��������vid
 */ 
void Server::uploadFile(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    
    int queueid = header["queueid"];
    string filename = header["filename"];
    string path = header["path"];
    string md5 = header["md5"];
    ll size = header["size"].get<ll>();
    int mtime = header["mtime"].get<int>();

    json res;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;


    // ���� �봫��� ����Ҫ���¶�Ӧ���ļ���¼ ���� �����ļ���¼
    
    // ���� md5 �ж��ļ��Ƿ���ڣ��Ƿ���Ҫ�봫
    // TODO �봫���߼�Ӧ���ǣ���pfile���ҵ�ƥ���md5��������֤complete=1���������ô�봫��������Ҫ�ͻ����ϴ�
    // TODO ���pfile��Ӧ�ļ�¼�����ڣ�=> ���pfile��¼�� ���pfile complete=0����ô������Ӽ�¼
    // TODO chunks_info, complete�ֶ� ��Ӧ������pfile���ֶ��У�������Ϊvfile���ֶ�
    // TODO Ȼ���ж�ʵ���ļ��Ƿ���ڣ��������򴴽�������ռ䣬�������ٴ���


    // ���ȴ� pfile ���и���md5�����Ƿ�����ļ����Լ��ļ��Ƿ��ϴ����
    json pinfo = if_pfile_complete(md5);
    int pid = pinfo["pid"].get<int>();
    int complete = pinfo["complete"].get<int>();

    bool error_occur = false;
    if (pid != -1 && complete == 1)
    {
        // ʵ���ļ����ڣ������ϴ���ϣ����Խ����봫
        // refcnt++
        error_occur = increase_pfile_refcnt(pid);
        if(error_occur){
            printf("refcnt++ ʧ��\n");
            res["error"] = 1;
            res["msg"] = "DB operate failed, need to retry";
            sinfo->send_header(res_type, res);
            return;
        }

        res["error"] = 0;
        res["msg"] = "lightning transfer success";
        // �봫�ɹ�
        trans("uid: %d uploadFile (md5:%s) lightning transfer success.", uid, md5.c_str());
    }
    else if (pid != -1 && complete == 0)
    {
        // ʵ���ļ����ڣ�����û���ϴ���ϣ���Ȼ��Ҫ�ϴ�
        // refcnt++
        error_occur = increase_pfile_refcnt(pid);
        if(error_occur){
            printf("refcnt++ ʧ��\n");
            sinfo->send_header(res_type, res);
            return;
        }

        // ��Ҫ�ϴ�
        res["error"] = 4;
        res["msg"] = "need to upload";
        trans("uid: %d uploadFile (md5:%s) exist, but not complete, so still need to upload.", uid, md5.c_str());
    }
    else
    {
        // ʵ���ļ������ڣ���Ҫ�ϴ�

        // ����md5�����ո�ʽ����ʵ�������ļ�����
        // string pname = generate_pname_by_md5(md5);
        string name = get_filename_by_md5(md5);

        // �����ļ�������ռ�
        bool create_file_fail = create_file_allocate_space(name, size);
        if(create_file_fail){
            res["error"] = 1;
            res["msg"] = "server create file failed for short of space";
            sinfo->send_header(res_type, res);
            return;
        }

        // ����chunks��Ϣ
        int total = get_chunks_num(size);
        json chunks_info = generate_chunks_info(size, total);

        // ��pfile������Ӽ�¼
        error_occur = create_pfile(md5, chunks_info.dump(4), total);
        if(create_file_fail){
            res["error"] = 1;
            res["msg"] = "server create file failed for short of space";
            sinfo->send_header(res_type, res);
            return;
        }

        // ��Ҫ�ϴ�
        res["error"] = 4;
        res["msg"] = "need to upload";
        trans("uid: %d uploadFile (md5:%s) not exist, so need to upload.", uid,md5.c_str());
    }

    // ���ˣ�pfile��ά����ϣ����Ҷ�Ӧ�������ļ��ض�����

    // ���濪ʼά��vfile��

    int dirid = get_dirid(uid, bindid, path);
    bool create_dir = false;

    // ���Ŀ¼�����ڣ���ô���𼶴���Ŀ¼
    if (dirid == -1)
    {
        // TODO Ŀ¼�����ڣ���ô�ʹ���Ŀ¼����֤Ŀ¼�Ǵ��ڵ�
        dirid = create_dir_return_dirid(uid, bindid, path);
        create_dir = true;
        if(dirid==-1)
        {
            res["error"] = 1;
            res["msg"] = "create dir failed";
            sinfo->send_header(res_type, res);
            return;
        }
    }

    // ��ȡ�ļ���Ϣ(vid, md5)
    json vinfo = get_vinfo(dirid, filename);
    int vid = vinfo["vid"].get<int>();
    string md5_old = vinfo["md5"].get<string>();

    int total = get_chunks_num(size);

    error_occur = false;

    // ����Ŀ¼������ -> ����Ŀ¼ -> ��Ӽ�¼�� vfile ����
    // �����ļ������� -> ��Ӽ�¼�� vfile ����
    if(create_dir || vid == -1)
    {
        // std::cout << "************ create_vfile ***********" << endl;
        vid = create_vfile_new(dirid, filename, size, md5, mtime);
        error_occur = (vid == -1 ? true : false);
    }
    // �����ļ����� -> ���������ļ�����Ϣ
    else if(md5 != md5_old)
    {
        // �� md5��Ӧ���ļ� refcnt--
        // std::cout << "************ update_vfile_whole ***********" << endl;
        error_occur = update_vfile_whole_new(vid, md5, size, mtime);
        decrease_pfile_refcnt_by_md5(md5_old);
    }
    else
    {
        // �ļ����ڲ���md5 ���� -> ����Ҫ���� vfile
    }

    if(error_occur){
        res["error"] = 1;
        res["msg"] = "create_vfile or update_vfile failed";
    }
    else
    {
        // res["error"] = 4;
        // res["msg"] = "need to upload this file";
        // TODO ������Ӧvid����ƵĲ��ã�Ӧ����md5������vid����һ�β��
        // TODO uploadFile��2��API��uploadFile-res��Ӧ����Ӧvid
        // TODO uploadChunk-req��Ӧ�÷���vid
        res["vfile_id"] = vid;
    }
    sinfo->send_header(res_type, res);
    return;
    
    // string pfile = get_filename_by_md5(md5);
    // bool file_exist = if_file_exist(pfile);
    // // std::cout << "*************" << file_exist << "*************" << endl;
    // if (file_exist)
    // {
    //     // �봫
    //     res["error"] = 0;
    //     res["msg"] = "lightning transfer success";

    //     // ��ȡ�ļ���Ϣ(vid, md5)
    //     json vinfo = get_vinfo(dirid, filename);
    //     int vid = vinfo["vid"].get<int>();
    //     string md5_old = vinfo["md5"].get<string>();

    //     int total = get_chunks_num(size);

    //     bool error_occur = false;

    //     // Ŀ¼������ -> ����Ŀ¼ -> insert ( md5 ��Ӧ���ļ� refcnt++ )-> �봫
    //     // �ļ������� -> insert ( md5 ��Ӧ���ļ� refcnt++ )-> �봫
    //     if(create_dir || vid == -1)
    //     {
    //     // std::cout << "************ create_vfile ***********" << endl;
    //         vid = create_vfile(dirid, filename, size, md5, mtime, "", total, total, 1);
    //         error_occur = (vid == -1 ? true : false);
    //     }
    //     else if(md5 != md5_old)
    //     {
    //     // �ļ����� ���� md5 �仯 -> update ( �� md5��Ӧ���ļ� refcnt--, ��md5��Ӧ�� refcnt++ )-> �봫
    //     // std::cout << "************ update_vfile_whole ***********" << endl;
    //         error_occur = update_vfile_whole(vid, md5, "", size, mtime, total, total, 1);
    //     }
    //     else
    //     {
    //     // �ļ����ڲ���md5 ���� -> ֱ���봫
    //     }

    //     if(error_occur){
    //         res["error"] = 1;
    //         res["msg"] = "DB operate failed, need to retry";
    //     }else{
            
    //     trans("uid: %d uploadFile (md5:%s) lightning transfer success.", uid,md5.c_str());
    //     }
    // }else{

    //     // ��Ҫ�ϴ�
    //     res["error"] = 4;
    //     res["msg"] = "need to upload";

    //     // �����ļ�����ռ� ( pfile: md5��Ӧ�� insert, refcnt = 1 )
    //     // TODO pfile

    //     // ʵ���ļ������ڣ���ô�����ļ�������ռ�
    //     bool create_file_fail = create_file_allocate_space(pfile, size);
    //     if(create_file_fail){
    //         res["error"] = 1;
    //         res["msg"] = "server create file failed for short of space";
    //         sinfo->send_header(res_type, res);
    //         return;
    //     }

    //     // ����chunks��Ϣ
    //     int total = get_chunks_num(size);
    //     json chunks_info = generate_chunks_info(size, total);

    //     // �ж��ļ�����
    //     // TODO �ж�vfile�Ƿ���ڣ���������ڣ���ô��Ӽ�¼��������ڣ���ô���и���

    //     // ��ȡ�����ļ��� vid
    //     int vid = get_vid(dirid, filename);

    //     bool error_occur = false;

    //     // Ŀ¼������ -> ����Ŀ¼ -> insert
    //     // �ļ������� -> insert
    //     if(create_dir || vid == -1)
    //     {
    //         // ��Ӧ��Ŀ¼�в����������ļ�����Ҫ����
    //         // TODO vfile�в�����Ҫ��¼ chunks_info cnt total complete
    //         vid = create_vfile(dirid, filename, size, md5, mtime, chunks_info.dump(4), 0, total, 0);
    //         error_occur = (vid == -1 ? true : false);
    //     }
    //     else
    //     {
    //     // �ļ����� -> update
    //         // ��Ӧ��Ŀ¼�д��������ļ�����Ҫ����
    //         // TODO vfile�в�����Ҫ��¼ chunks_info cnt total complete
    //         error_occur = update_vfile_whole(vid, md5, chunks_info.dump(4), size, mtime, 0, total, 0);
    //     }

    //     if(error_occur){
    //         res["error"] = 1;
    //         res["msg"] = "create_vfile or update_vfile failed";
    //     }
    //     else
    //     {
    //         res["error"] = 4;
    //         res["msg"] = "need to upload this file";
    //         res["vfile_id"] = vid;
    //     }
    //     trans("uid: %d uploadFile (md5:%s) need to upload.", uid,md5.c_str());
    // }


}

/**
 * �������ܣ�����[�ϴ��ļ�chunk]�¼�
 * ���������{session, vfile_id, queueid, chunkid, offset, chunksize}
 * ����ֵ����
 * ��ϸ���������� {session, vfile_id, queueid, chunkid, offset, chunksize} 
 *             -> �ϴ��ɹ�����Ӧ {error, msg, queueid }
 *             -> �ϴ�ʧ�ܣ���Ӧ {error, msg, queueid }
 * TODO Ӧ�ø�֪md5������Ҫvid
 * TODO ��Ҫ���һ��md5��vid��������ʱ
 */ 
void Server::uploadChunk(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // 1. sessionȨ����֤
    int queueid = header["queueid"];
    loff_t offset = header["offset"].get<loff_t>();
    size_t chunksize = header["chunksize"].get<size_t>();
    size_t vfile_id = header["vfile_id"].get<size_t>();
    int chunkid = header["chunkid"];
    debug("�ϴ� chunkid-%d ��", chunkid);

    json res;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;

    // 2. ����vfile_id��ȡ��Ӧ�ļ�����Ϣ5, �ж�chunkid��Ӧ��chunks�Ƿ������
    // TODO ����md5�ҵ�pid�������Ϣ
    string md5 = get_vfile_md5(vfile_id);
    json pfile = get_pfile_info(md5);
    int pid = pfile["pid"].get<int>();
    // json vfile = get_vfile_upload_info(vfile_id);

    string chunks_string = pfile["chunks"].get<string>();
    std::vector<std::vector<int>> chunks = json::parse(chunks_string);
    // std::vector<std::vector<int>> chunks = json::parse(chunks_string).get<std::vector<std::vector<int>>>();
    // json chunks = json::parse(vfile["chunks"].get<string>());
    // std::vector<std::vector<int>> chunks = vfile["chunks"].get <std::vector<std::vector<int>>>();
    int chunk_is_done = chunks[chunkid][2];

    if(chunk_is_done){
        debug("����chunkdid-%d", chunkid);
        // ready chunksize bytes from socket;
        // TODO uploadChunkӦ����2��ͨ��API��1�������ж��Ƿ��б�Ҫ�ϴ�������1�����������ϴ�����
        discard_extra(sinfo->sock, chunksize);

        res["error"] = 0;
        res["msg"] = "this chunk has been uploaded";
        sinfo->send_header(res_type, res);
        debug("����res �������");
        return;
    }

    // ����md5�ҵ���Ӧ���ļ�
    // string md5 = vfile["md5"];
    // string filename = get_filename_by_md5(md5);
    // string md5 = pfile["md5"];
    string filename = get_filename_by_md5(md5);

    debug("д�� chunkid-%d", chunkid);
    // ���д���
    ssize_t bytes = write_to_file(sinfo->sock, filename, offset, chunksize);

    if( bytes == chunksize )
    {
        // is done
        res["error"] = 0;
        res["msg"] = "upload success";

        // ���¶�Ӧ���ļ���¼������ chunks, cnt, complete
        // TODO ����pfile�������Ϣ

        // ���� chunks �ֶ�
        chunks[chunkid][2] = 1;
        // vfile["chunks"] = chunks.dump(4);

        // ���� cnt �ֶ�
        int cnt = pfile["cnt"];
        cnt++;
        // vfile["cnt"] = cnt + 1;

        // ���� complete 
        int total = pfile["total"];
        // vfile["complete"] = (cnt == total ? 1 : 0);

        update_pfile_upload_progress(pid, json(chunks).dump(4), cnt, (cnt == total ? 1 : 0));
        debug("�����ϴ�����");
        // update_vfile_upload_progress(vfile_id, json(chunks).dump(4), cnt, (cnt == total ? 1 : 0));
        trans("uid: %d uploadChunk (md5:%s) chunkid %d success.", uid,md5.c_str(),chunkid);
    }
    else
    {
        string temp = "upload failed, actural upload " + std::to_string(bytes);
        temp += " bytes, should be " + std::to_string(chunksize) + " bytes";
        // write failed
        res["error"] = 1;
        res["msg"] = temp;
        debug("д�� chunkid-%d ʧ��", chunkid);
        trans("uid: %d uploadChunk (md5:%s) chunkid %d failed.", uid,md5.c_str(),chunkid);
    }
    debug("д�� chunkid-%d ��Ӧ", chunkid);
    sinfo->send_header(res_type, res);
    debug("д�� chunkid-%d ��Ӧ���", chunkid);
    return;
}

/**
 * �������ܣ�session��֤
 * ����ֵ��uid
 * ��ϸ������
 * �����������⵽session�����ڣ���ôӦ��ֹͣ���У����򣬴����¼ƾ֤��Ч�����Խ���ҵ�����
 */ 
int Server::checkSession(json& header, json& res, std::shared_ptr<SOCK_INFO> & sinfo)
{
    string session = header["session"];
    int uid = get_uid_by_session(session);
    if (uid == -1)
    {
        res["error"] = 2;
        res["msg"] = "session has been destroyed, user need to login";
        sinfo->send_header(res_type, res);
    }
    return uid;
}

/**
 * �������ܣ���֤�û��Ƿ�󶨷�����Ŀ¼
 * ���������
 * ����ֵ��bindid
 * ��ϸ������
 * �����߼�⵽�û�û�а󶨷�����Ŀ¼����ôӦ��ֹͣ���У����򣬴����û�Ŀ¼�Ѿ��󶨣����Խ���ҵ�����
 */ 
int Server::checkBind(json& header, json& res, int uid, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // ������Ŀ¼����֤
    bool cont = true;
    int bindid = get_bindid_by_uid(uid);
    if(bindid == -1){
        res["error"] = 3;
        res["msg"] = "user not exist";
        sinfo->send_header(res_type, res);
    }
    else if(bindid == 0){
        res["error"] = 3;
        res["msg"] = "user not bind server dir";
        sinfo->send_header(res_type, res);
    }
    return bindid;
}


/**
 * �������ܣ�����[����ĳ��chunk]�¼�
 * ���������{session, md5, queueid, offset, chunksize }
 * ����ֵ��
 * ��ϸ���������� {session, md5, queueid, offset, chunksize } -> ��Ӧ {error, msg, queueid }
 *             -> ���سɹ�����Ӧ {error, msg, queueid } + [binary chunk data]
 *             -> ����ʧ�ܣ���Ӧ {error, msg, queueid }
 */ 
void Server::downloadFile(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    int queueid = header["queueid"];

    json res;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;

    string md5 = header["md5"];
    // string filename = get_pname_by_md5(md5);
    string filename = get_filename_by_md5(md5);
    loff_t offset = header["offset"].get<loff_t>();
    size_t chunksize = header["chunksize"].get<size_t>();

    // ssize_t bytes = send_to_socket(sinfo->sock, filename, offset, chunksize);

    // if( bytes == chunksize) {
        res["error"] = 0;
        res["msg"] = "download success";
    // }else{
    //     string temp = "download failed, actural download " + std::to_string(bytes);
    //     temp += " bytes, should be " + std::to_string(chunksize) + " bytes";
    //     res["error"] = 1;
    //     res["msg"] = temp;
    // }
    sinfo->send_header(res_type, res);
    ssize_t bytes = send_to_socket(sinfo->sock, filename, offset, chunksize);
    printf("���� %s �ļ���...\n", md5.c_str());
    if (bytes == 0 || bytes == -1)
    {
        printf("downFile %s ����!\n", md5.c_str());
        trans("uid: %d downloadChunk (md5:%s) chunk offset %ld failed.", uid, md5.c_str(), offset);
    }
    else
    {
        printf("downFile %s �ɹ�!\n", md5.c_str());
        trans("uid: %d downloadChunk (md5:%s) chunk offset %ld success.", uid, md5.c_str(), offset);
    }
    return;
}

/**
 * �������ܣ�����[ɾ���ļ�]�¼�
 * ���������{session, filename, path, queueid}
 * ����ֵ����
 * ��ϸ���������� {session, filename, path, queueid} 
 *             -> ɾ���ɹ�����Ӧ {error, msg, filename, path, queueid }
 *             -> ɾ��ʧ�ܣ���Ӧ {error, msg, filename, path, queueid }
 * 
 */ 
void Server::deleteFile(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    string filename = header["filename"].get<string>();
    string path = header["path"];
    int queueid = header["queueid"];

    json res;
    res["filename"] = filename;
    res["path"] = path;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;
    
    int dirid = get_dirid(uid, bindid, path);
    if(dirid == -1){
        res["error"] = 1;
        res["msg"] = "parent dir not exist";
        sinfo->send_header(res_type, res);
        return;
    }

    json vinfo = get_vinfo(dirid, filename);
    
    int vid = vinfo["vid"].get<int>();
    string md5 = vinfo["md5"].get<string>();
    if(vid != -1){
        bool error_occur = delete_file(dirid, filename);
        decrease_pfile_refcnt_by_md5(md5);
        if(error_occur){
            res["error"] = 1;
            res["msg"] = "file already has been deleted, delete failed";
        }else{
            res["error"] = 0;
            res["msg"] = "delete file success";
            trans("uid: %d delete file %s success.", uid,filename.c_str());
        }
    }else{
        res["error"] = 1;
        res["msg"] = "file already has been deleted, delete failed";
    }


    sinfo->send_header(res_type, res);
    return;
}

/**
 * �������ܣ�����[�����ļ���]�¼�
 * ���������{session, prefix, dirname, queueid}
 * ����ֵ����
 * ��ϸ���������� {session, prefix, dirname, queueid} 
 *             -> ɾ���ɹ�����Ӧ {error, msg, prefix, dirname, queueid }
 *             -> ɾ��ʧ�ܣ���Ӧ {error, msg, prefix, dirname, queueid }
 * 1. ��֤session�Ƿ����
 * 2. ���� uid ��ȡ bindid
 * 3. �� dir ���У�����¼�¼
 */ 
void Server::createDir(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    string prefix = header["prefix"];
    string dirname = header["dirname"];
    int queueid = header["queueid"];

    json res;
    res["prefix"] = prefix;
    res["dirname"] = dirname;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;

    string path = prefix + dirname;
    int dirid = get_dirid(uid, bindid, path);
    if(dirid != -1){
        res["error"] = 0;
        res["msg"] = "dir has been created";
        sinfo->send_header(res_type, res);
        return;
    }

    bool error_occur = create_dir(uid, bindid, path);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "create dir failed for unknown reason";
    }else{
        res["error"] = 0;
        res["msg"] = "create dir success";
        trans("uid: %d createDir %s success.", uid, path.c_str());
    }
    sinfo->send_header(res_type, res);
    return;
}


/**
 * �������ܣ�����[ɾ���ļ���]�¼�
 * ���������{session, prefix, dirname}
 * ����ֵ����
 * ��ϸ���������� {session, prefix, dirname} 
 *             -> ɾ���ɹ�����Ӧ {error, msg, prefix, dirname }
 *             -> ɾ��ʧ�ܣ���Ӧ {error, msg, prefix, dirname }
 * 
 */ 
void Server::deleteDir(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{

    string prefix = header["prefix"];
    string dirname = header["dirname"];
    int queueid = header["queueid"];

    json res;
    res["prefix"] = prefix;
    res["dirname"] = dirname;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;

    string path = prefix + dirname;
    int dirid = get_dirid(uid, bindid, path);
    if(dirid == -1){
        res["error"] = 0;
        res["msg"] = "dir has been deleted";
        sinfo->send_header(res_type, res);
        return;
    }

    bool error_occur = delete_dir(dirid);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "delete dir failed, dir has been deleted";
    }else{
        res["error"] = 0;
        res["msg"] = "delete dir success";
    }
    sinfo->send_header(res_type, res);
    return;
}

/* �����ӽ��� */
void handler(int sig) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char** argv)
{
    char serv_ip_str[30];   /* ָ��server��ip */
    int serv_port;          /* ָ��server��port */
    Choice options[] = {
        // {"--serverip", req_arg, must_appear, is_string, {.str_val=serv_ip_str}, 0, 0, 0},
        {"--serverport", req_arg, must_appear, is_int, {.int_val=&serv_port}, 0, 65535, 20000},
    };
    my_handle_option(options, 1, argc, argv);

    signal(SIGCHLD, handler); 
    my_daemon(1, 1);

    Server server(serv_port, "0.0.0.0");
    server.Run();
    return 0;
}