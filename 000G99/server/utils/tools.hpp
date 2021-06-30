
#include "json.hpp"
#include <string>

/* pipe */
#include <unistd.h>

/* splice */
#include <fcntl.h>

/* rand */
#include <stdlib.h>

/* openssl */
#include "openssl/sha.h"


using string = std::string;
using json = nlohmann::json;
typedef long long ll;

const ll CHUNK_SIZE = 4 * 1024 * 1024;
const string StorePath = "store/";

//=====================================
// DONE

int get_chunks_num(ll size)
{
    return static_cast<int>(size / CHUNK_SIZE + (size % CHUNK_SIZE == 0 ? 0 : 1));
}

json generate_chunks_info(ll size, int chunk_num)
{
    json res;
    json temp;
    // json total;
    for (int i = 0; i < chunk_num; i++)
    {
        ll chunk_bytes = size >= CHUNK_SIZE ? CHUNK_SIZE : size;
        temp.clear();
        temp.push_back(i * CHUNK_SIZE);
        temp.push_back(chunk_bytes);
        temp.push_back(0);
        res.push_back(temp);
        size -= chunk_bytes;
    }
    // res["chunks"] = total;
    return res;
}

/**
 * ��֤socket�������ļ����ڣ��ļ���С���ʵ������
 * ������д��ָ���ļ���
 * ����д���ֽ�����
 */ 
ssize_t write_to_file(int sock, const string& filename, loff_t offset, size_t chunksize)
{
    int pipefd[2];
    pipe(pipefd);

    // socket -> data -> file
    int fd = open(filename.c_str(), O_WRONLY, 766);

    ssize_t r_bytes = splice(sock, NULL, pipefd[1], NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);

    ssize_t w_bytes = splice( pipefd[0], NULL, fd, &offset, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE );
    printf("read %ld bytes from sock, write %ld bytes to file\n", r_bytes, w_bytes);

    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    char temp;
    read(sock, &temp, 1);
    if(temp !='\0')
    {
        printf("read chunks without end_zero\n");
    }
    return w_bytes;
}

/**
 * ��֤socket�������ļ����ڣ��ļ���С���ʵ������
 * ���ļ��ж�ȡ�ֽڣ����͸�socket
 * ���ط��͵��ֽ�����
 */ 
ssize_t send_to_socket(int sock, const string& filename, loff_t offset, size_t chunksize)
{
    int pipefd[2];
    pipe(pipefd);

    // file -> data -> socket
    int fd = open(filename.c_str(), O_RDONLY, 766);

    ssize_t r_bytes = splice(fd, &offset, pipefd[1], NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);

    ssize_t w_bytes = splice(pipefd[0], NULL, sock, NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);
    printf("read %ld from file, write %ld bytes to sock\n", r_bytes, w_bytes);

    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    char temp = '\0';
    write(sock, &temp, 1);
    return w_bytes;
}

/**
 * ǰ�ᣬ�ļ�������
 * �����ļ���������ָ�����ļ���
 * ����ָ����С�Ŀռ�
 * ��ɳɹ������� false
 * ����ʧ�ܣ����� true
 */ 
bool create_file_allocate_space(const string& filename, ll size)
{
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int res = fallocate(fd, 0, 0, size);
    close(fd);
    return res != 0 ;
}

/**
 * �ж��ļ��Ƿ����
 */ 
bool if_file_exist(const string& filename)
{
    return access(filename.c_str(), F_OK) == 0;
}

/**
 * 
 */ 
string get_filename_by_md5(const string& md5)
{
    return StorePath + md5;
}

/**
 * ��������м���
 */ 
string encode(const string& pwd)
{
    unsigned char temp[33];
    SHA256((const unsigned char*)pwd.c_str(), pwd.length(), temp); // ����sha256��ϣ
    char res[65] = {0};
    char t[3] = {0};
    for (int i = 0; i < 32;i++)
    {
        sprintf(t, "%02x", temp[i]);
        strcat(res, t);
    }
    res[32] = '\0';
    return string(res);
}

/**
 * DONE
 * Ӧ����֤ session�ľ���Ψһ 
 */
string generate_session()
{
    string res;
    for (int i = 0; i < 32;i++)
    {
        int temp = rand() % 3;
        if(temp == 0)
        {
            res += rand() % 26 + 'a';
        }
        else if(temp == 1)
        {
            res += rand() % 26 + 'A';
        }
        else{
            res += rand() % 10 + '0';
        }
    }
    return res;
}


void discard_extra(int sock, size_t chunksize)
{
    u_char buffer[2048];
    size_t res = chunksize;
    while(res != 0)
    {
        auto should_read_bytes = (res >= 2048 ? 2048 : res);
        auto actural_read = read(sock, buffer, should_read_bytes);
        if(actural_read == -1)
            actural_read = 0;
        res -= actural_read;
    }
    read(sock, buffer, 1);
}



//==========================================
// deprecated

// �ش�Ӧ�ò���readn��writen����Ϊ������loopǰִ�й�epoll_wait�ˣ����Ҳ�ͬ�Ĳ��������ǲ�һ���ģ�������http server����
int        /* Read "n" bytes from a descriptor */
readn(int fd, u_char *ptr, int n)
{
    int     nleft;
    int    nread;

    nleft = n;
    while (nleft > 0)
    {
        if ((nread = read(fd, ptr, nleft)) < 0)
        {
            if (nleft == n)
                return(-1);    /* error, return -1 */
            else
                break;        /* error, return amount read so far */
        }
        else if (nread == 0)
        {
            break;            /* EOF */
        }
        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);             /* return >= 0 */
}

int        /* Write "n" bytes to a descriptor */
writen(int fd, const u_char *ptr, int n)
{
    int    nleft;
    int   nwritten;

    nleft = n;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) < 0)
        {
            if (nleft == n)
                return(-1);    /* error, return -1 */
            else
                break;        /* error, return amount written so far */
        }
        else if (nwritten == 0)
        {
            break;            
        }
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n - nleft);    /* return >= 0 */
}