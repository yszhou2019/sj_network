  
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

/**
 * input: ������temp
 * 
 * "��0��Ŀǰ�Ѳ��õ���ҵ��[6]�⣬���ύ[5]�⣬����[0]���Ѵ�����ύ����"
 *                         ~~          ~~         ~~
 * ��ȡ��3���������ֱ����ܹ���Ŀ���������ύ��Ŀ�������Ѵﵽ���޵���Ŀ����
 * ע�⣬��ȡ��ʱ�򣬲���ֻ��\\d{1}������ȡ����Ŀ������������λ������λ��
 * output : ��ȡ���������ִ�ӡ����
 */ 
void search_1(string temp){
    regex pattern("\\[\\d+\\]");
    smatch result;
    string::const_iterator iter_begin = temp.cbegin();
    string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "���ҳɹ���" << result[0] << endl;
        std::string res = result[0];
        //cout << "���ҳɹ���" << res << endl;
        string snum = res.substr(1, res.size()-2);
        //cout<< snum <<endl;

        int number = atoi(snum.c_str());
        cout<<number<<endl;

        iter_begin = result[0].second;
    }
    return ;
}

/**
 * input: ������temp
 * "onChange(1, 1, 6, 'test1.tar.bz2', 1)"
 *           ~~        ~~~~~~~~~~~~~
 * "onChange(2, 1, 6, 'test2.pdf', 1)"
 *           ~~        ~~~~~~~~~
 * "onChange(3, 1, 6, 'test3.pdf', 1)"
 *           ~~        ~~~~~~~~~
 *           �ļ����   �ļ���
 * output: forѭ����ѭ���ڲ�����ӡ "�ļ����" "�ļ���"
 * 
 */ 
void search_2(string temp){
    regex pattern("onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    smatch result;
    string::const_iterator iter_begin = temp.cbegin();
    string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "���ҳɹ���" << result[0] << endl;
        std::string res = result[1];
        cout << "��ҵ���:" << res << endl;
        std::string res2 = result[2];
        cout << "�ļ�����:" << res2 << endl;

        iter_begin = result[0].second;
    }
    return ;
}


/**
 * ���涼��ɵĻ�������пգ���search_2�Ļ����ϣ���ÿ���ļ��ĵ�ǰ�ύ������������
 * "<select name="date" align="left" width="200"><option value="date2" selected disabled>��2��&nbsp-81:50:18&nbsp</option><option
 *                                                                                         ~
 *  value="date1" disabled>��1��&nbsp-126:26:35</option></select></td><td>100%</td><td><input type="hidden" name="MAX_FILE_SIZE1" 
 * 
 * value=65536 /><input type="file" id="browse1" name="upfile1" onchange="onChange(1, 1, 6, 'test1.tar.bz2', 1)" />"
 *                                                                                 ~         ~~~~~~~~~~~~~
 * output: forѭ����ѭ���ڲ�����ӡ"�ύ����" "�ļ����" "�ļ���"
 */ 
void search_3(string temp){
    regex pattern("\"date\\d+\"[\\s\\w]*>��(\\d+)��[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    smatch result;
    string::const_iterator iter_begin = temp.cbegin();
    string::const_iterator iter_end = temp.cend();

    while(regex_search(iter_begin, iter_end, result, pattern))
    {
        //cout << "���ҳɹ���" << result[0] << endl;

        std::string res = result[1];
        cout << "�ύ����:" << res << endl;

        std::string res2 = result[2];
        cout << "��ҵ���:" << res2 << endl;

        std::string res3 = result[3];
        cout << "�ļ�����:" << res3 << endl;

        iter_begin = result[0].second;
    }
    
    
    regex pattern2("\"date\\d+\"[\\s\\w]*>(��δ�ύ)[^(]*onChange\\((\\d+)[\\d,\\s]*'([^<>/\\|:\"\\*\?]+\\.\\w+)'");
    smatch result2;
    string::const_iterator iter_begin2 = temp.cbegin();
    string::const_iterator iter_end2 = temp.cend();

    while(regex_search(iter_begin2, iter_end2, result2, pattern2))
    {
        //cout << "���ҳɹ���" << result2[0] << endl;

        std::string res = result2[1];
        cout << "�ύ����:" << res << endl;

        std::string res2 = result2[2];
        cout << "��ҵ���:" << res2 << endl;

        std::string res3 = result2[3];
        cout << "�ļ�����:" << res3 << endl;

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