#include "json.hpp"
#include <string>



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

// string generate_pname_by_md5(const string md5)
// {
//     return md5;
// }


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





//==========================================
// deprecated

// // 回答，应该采用readn和writen，因为本身在loop前执行过epoll_wait了，而且不同的操作类型是不一样的，不能像http server那样
// int        /* Read "n" bytes from a descriptor */
// readn(int fd, u_char *ptr, int n)
// {
//     int     nleft;
//     int    nread;

//     nleft = n;
//     while (nleft > 0)
//     {
//         if ((nread = read(fd, ptr, nleft)) < 0)
//         {
//             if (nleft == n)
//                 return(-1);    /* error, return -1 */
//             else
//                 break;        /* error, return amount read so far */
//         }
//         else if (nread == 0)
//         {
//             break;            /* EOF */
//         }
//         nleft -= nread;
//         ptr   += nread;
//     }
//     return(n - nleft);             /* return >= 0 */
// }

// int        /* Write "n" bytes to a descriptor */
// writen(int fd, const u_char *ptr, int n)
// {
//     int    nleft;
//     int   nwritten;

//     nleft = n;
//     while (nleft > 0)
//     {
//         if ((nwritten = write(fd, ptr, nleft)) < 0)
//         {
//             if (nleft == n)
//                 return(-1);    /* error, return -1 */
//             else
//                 break;        /* error, return amount written so far */
//         }
//         else if (nwritten == 0)
//         {
//             break;            
//         }
//         nleft -= nwritten;
//         ptr   += nwritten;
//     }
//     return(n - nleft);    /* return >= 0 */
// }


// /**
//  * 判断文件是否存在
//  */ 
// bool if_file_exist(const string& filename)
// {
//     return access(filename.c_str(), F_OK) == 0;
// }