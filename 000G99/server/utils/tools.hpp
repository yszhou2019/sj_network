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





//==========================================
// deprecated

// // �ش�Ӧ�ò���readn��writen����Ϊ������loopǰִ�й�epoll_wait�ˣ����Ҳ�ͬ�Ĳ��������ǲ�һ���ģ�������http server����
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
//  * �ж��ļ��Ƿ����
//  */ 
// bool if_file_exist(const string& filename)
// {
//     return access(filename.c_str(), F_OK) == 0;
// }