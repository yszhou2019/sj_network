#include "mysql/mysql.h"

/* type */
#include "json.hpp"
#include <string>
using string = std::string;
using json = nlohmann::json;

/* debug */
#include <iostream>
using std::endl;

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

string get_username_by_uid(int uid)
{
    string username = "";
    char query[256];
    snprintf(query, sizeof(query), "select user_name from user where user_id = %d;", uid);

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
        username = string(row[0]);
    }

    mysql_free_result(result);   

    return username;
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

/**
 * 根据 uid 和 bindid
 * 获取所有的文件列表
 * 格式 { filename, path, md5, size, mtime }, { ... }, ...
 */ 
json get_file_dir(int uid, int bindid)
{
    char query[512];
    snprintf(query, sizeof(query), "select vf.vfile_name filename, d.dir_path path, vf.vfile_md5 md5, vf.vfile_size size, vf.vfile_mtime mtime from dir d, virtual_file vf, physical_file pf where d.dir_id = vf.vfile_dir_id and vf.vfile_md5 = pf.pfile_md5 and pf.pfile_complete = 1 and d.dir_id in (select d.dir_id dirid from dir d where d.dir_uid = %d and d.dir_bindid = %d);", uid, bindid);

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    json res;
    json temp;
    while ((row = mysql_fetch_row(result)) != NULL)
    {
        temp = {
            { "filename", row[0] },
            { "path", row[1] },
            { "md5", row[2] },
            { "size", atoll(row[3]) },
            { "mtime", atoi(row[4]) }
        };
        res.push_back(temp);
    }

    mysql_free_result(result);   

    return res;
}

/**
 * 根据 dirid fname 查找vfile的信息
 * 返回 {vid, md5}
 * 如果文件不存在，返回 {vid -1}
 */ 
json get_vinfo(int dirid, const string& filename)
{
    char query[256];
    snprintf(query, sizeof(query), "select vf.vfile_id vid, vf.vfile_md5 md5 from virtual_file vf where vf.vfile_dir_id = %d and vf.vfile_name = '%s';", dirid, filename.c_str());

    if(mysql_query(db,query)){
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;
    
    json res;
    res["vid"] = -1;
    res["md5"] = string("");

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        res["vid"] = atoi(row[0]);
        res["md5"] = string(row[1]);
        // std::cout << atoi(row[0]) << " " << string(row[1]) << endl;
    }
    return res;
}



/**
 * 根据 dirid 和 文件名，返回对应的 vid
 * 找到，返回 vid
 * 不存在，返回 -1
 */ 
int get_vid(int dirid, const string& filename)
{
    char query[256];
    snprintf(query, sizeof(query), "select vfile_id from virtual_file where vfile_dir_id = %d and vfile_name = '%s';", dirid, filename.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    int vid = -1;
    if ((row = mysql_fetch_row(result)) != NULL)
    {
        vid = atoi(row[0]);
    }

    mysql_free_result(result);   

    return vid;
}

/**
 * 根据 md5 获取对应的pfile相关信息
 * 找到，返回 { pid, chunks, cnt, total, complete}
 * 不存在，返回 { -1 }
 */ 
json get_pfile_info(const string& md5)
{
    char query[256];
    snprintf(query, sizeof(query), "select pf.pfile_id, pf.pfile_chunks, pf.pfile_cnt, pf.pfile_total from physical_file pf where pf.pfile_md5 = '%s';", md5.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;
    
    json res;
    res["pid"] = -1;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        res["pid"] = atoi(row[0]);
        // res["name"] = string(row[1]);
        res["chunks"] = string(row[1]);
        res["cnt"] = atoi(row[2]);
        res["total"] = atoi(row[3]);
    }

    mysql_free_result(result);   

    return res;
}

// TODO API的参数修改之后，这个接口就可以删除了
string get_vfile_md5(int vid)
{
    char query[256];
    snprintf(query, sizeof(query), "select vfile_md5 from virtual_file where vfile_id = %d;", vid);

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    string res = "";

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        res = string(row[0]);
    }

    mysql_free_result(result);   

    return res;
}




/**
 * 根据md5判断pfile表中是否存在，并且是否完成
 * 如果存在，那么返回 { pid, p_complete}
 * 不存在，返回{ pid -1 }
 */
json if_pfile_complete(const string& md5)
{
    char query[256];
    snprintf(query, sizeof(query), "select pf.pfile_id, pf.pfile_complete from physical_file pf where pf.pfile_md5 = '%s';", md5.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;
    
    json res;
    res["pid"] = -1;
    res["complete"] = 0;

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        res["pid"] = atoi(row[0]);
        res["complete"] = atoi(row[1]);
    }

    mysql_free_result(result);   

    return res;
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
    printf("%s\n", query);
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



int create_vfile_new(int dirid, const string& filename, ll size, const string& md5, int mtime)
{    
    string query = "insert into virtual_file (vfile_dir_id, vfile_name, vfile_size, vfile_md5, vfile_mtime) values( ";

    query += std::to_string(dirid) + ", '";

    query += filename + "', ";

    query += std::to_string(size) + ", '";

    query += md5 + "', ";

    query += std::to_string(mtime) + ")";

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

int create_pfile(const string& md5, const string& chunks, int total)
{
    string query = "insert into physical_file (pfile_md5, pfile_refcnt, pfile_chunks, pfile_cnt, pfile_total, pfile_complete) values( '";

    query += md5 + "', 1, '";

    query += chunks + "', 0, ";

    query += std::to_string(total) + ", 0)";

    bool error_occur = false;
    if (mysql_query(db, query.c_str())) {
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




bool update_pfile_upload_progress(int pid, const string& chunks, int cnt, int complete)
{
    string query = "update `physical_file` set pfile_chunks = '" + chunks + "', pfile_cnt = ";

    query += std::to_string(cnt) + ", pfile_complete = ";

    query += std::to_string(complete) + " where pfile_id = ";

    query += std::to_string(pid) + ";";

    bool error_occur = false;

    if (mysql_query(db, query.c_str()))
    {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}



bool update_vfile_whole_new(int vid, const string& md5, ll size, int mtime)
{    
    string query = "update `virtual_file` set vfile_md5 = '" + md5 + "', vfile_md5 = '";

    query += md5 + "', vfile_size = ";

    query += std::to_string(size) + ", vfile_mtime = ";

    query += std::to_string(mtime) + " where vfile_id = ";

    query += std::to_string(vid) + ";";

    bool error_occur = false;

    if (mysql_query(db, query.c_str()))
    {
        error_occur = true;
        finish_with_error(db);
    }

    return error_occur;
}

bool increase_pfile_refcnt(int pid)
{
    string query = "update `physical_file` set pfile_refcnt = pfile_refcnt + 1 where pfile_id = " + std::to_string(pid) + ";";

    bool error_occur = false;

    if (mysql_query(db, query.c_str()))
    {
        error_occur = true;
        finish_with_error(db);
    }
    if( (int)mysql_affected_rows(db) != 1)
    {
        error_occur = true;
    }

    return error_occur;
}

bool decrease_pfile_refcnt_by_md5(const string& md5)
{
    string query = "update `physical_file` set pfile_refcnt = pfile_refcnt - 1 where pfile_md5 = '" + md5 + "';";

    bool error_occur = false;

    if (mysql_query(db, query.c_str()))
    {
        error_occur = true;
        finish_with_error(db);
    }
    if( (int)mysql_affected_rows(db) != 1)
    {
        error_occur = true;
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
    if( (int)mysql_affected_rows(db) != 1)
    {
        error_occur = true;
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
    if( (int)mysql_affected_rows(db) != 1)
    {
        error_occur = true;
    }

    return error_occur;
}

/**
 * 在确定父级目录存在的情况下，删除指定文件
 * 删除文件成功，返回 false
 * 删除文件失败，返回 true
 */ 
bool delete_file(int dirid, const string& filename)
{
    char query[256];

    snprintf(query, sizeof(query), "delete from virtual_file where vfile_dir_id = %d and vfile_name = '%s';", dirid, filename.c_str());

    bool error_occur = false;
    if (mysql_query(db, query)) {
        error_occur = true;
        finish_with_error(db);
    }
    if( (int)mysql_affected_rows(db) != 1)
    {
        error_occur = true;
    }

    return error_occur;
}

//================================
// TODO 未实现 or 未测试

// deprecated


/*
string get_pname_by_md5(const string& md5)
{
    char query[256];
    snprintf(query, sizeof(query), "select pfile_name from physical_file where pfile_md5 = '%s';", md5.c_str());

    if (mysql_query(db, query)) {
        finish_with_error(db);
    }

    MYSQL_ROW row;
    MYSQL_RES *result;

    string res = "";

    if ((result = mysql_store_result(db))==NULL) {
        finish_with_error(db);
    }

    if ((row = mysql_fetch_row(result)) != NULL)
    {
        res = string(row[0]);
    }

    mysql_free_result(result);   

    return res;
}
*/

// /**
//  * 根据 dirid 和 filename 查找对应文件的信息
//  * 文件不存在，返回 { vid -1 }
//  * 文件存在，返回 { chunks, md5, cnt, total, vid }
//  */ 
// json get_vfile_upload_info(int vid)
// {
//     char query[256];
//     snprintf(query, sizeof(query), "select vfile_id, vfile_chunks, vfile_md5, vfile_cnt, vfile_total from virtual_file where vfile_id = %d;", vid);

//     if (mysql_query(db, query)) {
//         finish_with_error(db);
//     }

//     MYSQL_ROW row;
//     MYSQL_RES *result;
    
//     json res;
//     res["vid"] = -1;

//     if ((result = mysql_store_result(db))==NULL) {
//         finish_with_error(db);
//     }

//     if ((row = mysql_fetch_row(result)) != NULL)
//     {
//         res["vid"] = atoi(row[0]);
//         res["chunks"] = string(row[1]);
//         res["md5"] = string(row[2]);
//         res["cnt"] = atoi(row[3]);
//         res["total"] = atoi(row[4]);
//     }

//     mysql_free_result(result);   

//     return res;
// }

// /**
//  * 用于初次 uploadFile API
//  * 更新同名 vfile 的相关信息，md5, chunks, size, mtime, cnt, total, complete
//  */ 
// bool update_vfile_whole(int vid, const string &md5, const string &chunks, ll size, int mtime, int cnt, int total, int complete)
// {
//     string query = "update `virtual_file` set vfile_md5 = '" + md5 + "', vfile_chunks = '";

//     query += chunks + "', vfile_size = ";

//     query += std::to_string(size) + ", vfile_mtime = ";

//     query += std::to_string(mtime) + ", vfile_cnt = ";

//     query += std::to_string(cnt) + ", vfile_total = ";

//     query += std::to_string(total) + ", vfile_complete = ";

//     query += std::to_string(complete) + " where vfile_id = ";

//     query += std::to_string(vid) + ";";

//     bool error_occur = false;

//     if (mysql_query(db, query.c_str()))
//     {
//         error_occur = true;
//         finish_with_error(db);
//     }

//     return error_occur;
// }

// /**
//  * 前提条件，明确了文件是不存在的
//  * 创建成功， 返回 vid
//  * 创建失败， 返回 -1
//  */ 
// int create_vfile(int dirid, const string& filename, ll size, const string& md5, int mtime, const string& chunks, int cnt, int total, int complete)
// {
//     // string query = "insert into virtual_file (vfile_dir_id, vfile_name, vfile_size, vfile_md5, vfile_mtime, vfile_chunks, vfile_cnt, vfile_total, vfile_complete) values( " + dirid + ", '" + filename + "', " + size + ", '" + md5 + "', " + mtime + ", '" + chunks + "', " + cnt + ", " + total + ", " + complete + ")";

//     string query = "insert into virtual_file (vfile_dir_id, vfile_name, vfile_size, vfile_md5, vfile_mtime, vfile_chunks, vfile_cnt, vfile_total, vfile_complete) values( ";

//     query += std::to_string(dirid) + ", '";

//     query += filename + "', ";

//     query += std::to_string(size) + ", '";

//     query += md5 + "', ";

//     query += std::to_string(mtime) + ", '";

//     query += chunks + "', ";

//     query += std::to_string(cnt) + ", ";

//     query += std::to_string(total) + ", ";

//     query += std::to_string(complete) + ")";

//     int vid = -1;
//     if (mysql_query(db, query.c_str())) {
//         finish_with_error(db);
//     }else{
//         // 获取添加之后的vid
//         mysql_query(db, "select @@identity");
//         MYSQL_ROW row;
//         MYSQL_RES *result;
//         if ((result = mysql_store_result(db))==NULL) {
//             finish_with_error(db);
//         }
//         if ((row = mysql_fetch_row(result)) != NULL)
//         {
//             vid = atoi(row[0]);
//         }
//         mysql_free_result(result);  
//     }

//     return vid;
// }

// /**
//  * 前提条件，明确了文件是存在的
//  * 只更新文件的上传进度
//  * 这个 API 只有 uploadChunk 的情况下才会被调用
//  * 更新文件的上传进度相关信息，chunks, cnt, complete
//  */ 
// bool update_vfile_upload_progress(int vid, const string& chunks, int cnt, int complete)
// {
//     string query = "update `virtual_file` set vfile_chunks = '" + chunks + "', vfile_cnt = ";

//     query += std::to_string(cnt) + ", vfile_complete = ";

//     query += std::to_string(complete) + " where vfile_id = ";

//     query += std::to_string(vid) + ";";

//     bool error_occur = false;

//     if (mysql_query(db, query.c_str()))
//     {
//         error_occur = true;
//         finish_with_error(db);
//     }

//     return error_occur;
// }