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

/* STL */
#include <map>
#include <vector>


#include <regex>

#include <string>
#include <fstream>
#include <iostream> /* debug */


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
	// ���÷�����
	set_nonblock(fd);
}

void remove_event(int epoll_fd, int fd){
	epoll_event event;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL)<0){
        printf("remove event failed\n");
    }
}

class Client{
private:
    char ip[32];
    int port;
    int user;
    char passwd[32];
    char file[32];
    int epoll_fd;
    epoll_event events[MAX_EVENT_NUMBER];
    sockaddr_in serv_addr;
    char *buffer;
    std::string PHPSESSID;
    std::string auth_ask;
    std::string auth_answer;

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
 * ����HTTP GET���󣬻�ȡ��¼ҳ��
 * �ӷ��ص�ҳ���н�������SESSIONID
 */
void Client::phase_0(){
    int sock = create_socket(serv_addr);

    sprintf(buffer, "GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nConnection: keep-alive\r\n\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);

    /* edge-triggered */
    add_event(epoll_fd, sock, EPOLLIN | EPOLLET);

    /* write response to file */
    std::fstream outfile;
    outfile.open("phase_0.log", std::ios::in | std::ios::out | std::ios::trunc);
    if(!outfile){
        printf("create file phase_0.log failed\n");
    }

    int res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    if(res < 0){
        printf("epoll_wait failed\n");
    }
    /* ���Ͻ������ݣ�ֱ����ȡʧ�� */
    while(1){
        res = recv(sock, buffer, sizeof(buffer), 0);
        // TODO ���ﲢ����ȷ���ᵼ����ʱ��html��û�н�����Ͼ��˳�ѭ�����������ַ��������޷�����
        if(res == -1 && errno == EAGAIN){
            printf("close socket\n");
            remove_event(epoll_fd, sock);
            close(sock);
            break;
        }
        for (int i = 0; i < res;i++){
            outfile.put(buffer[i]);
            // fputc(buffer[i], outfile);
        }
    }

    /* find out PHPSESSID and set PHPSESSID */
    outfile.seekg(std::ios::beg);
    std::string temp;
    while (getline(outfile, temp)){
        int res = temp.find("PHPSESSID=");
        if(res != std::string::npos){
            PHPSESSID = temp.substr(res + 10, 26); /* PHPSESSID length 26 bytes */
            printf("%s\n", PHPSESSID.c_str());
            break;
        }
    }
    while (getline(outfile, temp)){
        int res = temp.find("auth_ask");
        if(res != std::string::npos){
            int end = temp.find_last_of("=");
            auth_ask = temp.substr(res + 16, end - res - 16);
            printf("%s\n", auth_ask.c_str());
            break;
        }
    }
    while (getline(outfile, temp)){
        int res = temp.find("auth_answer");
        if(res != std::string::npos){
            res = temp.find_last_of("=");
            int end = temp.find_last_of(">");
            auth_answer = temp.substr(res + 1, end - res - 2);
            printf("%s\n", auth_answer.c_str());
            getline(outfile, temp);
            break;
        }
    }

    outfile.close();
}


void Client::Run(){
    phase_0();
    phase_1();
    phase_2();
}

/**
 * ��¼
 * �ύ�˺������
 * �ӷ��ص�ҳ�����Ƿ����refresh�ֶΣ��ж��Ƿ��¼�ɹ�
 * �����¼ʧ�ܣ���������ʾ����exit()
 */ 
void Client::phase_1(){
    int sock = create_socket(serv_addr);
    // ���buffer GET / HTTP/1.0
    // ����req��ʱ����Ҫ�ȴ�����Ϊserver����һֱ���еģ�ֻ�н���res��ʱ����Ҫ�Լ��ȴ�
    // ���͵����� HTTP header + body��POST������
    // res��Ҫ�����Ƿ����refresh�ֶΣ��Ӷ��ж��Ƿ��¼�ɹ�
    
    char http_body[200];
    sprintf(http_body, "username=%d&password=%s\x25\x32\x31&input_ans=%s&auth_ask=%s%%3D&auth_answer=%s&login=%%B5%%C7%%C2%%BC\r\n", user, passwd, auth_answer.c_str(), auth_ask.c_str(), auth_answer.c_str());

    sprintf(buffer, "POST / HTTP/1.1\r\nHost: %s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Length: %d\r\n", strlen(http_body)); // TODO
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Cache-Control: max-age=0\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Upgrade-Insecure-Requests: 1\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Origin: http://%s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Referer: http://%s:%d/login.php\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Encoding: gzip, deflate\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Cookie: PHPSESSID=%s\r\n", PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Connection: keep-alive\r\n");
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(sock, buffer, strlen(buffer), 0);

    /* post body */
    send(sock, http_body, strlen(http_body), 0);
    // TODO ��Ҫ�滻��password�м�������ַ�


    std::string temp = http_body;
    std::cout << temp << std::endl;
    std::cout << temp.size() << std::endl;
    printf("%s\n", http_body); /* DEBUG */

    /* edge-triggered */
    add_event(epoll_fd, sock, EPOLLIN | EPOLLET);
    
    std::fstream outfile;
    outfile.open("phase_1.log", std::ios::in | std::ios::out | std::ios::trunc);
    if(!outfile){
        printf("create file phase_1.log failed\n");
    }
    printf("��¼��...\n");/* DEBUG */
    int res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    if(res < 0){
        printf("epoll_wait failed\n");
    }
    printf("recving...\n");/* DEBUG */
    /* ���Ͻ������ݣ�ֱ����ȡʧ�� */
    while(1){
        res = recv(sock, buffer, sizeof(buffer), 0);
        // printf("res %d\n",res);
        if(res == -1 && errno == EAGAIN){
            break;
        }
        for (int i = 0; i < res;i++){
            outfile.put(buffer[i]);
        }
    }

    outfile.seekg(std::ios::beg);
    bool loginSucc = false;
    while (getline(outfile, temp)) {
        res = temp.find("Refresh: 0;url=./lib/smain.php");
        if(res!=std::string::npos){
            loginSucc = true;
            printf("��¼�ɹ�\n");
            break;
        }
    }

    if(!loginSucc){
        printf("��¼ʧ�ܣ��˺����벻ƥ��\n");
        exit(-1);
    }

    /* ��¼�ɹ� */
    sprintf(buffer, "GET /lib/smain.php HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/login.php\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, ip, port, PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);

    printf("waiting...\n");/* DEBUG */
    res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    if(res < 0){
        printf("epoll_wait failed\n");
    }
    printf("recving...\n");/* DEBUG */
    // outfile.seekp(std::ios::end);
    while (1) {
        res = recv(sock, buffer, sizeof(buffer), 0);
        if(res == -1 && errno == EAGAIN){
            printf("close socket\n");
            remove_event(epoll_fd, sock);
            close(sock);
            break;
        }
        for (int i = 0; i < res;i++){
            outfile.put(buffer[i]);
        }
    }

    outfile.close();
}

/**
 * �ύָ���ļ�
 * ����ļ������ڣ������ļ�����ƥ�䣬��������ʾ����exit()
 * ����ļ������ڣ��ύ�ɹ��������������н���
 *              ����ύʧ�ܣ������ϴ��ļ�����0�ֽڣ�����������ʾ����exit()
 */
void Client::phase_2(){
    // ���ȼ�鱾���Ƿ���ڶ�Ӧ�ļ���������ֱ�ӱ���
    // �����ļ������ļ�length=0 ֱ�ӱ���
    // �����ļ������ļ�length>0��׼����ʼ�ϴ�
    // ����ļ�����ƥ�䣬��ô���Ǳ���
    // 
    /* header */
    int sock = create_socket(serv_addr);

    // TODO
    // ������������˵�����Ҫ����˵���ȡ���п��ύ���ļ�
    sprintf(buffer, "GET /lib/smain.php?action=%%B5%%DA0%%D5%%C2 HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/lib/smain.php\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, ip, port, PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);


    /* edge-triggered */
    add_event(epoll_fd, sock, EPOLLIN | EPOLLET);

    printf("��ȡ���ϴ��ļ��б�...\n");
    int res = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    if(res < 0){
        printf("epoll_wait failed\n");
    }
    printf("recving...\n");
    
    std::fstream outfile;
    outfile.open("phase_2.log", std::ios::in | std::ios::out | std::ios::trunc);
    if(!outfile){
        printf("create file phase_2.log failed\n");
    }
    while(1){
        res = recv(sock, buffer, sizeof(buffer), 0);
        if(res == -1 && errno == EAGAIN){
            printf("close socket\n");
            remove_event(epoll_fd, sock);
            close(sock);
            break;
        }
        for (int i = 0; i < res;i++){
            outfile.put(buffer[i]);
        }
    }

    // TODO
    // ������Ҫ�ϴ����ļ�����
    // �����ļ��б�
    // �����ж��ļ��Ƿ����
    // ��� �ļ��� ʣ����ϴ�����
    std::string temp;

    outfile.seekg(std::ios::beg);
    while(getline(outfile, temp)){
        res = temp.find("<div id=\"main\"");
        if(res!=std::string::npos){
            break;
        }
    }

    int file_cnt = 0;                        /* �ļ��б���ļ����� */
    int file_uploaded = 0;                   /* ���ύ�ļ����� */
    int file_reach_max = 0;                  /* �Ѿ��ﵽ���޵��ļ����� */
    int max_upload = 3;                      /* ��������ϴ����� */
    std::vector<int> res_upload(file_cnt, 3);/* ÿ���ļ���ʣ���ϴ����� */

// ��0��Ŀǰ�Ѳ��õ���ҵ��[6]�⣬���ύ[5]�⣬����[0]���Ѵ�����ύ���� -> file_cnt
// <td>0</td><td>${test4.cpp}</td>  -> �ļ���
// disabled>��${2}��&nbsp -> ʣ����� max_upload - (${����} - 1) =maxupload-��ǰ����+1

    std::smatch result;
    std::regex pattern("Ŀǰ�Ѳ��õ���ҵ��[(\\d{*})]�⣬���ύ[\\d{*}]�⣬����[(\\d{*})]���Ѵ�����ύ����");
    std::regex_match(temp, result, pattern);
    std::cout << result[0] << std::endl;
    std::cout << result[1] << std::endl;
    std::cout << result[2] << std::endl;
    std::cout << result[3] << std::endl;


    // /* ������ */
    // int beg = temp.find("Ŀǰ�Ѳ��õ���ҵ��[");
    // int end = temp.find("]�⣬���ύ[");
    // // std::cout << beg << std::endl;
    // // std::cout << end << std::endl;
    // file_cnt = stoi(temp.substr(beg + 19, end - beg - 19));
    // printf("�Ѳ���%s\n", temp.substr(beg + 19, end - beg - 19).c_str());


    // /* ���ύ */
    // beg = end;
    // end = temp.find("�⣬����");
    // // std::cout << beg << std::endl;
    // // std::cout << end << std::endl;
    // file_uploaded = stoi(temp.substr(beg + 12, end - beg - 13));
    // printf("���ύ%s\n", temp.substr(beg + 12, end - beg - 13).c_str());

    // /* �Ѿ��ﵽ���� */
    // beg = end;
    // end = temp.find("���Ѵ�����ύ����");
    // // std::cout << beg << std::endl;
    // // std::cout << end << std::endl;
    // file_reach_max = stoi(temp.substr(beg + 9, end - beg - 10));
    // printf("�Ѵ�����%s\n", temp.substr(beg + 9, end - beg - 10).c_str());

    // TODO
    // �ļ�����ƥ�� ʣ���ϴ�����Ϊ0 �����ύʱ��

    // // �ܲ���ֱ���ϴ��ļ���
    // // TODO content-length�仯��
    // sprintf(buffer, "POST /lib/smain.php?action=%B5%DA0%D5%C2 HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: 1665\r\nCache-Control: max-age=0\r\nUpgrade-Insecure-Requests: 1\r\nOrigin: http://%s:%d\r\nContent-Type: multipart/form-data; boundary=----WebKitFormBoundarynwBuibeBAVRnaE7q\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/lib/smain.php?action=%B5%DA0%D5%C2\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, ip, port, ip, port, PHPSESSID.c_str());
    // send(sock, buffer, strlen(buffer), 0);
    std::map<const char *, const char *> dict; /* map file suffix to certain HTTP content-type */
    dict["c"] = "text/plain";
    dict["cpp"] = "text/plain";
    dict["pdf"] = "application/pdf";
    dict["bz2"] = "application/octet-stream";

    /* body */
    // TODO ���ѭ��Ӧ���Ƕ�̬�仯��
    int file_total = 6;
    bool upload_forbid[100]; // TODO true������Ѿ��ﵽ���ޣ���ֹ�ϴ�
    for (int i = 1; i <= file_total; i++)
    {
        sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"MAX_FILE_SIZE%d\"\r\n\r\n65536\r\n", i);
        if(!upload_forbid[i]){
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\n");
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "Content-Disposition: form-data; name=\"upfile%d\"; filename=\"%s\"\r\n", i, file); // TODO �仯�� ����ж�Ӧ���ļ�����ô����fileָ�룬���û�ж�Ӧ���ļ�����ô����"" ����û��%s����ֶ�
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "Content-Type: application/octet-stream\r\n"); // TODO �仯��
            // Content-Type: application/octet-stream -> û���ϴ����ļ�
            // Content-Type: application/pdf -> test2.pdf
            // Content-Type: text/plain -> test4.cpp
            // Content-Type: text/plain -> test4.c
            // Content-Type: application/octet-stream-> test1.tar.bz2
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "\r\n");
            send(sock, buffer, strlen(buffer), 0);
            sprintf(buffer, "\r\n"); // TODO �����ϴ������� ���û��������ôֻ��0d 0a
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    /* submit */
    sprintf(buffer, "------WebKitFormBoundaryah9ixVLBPYABOUJA\r\nContent-Disposition: form-data; name=\"submit\"\r\n\r\n\xcc\xe1\xbd\xbb\x31\xcc\xe2\r\n------WebKitFormBoundaryah9ixVLBPYABOUJA--\r\n\r\n");
    send(sock, buffer, strlen(buffer), 0);
    // ��¼�ɹ�֮���������׶Σ��ύָ���ļ�
    // ���Ƚ����ļ��Ƿ���ڵ��ж� �ļ����Ƿ�ƥ��ѡ����ж�
    // HTTP header + POST���ݣ����ļ����Լ�ֱ���ϴ��أ�������Ҫ�ָ��أ���ʱ������ȫ��ֱ�ӷŵ�body���棩
    // ����res�������жϣ���ʾ�ϴ��ɹ� or �ϴ�ʧ��
    close(sock);
    outfile.close();
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