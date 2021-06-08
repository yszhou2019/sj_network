  
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <iomanip>
#include <assert.h>
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

struct Homework{
    int submitNum;
    string homeworkName;
};

map<int, Homework> search_3(string temp){

    map<int, Homework> resmap;

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

        int subnum = atoi(res.c_str());
        int workno = atoi(res2.c_str());
        Homework homework={subnum, res3};
        resmap.insert({workno, homework});

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

        int workno = atoi(res2.c_str());
        Homework homework = {0, res3};
        resmap.insert({workno, homework});

        iter_begin2 = result2[0].second;
    }

    return resmap;
}

struct RET_INFO{
    bool isSucc;
    string info;
};
/**
 * ������������phase_2.log�У������������һ����alert()��Ϣ
 * 
 * ���磬������һ��alert����
 * alert('�ļ��ϴ���Ϣ��\ntest4.cpp���ɹ�(53�ֽ�)\n')
 *        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ��ô������ֵ��{
 *      isSucc: true,
 *      info: "�ļ��ϴ���Ϣ��\ntest4.cpp���ɹ�(53�ֽ�)\n"
 * }
 * 
 * ���磬������һ��alert����
 * alert('�ļ��ϴ���Ϣ��\ntest6-socket-tcp-sync.pdf��ʧ��(�����ύ���ټ��30��)\n')
 *        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ����ֵ{
 *      isSucc: false,
 *      info: "�ļ��ϴ���Ϣ��\ntest6-socket-tcp-sync.pdf��ʧ��(�����ύ���ټ��30��)\n"
 * }
 * 
 */ 
RET_INFO search_4(){
    ifstream in;
    in.open("phase_2.log", ios::in);
    RET_INFO res;
    // ...
    in.close();
    return res;
}


/**
 * uri�����ַ���.
 * 
 * \param in ������utf-8����ģ�
 * \return uri�������ַ�����
 */
std::string encodeURIComponent(std::string in)
{
    std::stringstream ssUri;
    for (char c : in)
    {
        //0-9A-Za-z-_.!~*'()
        if ((c >= 'a' && c <= 'z') 
            || (c >= 'A' && c <= 'Z') 
            || (c >= '0' && c <= '9')
            || c == '!' 
            || (c >= '\'' && c <= '*') 
            || c == '-' 
            || c == '.' 
            || c== '_' 
            || c== '~')
        {
            ssUri << c;
        }
        else
        {
            ssUri << "%";
            if ((c & 0xf0) == 0)
                ssUri << 0;
            ssUri << setiosflags(ios::uppercase) << hex << (c & 0xff);
        }
    }
    return ssUri.str();
}

unsigned char ToHex(unsigned char x) 
{ 
    return  x > 9 ? x + 55 : x + 48; 
}

unsigned char FromHex(unsigned char x) 
{ 
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) || 
            (str[i] == '-') ||
            (str[i] == '_') || 
            (str[i] == '.') || 
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

int main(){
    map<int, Homework> resmap;
    fstream file;
    string temp;
    file.open("tmp", ios::in);
    getline(file, temp);
    file.close();

    //printf("%s\n", temp.c_str());
    search_1(temp);
    search_2(temp);
    resmap = search_3(temp);

    map<int, Homework>::iterator iter;
    iter = resmap.begin();
    while(iter!=resmap.end())
    {
        cout<<iter->first<<" "<<iter->second.submitNum<<" "<<iter->second.homeworkName<<endl;
        iter++;
    }

    cout << encodeURIComponent("TEMPpwd060!") << endl;
    cout << encodeURIComponent("~and!and/and\\and%") << endl;
    cout << UrlEncode("TEMPpwd060!") << endl;
    cout << UrlEncode("~and!and/and\\and%") << endl;
    return 0;
}