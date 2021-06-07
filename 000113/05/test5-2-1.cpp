#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include <sys/shm.h>
#include "my_etc.h"

int main()
{
    char mem[200];
    int shmem_id;
    char *addr = nullptr;

    my_daemon(1, 1);
    shmem_id = shmget(123, sizeof(mem), IPC_CREAT | 0666);
    addr = (char *)shmat(shmem_id, NULL, 0);
    if(addr==NULL){
        perror("[shmem assocate failed]\n");
        exit(-1);
    }
    strcpy(addr, "msg from 5-2-1");
    printf("[send success]\n");
    int res = shmdt(addr);
    if(res==-1){
        perror("[shmem delete failed]\n");
        exit(-1);
    }else if (res==0){
        printf("[shmem delete success]\n");
    }
    return 0;
}