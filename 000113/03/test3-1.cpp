#include <unistd.h>
#include <error.h>
#include <stdio.h> // perror printf
#include <stdlib.h> // exit
#include <signal.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include "my_etc.h"

static void sig_print(int signal){
    printf("[signal-%d captured]\n", signal);
    printf("[child keep running]\n");
}

static void sig_exit(int signal){
    printf("[signal-%d captured]\n", signal);
    printf("[child exit]\n");
    exit(0);
}

static void sig_childexit(int signal){
    printf("[signal-%d captured]\n", signal);
    printf("[parent exit]\n");
    exit(0);
}

int main()
{
    my_daemon(1, 1);
    int pid = fork();
    if(pid<0){
        perror("[fork failed]\n");
        exit(-1);
    }else if(pid == 0){
        // capture signal 47, then print
        // capture signal 53, then exit
        printf("[child running]\n");
        signal(47, sig_print);
        signal(53, sig_exit);
        while(1){
            sleep(1);
        }
    }else{
        signal(SIGCHLD, sig_childexit);
        printf("[child pid is %d]\n", pid);
        // wait for child exit, and then exit
        while(1){
            // 要么在fork之前注册47信号的处理函数
            // 要么在fork之后，parent先sleep，再发送47信号
            sleep(1);
            kill(pid, 47);
            // send signal 47 to child
        }
    }
    return 0;
}