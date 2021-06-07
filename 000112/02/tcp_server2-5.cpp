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

    // recv
    char buffer[200] = {0};
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock_new, &readfds);
    res = select(sock_new + 1, &readfds, NULL, NULL, NULL);
    printf("[select RV is %d]\n", res);
    if(res <= 0){
        exit(EXIT_FAILURE);
    }
    res = recv(sock_new, buffer, 20, 0);
    printf("[recv return value is %d]\n", res);
    printf("[errno is %d]\n", errno);
    close(sock_new);
    
    printf("[connection over]\n");
    return 0;
}