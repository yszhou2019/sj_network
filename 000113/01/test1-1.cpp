#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdlib.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include "my_etc.h"

int main(){

    my_daemon(1, 1);

    // fd[0] read; fd[1] write
    int fd[2];
    char buffer[200] = {0};
    char msg[] = "message from parent";
    int res;
    if (pipe(fd) < 0)
    {
        perror("[pipe error]\n");
        exit(-1);
    }
    int pid = fork();
    if(pid<0){
        perror("[fork error]\n");
        exit(-1);
    }else if(pid==0){
        close(fd[1]);
        res = read(fd[0], buffer, sizeof(buffer));
        if(res==-1){
            perror("[child read error]\n");
            exit(-1);
        }else{
            printf("[child read] %s\n", buffer);
        }
    }
    else
    {
        close(fd[0]);
        res = write(fd[1], msg, sizeof(msg));
        if(res==-1){
            perror("[parent write error]\n");
            exit(-1);
        }
    }

    return 0;
}