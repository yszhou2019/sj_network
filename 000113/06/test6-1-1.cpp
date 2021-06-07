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
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_fd==0){
        perror("[create socket failed]\n");
        exit(EXIT_FAILURE);
    }
    
    // address
	struct sockaddr_un address;
    bzero(&address, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, "temp");

    int opt = 1;
    // for reuse port and ip
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("[setsockopt failed]");
        exit(EXIT_FAILURE);
    }    

    // bind
    int res = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if(res<0){
        perror("[bind failed]\n");
        exit(EXIT_FAILURE);
    } else {
		printf("[bind success]\n");
	}

    res = listen(server_fd, 5);
    if(res<0){
		perror("[listen failed]\n");
		exit(EXIT_FAILURE);
	}else{ 
		printf("[listen success]\n");
	}

	int socket_new = accept(server_fd, NULL, NULL);
	if (socket_new == -1) {
		perror("[acccpet failed]\n");
		exit(-1);
	}else{
        printf("[accept success]\n");
    }


    char buffer[200] = {0};
    char msg[] = "msg from 6-1-1";
	res = recv(socket_new, buffer, 100, 0);
    printf("[recv RV is %d]\n", res);
    if(res>0){
        printf("[recv success] %s\n", buffer);
    }

    res = send(socket_new, msg, sizeof(msg), 0);
    printf("[send RV is %d]\n", res);
    if(res>0){
        printf("[send success]\n");
    }

    // while(1)
    //     sleep(1);

    unlink(address.sun_path);
	printf("[connection over]\n");

	return 0;
}