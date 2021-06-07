#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <stdio.h>
#include "my_etc.h"

int main(int argc, char** argv)
{
    char ip[30];
    int port = -1;
    int myport =-1;
    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val=&port}, 0, 65535, 4000},
        {"--ip", req_arg, must_appear, is_string, {.str_val=ip}, 0, 0, 0},
        {"--myport", req_arg, must_appear, is_int, {.int_val=&myport}, 0, 65535, 4000},
    };
    my_handle_option(options, 3, argc, argv);
    
    sockaddr_in serv_addr;
    int sock = create_client(ip, port, myport, serv_addr, false);
    // connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("[connection failed]\n");
        return -1; 
    }else{
        printf("[connection established]\n");
    }
    while(1){
        sleep(1);
    }
    return 0;
}