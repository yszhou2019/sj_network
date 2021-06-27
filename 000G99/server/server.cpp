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

/* debug */
#include <iostream>
using string = std::string;
using json = nlohmann::json;

#include <ctime>


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
    int buffer_len;             // buffer�е���Ч�ֽ�����
    u_char buffer[BUFFER_SIZE];
    // int C2S_len;
    // int S2C_len;
    // int C2S_total;              // ת���ֽ���
    // int S2C_total;              // ת���ֽ���
    // u_char C2S_buf[C2S_SIZE];
    // u_char S2C_buf[S2C_SIZE];
    SOCK_INFO(int _sock, u_short _client_port, u_int _client_ip):\
    sock(_sock), client_port(_client_port), client_ip(_client_ip), buffer_len(0) {};
    int recv_header();          // ��ָ����socket�ж�ȡһ��json��������
    json parse_header(int);        // �ӻ������н�����һ��json
    int send_header(json&);          // ��ָ����socket����һ��json����
};

/**
 * �������ܣ���ָ����socket�ж�ȡ���ݵ���������ֱ����ȡ��һ��jsonΪֹ
 * ����ֵ��һ���Զ�ȡ��ϣ����ض�ȡ���ֽ�����
 *         �����������-1
 * ��ϸ���������ϴ�socket�н������ݣ�ÿ�ζ�ȡ���ַ�'{'��������cnt++��ÿ�ζ�ȡ���ַ�'}'��cnt--
 *         ֱ��cnt=0��˵�������ض�ȡ��һ��json ��ʽ��header���������н���
 * TODO ���json��ʽ��header������2048�ֽ���???
 * TODO ֱ�Ӷ�ȡ���ֽ�����>header��Ҳ����˵������ȡ�˲����ļ����ݵ�buffer
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
    // TODO ����forѭ����ʵ���ϼ�����json�ĳ���С��buffer
    return -1;
}

/**
 * �������ܣ��ӻ�������ָ���ķ�Χ(0~len)������json��ʽ��header����Ϊ����ֵ���أ�ͬʱ��buffer�е�ʣ��������ǰ�ƶ�
 * ���������len�������������һ��'}'�ַ����±�
 * ����ֵ��
 * ��ϸ������������json��ʽ��header����ʣ��������ǰ�ƶ�
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

// ��socket����һ��json header
// easy�������json֮��ֱ�ӷ���msg����
int SOCK_INFO::send_header(json& header)
{
    string msg = header.dump();
    // TODO ����Ӧ�ò���writen
    write(sock, msg.c_str(), msg.size() + 1);
    return 1;
}

class Server{
private:
    /* ��־ */
    FILE *log;

    /* initial setting */
    // int serv_port;
    // char serv_ip_str[30];
    sockaddr_in serv_addr;

    /* socket��� */
    epoll_event events[MAX_EVENT_NUMBER];
    int listen_fd;
    int epoll_fd;

    /* ������TCP socket��Ҫά����һЩ���ݽṹ */
    std::unordered_map<int, std::shared_ptr<SOCK_INFO>> sinfos;  /* client_fd -> SOCK_INFO ���ܲ������飬��Ϊ�Ͽ�����֮��ǰ���fd�п��ܳ���ȱʧ */

private:
    void close_release(std::shared_ptr<SOCK_INFO> & sinfo);
    void loop_once(epoll_event *events, int number, int listen_fd);
    void download(json &, std::shared_ptr<SOCK_INFO> &);
    void upload(json &, std::shared_ptr<SOCK_INFO> &);
    

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

        // �����ں˱������������ ����50��fd
        epoll_fd = epoll_create(MAX_EVENT_NUMBER);
        if(epoll_fd == -1){
            exit(-1);
        }

        // ���ں˱������fd
        add_event(epoll_fd, listen_fd, EPOLLIN);
    }
    ~Server(){
        fclose(log);
    }
    void Run();
};



void Server::close_release(std::shared_ptr<SOCK_INFO> & sinfo){
    printf("[�ر� ]\n");
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

// ��Ҫ��ɵ�
void Server::Run()
{
    while(1){
        printf("��һ��epoll...\n");
        printf("[%d %d]\n", epoll_fd, MAX_EVENT_NUMBER);
        int ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            printf("epool failure\n");
            break;
        }
        loop_once(events, ret, listen_fd);
        printf("һ��epoll����\n*******************\n");
    }
}


void Server::loop_once(epoll_event* events, int number, int listen_fd) {
    printf("����ѭ��epoll");
	for(int i = 0; i < number; i++) {
		int sockfd = events[i].data.fd;
        // printf("checking %d, total %d, sock%d listen_fd%d\n",i,number,sockfd,listen_fd);
		if(sockfd == listen_fd) {
            struct sockaddr_in client;
            socklen_t addrlen = sizeof(client);
            int client_fd = accept(listen_fd, (struct sockaddr *)&client, &addrlen);

            // ����client��ӵ��ں˱���
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
            printf("%s%s:%d  %d�½�����.\n", dt, inet_ntoa(client.sin_addr), client.sin_port, client_fd);
        } 
        else if(events[i].events & EPOLLIN) {
            printf("�ͻ��˷�������");
            std::shared_ptr<SOCK_INFO> sinfo = sinfos[sockfd];

            // ��ȡ�ͻ��˵�header
            int len = sinfo->recv_header();
            if(len == -1)
            {
                printf("�����������\n");
                continue;
            }
            json header = std::move(sinfo->parse_header(len));
            printf("�ͻ�������header%s\n", header.dump().c_str());

            if(!header.contains("type")){
                printf("�����������\n");
                continue;
            }
            // ���ݲ�ͬ��type��ִ�в�ͬ�Ĳ���
            string type = header["type"].get<string>();
            if(type == "upload"){
                // client -> file -> server
                printf("����upload\n");
                upload(header, sinfo);
            }
            else if(type == "download"){
                // server -> json_header -> client 
                // server -> file_data -> client
                printf("����download\n");
                download(header, sinfo);
            }
            else if(type == "other"){

            }
        }
        else if(events[i].events & EPOLLHUP){
            // ��⵽�ͻ��˶Ͽ����ӣ��������ͷŶ�Ӧ��Դ
            printf("�Ͽ�����\n");
            close_release(sinfos[sockfd]);
        }
    }
}




// TODO �Ǹ���readn writen�أ�����˵Ӧ����http server������epoll_wait��
// �ش�Ӧ�ò���readn��writen����Ϊ������loopǰִ�й�epoll_wait�ˣ����Ҳ�ͬ�Ĳ��������ǲ�һ���ģ�������http server����

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
 * �������ܣ����ڷ��������տͻ����ϴ����ļ�(�ڴ�֮ǰ�Ѿ���ȡ�˿ͻ��˵�req_header)
 * ���������req_header
 * ����ֵ��
 * ��ϸ������ֻ��Ҫ�����ļ���Ȼ���socket�ж�ȡָ���ֽڵ����ݵ��ļ�������
 * 
 */ 
void Server::upload(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    int size = header["size"].get<int>();
    string filename = header["filename"].get<string>();

    int fd;
    if (access(filename.c_str(), F_OK) == -1)
    {
        if(fd = open(filename.c_str(),O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR)<0){
            fd = creat(filename.c_str(), O_WRONLY);
        }
        fallocate(fd, 0, 0, size);
    }
    else {
        fd = open(filename.c_str(), O_WRONLY | O_CREAT);
    }
    u_char buffer[2048];
    readn(sinfo->sock, buffer, size);
    writen(fd, buffer, size);

    // int res = splice(sinfo->sock, NULL, fd, NULL, size, SPLICE_F_MORE | SPLICE_F_MOVE);
    // int res = sendfile(fd, sinfo->sock, NULL, size);
    // printf("recv done %d\n", res);
    close(fd);
    // ��ʣ���ȡ���������
    while(1)
    {
        read(sinfo->sock, buffer, 1);
        if(buffer[0] == '#')
            break;
    }
    // printf("close done %d\n",res);
}

/**
 * �������ܣ��ͻ���������Դ��������������Դ
 * ���������req_header
 * ����ֵ��
 * ��ϸ������
 * 
 * 1. server -> JSON{ file_size } -> client 
 * 2. server -> file_data -> client
 */ 
void Server::download(json& header, std::shared_ptr<SOCK_INFO> & sinfo)
{
    string filename = header["filename"].get<string>();
    int fd = open(filename.c_str(), O_RDWR);
    struct stat stat_buf;
    fstat(fd, &stat_buf);

    /* ����res_header��client */
    json res_header;
    res_header["size"] = stat_buf.st_size;
    sinfo->send_header(res_header);

    /* ��client���������������ļ� */
    sendfile(sinfo->sock, fd, NULL, stat_buf.st_size);

    // TODO ������������sendfile()ʧ����ô��?write()ʧ����ô��?
    char buffer = '#';
    write(sinfo->sock, &buffer, 1);
    close(fd);
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

    // my_daemon(1, 1);

    Server server(serv_port, "0.0.0.0");
    server.Run();
    return 0;
}