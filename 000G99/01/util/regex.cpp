#include <regex>
#include <string>
#include <vector>
#include <fstream>
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

}

int main(){
    fstream file;
    string temp;
    file.open("temp", ios::in);
    getline(file, temp);
    file.close();

    search_1(temp);
    search_2(temp);
    search_3(temp);

    return 0;
}