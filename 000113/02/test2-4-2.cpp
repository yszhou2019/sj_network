#include <unistd.h>
#include <stdio.h>
#include <error.h> 
#include <stdlib.h> // exit
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // open
#include <netinet/in.h> // �����ʵ����Ҫ������link Ŀ���ļ���ʱ��ᱨ��"sockaddr_in"�ṹ��δ����
#include <sys/wait.h>
#include "my_etc.h"

int main()
{
    my_daemon(1, 1);
    char file[] = "temp";
    char buffer[200];
    int fd = open(file, O_RDONLY);
    if(fd<0){
        perror("[test2-4-2 open error]\n");
        exit(-1);
    }
    int res = read(fd, buffer, sizeof(buffer));
    if(res==-1){
        perror("[test2-4-2 read error]\n");
    }else{
        printf("[test2-4-2 read] %s\n", buffer);
    }
    close(fd);

    unlink(file); // delete tem FIFO file
    wait(NULL);
    return 0;
}