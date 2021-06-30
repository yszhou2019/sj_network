
#include "json.hpp"
#include <string>

/* pipe */
#include <unistd.h>

/* splice */
#include <fcntl.h>

/* select */
#include <sys/select.h>

/* fstat */
#include <sys/stat.h>

/* rand */
#include <stdlib.h>

/* openssl */
#include "openssl/sha.h"


using string = std::string;
using json = nlohmann::json;
typedef long long ll;

const ll CHUNK_SIZE = 4 * 1024 * 1024;

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
 * 最多等待1秒
 */ 
bool sock_ready_to_read(int sock)
{
    printf("检查socket是否就绪\n");
    fd_set rfds;
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    int res = select(sock + 1, &rfds, NULL, NULL, &tv);
    if(FD_ISSET(sock, &rfds))
    {
        return true;
    }
    return false;
}

bool sock_ready_to_write(int sock)
{
    printf("检查socket是否就绪\n");
    fd_set wfds;
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    int res = select(sock + 1, NULL, &wfds, NULL, &tv);
    if(FD_ISSET(sock, &wfds))
    {
        return true;
    }
    return false;
}

off_t get_file_size(const string& filename)
{
    int fd = open(filename.c_str(), O_RDONLY);
    struct stat buf;
    fstat(fd, &buf);
    close(fd);
    return buf.st_size;
}

/**
 * 保证socket正常，文件存在，文件大小合适的情况下
 * 将数据写入指定文件中
 * 返回写入字节数量
 */ 
ssize_t write_to_file(int sock, const string& filename, loff_t offset, size_t chunksize)
{

    off_t filesize = get_file_size(filename);
    if(offset + chunksize > filesize)
        return 0;

    int pipefd[2];
    pipe(pipefd);

    // socket -> data -> file
    int fd = open(filename.c_str(), O_WRONLY, 766);

    ssize_t cnt = 0;

    while(cnt != chunksize)
    {
        bool ready = sock_ready_to_read(sock);
        // printf("socket %s\n", ready ? "就绪" : "放弃");
        if (!ready)
        {
            close(fd);
            close(pipefd[0]);
            close(pipefd[1]);
            // 超出了可以忍受的时间，还没有收到足够的字节数量，就不再等待
            return 0;
        }

        ssize_t r_bytes = splice(sock, NULL, pipefd[1], NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);
        ssize_t w_bytes = splice( pipefd[0], NULL, fd, &offset, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE );
        printf("read %ld bytes from sock, write %ld bytes to file\n", r_bytes, w_bytes);

        if(w_bytes == 0)
            break;
        if(w_bytes == -1)
            return -1;
        cnt += w_bytes;
        offset += w_bytes;
        chunksize -= w_bytes;
    }

    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    char temp;
    read(sock, &temp, 1);
    if(temp !='\0')
    {
        printf("read chunks without end_zero\n");
    }
    return cnt;
}

/**
 * 保证socket正常，文件存在，文件大小合适的情况下
 * 从文件中读取字节，发送给socket
 * 返回发送的字节数量
 */ 
ssize_t send_to_socket(int sock, const string& filename, loff_t offset, size_t chunksize)
{
    int pipefd[2];
    pipe(pipefd);

    // file -> data -> socket
    int fd = open(filename.c_str(), O_RDONLY, 766);

    ssize_t cnt = 0;

    while(cnt != chunksize)
    {
        bool ready = sock_ready_to_write(sock);
        if (!ready)
        {
            close(fd);
            close(pipefd[0]);
            close(pipefd[1]);
            // 超出了可以忍受的时间，还没有缓冲区可以发送，就不再等待
            return 0;
        }
        
        ssize_t r_bytes = splice(fd, &offset, pipefd[1], NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);
        ssize_t w_bytes = splice(pipefd[0], NULL, sock, NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);

        printf("read %ld from file, write %ld bytes to sock\n", r_bytes, w_bytes);
        if(w_bytes == 0)
            break;
        if(w_bytes == -1)
            return -1;
        cnt += w_bytes;
        offset += w_bytes;
        chunksize -= w_bytes;
    }


    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    char temp = '\0';
    write(sock, &temp, 1);
    return cnt;
}

/**
 * 前提，文件不存在
 * 根据文件名，创建指定的文件名
 * 分配指定大小的空间
 * 完成成功，返回 false
 * 创建失败，返回 true
 */ 
bool create_file_allocate_space(const string& filename, ll size)
{
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int res = fallocate(fd, 0, 0, size);
    close(fd);
    return res != 0 ;
}

/**
 * 判断文件是否存在
 */ 
bool if_file_exist(const string& filename)
{
    return access(filename.c_str(), F_OK) == 0;
}


/**
 * 对密码进行加密
 */ 
string encode(const string& pwd)
{
    unsigned char temp[33];
    SHA256((const unsigned char*)pwd.c_str(), pwd.length(), temp); // 调用sha256哈希
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
 * 应当保证 session的绝对唯一 
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
    bool ready = sock_ready_to_read(sock);
    if (!ready)
    {
        return;
    }
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

// 回答，应该采用readn和writen，因为本身在loop前执行过epoll_wait了，而且不同的操作类型是不一样的，不能像http server那样
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