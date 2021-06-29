//mysql_demo.cpp
#include <iostream>	// cin,cout��
#include <iomanip>	// setw��
#include <mysql.h>	// mysql����

#include "../utils/db.hpp"
using namespace std;


MYSQL     *db;  


int demo()
{
    MYSQL_RES *result;   
    MYSQL_ROW  row;
    /* ���в�ѯ���ɹ�����0�����ɹ���0
       1����ѯ�ַ��������﷨����
       2����ѯ�����ڵ����ݱ� */
    if (mysql_query(db, "select * from user")) {
    	cout << "mysql_query failed(" << mysql_error(db) << ")" << endl;
    	return -1;
    	}

    /* ����ѯ����洢���������ִ����򷵻�NULL
       ע�⣺��ѯ���ΪNULL�����᷵��NULL */
    if ((result = mysql_store_result(db))==NULL) {
    	cout << "mysql_store_result failed" << endl;
    	return -1;
    	}

    /* ��ӡ��ǰ��ѯ���ļ�¼������ */
    cout << "select return " << (int)mysql_num_rows(result) << " records" << endl;

    /* ѭ����ȡ�������������ļ�¼
	   1�����ص���˳����selectָ������˳����ͬ����row[0]��ʼ
	   2���������ݿ�����ʲô���ͣ�C�ж��������ַ��������д�������б�Ҫ����Ҫ�Լ�����ת��
	   3�������Լ�����Ҫ��֯�����ʽ */
    while((row=mysql_fetch_row(result))!=NULL) {
        cout << setiosflags(ios::left);             //��������
        cout << "id" << setw(12) << row[0];     //���12λ�������
        cout << "name" << setw(8)  << row[1];     //    8
        cout << "pwd" << setw(4)  << row[2];     //    4
        cout << "islogin" << setw(4)  << row[3];     //    4
        cout << "bindid" << setw(4)  << row[4];     //    4
        cout << "session" << setw(4)  << row[5];     //    4
        cout << endl;
        }

    /* �ͷ�result */
    mysql_free_result(result);
    return 1;
}


//===================================
// test


void test()
{

    // cout << "get_uid_by_session: " << get_uid_by_session("123") << endl;
    // cout << "disbind_dir: " << disbind_dir(123) << endl;
    // cout << "create_user: " << create_user("123","root123") << endl;

    cout << "check" << endl;
    // cout << "if user exist:" << endl;
    // cout << (if_user_exist("root") == true ? "right" : "wrong") << endl;
    // cout << (if_user_exist("123") == true ? "right" : "wrong") << endl;
    // cout << (if_user_exist("321") != true ? "right" : "wrong") << endl;

    // cout << (get_uid_by_name_pwd("root","root123") != -1 ? "right" : "wrong") << endl;
    // cout << (get_uid_by_name_pwd("123","root123") != -1 ? "right" : "wrong") << endl;
    // cout << (get_uid_by_name_pwd("321","root123") == -1 ? "right" : "wrong") << endl;
    // cout << (get_uid_by_name_pwd("321","333") == -1 ? "right" : "wrong") << endl;

    // cout << (write_session(1,"333") == false ? "right" : "wrong") << endl;
    // cout << (write_session(1,"333") == false ? "right" : "wrong") << endl;
    // cout << (write_session(1,"abcd") == false ? "right" : "wrong") << endl;

    // cout << (destroy_session("333") == false ? "right" : "wrong") << endl;
    // cout << (destroy_session("333") == false ? "right" : "wrong") << endl;
    // cout << (destroy_session("abcd") == false ? "right" : "wrong") << endl;
    // cout << (destroy_session("aaa") == false ? "right" : "wrong") << endl;

    // cout << (set_bind_dir(1, 2) == false ? "right" : "wrong") << endl;
    // cout << (set_bind_dir(2, 2) == false ? "right" : "wrong") << endl;
    // cout << (set_bind_dir(3, 2) == false ? "right" : "wrong") << endl;
    // cout << (set_bind_dir(4, 2) == false ? "right" : "wrong") << endl;
    // cout << (set_bind_dir(9, 2) == false ? "right" : "wrong") << endl;
    
    // cout << (get_bindid_by_uid(1)) << endl;
    // cout << (get_bindid_by_uid(2)) << endl;
    // cout << (get_bindid_by_uid(3)) << endl;
    // cout << (get_bindid_by_uid(4)) << endl;
    // cout << (get_bindid_by_uid(5)) << endl;
    // cout << (get_bindid_by_uid(11)) << endl;

    // cout << (create_dir(1, 2, "/doc") == false ? "right" : "wrong") << endl;
    // cout << (create_dir(1, 2, "/doc/txt") == false ? "right" : "wrong") << endl;
    
    // cout << (delete_dir(1) == false ? "right" : "wrong") << endl;
    // cout << (delete_dir(2) == false ? "right" : "wrong") << endl;

    // cout << (get_dirid(1, 2, "/doc") == 3 ? "right" : "wrong") << endl;
    // cout << (get_dirid(1, 2, "/doc/txt") == 4 ? "right" : "wrong") << endl;
    // cout << (get_dirid(1,2,"/doc/abc") == -1 ? "right" : "wrong") << endl;

    // cout << (get_dirid(1,2,"/doc/abc") == -1 ? "right" : "wrong") << endl;

    cout << create_dir_return_dirid(1, 2, "/doc/test") << endl;
    cout << create_dir_return_dirid(33, 2, "/doc/test") << endl;
}

int main(int argc, char* argv[])
{

    /* ��ʼ�� mysql ������ʧ�ܷ���NULL */
    if ((db = mysql_init(NULL))==NULL) {
    	cout << "mysql_init failed" << endl;
    	return -1;
    	}

    /* �������ݿ⣬ʧ�ܷ���NULL
       1��mysqldû����
       2��û��ָ�����Ƶ����ݿ���� */
    if (mysql_real_connect(db,"47.102.201.228","root", "root123","netdrive",0, NULL, 0)==NULL) {
    	cout << "mysql_real_connect failed(" << mysql_error(db) << ")" << endl;
    	return -1;
    	}

    /* �����ַ���������������ַ����룬��ʹ/etc/my.cnf������Ҳ���� */
    mysql_set_character_set(db, "gbk"); 



   //  test
    test();

    /* �ر��������� */
    mysql_close(db);   

    return 0;
}

