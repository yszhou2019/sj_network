#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <netinet/in.h> // �����ʵ����Ҫ������link Ŀ���ļ���ʱ��ᱨ��"sockaddr_in"�ṹ��δ����
#include <sys/file.h>
#include "my_etc.h"

int main(int argc, char** argv)
{
    char file[30];
    Choice options[] = {
        {"--file", req_arg, must_appear, is_string, {.str_val=file}, 0, 0, 0},
    };
    my_handle_option(options, 1, argc, argv);

    // O_CREAT ����������򴴽�
    // O_TRUNC ������������
    int fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0666);
    printf("[open RV is %d]\n", fd);
    if(fd<0){
        printf("[open file %s failed]\n", file);
        exit(-1);
    }else{
        printf("[open file %s success]\n", file);
    }

    // set non-block
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    // fcntl - lock
    // struct flock lk;
    // memset(&lk, 0, sizeof(lk));
    // lk.l_type = F_WRLCK;
    // lk.l_whence = SEEK_SET;
    // int res = fcntl(fd, F_SETLKW, &lk);
    // printf("[fcntl RV is %d]\n", res);

    // fcntl - unlock
    // lk.l_type = F_UNLCK;
    // fcntl(fd, F_SETLKW, &lk)

    int res = flock(fd, LOCK_EX);
    printf("[flock RV is %d]\n", res);
    if(res<0){
        printf("[flock failed]\n");
        exit(-1);
    }else{
        printf("[flock success]\n");
    }

    char msg[] = "msg from 7-2-1";
    res = write(fd, msg, sizeof(msg));
    printf("[write RV is %d]\n", res);

    res = flock(fd, LOCK_UN);
    printf("[unlock RV is %d]\n", res);
    if(res<0){
        printf("[unlock failed]\n");
        exit(-1);
    }else{
        printf("[unlock success]\n");
    }

    close(fd);

    return 0;
}