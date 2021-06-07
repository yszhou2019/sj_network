  
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

/**
 * input: 见附件temp
 * 
 * "第0章目前已布置的作业共[6]题，已提交[5]题，其中[0]题已达最大提交次数"
 *                         ~~          ~~         ~~
 * 提取出3个整数，分别是总共题目数量，已提交题目数量，已达到上限的题目数量
 * 注意，提取的时候，不能只用\\d{1}进行提取，题目数量可能是两位数，三位数
 * output : 提取出来的数字打印出来
 */ 
void search_1(string temp){
    regex pattern("\\[\\d+\\]");
    smatch result;
    string::const_iterator iter_begin = temp.cbegin();
    string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "查找成功：" << result[0] << endl;
        std::string res = result[0];
        //cout << "查找成功：" << res << endl;
        string snum = res.substr(1, res.size()-2);
        //cout<< snum <<endl;

        int number = atoi(snum.c_str());
        cout<<number<<endl;

        iter_begin = result[0].second;
    }
    return ;
}

/**
 * input: 见附件temp
 * "onChange(1, 1, 6, 'test1.tar.bz2', 1)"
 *           ~~        ~~~~~~~~~~~~~
 * "onChange(2, 1, 6, 'test2.pdf', 1)"
 *           ~~        ~~~~~~~~~
 * "onChange(3, 1, 6, 'test3.pdf', 1)"
 *           ~~        ~~~~~~~~~
 *           文件序号   文件名
 * output: for循环，循环内部，打印 "文件序号" "文件名"
 * 
 */ 
void search_2(string temp){
    regex pattern("onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    smatch result;
    string::const_iterator iter_begin = temp.cbegin();
    string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "查找成功：" << result[0] << endl;
        std::string res = result[1];
        cout << "作业序号:" << res << endl;
        std::string res2 = result[2];
        cout << "文件名称:" << res2 << endl;

        iter_begin = result[0].second;
    }
    return ;
}


/**
 * 上面都完成的话，如果有空，在search_2的基础上，把每个文件的当前提交次数解析出来
 * "<select name="date" align="left" width="200"><option value="date2" selected disabled>第2次&nbsp-81:50:18&nbsp</option><option
 *                                                                                         ~
 *  value="date1" disabled>第1次&nbsp-126:26:35</option></select></td><td>100%</td><td><input type="hidden" name="MAX_FILE_SIZE1" 
 * 
 * value=65536 /><input type="file" id="browse1" name="upfile1" onchange="onChange(1, 1, 6, 'test1.tar.bz2', 1)" />"
 *                                                                                 ~         ~~~~~~~~~~~~~
 * output: for循环，循环内部，打印"提交次数" "文件序号" "文件名"
 */ 
void search_3(string temp){
    regex pattern("\"date\\d+\"[\\s\\w]*>第(\\d+)次[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    smatch result;
    string::const_iterator iter_begin = temp.cbegin();
    string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "查找成功：" << result[0] << endl;

        std::string res = result[1];
        cout << "提交次数:" << res << endl;

        std::string res2 = result[2];
        cout << "作业序号:" << res2 << endl;

        std::string res3 = result[3];
        cout << "文件名称:" << res3 << endl;

        iter_begin = result[0].second;
    }
    
    
    regex pattern2("\"date\\d+\"[\\s\\w]*>(尚未提交)[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    smatch result2;
    string::const_iterator iter_begin2 = temp.cbegin();
    string::const_iterator iter_end2 = temp.cend();

    while(regex_search(iter_begin2, iter_end2, result2, pattern2))
    {
        //cout << "查找成功：" << result2[0] << endl;

        std::string res = result2[1];
        cout << "提交次数:" << res << endl;

        std::string res2 = result2[2];
        cout << "作业序号:" << res2 << endl;

        std::string res3 = result2[3];
        cout << "文件名称:" << res3 << endl;

        iter_begin2 = result2[0].second;
    }

    return ;
}

int main(){
    fstream file;
    string temp;
    file.open("tmp", ios::in);
    getline(file, temp);
    file.close();

    //printf("%s\n", temp.c_str());
    search_1(temp);
    search_2(temp);
    search_3(temp);

    return 0;
}