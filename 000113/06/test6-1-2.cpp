#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <netinet/in.h> // 这个其实不需要，但是link 目标文件的时候会报错"sockaddr_in"结构体未声明
#include <sys/un.h> // sockaddr_un
#include "my_etc.h"

int main()
{

    // create socket file descriptor
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock==0){
        perror("[create socket failed]\n");
        exit(EXIT_FAILURE);
    }
    
    // server address
	struct sockaddr_un serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, "temp");

    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(res<0){
        perror("[connect failed]\n");
        exit(EXIT_FAILURE);
    } else {
		printf("[connect success]\n");
	}

    char buffer[200] = {0};
    char msg[] = "msg from 6-1-2";
    res = send(sock, msg, sizeof(msg), 0);
    printf("[send RV is %d]\n", res);
    if(res>0){
        printf("[send success]\n");
    }

	res = recv(sock, buffer, 100, 0);
    printf("[recv RV is %d]\n", res);
    if(res>0){
        printf("[recv success] %s\n", buffer);
    }
    // while(1){
    //     res = send(sock, msg, sizeof(msg), 0);
    //     printf("[send RV is %d]\n", res);
    // }

	printf("[connection over]\n");

	return 0;
}