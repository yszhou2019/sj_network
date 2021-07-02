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
//     string host;     //������ַ
//     string user;     //�û���
//     string pwd;      //����
//     string database; //���ݿ�
//     string charset;  //�ַ���
//     DBConf():host("47.102.201.228"), user("root"), pwd("root123"), database("netdrive"), charset("gbk") {}
// };

// class DBConnection{

// };

#define finish_with_error(mysql) _finish_with_error(mysql, __LINE__) 

void _finish_with_error(MYSQL *mysql, int line)
{
    printf("������ %d, ����ԭ�� %s\n", line, mysql_error(mysql));  
    // fprintf(stderr, "�� %d: %s\n", line, mysql_error(mysql));  
}

void ExecQuery(const string& query)
{
    if (mysql_query(db, query.c_str())) 
    {
        printf("[SQL ִ�д���] %s\n", query.c_str());
        return;
    }

    MYSQL_RES *result = mysql_store_result(db);
    
    if (result == NULL) 
    {
        printf("[SQL ִ�н��Ϊ��] %s\n", query.c_str());
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

    /* ��ӡ��ǰ��ѯ���ļ�¼������ */
    printf("select return %d records\n", (int)mysql_num_rows(result));

    /* ѭ����ȡ�������������ļ�¼
	   1�����ص���˳����selectָ������˳����ͬ����row[0]��ʼ
	   2���������ݿ�����ʲô���ͣ�C�ж��������ַ��������д�������б�Ҫ����Ҫ�Լ�����ת��
	   3�������Լ�����Ҫ��֯�����ʽ */
    while((row=mysql_fetch_row(result))!=NULL) {
        printf("%s\t%s\t%s\t%s\t%s\t%s\t\n", row[0], row[1], row[2], row[3], row[4], row[5]);
    }

    mysql_free_result(result);   
}



//================================
// GET

/**
 * ����Ҳ���uid����ô����-1�����򷵻�uid
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
 * �ж�username�Ƿ�ռ�ã� 
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
 * �ж��û��������Ƿ�ƥ�� 
 * ����uid �����ƥ�䣬����-1
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
 * ��ȡָ���û���bindid�����û�ж�Ӧ���û���¼������-1
 * Ĭ�ϵ��ļ���=0������û�а�
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
 * �ж� ָ���û���Ŀ¼�Ƿ����
 * ָ��Ŀ¼���ڣ����� dirid
 * ָ��Ŀ¼�����ڣ�����-1
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
 * ���� uid �� bindid
 * ��ȡ���е��ļ��б�
 * ��ʽ { filename, path, md5, size, mtime }, { ... }, ...
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
 * ���� dirid fname ����vfile����Ϣ
 * ���� {vid, md5}
 * ����ļ������ڣ����� {vid -1}
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
 * ���� dirid �� �ļ��������ض�Ӧ�� vid
 * �ҵ������� vid
 * �����ڣ����� -1
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
 * ���� md5 ��ȡ��Ӧ��pfile�����Ϣ
 * �ҵ������� { pid, chunks, cnt, total, complete}
 * �����ڣ����� { -1 }
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

// TODO API�Ĳ����޸�֮������ӿھͿ���ɾ����
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
 * ����md5�ж�pfile�����Ƿ���ڣ������Ƿ����
 * ������ڣ���ô���� { pid, p_complete}
 * �����ڣ�����{ pid -1 }
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
 * �����ɹ�����false
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
 * ��sessionд��table�� 
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
 * ��ȷ��Ŀ¼û�д���������£�����Ŀ¼
 * ����Ŀ¼�ɹ������� false
 * ����Ŀ¼ʧ�ܣ����� true
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
        // ��ȡ���֮���vid
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
 * ��֪Ŀ¼������
 * ����Ŀ¼�����ض�Ӧ�� dir_id
 * �������ʧ�ܣ�����-1
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
 * ����Ŀ¼id 
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
 * �󶨷�����Ŀ¼
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
 * ����ָ����session 
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
 * ��ȷ��Ŀ¼û�б�ɾ��������£�ɾ��Ŀ¼
 * ɾ��Ŀ¼�ɹ������� false
 * ɾ��Ŀ¼ʧ�ܣ����� true
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
 * ��ȷ������Ŀ¼���ڵ�����£�ɾ��ָ���ļ�
 * ɾ���ļ��ɹ������� false
 * ɾ���ļ�ʧ�ܣ����� true
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
// TODO δʵ�� or δ����

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
//  * ���� dirid �� filename ���Ҷ�Ӧ�ļ�����Ϣ
//  * �ļ������ڣ����� { vid -1 }
//  * �ļ����ڣ����� { chunks, md5, cnt, total, vid }
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
//  * ���ڳ��� uploadFile API
//  * ����ͬ�� vfile �������Ϣ��md5, chunks, size, mtime, cnt, total, complete
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
//  * ǰ����������ȷ���ļ��ǲ����ڵ�
//  * �����ɹ��� ���� vid
//  * ����ʧ�ܣ� ���� -1
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
//         // ��ȡ���֮���vid
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
//  * ǰ����������ȷ���ļ��Ǵ��ڵ�
//  * ֻ�����ļ����ϴ�����
//  * ��� API ֻ�� uploadChunk ������²Żᱻ����
//  * �����ļ����ϴ����������Ϣ��chunks, cnt, complete
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