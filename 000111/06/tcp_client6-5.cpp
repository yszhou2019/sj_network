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
int myport =-1;
int getrbuf=0;
int getwbuf=0;
int setrbuf=8;
int setwbuf=8;
int getnrbuf=0;
int getnwbuf=0;

int handle_option(int argc, char **argv)
{
    if(argc<7||argc>15){
        printf("Incorrect arguement number, plz retry.\n");
        exit(0);
    }
    option long_opt[] = {
        {"port", required_argument, NULL, 1},
        {"ip", required_argument, NULL, 2},
        {"myport", required_argument, NULL, 3},
        {"getrbuf", no_argument, NULL, 4},
        {"getwbuf", no_argument, NULL, 5},
        {"setrbuf", required_argument, NULL, 6},
        {"setwbuf", required_argument, NULL, 7},
        {"getnrbuf", no_argument, NULL, 8},
        {"getnwbuf", no_argument, NULL, 9},
        {NULL, 0, NULL, 0}};
    int opt, opt_idx;
    char opt_str[] = "a:b:c:def:g:hij";
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
        case 2:
            myport=atoi(optarg);
            break;
        case 3:
            getrbuf=1;
            break;
        case 4:
            getwbuf=1;
            break;
        case 5:
            setrbuf=atoi(optarg);
            break;
        case 6:
            setwbuf=atoi(optarg);
            break;
        case 7:
            getnrbuf=1;
            break;
        case 8:
            getnwbuf=1;
            break;
        default:
            break;
        }
    }
    if (port<0||port>65535){
        printf("Arguement port not correct\n");
        exit(0);
    }
    if(myport<0||myport>65535){
        printf("Arguement myport not correct\n");
        exit(0);
    }
    if(setrbuf<0||setrbuf>16384){
        printf("Arguement setrbuf not correct\n");
        exit(0);
    }
    if(setwbuf<0||setwbuf>16384){
        printf("Arguement setrbuf not correct\n");
        exit(0);
    }
    return 0;
}

int main(int argc, char** argv)
{
    handle_option(argc, argv);
    printf("[Connecting to server at %s %d]\n",ip,port);
    
    // resources about socket
    int sock=socket(PF_INET,SOCK_STREAM,0);
    
    // optional config
    int bufsize=0;
    int optLen=sizeof(bufsize);
    if(getrbuf){
        getsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&bufsize,(socklen_t*)&optLen);
        printf("[before change] sys default read buf size is %d bytes\n",bufsize);
    }
    if(getwbuf){
        getsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char*)&bufsize,(socklen_t*)&optLen);
        printf("[before change] sys default write buf size is %d bytes\n",bufsize);
    }
    int nRecvBuf=setrbuf*1024;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int))){
        perror("[set recv buf size failed]");
        exit(EXIT_FAILURE);
    }
    int nSendBuf=setwbuf*1024;
    if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF,(const char*)&nSendBuf,sizeof(int))){
        perror("[set send buf size failed]");
        exit(EXIT_FAILURE);
    }
    if(getnrbuf){
        getsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&bufsize,(socklen_t*)&optLen);
        printf("[after change] sys default read buf size is %d bytes\n",bufsize);
    }
    if(getnwbuf){
        getsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char*)&bufsize,(socklen_t*)&optLen);
        printf("[after change] sys default write buf size is %d bytes\n",bufsize);
    }
    // resources about addr and port
    // remote server
    struct sockaddr_in serv_addr; 
    bzero(&serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
	serv_addr.sin_addr.s_addr = inet_addr(ip);
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) { 
        printf("[invalid address]\n");
        return -1; 
    } else{
        printf("[valid address]\n");
    }
    // localhost
    struct sockaddr_in localhost;
    localhost.sin_family=AF_INET;
    localhost.sin_port=htons(myport);
    localhost.sin_addr.s_addr=inet_addr("192.168.80.233");
    // bind local port
    int res=bind(sock, (struct sockaddr *)&localhost,sizeof(localhost));
    if(res==0){
        printf("[bind success]\n");
    }else{
        printf("[bind failed, check myport]\n");
    }
    // connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("[connection failed]\n");
        return -1; 
    }else{
        printf("[connection established]\n");
    }
    // printf("[send msg to server]\n");
    char data[]="ABCDEabcde";
    int cnt=0;
    while(1){
        write(sock, data, 10);
        cnt+=10;
        printf("[already send %d bytes]\n",cnt);
    }
    // recv(sock, buffer, sizeof(buffer),0);
    // printf("[recv from server] %s\n",buffer);
    return 0;
}