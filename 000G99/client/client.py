# coding=gbk
import socket
import select
import threading
import sys
import os
import time
import json
import queue
import sqlite3
import threading
import hashlib
import getpass
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
    head = req.split('\n')[0]
    body = req.split('\n')[1]
    body = body.split('\0')[0]
    return head, body


def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d


def create_file(prefix, path, file, f_size):
    path_list = path.split('/')[0:-1]
    f_real_path = prefix + file + f_size

    with open(f_real_path, 'wb') as f:
        f.seek(f_size-1)
        f.write(b'0x00')

    print(path_list)


def isSucure(password):
    upper = 0
    lower = 0
    digit = 0
    if len(password) < 8:
        print("����������8λ��ɡ�")
        return False
    for char in password:
        if char.islower():
            lower += 1
        if char.isdigit():
            digit += 1
        if char.isupper():
            upper += 1
    if upper and lower and digit:
        return True
    else:
        print("�������ٰ������֡���д��Сд��ĸ��")
        return False


class Client:
    def __init__(self):
        self.init_path = './init/'  # �洢��Ҫ����ļ���Ŀ¼
        self.user_name = ''  # �û��˺�
        self.user_pwd = ''  # �û�����
        self.user_session = ''  # �û�session
        self.BUF_SIZE = 4194304  #
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
        self.table_name = 'local_file_table'
        self.mutex = threading.Lock()

        # ��ʼ���ͻ���
        print('��ʼ���ͻ���...')

        # 1. ����sock����
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
        name = input('�������˺ţ�')
        pwd = getpass.getpass('���������룺')
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
        print('signup:')
        account = input('�������˺ţ�')
        password = getpass.getpass('���������룺')
    
        if not isSucure(password):
            return False
        
        tmpStr = "signup\n{\"username\":{0},\"password\":{1}}\0".format(account, password)
        self.sock.send(tmpStr.encode(encoding='gbk'))

        result = socket.recv(1024)
        resultStr = result.decode(encoding='gbk')
        resultJson = json.loads(resultStr[0:-1].split('\n')[1])
        
        print(resultJson['msg'])
        
        if resultJson['error']:
            return False
        else:
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

    def db_select(self):
        cur = self.sql_conn.cursor()
        res = cur.execute('select * from {0}'.format(self.table_name))
        db_file = res.fetchall()  # ͬ������ֵ���ʽ
        # ����һ��map��key:"path/file_name", value:(size,mtime,md5)
        db_map = {local_file['relative_path']: {'size': local_file['size'], 'mtime': local_file['mtime'],
                                                'md5': local_file['md5']}
                  for local_file in db_file}
        return db_map

    def db_init(self):
        self.sql_conn = sqlite3.connect(self.per_db_name)  # �����ڣ�ֱ�Ӵ���
        self.sql_conn.row_factory = dict_factory

    def db_create_local_file_table(self):
        sql = """
        DROP TABLE IF EXISTS {0};
        CREATE TABLE {0} (
          "path_file_name" text NOT NULL,
          "size" integer,
          "mtime" text,
          "md5" text(32),
          PRIMARY KEY ("path_file_name")
        );
        """.format(self.table_name)

        cur = self.sql_conn.cursor()
        cur.execute(sql)

    def db_insert(self, path_file_name, size, mtime, md5):
        cur = self.sql_conn.cursor()
        sql = "INSERT INTO {0} VALUES ({1}, {2}, {3}, {4})".format(self.table_name, path_file_name, size, mtime, md5)
        cur.execute(sql)

    def db_update(self, relative_path_file, size, mtime, md5):
        cur = self.sql_conn.cursor()
        sql = "UPDATE {0} SET size = {2}, mtime = {3}, md5 = {4} WHERE path_file_name = {1}".\
            format(self.table_name, relative_path_file, size, mtime, md5)
        cur.execute(sql)

    def db_delete(self, path_file_name):
        cur = self.sql_conn.cursor()
        sql = "DELETE FROM TABLE {0} WHERE path_file_name = {1}".format(self.table_name, path_file_name)
        cur.execute(sql)

    def gen_req_createDir(self, path, filename):
        que_id = self.get_que_id()
        f_name = "{0}{1}{2}".format(self.bind_path_prefix, path, filename)
        d = {"session": self.user_session, "prefix": path, "dirname": filename, "queueid": que_id}

        req_createDir = "createDir\n{0}\0".format(json.dumps(d))
        return req_createDir

    def gen_req_uploadFile(self, path, filename, size, md5, task_id, mtime):
        que_id = self.get_que_id()
        d = {"session": self.user_session, "taskid": task_id, "queueid": que_id,
             "filename": filename, "path": path, "md5": md5, "size": size, "mtime": mtime}

        req_uploadFile = "uploadFile\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_uploadFile)

    def gen_req_deleteFile(self, path_file):
        que_id = self.get_que_id()
        file = path_file.split('/')[-1]
        path = path_file[0:-len(file)]
        print(file, path)
        d = {
            'session': self.user_session,
            'filename': file,
            'path': path,
            'queueid': que_id
        }
        req_deleteFile = "deleteFile\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_deleteFile)

    def handle_getDir(self):
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

    def get_chunk_num(self, size):
        return int(size/self.BUF_SIZE)

    def add_to_task_que(self, task_id, task_type, f_path, f_name, f_size, f_mtime, md5, cnt, total):
        d = {
            'task_type': task_type,
            'f_path': f_path,
            'f_name': f_name,
            'f_size': f_size,
            'f_time': f_mtime,
            'md5': md5,
            'cnt': cnt,
            'total': total
        }
        self.task_queue[task_id] = d

    def gen_req_download(self, f_path, f_name, f_size, md5, task_id):
        off = 0
        chunk_id = 0
        s = f_size
        while s > 0:
            if s > self.BUF_SIZE:
                chunk_size = self.BUF_SIZE
            else:
                chunk_size = s
            que_id = self.get_que_id()
            d = {
                'session': self.user_session,
                'filename': f_name,
                'path': f_path,
                'md5': md5,
                'taskid': task_id,
                'queueid': que_id,
                'chunkid': chunk_id,
                'offset': off,
                'chunksize': chunk_size
            }
            req = 'downloadFile\n{0}\0'.format(json.dumps(d))
            self.req_queue.put(req)
            off += chunk_size
            chunk_id += 1
            s -= chunk_size
        return chunk_id

    def gen_task_download_upload(self, task_type, f_path, f_name, f_size, f_mtime, md5):
        """
        �������ƣ�gen_task_download_upload
        ����������self, task_type, f_path, f_name, f_size, md5
        ��������ֵ����
        �������ܣ�1. �������غ��ϴ�����
                2. ������������(1)�����ļ��Ĵ�С�������ɸ����ص�req����req_que�У�
                                (2)Ȼ������task,��Ӧcnt=0��total=num(req)��(3)Ȼ�����·�����ؽ����ļ����Լ��ļ�
                3. �����ϴ�����(1) ����һ���ϴ���req����req_que�У�
                                (2)����һ��task����Ӧcnt=0,total=1��ֻ�е�recv���յ�req��res֮�����봫ʧ�ܣ��Ž�total��Ϊnum(req)
        """
        task_id = self.get_task_id()

        if task_type == 'download':
            create_file(self.bind_path_prefix, f_path, f_name, f_size)
            total = self.gen_req_download(f_path, f_name, f_size, md5, task_id)
            self.add_to_task_que(task_id, task_type, f_path, f_name, f_size, f_mtime, md5, 0, total)

        elif task_type == 'upload':
            self.gen_req_uploadFile(f_path, f_name, f_size, md5, task_id, f_mtime)
            self.add_to_task_que(task_id, task_type, f_path, f_name, f_size, f_mtime, md5, 0, 1)

        else:
            print('unknown task type')

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
        s_path = self.bind_path_prefix
        for path, dirs, files in os.walk(s_path):
            sub_path = path[len(s_path):] + '/'
            for file in files:
                # ����md5
                relative_path = sub_path + file
                real_path = path + file
                md5 = get_file_md5(real_path)
                size = os.path.getsize(real_path)
                mtime = os.path.getmtime(real_path)
                mtime = int(mtime)

                # �����ϴ�����
                self.gen_task_download_upload('upload', sub_path, file, size, md5, mtime)

                # ���ļ���Ϣ�Ǽǽ���local_file_map
                local_file_map[relative_path] = md5

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
        remote_dir_list = self.handle_getDir()

        """
        local_file_map = {"./txt": "md5", "./txt2": "md5"}
        """
        # ��local_file��remote_dir_list���жԱ�, ������������ÿһ���ļ�remote_file
        for remote_file in remote_dir_list:
            # ��map���ҵ�keyΪremote_file['filename']����
            remote_relative_path = remote_file['path'] + remote_file['filename']
            local_md5 = local_file_map.get(remote_relative_path, -1)
            # ������ؾ���ͬ���ļ�
            if local_md5 != -1:
                # ������ص�ͬ���ļ���md5��ͬ
                if local_md5 != remote_file['md5']:
                    # task_type, f_path, f_name, f_size, md5
                    self.gen_task_download_upload('download', remote_file['path'], remote_file['filename'], remote_file['size'], remote_file['mtime'], remote_file['md5'])
                    # ����Ҫ���ص��ļ���Ϣд��db
                    self.db_insert(remote_relative_path, remote_file['size'], remote_file['mtime'], remote_file['md5'])

            else:   #û��remote_file��Ӧ����
                # task_type, f_path, f_name, f_size, md5
                self.gen_task_download_upload('download', remote_file['path'], remote_file['filename'],
                                              remote_file['size'], remote_file['mtime'], remote_file['md5'])
                # ����Ҫ���ص��ļ���Ϣд��db
                self.db_insert(remote_relative_path, remote_file['size'], remote_file['mtime'], remote_file['md5'])

    def not_first_sync(self):
        # 1. �����ص������ļ���db���жԱȣ�ɨ�豾�������ļ�
        # ͨ��db��ȡ���ص����ݿ����е�����, �����ڴ���ά��һ����¼db��Ϣ��map
        db_map = self.db_select()
        # db_map = {"./txt": (5, 'hhhh-yy-dd', "md1"), "./txt2": (6, 'hhhh-yy-dd', "md2")}

        # 1.1 ��ʼɨ�豾�������ļ�
        # ά��һ��list����map��¼�����ļ�����Ϣ���ļ�·�����ļ��������ڴ�
        local_file_map = {}    # ��¼�ļ������Ӧ��md5
        s_path = self.bind_path_prefix
        for path, dirs, files in os.walk(s_path):
            sub_path = path[len(s_path):] + '/'
            for file in files:
                # ������Ե�ַ����ʵ��ַ
                relative_path_file = sub_path + file
                real_path = path + file

                # ����file�������Ϣ��size, mtime
                size = os.path.getsize(real_path)
                mtime = os.path.getmtime(real_path)
                mtime = int(mtime)

                if relative_path_file in db_map.keys():
                    db_map_item = db_map[relative_path_file]
                    if size == db_map_item['size'] and mtime == db_map_item['mtime']:
                        local_file_map[relative_path_file] = db_map_item['md5']
                    else:
                        md5 = get_file_md5(real_path)
                        local_file_map[relative_path_file] = md5
                        if md5 != db_map_item['md5']:
                            self.gen_task_download_upload('upload', sub_path, file, size, mtime, md5)
                        self.db_update(relative_path_file, size, mtime, md5)     # need
                else:
                    # ���ļ���Ϣ�Ǽǽ���local_file_map
                    md5 = get_file_md5(real_path)
                    local_file_map[relative_path_file] = md5
                    self.gen_task_download_upload('upload', sub_path, file, size, mtime, md5)
                    self.db_insert(relative_path_file, size, mtime, md5)   # need

        # 1.2 ɨ��db�е����м�¼
        for db_file_path_name in db_map.keys():
            if db_file_path_name not in local_file_map.keys():
                for task in self.task_queue.values():
                    task_path_name = task['f_path'] + task['f_name']
                    if task_path_name == db_file_path_name:
                        task_id = task['task_id']
                        self.task_queue.pop(task_id)
                        req_list = list(self.req_queue.queue)
                        self.req_queue = queue.Queue()
                        for req in req_list:
                            req_body = req.split('\n')[1].split('\0')[0]
                            req_body = json.loads(req_body)
                            if req_body['taskid'] != task_id:
                                self.req_queue.put(req)
                self.gen_req_deleteFile(db_file_path_name)
                self.db_delete(db_file_path_name)

        # 1.3 �������˵��������ݣ���������˵���������
        # ����getdir����
        remote_dir_list = self.handle_getDir()
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
        # ��local_file��remote_dir_list���жԱ�, ������������ÿһ���ļ�remote_file
        for remote_file in remote_dir_list:
            # ��map���ҵ�keyΪremote_file['filename']����
            remote_relative_path = remote_file['path'] + remote_file['filename']
            local_md5 = local_file_map.get(remote_relative_path, -1)
            # ������ؾ���ͬ���ļ�
            if local_md5 != -1:
                # ������ص�ͬ���ļ���md5��ͬ
                if local_md5 != remote_file['md5']:
                    # task_type, f_path, f_name, f_size, md5
                    self.gen_task_download_upload('download', remote_file['path'], remote_file['filename'],
                                                  remote_file['size'], remote_file['mtime'], remote_file['md5'])
                    # ����Ҫ���ص��ļ���Ϣд��db
                    self.db_insert(remote_relative_path, remote_file['size'], remote_file['mtime'], remote_file['md5'])

            else:  # û��remote_file��Ӧ����
                # task_type, f_path, f_name, f_size, md5
                self.gen_task_download_upload('download', remote_file['path'], remote_file['filename'],
                                              remote_file['size'], remote_file['mtime'], remote_file['md5'])
                # ����Ҫ���ص��ļ���Ϣд��db
                self.db_insert(remote_relative_path, remote_file['size'], remote_file['mtime'], remote_file['md5'])

    def read_bind_persistence(self):
        """
        �������ƣ�read_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. �ж�bind�־û��ļ��Ƿ���ڣ�
                    �־û��ļ������ڣ�is_bind = 0
                3. ���ڣ�������user_name���־û�bind�ļ���ȡ�����is_bind�Լ�bind_path��Ϣ
        """
        # 2.1 ����Ϣ
        per_bind_name = "{0}/{1}_bind".format(self.init_path, str(self.user_name))
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
                3. ���ڣ�������user_name���־û�bind�ļ���ȡ�����is_bind�Լ�bind_path��Ϣ
        """
        # 2.1 ����Ϣ
        per_bind_name = "{0}/{1}_bind".format(self.init_path, str(self.user_name))
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
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_name�Լ�bind��Ϣȷ��queue�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼��queue��������Ϊ��
                3. ���ڣ�������user_name���־û�queue�����ڴ棬��ʼ��queue
        """
        # 1. ���ȫ�ֵ�queue
        self.init_queue()

        # 2. �жϳ־û��ļ��Ƿ���ڣ��־û��ļ���1.����Ϣ��2.queue��Ϣ��3.db��Ϣ��
        # 2.2 queue��Ϣ
        per_queue_name = "{0}/{1}_{2}_queue".format(self.init_path, str(self.user_name), str(self.bind_path_prefix))
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
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_name�Լ�bind��Ϣȷ��queue�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼��queue��������Ϊ��
                3. ���ڣ�������user_name���־û�queue�����ڴ棬��ʼ��queue
        """
        # 2.2 queue��Ϣ
        print('<��ʼ>�־û�req�Լ�task queue��Ϣ���ļ�')
        for v in self.res_queue.values():
            self.req_queue.put(v)

        req_list = list(self.req_queue.queue)
        task_map = self.task_queue
        d = {'req': req_list, 'task': task_map}
        d_str = json.dumps(d)

        per_queue_name = "{0}/{1}_{2}_queue".format(self.init_path, str(self.user_name), str(self.bind_path_prefix))
        f = open(per_queue_name, 'r')
        f.write(d_str)
        f.close()
        print('�־û�req�Լ�task queue��Ϣ���ļ�%s<����>��' % per_queue_name)

    def read_db_persistence(self):
        """
        �������ƣ�read_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_name�Լ�bind��Ϣȷ��db�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼������first_sync
                3. ���ڣ�����not_first_sync
        """
        # 2. �жϳ־û��ļ��Ƿ���ڣ��־û��ļ���1.����Ϣ��2.queue��Ϣ��3.db��Ϣ��
        # 2.2 db��Ϣ
        self.per_db_name = "{0}{1}_{2}_db.db".format(self.init_path, str(self.user_name), str(self.bind_path_prefix))
        if not os.path.exists(self.per_db_name):
            self.db_init()
            self.db_create_local_file_table()
            self.first_sync()
        else:
            self.db_init()
            self.not_first_sync()

    # �п��������������Ҫ
    def write_db_persistence(self):
        """
        �������ƣ�write_db_persistence
        ����������self
        ��������ֵ����
        �������ܣ�1. ��ʱ�Ѿ���½�����Ұ���ϣ�����user_name�Լ�bind��Ϣȷ��db�ļ��Ƿ����
                2. �����ڣ�˵����һ�ΰ󶨸ñ���Ŀ¼������first_sync
                3. ���ڣ�����not_first_sync
        """
        # 2. �жϳ־û��ļ��Ƿ���ڣ��־û��ļ���1.����Ϣ��2.queue��Ϣ��3.db��Ϣ��
        # 2.2 db��Ϣ
        per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_name), str(self.bind_path_prefix))

    def is_cancel_bind(self):
        """
        �������ƣ�is_cancel_bind
        ����������self
        �������ܣ����û�ѡ���Ƿ�Ҫȡ����Ŀ¼
        """
        cancel_str = input("��ȷ���Ƿ���Ҫ����󶨣�y/Y����󶨣�n/N���Ӵ���")
        if cancel_str == 'y' or cancel_str == 'Y':
            cancel_str_2 = input("����y/Y�ٴ�ȷ��")
            if cancel_str_2 == 'y' or cancel_str_2 == 'Y':
                return True
        return False

    def handle_bind(self):
        """
        �������ƣ�handle_bind
        ����������self
        �������ܣ������ͬ��Ŀ¼����Ҫ���������
                1. ȷ�����ذ󶨵�Ŀ¼���޸�ȫ�ֱ���
                2. ȷ����������Ҫ�󶨵�Ŀ¼
                3. ����ȫ�ֱ���
        """
        while True: # ֱ���󶨳ɹ����˳�ѭ��
            input_path = input('ȷ�����ذ󶨵�Ŀ¼:')
            while not os.path.exists(input_path):
                print('ѡ���Ŀ¼��%s������' % input_path)
                input_path = input('���ٴ�ȷ�����ذ󶨵�Ŀ¼:')

            bind_id = input('��������ѡĿ¼��1��2��3\n��ѡ��Ҫ�󶨵�Ŀ¼��')
            while int(bind_id) > 3 or int(bind_id) < 1:
                bind_id = input('������Ŀ¼�ķ�Χ��1~3��������ѡ��')

            # �γ�setbind�������Ͳ�����res
            d = {"session": self.user_session, "bindid": bind_id}
            bind_pack = "setbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
            self.sock.send(bind_pack)
            bind_res = self.sock.recv(self.BUF_SIZE).decode(encoding='gbk')
            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)

            if bind_res_body['error'] == 0:
                self.bind_path_prefix = input_path
                print('msg:%s' % bind_res_body['msg'])
                break
            else:
                print('��ʧ��,bindid:%s\n msg:%s' % (bind_res_body['bindid'], bind_res_body['msg']))

    def handle_cancel_bind(self):
        """
        �������ƣ�handle_cancel_bind
        ����������self
        �������ܣ�����ȡ���󶨵����󣬷���ȡ���󶨵İ�
        """
        while True: # ֱ���󶨳ɹ����˳�ѭ��
            # �γ�disbind�������Ͳ�����res
            d = {"session": self.user_session}
            bind_pack = "disbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
            self.sock.send(bind_pack)
            bind_res = self.sock.recv(self.BUF_SIZE).decode(encoding='gbk')
            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)

            if bind_res_body['error'] == 0:
                print('msg:%s' % bind_res_body['msg'])
                self.bind_path_prefix = ''
                break
            else:
                print('�Ӵ���ʧ��\n msg:%s' % bind_res_body['msg'])

    def req2server(self, head, body):
        new_body = {}
        if head =="uploadFile":
            """
            ���������body(dict):{"session":"32��char","taskid":,"queueid":,"filename": ,"path": ,"md5": ,"size": ,"mtime": ,}
            ���������ȥ��taskid������Ӱ�ͷ�Լ�β0�����Ͱ�
            """
            # ����д��Ϊ�˷�����Ӻ�ɾ������
            new_body['session'] = body['session']
            new_body['queueid'] = body['queueid']
            new_body['filename'] = body['filename']
            new_body['path'] = body['path']
            new_body['md5'] = body['md5']
            new_body['size'] = body['size']
            new_body['mtime'] = body['mtime']

        elif head == 'downloadFile':
            """
            ���������{"session":"32��char","filename":"path":"md5":"taskid""queueid":,"chunk","offset":,"chunksize":}
            ���������ȥ��filename, path, taskid, chunkid
            """
            new_body['session'] = body['session']
            new_body['md5'] = body['md5']
            new_body['offset'] = body['offset']
            new_body['chunksize'] = body['chunksize']

        elif head == 'deleteFile':
            """
            ���������{"session":,"filename":,"path":,"queueid":,}
            ���������������4��
            """
            new_body['session'] = body['session']
            new_body['filename'] = body['filename']
            new_body['path'] = body['path']
            new_body['queueid'] = body['queueid']

        elif head == 'createDir':
            """
            ���������{"session":,"prefix":,"dirname":,"queueid":,}
            ���������������4��
            """
            new_body['session'] = body['session']
            new_body['prefix'] = body['prefix']
            new_body['dirname'] = body['dirname']
            new_body['queueid'] = body['queueid']

        elif head == 'deleteDir':
            """
            ���������{"session":,"prefix":,"dirname":,"queueid":,}
            ���������������4��
            """
            new_body['session'] = body['session']
            new_body['prefix'] = body['prefix']
            new_body['dirname'] = body['dirname']
            new_body['queueid'] = body['queueid']

        pack = "{0}\n{1}\0".format(head, json.dumps(body)).encode(encoding='gbk')
        self.sock.send(pack)

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
                    else:
                        self.req2server(head, body)
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
            print('�ϴ����!')
            return True
        return False

    def manage_api_download(self, Done, task_id, path, filename, offset, chunksize, file_content):
        file_path = "{0}/{1}/{2}".format(self.bind_path_prefix, path, filename)
        try:
            f = open(file_path, 'w')
            f.seek(offset)
            f.write(file_content[0:chunksize-1])
        except IOError:
            print('<download>д���ļ�ʧ�ܣ�')

        if Done:
            mtime = self.task_queue[task_id]['mtime']
            os.utime(file_path, (mtime, mtime))

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
                    """
                    ���������error, msg, queueid, vfile_id
                    """
                    # err=1 �ļ��ڷ����������ڣ���Ҫ�ϴ�, ��res_que�еĶ�Ӧɾ��
                    req = self.res_queue.pop(que_id)
                    # ����req���õ�filename�Լ�path
                    req_body = req.split('\n')[1].split('\0')[0]
                    req_body = json.loads(req_body)
                    path, filename, taskid = req_body['path'], req_body['filename'], req_body['taskid']

                    # ��λ�ļ�����ȡ��С
                    print(body_json['msg'])
                    fpath = "{0}{1}".format(path, filename)
                    fsize = os.path.getsize(fpath)

                    # �γ����ɸ�req uploadChunk����
                    off = 0
                    chunk_id = 0
                    while fsize >= 0:
                        s = fsize % self.BUF_SIZE
                        fsize -= s
                        q_id = self.get_que_id()
                        req_uploadChunk = self.gen_req_uploadChunk(body_json['vfile_id'], taskid, q_id, chunk_id, off, s, path, filename)
                        self.req_queue.put(req_uploadChunk)
                        off += s
                        chunk_id += 1

                    # �γ�req��upChunk����֮�󣬽�task_que�е�cnt��total�޸�
                    self.update_task(taskid, 0, chunk_id + 1)

                else:
                    if error == 0:
                        # ��res_que�еĶ�Ӧɾ��, �������õ�taskid
                        req = self.res_queue.pop(que_id)
                        req_body = req.split('\n')[1].split('\0')[0]
                        req_body = json.loads(req_body)
                        task_id = req_body['taskid']

                        # ������task_id�������Ӧ��task��cnt�����޸�
                        if head == 'downloadFileRes':
                            # ��Ӧtask_que�е�cnt+1
                            Done = self.add_task(task_id)

                            # ����body�������ݰ���offд�뱾�أ����д������Ҫ����mtime
                            con = body_con.split('\0')[1]
                            self.manage_api_download(Done, task_id, body_json['path'], body_json['filename'],
                                                     body_json['offset'], body_json['chunksize'], con)

                        elif head == 'uploadFileRes' or head == 'uploadChunkRes':
                            # ��Ӧtask_que�е�cnt+1
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
