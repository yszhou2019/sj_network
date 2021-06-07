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
    int fd = open(file, O_RDONLY, 0666);
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
    
    int res;
    char buffer[200] = {0};
    while(1){ 
        res = flock(fd, LOCK_SH);
        printf("*******************\n");
        printf("[flock RV is %d]\n", res);
        if(res<0){
            printf("[flock failed]\n");
            continue;
        }else{
            printf("[flock success]\n");
        }
        res = read(fd, buffer, sizeof(buffer));
        printf("[read RV is %d]\n", res);
        if(res>0){
            printf("[read success] %s\n", buffer);
            break;
        }
        res = flock(fd, LOCK_UN);
        if(res<0){
            printf("[unlock failed]\n");
        }else{
            printf("[unlock success]\n");
        }
        sleep(1);
    }

    res = flock(fd, LOCK_UN);
    printf("[unlock RV is %d]\n", res);
    if(res<0){
        printf("[unlock failed]\n");
        exit(-1);
    }else{
        printf("[unlock success]\n");
    }
    close(fd);
    unlink(file);
    return 0;
}