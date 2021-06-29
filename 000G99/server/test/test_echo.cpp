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

/* STL */
#include <memory>
#include <string>
#include <utility>


/* utils */
#include "../utils/json.hpp"
#include <iostream>	// cin,cout等
#include <iomanip>	// setw等
#include <exception>

using namespace std;

using json = nlohmann::json;


const int listen_Q = 20;
const int MAX_EVENT_NUMBER = 100;

const int BUFFER_SIZE = 1024;
// const int C2S_SIZE = 1024;
// const int S2C_SIZE = 1024;


// /* level-triggered */
// void add_event(int epoll_fd, int fd, int state) {
// 	epoll_event event;
// 	event.data.fd = fd;
// 	event.events = state;
//     if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)<0){
//         printf("add event failed\n");
//     }
// 	// 设置非阻塞
// 	set_nonblock(fd);
// }

// void modify_event(int epoll_fd, int fd, int state){
// 	epoll_event event;
//     event.events = state;
//     event.data.fd = fd;
//     if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event)<0){
//         // printf("modify event failed\n");
//     }
// }

// void remove_event(int epoll_fd, int fd){
// 	epoll_event event;
//     event.data.fd = fd;
//     if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL)<0){
//         // printf("remove event failed\n");
//     }
// }

char buffer[2048];
int sock;

string parse_req_type(int sock)
{
    int i = 0;
    while (1)
    {
        read(sock, buffer + i, 1);
        if(buffer[i] == '\n')
        {
            break;
        }
        i++;
    }
    buffer[i] = '\0';
    return string(buffer);
}

int recv_header(int sock)
{
    int i = 0;
    int cnt = 0;
    while (1)
    {
        read(sock, buffer + i, 1);
        cnt++;
        if(buffer[i]=='\0')
        {
            return i;
        }
        if(cnt == BUFFER_SIZE)
            break;
        i++;
    }
    return -1;
}

json parse_header(int len)
{
    if(len == -1){
        printf("buffer: %s\n", buffer);
    }
    printf("parse int %d\n", len);
    // auto header = json::parse(buffer);
    auto header = json::parse(buffer, buffer + len + 1);
    // memmove(buffer, buffer + len + 1, buffer_len - len - 1);
    // buffer_len -= (len + 1);
    return header;
}

int send_header(json& header)
{
    string msg = header.dump();
    // TODO 这里应该采用writen
    write(sock, msg.c_str(), msg.size() + 1);
    // char temp = '\0';
    // write(sock, &temp, 1);
    return 1;
}


/*
void loop_once(epoll_event* events, int number, int listen_fd)
{
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
            printf("%s:%d  %d新建连接.\n", inet_ntoa(client.sin_addr), client.sin_port, client_fd);
        } 
        else if(events[i].events & EPOLLIN) {
            printf("客户端发起请求");
            std::shared_ptr<SOCK_INFO> sinfo = sinfos[sockfd];

            string type = sinfo->parse_req_type();
            // 读取客户端的header
            int len = sinfo->recv_header();
            if(len == -1)
            {
                printf("跳过这次请求\n");
                continue;
            }
            json header = std::move(sinfo->parse_header(len));
            printf("客户端请求header%s\n", header.dump().c_str());
        }
        else if(events[i].events & EPOLLHUP){
            // 侦测到客户端断开连接，服务器释放对应资源
            printf("断开连接\n");
            close()
        }
}
*/

int main(int argc, char** argv)
{
    cout << "in main" << endl;
    int serv_port; /* 指定server的port */
    Choice options[] = {
        {"--serverport", req_arg, must_appear, is_int, {.int_val=&serv_port}, 0, 65535, 20000},
    };
    cout << "handling" << endl;
    my_handle_option(options, 1, argc, argv);
    int listen_fd = create_server("0.0.0.0", serv_port, true);
    int res = listen(listen_fd, listen_Q);
    if ( res < 0){
        perror("[listen failed]");
        exit(EXIT_FAILURE);
    }
    cout << "running" << endl;
    while (1)
    {
        struct sockaddr_in client;
        socklen_t addrlen = sizeof(client);
        int client_fd = accept(listen_fd, (struct sockaddr *)&client, &addrlen);
        sock = client_fd;
        if (client_fd > 0)
        {
            string type = parse_req_type(client_fd);
            int len = recv_header(client_fd);
            json req;
            try
            {
                req = parse_header(len);
            }
            catch (exception e)
            {
                cout << "parse error, continue" << endl;
            }
            cout << "type:" << type << endl;
            cout << "len:" << len << endl;
            cout << "req:" << req << endl;
            int res = read(client_fd, buffer, 200);
            cout << "read:" << res << " bytes" << endl;
            send_header(req);
            close(client_fd);
        }
    }

    // // 创建内核表，设置最大容量 容纳50个fd
    // epoll_fd = epoll_create(MAX_EVENT_NUMBER);
    // if(epoll_fd == -1){
    //     exit(-1);
    // }
    // add_event(epoll_fd, listen_fd, EPOLLIN);
    // while(1){
    //     printf("新一轮epoll...\n");
    //     printf("[%d %d]\n", epoll_fd, MAX_EVENT_NUMBER);
    //     int ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    //     if (ret < 0)
    //     {
    //         printf("epool failure\n");
    //         break;
    //     }
    //     loop_once(events, ret, listen_fd);
    //     printf("一轮epoll结束\n*******************\n");
    // }
    return 0;   
}