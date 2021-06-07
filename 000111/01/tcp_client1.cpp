#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <stdio.h>


char ip[30];
int port = -1;

int handle_option(int argc, char **argv)
{
    if(argc!=5){
        printf("Incorrect arguement number, plz retry.\n");
        exit(0);
    }
    option long_opt[] = {
        {"port", required_argument, NULL, 1},
        {"ip", required_argument, NULL, 2},
        {NULL, 0, NULL, 0}};
    int opt, opt_idx;
    char opt_str[] = "a:b:c";
    while ((opt = getopt_long_only(argc, argv, opt_str, long_opt, &opt_idx)) != -1)
    {
        if (((char)opt) == '?') 
            exit(0);
        switch (opt_idx)
        {
        case 0:
            port = atoi(optarg);
            break;
        case 1:
            strcpy(ip, optarg);
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
    printf("[Connecting to server at %s %d]\n",ip,port);
    
    // data to send
    char data[]="Hello from client";
    // IPv4 socket address
    struct sockaddr_in serv_addr; 
    int sockfd=socket(PF_INET,SOCK_STREAM,0);
    char buffer[1024] = {0}; 
    // init
    memset(&serv_addr, 0, sizeof(serv_addr)); 
    // config ip and port
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
	serv_addr.sin_addr.s_addr = inet_addr(ip);
    // invalid server ip
    // [inet_pton] convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) { 
        printf("[invalid address]\n");
        return -1; 
    } else{
        printf("[address valid]\n");
    }
    // failed
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("[connection failed]\n");
        return -1; 
    }else{
        printf("[connection established]\n");
    }
    // printf("[send msg to server]\n");
    // send(sockfd, data, sizeof(data),0);
    recv(sockfd, buffer, sizeof(buffer),0);
    // printf("[recv from server] %s\n",buffer);
    return 0;
}