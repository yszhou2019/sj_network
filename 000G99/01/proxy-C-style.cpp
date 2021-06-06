#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h> // set socket non-block
#include "my_etc.h"
#include <sys/epoll.h>
#include <cassert>


#include <vector>
#include <unordered_map>
#include <memory>

#include <fstream>

const int listen_Q = 20;
const int MAX_EVENT_NUMBER = 100;

const int C2S_SIZE = 60000;
const int S2C_SIZE = 60000;

sockaddr_in serv_addr;
char serv_ip_str[30];
int serv_port;

struct SOCK_INFO{
    int     sock;               // client fd
    int server_fd;              // server fd
    u_int server_ip;
    u_short server_port;
    u_short client_port;
    u_int client_ip;
    int C2S_len;
    int S2C_len;
    int C2S_total;              // 转发字节数
    int S2C_total;              // 转发字节数
    u_char C2S_buf[C2S_SIZE];
    u_char S2C_buf[S2C_SIZE];
    SOCK_INFO(int _sock, int _serv_fd, u_int _serv_ip, u_short _serv_port, u_short _client_port, u_int _client_ip):\
    sock(_sock), server_fd(_serv_fd), server_ip(_serv_ip), server_port(_serv_port), client_port(_client_port), client_ip(_client_ip),\
    C2S_len(0), S2C_len(0), C2S_total(0), S2C_total(0){};
};

void add_event(int epoll_fd, int fd, int state) {
	epoll_event event;
	event.data.fd = fd;
	event.events = state;
	// ET触发 否则默认LT触发
    bool enable_et = false;
	if(enable_et) {
		event.events |= EPOLLET;
	}
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
        printf("modify event failed\n");
    }
}

void remove_event(int epoll_fd, int fd){
	epoll_event event;
    // event.events = state;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL)<0){
        printf("remove event failed\n");
    }else{
        printf("remove success\n");
    }
}

int accept_client(int listen_fd, struct sockaddr_in& client){
    socklen_t addrlen = sizeof(client);
    int client_fd = accept(listen_fd, (struct sockaddr *)&client, &addrlen);
    printf("[accept return values is %d]\n", client_fd);
    if (client_fd == -1){
        printf("[accept failed, errno is %d]", errno);
        exit(EXIT_FAILURE);
    }
    return client_fd;
}

void except_handler(const char* info, int res){
    printf("[%s return values is %d]\n", info, res);
    if (res == -1){
        printf("[%s failed, errno is %d]", info, errno);
        exit(EXIT_FAILURE);
    }
}

const int is_client = 1;
const int is_server = 2;
std::unordered_map<int, int> status;
std::unordered_map<int, int> mapServerClient; // map server_fd to client_fd
std::unordered_map<int, std::shared_ptr<SOCK_INFO>> sinfos; // 不能采用数组，因为断开连接之后前面的fd有可能出现缺失
// client_fd -> SOCK_INFO

void close_release(int epoll_fd, std::shared_ptr<SOCK_INFO>& sinfo){
    close(sinfo->sock);
    close(sinfo->server_fd);
    status.erase(sinfo->sock);
    status.erase(sinfo->server_fd);
    mapServerClient.erase(sinfo->server_fd);
    struct sockaddr_in client;
    client.sin_addr.s_addr = sinfo->client_ip;
    printf("closing... %s:%d, close serv-fd-%d client-fd-%d\n", inet_ntoa(client.sin_addr), sinfo->client_port, sinfo->server_fd, sinfo->sock);
    remove_event(epoll_fd, sinfo->server_fd);
    remove_event(epoll_fd, sinfo->sock);
    sinfos.erase(sinfo->sock);
}


void lt(epoll_event* events, int number, int epoll_fd, int listen_fd) {
	for(int i = 0; i < number; i++) {
		int sockfd = events[i].data.fd;
        printf("checking %d, total %d, sock%d listen_fd%d\n",i,number,sockfd,listen_fd);
		if(sockfd == listen_fd) {
            // proxy <-> client
            struct sockaddr_in client;
            int client_fd = accept_client(listen_fd, client);
            // 将新client添加到内核表中
            add_event(epoll_fd, client_fd, EPOLLIN );
            
            // proxy <-> server proxy的port随意，不需要指定
            int serv_fd = socket(PF_INET, SOCK_STREAM, 0);
            int res = connect(serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            // 添加到内核表中
            add_event(epoll_fd, serv_fd, EPOLLIN );

            status[serv_fd] = is_server;
            status[client_fd] = is_client;
            mapServerClient[serv_fd] = client_fd;
            sinfos[client_fd] = std::make_shared<SOCK_INFO>(client_fd, serv_fd, serv_addr.sin_addr.s_addr, serv_port, client.sin_port, client.sin_addr.s_addr);
            printf("add new client done, client %s:%d, serv-fd-%d client-fd-%d\n",inet_ntoa(client.sin_addr),client.sin_port,serv_fd,client_fd);
        } else if(status[sockfd] == is_client) {
            printf("sock%d is client\n", sockfd);
            std::shared_ptr<SOCK_INFO> &sinfo = sinfos[sockfd];

            // recv data from client to buffer : C -> S
            if( (events[i].events & EPOLLIN) && (sinfo->C2S_len < C2S_SIZE) ){
                printf("recv data from client\n");
                int len = recv(sinfo->sock, sinfo->C2S_buf + sinfo->C2S_len, C2S_SIZE - sinfo->C2S_len, 0);
                if(len<=0){
                    close_release(epoll_fd, sinfo);
                    continue;
                }
                printf("done\n");
                sinfo->C2S_total += len;
                sinfo->C2S_len += len;
                if(sinfo->C2S_len > 0){
                    modify_event(epoll_fd, sinfo->server_fd, EPOLLIN | EPOLLOUT);
                }
            }

            // forward data from buffer to client : S -> C
            if( (events[i].events & EPOLLOUT) && (sinfos[sockfd]->S2C_len > 0) ){
                printf("send data to client\n");
                int len = send(sinfo->sock, sinfo->S2C_buf, sinfo->S2C_len, 0);
                if(len<=0){
                    close_release(epoll_fd, sinfo);
                    continue;
                }
                printf("done\n");
                sinfo->S2C_len -= len;
                if(sinfo->S2C_len > 0){
                    memmove(sinfo->S2C_buf, sinfo->S2C_buf + len, sinfo->S2C_len);
                }else if(sinfo->S2C_len == 0){
                    /* 取消EPOLLOUT */
                    modify_event(epoll_fd, sinfo->sock, EPOLLIN);
                }
            }
		} else if(status[sockfd] == is_server){
            printf("sock%d is server\n", sockfd);
            int client_fd = mapServerClient[sockfd];
            std::shared_ptr<SOCK_INFO>& sinfo = sinfos[client_fd];

            // recv data from server to buffer : S -> C
            if( (events[i].events & EPOLLIN) && (sinfos[client_fd]->S2C_len < S2C_SIZE) ){
                printf("recv data from server\n");
                int len = recv(sockfd, sinfo->S2C_buf + sinfo->S2C_len, S2C_SIZE - sinfo->S2C_len, 0);
                if(len<=0){
                    close_release(epoll_fd, sinfo);
                    continue;
                }
                printf("done\n");
                sinfo->S2C_total += len;
                sinfo->S2C_len += len;
                if(sinfo->S2C_len > 0){
                    modify_event(epoll_fd, client_fd, EPOLLIN | EPOLLOUT);
                }
            }

            // forward data from buffer to server : C -> S
            if( (events[i].events & EPOLLOUT) && (sinfos[client_fd]->C2S_len > 0) ){
                printf("send data to server\n");
                int len = send(sockfd, sinfo->C2S_buf, sinfo->C2S_len, 0);
                if(len<=0){
                    close_release(epoll_fd, sinfo);
                    continue;
                }
                printf("done\n");
                sinfo->C2S_len -= len;
                if(sinfo->C2S_len > 0){
                    memmove(sinfo->C2S_buf, sinfo->C2S_buf + len, sinfo->C2S_len);
                }else if(sinfo->C2S_len == 0){
                    /* 取消EPOLLOUT */
                    modify_event(epoll_fd, sinfo->server_fd, EPOLLIN);
                }
            }
		}
	}
}

int main(int argc, char** argv)
{
    // 指定server的ip
    // 指定server的port
    // 指定proxy的port
    int proxy_port = -1;
    Choice options[] = {
        {"--proxyport", req_arg, must_appear, is_int, {.int_val=&proxy_port}, 20000, 20009, 20000},
        {"--serverport", req_arg, must_appear, is_int, {.int_val=&serv_port}, 0, 65535, 80},
        {"--serverip", req_arg, must_appear, is_string, {.str_val=serv_ip_str}, 0, 0, 0},
    };
    my_handle_option(options, 3, argc, argv);
    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(serv_port); 
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip_str);

    // proxy <-> client
    int listen_fd = create_server("192.168.1.246", proxy_port, false);
    // listen to socket
    int res = listen(listen_fd, listen_Q);
    if ( res < 0){
        perror("[listen failed]");
        printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }else{
        printf("[listening to port %d]\n", proxy_port);
    }
    
	// 创建event数组
	epoll_event events[MAX_EVENT_NUMBER];
	// 创建内核表，设置最大容量 容纳50个fd
	int epoll_fd = epoll_create(50);
    if(epoll_fd==-1){
        printf("epoll create fail");
        exit(-1);
    }
    // 向内核表中添加fd
	add_event(epoll_fd, listen_fd, EPOLLIN);

    while(1){
		// 等待内核表中发生事件
		int ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        printf("****************************\nepoll wait done: %d\n", ret);
        if (ret < 0)
        {
            printf("epool failure\n");
			break;
        }
        // 事件处理
		lt(events, ret, epoll_fd, listen_fd);
        printf("loop done, begin new epoll_wait\n");
    }

    return 0;
}




// void hand()
// {
//     while(1){
//         epoll_wait();
//         fori {
//             if (events[i].fd == listen_fd){
//                 int client_fd = accept();
//                 int connfd = connet();
//                 map.add(connfd, client);
//                 SOCK_INFO sinfo;
//                 sinfos.insert()
//                 sinfos.push_back(std::move(sinfo))
//                 // SOCK_INFO添加到infos数组中
//                 status[client_fd] = is_client;
//                 status[conn_fd] = is_server;
// }