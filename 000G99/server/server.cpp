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

/* db operator */
#include "utils/db.hpp"
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

#define conn(...) Server::LOG("CONN", __VA_ARGS__)
#define logger(...) Server::LOG("LOG", __VA_ARGS__)
#define trans(...) Server::LOG("TRANSACION", __VA_ARGS__)

const int listen_Q = 20;
const int MAX_EVENT_NUMBER = 100;

const int BUFFER_SIZE = 1024;

/* level-triggered */
void add_event(int epoll_fd, int fd, int state) {
	epoll_event event;
	event.data.fd = fd;
	event.events = state;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)<0){
        printf("add event failed\n");
    }
	// 设置非阻塞
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

// typedef std::shared_ptr<Readbuf_> Rdbuf_ptr;
struct SOCK_INFO{
    int     sock;               // client fd
    u_short client_port;
    u_int client_ip;
    int buffer_len;             // buffer中的有效字节数量
    u_char buffer[BUFFER_SIZE];
    // string type;
    // json header;
    SOCK_INFO(int _sock, u_short _client_port, u_int _client_ip) : sock(_sock), client_port(_client_port), client_ip(_client_ip), buffer_len(0){};
    int recv_header();          // 从指定的socket中读取一个json到缓冲区
    json parse_header(int);        // 从缓冲区中解析出一个json
    int send_header(string, json&);          // 向指定的socket发送一个json数据
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

    // 首先读取，然后解析出type，也就是本次操作的类型
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

    // 然后读取，并解析出JSON格式的header
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
    // JSON格式的header解析完毕

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
            // 超出等待时间
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
            // 超出等待时间
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


//  *         断开连接返回-2
/**
 * 解析请求类型
 * 首先读取1字节，如果读取=0，说明对端关闭socket，那么返回"close"
 * 否则，读取请求
 * 解析请求并返回
 * !如果超出20字节，那么说明非法请求
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
        read(sock, buffer + i, 1);
        if(buffer[i] == '\n')
        {
            break;
        }
        i++;
        // if(i == 20)
        // {
        //     break;
        // }
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
 * 函数功能：从指定的socket中读取数据到缓冲区，直到读取到\0，表示读取了一个json
 * 返回值：返回读取的字节数量
 *         其他情况返回-1
 * 详细描述：不断从socket中接收数据，每次读取到字符直到\0，说明完整地读取了一个 json 格式的header，函数运行结束
 */ 
int SOCK_INFO::recv_header()
{
    int i = 0;
    int cnt = 0;
    int res = -1;
    while (1)
    {
        read(sock, buffer + i, 1);
        cnt++;
        buffer_len += 1;
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
    // TODO 这里for循环，实际上假设了json的长度小于buffer
    return res;
}

/**
 * 函数功能：从缓冲区中指定的范围(0~len)解析出json格式的header并作为返回值返回
 * 输入参数：len代表缓冲区中'\0'的下标
 * 返回值：
 * 详细描述：解析出json格式的header，将剩余数据向前移动
 */ 
json SOCK_INFO::parse_header(int len)
{
    if(len == -1){
        printf("buffer: %s\n", buffer);
    }
    // auto header = json::parse(buffer, buffer + len + 1);
    json header = json::parse(buffer);
    memset(buffer, 0, sizeof(buffer));
    buffer_len = 0;
    return header;
}

/**
 * 发送一个json+尾零
 */ 
int SOCK_INFO::send_header(string res_type, json& header)
{
    string msg = res_type + header.dump();
    printf("***发送res*****\n");
    printf("%s\n", msg.c_str());
    print_buffer((u_char *)msg.c_str(), msg.size());
    // TODO 这里应该采用writen
    // write有可能没有把数据全部发送
    sendn(msg.c_str(), msg.size() + 1);
    // write(sock, msg.c_str(), msg.size() + 1); // 这里有 +1 多发送了尾零
    return 1;
}

class Server{
private:
    /* 日志 */
    FILE *log;

    /* initial setting */
    // int serv_port;
    // char serv_ip_str[30];
    sockaddr_in serv_addr;

    string db_ip;
    string db_name;
    string db_user;
    string db_pwd;
    string store_path;
    string log_name;
    string realfile_path;

    /* socket相关 */
    epoll_event events[MAX_EVENT_NUMBER];
    int listen_fd;
    int epoll_fd;

    /* 处理多个TCP socket需要维护的一些数据结构 */
    std::unordered_map<int, std::shared_ptr<SOCK_INFO>> sinfos;  /* client_fd -> SOCK_INFO 不能采用数组，因为断开连接之后前面的fd有可能出现缺失 */
    std::unordered_map<string, string> m_type;
    string res_type;

private:
    void readConf();
    string get_filename_by_md5(const string &);
    int checkSession(json &, json &, std::shared_ptr<SOCK_INFO> &);
    int checkBind(json &, json &, int, std::shared_ptr<SOCK_INFO> &);
    void LOG(const char *logLevel, const char *format, ...);

private:
    void close_release(std::shared_ptr<SOCK_INFO> & sinfo);
    void loop_once(epoll_event *events, int number, int listen_fd);
    void download(json &, std::shared_ptr<SOCK_INFO> &);
    void upload(json &, std::shared_ptr<SOCK_INFO> &);

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
    Server(int _s_port, const char* _s_ip)
    {
        readConf();
        log = fopen( log_name.c_str(), "a+");
        if(log==nullptr){
            printf("open log failed!\nlog name %s\n",log_name.c_str());
        }
        /* stdio functions are bufferd IO, so need to close buffer */
        if(setvbuf(log, NULL, _IONBF, 0)!=0){
            printf("close fprintf buffer failed!\n");
        }

        listen_fd = create_server("0.0.0.0", _s_port, true);
        // listen to socket
        int res = listen(listen_fd, listen_Q);
        if ( res < 0){
            perror("[listen failed]");
            exit(EXIT_FAILURE);
        }

        // 创建内核表，设置最大容量 容纳50个fd
        epoll_fd = epoll_create(MAX_EVENT_NUMBER);
        if(epoll_fd == -1){
            exit(-1);
        }

        // 向内核表中添加fd
        add_event(epoll_fd, listen_fd, EPOLLIN);


        // 连接db
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
        // m_type[""] = "Res\n";
        

    }
    ~Server(){
        fclose(log);

        mysql_close(db);
    }
    void Run();
};


void Server::readConf()
{
    string conf_file = "/home/G1752240/u1752240.conf";
    // std::cout << conf_file << endl;
    db_ip = "127.0.0.1";
    db_name = "db1752240";
    db_user = "u1752240";
    db_pwd = "u1752240";
    // printf("ip %s\nname %s\nuser %s\npwd %s\n", db_ip.c_str(), db_name.c_str(), db_user.c_str(), db_pwd.c_str());
    std::fstream in;
    in.open(conf_file, std::ios::in);
    in >> db_ip >> db_name >> db_user >> db_pwd >> store_path;
    in.close();
    log_name = store_path + "/log/u1752240.log";
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


void Server::close_release(std::shared_ptr<SOCK_INFO> & sinfo){
    printf("[关闭 ]\n");
    close(sinfo->sock);
    struct sockaddr_in client;
    client.sin_addr.s_addr = sinfo->client_ip;
    // printf("closing... %s:%d, close serv-fd-%d client-fd-%d\n", inet_ntoa(client.sin_addr), sinfo->client_port, sinfo->server_fd, sinfo->sock);
    
    time_t now = time(0);
    char *dt = ctime(&now);
    conn("%s:%d disconnect.", inet_ntoa(client.sin_addr), sinfo->client_port);
    // int res = fprintf(log, "%s%s:%d disconnect. Total bytes from client: %d, total bytes from server: %d.\n", dt, inet_ntoa(client.sin_addr), sinfo.client_port, sinfo.C2S_total, sinfo.S2C_total);
    remove_event(epoll_fd, sinfo->sock);
    sinfos.erase(sinfo->sock);
}

// 需要完成的
void Server::Run()
{
    // showUser();
    while(1){
        printf("新一轮epoll...\n");
        printf("[%d %d]\n", epoll_fd, MAX_EVENT_NUMBER);
        int ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            printf("epool failure\n");
            break;
        }
        loop_once(events, ret, listen_fd);
        printf("一轮epoll结束\n*******************\n");
    }
}


void Server::loop_once(epoll_event* events, int number, int listen_fd) {
    printf("进入循环epoll\n");
	for(int i = 0; i < number; i++) {
		int sockfd = events[i].data.fd;
        // printf("checking %d, total %d, sock%d listen_fd%d\n",i,number,sockfd,listen_fd);
		if(sockfd == listen_fd) {
            struct sockaddr_in client;
            socklen_t addrlen = sizeof(client);
            int client_fd = accept(listen_fd, (struct sockaddr *)&client, &addrlen);

            // 将新client添加到内核表中
            add_event(epoll_fd, client_fd, EPOLLIN | EPOLLHUP | EPOLLERR );

            // SOCK_INFO sinfo(client_fd, client.sin_port, client.sin_addr.s_addr);
            // sinfos.emplace(client_fd, SOCK_INFO(client_fd, client.sin_port, client.sin_addr.s_addr));
            // sinfos.insert(std::pair<int, SOCK_INFO>(client_fd, {client_fd, client.sin_port, client.sin_addr.s_addr}));
            // sinfos.insert(make_pair(client_fd,{ client_fd, client.sin_port, client.sin_addr.s_addr }))
            // sinfos[client_fd] = SOCK_INFO(client_fd, client.sin_port, client.sin_addr.s_addr);
            sinfos[client_fd] = std::make_shared<SOCK_INFO>(client_fd, client.sin_port, client.sin_addr.s_addr);

            time_t now = time(0);
            char *dt = ctime(&now);
            conn("%s:%d connect.", inet_ntoa(client.sin_addr), client.sin_port);
            printf("%s%s:%d  %d新建连接.\n", dt, inet_ntoa(client.sin_addr), client.sin_port, client_fd);
        } 
        else if(events[i].events & EPOLLIN) {
            printf("进入EPOLLIN\n");
            std::shared_ptr<SOCK_INFO> sinfo = sinfos[sockfd];

            // auto readbuf = std::shared_ptr<Readbuf_>(new Readbuf_(sockfd));
            // if((sinfo->parse_req(readbuf)) != 0 ){
            //     printf("请求非法，跳过这次请求\n");
            //     continue;
            // }

            // string type = sinfo->type;
            // json header = sinfo->header;

            string type = sinfo->parse_req_type();
            if(type == "")
            {
                printf("type非法 跳过这次请求\n");
                continue;
            }
            else if(type == "close")
            {
                close_release(sinfo);
                continue;
            }
            // 读取客户端的header
            int len = sinfo->recv_header();
            if(len == -1)
            {
                printf("json非法 跳过这次请求\n");
                continue;
            }

            print_json(type, sinfo->buffer, len);

            json header;
            try{
                header = std::move(sinfo->parse_header(len));
            }catch(json::exception e){
                printf("接收到错误的json，跳过\n");
                std::cout << e.what() << std::endl;
                continue;
            }
            printf("客户端请求header%s\n", header.dump().c_str());

            std::cout << type << std::endl;
            res_type = m_type[type];
            try{
                // 根据不同的type，执行不同的操作
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
                else if(type == "other"){

                }
            }catch(json::exception e)
            {
                std::cout << e.what() << std::endl;
                close_release(sinfos[sockfd]);
            }
        }
        else if(events[i].events & EPOLLHUP){
            // 侦测到客户端断开连接，服务器释放对应资源
            printf("进入EPOLLHUP\n");
            close_release(sinfos[sockfd]);
        }
        else if(events[i].events & EPOLLERR){
            printf("进入EPOLLERR\n");
            close_release(sinfos[sockfd]);
        }
    }
}


/**
 * 函数功能：处理[注册]事件
 * 输入参数：{username,password}
 * 返回值：空
 * 详细描述：请求 {username, password} -> 响应 {error, msg}
 */ 
void Server::signup(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // 验证用户名是否存在
    //  -> 已占用，告知client
    //  -> 未被占用，对密码进行加密，创建新用户
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
 * 函数功能：处理[登录]事件
 * 输入参数：{username, password}
 * 返回值：空
 * 详细描述：请求 {username, password} 
 *             -> 登录成功，响应 {error, msg, session }
 *             -> 登录失败，响应 {error, msg }
 */ 
void Server::login(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // 尝试进行登录操作
    // 1. 筛选username 和 加密后的pwd是否存在对应的行
    //   如果不存在，那么说明账号密码不匹配，返回
    // 2. 否则，随机生成session，写入对应行，告知
    //    sql执行出错，也进行告知

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

    // TODO 这里可以改写成location
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
 * 函数功能：处理[退出登录]事件
 * 输入参数：{session}
 * 返回值：空
 * 详细描述：请求 {session} 
 *             -> 退出成功，响应 {error, msg }
 *             -> 退出失败，响应 {error, msg }
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
 * 函数功能：处理[绑定服务器目录]事件
 * 输入参数：{session}
 * 返回值：空
 * 详细描述：请求 {session, bindid} 
 *             -> 绑定成功，响应 {error, msg, bindid }
 *             -> 绑定失败，响应 {error, msg, bindid }
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
 * 函数功能：处理[解除绑定服务器目录]事件
 * 输入参数：{session}
 * 返回值：空
 * 详细描述：请求 {session} 
 *             -> 解绑成功，响应 {error, msg }
 *             -> 解绑失败，响应 {error, msg }
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
 * 函数功能：处理[获取服务器的文件目录]事件
 * 输入参数：{session}
 * 返回值：空
 * 详细描述：请求 {session} 
 *             -> 获取成功，响应 {error, msg, dir_list }
 *             -> 获取失败，响应 {error, msg }
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
 * 函数功能：处理[上传文件]事件，用于预先判断能否秒传
 * 输入参数：{session, queueid, filename, path, md5, size, mtime }
 * 返回值：
 * 详细描述：请求 {session, queueid, filename, path, md5, size, mtime } -> 响应 {error, msg, queueid }
 *             -> 下载成功，响应 {error, msg, queueid }
 *             -> 下载失败，响应 {error, msg, queueid, vfile_id }
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

    int dirid = get_dirid(uid, bindid, path);
    bool create_dir = false;
    // 如果目录不存在，那么就逐级创建目录
    if (dirid == -1)
    {
        // TODO 目录不存在，那么就创建目录，保证目录是存在的
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

    // 无论 秒传与否， 都需要更新对应的文件记录 或者 增加文件记录
    
    // 根据 md5 判断文件是否存在，是否需要秒传
    string pfile = get_filename_by_md5(md5);
    bool file_exist = if_file_exist(pfile);
    // std::cout << "*************" << file_exist << "*************" << endl;
    if (file_exist)
    {
        // 秒传
        res["error"] = 0;
        res["msg"] = "lightning transfer success";

        // 获取文件信息(vid, md5)
        json vinfo = get_vinfo(dirid, filename);
        int vid = vinfo["vid"].get<int>();
        string md5_old = vinfo["md5"].get<string>();

        int total = get_chunks_num(size);

        bool error_occur = false;

        // 目录不存在 -> 创建目录 -> insert ( md5 对应的文件 refcnt++ )-> 秒传
        // 文件不存在 -> insert ( md5 对应的文件 refcnt++ )-> 秒传
        if(create_dir || vid == -1)
        {
        // std::cout << "************ create_vfile ***********" << endl;
            vid = create_vfile(dirid, filename, size, md5, mtime, "", total, total, 1);
            error_occur = (vid == -1 ? true : false);
        }
        else if(md5 != md5_old)
        {
        // 文件存在 但是 md5 变化 -> update ( 旧 md5对应的文件 refcnt--, 新md5对应的 refcnt++ )-> 秒传
        // std::cout << "************ update_vfile_whole ***********" << endl;
            error_occur = update_vfile_whole(vid, md5, "", size, mtime, total, total, 1);
        }
        else
        {
        // 文件存在并且md5 不变 -> 直接秒传
        }

        if(error_occur){
            res["error"] = 1;
            res["msg"] = "DB operate failed, need to retry";
        }else{
            
        trans("uid: %d uploadFile (md5:%s) lightning transfer success.", uid,md5.c_str());
        }
    }else{

        // 需要上传
        res["error"] = 4;
        res["msg"] = "need to upload";

        // 创建文件分配空间 ( pfile: md5对应的 insert, refcnt = 1 )
        // TODO pfile

        // 实际文件不存在，那么创建文件并分配空间
        bool create_file_fail = create_file_allocate_space(pfile, size);
        if(create_file_fail){
            res["error"] = 1;
            res["msg"] = "server create file failed for short of space";
            sinfo->send_header(res_type, res);
            return;
        }

        // 生成chunks信息
        int total = get_chunks_num(size);
        json chunks_info = generate_chunks_info(size, total);

        // 判断文件存在

        // 获取虚拟文件的 vid
        int vid = get_vid(dirid, filename);

        bool error_occur = false;

        // 目录不存在 -> 创建目录 -> insert
        // 文件不存在 -> insert
        if(create_dir || vid == -1)
        {
            // 对应的目录中不存在虚拟文件，需要新增
            vid = create_vfile(dirid, filename, size, md5, mtime, chunks_info.dump(4), 0, total, 0);
            error_occur = (vid == -1 ? true : false);
        }
        else
        {
        // 文件存在 -> update
            // 对应的目录中存在虚拟文件，需要更新
            error_occur = update_vfile_whole(vid, md5, chunks_info.dump(4), size, mtime, 0, total, 0);
        }

        if(error_occur){
            res["error"] = 1;
            res["msg"] = "create_vfile or update_vfile failed";
        }
        else
        {
            res["error"] = 4;
            res["msg"] = "need to upload this file";
            res["vfile_id"] = vid;
        }
        trans("uid: %d uploadFile (md5:%s) need to upload.", uid,md5.c_str());
    }
    sinfo->send_header(res_type, res);
    return;
}

/**
 * 函数功能：处理[上传文件chunk]事件
 * 输入参数：{session, vfile_id, queueid, chunkid, offset, chunksize}
 * 返回值：空
 * 详细描述：请求 {session, vfile_id, queueid, chunkid, offset, chunksize} 
 *             -> 上传成功，响应 {error, msg, queueid }
 *             -> 上传失败，响应 {error, msg, queueid }
 * 
 */ 
void Server::uploadChunk(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // 1. session权限验证
    int queueid = header["queueid"];
    loff_t offset = header["offset"].get<loff_t>();
    size_t chunksize = header["chunksize"].get<size_t>();
    size_t vfile_id = header["vfile_id"].get<size_t>();
    int chunkid = header["chunkid"];

    json res;
    res["queueid"] = queueid;

    int uid = checkSession(header, res, sinfo);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid, sinfo);
    if(bindid == 0 || bindid == -1)
        return;

    // 2. 根据vfile_id获取对应文件的信息5, 判断chunkid对应的chunks是否传输完成
    json vfile = get_vfile_upload_info(vfile_id);

    string chunks_string = vfile["chunks"].get<string>();
    std::vector<std::vector<int>> chunks = json::parse(chunks_string);
    // std::vector<std::vector<int>> chunks = json::parse(chunks_string).get<std::vector<std::vector<int>>>();
    // json chunks = json::parse(vfile["chunks"].get<string>());
    // std::vector<std::vector<int>> chunks = vfile["chunks"].get <std::vector<std::vector<int>>>();
    int chunk_is_done = chunks[chunkid][2];

    if(chunk_is_done){
        // ready chunksize bytes from socket;
        discard_extra(sinfo->sock, chunksize);

        res["error"] = 0;
        res["msg"] = "this chunk has been uploaded";
        sinfo->send_header(res_type, res);
        return;
    }

    // 根据md5找到对应的文件
    string md5 = vfile["md5"];
    string filename = get_filename_by_md5(md5);

    // 进行传输
    ssize_t bytes = write_to_file(sinfo->sock, filename, offset, chunksize);

    if( bytes == chunksize )
    {
        // is done
        res["error"] = 0;
        res["msg"] = "upload success";

        // 更新对应的文件记录，更新 chunks, cnt, complete

        // 更新 chunks 字段
        chunks[chunkid][2] = 1;
        // vfile["chunks"] = chunks.dump(4);

        // 更新 cnt 字段
        int cnt = vfile["cnt"];
        cnt++;
        // vfile["cnt"] = cnt + 1;

        // 更新 complete 
        int total = vfile["total"];
        // vfile["complete"] = (cnt == total ? 1 : 0);

        update_vfile_upload_progress(vfile_id, json(chunks).dump(4), cnt, (cnt == total ? 1 : 0));
        trans("uid: %d uploadChunk (md5:%s) chunkid %d success.", uid,md5.c_str(),chunkid);
    }
    else
    {
        string temp = "upload failed, actural upload " + std::to_string(bytes);
        temp += " bytes, should be " + std::to_string(chunksize) + " bytes";
        // write failed
        res["error"] = 1;
        res["msg"] = temp;
        trans("uid: %d uploadChunk (md5:%s) chunkid %d failed.", uid,md5.c_str(),chunkid);
    }
    sinfo->send_header(res_type, res);
    return;
}

/**
 * 函数功能：session验证
 * 返回值：uid
 * 详细描述：
 * 调用者如果检测到session不存在，那么应当停止运行；否则，代表登录凭证有效，可以进行业务操作
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
 * 函数功能：验证用户是否绑定服务器目录
 * 输入参数：
 * 返回值：bindid
 * 详细描述：
 * 调用者检测到用户没有绑定服务器目录，那么应当停止运行；否则，代表用户目录已经绑定，可以进行业务操作
 */ 
int Server::checkBind(json& header, json& res, int uid, std::shared_ptr<SOCK_INFO> & sinfo)
{
    // 服务器目录绑定验证
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
 * 函数功能：处理[下载某个chunk]事件
 * 输入参数：{session, md5, queueid, offset, chunksize }
 * 返回值：
 * 详细描述：请求 {session, md5, queueid, offset, chunksize } -> 响应 {error, msg, queueid }
 *             -> 下载成功，响应 {error, msg, queueid } + [binary chunk data]
 *             -> 下载失败，响应 {error, msg, queueid }
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
    printf("下载 %s 文件中...\n", md5.c_str());
    if (bytes == 0 || bytes == -1)
    {
        printf("downFile %s 出错!\n", md5.c_str());
        trans("uid: %d downloadChunk (md5:%s) chunk offset %ld failed.", uid, md5.c_str(), offset);
    }
    else
    {
        printf("downFile %s 成功!\n", md5.c_str());
        trans("uid: %d downloadChunk (md5:%s) chunk offset %ld success.", uid, md5.c_str(), offset);
    }
    return;
}

/**
 * 函数功能：处理[删除文件]事件
 * 输入参数：{session, filename, path, queueid}
 * 返回值：空
 * 详细描述：请求 {session, filename, path, queueid} 
 *             -> 删除成功，响应 {error, msg, filename, path, queueid }
 *             -> 删除失败，响应 {error, msg, filename, path, queueid }
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

    bool error_occur = delete_file(bindid, filename);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "delete file failed for unknown reason";
    }else{
        res["error"] = 0;
        res["msg"] = "delete file success";
        trans("uid: %d delete file %s success.", uid,filename.c_str());
    }
    sinfo->send_header(res_type, res);
    return;
}

/**
 * 函数功能：处理[创建文件夹]事件
 * 输入参数：{session, prefix, dirname, queueid}
 * 返回值：空
 * 详细描述：请求 {session, prefix, dirname, queueid} 
 *             -> 删除成功，响应 {error, msg, prefix, dirname, queueid }
 *             -> 删除失败，响应 {error, msg, prefix, dirname, queueid }
 * 1. 验证session是否过期
 * 2. 根据 uid 获取 bindid
 * 3. 在 dir 表中，添加新记录
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
 * 函数功能：处理[删除文件夹]事件
 * 输入参数：{session, prefix, dirname}
 * 返回值：空
 * 详细描述：请求 {session, prefix, dirname} 
 *             -> 删除成功，响应 {error, msg, prefix, dirname }
 *             -> 删除失败，响应 {error, msg, prefix, dirname }
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
        res["msg"] = "delete dir failed for unknown reason";
    }else{
        res["error"] = 0;
        res["msg"] = "delete dir success";
    }
    sinfo->send_header(res_type, res);
    return;
}

int main(int argc, char** argv)
{
    char serv_ip_str[30];   /* 指定server的ip */
    int serv_port;          /* 指定server的port */
    Choice options[] = {
        // {"--serverip", req_arg, must_appear, is_string, {.str_val=serv_ip_str}, 0, 0, 0},
        {"--serverport", req_arg, must_appear, is_int, {.int_val=&serv_port}, 0, 65535, 20000},
    };
    my_handle_option(options, 1, argc, argv);

    my_daemon(1, 1);

    Server server(serv_port, "0.0.0.0");
    server.Run();
    return 0;
}