#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include <sys/file.h>
#include "my_etc.h"

int main(int argc, char** argv)
{
    char file[30];
    Choice options[] = {
        {"--file", req_arg, must_appear, is_string, {.str_val=file}, 0, 0, 0},
    };
    my_handle_option(options, 1, argc, argv);

    // O_CREAT 如果不存在则创建
    // O_TRUNC 如果存在则清空
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