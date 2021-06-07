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
    char ip[30];
    int port = -1;
    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--ip", req_arg, must_appear, is_string, {.str_val=ip}, 0, 0, 0},
    };
    my_handle_option(options, 2, argc, argv);

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
    // first time select, before accept
    int res = select(sock + 1, &rfds, &wfds, NULL, NULL);
    printf("[before accept, select return values is %d]\n", res);

    // begin to accept
    int sock_new = accept(sock, (struct sockaddr *)&client, &addrlen);
    printf("[accept return values is %d]\n", sock_new);
    if (sock_new == -1){
        printf("[accept failed, errno is %d]", errno);
        exit(EXIT_FAILURE);
    }

    char buffer[100]={0};
    if(res <= 0){
        perror("[select failed]");
        exit(EXIT_FAILURE);
    }else{   
        printf("[select success]\n");
        // print remote ip and port
        char clientIP[INET_ADDRSTRLEN];
        printf("[connected with %s %d]\n", inet_ntop(AF_INET, &client.sin_addr, clientIP, INET_ADDRSTRLEN), ntohs(client.sin_port));

        // set non-block
        set_nonblock(sock_new);

        fd_set readfds;
        fd_set writefds;

        // recv
        while(1){
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_SET(sock_new, &readfds);
            FD_SET(sock_new, &writefds);
            res = select(sock_new + 1, &readfds, &writefds, NULL, NULL);
            printf("*************************\n");
            printf("[select RV is %d]\n", res);
            int err;
            socklen_t errlen = sizeof(int);
            res = getsockopt(sock_new, SOL_SOCKET, SO_ERROR, &err, &errlen);
            printf("[getsockopt RV is %d]\n", res);
            printf("[err is %d]\n", err);
            printf("[errno is %d]\n", errno);
            res = send(sock_new, buffer, 10, 0);
            printf("[send %d bytes]\n", res);
            printf("[errno is %d]\n", errno);
            sleep(1);
        }
        close(sock_new);
        
        printf("[connection over]\n");
    }

    // printf("[recv return code is %d\n",res);
    return 0;
}