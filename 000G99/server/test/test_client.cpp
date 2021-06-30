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
#include "../utils/json.hpp"
#include <ctime>
using namespace std;
using json = nlohmann::json;

// char ip[30] = "10.60.102.252";
char ip[30] = "47.102.201.228";
int port = -1;
int myport = 4000;
int sock;
sockaddr_in serv_addr;

int crea_clie(const char serv_ip[], int serv_port, int myport, sockaddr_in& serv_addr, bool isNonBlock)
{
    // printf("[Connecting to server at %s %d]\n", serv_ip, serv_port);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if(isNonBlock){
        set_nonblock(sock);
    }
    int on = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
    // resources about remote server's addr and port
    bzero(&serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(serv_port); 
	serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    if(inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr)<=0) { 
        // perror("[invalid address]");
        // printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }else{
        // printf("[valid address]\n");
    }
    // localhost
    struct sockaddr_in localhost;
    localhost.sin_family = AF_INET;
    localhost.sin_port = htons(myport);
    localhost.sin_addr.s_addr = INADDR_ANY;
    // bind local port
    int res = bind(sock, (struct sockaddr *)&localhost, sizeof(localhost));
    if(res == 0){
        // printf("[bind success]\n");
    }else{
        // perror("[bind failed, check myport]");
        // printf("[errno is %d]\n", errno);
        exit(EXIT_FAILURE);
    }
    return sock;
}

void conn()
{
    sock = crea_clie(ip, port, myport++, serv_addr, false);
    
    // connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        // printf("[connection failed]\n");
    }else{
        // printf("[connection established]\n");
    }
}

void start(string phase)
{
    conn();
    cout << "====================================" << endl;
    cout << "测试 " << phase << " 开始" << endl;
}

void end(string phase)
{
    cout << "测试 " << phase << " 结束" << endl;
    close(sock);
}

// void send_type(string type)
// {
//     int res = write(sock, type.c_str(), type.size());
//     cout << "send type bytes " << res << endl;
// }

// void send_req(json& req)
// {
//     string msg = req.dump(4);
//     write(sock, msg.c_str(), msg.size() + 1);
// }

void send_header(string& msg, json& req)
{
    char buffer[2048];
    string header = msg + req.dump(4);
    int res = write(sock, header.c_str(), header.size() + 1);
    cout << "发送字节数量 " << res << endl;

    // char buffer[2048];
    // string header = msg + req.dump(4);
    // sprintf(buffer, "%s", header.c_str());
    // int i;
    // for (i = 0; i < header.size() + 1; i++)
    //     buffer[i] = header.c_str()[i];
    // for (int k = 0; k < 1025;k++)
    //     buffer[i + k] = 'A';
    // buffer[i + 1024] = 'B';
    // buffer[i + 1025] = '\0';

    // int res = write(sock, buffer, i+1026);
    // // int res = write(sock, header.c_str(), header.size() + 1);
    // cout << "发送字节数量 " << res << endl;
}

void recv_print()
{
    char buffer[300];
    // 接收res并打印
    memset(buffer, 0, sizeof(buffer));
    int bytes = read(sock, buffer, sizeof(buffer));
    cout << "接收字节数量 " << bytes << endl;
    for (int i = 0; i < bytes; i++)
    {
        cout << buffer[i];
    }
    cout << endl;
    // memset(buffer, 0, sizeof(buffer));
    // bytes = read(sock, buffer, sizeof(buffer));
    // cout << "接收字节数量 " << bytes << endl;
    // for (int i = 0; i < bytes; i++)
    // {
    //     cout << buffer[i];
    // }
    // cout << endl;
}

//=========================================
// TODO 已经检查，全部正确

// 注册
void _signup(string phase, string username,string password)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"username", username},
        {"password", password}};

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
    
}

// 登录
void _login(string phase,string username,string password)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"username", username},
        {"password", password}};

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

// 绑定
void _setbind(string phase,int bindid,string session)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"bindid", bindid},
        {"session", session}};

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
} 

// 解绑
void _disbind(string phase, string session)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", session}};

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

// 退出
void _logout(string phase,string session)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", session}};

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

//=========================================

typedef long long ll;

void _uploadFile(string phase, string sid, int qid, string fname,string path,string md5,ll size, int mtime)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", sid},
        {"queueid",qid},
        {"filename",fname},
        {"path",path},
        {"md5",md5},
        {"size",size},
        {"mtime",mtime}
        };

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

void _uploadChunk(string phase, string sid, int vid,int qid, int chunkid, ll offset,int chunksize)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", sid},
        {"vfile_id",vid},
        {"queueid",qid},
        {"chunkid",chunkid},
        {"offset",offset},
        {"chunksize",chunksize}
        };

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

void _downloadFile(string phase, string sid, string md5, int qid, ll offset, int chunksize)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", sid},
        {"md5",md5},
        {"queueid",qid},
        {"offset",offset},
        {"chunksize",chunksize}
        };

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

void _deleteFile(string phase, string session,string fname,string path,int qid)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", session},
        {"filename",fname},
        {"path",path},
        {"queueid",qid}
        };

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

void _createDir(string phase, string sid,string prefix,string dirname, int qid)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", sid},
        {"prefix",prefix},
        {"dirname",dirname},
        {"queueid",qid}
        };

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}

void _deleteDir(string phase, string sid, string prefix, string dirname, int qid)
{
    start(phase);
    string msg = phase + '\n';
    json req = {
        {"session", sid},
        {"dirname",dirname},
        {"prefix",prefix},
        {"queueid",qid}
        };

    send_header(msg, req);
    cout << phase << endl;
    cout << req << endl;
    recv_print();
    end(phase);
}




//=========================================

void test()
{
    // // 检查username
    // // 1 用户存在 -> 注册失败
    // // 2 否则 -> 注册成功

    // _signup("signup", "yszhou2019","yszhou2019");
    // _signup("signup", "root2019","yszhou2019");

    
    // // login
    // // 1 用户不存在 -> 失败
    // // 2 账号密码不匹配 -> 失败
    // // 3 成功
    // _login("login","root2019", "123");
    // _login("login","root2018", "123");
    // _login("login","root2017", "123");
    // _login("login","root2019", "yszhou2019");

    // // bind
    // // 1 session 不存在 -> 失败
    // // 2 bind失败
    // // 3 bind成功
    // _setbind("setbind",2,"WjkR4t94UJWfA47bivjv1TbFVYxA3nr8");
    // _setbind("setbind",1,"123");
    // _setbind("setbind",1,"321");

    // // disbind
    // // session 不存在 -> 重新登录
    // // 解绑成功
    // _disbind("disbind", "123");
    // _disbind("disbind", "321");
    // _disbind("disbind", "WjkR4t94UJWfA47bivjv1TbFVYxA3nr8");
    
    // // logout
    // // session 不存在 -> 已经退出
    // // session 存在 -> 销毁
    // _logout("logout","Y218XE860A1P6I80jWd8qsjq6ih80Vav");
    // _logout("logout","333");
    // _logout("logout","123");

    // // createDir
    // // 1 session 不存在 -> 重新登录
    // // 2 没有绑定 -> 绑定
    // // 3 创建目录成功
    // _createDir("createDir", "321", "/root", "doc", 100);
    // _createDir("createDir", "123", "/root", "doc", 100);
    // _createDir("createDir", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "/root", "doc", 100);
    // _createDir("createDir", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "/123", "doc", 100);
    // _createDir("createDir", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "/test/here", "doc", 100);


    // // deleteDir
    // // 1 session 不存在 -> 重新登录
    // // 2 没有绑定 -> 绑定
    // // 3 删除目录成功
    // _deleteDir("deleteDir", "321", "/root", "doc", 100);
    // _deleteDir("deleteDir", "123", "/root", "doc", 100);
    // _deleteDir("deleteDir", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "/root", "doc", 100);
    // _deleteDir("deleteDir", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "/test/here", "doc", 100);

    // // uploadFile
    // // 1 session 不存在 -> 重新登录 done
    // // 2 没有绑定 -> 绑定 done
    // // 3 md5对应的文件存在 -> 秒传 done
    // //      
    // // 4 md5对应的文件不存在 -> 
    // _uploadFile("uploadFile", "dcba", 100, "test.doc", "/root", "abcd123", 100, 2021);
    // _uploadFile("uploadFile", "123", 100, "test.doc", "/root", "abcd123", 100, 2021);
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "test.doc", "/root123", "233", 100, 8*1024*1024+1);
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "test.doc", "/root123", "666", 8*1024*1024+1, 100);
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "tt.doc", "/root123", "777", 1025, 100);
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "test.doc", "/root", "321dcba", 8*1024*1024+1, 2021);
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "test.doc", "/test/here", "abcd123", 100, 2021);
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "test.doc", "/test/here", "321dcba", 8*1024*1024+1, 2021);
    // long long sz = ll(4) * 1024 * 1024 * 1024;
    // _uploadFile("uploadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 100, "bigfile.doc", "/test/here", "bigfile", sz, 2021);

    // // deleteFile
    // // 1 session 不存在 -> 重新登录 done
    // // 2 没有绑定 -> 绑定 done
    // // 3 删除成功
    // _deleteFile("deleteFile", "666", "test.doc", "/root123", 100);
    // _deleteFile("deleteFile", "123", "test.doc", "/root123", 100);
    // _deleteFile("deleteFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "test.doc", "/root123", 100);

    // // uploadChunk
    // // 1 session 不存在 -> 重新登录 done
    // // 2 没有绑定 -> 绑定 done
    // // 3 删除成功
    // _uploadChunk("uploadChunk", "666", 15, 100, 0, 0, 4 * 1024 * 1024);
    // _uploadChunk("uploadChunk", "123", 15, 100, 0, 0, 4 * 1024 * 1024);
    // _uploadChunk("uploadChunk", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", 15, 100, 0, 0, 1025);

    // // downloadFile
    // // 1 session 不存在 -> 重新登录 done
    // // 2 没有绑定 -> 绑定 done
    // // 3 下载成功
    // _downloadFile("downloadFile", "666", "666", 100, 100, 100); // no session
    // _downloadFile("downloadFile", "aaa", "666", 100, 100, 100); // no bind
    // _downloadFile("downloadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "666", 100, 100, 100); // 应该检测到vfile中没有对应的md5文件而失败
    // _downloadFile("downloadFile", "3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU", "777", 100, 100, 100); // 成功
}

// _login("login","root2019", "yszhou2019");
// _setbind("setbind",2,"3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU");

int main(int argc, char** argv)
{
    srand(time(NULL));
    Choice options[] = {
        {"--port", req_arg, must_appear, is_int, {.int_val = &port}, 0, 65535, 4000},
        {"--myport", req_arg, must_appear, is_int, {.int_val = &myport}, 0, 65535, 4000},
    };
    my_handle_option(options, 1, argc, argv);
    myport = rand() % 1000 + 4000;
    


    test();


    // char buffer[200] = "logout\n{\"session\":\"b09e4c9a4c9eb7eb4a5d58c7e78815db\"}\0";
    // int res = write(sock, buffer, strlen(buffer) + 1);
    // memset(buffer, 200, 0xff);
    // res = read(sock, buffer, 100);
    // cout << "读取字节" << res << " bytes" << endl;
    // for (int i = 0; i < 10;i++)
    // {
    //     printf("%x %x %x %x   %x %x %x %x\n", buffer[i * 8], buffer[i * 8 + 1], buffer[i * 8 + 2], buffer[i * 8 + 3], buffer[i * 8 + 4], buffer[i * 8 + 5], buffer[i * 8 + 6], buffer[i * 8 + 7]);
    // }

    // strcpy(buffer, "hello");
    // res = write(sock, buffer, strlen(buffer));
    // cout << "发送字节数量 " << res << endl;
    // cout << buffer << endl;

    // close(sock);
    return 0;
}