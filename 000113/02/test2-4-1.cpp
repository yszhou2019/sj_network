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
    char file[] = "temp";
    char msg[] = "IPC msg from test2-4-1";
    char buffer[200];
    int res = mkfifo(file, S_IRWXU);
    if(res!=0){
        perror("[mkfifo error]\n");
        exit(-1);
    }
    int fd = open(file, O_WRONLY);
    if(fd<0){
        perror("[test2-4-1 open error]\n");
        exit(-1);
    }
    res = write(fd, msg, sizeof(msg));
    if(res==-1){
        perror("[test2-4-1 write error]\n");
    }else{
        printf("[test2-4-1 write success]\n");
    }
    close(fd);

    unlink(file); // delete tem FIFO file
    wait(NULL);
    return 0;
}