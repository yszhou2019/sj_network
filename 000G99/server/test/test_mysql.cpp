//mysql_demo.cpp
#include <iostream>	// cin,cout等
#include <iomanip>	// setw等
#include <mysql.h>	// mysql特有

#include "../utils/db.hpp"
using namespace std;


MYSQL     *db;  


int demo()
{
    MYSQL_RES *result;   
    MYSQL_ROW  row;
    /* 进行查询，成功返回0，不成功非0
       1、查询字符串存在语法错误
       2、查询不存在的数据表 */
    if (mysql_query(db, "select * from user")) {
    	cout << "mysql_query failed(" << mysql_error(db) << ")" << endl;
    	return -1;
    	}

    /* 将查询结果存储起来，出现错误则返回NULL
       注意：查询结果为NULL，不会返回NULL */
    if ((result = mysql_store_result(db))==NULL) {
    	cout << "mysql_store_result failed" << endl;
    	return -1;
    	}

    /* 打印当前查询到的记录的数量 */
    cout << "select return " << (int)mysql_num_rows(result) << " records" << endl;

    /* 循环读取所有满足条件的记录
	   1、返回的列顺序与select指定的列顺序相同，从row[0]开始
	   2、不论数据库中是什么类型，C中都当作是字符串来进行处理，如果有必要，需要自己进行转换
	   3、根据自己的需要组织输出格式 */
    while((row=mysql_fetch_row(result))!=NULL) {
        cout << setiosflags(ios::left);             //输出左对齐
        cout << "id" << setw(12) << row[0];     //宽度12位，左对齐
        cout << "name" << setw(8)  << row[1];     //    8
        cout << "pwd" << setw(4)  << row[2];     //    4
        cout << "islogin" << setw(4)  << row[3];     //    4
        cout << "bindid" << setw(4)  << row[4];     //    4
        cout << "session" << setw(4)  << row[5];     //    4
        cout << endl;
        }

    /* 释放result */
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

    /* 初始化 mysql 变量，失败返回NULL */
    if ((db = mysql_init(NULL))==NULL) {
    	cout << "mysql_init failed" << endl;
    	return -1;
    	}

    /* 连接数据库，失败返回NULL
       1、mysqld没运行
       2、没有指定名称的数据库存在 */
    if (mysql_real_connect(db,"47.102.201.228","root", "root123","netdrive",0, NULL, 0)==NULL) {
    	cout << "mysql_real_connect failed(" << mysql_error(db) << ")" << endl;
    	return -1;
    	}

    /* 设置字符集，否则读出的字符乱码，即使/etc/my.cnf中设置也不行 */
    mysql_set_character_set(db, "gbk"); 



   //  test
    test();

    /* 关闭整个连接 */
    mysql_close(db);   

    return 0;
}

