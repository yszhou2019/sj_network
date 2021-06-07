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
    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--ip", req_arg, must_appear, is_string, {.str_val=ip}, 0, 0, 0},
        {"--myport", req_arg, must_appear, is_int, {.int_val=&myport}, 0, 65535, 4000},
    };
    my_handle_option(options, 3, argc, argv);
    
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
    // �������û��������������ôֻ�е�errno == EINPROGRESS��ʱ�򣬲ű�ʾ�������ڽ�����
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_SET(sock, &rfds);
    FD_SET(sock, &wfds);
    res = select(sock + 1, &rfds, &wfds, NULL, NULL);
    printf("[select return values is %d]\n", res);
    printf("[errno is %d]\n", errno);
    // select��ʱ���߳�����������
    if(res < 0){
        perror("[select failed]");
        exit(EXIT_FAILURE);
    }else if(res == 0){
        printf("[connection timeout]\n");
        exit(-1);
    }

    if(!FD_ISSET(sock, &rfds) && !FD_ISSET(sock, &wfds)){
        perror("[no event on socket found]");
    }
    int err;
    socklen_t errlen = sizeof(int);
    // ͨ��getsockopt����ȡ�����sock�ϵĴ���
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0){
        printf("[get sock option failed]\n");
        exit(EXIT_FAILURE);
    } else if (0 != err){ 
    // ������������0����ô���ӳ���
        printf("[error is %d]\n", err);
        close(sock);
        exit(EXIT_FAILURE);
    }
    // ���ӳɹ�
    char buffer[200] = {0};
    fd_set readfds;
    fd_set writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(sock, &readfds);
    FD_SET(sock, &writefds);
    // �ٽ���select
    while(1){
        res = select(sock + 1, &readfds, &writefds, NULL, NULL);
        printf("*************************\n");
        printf("[select RV is %d]\n", res);
        int err;
        socklen_t errlen = sizeof(int);
        res = getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errlen);
        printf("[getsockopt RV is %d]\n", res);
        printf("[err is %d]\n", err);
        printf("[errno is %d]\n", errno);
        res = send(sock, buffer, 10, 0);
        printf("[send RV is %d]\n", res);
        printf("[errno is %d]\n", errno);
        sleep(1);
    }
    close(sock);
    printf("[connection closed]\n");
    
    return 0;
}