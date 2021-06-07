#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>


int port = -1;

int handle_option(int argc, char **argv)
{
    if(argc!=3){
        printf("Incorrect Arguement number, plz retry.\n");
        exit(0);
    }
    option long_opt[] = {
        {"port", required_argument, NULL, 1},
        {NULL, 0, NULL, 0}};
    int opt, opt_idx;
    char opt_str[] = "a:b";
    while ((opt = getopt_long_only(argc, argv, opt_str, long_opt, &opt_idx)) != -1)
    {
        if (((char)opt) == '?') 
            exit(0);
        switch (opt_idx)
        {
        case 0:
            port = atoi(optarg);
            break;
        default:
            break;
        }
    }
    if (port<0||port>65535)
    {
        printf("Arguement port exceed, plz retry.\n");
        exit(0);
    }
    return 0;
}

int main(int argc, char** argv)
{
    handle_option(argc, argv);
    printf("[Begin listening to PORT %d]\n",port);

    // some data and buffer
    char reply[]="Hello from server";
    char buffer[2048]={0};
    // resources about socket
    int sock= socket(PF_INET, SOCK_STREAM, 0);
    if (sock == 0)
    {
        perror("[socket failed]");
        exit(EXIT_FAILURE);
    }
    // resources about addr and port
    struct sockaddr_in address;
    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port); // 端口号由主机序到网络序

    int opt=1;
    // for reuse port and ip
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("[setsockopt failed]");
        exit(EXIT_FAILURE);
    }
    // bind socket to port
    int ret=bind(sock, (struct sockaddr *)&address,sizeof(address));
    if ( ret< 0){
        perror("[bind failed]");
        exit(EXIT_FAILURE);
    }else{
        printf("[bind success]\n");
    }
    // listen to socket
    ret = listen(sock, 5);
    if ( ret < 0){
        perror("[listen failed]");
        exit(EXIT_FAILURE);
    }else{
        printf("[listening to port %d]\n",port);
    }
    // accept from socket
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client,&addrlen);
    if (connfd < 0){
        printf("[accept failed, errno is %d\n]",errno);
        exit(EXIT_FAILURE);
    }else{
        char remote[INET_ADDRSTRLEN];
        // print remote ip and port
        printf("[connected with %s %d]\n",inet_ntop(AF_INET,&client.sin_addr,remote,INET_ADDRSTRLEN),ntohs(client.sin_port));
    }
    auto res = recv(connfd,buffer,sizeof(buffer),0);
    // printf("[recv from client] %s \n",buffer);
    // send(connfd,reply,strlen(reply),0);
    // printf("[send reply to client]\n");
    printf("[connection over]\n");
    printf("[recv return code is %d\n",res);
    return 0;
}