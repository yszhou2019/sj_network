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
    // parent to child
    int send_id = msgget(123, IPC_CREAT | 0666);
    // child to parent
    int recv_id = msgget(124, IPC_CREAT | 0666);
    if(send_id==-1){
        perror("[send msg get failed]\n");
        exit(-1);
    }else{
        printf("[send msg id is %d]\n", send_id);
    }
    if(recv_id==-1){
        perror("[recv msg get failed]\n");
        exit(-1);
    }else{
        printf("[recv msg id is %d]\n", recv_id);
    }
    Msg msg[5]={
        {3, {"Chopper", 15}},
        {4, {"Robin", 28}},
        {4, {"Franky", 34}},
        {5, {"Brook", 88}},
        {6, {"Sunny", 2}},
    };
    for (int i = 0; i < 5;i++){
        int res = msgsnd(send_id, &msg[i], sizeof(Person), 0);
        if(res==-1){
            printf("[5-1 send msg-%d failed]\n", i);
        }else{
            printf("[5-1 send msg-%d success]%s %d\n", i, msg[i].person.name, msg[i].person.age);
        }
    }
    for (int i = 0; i < 5;i++){
        int res = msgrcv(recv_id, &msg[i], sizeof(Person), 0, 0); // 最后一个参数，代表默认阻塞读取
        if(res==-1){
            printf("[5-1 recv msg-%d failed]\n", i);
        }else{
            printf("[5-1 recv msg-%d success]%s %d\n", i, msg[i].person.name, msg[i].person.age);
        }
    }
    return 0;
}