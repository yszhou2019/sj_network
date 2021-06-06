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

#include <sys/epoll.h>

#include <map>


const int MAX_EVENT_NUMBER = 1;

/**
 * working in non-block mode
 */ 
int create_socket(const sockaddr_in& serv_addr){
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(res<0){
        printf("connect to server failed\n");
        exit(-1);
    }else{
        printf("connect success\n");
    }
    set_nonblock(sock);
    return sock;
}

/* level-triggered */
void add_event(int epoll_fd, int fd, int state) {
	epoll_event event;
	event.data.fd = fd;
	event.events = state;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)<0){
        // printf("add event failed\n");
    }
	// 设置非阻塞
	set_nonblock(fd);
}

void remove_event(int epoll_fd, int fd){
	epoll_event event;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL)<0){
        // printf("remove event failed\n");
    }
}

class Client{
private:
    char ip[32];
    int port;
    int user;
    char passwd[32];
    char file[32];
    char PHPSESSID[32];
    int epoll_fd;
    epoll_event events[MAX_EVENT_NUMBER];
    sockaddr_in serv_addr;
    char *buffer;
    std::map<const char *, const char *> dict; /* map file suffix to certain HTTP content-type */

public:
    void Run();

private:
    void phase_0();
    void phase_1();
    void phase_2();

public:
    Client(const char* _ip, int _port, int _user, const char* _pwd, const char* _file):
    port(_port), user(_user)
    {
        strcpy(ip, _ip);
        strcpy(passwd, _pwd);
        strcpy(file, _file);
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET; 
        serv_addr.sin_port = htons(port); 
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        dict["c"] = "text/plain";
        dict["cpp"] = "text/plain";
        dict["pdf"] = "application/pdf";
        dict["bz2"] = "application/octet-stream";
        buffer = new char[60000];
        if(buffer == nullptr){
            printf("create client failed, no enough space for buffer\n");
            exit(-1);
        }
        epoll_fd = epoll_create(MAX_EVENT_NUMBER);
        if(epoll_fd == -1){
            printf("epoll create failed\n");
            exit(-1);
        }
    }
    ~Client(){
        if(buffer!=nullptr){
            delete[] buffer;
            buffer = nullptr;
        }
    }
    
};

/**
 * 发送HTTP GET请求，获取登录页面
 * 从返回的页面中解析出来SESSIONID
 */
void Client::phase_0(){
    int sock = create_socket(serv_addr);

    sprintf(buffer, "GET / HTTP/1.1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Host: %s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Upgrade-Insecure-Requests: 1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Encoding: gzip, deflate\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Connection: keep-alive\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);

    // 发送的数据HTTP header，GET
    add_event(epoll_fd, sock, EPOLLIN);

    /* write response to file */
    FILE *outfile = fopen("phase_0.log", "w+");
    if(outfile == nullptr){
        printf("create outfile for phase_0 failed\n");
    }
    /* stdio functions are bufferd IO, so need to close buffer */
    if(setvbuf(outfile, NULL, _IONBF, 0)!=0){
        printf("close fprintf buffer failed!\n");
    }

    while(1){
        int res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if(res<0){
            printf("epoll_wait failed\n");
        }else if(res == 0 && errno == EAGAIN){
            printf("close socket\n");
            remove_event(epoll_fd, sock);
            close(sock);
            break;
        }
        res = recv(sock, buffer, sizeof(buffer), 0);
        for (int i = 0; i < res;i++){
            fputc(buffer[i], outfile);
        }
    }

    fclose(outfile);
}


void Client::Run(){
    phase_0();
}

/**
 * 登录
 * 提交账号密码表单
 * 从返回的页面中是否存在refresh字段，判断是否登录成功
 * 如果登录失败，则予以提示，并exit()
 */ 
void Client::phase_1(){
    int sock = create_socket(serv_addr);
    // 填充buffer GET / HTTP/1.0
    // 发送req的时候不需要等待，因为server端是一直运行的，只有接收res的时候需要自己等待
    // 发送的数据 HTTP header + body的POST表单数据
    // res需要解析是否存在refresh字段，从而判断是否登录成功
    
    sprintf(buffer, "POST / HTTP/1.1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Host: %s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Length: 103\r\n"); // TODO
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Cache-Control: max-age=0\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Upgrade-Insecure-Requests: 1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Origin: %s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Referer: http://%s:%d/\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Encoding: gzip, deflate\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Language: zh-CN,zh;q=0.9\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Cookie: PHPSESSID=%s\r\n", PHPSESSID); // TODO
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Connection: keep-alive\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "username=%d&password=%s\%21&input_ans=20&auth_ask=4*5\%3D&auth_answer=20&login=\%B5\%C7\%C2\%BC\r\n",user,passwd);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);

    add_event(epoll_fd, sock, EPOLLIN);
    int res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    if(res<0){
        printf("epoll_wait failed\n");
    }
    recv(sock, buffer, sizeof(buffer), 0);
    // TODO 根据返回的数据中是否有refresh字段，判断是否登录成功
    if(true){
        printf("login success, start to upload file...\n");
        // TODO 然后开启第三阶段
    }else{
        printf("login failed, username and password not match\n");
    }
    remove_event(epoll_fd, sock);
    close(sock);
}

/**
 * 提交指定文件
 * 如果文件不存在，或者文件名不匹配，则予以提示，并exit()
 * 如果文件名存在，提交成功，程序正常运行结束
 *              如果提交失败（比如上传文件长度0字节），则予以提示，并exit()
 */
void Client::phase_2(){
    // 首先检查本地是否存在对应文件，不存在直接报错
    // 存在文件但是文件length=0 直接报错
    // 存在文件并且文件length>0，准备开始上传
    // 如果文件名不匹配，那么还是报错
    // 
    /* header */
    int sock = create_socket(serv_addr);
    sprintf(buffer, "POST /lib/smain.php?action=%B5%DA0%D5%C2 HTTP/1.1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Host: %s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Length: 1563\r\n"); // TODO 变化项
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Cache-Control: max-age=0\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Upgrade-Insecure-Requests: 1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Origin: http://%s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryah9ixVLBPYABOUJA\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Referer: http://%s:%d/lib/smain.php?action=%B5%DA0%D5%C2\r\n", ip, port); // TODO 变化项
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Encoding: gzip, deflate\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Language: zh-CN,zh;q=0.9\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Cookie: PHPSESSID=%s\r\n", PHPSESSID); // TODO 变化项
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Connection: keep-alive\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);

    /* body */
    // TODO 这个循环应该是动态变化的
    int file_total = 6;
    bool upload_forbid[100]; // TODO true则代表已经达到上限，禁止上传
    for (int i = 1; i <= file_total; i++)
    {
        sprintf(buffer, "------WebKitFormBoundaryah9ixVLBPYABOUJA\r\n");
        send(sock, buffer, strlen(buffer), 0);
        sprintf(buffer, "Content-Disposition: form-data; name=\"MAX_FILE_SIZE%d\"\r\n", i);
        send(sock, buffer, strlen(buffer), 0);
        sprintf(buffer, "\r\n");
        send(sock, buffer, strlen(buffer), 0);
        sprintf(buffer, "65536\r\n");
        send(sock, buffer, strlen(buffer), 0);
        if(!upload_forbid[i]){
            sprintf(buffer, "------WebKitFormBoundaryah9ixVLBPYABOUJA\r\n");
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "Content-Disposition: form-data; name=\"upfile%d\"; filename=\"%s\"\r\n", i, file); // TODO 变化项 如果有对应的文件，那么就是file指针，如果没有对应的文件，那么就是"" 或者没有%s这个字段
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "Content-Type: application/octet-stream\r\n"); // TODO 变化项
            // Content-Type: application/octet-stream -> 没有上传的文件
            // Content-Type: application/pdf -> test2.pdf
            // Content-Type: text/plain -> test4.cpp
            // Content-Type: text/plain -> test4.c
            // Content-Type: application/octet-stream-> test1.tar.bz2
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "\r\n");
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "\r\n"); // TODO 真正上传的数据 如果没有数据那么只有0d 0a
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    /* submit */
    sprintf(buffer, "------WebKitFormBoundaryah9ixVLBPYABOUJA\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Disposition: form-data; name=\"submit\"\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\xcc\xe1\xbd\xbb\x31\xcc\xe2\r\n"); // TODO 奇怪的数据 代表submit 按钮 被点击
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "------WebKitFormBoundaryah9ixVLBPYABOUJA--\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);
    // 登录成功之后进入第三阶段，提交指定文件
    // 首先进行文件是否存在的判断 文件名是否匹配选项的判断
    // HTTP header + POST数据（大文件是自己直接上传呢，还是需要分割呢？暂时先理解成全部直接放到body里面）
    // 根据res，进行判断，提示上传成功 or 上传失败
    close(sock);
}

int main(int argc, char** argv)
{
    char ip[32];
    int port = -1;
    int user = -1;
    char passwd[32];
    char file[32];
    Choice options[] = {
        {"--ip", req_arg, must_appear, is_string, {.str_val = ip}, 0, 0, 0},
        {"--port", req_arg, must_appear, is_int, {.int_val = &port}, 0, 65535, 4000},
        {"--user", req_arg, must_appear, is_int, {.int_val = &user}, 1750000, 2050000, 1752240},
        {"--passwd", req_arg, must_appear, is_string, {.str_val = passwd}, 0, 0, 0},
        {"--file", req_arg, must_appear, is_string, {.str_val = file}, 0, 0, 0},
    };
    my_handle_option(options, 5, argc, argv);

    Client client(ip, port, user, passwd, file);
    client.Run();


    return 0;
}