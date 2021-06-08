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
    return res;
}

std::map<int, FILE_INFO> parse_file_info(const std::string temp){
    std::map<int, FILE_INFO> resmap;

    std::regex pattern("\"date\\d+\"[\\s\\w]*>第(\\d+)次[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    std::smatch result;
    std::string::const_iterator iter_begin = temp.cbegin();
    std::string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "查找成功：" << result[0] << endl;

        std::string res = result[1];
        // cout << "提交次数:" << res << endl;

        std::string res2 = result[2];
        // cout << "作业序号:" << res2 << endl;

        std::string res3 = result[3];
        // cout << "文件名称:" << res3 << endl;

        int subnum = atoi(res.c_str());
        int workno = atoi(res2.c_str());
        FILE_INFO homework={subnum, res3};
        resmap.insert({workno, homework});

        iter_begin = result[0].second;
    }
    
    
    std::regex pattern2("\"date\\d+\"[\\s\\w]*>(尚未提交)[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    std::smatch result2;
    std::string::const_iterator iter_begin2 = temp.cbegin();
    std::string::const_iterator iter_end2 = temp.cend();

    while(regex_search(iter_begin2, iter_end2, result2, pattern2))
    {
        //cout << "查找成功：" << result2[0] << endl;

        std::string res = result2[1];
        // cout << "提交次数:" << res << endl;

        std::string res2 = result2[2];
        // cout << "作业序号:" << res2 << endl;

        std::string res3 = result2[3];
        // cout << "文件名称:" << res3 << endl;

        int workno = atoi(res2.c_str());
        FILE_INFO homework = {0, res3};
        resmap.insert({workno, homework});

        iter_begin2 = result2[0].second;
    }

    return resmap;
}

std::string parse_response(std::ifstream& in){    
    //read string from file
    std::string temp;
    std::regex pattern("alert\\('([^']*)'\\)");
    std::smatch result;

    //record the string content and record whether find the string
    std::string alertStr;
    bool isFind=false;
    while(getline(in, temp))
    {
        std::string::const_iterator iter_begin = temp.cbegin();
        std::string::const_iterator iter_end = temp.cend();
        //cout << temp << endl;
        while(regex_search(iter_begin, iter_end, result, pattern))
        {
            // cout<< result[0] << endl;
            // cout<< result[1] << endl;
            alertStr = result[1];
            isFind = true;
            break;
            iter_begin = result[0].second;
        }
    }
    in.close();

    //search the string "成功" is in the string alertStr or not
    if(regex_search(alertStr, std::regex("成功")))
    {
        return alertStr;
    }
    else
    {
        if(isFind)
        {
            return alertStr;
        }
        else
        {
            return "can not find string like 'alert xxx'";
        }
    }

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


const int MAX_SUBMIT = 3;   /* 最大允许上传次数 */
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
        // printf("connect success\n");
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
        printf("remove event failed\n");
    }
}

class Client{
private:
    char ip[32];
    int port;
    int user;
    char passwd[64];/* 这里passwd和file的64，实际上并不正确，应该设置成string */
    char file[64];
    int epoll_fd;
    epoll_event events[MAX_EVENT_NUMBER];
    sockaddr_in serv_addr;
    char *buffer;
    std::string PHPSESSID;               /* 保存在浏览器的登录凭证 */
    std::string auth_ask;                /* 验证码的问题 */
    std::string auth_answer;             /* 验证码的答案 */
    std::string file_type;               /* 文件对应的类型 */
    int file_num;                        /* 文件列表的文件数量 */
    int file_uploaded;                   /* 已提交文件数量 */
    int file_reach_max;                  /* 已经达到上限的文件数量 */
    int file_length;                     /* 本次要上传的文件长度 */
    int file_index;                      /* 本次要上传的文件对应的序号 */
    std::map<int, FILE_INFO> file_info;  /* 文件序号 ->已提交次数 文件名 */

public:
    void Run();

private:
    void get_login_page();
    void parse_login_page();
    void post_login_request();
    void parse_login_succ();
    void get_file_menu();
    void parse_file_menu();
    void check_submit_illegal();
    void config_file_info();
    void post_file_request();
    void parse_post_succ();
    int compute_content_length();
    void upload_file_data(int);
    void write_reponse_to_file(int, std::string);

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
 * 函数功能: 发送HTTP GET请求，获取登录页面，将页面保存到 log_loginpage 中
 * 函数参数: 无
 * 函数返回值: void
 */
void Client::get_login_page(){
    int sock = create_socket(serv_addr);

    /* send_GET_header() */
    sprintf(buffer, "GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nConnection: keep-alive\r\n\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);

    /* write response to file */
    write_reponse_to_file(sock, "log_loginpage");
}

/**
 * 函数功能: 解析 log_loginpage 登录页面，解析出SESSIONID auth_key auth_ans
 * 函数参数: 无
 * 函数返回值: void
 */
void Client::parse_login_page(){
    std::fstream logfile;
    logfile.open("log_loginpage", std::ios::in);
    logfile.seekg(std::ios::beg);
    std::string temp;
    while (getline(logfile, temp)){
        int res = temp.find("PHPSESSID=");
        if(res != std::string::npos){
            PHPSESSID = temp.substr(res + 10, 26); /* PHPSESSID length 26 bytes */
            // printf("%s\n", PHPSESSID.c_str());
            break;
        }
    }
    while (getline(logfile, temp)){
        int res = temp.find("auth_ask");
        if(res != std::string::npos){
            int end = temp.find_last_of("=");
            auth_ask = temp.substr(res + 16, end - res - 16);
            // printf("%s\n", auth_ask.c_str());
            break;
        }
    }
    while (getline(logfile, temp)){
        int res = temp.find("auth_answer");
        if(res != std::string::npos){
            res = temp.find_last_of("=");
            int end = temp.find_last_of(">");
            auth_answer = temp.substr(res + 1, end - res - 2);
            // printf("%s\n", auth_answer.c_str());
            getline(logfile, temp);
            break;
        }
    }
};

void Client::Run(){
    get_login_page();
    parse_login_page();
    post_login_request();
    parse_login_succ();
    get_file_menu();
    parse_file_menu();
    check_submit_illegal();
    config_file_info();
    post_file_request();
    parse_post_succ();
}


/**
 * 函数功能: 模拟登录，发起POST登录请求，提交账号密码表单
 * 函数参数: 无
 * 函数返回值: void
 * 详细描述:  模拟登录，发起POST请求，提交账号密码进行登录
 *            服务器返回的POST响应内容，保存在 log_login 文件中，以供后续解析
 *            发送req的时候不需要等待，因为server端是一直运行的，只有接收res的时候需要自己等待
 *            发送的数据 HTTP header + body的POST表单数据
 */ 
void Client::post_login_request(){
    int sock = create_socket(serv_addr);
    
    /* 计算POST body的长度 */
    char http_body[200];
    sprintf(http_body, "username=%d&password=%s&input_ans=%s&auth_ask=%s%%3D&auth_answer=%s&login=%%B5%%C7%%C2%%BC\r\n", user, passwd, auth_answer.c_str(), auth_ask.c_str(), auth_answer.c_str());

    sprintf(buffer, "POST / HTTP/1.1\r\nHost: %s:%d\r\n", ip, port);
    send(sock, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Length: %d\r\n", strlen(http_body));
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
    // printf("%s\n", http_body);

    printf("登录中...\n");

    write_reponse_to_file(sock, "log_login");
}

/**
 * 函数功能: 解析 log_login (登录响应)，判断是否登录成功
 * 函数参数: 无
 * 函数返回值: void
 * 详细描述: 从登录响应中，判断存在refresh字段，判断是否登录成功
 *           如果登录失败，则予以提示，并exit()
 */ 
void Client::parse_login_succ(){
    std::ifstream logfile;
    logfile.open("log_login", std::ios::in);
    std::string temp;
    while (getline(logfile, temp)) {
        int res = temp.find("Refresh: 0;url=./lib/smain.php");
        if(res!=std::string::npos){
            printf("登录成功\n");
            return;
        }
    }
    printf("登录失败，账号密码不匹配\n");
    exit(-1);
}

/**
 * 函数功能: 发起GET请求，获取可提交文件列表，将获取到的页面写入 log_filemenu 文件
 * 函数参数: 无
 * 函数返回值: void
 */ 
void Client::get_file_menu(){
    int sock = create_socket(serv_addr);

    /* 点击菜单，获取所有可提交的文件 */
    /* GET HTTP header */
    sprintf(buffer, "GET /lib/smain.php?action=%%B5%%DA0%%D5%%C2 HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/lib/smain.php\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, ip, port, PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);
    
    printf("获取可上传文件列表...\n");
    
    write_reponse_to_file(sock, "log_filemenu");
}

/**
 * 函数功能: 解析可提交文件列表，将解析的结果存入 Client.file_info, Client.file_num, Client.file_uploaded, Client.file_reach_max中
 * 函数参数: 无
 * 函数返回值: void
 */ 
void Client::parse_file_menu(){
    std::ifstream logfile;
    logfile.open("log_filemenu", std::ios::in);

    /* 文件指针移动到指定行 */
    std::string temp;
    logfile.seekg(std::ios::beg);
    while(getline(logfile, temp)){
        int res = temp.find("<div id=\"main\"");
        if(res!=std::string::npos){
            break;
        }
    }

    /* 开始解析HTML */
    std::vector<int> file_number = parse_file_num(temp);        /* 文件总数 已提交总数 已达上限总数 */
    file_info = parse_file_info(temp);                          /* 文件序号 . 已提交次数 文件名 */
    // printf("正常返回 vec len %d\n",file_number.size());
    file_num = file_number[0];                                  /* 文件列表的文件数量 */
    file_uploaded = file_number[1];                             /* 已提交文件数量 */
    file_reach_max = file_number[2];                            /* 已经达到上限的文件数量 */
    // printf("%d %d %d\n", file_num, file_uploaded, file_reach_max);

    /* print file list */
    printf("文件序号\t\t文件名称\t\t当前提交次数\n");
    for (int i = 1; i <= file_num; i++) {
        printf("%d\t\t%s\t\t%d\n", i, file_info[i].file_name.c_str(), file_info[i].submit_num);
    }
    printf("一共[%d]个文件，已提交[%d]个文件，[%d]个文件已经达到上传最大次数\n", file_num, file_uploaded, file_reach_max);

    logfile.close();
}

/**
 * 函数功能: 检查文件是否合法，并获取文件长度存入 Client.file_length，如果不合法，程序直接结束
 * 函数参数: 无
 * 函数返回值: void
 * 详细描述: 不合法情况包括
 *   1. 本地不存在对应文件
 *   2. 存在文件但是文件length == 0
 *   3. 文件名与文件列表中的不匹配
 *   4. 对应文件已经达到最大上传次数
 *   TODO 5. 提交时间超期
 */ 
void Client::check_submit_illegal(){
    std::ifstream infile;
    infile.open(file, std::ios::in);

    /* exit if no file */
    if(!infile){
        printf("本地文件%s不存在，结束上传\n", file);
        exit(-1);
    }

    /* get file length */
    infile.seekg(0, std::ios::end);
    std::streampos ps = infile.tellg();
    file_length = static_cast<int>(ps);
    infile.seekg(0, std::ios::beg);

    /* exit if length == 0 */
    printf("文件%s字节数量=%d\n", file, file_length);
    if (file_length == 0) {
        printf("文件%s字节数量=0，结束上传\n", file);
        exit(-1);
    }

    /* exit if file_name not match */
    bool match = false;
    std::string temp_file_name(file);
    if(temp_file_name.find("/")!=std::string::npos){
        /* 需要处理一下，比如上传 ./test/test2.cpp => test2.cpp */
        temp_file_name = temp_file_name.substr(temp_file_name.find_last_of("/") + 1);
    }

    /* 如果存在并且合法，返回文件对应的序号 */
    file_index = 1;
    for (; file_index <= file_num; file_index++){
        if(file_info[file_index].file_name == temp_file_name){
            match = true;
            break;
        }
    }

    if (!match){
        printf("文件%s不存在于列表中，结束上传\n", file);
        exit(-1);
    }

    /* exit if res_upload >= max */
    if(file_info[file_index].submit_num >= MAX_SUBMIT){
        printf("文件%s上传次数已经达到上限，结束上传\n", file);
        exit(-1);
    }
    infile.close();
}

/**
 * 函数功能: 确定本次上传文件对应的类型，存入 Client.file_type 中
 * 函数参数: 无
 * 函数返回值: void
 */ 
void Client::config_file_info(){
    /* map file suffix to certain HTTP content-type */
    std::map<std::string, std::string> dict; 
    dict["c"] = "text/plain";
    dict["cpp"] = "text/plain";
    dict["pdf"] = "application/pdf";
    dict["bz2"] = "application/octet-stream";/* 本次没有上传的其他文件默认类型也是 application/octet-stream */

    /* 确定本次上传文件对应的content-type等信息 */
    auto cur = file_info[file_index];
    int dot_pos = cur.file_name.find(".");
    if(dot_pos == std::string::npos){
        printf("文件%s没有后缀名，结束上传\n", cur.file_name.c_str());
        exit(-1);
    }
    std::string suffix = cur.file_name.substr(dot_pos + 1);

    /* 本次要上传文件的类型 */
    file_type = dict[suffix];
}

/**
 * 函数功能: 计算本次POST-body的字节数量
 * 函数参数: 无
 * 函数返回值: int
 */ 
int Client::compute_content_length(){
    int content_length = 0;
    for (int i = 1; i <= file_info.size(); i++)
    {
        sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"MAX_FILE_SIZE%d\"\r\n\r\n65536\r\n", i);
        content_length += strlen(buffer);
        if(i == file_index){
            /* 本次要上传的文件 */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n", i, file_info[i].file_name.c_str(), file_type.c_str());
            content_length += strlen(buffer);
            content_length += file_length;
        }else if(file_info[i].submit_num < MAX_SUBMIT){
            /* 其他文件，没有达到上传次数限制 */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"\"\r\nContent-Type: application/octet-stream\r\n\r\n\r\n", i);
            content_length += strlen(buffer);
        }else{
            /* 其他文件，已经达到上传次数限制 */
            continue;
        }
    }
    /* submit */
    content_length += strlen("------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"submit\"\r\n\r\n\xcc\xe1\xbd\xbb\x31\xcc\xe2\r\n------WebKitFormBoundarynwBuibeBAVRnaE7q--\r\n\r\n");
    return content_length;
}

/**
 * 函数功能: 将文件进行上传
 * 函数参数: 无
 * 函数返回值: void
 */ 
void Client::upload_file_data(int sock){
    std::ifstream infile;
    infile.open(file, std::ios::in);

    int read_file_length = file_length; /* 剩余待读取的文件长度 */
    int send_file_length = 0;
    int buffer_len = 0;
    while(send_file_length != file_length){
        /* 有剩余空间则读取 file -> buffer */
        if( read_file_length > 0 && buffer_len < BUFFER_SIZE){
            int read_bytes = (read_file_length > BUFFER_SIZE - buffer_len) ? BUFFER_SIZE - buffer_len : read_file_length;
            infile.read(buffer + buffer_len, read_bytes);
            read_file_length -= read_bytes;
            buffer_len += read_bytes;
        }
        /* 发送 buffer -> server */
        int res = send(sock, buffer, buffer_len, 0);
        if(res == -1)
            usleep(1);
        else{
            send_file_length += res;
            buffer_len -= res;
            memmove(buffer, buffer + res, buffer_len);
        }
        // printf("本次发送%d 剩余字节数量%d\n", res, file_length - send_file_length);
    }

    infile.close();
}

/**
 * 函数功能: 将sock中的数据写入 file_name 对应的文件中
 * 函数参数: 无
 * 函数返回值: void
 * 详细描述: sock对应ET触发
 */ 
void Client::write_reponse_to_file(int sock, std::string file_name){
    std::ofstream logfile;
    logfile.open(file_name, std::ios::out);
    
    /* edge-triggered */
    add_event(epoll_fd, sock, EPOLLIN | EPOLLET);

    if(epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1)<0){
        printf("epoll_wait failed\n");
    }

    while (1){
        int res = recv(sock, buffer, sizeof(buffer), 0);
        if(res == -1 && errno == EAGAIN){
            remove_event(epoll_fd, sock);
            close(sock);
            break;
        }
        for (int i = 0; i < res;i++){
            logfile.put(buffer[i]);
        }
    }

    logfile.close();
}

/**
 * 函数功能: 发起POST请求，上传文件，将response写入文件 log_postfile 中
 * 函数参数: 无
 * 函数返回值: void
 */
void Client::post_file_request(){
    int sock = create_socket(serv_addr);

    /* now, we can upload file */

    /* content-length等于post body的字节数量 */
    int content_length = compute_content_length();

    /* send POST header */
    sprintf(buffer, "POST /lib/smain.php?action=%%B5%%DA0%%D5%%C2 HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nCache-Control: max-age=0\r\nUpgrade-Insecure-Requests: 1\r\nOrigin: http://%s:%d\r\nContent-Type: multipart/form-data; boundary=----WebKitFormBoundarynwBuibeBAVRnaE7q\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Safari/537.36 Edg/91.0.864.41\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nReferer: http://%s:%d/lib/smain.php?action=%%B5%%DA0%%D5%%C2\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nCookie: PHPSESSID=%s\r\n\r\n", ip, port, content_length, ip, port, ip, port, PHPSESSID.c_str());
    send(sock, buffer, strlen(buffer), 0);

    /* send POST body */
    // idx是一定上传成功的，然后剩下的，有两种可能，已经达到上限的 or 不上传的
    
    for (int i = 1; i <= file_num; i++)
    {
        sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"MAX_FILE_SIZE%d\"\r\n\r\n65536\r\n", i);
        send(sock, buffer, strlen(buffer), 0);
        if(i == file_index){
            /* 本次要上传的文件 */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n", i, file_info[i].file_name.c_str(), file_type.c_str());
            send(sock, buffer, strlen(buffer), 0);

            /* send file data begin */
            upload_file_data(sock);

            /* send file data end */
            sprintf(buffer, "\r\n");
            send(sock, buffer, strlen(buffer), 0);

        }else if(file_info[i].submit_num < MAX_SUBMIT){

            /* 其他文件，没有达到上传次数限制 */
            sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"upfile%d\"; filename=\"\"\r\nContent-Type: application/octet-stream\r\n\r\n\r\n", i);
            send(sock, buffer, strlen(buffer), 0);
        }else{
            /* 其他文件，已经达到上传次数限制 */
            continue;
        }
    }

    /* submit */
    sprintf(buffer, "------WebKitFormBoundarynwBuibeBAVRnaE7q\r\nContent-Disposition: form-data; name=\"submit\"\r\n\r\n\xcc\xe1\xbd\xbb\x31\xcc\xe2\r\n------WebKitFormBoundarynwBuibeBAVRnaE7q--\r\n\r\n");
    send(sock, buffer, strlen(buffer), 0);

    /* POST body end */

    write_reponse_to_file(sock, "log_postfile");
}

/**
 * 函数功能: 解析文件 log_postfile 中的响应内容，给出本次上传文件的提示信息(成功 失败)
 * 函数参数: 无
 * 函数返回值: void
 */ 
void Client::parse_post_succ(){
    std::ifstream logfile;
    logfile.open("log_postfile", std::ios::in);
    /* 本次上传文件的提示信息(成功 失败) */
    printf("%s\n", parse_response(logfile).c_str());
    logfile.close();
}

int main(int argc, char** argv)
{
    char ip[32];
    int port = -1;
    int user = -1;
    char passwd[64];
    char file[64];
    Choice options[] = {
        {"--ip", req_arg, must_appear, is_string, {.str_val = ip}, 0, 0, 0},
        {"--port", req_arg, must_appear, is_int, {.int_val = &port}, 0, 65535, 4000},
        {"--user", req_arg, must_appear, is_int, {.int_val = &user}, 1750000, 2050000, 1752240},
        {"--passwd", req_arg, must_appear, is_string, {.str_val = passwd}, 0, 0, 0},
        {"--file", req_arg, must_appear, is_string, {.str_val = file}, 0, 0, 0},
    };
    my_handle_option(options, 5, argc, argv);

    /* 把Password中特殊的字符进行替换，比如! => %21 */
    // printf("%s\n", passwd);
    std::string encoded_pwd = UrlEncode(passwd);

    Client client(ip, port, user, encoded_pwd.c_str(), file);
    client.Run();


    return 0;
}