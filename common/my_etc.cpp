#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include "my_etc.h"

// int���͵Ĳ���
// string���͵Ĳ���
// ��������
// ������ȷ����1��������ڴ��󷵻�0
int my_handle_option(Choice options[], int opt_size, int argc, char** argv)
{
    for (int k = 0; k < opt_size;k++) {
        // ����Ƿ�ȱ�ٱ�Ҫ�Ĳ���
        if(options[k].is_must){
            bool flag = true;
            for (int i = 0; i < argc;i++){
                if(strcmp(argv[i], options[k].name)==0){
                    flag = false;
                    break;
                }
            }
            if(flag){
                printf("[lack of necessary arguement] --%s\n", options[k].name);
                return 0;
            }
        }
        // �޲β���
        if(options[k].has_arg == no_arg){
            for (int i = 0; i < argc;i++){
                if(strcmp(argv[i], options[k].name)==0){
                    *options[k].val.int_val = 1;
                    break;
                }
            }
        } 
        // �вβ���
        else if(options[k].has_arg == req_arg){
            int i = 0;
            for (; i < argc;i++){
                if(strcmp(argv[i],options[k].name)==0)
                    break;
            }
            if(options[k].is_int){
                int temp = atoi(argv[i + 1]);
                if(temp<options[k].lower_bound||temp>options[k].upper_bound){
                    temp = options[k].default_val;
                }
                *options[k].val.int_val = temp;
                printf("[arg %s value is %d]\n", options[k].name, *options[k].val.int_val);
            }else{
                strcpy(options[k].val.str_val, argv[i + 1]);
                printf("[arg %s value is %s]\n", options[k].name, options[k].val.str_val);
            }
        }
    }
    return 1;
}

int my_daemon(int nochdir,int noclose)
{
    int fd;

    switch (fork()) {
        case -1:
            printf("fork() failed\n");
            return -1;

        case 0:
            break;

        default:
            exit(0);
    }
    
    /*
    pid_t setsid(void);
    ���̵���setsid()�ɽ���һ���¶Ի��ڼ䡣
    ������ô˺����Ľ��̲���һ����������鳤����˺�������һ���¶Ի��ڣ����Ϊ��
        1���˽��̱�ɸ��¶Ի��ڵĶԻ����׽��̣�session leader���Ի����׽����Ǵ����öԻ��ڵĽ��̣���
           �˽����Ǹ��¶Ի����е�Ψһ���̡�
        2���˽��̳�Ϊһ���½�������鳤���̡��½�����ID���ǵ��ý��̵Ľ���ID��
        3���˽���û�п����նˡ�����ڵ���setsid֮ǰ�ν�����һ�������նˣ���ô������ϵҲ�������
    ������ý����Ѿ���һ����������鳤����˺������ش���Ϊ�˱�֤���������������ͨ���ȵ���fork()��
    Ȼ��ʹ�丸������ֹ�����ӽ��̼���ִ�С���Ϊ�ӽ��̼̳��˸����̵Ľ�����ID�����ӽ��̵Ľ���ID������
    ����ģ����߲�������ȣ�������ͱ�֤���ӽ��̲���һ����������鳤��
    */
    if (setsid() == -1) {
        printf("setsid() failed\n");
        return -1;
    }
    
    switch (fork()) {
        case -1:
            printf("fork() failed\n");
            return -1;

        case 0:
            break;

        default:
            exit(0);
    }

    umask(0);
    if(nochdir==0){
        chdir("/");
    }
    

    if(noclose==0){
        // close(0);
        // fd0=open("/dev/null",O_RDWR);//�رձ�׼���� ����׼�������׼����
        // dup2(fd0,1);
        // dup2(fd0,2);
        long maxfd;
        if ((maxfd = sysconf(_SC_OPEN_MAX)) != -1)
        {
            for (fd = 0; fd < maxfd; fd++)
            {
                close(fd);
            }
        }
        fd = open("/dev/null", O_RDWR);
        if (fd == -1) {
            printf("open(\"/dev/null\") failed\n");
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            printf("dup2(STDIN) failed\n");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            printf("dup2(STDOUT) failed\n");
            return -1;
        }

        if (dup2(fd, STDERR_FILENO) == -1) {
            printf("dup2(STDERR) failed\n");
            return -1;
        }

        if (fd > STDERR_FILENO) {
            if (close(fd) == -1) {
                printf("close() failed\n");
                return -1;
            }
        }
    }
    
    /*
    // Standard file descriptors.
    #define STDIN_FILENO    0   // Standard input.
    #define STDOUT_FILENO   1   // Standard output.
    #define STDERR_FILENO   2   // Standard error output.
    */
    
    /*
    int dup2(int oldfd, int newfd);
    dup2()�������Ʋ���oldfd��ָ���ļ�������������������������newfd��һ�鷵�ء�
    ���newfd�Ѿ��򿪣����Ƚ���رա�
    ���oldfdΪ�Ƿ���������dup2()���ش��󣬲���newfd���ᱻ�رա�
    ���oldfdΪ�Ϸ�������������newfd��oldfd��ȣ���dup2()�����κ��£�ֱ�ӷ���newfd��
    */


    return 0;
}

// set sock in non-blocking mode
void set_nonblock(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        exit(EXIT_FAILURE);
    }
    int res = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    if ( res < 0) {
        perror("fcntl(F_SETFL) failed");
        exit(EXIT_FAILURE);
    }
    // printf("[set socket NONBLOCK, return values is %d]\n", res);
}

// ��ȡ�������е�ip�����Լ�ip��ַ
static void _getAllIP(int &ipNumber, char ipList[20][30])
{    char ip[30];
    int fd, intrface, retn = 0;
    struct ifreq buf[INET_ADDRSTRLEN];
    struct ifconf ifc;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        // caddr_t,linux�ں�Դ���ﶨ��ģ�typedef void *caddr_t��
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

// ��������/������socket��ָ�����ض˿ڣ���ָ��IP�Ͷ˿ڵ�server
// ִ�е�bind֮�󷵻�
// �޸�serve_addr
// ����socket
int create_client(const char serv_ip[], int serv_port, int myport, sockaddr_in& serv_addr, bool isNonBlock)
{
    printf("[Connecting to server at %s %d]\n", serv_ip, serv_port);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if(isNonBlock){
        set_nonblock(sock);
    }
    // resources about remote server's addr and port
    bzero(&serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(serv_port); 
	serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    if(inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr)<=0) { 
        perror("[invalid address]");
        printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }else{
        printf("[valid address]\n");
    }
    // localhost
    struct sockaddr_in localhost;
    localhost.sin_family = AF_INET;
    localhost.sin_port = htons(myport);
    localhost.sin_addr.s_addr = INADDR_ANY;
    // bind local port
    int res = bind(sock, (struct sockaddr *)&localhost, sizeof(localhost));
    if(res == 0){
        printf("[bind success]\n");
    }else{
        perror("[bind failed, check myport]");
        printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }
    return sock;
}

// ��������/������socket��ָ������IP�Ͷ˿�
// ִ�е�bind֮�󷵻�
// ����socket
int create_server(const char ip[], int port, bool isNonBlock)
{
    printf("[Begin listening to PORT %d]\n", port);
    // print all ip address
	int ipNumber = 0;
	char ipList[20][30];
    _getAllIP(ipNumber, ipList);
    // resources about socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == 0){
        perror("[socket failed]");
        exit(EXIT_FAILURE);
    }
    if(isNonBlock){
        set_nonblock(sock);
    }
    // resources about addr and port
    struct sockaddr_in address;
    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // �˿ں���������������

    // check if invalid
    int wrong = 1;
    if(0 == strcmp(ip, "0.0.0.0")){
        // ip default or 0.0.0.0, bind all
        address.sin_addr.s_addr = INADDR_ANY;
    }else{
        // else check if invalid
        for(int i = 0; i < ipNumber; i++){
            if(0 == strcmp(ip, ipList[i])){
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

    int opt = 1;
    // for reuse port and ip
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("[setsockopt failed]");
        exit(EXIT_FAILURE);
    }    
    // bind socket to port
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    if ( ret< 0){
        perror("[bind failed]");
        exit(EXIT_FAILURE);
    }else{
        printf("[bind success]\n");
    }
    return sock;
}

// ��ӡָ��sock��tcp recv��������С��������(��λ:�ֽ�)
int get_recv_buf(int sock)
{
    int bufsize = 0;
    int optLen = sizeof(bufsize);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, (socklen_t *)&optLen);
    printf("[before change] sys default read buf size is %d bytes\n", bufsize);
    return bufsize;
}

// ��ӡָ��sock��tcp send��������С��������(��λ:�ֽ�)
int get_send_buf(int sock)
{
    int bufsize = 0;
    int optLen = sizeof(bufsize);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, (socklen_t *)&optLen);
    printf("[before change] sys default read buf size is %d bytes\n", bufsize);
    return bufsize;
}

// ����ָ��sock��tcp recv��������С��(��λ:KB)
void set_recv_buf(int sock, int size)
{
    int nRecvBuf = size * 1024;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int))){
        perror("[set recv buf size failed]");
        exit(EXIT_FAILURE);
    }
}

// ����ָ��sock��tcp send��������С��(��λ:KB)
void set_send_buf(int sock, int size)
{
    int nSendBuf = size * 1024;
    if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF,(const char*)&nSendBuf,sizeof(int))){
        perror("[set send buf size failed]");
        exit(EXIT_FAILURE);
    }
}