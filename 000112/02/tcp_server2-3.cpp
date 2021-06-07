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
int updateMaxFd(const fd_set& fds, int maxfd) {
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
    char clientIP[INET_ADDRSTRLEN];
    char buffer[200] = {0};

    int maxfd = sock;
    fd_set readfds;
    fd_set readfds_bak;
    FD_ZERO(&readfds);
    FD_ZERO(&readfds_bak);
    FD_SET(sock, &readfds_bak);

    while(1){
        // struct readfds, tv will be changed after select, need to reset every time
        readfds = readfds_bak;
        maxfd = updateMaxFd(readfds, maxfd);
        printf("[selecting maxfd=%d]\n", maxfd);
        
        // begin to select
        int res = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (res == -1) {
            perror("[select failed]");
            exit(EXIT_FAILURE);
        } else if (res == 0) {
            printf("[no socket ready]\n");
            continue;
        }
        printf("[select return values is %d]\n", res);
        printf("[errno is %d]\n", errno);

        // check each socket, and read; if is sock, then call accept
        for (int i = 0; i <= maxfd;i++){
            // not ready to read, just pass
            if(!FD_ISSET(i, &readfds)){
                continue;
            }
            // ready sock
            if(i==sock){
                // accept new connection
                addrlen = sizeof(client);
                int sock_new = accept(sock, (struct sockaddr *)&client, &addrlen);
                if(sock_new == -1){
                    printf("[accept failed, errno is %d]", errno);
                    exit(EXIT_FAILURE);
                }
                // print remote ip and port
                printf("[connected with %s %d]\n", inet_ntop(AF_INET, &client.sin_addr, clientIP, INET_ADDRSTRLEN), ntohs(client.sin_port));
                // set non-block
                set_nonblock(sock_new);
                if(sock_new > maxfd){
                    maxfd = sock_new;
                }
                FD_SET(sock_new, &readfds_bak);
            }else{
                res = recv(i, buffer, 20, 0);
                if(res == -1){
                    perror("[recv failed]");
                    exit(EXIT_FAILURE);
                }
                printf("[recv from sock-%d]\n", i);
                printf("[recv return value is %d]\n", res);
                printf("[errno is %d]\n", errno);
                printf("[connection over]\n");
                close(i);
                printf("[close sock-%d]\n", i);
                FD_CLR(i, &readfds_bak);
            }
        }
    }
    return 0;
}