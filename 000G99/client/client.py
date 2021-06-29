# coding=gbk
import socket
import select
import threading
import sys
import os
import json
import queue
import sqlite3
import threading
import hashlib
from api.download_header import *
from api.upload_header import *
from utils import *


def get_file_md5(f_name):
    """
    ���������(1)path �ļ�·��
            (2)f_name �ļ���
    ����������ļ���md5
    """
    buf_size = 4096
    m = hashlib.md5()
    with open(f_name, 'rb') as f:
        while True:
            data = f.read(buf_size)
            if not data:
                break
            m.update(data)
    return m.hexdigest()

def read_config(file_name):
    """
    �������ļ��ж�ȡ���ӷ�������ip��port
    """
    f = open(file_name, 'r')
    text = f.read()
    f.close()
    j_ = json.loads(text)
    return j_['host'], j_['port']


def get_connect():
    host_, port_ = read_config('./config.cfg')
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print("try to connect to %s:%s" % (host_, port_))
        sock.connect((host_, port_))
    except socket.error:
        print("Could not make a connection to the server")
        sys.exit(0)
    print('connect success!')
    return sock


def decode_req(req):
    """
    ���������req(str), format: req:"head\n{body}\0"
    �������: 1. head(str)
            2. body(str)
    """
    head = req.splict('\n')[0]
    body = req.splict('\n')[1]
    body = body.splict('\0')[0]
    return head, body


def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d


class Client:
    def __init__(self):
        self.init_path = './init/'  # �洢��Ҫ����ļ���Ŀ¼
        self.user_name = ''  # �û��˺�
        self.user_pwd = ''  # �û�����
        self.user_session = ''  # �û�session
        self.BUF_SIZE = 4096  # const(fake) var buf_size����Ҫ����4096����Ϊdownload��ʱ��������������
        self.GET_DIR_SIZE = 4194304     # 4M
        self.is_die = 0  # �Ƿ���Ҫ��ͣ�߳�
        self.is_bind = 0  # �û��Ƿ��Ѿ���Ŀ¼
        self.bind_path_prefix = ''  # ��ǰ���ذ�Ŀ¼
        self.sql_conn = 0  # �������ݿ�
        self.req_queue = queue.Queue()  # request������У����Ѻ�ģ�������
        self.res_queue = {}  # response�ȴ���Ӧ���У���request��������get�����ٷ���res����
        self.task_queue = {}  # ������У��洢�ϴ������صȵ��û����񣨴�����
        self.complete_queue = {}  # ������ɶ��У���ɺ��task����Ž���
        self.per_db_name = ''
        self.que_id = 0
        self.chunk_id = 0
        self.task_id = 0
        self.mutex = threading.Lock()

        # ����sock����
        print('��ʼ���ͻ���...')
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # ����server
        host_, port_ = read_config('./config.cfg')
        try:
            print("try to connect to %s:%s" % (host_, port_))
            self.sock.connect((host_, port_))
        except socket.error:
            print("Could not make a connection to the server")
            sys.exit(0)
        print('connect success!\n ��ʼ����ɣ�')

    # need :
    def handle_login(self):
        """
        �������ƣ�handle_login
        ����������self
        ��������ֵ��bool����½�ɹ�ΪTrue������ΪFalse����Ҫ�������ԭ��
        �������ܣ����û������˺ź�������е�½
        �������裺1. Ҫ���û������˺ź�����
                2. ���˺ź������γ��������ͨ��self.sock���ͣ�������
                3. ����response������res������error�ж�ʧ�ܳɹ�
                4.  �ɹ����ӡ��Ϣ������ȫ��self.user��Ϣ�Լ�session��Ϣ��return True��
                    ʧ�����ӡ��Ϣ��return False
        �����ʽ��signup\n{"username": "root","password": "root123"}\0
                ����Ҫstrתbytes��ʽ����API�ĵ�Ϊ׼��
        """
        print('login')
        # ������Ҫ�Ĳ���
        name = input()
        pwd = input()
        login_req = "login\n{\"username\":{0},\"password\":{1}}\0".format(name, pwd)

        # Ĭ��sock�ڵ��ú���֮ǰ���ӳɹ�
        self.sock.send(login_req.encode(encoding='gbk'))
        res_bytes = self.sock.recv(self.BUF_SIZE)
        res = res_bytes.decode(encoding='gbk')

        res_head = res.split('\n')[0]
        res_body = res.split('\n')[1]
        res_body = res_body[0:-1]  # ȥ'\0'��Ҳ������split�ķ�ʽ

        if res_head == 'loginRes':
            res_body = json.loads(res_body)
            if res_body['error'] == 1:
                # ����ɹ��Ĳ���
                print()
                return True
            else:
                # ����ʧ�ܵĲ���
                print()
        return False

    def handle_logout(self):
        """
        �������ƣ�handle_logout
        ����������self
        ��������ֵ��bool���˳��ɹ�ΪTrue��ʧ��ΪFalse����Ҫ�������ԭ��
        �������裺1. �γ��˳����������ͨ��sock����
                2. ����sock��res��������������error�ж��˳��ɹ�ʧ��
                3.  �ɹ�����ӡ��Ϣ������True
                    ʧ�ܣ���ӡ��Ϣ��ʧ��ԭ�򣩣�����False
        """
        print('login')
        return True

    # need :
    def handle_signup(self):
        """
        �������ƣ�handle_signup
        ����������self
        �������ܣ�bool���ɹ�ΪTrue��ʧ��ΪFalse����Ҫ�������ԭ��
        �������裺1. Ҫ���û������˺ź�����
                2. �ڿͻ����ж�����İ�ȫ�ԣ�����ȫ�������������루Ҳ����Ҫ�����������˺ţ�
                2. �γ�ע�����������ͨ��sock����
                2. ����sock��res�����������ж��˳��ɹ�ʧ��
                3.  �ɹ�����ӡ��Ϣ������True
                    ʧ�ܣ���ӡ��Ϣ��ʧ��ԭ�򣩣�����False
        """
        print('signup')
        return True

    def login_signup(self):
        while True:
            start_input = input("������login����signup...")
            if start_input == "login":
                login_res = self.handle_login()
                if not login_res:  # ��½ʧ��
                    print('�����µ�¼...')
                    continue
                else:  # ��½�ɹ�
                    print('��¼�ɹ�...')
                    break

            elif start_input == "signup":
                signup_res = self.handle_signup()
                if not signup_res:
                    print('������ע��...')
                    continue
                else:
                    print('ע��ɹ�...')
            else:
                print('��������ָ�����ƴд����Сд..')

    def init_queue(self):
        """ ��ʼ������ """
        if self.req_queue.not_empty:
            self.req_queue = queue.Queue()
        self.res_queue = {}
        self.task_queue = {}
        self.complete_queue = {}

    def get_que_id(self):
        # ���̺߳�recver�̶߳�����ã�����Ҫ����
        self.mutex.acquire()
        que_id = self.que_id
        if self.que_id >= 10000:
            self.que_id = 0
        else:
            self.que_id += 1
        self.mutex.release()
        return que_id

    def get_task_id(self):
        # �������̵߳����������
        task_id = self.task_id
        if self.task_id >= 10000:
            self.task_id = 0
        else:
            self.task_id += 1
        return task_id

    def gen_req_createDir(self, path, filename):
        que_id = self.get_que_id()
        f_name = "{0}{1}{2}".format(self.bind_path_prefix, path, filename)
        d = {"session": self.user_session, "prefix": path, "dirname": filename, "queueid": que_id}

        req_createDir = "createDir\n{0}\0".format(json.dumps(d))
        return req_createDir

    def gen_req_uploadFile(self, path, filename):
        task_id = self.get_task_id()
        que_id = self.get_que_id()
        f_name = "{0}{1}{2}".format(self.bind_path_prefix, path, filename)
        md5 = get_file_md5(f_name)
        size = os.path.getsize(f_name)
        mtime = os.path.getmtime(f_name)
        d = {"session": self.user_session, "taskid": task_id, "queueid": que_id,
             "filename": filename, "path": path, "md5": md5, "size": size, "mtime": mtime}

        req_uploadFile = "uploadFile\n{0}\0".format(json.dumps(d))
        return req_uploadFile, md5

    def res_getDir(self):
        d = {"session": self.user_session}
        getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        self.sock.send(getDir)
        get_dir_res = self.sock.recv(self.GET_DIR_SIZE)

        # 2.2 ��֤getDir��res
        get_dir_str = get_dir_res.decode(encoding='gbk')
        get_dir_head = get_dir_str.split('\n')[0]
        if get_dir_head == 'getdirRes':
            # 2.3 ��֤�ɹ�����strתΪjson, �õ�������Ŀ¼�������ļ���list
            get_dir_body = get_dir_str.split('\n')[1]
            get_dir_body = get_dir_body.split('\0')[0]
            get_dir = json.loads(get_dir_body)
            if get_dir['error'] == 0:
                print(get_dir['msg'])
                return get_dir['dir_list']
        return []

    def insert_into_db(self):

    def get_chunk_num(self, size):
        return int(size/self.BUF_SIZE)

    def gen_task_que(self):
        task_id = self.get_task_id()

    def gen_download_task_queue(self):
        # ����task_id����¼

        self.gen_task_que()
        download_to_task()
        # ������task_id��download����ŵ�req����
        download_to_req()
        # �ڱ�������һ�����ļ�


    # ���Ŀ¼���ļ�֮����д
    def first_sync(self):
        """
        �������ƣ�first_sync
        ����������self
        ��������ֵ����
        �������ܣ�1. ���г���ͬ������Ҫ����������
                2. ��������������getDir��Ȼ��������Ľ��
        """

        # 1. ɨ�豾�������ļ������������ļ��Լ��ļ��е�upload api
        # ����local_file_map��key:"path/file_name", value:"md5"��֮�������˶Ա�ʱʹ��
        local_file_map = {}
        # ����һ��local_dir����¼�����ļ�������
        local_dir_set = set()
        s_path = self.bind_path_prefix
        for path, dirs, files in os.walk(s_path):
            path_2 = path[len(s_path):] + '/'
            for dir_ in dirs:
                req_createDir = self.gen_req_createDir(path_2, dir_)
                self.req_queue.put(req_createDir)

                # ���ļ�����Ϣ�Ǽǽ���local_file_map
                relative_path = path_2 + dir_
                local_dir_set.add(relative_path)

            for file in files:
                req_uploadFile, md5 = self.gen_req_uploadFile(path_2, file)
                self.req_queue.put(req_uploadFile)

                # ���ļ���Ϣ�Ǽǽ���local_file_map
                m = {"is_dir": 0, "md5": md5}
                relative_path = path_2 + file
                local_file_map[relative_path] = m

        # 2. �������˵��������ݣ���������˵���������
        # 2.1 ����getdir����
        """
        list = [{
            "filename":,
            "path":,���������ڷ������������ӣ�
            "size":
            "md5"
            "modtime"
        },
        {},...,
        {}]
        """
        remote_dir_list = self.res_getDir()

        """
        local_file_map = {"./txt": "md5", "./txt2": "md5"}
        """
        # 2.4 ��local_file��get_dir_list���жԱ�, ������������ÿһ���ļ�remote_file
        for remote_file in remote_dir_list:
            # ��map���ҵ�keyΪremote_file['filename']����
            if remote_file['is_dir']:   # ���ļ��У�����������򴴽�
                r_path, r_name = remote_file['path'], remote_file['filename']
                r_relative_path = r_path + r_name
                if r_relative_path not in local_dir_set:
                    os.makedirs(self.bind_path_prefix+r_relative_path)
            else:
                # ���ļ�
                remote_md5 = local_file_map.get(remote_file['filename'], -1)
                if remote_md5 == -1 or remote_md5 != remote_file['md5']:
                    # ��download����ŵ�task���в�����queue
                    self.gen_download_task_queue()

                    # ����Ҫ���ص��ļ���Ϣд��db
                    self.insert_into_db()


    def not_first_sync(self):
        # 1. �����ص������ļ���db���жԱȣ�ɨ�豾�������ļ�
        # ͨ��db��ȡ���ص����ݿ����е�����, �����ڴ���ά��һ����¼db��Ϣ��map
        self.sql_conn.row_factory = dict_factory
        cur = self.sql_conn.cursor()
        res = cur.execute('select * from local_file_table')
        db_file = res.fetchall()  # ͬ������ֵ���ʽ
        # ����һ��map��key:"path/file_name", value:(size,mtime,md5)
        map = {"{0}/{1}".format(local_file['relative_path'],
                                local_file['file_name']): (local_file['size'], local_file['mtime'], local_file['md5'])
               for local_file in db_file}

        # map = {"./txt": (5, 'hhhh-yy-dd', "md1"), "./txt2": (6, 'hhhh-yy-dd', "md2")}


        # 1.1 ��ʼɨ�豾�������ļ�
        # ά��һ��list����map��¼�����ļ�����Ϣ���ļ�·�����ļ��������ڴ�
        list_or_map = {}    # ��¼�ļ������Ӧ��md5
        for local_file in cur_dir:
            f = map.get(strcat(local_file.path,local_file.fname),-1)
            if f != -1: # ������ڣ��Ա�size�Լ�mtime�����ܶԱ�md5����Ϊ�������ټ��㱾��md5�Ļ���
                if local_file.size != f[0] or local_file.mtime!= f[1]:
                    # ˵����ʱͬ�����ļ��Ѿ��������޸ģ���Ҫ����md5
                    local_file_md5 = calculate(local_file)
                    if local_file_md5 != f[2]:
                        # ˵���޸������������ݣ���Ҫ�ϴ�
                        upload_header = "uploadFile\n" + local_file_info_json + '\0'.encode()
                        self.req_queue.put(upload_header)
                    # �����Ƿ��޸����������ݣ�size��mtime�Ѿ���ͬ��������Ҫ����
                    self.sql_conn.execute('update [table] set md5 = % , size = % , mtime = % where path_filename = %' % (local_file_md5, path, filename))
            # �����ǰɨ����ļ���db����������
            # ���㱾��md5
            local_file_md5 = calculate(local_file)
            # �����ϴ�����
            upload_header = "uploadFile\n" + local_file_info_json + '\0'.encode()
            self.req_queue.put(upload_header)
            # ����db��path, name, size, mtime, md5
            self.sql_conn.execute('insert into ...')

        # 1.2 ɨ��db�е����м�¼
        for item in map:
            if item.first not in local_map:
                # ��ǰdb�ļ�¼�ڱ��ز�����
                # ���task�����Ƿ���ڶ�Ӧ���ϴ�or��������
                for task in self.task_queue.values():
                    if task.f_path && task.f_name:
                        # �����Ҫɾ��req�Ķ�Ӧ��task_id
                        task_id = task.task_id
                        # �������е�req_queue��һ��list
                        req_list = get_req_queue_to_list()
                        # ����һ��list����task_id��Ӧ��ɾ��
                        delete_task_id_in_req_list(req_list, task_id)
                        # ��ɾ�����req_list����������put��queue��
                        for req in req_list:
                            self.req_queue.put(req)
                # ��֪serverɾ����Ӧ�ļ�¼
                req_delete = 'deleteFile\n' + file_info + '\0'
                self.req_queue.put(req_delete)
                # ɾ��db�еļ�¼
                self.sql_conn.execute('delete ��Ӧ�ļ�¼')

        # 1.3 �������˵��������ݣ���������˵���������
        # ����getdir����
        d = {"session": self.user_session}
        getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        self.sock.send(getDir)
        get_dir_res = self.sock.recv(self.GET_DIR_SIZE)

        # ��֤getDir��res
        get_dir_str = get_dir_res.decode(encoding='gbk')
        get_dir_head = get_dir_str.split('\n')[0]
        if get_dir_head != 'getdirRes':
            print('����getDirʧ�ܣ�')
            exit(1)

        # ��֤�ɹ�����strתΪjson, �õ�������Ŀ¼�������ļ���list
        get_dir_body = get_dir_str.split('\n')[1]
        get_dir_body = get_dir_body.split('\0')[0]
        get_dir_list = json.loads(get_dir_body)
        """{
            "filename":,
            "path":,���������ڷ������������ӣ�
            "size":
            "md5"
            "modtime"
        }"""

        # ����get_dir_list���жԱ�, ������������ÿһ���ļ�remote_file
        for remote_file in get_dir_list:
            # ��map���ҵ�keyΪremote_file['filename']����
            remote_md5 = map.get(remote_file['filename'], -1)
            if remote_md5 == -1 or remote_md5 != remote_file['md5']:
                download_to_req()
                download_to_task()
                add_to_db()


    def read_bind_persistence(self):
        """
        �������ƣ�read_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. �ж�bind�־û��ļ��Ƿ���ڣ�
                    �־û��ļ������ڣ�is_bind = 0
                3. ���ڣ�������user_id���־û�bind�ļ���ȡ�����is_bind�Լ�bind_path��Ϣ
        """
        # 2.1 ����Ϣ
        per_bind_name = "{0}/{1}_bind".format(self.init_path, str(self.user_id))
        f = open(per_bind_name, 'r')
        data = f.read(self.BUF_SIZE)
        f.close()
        if data:
            json_data = json.loads(data)
            self.is_bind = json_data['is_bind']
            self.bind_path_prefix = json_data['path']
        else:
            self.is_bind = 0

    def write_bind_persistence(self):
        """
        �������ƣ�write_bind_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. �ж�bind�־û��ļ��Ƿ���ڣ�
                    �־û��ļ������ڣ�is_bind = 0
                3. ���ڣ�������user_id���־û�bind�ļ���ȡ�����is_bind�Լ�bind_path��Ϣ
        """
        # 2.1 ����Ϣ
        per_bind_name = "{0}/{1}_bind".format(self.init_path, str(self.user_id))
        f = open(per_bind_name, 'w')
        d = {'is_bind': self.is_bind, 'path': self.bind_path_prefix}
        d_str = json.dumps(d)
        f.write(d_str)
        f.close()

    def read_queue_persistence(self):
        """
        �������ƣ�read_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_id�Լ�bind��Ϣȷ��queue�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼��queue��������Ϊ��
                3. ���ڣ�������user_id���־û�queue�����ڴ棬��ʼ��queue
        """
        # 1. ���ȫ�ֵ�queue
        self.init_queue()

        # 2. �жϳ־û��ļ��Ƿ���ڣ��־û��ļ���1.����Ϣ��2.queue��Ϣ��3.db��Ϣ��
        # 2.2 queue��Ϣ
        per_queue_name = "{0}/{1}_{2}_queue".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        f = open(per_queue_name, 'r')
        data = f.read(self.BUF_SIZE)
        f.close()
        if data:
            json_data = json.loads(data)
            # queue����ʽ "op_type\n{content}\0"
            # important!
            req_json = json_data['req']
            task_map = json_data['task']
            for req_item in req_json:
                self.req_queue.put(req_item)
            for key, value in task_map:
                self.task_queue[key] = value

    def write_queue_persistence(self):
        """
        �������ƣ�write_queue_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_id�Լ�bind��Ϣȷ��queue�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼��queue��������Ϊ��
                3. ���ڣ�������user_id���־û�queue�����ڴ棬��ʼ��queue
        """
        # 2.2 queue��Ϣ
        print('<��ʼ>�־û�req�Լ�task queue��Ϣ���ļ�')
        for v in self.res_queue.values():
            self.req_queue.put(v)

        req_list = list(self.req_queue.queue)
        task_map = self.task_queue
        d = {'req': req_list, 'task': task_map}
        d_str = json.dumps(d)

        per_queue_name = "{0}/{1}_{2}_queue".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        f = open(per_queue_name, 'r')
        f.write(d_str)
        f.close()
        print('�־û�req�Լ�task queue��Ϣ���ļ�%s<����>��' % per_queue_name)

    def create_local_file_table(self, per_db_name):
        sql = "DROP TABLE IF EXISTS \"local_file_table\";\
            CREATE TABLE \"local_file_table\" (\
              \"relative_path\" text,\
              \"file_name\" text,\
              \"size\" integer,\
              \"mtime\" text,\
              \"md5\" text(32) NOT NULL,\
              PRIMARY KEY (\"md5\")\
            );"
        self.sql_conn = sqlite3.connect(per_db_name)  # �����ڣ�ֱ�Ӵ���
        cur = self.sql_conn.cursor()
        cur.execute(sql)
        # self.sql_conn.close()

    def read_db_persistence(self):
        """
        �������ƣ�read_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_id�Լ�bind��Ϣȷ��db�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼������first_sync
                3. ���ڣ�����not_first_sync
        """
        # 2. �жϳ־û��ļ��Ƿ���ڣ��־û��ļ���1.����Ϣ��2.queue��Ϣ��3.db��Ϣ��
        # 2.2 db��Ϣ
        self.per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        if not os.path.exists(self.per_db_name):
            self.create_local_file_table(self.per_db_name)
            self.first_sync()
        else:
            # ������Ҫfetchall
            self.not_first_sync()

    # �п��������������Ҫ
    def write_db_persistence(self):
        """
        �������ƣ�write_db_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_id�Լ�bind��Ϣȷ��db�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼������first_sync
                3. ���ڣ�����not_first_sync
        """
        # 2. �жϳ־û��ļ��Ƿ���ڣ��־û��ļ���1.����Ϣ��2.queue��Ϣ��3.db��Ϣ��
        # 2.2 db��Ϣ
        per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))

    def is_cancel_bind(self):
        cancel_str = input("��ȷ���Ƿ���Ҫ����󶨣�y/Y����󶨣�n/N���Ӵ���")
        if cancel_str == 'y' or cancel_str == 'Y':
            cancel_str_2 = input("����y/Y�ٴ�ȷ��")
            if cancel_str_2 == 'y' or cancel_str_2 == 'Y':
                return True
        return False

    # need ��
    def handle_bind(self):
        """
        �������ƣ�handle_bind
        ����������self
        �������ܣ������ͬ��Ŀ¼����Ҫ���������
                1. ȷ�����ذ󶨵�Ŀ¼���޸�ȫ�ֱ���
                2. ȷ����������Ҫ�󶨵�Ŀ¼
        """
        path = input('ȷ�����ذ󶨵�Ŀ¼')
        # check(path)
        # bind_id = input('��������ѡĿ¼��1��2��3\n��ѡ��Ҫ�󶨵�Ŀ¼��')
        # check(bind_id)
        # send(bind_header)
        # recv(bind_res)

    # sender����
    def sender(self):
        while self.is_die != 0:
            rs, ws, es = select.select([], [self.sock], [])
            if self.sock in ws:
                if not self.req_queue.empty():
                    init_req = self.req_queue.get()
                    head, body = decode_req(init_req)
                    body = json.loads(body)
                    que_id = body['queueid']

                    if head == 'uploadChunk':
                        off, size = body['offset'], body['chunksize']
                        path, filename = body.pop('path'), body.pop('filename')
                        file_path = "{0}/{1}/{2}".format(self.bind_path_prefix, path, filename)
                        try:
                            f = open(file_path, 'r')
                            f.seek(off)
                            data = f.read(size)
                            req_send = "{0}\n{1}\0{2}\0".format(head, json.dumps(dict), data).encode(encoding='gbk')
                            self.sock.send(req_send)
                            self.res_queue[que_id] = init_req
                        except IOError:
                            print('<sender>:���ļ�ʧ��')
                    elif head == 'uploadFile':
                        body.delete()
                        self.sock.send(req_send)
                        self.res_queue[que_id] = init_req

                    elif head == 'downloadFile':

                    elif head == 'deleteFile':

                    elif head == 'createDir':

                    elif head == 'deleteDir':

                    else:
                        req_send = init_req.encode(encoding='gbk')
                        self.sock.send(req_send)
                        self.res_queue[que_id] = init_req


    def gen_req_uploadChunk(self, v_id, t_id, q_id, c_id, off, size, path, filename):
        d = {'session': self.user_session, 'vfile_id': v_id, 'taskid': t_id, 'queueid': q_id, 'chunkid': c_id,
             'offset': off, 'chunksize': size, 'path': path, 'filename': filename}
        req_up = "uploadChunk\n{0}\0".format(json.dumps(d))
        return req_up

    def update_task(self, task_id, cnt, total):
        self.task_queue[task_id]['cnt'] = cnt
        self.task_queue[task_id]['total'] = total

    def add_task(self, task_id):
        self.task_queue[task_id]['cnt'] += 1
        if self.task_queue[task_id]['cnt'] >= self.task_queue[task_id]['total']:
            self.complete_queue[task_id] = self.task_queue.pop(task_id)

    def manage_api_download(self, path, filename, offset, chunksize, file_content):
        file_path = "{0}/{1}/{2}".format(self.bind_path_prefix, path, filename)
        try:
            f = open(file_path, 'w')
            f.seek(offset)
            f.write(file_content[0:chunksize-1])
        except IOError:
            print('<download>д���ļ�ʧ�ܣ�')

    # receiver����
    def receiver(self):
        while self.is_die != 0:
            rs, ws, es = select.select([self.sock], [], [])
            if self.sock in rs:
                init_res = self.sock.recv(self.BUF_SIZE * 2).decode(encoding='gbk')

                # �����������ݵ�resͷ�Լ�res_body��ת��Ϊjson��dict��ʽ
                head = init_res.split('\n')[0]
                body_con = init_res.split('\n')[1]
                body = body_con.split('\0')[0]
                body_json = json.loads(body)

                # ��¼��Ҫ��error��Ϣ��queue_id��Ϣ
                error = body_json['error']
                que_id = body_json['queueid']

                # �ļ��봫ʧ�ܣ���Ҫ�ϴ��ļ�����ϸ����
                # ��Ҫ�޸�task��cnt��total
                if head == 'uploadFileRes' and error == 1:
                    # err=1 �ļ��ڷ����������ڣ���Ҫ�ϴ�, ��res_que�еĶ�Ӧɾ��
                    self.res_queue.pop(que_id)

                    # ��λ�ļ�����ȡ��С
                    path, filename = body_json['path'], body_json['filename']
                    print(body_json['msg'])
                    fpath = "{0}/{1}".format(path, filename)
                    fsize = os.path.getsize(fpath)

                    # �γ����ɸ�req uploadChunk����
                    off = 0
                    chunk_id = 0
                    while fsize >= 0:
                        s = fsize % self.BUF_SIZE
                        fsize -= s
                        q_id = self.get_que_id()
                        req_uploadChunk = self.gen_req_uploadChunk(body_json['vfile_id'], body_json['taskid'], q_id, chunk_id, off, s, path, filename)
                        self.req_queue.put(req_uploadChunk)
                        off += s
                        chunk_id += 1

                    # �γ�req��upChunk����֮�󣬽�task_que�е�cnt��total�޸�
                    t_id = body_json['taskid']
                    self.update_task(t_id, 0, chunk_id + 1)

                else:
                    if error == 0:
                        # ��res_que�еĶ�Ӧɾ��
                        self.res_queue.pop(que_id)

                        # ������task_id�������Ӧ��task��cnt�����޸�
                        if head == 'downloadFileRes':
                            # ����body�������ݰ���offд�뱾�أ����޸�task�ļ��ļ���
                            con = body_con.split('\0')[1]
                            self.manage_api_download(body_json['path'], body_json['filename'],
                                                     body_json['offset'], body_json['chunksize'], con)

                            # ��Ӧtask_que�е�cnt+1
                            task_id = body_json['taskid']
                            self.add_task(task_id)
                        elif head == 'uploadFileRes' or head == 'uploadChunkRes':
                            # ��Ӧtask_que�е�cnt+1
                            task_id = body_json['taskid']
                            self.add_task(task_id)

                    elif error == 1:
                        # ��res_que�ж�Ӧ��que_id�������ٴη���req_que��
                        self.req_queue.put(self.res_queue.pop(que_id))

    def sync(self):
        if not self.is_die:
            self.not_first_sync()
            self.sync_timer()

    def sync_timer(self):
        t = threading.Timer(15.0, self.sync())
        if not self.is_die:
            t.start()
        else:
            t.cancel()

    def run(self):
        """
        �������ƣ�run
        ����������self
        �������ܣ��������������е���Ҫ����
        """

        # �����½��ע���¼���ֱ����½�ɹ��Ž�������
        self.login_signup()

        # ��bind�ĳ־û����ݶ�ȡ����
        self.read_bind_persistence()

        # �����Ŀ¼
        is_cancel = False
        if self.is_bind:
            is_cancel = self.is_cancel_bind()
            if is_cancel:
                self.handle_cancel_bind()
        if (not self.is_bind) or is_cancel:  # Ŀǰδ�󶨻��߰�֮��ѡ��ȡ����
            self.handle_bind()

        # ����queue�ĳ־û��ļ�������ϴ�δ��ɻ��ٴδ�������
        self.read_queue_persistence()
        # ����db�ĳ־û��ļ������бȶԲ�ͬ��
        self.read_db_persistence()

        # ��������Ϊ������
        self.sock.setblocking(False)

        # �����̲߳�����
        t_sender = threading.Thread(target=self.sender())
        t_receiver = threading.Thread(target=self.receiver())
        t_sync_timer = threading.Thread(target=self.sync_timer())
        t_sender.run()
        t_receiver.run()
        t_sync_timer.run()

        # ���̴߳������IO
        while True:
            io_str = input('������ָ��...')
            if io_str == "log_out":
                self.is_die = 1
                t_sender.join()
                t_receiver.join()

                # ���ն��־û��ļ��ķ�˳����г־û�
                self.write_db_persistence()
                self.write_queue_persistence()
                self.write_bind_persistence()

                # �˳���½�������˳���½��api
                self.handle_logout()
                exit(1)

            elif io_str == "op2":
                print('op2')

            elif io_str == "op3":
                print('op3')

            else:
                print('���������ָ��..')


if __name__ == '__main__':
    client = Client()
    client.run()
