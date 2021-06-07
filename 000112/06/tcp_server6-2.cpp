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
    char clientIP[INET_ADDRSTRLEN];

    int maxfd = sock;
    fd_set readfds;
    fd_set readfds_bak;
    FD_ZERO(&readfds);
    FD_ZERO(&readfds_bak);
    FD_SET(sock, &readfds_bak);
    int clientSock[20] = {0};
    int clientNum = 0;
    timeval tv;
    tv.tv_sec = wd;
    tv.tv_usec = 0;
    char buffer[70000] = {0};
    int cntSend[20] = {0};
    int cntRecv[20] = {0};
    while (1) {
        // struct readfds will be changed after select, need to reset every time
        readfds = readfds_bak;
        maxfd = updateMaxFd(readfds, maxfd);
        
        // begin to select
        int res = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (res == -1) {
            perror("[select failed]");
            exit(EXIT_FAILURE);
        }
        printf("*************************\n");
        printf("[select RV is %d]\n", res);
        printf("[tv.tv_sec = %d, tv.tv_usec = %d]\n", tv.tv_sec, tv.tv_usec);

        if(FD_ISSET(sock,&readfds)){
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
            clientSock[clientNum] = sock_new;
            clientNum++;
            FD_SET(sock_new, &readfds_bak);
        }
        for (int i = 0; i < clientNum;i++){
            int client = clientSock[i];
            bool flag1 = false;
            bool flag2 = false;
            printf("[checking sock-%d]\n", client);
            // 只有超时的时候才会打印
            if (tv.tv_sec == 0 && tv.tv_usec == 0){
                res = send(client, buffer, wn, 0);
                cntSend[i] += res == -1 ? 0 : res;
                printf("[send %d bytes to sock-%d]\n", res, client);
                flag1 = true;
            }
            // 如果就绪，就立即读取
            if(FD_ISSET(client, &readfds)){
                res = recv(client, buffer, rn, 0);
                cntRecv[i] += res == -1 ? 0 : res;
                printf("[recv %d bytes from sock-%d]\n", res, client);
                flag2 = true;
            }
            // 只有当本sock至少发生send或者recv中的一项，才会print
            if (debug && (flag1 || flag2)) {
                printf("[already send %d bytes to sock-%d]\n", cntSend[i], client);
                printf("[already recv %d bytes from sock-%d]\n", cntRecv[i], client);
            }
            // if(flag2&&flag1)
            // printf("[both send and recv]\n");
        }
        // printf("[tv_sec %d usec %d]\n", tv.tv_sec, tv.tv_usec);
        if (tv.tv_sec == 0 && tv.tv_usec == 0){
            tv.tv_sec = wd;
            tv.tv_usec = 0;
            // printf("[set time]\n");
        }
    }

    printf("[connection over]\n");
    return 0;
}