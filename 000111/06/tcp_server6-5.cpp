#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
// #include <sys/types.h>
// #include <linux/if_link.h>
// #include <netdb.h>
// #include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>


int port = -1;
char ip[30];
int getrbuf=0;
int getwbuf=0;
int setrbuf=8;
int setwbuf=8;
int getnrbuf=0;
int getnwbuf=0;

int handle_option(int argc, char **argv) {
    if(argc<5||argc>13){
        printf("Incorrect arguement number, plz retry.\n");
        exit(0);
    }
    option long_opt[] = {
        {"port", required_argument, NULL, 1},
        {"ip", required_argument, NULL, 2},
        {"getrbuf", no_argument, NULL, 3},
        {"getwbuf", no_argument, NULL, 4},
        {"setrbuf", required_argument, NULL, 5},
        {"setwbuf", required_argument, NULL, 6},
        {"getnrbuf", no_argument, NULL, 7},
        {"getnwbuf", no_argument, NULL, 8},
        {NULL, 0, NULL, 0}};
    int opt, opt_idx;
    char opt_str[] = "a:b:cde:f:ghi";
    while ((opt = getopt_long_only(argc, argv, opt_str, long_opt, &opt_idx)) != -1) {
        if (((char)opt) == '?') 
            exit(0);
        switch (opt_idx)
        {
        case 0:
            port = atoi(optarg);
            break;
        case 1:
            strcpy(ip,optarg);
            break;
        case 2:
            getrbuf=1;
            break;
        case 3:
            getwbuf=1;
            break;
        case 4:
            setrbuf=atoi(optarg);
            break;
        case 5:
            setwbuf=atoi(optarg);
            break;
        case 6:
            getnrbuf=1;
            break;
        case 7:
            getnwbuf=1;
            break;
        default:
            break;
        }
    }
    if (port<0||port>65535) {
        printf("Arguement port exceed, plz retry.\n");
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

void getAllIP(int &ipNumber, char ipList[20][30])
{       
    char ip[30];
    int fd, intrface, retn = 0;
    struct ifreq buf[INET_ADDRSTRLEN];
    struct ifconf ifc;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        // caddr_t,linux内核源码里定义的：typedef void *caddr_t；
        ifc.ifc_buf = (caddr_t)buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
        {
            intrface = ifc.ifc_len/sizeof(struct ifreq);
            while (intrface-- > 0)
            {
                if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface])))
                {
                    strcpy(ipList[ipNumber],inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
                    printf("IP-%d: %s\n",ipNumber+1, ipList[ipNumber]);
                    ipNumber++;
                }
            }
        }
        close(fd);
    }
}

int main(int argc, char** argv)
{
    handle_option(argc, argv);
    printf("[Begin listening to PORT %d]\n",port);
    // print all ip address
	int ipNumber = 0;
	char ipList[20][30];
    getAllIP(ipNumber,ipList);

    // some data and buffer
    char reply[]="Hello from server";
    char buffer[20000]={0};
    // resources about socket
    int sock= socket(PF_INET, SOCK_STREAM, 0);
    if (sock == 0) {
        perror("[socket failed]");
        exit(EXIT_FAILURE);
    }
    // resources about addr and port
    struct sockaddr_in address;
    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // 端口号由主机序到网络序

    // check if invalid
    int wrong=1;
    if(argc==3||0==strcmp(ip,"0.0.0.0")){
        // ip default or 0.0.0.0, bind all
        address.sin_addr.s_addr = INADDR_ANY;
    }else{
        // else check if invalid
        for(int i=0;i<ipNumber;i++){
            if(0==strcmp(ip,ipList[i])){
                address.sin_addr.s_addr = inet_addr(ip);
                wrong=0;
                break;
            }
        }
        if(wrong){
            perror("[invalid ip]");
            exit(0);
        }
    }

    int opt=1;
    // for reuse port and ip
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("[setsockopt failed]");
        exit(EXIT_FAILURE);
    }

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
        printf("[accept failed, errno is %d]",errno);
        exit(EXIT_FAILURE);
    }else{
        char remote[INET_ADDRSTRLEN];
        // print remote ip and port
        printf("[connected with %s %d]\n",inet_ntop(AF_INET,&client.sin_addr,remote,INET_ADDRSTRLEN),ntohs(client.sin_port));
    }
    getchar();
    int cnt=0;
    while(1){
        ret = read(connfd,buffer,10000);
        cnt+=10000;
        if(ret>0){
            printf("[already read %d bytes]\n",cnt);
        }else{
            printf("[something error, read %d bytes]\n",ret);
        }
        getchar();
    }
    // printf("[recv from client] %s \n",buffer);
    // send(connfd,reply,strlen(reply),0);
    // printf("[send reply to client]\n");
    printf("[connection over]\n");
    printf("[recv return code is %d\n",ret);
    return 0;
}