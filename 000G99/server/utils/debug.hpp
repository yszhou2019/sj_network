#include <iostream>
#include "json.hpp"
#include <string>
using string = std::string;
using json = nlohmann::json;
using std::cout;
using std::endl;

void print_json(string type, u_char* buffer, int len)
{
    std::cout << "type:" << type << std::endl;
    std::cout << "type length:" << type.size() << std::endl;
    std::cout << "json内容" << std::endl;
    for (int i = 0; i < len; i++)
    {
        std::cout << buffer[i];
    }
    std::cout << std::endl;
    std::cout << "json length: " << len << std::endl;
}

void print_buffer(u_char* buffer, int len)
{
    len = (len > 100 ? 20 : len);

    for (int i = 0; i < len + 1; i++)
    {
        printf("%x ", buffer[i]);
    }
    cout << endl;
    for (int i = 0; i < len + 1;i++)
    {
        printf("%c ", buffer[i]);
    }
    cout << endl;
    cout << "一共 " << len << " 字节" << endl;
}