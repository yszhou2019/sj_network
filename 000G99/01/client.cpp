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
using std::cout;
using std::endl;

const int BUFFER_SIZE = 60000;

/* HTML parser */
struct FILE_INFO{
    int submit_num;
    std::string file_name;
};

std::vector<int> parse_file_num(const std::string temp){
    std::regex pattern("\\[\\d+\\]");
    std::smatch result;
    std::string::const_iterator iter_begin = temp.cbegin();
    std::string::const_iterator iter_end = temp.cend();
    std::vector<int> res;

    while(regex_search(iter_begin, iter_end, result, pattern))
    {       
        std::string _temp = result[0];
        std::string snum = _temp.substr(1, _temp.size()-2);
        int number = atoi(snum.c_str());
        res.push_back(number);
        iter_begin = result[0].second;
    }
    printf("����1û����\n");
    return res;
}

std::map<int, FILE_INFO> parse_file_info(const std::string temp){
    std::map<int, FILE_INFO> resmap;

    std::regex pattern("\"date\\d+\"[\\s\\w]*>��(\\d+)��[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    std::smatch result;
    std::string::const_iterator iter_begin = temp.cbegin();
    std::string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "���ҳɹ���" << result[0] << endl;

        std::string res = result[1];
        // cout << "�ύ����:" << res << endl;

        std::string res2 = result[2];
        // cout << "��ҵ���:" << res2 << endl;

        std::string res3 = result[3];
        // cout << "�ļ�����:" << res3 << endl;

        int subnum = atoi(res.c_str());
        int workno = atoi(res2.c_str());
        FILE_INFO homework={subnum, res3};
        resmap.insert({workno, homework});

        iter_begin = result[0].second;
    }
    
    
    std::regex pattern2("\"date\\d+\"[\\s\\w]*>(��δ�ύ)[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    std::smatch result2;
    std::string::const_iterator iter_begin2 = temp.cbegin();
    std::string::const_iterator iter_end2 = temp.cend();

    while(regex_search(iter_begin2, iter_end2, result2, pattern2))
    {
        //cout << "���ҳɹ���" << result2[0] << endl;

        std::string res = result2[1];
        // cout << "�ύ����:" << res << endl;

        std::string res2 = result2[2];
        // cout << "��ҵ���:" << res2 << endl;

        std::string res3 = result2[3];
        // cout << "�ļ�����:" << res3 << endl;

        int workno = atoi(res2.c_str());
        FILE_INFO homework = {0, res3};
        resmap.insert({workno, homework});

        iter_begin2 = result2[0].second;
    }

    printf("����2û����\n");
    return resmap;
}


std::string parse_response(std::fstream& file){

}

/* URL encoder */
unsigned char ToHex(unsigned char x) 
{ 
    return  x > 9 ? x + 55 : x + 48; 
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) || 
            (str[i] == '-') ||
            (str[i] == '_') || 
            (str[i] == '.') || 
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}


const int MAX_SUBMIT = 3;                                         /* ��������ϴ����� */
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
    int compute_content_length(int idx, int file_length, std::map<int, FILE_INFO>&file_info, std::string& file_type);

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
        buffer = new char[BUFFER_SIZE];
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

int Client::compute_content_length(int idx, int file_length, std::map<int, FILE_INFO>& file_info, std::string& file_type){
    int content_length = 0;
    for (int i = 1; i <= file_info.size(); i++)
    {
        sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"MAX_FILE_SIZE%d\"\r\n\r\n65536\r\n", i);
        content_length += strlen(buffer);
        if(i == idx){
            /* ����Ҫ�ϴ����ļ� */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n", i, file_info[i].file_name.c_str(), file_type.c_str());
            content_length += strlen(buffer);
            content_length += file_length;
        }else if(file_info[i].submit_num < MAX_SUBMIT){
            /* �����ļ���û�дﵽ�ϴ��������� */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"\"\r\nContent-Type: application/octet-stream\r\n\r\n\r\n", i);
            content_length += strlen(buffer);
        }else{
            /* �����ļ����Ѿ��ﵽ�ϴ��������� */
            continue;
        }
    }
    /* submit */
    content_length += strlen("------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"submit\"\r\n\r\n\xcc\xe1\xbd\xbb\x31\xcc\xe2\r\n------WebKitFormBoundarynwBuibeBAVRnaE7q--\r\n\r\n");
    return content_length;
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
    // TODO password�е������ַ���Ҫ�����滻
    sprintf(http_body, "username=%d&password=%s&input_ans=%s&auth_ask=%s%%3D&auth_answer=%s&login=%%B5%%C7%%C2%%BC\r\n", user, passwd, auth_answer.c_str(), auth_ask.c_str(), auth_answer.c_str());

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
    /* GET HTTP header */
    int sock = create_socket(serv_addr);

    // TODO
    // ������������˵�����Ҫ����˵���ȡ���п��ύ���ļ�
    sprintf(buffer, "GET /lib/smain.php?action=%%B5%%DA0%%D5%%C2 HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/lib/smain.php\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, ip, port, PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);


    /* edge-triggered */
    add_event(epoll_fd, sock, EPOLLIN | EPOLLET);

    printf("��ȡ���ϴ��ļ��б�...\n");
    if(epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1)<0){
        printf("epoll_wait failed\n");
    }
    printf("recving...\n");
    
    std::fstream outfile;
    outfile.open("phase_2.log", std::ios::in | std::ios::out | std::ios::trunc);
    if(!outfile){
        printf("create file phase_2.log failed\n");
    }
    while(1){
        int res = recv(sock, buffer, sizeof(buffer), 0);
        if(res == -1 && errno == EAGAIN){
            break;
        }
        for (int i = 0; i < res;i++){
            outfile.put(buffer[i]);
        }
    }
    
    /* �ļ�ָ���ƶ���ָ���� */
    std::string temp;
    outfile.seekg(std::ios::beg);
    while(getline(outfile, temp)){
        int res = temp.find("<div id=\"main\"");
        if(res!=std::string::npos){
            break;
        }
    }
    /* ��ʼ����HTML */
    std::vector<int> file_number = parse_file_num(temp);        /* �ļ����� ���ύ���� �Ѵ��������� */
    std::map<int, FILE_INFO> file_info = parse_file_info(temp); /* �ļ���� . ���ύ���� �ļ��� */
    printf("�������� vec len %d\n",file_number.size());
    int file_num = file_number[0];                              /* �ļ��б���ļ����� */
    int file_uploaded = file_number[1];                         /* ���ύ�ļ����� */
    int file_reach_max = file_number[2];                        /* �Ѿ��ﵽ���޵��ļ����� */
    printf("%d %d %d\n", file_num, file_uploaded, file_reach_max);

    /* print file list */
    printf("�ļ����\t\t�ļ�����\t\t��ǰ�ύ����\n");
    for (int i = 1; i <= file_num; i++) {
        printf("%d\t\t%s\t\t%d\n", i, file_info[i].file_name.c_str(), file_info[i].submit_num);
    }
    printf("һ��[%d]���ļ������ύ[%d]���ļ���[%d]���ļ��Ѿ��ﵽ�ϴ�������\n", file_num, file_uploaded, file_reach_max);


    // TODO
    // �ļ�����ƥ�� ʣ���ϴ�����Ϊ0 �����ύʱ�� => error message
    // ��������ϴ�
    
    // 1 �����ļ��Ƿ����
    // 2 �����ļ�length>0
    // 3 �����ļ��������б���
    // 4 ��Ӧ�б��ļ����ϴ�����>0
    // TODO 5 �ύʱ���Ƿ���
    // then �ϴ��ļ�
    std::ifstream infile;
    infile.open(file, std::ios::in);
    /* exit if no file */
    if(!infile){
        printf("�����ļ�%s�����ڣ���������\n", file);
        exit(-1);
    }
    /* get file length */
    infile.seekg(0, std::ios::end);
    std::streampos ps = infile.tellg();
    int file_length = static_cast<int>(ps);
    infile.seekg(0, std::ios::beg);
    /* exit if length == 0 */
    printf("�ļ�%s�ֽ�����=%d\n", file, file_length);
    if (file_length == 0) {
        printf("�ļ�%s�ֽ�����=0����������\n", file);
        exit(-1);
    }
    /* exit if file_name not match */
    bool match = false;
    std::string temp_file_name(file);
    if(temp_file_name.find("/")!=std::string::npos){
        /* ��Ҫ����һ�£������ϴ� ./test/test2.cpp => test2.cpp*/
        temp_file_name = temp_file_name.substr(temp_file_name.find_last_of("/") + 1);
    }
    int idx = 1;
    for (; idx <= file_num; idx++){
        if(file_info[idx].file_name == temp_file_name){
            match = true;
            break;
        }
    }
    if (!match){
        printf("�ļ�%s���������б��У���������\n", file);
        exit(-1);
    }
    /* exit if res_upload >= max */
    if(file_info[idx].submit_num >= MAX_SUBMIT){
        printf("�ļ�%s�ϴ������Ѿ��ﵽ���ޣ���������\n", file);
        exit(-1);
    }
    /* now, we can upload file */


    /* ȷ�������ϴ��ļ�����Ϣ */
    // ! ������Ϊ��ȷ��post body�е�content-type
    auto cur = file_info[idx];
    int dot_pos = cur.file_name.find(".");
    if(dot_pos == std::string::npos){
        printf("�ļ�%sû�к�׺������������\n", cur.file_name.c_str());
        exit(-1);
    }
    std::string suffix = cur.file_name.substr(dot_pos + 1);
    
    /* map file suffix to certain HTTP content-type */
    std::map<std::string, std::string> dict; 
    dict["c"] = "text/plain";
    dict["cpp"] = "text/plain";
    dict["pdf"] = "application/pdf";
    dict["bz2"] = "application/octet-stream";/* ����û���ϴ��������ļ�Ĭ������Ҳ�� application/octet-stream */

    /* ����Ҫ�ϴ��ļ������� */
    std::string file_type = dict[suffix];
    /* send POST header */
    int content_length = compute_content_length(idx, file_length, file_info, file_type);
    sprintf(buffer, "POST /lib/smain.php?action=%%B5%%DA0%%D5%%C2 HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nCache-Control: max-age=0\r\nUpgrade-Insecure-Requests: 1\r\nOrigin: http://%s:%d\r\nContent-Type: multipart/form-data; boundary=----WebKitFormBoundarynwBuibeBAVRnaE7q\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/lib/smain.php?action=%%B5%%DA0%%D5%%C2\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, content_length, ip, port, ip, port, PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);

    /* POST body begin */
    /* content-length����post body���ֽ����� */
    // idx��һ���ϴ��ɹ��ģ�Ȼ��ʣ�µģ������ֿ��ܣ��Ѿ��ﵽ���޵� or ���ϴ���
    for (int i = 1; i <= file_num; i++)
    {
        sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"MAX_FILE_SIZE%d\"\r\n\r\n65536\r\n", i);
        send(sock, buffer, strlen(buffer), 0);
        if(i == idx){
            /* ����Ҫ�ϴ����ļ� */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n", i, file_info[i].file_name.c_str(), file_type.c_str());
            send(sock, buffer, strlen(buffer), 0);
            /* send file data begin */
            int res_file_length = file_length;
            while(res_file_length > BUFFER_SIZE){
                infile.read(buffer, BUFFER_SIZE);
                res_file_length -= BUFFER_SIZE;
                send(sock, buffer, BUFFER_SIZE, 0);
            }
            infile.read(buffer, res_file_length);
            send(sock, buffer, res_file_length, 0);
            /* send file data end */
            sprintf(buffer, "\r\n");
            send(sock, buffer, strlen(buffer), 0);
        }else if(file_info[i].submit_num < MAX_SUBMIT){
            /* �����ļ���û�дﵽ�ϴ��������� */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"\"\r\nContent-Type: application/octet-stream\r\n\r\n\r\n", i);
            send(sock, buffer, strlen(buffer), 0);
        }else{
            /* �����ļ����Ѿ��ﵽ�ϴ��������� */
            continue;
        }
    }
    /* submit */
    sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"submit\"\r\n\r\n\xcc\xe1\xbd\xbb\x31\xcc\xe2\r\n------WebKitFormBoundarynwBuibeBAVRnaE7q--\r\n\r\n");
    send(sock, buffer, strlen(buffer), 0);

    /* POST body end */
    printf("send POST body finished\n");
    printf("content-length=%d\n", content_length);

    // ��¼�ɹ�֮���������׶Σ��ύָ���ļ�
    // ���Ƚ����ļ��Ƿ���ڵ��ж� �ļ����Ƿ�ƥ��ѡ����ж�
    // HTTP header + POST���ݣ����ļ����Լ�ֱ���ϴ��أ�������Ҫ�ָ��أ���ʱ������ȫ��ֱ�ӷŵ�body���棩
    if (epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1) < 0)
    {
        printf("epoll_wait failed\n");
        exit(-1);
    }
    printf("recving...\n");
    while(1){
        int res = recv(sock, buffer, sizeof(buffer), 0);
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
    
    /* �����ϴ��ļ�����ʾ��Ϣ(�ɹ� ʧ��) */
    outfile.seekg(std::ios::beg);
    std::string response = parse_response(outfile);
    printf("%s\n", response.c_str());

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

    // TODO ���Ȱ�Password��������ַ������滻������! => %%21
    std::string encoded_pwd = UrlEncode(passwd);

    Client client(ip, port, user, encoded_pwd.c_str(), file);
    client.Run();


    return 0;
}