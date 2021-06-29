
#include "json.hpp"
#include <string>

/* pipe */
#include <unistd.h>

/* splice */
#include <fcntl.h>


using string = std::string;
using json = nlohmann::json;
typedef unsigned long long ull;

const ull CHUNK_SIZE = 4 * 1024 * 1024;

int get_chunks_num(ull size)
{
    return static_cast<int>(size / CHUNK_SIZE + (size % CHUNK_SIZE == 0 ? 0 : 1));
}

json generate_chunks_info(ull size, int chunk_num)
{
    json res;
    json temp;
    json total;
    for (int i = 0; i < chunk_num; i++)
    {
        ull chunk_bytes = size >= CHUNK_SIZE ? CHUNK_SIZE : size;
        temp.clear();
        temp.push_back(i * CHUNK_SIZE);
        temp.push_back(chunk_bytes);
        temp.push_back(0);
        total.push_back(temp);
        size -= chunk_bytes;
    }
    res["chunks"] = total;
    return res;
}

/**
 * 保证socket正常，文件存在，文件大小合适的情况下
 * 将数据写入指定文件中
 * 返回写入字节数量
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
    return w_bytes;
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

    ssize_t r_bytes = splice(fd, &offset, pipefd[1], NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);

    ssize_t w_bytes = splice(pipefd[0], NULL, sock, NULL, chunksize, SPLICE_F_MORE | SPLICE_F_MOVE);
    printf("read %ld from file, write %ld bytes to sock\n", r_bytes, w_bytes);

    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    return w_bytes;
}

string get_filename_by_md5(const string& md5)
{
    return md5;
}

/**
 * 根据文件名，创建指定的文件名
 * 分配指定大小的空间
 * 完成成功，返回 false
 * 创建失败，返回 true
 */ 
bool create_file_allocate_space(const string& filename, off_t size)
{
    // fallocate(fd,mod,off,)
    bool create_file_fail = false;

    int fd;
    if (access(filename.c_str(), F_OK) == -1)
    {
        if(fd = open(filename.c_str(),O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR)<0){
            fd = creat(filename.c_str(), O_WRONLY);
        }
        fallocate(fd, 0, 0, size);
    }
    else {
        fd = open(filename.c_str(), O_WRONLY | O_CREAT);
    }
    u_char buffer[2048];
    readn(sinfo->sock, buffer, size);
    writen(fd, buffer, size);
    
    return create_file_fail;
}

string encode(const string& pwd)
{

}

/**
 * 应当保证 session的绝对唯一 
 */
string generate_session()
{

}


/**
 * 根据md5来判断文件是否存在
 */ 
bool if_file_exist(const string& md5)
{
    return access(md5.c_str(), F_OK) == 0;
}