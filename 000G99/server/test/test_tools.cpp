
#include <string>
#include <stdio.h>
#include <iostream>

#include "../utils/tools.hpp"
using string = std::string;
using json = nlohmann::json;
using namespace std;

void print(json &info)
{
    json chunks = info["chunks"];
    // for (auto it = chunks.begin();it!=chunks.end();it++)
    // {
    //     printf("%ld %ld %ld\n", it.value()[0].get<long>(), it.value()[1].get<long>(), it.value()[2].get<long>());
    // }
    for (auto it = 0; it != chunks.size();it++)
    {
        json chunk = chunks[it];
        printf("%ld %ld %ld\n", chunk[0].get<ull>(), chunk[1].get<ull>(), chunk[2].get<ull>());
    }
    cout << "Ò»¹² " << chunks.size() << " ¿é" << endl;
    // cout << chunks << endl;
    return;
}


void init_file()
{
    int fd = open("/store/f1", O_RDWR | O_CREAT, 766);
    fallocate(fd, 0, 0, 100);
    close(fd);
}

void test()
{
    // ull size = ull(4) * 1024 * 1024 * 1024;
    // int chunk_num = get_chunks_num(size);
    // json info = generate_chunks_info(size, chunk_num);
    // print(info);
    
    // info.clear();
    // size = 300;
    // chunk_num = get_chunks_num(size);
    // info = generate_chunks_info(size, chunk_num);
    // print(info);

    // info.clear();
    // size = 4*1024*1024;
    // chunk_num = get_chunks_num(size);
    // info = generate_chunks_info(size, chunk_num);
    // print(info);

    // info.clear();
    // size = 4 * 1024 * 1024 + 1;
    // chunk_num = get_chunks_num(size);
    // info = generate_chunks_info(size, chunk_num);
    // print(info);

    // cout << create_file_allocate_space("store/f1", 100) << endl;
    // cout << create_file_allocate_space("store/f2", 1024*1024) << endl;

    // cout << encode("abcd123").size() << endl;
    // cout << encode("root123").size() << endl;

    // cout << generate_session() << endl;
    // cout << generate_session() << endl;
}

int main()
{
    test();
    return 0;
}