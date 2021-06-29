#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <stdio.h>
#include <fcntl.h> // set socket non-block
#include "my_etc.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
    char ip[30] = "10.60.102.252";
    int port = -1;
    int myport = 4000;
    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--myport", req_arg, must_appear, is_int, {.int_val=&myport}, 0, 65535, 4000},
    };
    my_handle_option(options, 1, argc, argv);
    
    sockaddr_in serv_addr;
    int sock = create_client(ip, port, myport, serv_addr, false);
    
    // connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("[connection failed]\n");
        return -1; 
    }else{
        printf("[connection established]\n");
    }
    // char buffer[200] = "login\n{\"username\": \"root\",\"password\": \"root123\"}\0";

    char buffer[200] = "createDir\n{\"session\":\"abc\",\"prefix\":\"hello\",\"dirname\":\"/root\",\"queueid\":123}\0";
    int res = write(sock, buffer, strlen(buffer)+1);
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "hello");
    res = write(sock, buffer, strlen(buffer));
    cout << "发送字节数量 " << res << endl;
    // cout << buffer << endl;
    memset(buffer, 200, 0xff);
    res = read(sock, buffer, 100);
    cout << "读取字节" << res << " bytes" << endl;
    for (int i = 0; i < 10;i++)
    {
        printf("%x %x %x %x   %x %x %x %x\n", buffer[i * 8], buffer[i * 8 + 1], buffer[i * 8 + 2], buffer[i * 8 + 3], buffer[i * 8 + 4], buffer[i * 8 + 5], buffer[i * 8 + 6], buffer[i * 8 + 7]);
    }

    close(sock);
    return 0;
}