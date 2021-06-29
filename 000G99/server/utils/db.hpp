#include "mysql/mysql.h"

/* type */
#include "json.hpp"
#include <string>
using string = std::string;
using json = nlohmann::json;

extern MYSQL* db;


// struct DBConf{
//     string host;     //主机地址
//     string user;     //用户名
//     string pwd;      //密码
//     string database; //数据库
//     string charset;  //字符集
//     DBConf():host("47.102.201.228"), user("root"), pwd("root123"), database("netdrive"), charset("gbk") {}
// };

// class DBConnection{

// };

#define finish_with_error(mysql) _finish_with_error(mysql, __LINE__) 

void _finish_with_error(MYSQL *mysql, int line)
{
    printf("出错行 %d, 错误原因 %s\n", line, mysql_error(mysql));  
    // fprintf(stderr, "行 %d: %s\n", line, mysql_error(mysql));  
}

void ExecQuery(const string& query)
{
    if (mysql_query(db, query.c_str())) 
    {
        printf("[SQL 执行错误] %s\n", query.c_str());
        return;
    }

    MYSQL_RES *result = mysql_store_result(db);
    
    if (result == NULL) 
    {
        printf("[SQL 执行结果为空] %s\n", query.c_str());
        return;
    }
}

void showUser()
{
    char query[256];
    snprintf(query, sizeof(query), "select * from user");
    if (mysql_query(db, "select * from user"))
    {
        printf("mysql_query failed(%s)\n", mysql_error(db));
    }
    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        printf("mysql_store_result failed\n");
    }

    /* 打印当前查询到的记录的数量 */
    printf("select return %d records\n", (int)mysql_num_rows(result));

    /* 循环读取所有满足条件的记录
	   1、返回的列顺序与select指定的列顺序相同，从row[0]开始
	   2、不论数据库中是什么类型，C中都当作是字符串来进行处理，如果有必要，需要自己进行转换
	   3、根据自己的需要组织输出格式 */
    while((row=mysql_fetch_row(result))!=NULL) {
        printf("%s\t%s\t%s\t%s\t%s\t%s\t\n", row[0], row[1], row[2], row[3], row[4], row[5]);
    }

    mysql_free_result(result);   
}



//================================
// GET

/**
 * 如果找不到uid，那么返回-1，否则返回uid
 */ 
int get_uid_by_session(const string& session)
{
    char query[256];
    snprintf(query, sizeof(query), "select session_uid from user_session where session = '%s';", session.c_str());

    if (mysql_query(db, query))
    {
        finish_with_error(db);
    }
    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    // printf("select return %d records\n", (int)mysql_num_rows(result));

    int uid = -1;
    if ((row = mysql_fetch_row(result)) != NULL)
    {
        uid = atoi(row[0]);
    }

    mysql_free_result(result);   
    return uid;
}

/**
 * 判断username是否被占用， 
 */
bool if_user_exist(const string& username)
{
    bool user_exist = false;

    char query[256];
    snprintf(query, sizeof(query), "select user_id from user where user_name = '%s';", username.c_str());

    if (mysql_query(db, query))
    {
        finish_with_error(db);
    }
    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        user_exist = true;
    }

    mysql_free_result(result);   

    return user_exist;
}

/**
 * 判断用户和密码是否匹配 
 * 返回uid 如果不匹配，返回-1
 */
int get_uid_by_name_pwd(const string& username, const string& pwd_encoded)
{

    int uid = -1;
    char query[256];
    snprintf(query, sizeof(query), "select user_id from user where user_name = '%s' and user_pwd = '%s';", username.c_str(), pwd_encoded.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        uid = atoi(row[0]);
    }

    mysql_free_result(result);   

    return uid;
}

/**
 * 获取指定用户的bindid，如果没有对应的用户记录，返回-1
 * 默认的文件夹=0，代表没有绑定
 */ 
int get_bindid_by_uid(int uid)
{
    int bindid = -1;
    char query[256];
    snprintf(query, sizeof(query), "select user_bindid from user where user_id = %d;", uid);

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        bindid = atoi(row[0]);
    }

    mysql_free_result(result);   

    return bindid;
}

/**
 * 判断 指定用户的目录是否存在
 * 指定目录存在，返回 dirid
 * 指定目录不存在，返回-1
 */ 
int get_dirid(int uid, int bindid, const string& path)
{
    char query[256];
    snprintf(query, sizeof(query), "select dir_id from dir where dir_uid = %d and dir_bindid = %d and dir_path = '%s';", uid, bindid, path.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    int dirid = -1;
    if ((row = mysql_fetch_row(result)) != NULL)
    {
        dirid = atoi(row[0]);
    }

    mysql_free_result(result);   

    return dirid;
}

//================================
// INSERT

/**
 * 创建成功返回false
 */ 
bool create_user(const string& username, const string& pwd_encoded)
{
    char query[256];
    snprintf(query, sizeof(query),
    "insert into user (user_name, user_pwd)values('%s','%s');", username.c_str(), pwd_encoded.c_str());

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

/**
 * 将session写入table中 
 */
bool write_session(int uid, const string& session)
{
    char query[256];
    snprintf(query, sizeof(query), "insert into user_session (`session`, `session_uid`) values('%s', %d);", session.c_str(), uid);

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }
    return error_occur;
}

/**
 * 在确定目录没有创建的情况下，创建目录
 * 创建目录成功，返回 false
 * 创建目录失败，返回 true
 */ 
bool create_dir(int uid, int bindid, const string& path)
{
    char query[256];

    snprintf(query, sizeof(query),
    "insert into dir (dir_path, dir_uid, dir_bindid) values('%s', %d, %d);", path.c_str(), uid, bindid);

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

//================================
// UPDATE

/**
 * 解绑的目录id 
 */
bool disbind_dir(int uid)
{
    char query[256];
    snprintf(query, sizeof(query), "update `user` set user_bindid = 0 where user_id = %d;", uid);

    bool error_occur = false;

    if (mysql_query(db, query))
    {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

/**
 * 绑定服务器目录
 */ 
bool set_bind_dir(int uid, int bindid)
{
    char query[256];
    snprintf(query, sizeof(query), "update `user` set user_bindid = %d  where user_id = %d;", bindid, uid);

    bool error_occur = false;

    if (mysql_query(db, query))
    {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

//================================
// DELETE

/**
 * 销毁指定的session 
 */
bool destroy_session(const string& session)
{
    char query[256];
    snprintf(query, sizeof(query), "delete from user_session where `session` = '%s';", session.c_str());

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }
    
    return error_occur;
}

/**
 * 在确定目录没有被删除的情况下，删除目录
 * 删除目录成功，返回 false
 * 删除目录失败，返回 true
 */ 
bool delete_dir(int dirid)
{
    char query[256];

    snprintf(query, sizeof(query),
    "delete from dir where dir_id = %d;", dirid);

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

//================================
// TODO 未实现 or 未测试



json get_file_dir(int uid)
{
    char query[256];
    snprintf(query, sizeof(query), "select dir_id from dir where dir_uid = %d and dir_bindid = %d and dir_path = '%s';", uid);

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    int dirid = -1;
    if ((row = mysql_fetch_row(result)) != NULL)
    {
        dirid = atoi(row[0]);
    }

    mysql_free_result(result);   

    return dirid;
}


/**
 * 根据 dirid 和 filename 查找对应文件的记录
 * 找到，返回 vid
 * 不存在，返回 -1
 * 获取chunks信息
 * 获取cnt
 * 获取total
 * 获取complete
 */ 
json get_vfile(int dirid, const string& filename)
{
    char query[256];
    snprintf(query, sizeof(query), "select vfile_id, vfile_cnt, vfile_total, v_file_complete from virtual_file where vfile_dir_id = %d and vfile_name = '%s';", dirid, filename.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;
    
    json res;
    res["vid"] = -1;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    int vid = -1;
    if ((row = mysql_fetch_row(result)) != NULL)
    {
        res["vid"] = atoi(row[0]);
        res["chunks"] = row[1];
        res["cnt"] = atoi(row[2]);
        res["total"] = atoi(row[3]);
        res["complete"] = atoi(row[4]);
    }

    mysql_free_result(result);   

    return res;
}


/**
 * 根据vid，获取对应的行记录
 * 真正需要的信息
 * 
 * md5 
 * chunks
 * cnt
 * total
 * complete
 */ 
json get_vfile_by_vid(size_t vid)
{
    json res;

}

/**
 * 根据 dirid 和 文件名，返回对应的 vid
 * 找到，返回 vid
 * 不存在，返回 -1
 */ 
int get_vid(int dirid, const string& filename)
{

}

/**
 * vfile_id 找到返回，不存在返回 -1
 * md5
 */ 
json get_vinfo(int dirid, const string& filename)
{
    
}

/**
 * 前提条件，明确了文件是不存在的
 * 创建成功， 返回 vid
 * 创建失败， 返回 -1
 */ 
int create_vfile(int dirid, const string& filename, off_t size, const string& md5, int mtime, const string& chunks, int cnt, int total, int complete)
{
    // string query = "insert into virtual_file (vfile_dir_id, vfile_name, vfile_size, vfile_md5, vfile_mtime, vfile_chunks, vfile_cnt, vfile_total, vfile_complete) values( " + dirid + ", '" + filename + "', " + size + ", '" + md5 + "', " + mtime + ", '" + chunks + "', " + cnt + ", " + total + ", " + complete + ")";

    string query = "insert into virtual_file (vfile_dir_id, vfile_name, vfile_size, vfile_md5, vfile_mtime, vfile_chunks, vfile_cnt, vfile_total, vfile_complete) values( ";

    query += dirid + ", '";

    query += filename + "', ";

    query += size + ", '";

    query += md5 + "', ";

    query += mtime + ", '";

    query += chunks + "', ";

    query += cnt + ", ";

    query += total + ", ";

    query += complete + ")";

    int vid = -1;
    if (mysql_query(db, query.c_str())) {
        finish_with_error(db);
    }else{
        // 获取添加之后的vid
        mysql_query(db, "select @@identity");
        MYSQL_ROW row;
        MYSQL_RES *result;
        if ((result = mysql_store_result(db))==NULL) {
            finish_with_error(db);
        }
        if ((row = mysql_fetch_row(result)) != NULL)
        {
            vid = atoi(row[0]);
        }
        mysql_free_result(result);  
    }

    return vid;
}




/**
 * 在确定父级目录存在的情况下，删除指定文件
 * 删除文件成功，返回 false
 * 删除文件失败，返回 true
 */ 
bool delete_file(int dirid, const string& filename)
{
    char query[256];

    snprintf(query, sizeof(query), "delete from virtual_file where where vfile_dir_id = %d and vfile_name = '%s';", dirid, filename.c_str());

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}


/**
 * 前提条件，明确了文件是存在的
 * 只更新文件的上传进度
 * 这个 API 只有 uploadChunk的情况下才会被调用
 * 更新文件的 chunks 信息
 * 将下面的信息写入到 vid 那一行的记录中
 * chunks
 * cnt
 * total
 * complete
 */ 
bool update_vfile_upload_progress(int vid, const string& chunks, int cnt, int total, int complete)
{
    string query = "update `virtual_file` set vfile_chunks = '";

    query += chunks + "', vfile_cnt = ";

    query += cnt + ", vfile_complete = ";

    query += complete + " where vfile_id = ";

    query += vid + ";";

    bool error_occur = false;

    if (mysql_query(db, query.c_str()))
    {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

/**
 * 客户端同名文件内容发生变化，需要修改相关信息
 * 需要的信息
 * md5
 * chunks
 * size
 * mtime
 * cnt(0)
 * total
 * complete(0)
 */ 
bool update_vfile_whole(int vid, const string &md5, const string &chunks, off_t size, int mtime, int cnt, int total, int complete)
{

}

/**
 * 已知目录不存在
 * 创建目录，返回对应的 dir_id
 * 如果创建失败，返回-1
 */ 
int create_dir_return_dirid(int uid, int bindid, const string& path)
{
    char query[256];
    snprintf(query, sizeof(query),
    "insert into dir (dir_path, dir_uid, dir_bindid) values('%s', %d, %d);", path.c_str(), uid, bindid);

    int dirid = -1;
    if (mysql_query(db, query))
    {
        finish_with_error(db);
    }else{
        mysql_query(db, "select @@identity");
        MYSQL_ROW row;
        MYSQL_RES *result;
        if ((result = mysql_store_result(db))==NULL) {
            finish_with_error(db);
        }
        if ((row = mysql_fetch_row(result)) != NULL)
        {
            dirid = atoi(row[0]);
        }
        mysql_free_result(result);  
    }
    return dirid;
}