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
    
    int sock = create_server(ip, port, false);
    
    // listen to socket
    int ret = listen(sock, 5);
    if ( ret < 0){
        perror("[listen failed]");
        exit(EXIT_FAILURE);
    }else{
        printf("[listening to port %d]\n",port);
    }
    // accept from socket
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client,&addrlen);
    if (connfd < 0){
        printf("[accept failed, errno is %d]",errno);
        exit(EXIT_FAILURE);
    }else{
        // print remote ip and port
        char remote[INET_ADDRSTRLEN];
        printf("[connected with %s %d]\n",inet_ntop(AF_INET,&client.sin_addr,remote,INET_ADDRSTRLEN),ntohs(client.sin_port));

        set_nonblock(connfd);

        char buffer[200] = {0};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(connfd, &rfds);
        int res = select(connfd+1, &rfds, NULL, NULL, NULL);
        printf("[select return values is %d]\n",res);
        printf("[errno is %d]\n", errno);

        res = recv(connfd, buffer, 20, 0);
        printf("[recv return value is %d]\n", res);
        printf("[errno is %d]\n", errno);
    }
    // int res = recv(connfd,buffer,sizeof(buffer),0);
    printf("[connection over]\n");
    close(connfd);
    // printf("[recv return code is %d\n",res);
    return 0;
}