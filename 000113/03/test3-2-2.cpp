#include <unistd.h>
#include <error.h>
#include <stdio.h> // perror printf
#include <stdlib.h> // exit
#include <signal.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include <sys/stat.h> // mkfifo
#include <fcntl.h> // open
#include "my_etc.h"



char file[] = "temp";
int fd;
static void sig_print(int signal)
{
    printf("[signal-%d captured]\n", signal);
    printf("[process keep running]\n");
}

static void sig_exit(int signal){
    printf("[signal-%d captured]\n", signal);
    printf("[process exit]\n");
    close(fd);
    unlink(file);
    exit(0);
}

int main()
{
    my_daemon(1, 1);
    signal(47, sig_print);
    signal(53, sig_exit);
    int res = mkfifo(file, S_IRWXU);
    if(res==-1){
        perror("[mkfifo error]\n");
        exit(-1);
    }
    fd = open(file, O_WRONLY);
    if(res==-1){
        perror("[open failed]\n");
        exit(-1);
    }
    int pid = getpid();
    char pid_str[20] = {0};
    printf("[test3-2-2 pid is %d]\n", pid);
    sprintf(pid_str, "%d", pid);
    res = write(fd, pid_str, sizeof(pid_str));
    if(res==-1){
        perror("[write to fifo error]\n");
        exit(-1);
    }

    while(1)
        sleep(1);
    return 0;
}