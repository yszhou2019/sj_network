#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <iostream>
using namespace std;

int main()
{
    // auto t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // cout<< std::put_time(std::localtime(&t), "%Y-%m-%d %X")<<endl;
    // cout<< std::put_time(std::localtime(&t), "%Y-%m-%d %H.%M.%S")<<endl;
    struct tm* t;
    time_t now;
    time(&now);
    t = localtime(&now);
    printf("%d-%d-%d %d:%d:%d ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    return 0;
}