#include <unistd.h>
#include <stdio.h>
#include <error.h> 
#include <stdlib.h> // exit
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // open
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include <sys/wait.h>
#include "my_etc.h"

int main()
{
    my_daemon(1, 1);
    char file1[] = "temp1";
    char file2[] = "temp2";
    char msg[] = "IPC msg from test2-5-2";
    char buffer[200];
    int fd[2];
    fd[0] = open(file1, O_RDONLY);
    if(fd[0]<0){
        perror("[test2-5-2 open error]\n");
        exit(-1);
    }
    fd[1] = open(file2, O_WRONLY);
    if(fd[1]<0){
        perror("[test2-5-2 open error]\n");
        exit(-1);
    }
    int res = write(fd[1], msg, sizeof(msg));
    if(res==-1){
        perror("[test2-5-2 write error]\n");
    }else{
        printf("[test2-5-2 write success]\n");
    }
    res = read(fd[0], buffer, sizeof(buffer));
    if(res==-1){
        perror("[test2-5-2 read error]\n");
    }else{
        printf("[test2-5-2 read] %s\n", buffer);
    }
    close(fd[0]);
    close(fd[1]);

    unlink(file1); // delete tem FIFO file1
    unlink(file2);
    wait(NULL);
    return 0;
}