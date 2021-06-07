#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <stdio.h>
#include <fcntl.h> // set socket non-block
#include "my_etc.h"

int main(int argc, char** argv)
{
    char ip[30];
    int port = -1;
    int myport =-1;
    int rn = 1024;
    int wn = 1024;
    int rd = 0;
    int wd = 0;
    int debug = 0;

    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--ip", req_arg, must_appear, is_string, {.str_val=ip}, 0, 0, 0},
        {"--myport", req_arg, must_appear, is_int, {.int_val=&myport}, 0, 65535, 4000},
        {"--readbyte", req_arg, not_must, is_int, {.int_val=&rn}, 1, 65536, 1024},
        {"--writebyte", req_arg, not_must, is_int, {.int_val=&wn}, 1, 65536, 1024},
        {"--rdelay", req_arg, not_must, is_int, {.int_val=&rd}, 0, 15, 0},
        {"--wdelay", req_arg, not_must, is_int, {.int_val=&wd}, 0, 15, 0},
        {"--debug", no_arg, not_must, is_int, {.int_val=&debug}, 0, 0, 0},
    };
    my_handle_option(options, 8, argc, argv);
    
    int sock[2];
    sockaddr_in serv_addr[2];
    sock[0] = create_client(ip, port, myport, serv_addr[0], true);
    sock[1] = create_client(ip, 2 * port, 2 * myport, serv_addr[1], true);

    // begin to connect
    int res = connect(sock[0], (struct sockaddr *)&serv_addr[0], sizeof(sockaddr_in));
    if(res == 0){
        printf("[connection established]\n");
    }else if(errno!=EINPROGRESS){
        perror("[connection failed]");
        printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }
    res = connect(sock[1], (struct sockaddr *)&serv_addr[1], sizeof(sockaddr_in));
    if(res == 0){
        printf("[connection established]\n");
    }else if(errno!=EINPROGRESS){
        perror("[connection failed]");
        printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }
    // 如果连接没有立即建立，那么只有当errno == EINPROGRESS的时候，才表示连接仍在进行中
    char buffer[70000] = {0}; 
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_SET(sock[0], &rfds);
    FD_SET(sock[1], &rfds);
    FD_SET(sock[0], &wfds);
    FD_SET(sock[1], &wfds);
    int maxfd = sock[0] > sock[1] ? sock[0] : sock[1];
    res = select(maxfd + 1, &rfds, &wfds, NULL, NULL);
    printf("[select return values is %d]\n", res);
    printf("[errno is %d]\n", errno);
    // select超时或者出错，立即返回
    if(res < 0){
        perror("[select failed]");
        exit(EXIT_FAILURE);
    }else if(res == 0){
        printf("[connection timeout]\n");
        exit(-1);
    }else{
        if(!FD_ISSET(sock[0], &rfds) && !FD_ISSET(sock[0], &wfds) && !FD_ISSET(sock[1], &rfds) && !FD_ISSET(sock[1], &wfds)){
            perror("[no event on socket found]");
        }
        int err;
        socklen_t errlen = sizeof(int);
        // 通过getsockopt来获取并清除sock上的错误
        if(getsockopt(sock[0], SOL_SOCKET, SO_ERROR, &err, &errlen) < 0){
            printf("[get sock option failed]\n");
            exit(EXIT_FAILURE);
        } else if (0 != err){ 
        // 错误号如果不是0，那么连接出错
            printf("[error is %d]\n", err);
            close(sock[0]);
            exit(EXIT_FAILURE);
        }
        // 通过getsockopt来获取并清除sock上的错误
        if(getsockopt(sock[1], SOL_SOCKET, SO_ERROR, &err, &errlen) < 0){
            printf("[get sock option failed]\n");
            exit(EXIT_FAILURE);
        } else if (0 != err){ 
        // 错误号如果不是0，那么连接出错
            printf("[error is %d]\n", err);
            close(sock[1]);
            exit(EXIT_FAILURE);
        }
        // 连接成功
        int cntSend[2] = {0};
        int cntRecv[2] = {0};
        fd_set readfds;
        timeval tv;
        tv.tv_sec = wd;
        tv.tv_usec = 0;
        printf("[while loop]\n");
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(sock[0], &readfds);
            FD_SET(sock[1], &readfds);
            maxfd = sock[0] > sock[1] ? sock[0] : sock[1];
            res = select(maxfd + 1, &readfds, NULL, NULL, &tv);
            printf("*************************\n");
            printf("[select RV is %d]\n", res);
            printf("[tv.tv_sec = %d, tv.tv_usec = %d]\n", tv.tv_sec, tv.tv_usec);
            for (int i = 0; i < 2; i++){
                if(tv.tv_sec == 0 && tv.tv_usec == 0){
                    res = send(sock[i], buffer, wn, 0);
                    cntSend[i] += res == -1 ? 0 : res;
                    printf("[send %d bytes to sock-%d]\n", res, sock[i]);
                }
                // 如果就绪，就立即读取
                if(FD_ISSET(sock[i], &readfds)){
                    res = recv(sock[i], buffer, rn, 0);
                    cntRecv[i] += res == -1 ? 0 : res;
                    printf("[recv %d bytes from sock-%d]\n", res, sock[i]);
                }
                if (debug) {
                    printf("[already send %d bytes to sock-%d]\n", cntSend[i], sock[i]);
                    printf("[already recv %d bytes from sock-%d]\n", cntRecv[i], sock[i]);
                }
            }
            if(tv.tv_sec == 0 && tv.tv_usec == 0){
                tv.tv_sec = wd;
            }
        }
        close(sock[0]);
        close(sock[1]);
        printf("[connection closed]\n");
    }
    return 0;
}