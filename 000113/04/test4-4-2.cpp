#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include "my_etc.h"

struct Person{
    char name[20];
    int age;
};

struct Msg
{
    long mtype;
    Person person;
};

int main()
{
    my_daemon(1, 1);
    int msg_id = msgget(123, IPC_CREAT | 0666);
    if(msg_id==-1){
        perror("[msg get failed]\n");
        exit(-1);
    }else{
        printf("[msg id is %d]\n", msg_id);
    }
    Msg msg[10];
    for (int i = 0; i < 10;i++){
        int res = msgrcv(msg_id, &msg[i], sizeof(Person), 0, 0); // 最后一个参数，代表默认阻塞读取
        if(res==-1){
            printf("[recv msg-%d failed]\n", i);
        }else{
            printf("[recv msg-%d success]\n%s %d\n", i, msg[i].person.name, msg[i].person.age);
        }
    }
    return 0;
}