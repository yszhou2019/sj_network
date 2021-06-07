#include <unistd.h>
#include <error.h>
#include <stdio.h> // perror printf
#include <stdlib.h> // exit
#include <signal.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include <sys/stat.h> // mkfifo
#include <fcntl.h> // open
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

static void detect(int pid, bool& isRunning){
    int res = kill(pid, 0);
    if(res==-1){
        printf("[process-%d not exist]\n", pid);
        isRunning = false;
    }
    else if (res == 0)
    {
        printf("[process-%d is running]\n", pid);
        isRunning = true;
    }
}

int main()
{
    my_daemon(1, 1);
    // get pid
    char pid_str[20] = {0};
    char file[] = "temp";
    int fd = open(file, O_RDONLY);
    if(fd==-1){
        perror("[open file error]\n");
        exit(-1);
    }
    int res = read(fd, pid_str, sizeof(pid_str));
    if(res==-1){
        perror("[read fifo error]\n");
        exit(-1);
    }
    close(fd);
    int pid = atoi(pid_str);
    printf("[target pid is %d]\n", pid);
    bool isRunning = false;
    int cnt = 0;
    bool jump = false;
    while (1)
    {
        detect(pid, isRunning);
        // detect 3-2-2 is running
        if(isRunning){
            for (; cnt < 9;cnt++){
                // detect
                detect(pid, isRunning);
                // send signal 47
                if(isRunning){
                    printf("[send signal 47 to process-%d for the %d time] success\n", pid, cnt);
                    kill(pid, 47);
                    jump = false;
                }else{
                    printf("[send signal 47 to process-%d for the %d time] failed\n", pid, cnt);
                    jump = true;
                    break;
                }
                sleep(1);
            }
            if(jump)
                continue;
            // send signal 53
            detect(pid, isRunning);
            if(isRunning){
                printf("[send signal 53 to process-%d] success\n", pid);
                kill(pid, 53);
                jump = false;
            }else{
                printf("[send signal 53 to process-%d] failed\n", pid);
                jump = true;
            }
            if(jump)
                continue;
            return 0;
        }else{
            sleep(1);
        }
    }
    return 0;
}