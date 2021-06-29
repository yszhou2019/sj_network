#include "mysql/mysql.h"

/* type */
#include "json.hpp"
#include <string>
using string = std::string;
using json = nlohmann::json;

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

    return error_occur;
}

//================================
// TODO δʵ�� or δ����



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
 * ���� dirid �� filename ���Ҷ�Ӧ�ļ��ļ�¼
 * �ҵ������� vid
 * �����ڣ����� -1
 * ��ȡchunks��Ϣ
 * ��ȡcnt
 * ��ȡtotal
 * ��ȡcomplete
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
 * ����vid����ȡ��Ӧ���м�¼
 * ������Ҫ����Ϣ
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
 * ���� dirid �� �ļ��������ض�Ӧ�� vid
 * �ҵ������� vid
 * �����ڣ����� -1
 */ 
int get_vid(int dirid, const string& filename)
{

}

/**
 * vfile_id �ҵ����أ������ڷ��� -1
 * md5
 */ 
json get_vinfo(int dirid, const string& filename)
{
    
}

/**
 * ǰ����������ȷ���ļ��ǲ����ڵ�
 * �����ɹ��� ���� vid
 * ����ʧ�ܣ� ���� -1
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
 * ��ȷ������Ŀ¼���ڵ�����£�ɾ��ָ���ļ�
 * ɾ���ļ��ɹ������� false
 * ɾ���ļ�ʧ�ܣ����� true
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
 * ǰ����������ȷ���ļ��Ǵ��ڵ�
 * ֻ�����ļ����ϴ�����
 * ��� API ֻ�� uploadChunk������²Żᱻ����
 * �����ļ��� chunks ��Ϣ
 * ���������Ϣд�뵽 vid ��һ�еļ�¼��
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
 * �ͻ���ͬ���ļ����ݷ����仯����Ҫ�޸������Ϣ
 * ��Ҫ����Ϣ
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