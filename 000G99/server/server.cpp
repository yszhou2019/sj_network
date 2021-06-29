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


/* utils */
#include "utils/json.hpp"
#include "utils/log.hpp"
#include "utils/tools.hpp"

/* db operator */
#include "utils/db.hpp"
#include <mysql.h>

/* md5 */
#include <openssl/md5.h>

/* debug */
#include <iostream>


using string = std::string;
using json = nlohmann::json;

#include <ctime>

MYSQL* db;


const int listen_Q = 20;
const int MAX_EVENT_NUMBER = 100;

const int BUFFER_SIZE = 1024;
// const int C2S_SIZE = 1024;
// const int S2C_SIZE = 1024;


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

struct SOCK_INFO{
    int     sock;               // client fd
    u_short client_port;
    u_int client_ip;
    int buffer_len;             // buffer中的有效字节数量
    u_char buffer[BUFFER_SIZE];
    // int C2S_len;
    // int S2C_len;
    // int C2S_total;              // 转发字节数
    // int S2C_total;              // 转发字节数
    // u_char C2S_buf[C2S_SIZE];
    // u_char S2C_buf[S2C_SIZE];
    SOCK_INFO(int _sock, u_short _client_port, u_int _client_ip):\
    sock(_sock), client_port(_client_port), client_ip(_client_ip), buffer_len(0) {};
    int recv_header();          // 从指定的socket中读取一个json到缓冲区
    json parse_header(int);        // 从缓冲区中解析出一个json
    int send_header(json&);          // 向指定的socket发送一个json数据
};

/**
 * 函数功能：从指定的socket中读取数据到缓冲区，直到读取完一个json为止
 * 返回值：一次性读取完毕，返回读取的字节数量
 *         其他情况返回-1
 * 详细描述：不断从socket中接收数据，每次读取到字符'{'，计数器cnt++；每次读取到字符'}'，cnt--
 *         直到cnt=0，说明完整地读取了一个json 格式的header，函数运行结束
 * TODO 如果json格式的header超出了2048字节呢???
 * TODO 直接读取的字节数量>header，也就是说，还读取了部分文件数据到buffer
 */ 
int SOCK_INFO::recv_header()
{
    int i = 0;
    int cnt = 0;
    while (1)
    {
        read(sock, buffer + i, 1);
        buffer_len += 1;
        if(buffer[i]=='{'){
            cnt++;
        }
        else if(buffer[i]=='}'){
            cnt--;
            if(cnt==0)
                return i;
        }
        i++;
        if(i>BUFFER_SIZE)
            break;
    }
    // buffer_len = read(sock, buffer, sizeof(buffer));
    // int cnt = 0;
    // for (int i = 0; i < buffer_len;i++){
    //     if(buffer[i] == '{')
    //         cnt++;
    //     else if (buffer[i] == '}'){
    //         cnt--;
    //         if(cnt == 0){
    //             return i;
    //         }
    //     }
    // }
    // TODO 这里for循环，实际上假设了json的长度小于buffer
    return -1;
}

/**
 * 函数功能：从缓冲区中指定的范围(0~len)解析出json格式的header并作为返回值返回，同时将buffer中的剩余数据向前移动
 * 输入参数：len代表缓冲区中最后一个'}'字符的下标
 * 返回值：
 * 详细描述：解析出json格式的header，将剩余数据向前移动
 */ 
json SOCK_INFO::parse_header(int len)
{
    if(len == -1){
        printf("buffer: %s\n", buffer);
    }
    printf("parse int %d\n", len);
    // auto header = json::parse(buffer);
    auto header = json::parse(buffer, buffer + len + 1);
    // memmove(buffer, buffer + len + 1, buffer_len - len - 1);
    // buffer_len -= (len + 1);
    buffer_len = 0;
    return header;
}

// 向socket发送一个json header
// easy，整理好json之后，直接发送msg即可
int SOCK_INFO::send_header(json& header)
{
    string msg = header.dump();
    // TODO 这里应该采用writen
    write(sock, msg.c_str(), msg.size() + 1);
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

    /* socket相关 */
    epoll_event events[MAX_EVENT_NUMBER];
    int listen_fd;
    int epoll_fd;

    /* 处理多个TCP socket需要维护的一些数据结构 */
    std::unordered_map<int, std::shared_ptr<SOCK_INFO>> sinfos;  /* client_fd -> SOCK_INFO 不能采用数组，因为断开连接之后前面的fd有可能出现缺失 */

    

private:
    int checkSession(json &, json &);
    int checkBind(json &, json &, int);

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
        log = fopen("u1752240.log", "a+");
        if(log==nullptr){
            printf("open log failed!\n");
        }
        /* stdio functions are bufferd IO, so need to close buffer */
        if(setvbuf(log, NULL, _IONBF, 0)!=0){
            printf("close fprintf buffer failed!\n");
        }

        // strcpy(serv_ip_str, _s_ip);
        // memset(&serv_addr, 0, sizeof(serv_addr));
        // serv_addr.sin_family = AF_INET; 
        // serv_addr.sin_port = htons(serv_port);
        // serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        // serv_addr.sin_addr.s_addr = inet_addr(serv_ip_str);


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

        if (mysql_real_connect(db,"47.102.201.228","root", "root123","netdrive", 0, NULL, 0)==NULL) {
            printf("mysql_real_connect failed(%s)\n", mysql_error(db));
        }

        mysql_set_character_set(db, "gbk"); 

    }
    ~Server(){
        fclose(log);

        mysql_close(db);
    }
    void Run();
};



void Server::close_release(std::shared_ptr<SOCK_INFO> & sinfo){
    printf("[关闭 ]\n");
    close(sinfo->sock);
    struct sockaddr_in client;
    client.sin_addr.s_addr = sinfo->client_ip;
    // printf("closing... %s:%d, close serv-fd-%d client-fd-%d\n", inet_ntoa(client.sin_addr), sinfo->client_port, sinfo->server_fd, sinfo->sock);
    
    time_t now = time(0);
    char *dt = ctime(&now);
    int res = fprintf(log, "%s%s:%d disconnect.\n", dt, inet_ntoa(client.sin_addr), sinfo->client_port);
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
    printf("进入循环epoll");
	for(int i = 0; i < number; i++) {
		int sockfd = events[i].data.fd;
        // printf("checking %d, total %d, sock%d listen_fd%d\n",i,number,sockfd,listen_fd);
		if(sockfd == listen_fd) {
            struct sockaddr_in client;
            socklen_t addrlen = sizeof(client);
            int client_fd = accept(listen_fd, (struct sockaddr *)&client, &addrlen);

            // 将新client添加到内核表中
            add_event(epoll_fd, client_fd, EPOLLIN | EPOLLHUP );

            // SOCK_INFO sinfo(client_fd, client.sin_port, client.sin_addr.s_addr);
            // sinfos.emplace(client_fd, SOCK_INFO(client_fd, client.sin_port, client.sin_addr.s_addr));
            // sinfos.insert(std::pair<int, SOCK_INFO>(client_fd, {client_fd, client.sin_port, client.sin_addr.s_addr}));
            // sinfos.insert(make_pair(client_fd,{ client_fd, client.sin_port, client.sin_addr.s_addr }))
            // sinfos[client_fd] = SOCK_INFO(client_fd, client.sin_port, client.sin_addr.s_addr);
            sinfos[client_fd] = std::make_shared<SOCK_INFO>(client_fd, client.sin_port, client.sin_addr.s_addr);

            time_t now = time(0);
            char *dt = ctime(&now);
            fprintf(log, "%s%s:%d connect.\n", dt, inet_ntoa(client.sin_addr), client.sin_port);
            printf("%s%s:%d  %d新建连接.\n", dt, inet_ntoa(client.sin_addr), client.sin_port, client_fd);
        } 
        else if(events[i].events & EPOLLIN) {
            printf("客户端发起请求");
            std::shared_ptr<SOCK_INFO> sinfo = sinfos[sockfd];

            // 读取客户端的header
            int len = sinfo->recv_header();
            if(len == -1)
            {
                printf("跳过这次请求\n");
                continue;
            }
            json header = std::move(sinfo->parse_header(len));
            printf("客户端请求header%s\n", header.dump().c_str());

            if(!header.contains("type")){
                printf("跳过这次请求\n");
                continue;
            }
            // 根据不同的type，执行不同的操作
            string type = header["type"].get<string>();
            if(type == "upload"){
                // client -> file -> server
                printf("进入 upload \n");
                upload(header, sinfo);
            }
            else if(type == "download"){
                // server -> json_header -> client 
                // server -> file_data -> client
                printf("进入 download \n");
                download(header, sinfo);
            }


            else if(type == "signup"){
                printf("进入 signup \n");
                signup(header, sinfo);
            }
            else if(type == "login"){
                printf("进入 login \n");
                login(header, sinfo);
            }
            else if(type == "logout"){
                printf("进入 logout \n");
                logout(header, sinfo);
            }
            else if(type == "setbind"){
                printf("进入 setbind \n");
                setbind(header, sinfo);
            }
            else if(type == "disbind"){
                printf("进入 disbind \n");
                disbind(header, sinfo);
            }
            else if(type == "getdir"){
                printf("进入 getdir \n");
                getdir(header, sinfo);
            }
            else if(type == "uploadFile"){
                printf("进入 uploadFile \n");
                uploadFile(header, sinfo);
            }
            else if(type == "uploadChunk"){
                printf("进入 uploadChunk \n");
                uploadChunk(header, sinfo);
            }
            else if(type == "downloadFile"){
                printf("进入 downloadFile \n");
                downloadFile(header, sinfo);
            }
            else if(type == "deleteFile"){
                printf("进入 deleteFile \n");
                deleteFile(header, sinfo);
            }
            else if(type == "createDir"){
                printf("进入 createDir \n");
                createDir(header, sinfo);
            }
            else if(type == "deleteDir"){
                printf("进入 deleteDir \n");
                deleteDir(header, sinfo);
            }
            else if(type == "other"){

            }
        }
        else if(events[i].events & EPOLLHUP){
            // 侦测到客户端断开连接，服务器释放对应资源
            printf("断开连接\n");
            close_release(sinfos[sockfd]);
        }
    }
}




// TODO 是该用readn writen呢，还是说应该像http server那样，epoll_wait呢
// 回答，应该采用readn和writen，因为本身在loop前执行过epoll_wait了，而且不同的操作类型是不一样的，不能像http server那样

int        /* Read "n" bytes from a descriptor */
readn(int fd, u_char *ptr, int n)
{
    int     nleft;
    int    nread;

    nleft = n;
    while (nleft > 0)
    {
        if ((nread = read(fd, ptr, nleft)) < 0)
        {
            if (nleft == n)
                return(-1);    /* error, return -1 */
            else
                break;        /* error, return amount read so far */
        }
        else if (nread == 0)
        {
            break;            /* EOF */
        }
        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);             /* return >= 0 */
}

int        /* Write "n" bytes to a descriptor */
writen(int fd, const u_char *ptr, int n)
{
    int    nleft;
    int   nwritten;

    nleft = n;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) < 0)
        {
            if (nleft == n)
                return(-1);    /* error, return -1 */
            else
                break;        /* error, return amount written so far */
        }
        else if (nwritten == 0)
        {
            break;            
        }
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n - nleft);    /* return >= 0 */
}


/**
 * ! deprecated
 * 函数功能：处理[上传文件]事件，用于预先判断能否秒传
 * 输入参数：{session, queueid, filename, path, md5, size, mtime }
 * 返回值：
 * 详细描述：请求 {session, queueid, filename, path, md5, size, mtime } -> 响应 {error, msg, queueid }
 *             -> 下载成功，响应 {error, msg, queueid }
 *             -> 下载失败，响应 {error, msg, queueid, vfile_id }
 */ 
void Server::upload(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    int size = header["size"].get<int>();
    string filename = header["filename"].get<string>();
    string md5 = header["md5"];


}

/**
 * ! deprecated
 * 函数功能：处理[下载某个chunk]事件
 * 输入参数：{session, md5, queueid, offset, chunksize }
 * 返回值：
 * 详细描述：请求 {session, md5, queueid, offset, chunksize } -> 响应 {error, msg, queueid }
 *             -> 下载成功，响应 {error, msg, queueid } + [binary chunk data]
 *             -> 下载失败，响应 {error, msg, queueid }
 */ 
void Server::download(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    string filename = header["filename"].get<string>();
    int fd = open(filename.c_str(), O_RDWR);
    struct stat stat_buf;
    fstat(fd, &stat_buf);

    /* 发送res_header给client */
    json res_header;
    res_header["size"] = stat_buf.st_size;
    sinfo->send_header(res_header);

    /* 向client发送真正的数据文件 */
    sendfile(sinfo->sock, fd, NULL, stat_buf.st_size);

    // TODO 这里有隐患，sendfile()失败怎么办?write()失败怎么办?
    char buffer = '#';
    write(sinfo->sock, &buffer, 1);
    close(fd);
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
        res["msg"] = "用户名已被注册";
        send();
        return;
    }

    string pwd_encoded = std::move(encode(pwd));

    bool error_occur = create_user(username, pwd_encoded);

    if(!error_occur){
        res["error"] = 0;
        res["msg"] = "注册成功";
    }
    else{
        res["error"] = 1;
        res["msg"] = "注册用户失败(数据库添加失败)";
    }
    send();
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

    int uid = get_uid_by_name_pwd(username, pwd_encoded); // select(username,pwd)

    json res;
    if (uid == -1){
        res["error"] = 1;
        res["msg"] = "账号密码不匹配";
        send();
        return;
    }

    string session = generate_session();
    
    // TODO 这里可以改写成location
    bool error_occur = write_session(uid, session);

    if(error_occur){
        res["error"] = 1;
        res["msg"] = "生成session过程中出现错误";
    }else{
        res["error"] = 0;
        res["msg"] = "登录成功";
        res["session"] = "";
    }
    send();
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

    int uid = get_uid_by_session(session);

    if(uid == -1){
        res["error"] = 0;
        res["msg"] = "session已经被销毁";
    }

    json res;
    bool error_occur = destroy_session(session);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "销毁session失败";
    }
    else if(updated_rows == 1){
        res["error"] = 0;
        res["msg"] = "用户成功退出";
    }
    send();
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
    string session = header["session"].get<string>();
    int bindid = header["bindid"].get<int>();

    // 首先，session生成的时候，应当保证检测到session应当只有1份，
    // 如果session存在，那么返回true
    json res;
    res["bindid"] = bindid;

    int uid = checkSession(header, res);
    if(uid == -1)
        return;

    bool error_occur = set_bind_dir(uid, bindid);

    if(error_occur){
        res["error"] = 1;
        res["msg"] = "绑定操作执行失败";
    }
    else{
        res["error"] = 0;
        res["msg"] = "绑定成功";
    }
    send();
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
    int uid = checkSession(header, res);
    if(uid == -1)
        return;

    bool error_occur = disbind_dir(uid);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "解绑失败，发生未知原因";
    }else{
        res["error"] = 0;
        res["msg"] = "解绑成功";
    }
    send();
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

    // string session = header["session"].get<string>();

    // int uid = get_uid_by_session(session);

    // json res;
    // res["bindid"] = bindid;

    // if(uid == -1){
    //     res["error"] = 2;
    //     res["msg"] = "获取服务器的文件目录失败，用户session已被销毁，客户端需要重新登录";
    //     send();
    //     return;
    // }

    // res["error"] = 0;
    // res["msg"] = "获取服务器的文件目录成功";
    // json file_dir = std::move(get_file_dir(uid));
    // res["dir_list"] = std::move(file_dir);
    // send();
    // return;

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
    off_t size = header["size"].get<off_t>();
    int mtime = header["mtime"].get<int>();

    json res;
    res["queueid"] = queueid;

    int uid = checkSession(header, res);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid);
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
            res["msg"] = "创建目录失败";
            send();
            return;
        }
    }

    // 无论 秒传与否， 都需要更新对应的文件记录 或者 增加文件记录
    
    // 根据 md5 判断文件是否存在，是否需要秒传
    string pfile = get_filename_by_md5(md5);
    bool file_exist = if_file_exist(md5);
    if(file_exist)
    {
        // 秒传
        res["error"] = 0;
        res["msg"] = "秒传成功";

        // 获取文件信息(vid, md5)
        json vinfo = get_vinfo(dirid, filename);
        int vid = vinfo["vfile_id"];
        string md5_old = vinfo["md5"];

        int total = get_chunks_num(size);

        bool error_occur = false;

        // 目录不存在 -> 创建目录 -> insert ( md5 对应的文件 refcnt++ )-> 秒传
        // 文件不存在 -> insert ( md5 对应的文件 refcnt++ )-> 秒传
        if(create_dir || vid == -1)
        {
            vid = create_vfile(dirid, filename, size, md5, mtime, "", total, total, 1);
            error_occur = (vid == -1 ? true : false);
        }
        else if(md5 != md5_old)
        {
        // 文件存在 但是 md5 变化 -> update ( 旧 md5对应的文件 refcnt--, 新md5对应的 refcnt++ )-> 秒传
            error_occur = update_vfile_whole(vid, md5, "", size, mtime, total, total, 1);
        }
        else
        {
        // 文件存在并且md5 不变 -> 直接秒传
        }

        if(error_occur){
            res["error"] = 1;
            res["msg"] = "db操作失败，需要retry";
        }
    }else{

        // 需要上传
        res["error"] = 4;
        res["msg"] = "需要上传";

        // 创建文件分配空间 ( pfile: md5对应的 insert, refcnt = 1 )
        // TODO pfile

        // 实际文件不存在，那么创建文件并分配空间
        bool create_file_fail = create_file_allocate_space(pfile, size);
        if(create_file_fail){
            res["error"] = 1;
            res["msg"] = "服务器创建实际文件失败，可能空间不足";
            send();
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
            res["msg"] = "create_vfile or update_vfile 失败";
        }
        else
        {
            res["error"] = 4;
            res["msg"] = "需要上传文件";
            res["vfile_id"] = vid;
        }
    }
    send();
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

    int uid = checkSession(header, res);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid);
    if(bindid == 0 || bindid == -1)
        return;

    // 2. 根据vfile_id获取对应文件的信息5, 判断chunkid对应的chunks是否传输完成
    json vfile = get_info_by_vfileid(vfile_id);

    json chunks = vfile["chunks"].parse();


    int chunk_is_done = chunks[chunkid][2];

    if(chunk_is_done){
        // TODO
        // ready chunksize bytes from socket;
        u_char buffer[2048];
        readn(sinfo->sock, buffer, chunksize);

        res["error"] = 0;
        res["msg"] = "该chunk已上传过";
        send();
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
        res["msg"] = "上传成功";

        // 更新对应的文件记录，更新 chunks, cnt, complete

        // 更新 chunks 字段
        chunks[chunkid][2] = 1;
        // vfile["chunks"] = chunks.dump(4);

        // 更新 cnt 字段
        int cnt = vfile["cnt"];
        // vfile["cnt"] = cnt + 1;

        // 更新 complete 
        int total = vfile["total"];
        // vfile["complete"] = (cnt == total ? 1 : 0);

        update_vfile_upload_progress(vfile_id, chunks.dump(4), cnt + 1, total, (cnt == total ? 1 : 0));

    }
    else
    {
        string temp = "上传失败，实际上传 " + bytes + " 字节，应该上传 " + chunksize + " 字节";
        // write failed
        res["error"] = 1;
        res["msg"] = temp;
    }
    send();
    return;
}

/**
 * 函数功能：session验证
 * 返回值：uid
 * 详细描述：
 *    caller 如果检测到uid == -1，那么应当结束hanlder()
 *           uid != -1，权限验证成功
 */ 
int Server::checkSession(json& header, json& res)
{
    string session = header["session"];
    int uid = get_uid_by_session(session);
    if (uid == -1)
    {
        res["error"] = 2;
        res["msg"] = "用户session已被销毁，客户端需要重新登录";
        send();
    }
    return uid;
}

/**
 * 传递进去 res
 * 返回 返回true -> continue
 *      返回false -> return
 * 返回值：bindid
 * 详细描述：
 *    caller 检测到 bindid == 0 || bindid == -1，那么应当结束handler()
 *           bindid !=0 且 bindid !=1，代表用户目录已经绑定，可以进行业务操作
 */ 
int Server::checkBind(json& header, json& res, int uid)
{
    // 服务器目录绑定验证
    bool cont = true;
    int bindid = get_bindid_by_uid(uid);
    if(bindid == -1){
        res["error"] = 3;
        res["msg"] = "用户不存在";
        send();
    }
    else if(bindid == 0){
        res["error"] = 3;
        res["msg"] = "尚未绑定服务器目录";
        send();
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

    int uid = checkSession(header, res);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid);
    if(bindid == 0 || bindid == -1)
        return;

    string md5 = header["md5"];
    string filename = get_filename_by_md5(md5);
    loff_t offset = header["offset"].get<loff_t>();
    size_t chunksize = header["chunksize"].get<size_t>();

    ssize_t bytes = send_to_socket(sinfo->sock, filename, offset, chunksize);

    if( bytes == chunksize) {
        res["error"] = 0;
        res["msg"] = "下载成功";
    }else{
        string temp = "下载失败, 实际下载 " + bytes + " 字节，应该下载 " + chunksize + " 字节";
        res["error"] = 1;
        res["msg"] = temp;
    }
    send();
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

    int uid = checkSession(header, res);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid);
    if(bindid == 0 || bindid == -1)
        return;
    
    int dirid = get_dirid(uid, bindid, path);
    if(dirid == -1){
        res["error"] = 1;
        res["msg"] = "对应的目录不存在";
        send();
        return;
    }

    bool error_occur = delete_file(bindid, filename);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "删除文件失败，发生未知原因";
    }else{
        res["error"] = 0;
        res["msg"] = "删除文件成功";
    }
    send();
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
    string session = header["session"];

    int uid = get_uid_by_session(session);

    json res;
    res["prefix"] = prefix;
    res["dirname"] = dirname;
    res["queueid"] = queueid;

    int uid = checkSession(header, res);
    if(uid == -1)
        return;
    int bindid = checkBind(header, res, uid);
    if(bindid == 0 || bindid == -1)
        return;

    string path = prefix + dirname;
    int dirid = get_dirid(uid, bindid, path);
    if(dirid != -1){
        res["error"] = 0;
        res["msg"] = "目录已经创建";
        send();
        return;
    }

    bool error_occur = create_dir(uid, bindid, path);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "创建目录失败，发生未知原因";
    }else{
        res["error"] = 0;
        res["msg"] = "创建目录成功";
    }
    send();
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
    string session = header["session"];

    int uid = get_uid_by_session(session);

    json res;
    res["prefix"] = prefix;
    res["dirname"] = dirname;
    res["queueid"] = queueid;

    if(uid == -1){
        res["error"] = 2;
        res["msg"] = "用户session已被销毁，客户端需要重新登录";
        send();
        return;
    }

    int bindid = get_bindid_by_uid(uid);
    if (bindid == 0)
    {
        res["error"] = 3;
        res["msg"] = "尚未设置绑定目录";
        send();
        return;
    }
    else if(bindid == -1){
        res["error"] = 1;
        res["msg"] = "用户不存在!";
        send();
        return;
    }

    string path = prefix + dirname;
    int dirid = get_dirid(uid, bindid, path);
    if(dirid == -1){
        res["error"] = 0;
        res["msg"] = "目录已经删除";
        send();
        return;
    }

    bool error_occur = delete_dir(dirid);
    if(error_occur){
        res["error"] = 1;
        res["msg"] = "删除目录失败，发生未知原因";
    }else{
        res["error"] = 0;
        res["msg"] = "删除目录成功";
    }
    send();
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

    // my_daemon(1, 1);

    Server server(serv_port, "0.0.0.0");
    server.Run();
    return 0;
}