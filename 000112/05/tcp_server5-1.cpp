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

int main(int argc, char** argv)
{
    int port = -1;
    char ip[30];
    int rn = 1024;
    int wn = 1024;
    int rd = 0;
    int wd = 0;
    int debug = 0;
        Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--ip", req_arg, must_appear, is_string, {.str_val=ip}, 0, 0, 0},
        {"--readbyte", req_arg, not_must, is_int, {.int_val=&rn}, 1, 65536, 1024},
        {"--writebyte", req_arg, not_must, is_int, {.int_val=&wn}, 1, 65536, 1024},
        {"--rdelay", req_arg, not_must, is_int, {.int_val=&rd}, 0, 15, 0},
        {"--wdelay", req_arg, not_must, is_int, {.int_val=&wd}, 0, 15, 0},
        {"--debug", no_arg, not_must, is_int, {.int_val=&debug}, 0, 0, 0},
    };
    my_handle_option(options, 7, argc, argv);
    
    int sock = create_server(ip, port, true);

    // listen to socket
    int ret = listen(sock, 5);
    if ( ret < 0){
        perror("[listen failed]");
        printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }else{
        printf("[listening to port %d]\n",port);
    }
    // accept from socket
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);

    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_SET(sock, &rfds);
    FD_SET(sock, &wfds);
    // select to make sure sock is ready
    int res = select(sock + 1, &rfds, &wfds, NULL, NULL);
    printf("[select return values is %d]\n", res);

    if(res <= 0){
        perror("[select failed]");
        exit(EXIT_FAILURE);
    } 

    printf("[select success]\n");

    // begin to accept
    int sock_new = accept(sock, (struct sockaddr *)&client, &addrlen);
    printf("[accept return values is %d]\n", sock_new);
    if (sock_new == -1){
        printf("[accept failed, errno is %d]", errno);
        exit(EXIT_FAILURE);
    }

    // print remote ip and port
    char clientIP[INET_ADDRSTRLEN];
    printf("[connected with %s %d]\n", inet_ntop(AF_INET, &client.sin_addr, clientIP, INET_ADDRSTRLEN), ntohs(client.sin_port));

    // set non-block
    set_nonblock(sock_new);

    char buffer[70000]={0};
    int cntSend = 0;
    int cntRecv = 0;
    fd_set readfds;
    timeval tv;
    tv.tv_sec = wd;
    tv.tv_usec = 0;
    while(1){
        FD_ZERO(&readfds);
        FD_SET(sock_new, &readfds);
        res = select(sock_new + 1, &readfds, NULL, NULL, &tv);
        printf("*************************\n");
        printf("[select RV is %d]\n", res);
        printf("[tv.tv_sec = %d, tv.tv_usec = %d]\n", tv.tv_sec, tv.tv_usec);
        if(tv.tv_sec == 0 && tv.tv_usec == 0){
            res = send(sock_new, buffer, wn, 0);
            cntSend += res == -1 ? 0 : res;
            printf("[send RV is %d]\n", res);
        }
        if(FD_ISSET(sock_new, &readfds)){
            // 如果就绪，就立即读取
            res = recv(sock_new, buffer, rn, 0);
            cntRecv += res == -1 ? 0 : res;
            printf("[recv RV is %d]\n", res);
        }
        if (debug) {
            printf("[already send %d bytes]\n", cntSend);
            printf("[already recv %d bytes]\n", cntRecv);
        }
        if(tv.tv_sec == 0 && tv.tv_usec == 0){
            tv.tv_sec = wd;
        }
    }
    close(sock_new);
    
    printf("[connection over]\n");
    return 0;
}