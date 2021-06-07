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

    set_nonblock(sock);

    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("[connect RV is %d]\n", res);
    if(res==0){
        printf("[connection established]\n");
    }else if(errno != EINPROGRESS){
        perror("[connect failed]\n");
        exit(EXIT_FAILURE);
    }
    

    char buffer[200] = {0};
    char msg[] = "msg from 6-1-2";

    fd_set wfds;
    for (int i = 0; i < 5;i++){
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        res = select(sock + 1, NULL, &wfds, NULL, NULL);
        printf("[select RV is %d]\n", res);
        res = send(sock, msg, sizeof(msg), 0);
        printf("[send RV is %d]\n", res);
        if(res>0){
            printf("[send success]\n");
        }
    }

    fd_set rfds;
    for (int i = 0; i < 5;i++){
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        res = select(sock + 1, &rfds, NULL, NULL, NULL);
        printf("[select RV is %d]\n", res);
        res = recv(sock, buffer, 15, 0);
        printf("[recv RV is %d]\n", res);
        if(res>0){
            printf("[recv success] %s\n", buffer);
        }
    }

    // while(1){
    //     res = send(sock, msg, sizeof(msg), 0);
    //     printf("[send RV is %d]\n", res);
    // }


	printf("[connection over]\n");

	return 0;
}