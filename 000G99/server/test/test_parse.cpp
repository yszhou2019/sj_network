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
#include <iostream>
#include "../utils/json.hpp"
#include <ctime>
using namespace std;
using json = nlohmann::json;

int main()
{
    char buffer[200] = "{\"username\": \"yszhou\", \"password\":\"yszhou123\"}";
    // if (len == -1)
    // {
    //     printf("buffer: %s\n", buffer);
    // }
    // printf("parse int %d\n", len);
    // auto header = json::parse(buffer, buffer + len + 1);
    json header = json::parse(buffer);
    // buffer_len = 0;
    cout << header << endl;
    return 0;
}