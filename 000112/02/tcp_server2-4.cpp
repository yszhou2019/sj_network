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

// update max fd
int updateMaxFd(fd_set fds, int maxfd) {
    int res = 0;
    for (int i = 0; i <= maxfd; i++) {
        if (FD_ISSET(i, &fds) && i > res) {
            res = i;
        }
    }
    return res;
}

int main(int argc, char** argv)
{
    char ip[30];
    int port = -1;
    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--ip", req_arg, must_appear, is_string, {.str_val=ip}, 0, 0, 0},
    };
    my_handle_option(options, 2, argc, argv);
    
    my_daemon(1, 1);
    int sock = create_server(ip, port, false);
    
    // listen to socket
    int ret = listen(sock, 5);
    if ( ret < 0){
        perror("[listen failed]");
        exit(EXIT_FAILURE);
    }else{
        printf("[listening to port %d]\n",port);
    }

    // client machine information
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    char clientIP[INET_ADDRSTRLEN];

    int maxfd = sock;
    fd_set readfds;
    char buffer[200] = {0};

    int pid;
    while(1){
        // accept from socket
        int sock_new = accept(sock, (struct sockaddr *)&client,&addrlen);
        if (sock_new < 0){
            printf("[accept failed, errno is %d]",errno);
            exit(EXIT_FAILURE);
        }else{
            // print remote ip and port
            printf("[connected with %s %d]\n", inet_ntop(AF_INET, &client.sin_addr, clientIP, INET_ADDRSTRLEN), ntohs(client.sin_port));
            // set non-block
            set_nonblock(sock_new);
            FD_ZERO(&readfds);
            FD_SET(sock_new, &readfds);
            pid = fork();
            if(pid==0){
                int res = select(sock_new + 1, &readfds, NULL, NULL, NULL);
                printf("[select return values is %d]\n",res);
                printf("[errno is %d]\n", errno);
                res = recv(sock_new, buffer, 20, 0);
                printf("[recv return value is %d]\n", res);
                printf("[errno is %d]\n", errno);
                return 0;
            }
        }
    }

    return 0;
}