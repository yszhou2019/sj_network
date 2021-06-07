#include <unistd.h>
#include <stdio.h>
#include <error.h> 
#include <stdlib.h> // exit
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // open
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include "my_etc.h"

int main()
{
    my_daemon(1, 1);
    char file[] = "temp";
    char msg_parent[] = "IPC msg from parent";
    char msg_child[] = "IPC msg from child";
    char buffer[200];
    int res;
    res = mkfifo(file, S_IRWXU);
    if(res==-1){
        perror("[mkfifo error]\n");
        exit(-1);
    }
    int pid = fork();
    if(pid==0){
        int fd = open(file, O_RDWR);
        if(fd<0){
            perror("[child open error]\n");
            exit(-1);
        }
        res = write(fd, msg_child, sizeof(msg_child));
        if(res==-1){
            perror("[child write error]\n");
        }
        res = read(fd, buffer, sizeof(buffer));
        if(res==-1){
            perror("[child read error]\n");
        }else{
            printf("[child read from FIFO] %s\n", buffer);
        }
        close(fd);
    }
    else
    {
        int fd = open(file, O_RDWR);
        if(fd<0){
            perror("[parent open error]\n");
            exit(-1);
        }
        res = write(fd, msg_parent, sizeof(msg_parent));
        if(res==-1){
            perror("[parent write error]\n");
        }
        res = read(fd, buffer, sizeof(buffer));
        if(res==-1){
            perror("[parent read error]\n");
        }else{
            printf("[parent read from FIFO] %s\n", buffer);
        }
        close(fd);
    }

    unlink(file); // delete tem FIFO file
    return 0;
}