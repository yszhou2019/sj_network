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

    sockaddr_in serv_addr;
    int sock = create_client(ip, port, myport, serv_addr, true);

    // begin to connect
    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
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
    FD_SET(sock, &rfds);
    FD_SET(sock, &wfds);
    res = select(sock + 1, &rfds, &wfds, NULL, NULL);
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
        if(!FD_ISSET(sock, &rfds) && !FD_ISSET(sock, &wfds)){
            perror("[no event on socket found]");
        }
        int err;
        socklen_t errlen = sizeof(int);
        // 通过getsockopt来获取并清除sock上的错误
        if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0){
            printf("[get sock option failed]\n");
            exit(EXIT_FAILURE);
        } else if (0 != err){ 
        // 错误号如果不是0，那么连接出错
            printf("[error is %d]\n", err);
            close(sock);
            exit(EXIT_FAILURE);
        }
        // 连接成功
        int cntSend = 0;
        int cntRecv = 0;
        fd_set readfds;
        timeval tv;
        tv.tv_sec = wd;
        tv.tv_usec = 0;
        while(1){
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            res = select(sock + 1, &readfds, NULL, NULL, &tv);
            printf("*************************\n");
            printf("[select RV is %d]\n", res);
            printf("[tv.tv_sec = %d, tv.tv_usec = %d]\n", tv.tv_sec, tv.tv_usec);
            // 到指定时间则发送数据
            if(tv.tv_sec == 0 && tv.tv_usec == 0){
                res = send(sock, buffer, wn, 0);
                cntSend += res == -1 ? 0 : res;
                printf("[send RV is %d]\n", res);
            }
            // 如果就绪，就立即读取
            if(FD_ISSET(sock, &readfds)){
                res = recv(sock, buffer, rn, 0);
                cntRecv += res == -1 ? 0 : res;
                printf("[recv RV is %d]\n", res);
            }
            if (debug) {
                printf("[already send %d bytes]\n", cntSend);
                printf("[already recv %d bytes]\n", cntRecv);
            }
            if(tv.tv_sec == 0 && tv.tv_usec == 0){
                tv.tv_sec = wd;
                tv.tv_usec = 0;
            }
        }
        close(sock);
        printf("[connection closed]\n");
    }
    return 0;
}