#include <regex>
#include <string>
#include <vector>
#include <fstream>
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