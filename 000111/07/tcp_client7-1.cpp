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
int readbyte=1024;
int sendbyte=1024;
int delay=0;
int debug=0;

int handle_option(int argc, char **argv)
{
    if(argc<7||argc>14){
        printf("Incorrect arguement number, plz retry.\n");
        exit(0);
    }
    option long_opt[] = {
        {"port", required_argument, NULL, 1},
        {"ip", required_argument, NULL, 2},
        {"myport", required_argument, NULL, 3},
        {"readbyte", required_argument, NULL, 4},
        {"sendbyte", required_argument, NULL, 5},
        {"delay", required_argument, NULL, 6},
        {"debug", no_argument, NULL, 7},
        {NULL, 0, NULL, 0}};
    int opt, opt_idx;
    char opt_str[] = "a:b:c:d:e:f:gh";
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
            readbyte=atoi(optarg);
            break;
        case 4:
            sendbyte=atoi(optarg);
            break;
        case 5:
            delay=atoi(optarg);
            break;
        case 6:
            debug=1;
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
    if(readbyte<1||readbyte>65536){
        readbyte=1024;
    }
    if(sendbyte<1||sendbyte>65536){
        sendbyte=1024;
    }
    if(delay<0||delay>5000000){
        delay=0;
    }
    return 0;
}

int main(int argc, char** argv)
{
    handle_option(argc, argv);
    printf("[Connecting to server at %s %d]\n",ip,port);
    
    // resources about socket
    int sock=socket(PF_INET,SOCK_STREAM,0);
    
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
    char buffer[70000]={0};
    memset(buffer,'A',70000);
    int readCnt=0,writeCnt=0;
    int ret=0;
    while(1){
        ret=read(sock, buffer, readbyte);
        readCnt+=ret;
        if(debug){
            printf("[already read %d bytes]\n",readCnt);
        }
        ret=write(sock,buffer,sendbyte);
        writeCnt+=ret;
        if(debug){
            printf("[already write %d bytes]\n",writeCnt);
        }
        usleep(delay);
    }
    // recv(sock, buffer, sizeof(buffer),0);
    // printf("[recv from server] %s\n",buffer);
    return 0;
}