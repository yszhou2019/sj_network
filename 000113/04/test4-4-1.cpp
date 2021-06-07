#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <netinet/in.h> // �����ʵ����Ҫ������link Ŀ���ļ���ʱ��ᱨ��"sockaddr_in"�ṹ��δ����
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
    Msg msg[10] = {
        {1, {"Luffy", 17}},
        {1, {"Zoro", 19}},
        {2, {"Nami", 18}},
        {2, {"Usopo", 17}},
        {1, {"Sanji", 19}},
        {3, {"Chopper", 15}},
        {4, {"Robin", 28}},
        {4, {"Franky", 34}},
        {5, {"Brook", 88}},
        {6, {"Sunny", 2}}
    };
    for (int i = 0; i < 10;i++){
        int res = msgsnd(msg_id, &msg[i], sizeof(Person), 0);
        if(res==-1){
            printf("[send msg-%d failed]\n", i);
        }else{
            printf("[send msg-%d success]\n", i);
        }
    }
    return 0;
}