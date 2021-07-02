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
# import fallocate
import signal

mid_file_ex = ['.swp', '.swpx', '.tmp', '.download']

def print_info(head, errno, info):
    print('[{0}] [err = {1}] {2}'.format(head, errno, info))


def is_tmp_file(filename):
    file_extension = os.path.splitext(filename)[-1]
    if file_extension in mid_file_ex:
        return True
    return False 


def add_path_bound(path):
    if path[0] != '/':
        path = '/' + path
    if path[-1] != '/':
        path += '/'
    return path


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


def prefix_to_filename(prefix):
    """
    prefix: /a/b/c/
    return a_b_c
    """
    return prefix.replace('/', '_')


def get_connect():
    host_, port_ = read_config('./config.cfg')
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # print("try to connect to %s:%s" % (host_, port_))
        sock.connect((host_, port_))
    except socket.error:
        print("Could not make a connection to the server")
        sys.exit(0)
    # print('connect success!')
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


def path_translate(input_path):
    abs_path = os.path.abspath(input_path) + '/'
    return abs_path


def create_path(prefix):
    p_list = prefix.split('/')[1:-1]
    cur_path = '/'
    for p in p_list:
        cur_path += p + '/'
        if not os.path.exists(cur_path):
            os.makedirs(cur_path)


def create_file(prefix, path, file, f_size):
    """
    prefix: /root/test/
    path: /sum/pa/
    file: name.txt
    """
    path_list = path.split('/')[1:-1]
    f_real_path = prefix + path[1:] + file
    # print(path)
    # print(file)
    # ����path_list�����ɲ����ڵ�path
    cur_path = prefix
    for sub_path in path_list:
        cur_path += '/{}'.format(sub_path)
        if not os.path.exists(cur_path):
            os.makedirs(cur_path)

    if os.path.exists(cur_path):
        # print(cur_path)

        with open(f_real_path, 'wb') as f:
            # print('path,', path, 'file,', file, 'size,', f_size, 'f_real_path,' ,f_real_path)
            os.posix_fallocate(f.fileno(), 0, f_size)
            # print('path,', path, 'file,', file, 'size,', f_size, 'f_real_path,' ,f_real_path)
            # f.seek(f_size-1)
            # f.write(b'\0')


def is_sucure(password):
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


def is_cancel_bind():
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


def write2sock(sock, data, n):
    """
    write n bytes to a sock
    """
    left = n
    ptr = 0
    while left > 0:
        nwritten = sock.send(data[ptr: ptr+left])
        if nwritten < 0:
            if left == n:
                return -1
            else:
                break
        elif nwritten == 0:
            break
        left -= nwritten
        ptr += nwritten
    return n - left


# ��һ����Ҫ
def read_from_sock(sock, data, n):
    """
    read n bytes from a sock to a buffer
    """
    nread = 0
    ptr = 0
    left = n
    while left > 0:
        datan = sock.recv(left)
        nread = len(datan)
        data[ptr:ptr+nread] = datan
        if nread < 0:
            if left == n:
                return -1
            else:
                break
        elif nread == 0:
            break
        left -= nread
        ptr += nread
    return n-left


def sock_ready_to_write(sock):
    rs, ws, es = select.select([], [sock], [], 10.0)
    if sock in ws:
        return True
    return False


def sock_ready_to_read(sock):
    rs, ws, es = select.select([sock], [], [], 10.0)
    if sock in rs:
        return True
    return False


def file2sock(sock, file, off, chunksize):
    # print('..exec file2sock ...')
    # print('sock', sock, 'file', file, 'off', off, 'chunksize', chunksize)
    cnt = 0
    size = chunksize
    # f = open(file, 'r')
    while cnt != chunksize:
        ready = sock_ready_to_write(sock)
        if not ready:
            return 0

        # w_bytes = splice(f.fileno(), sock.fileno(), offset=off, nbytes=size)
        file.seek(off)
        data = file.read(size)
        w_bytes = sock.send(data)
        # print(data)

        if w_bytes == 0:
            break
        if w_bytes == -1:
            return -1
        cnt += w_bytes
        off += w_bytes
        size -= w_bytes

    sock.send(b'\0')
    return 1


def sock2file(sock, file, off, chunksize):
    """
    ���ý����ǵü�β0
    """
    cnt = 0
    size = chunksize
    file.seek(off, 0)
    # r_pipe, w_pipe = os.pipe()
    while cnt != chunksize:
        ready = sock_ready_to_read(sock)
        if not ready:
            file.close()
            return 0

        # r_bytes = splice(sock.fileno(), r_pipe, offset=off, nbytes=size)
        # w_bytes = splice(w_pipe, file.fileno(), offset=off, nbytes=size)
        # print('read {0} bytes from sock, and write {1} bytes to file'.format(r_bytes, w_bytes))
        data = sock.recv(size)
        w_bytes = file.write(data)
        # print(data, size, w_bytes)
        if w_bytes == 0:
            # print("w_bytes = 0���˳�")
            break
        if w_bytes == -1:
            # print("w_bytes = -1����⵽�������˳�")
            file.close()
            return -1
        cnt += w_bytes
        size -= w_bytes

    if sock_ready_to_read(sock):
        ch = sock.recv(1)

    file.close()
    return 1


def recv_pack(sock_in):
    res = b''
    if sock_ready_to_read(sock_in):
        ch = sock_in.recv(1)
        while ch != b'\0':
            res += ch
            if sock_ready_to_read(sock_in):
                ch = sock_in.recv(1)
            else:
                break
        res += ch
    return res


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
        self.update_flag = 0
        # self.main_thread_begin = 0

        # ��ʼ���ͻ���
        print('��ʼ���ͻ���...')

        # 0. ����Ŀ¼
        self.init_path = path_translate(self.init_path)
        create_path(self.init_path)

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

    def handler_signal(self, signum, frame):
            self.is_die = 1

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
        d = {
            'username': name,
            'password': pwd
        }
        login_req = "login\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')

        # Ĭ��sock�ڵ��ú���֮ǰ���ӳɹ�
        # self.sock.send(login_req.encode(encoding='gbk'))
        write2sock(self.sock, login_req, len(login_req))
        res_bytes = recv_pack(self.sock)
        res = res_bytes.decode(encoding='gbk')
        # print(res)

        res_head = res.split('\n')[0]
        res_body = res.split('\n')[1]
        res_body = json.loads(res_body[0:-1])

        if res_head == 'loginRes':
            if res_body['error']:
                # ����ʧ�ܵĲ���
                print('��¼ʧ�ܣ�')
            else:
                # ����ɹ��Ĳ���
                print('��¼�ɹ���')
                self.user_session = res_body['session']
                self.user_name = name
                self.user_pwd = pwd
                return True

        # ����ͳһ��ӡ������
        print(res_body['msg'])
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
        # print('loginout')

        d = {'session': self.user_session}
        request = "logout\n{0}\0".format(json.dumps(d))

        # Ĭ��sock�ڵ��ú���֮ǰ���ӳɹ�
        request = request.encode(encoding='gbk')
        write2sock(self.sock, request, len(request))

        # print('\n�������˳�..')
        # return True
        # res_bytes = recv_pack(self.sock)
        # res = res_bytes.decode(encoding='gbk')
        #
        # res_head = res.split('\n')[0]
        # res_body = res.split('\n')[1]
        # res_body = json.loads(res_body[0:-1])
        #
        # if res_head == 'logoutRes':
        #     if res_body['error']:
        #         # ����ʧ�ܵĲ���
        #         print('�ǳ�ʧ�ܣ�')
        #     else:
        #         # ����ɹ��Ĳ���
        #         print('�ǳ��ɹ���')
        #         self.user_session = ''
        #         self.user_name = ''
        #         self.user_pwd = ''
        #         return True
        #
        # # ����ͳһ��ӡ������
        # print(res_body['msg'])
        # return False

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

        if not is_sucure(password):
            return False

        d = {
            'username': account,
            'password': password
        }
        tmpStr = "signup\n{0}\0".format(json.dumps(d))
        print('test [signup_pack]', tmpStr)
        tmpStr = tmpStr.encode(encoding='gbk')
        # self.sock.send(tmpStr.encode(encoding='gbk'))
        write2sock(self.sock, tmpStr, len(tmpStr))

        result = recv_pack(self.sock)
        print('test [sign_up_res]', result)
        resultStr = result.decode(encoding='gbk')
        resultJson = json.loads(resultStr[0:-1].split('\n')[1])

        print(resultJson['msg'])

        if resultJson['error']:
            return False
        else:
            return True

    def login_signup(self):
        while True:
            start_input = input("������login����signup...\n")
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

    """
=================        db��صĲ���     ===============================
    """
    def db_select(self):
        """
        �������ƣ�db_select
        ����������null
        �������ܣ������ݿ������ɸѡ����ת��Ϊһ�¸�ʽ��
            db_map = {"./txt": {"size":5, "mtime":'hhhh-yy-dd', "md1"}, "./txt2": {"size":5, "mtime":'hhhh-yy-dd', "md2"}}
        """
        cur = self.sql_conn.cursor()
        res = cur.execute('select * from {0}'.format(self.table_name))
        db_file = res.fetchall()  # ͬ������ֵ���ʽ
        # ����һ��map��key:"path/file_name", value:(size,mtime,md5)
        db_map = {local_file['path_file_name']: {'size': local_file['size'], 'mtime': local_file['mtime'],
                                                'md5': local_file['md5']}
                  for local_file in db_file}
        return db_map

    def db_connect(self):
        """
        �������ƣ�db_connect
        ����������null
        �������ܣ��������ݿ�����ӣ��������ݿⷵ��ֵ����Ϊdict�ĸ�ʽ
        """
        # if not os.path.exists(self.per_db_name):
        #
        # print(self.per_db_name)
        self.sql_conn = sqlite3.connect(self.per_db_name)  # �����ڣ�ֱ�Ӵ���
        self.sql_conn.row_factory = dict_factory

    def db_disconnect(self):
        self.sql_conn.close()

    def db_create_local_file_table(self):
        """
        �������ƣ�db_create_local_file_table
        ����������null
        �������ܣ���ʼ��clientʱ�������ǰbind��Ŀ¼û��db�ļ���Ҳ���ǵ�һ������Ŀ¼ʱ����Ҫ������
        """
        self.db_connect()
        sql_drop = "DROP TABLE IF EXISTS {0};".format(self.table_name)
        sql = """
        CREATE TABLE {0} (
          "path_file_name" text NOT NULL,
          "size" integer,
          "mtime" integer,
          "md5" text(32),
          PRIMARY KEY ("path_file_name")
        );
        """.format(self.table_name)
        cur = self.sql_conn.cursor()
        cur.execute(sql_drop)
        cur.execute(sql)
        self.db_disconnect()

    def db_insert(self, sql_conn, relative_path, size, mtime, md5):
        """
        �������ƣ�db_insert
        ����������path_file_name, size, mtime, md5 (path_file_name is a relative path)
        �������ܣ������ݿ��в���һ������
        """
        cur = sql_conn.cursor()
        try:
            sql = "REPLACE INTO {0} VALUES ('{1}', {2}, {3}, '{4}')".format(self.table_name, relative_path, size, mtime, md5)
            # print(sql)
            cur.execute(sql)
            sql_conn.commit()
        except:
            sql_conn.rollback()

    def db_update(self, relative_path_file, size, mtime, md5):
        """
        �������ƣ�db_update
        ����������relative_path_file, size, mtime, md5
        �������ܣ���relative_path_fileΪ��������db�����ݽ��и���
        """
        cur = self.sql_conn.cursor()
        sql = "UPDATE {0} SET size = {2}, mtime = {3}, md5 = '{4}' WHERE path_file_name = '{1}'".\
            format(self.table_name, relative_path_file, size, mtime, md5)
        try:
            cur.execute(sql)
            self.sql_conn.commit()
        except:
            self.sql_conn.rollback()

    def db_delete(self, relative_path):
        """
        �������ƣ�db_delete
        ����������path_file_name
        �������ܣ���path_file_nameΪ����������Ӧ������ɾ��
        """
        cur = self.sql_conn.cursor()
        sql = "DELETE FROM {0} WHERE path_file_name = '{1}'".format(self.table_name, relative_path)
        # print(sql)
        try:
            cur.execute(sql)
            self.sql_conn.commit()
        except:
            self.sql_conn.rollback()

    """ 
=================        ����req������Ҫ��pack����ز���     ===============================
    gen�ĺ���Ϊgenerate����������ĺ���ֻ�漰����pack������pack�ŵ�req_queue���棬���漰��������
    """
    def gen_req_createDir(self, path, filename):
        que_id = self.get_que_id()
        d = {"session": self.user_session, "prefix": path, "dirname": filename, "queueid": que_id}

        req_createDir = "createDir\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_createDir)

    def gen_req_deleteDir(self, path, filename):
        que_id = self.get_que_id()
        d = {"session": self.user_session, "prefix": path, "dirname": filename, "queueid": que_id}

        req_createDir = "deleteDir\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_createDir)

    def gen_req_uploadFile(self, path, filename, size, md5, task_id, mtime):
        que_id = self.get_que_id()
        d = {"session": self.user_session, "taskid": int(task_id), "queueid": que_id,
             "filename": filename, "path": path, "md5": md5, "size": size, "mtime": mtime}

        req_uploadFile = "uploadFile\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_uploadFile)

    def handle_deleteFile(self, path_file):
        new_sock = get_connect()
        que_id = self.get_que_id()
        file = path_file.split('/')[-1]
        path = path_file[0:-len(file)]
        # print(file, path)
        d = {
            'session': self.user_session,
            'filename': file,
            'path': path,
            'queueid': que_id
        }
        req_deleteFile = "deleteFile\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        write2sock(new_sock, req_deleteFile, len(req_deleteFile))
        delete_res = recv_pack(new_sock)
        # print('deleteFile:', req_deleteFile)
        # print('delete_req_res:', delete_res)
        new_sock.close()

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
        
        self.test_req()
        return chunk_id

    def gen_req_uploadChunk(self, v_id, t_id, q_id, c_id, off, size, path, filename):
        d = {'session': self.user_session, 'vfile_id': v_id, 'taskid': t_id, 'queueid': q_id, 'chunkid': c_id,
             'offset': off, 'chunksize': size, 'path': path, 'filename': filename}
        req_up = "uploadChunk\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_up)

    # 'download', 'sub_path', 'file', 5000000, 'md5', 800000)
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
        f_path = add_path_bound(f_path)
        if not os.path.exists(self.bind_path_prefix[:-1] + f_path):
            create_path(self.bind_path_prefix[:-1] + f_path)

        if f_size > 4*1024*1024*1024:
            print('can not download or upload file bigger than 4GB')
            return

        if f_size < 0:
            print('can not download or upload file less than 0 byte')
            return

        remote_relative_path = f_path + f_name
        if task_type == 'download':
            f_d_name = f_name + '.download'
            create_file(self.bind_path_prefix, f_path, f_d_name, f_size)
            # ����Ҫ���ص��ļ���Ϣ����Ҫд��db����Ϊ���м��ļ�������ɨ��ʱ��ɨ���м��ļ�
            total = self.gen_req_download(f_path, f_d_name, f_size, md5, task_id)
            self.add_to_task_que(task_id, task_type, f_path, f_d_name, f_size, f_mtime, md5, 0, total)

        elif task_type == 'upload':
            self.gen_req_uploadFile(f_path, f_name, f_size, md5, task_id, f_mtime)
            self.add_to_task_que(task_id, task_type, f_path, f_name, f_size, f_mtime, md5, 0, 1)
            self.db_insert(self.sql_conn, relative_path=remote_relative_path, size=f_size, mtime=f_mtime, md5=md5)

        else:
            print('unknown task type')

    def test_req(self):
        print('\ntest req_queue:\n')
        for item in list(self.req_queue.queue):
            print(item.encode())

    def test_task(self):
        print('test task_queue:\n')
        for key,value in self.task_queue.items():
            print('{0}:{1}'.format(key, value))
    """ 
=================        ����req������Ҫ��pack����ز���     ===============================
    gen�ĺ���Ϊgenerate����������ĺ���ֻ�漰����pack������pack�ŵ�req_queue���棬���漰��������
    """
    def handle_getDir(self):
        d = {"session": self.user_session}
        getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        # self.sock.send(getDir)

        sock_in = get_connect()
        write2sock(sock_in, getDir, len(getDir))
        get_dir_res = recv_pack(sock_in)
        sock_in.close()

        # 2.2 ��֤getDir��res
        get_dir_str = get_dir_res.decode(encoding='gbk')
        get_dir_head = get_dir_str.split('\n')[0]
        if get_dir_head == 'getdirRes':
            # 2.3 ��֤�ɹ�����strתΪjson, �õ�������Ŀ¼�������ļ���list
            get_dir_body = get_dir_str.split('\n')[1]
            get_dir_body = get_dir_body.split('\0')[0]
            get_dir = json.loads(get_dir_body)
            if get_dir['error'] == 0:
                # print('test [get_dir_res]', get_dir_res)
                # print('test [get_dir_list]', get_dir['dir_list'])
                # print(get_dir['msg'])

                if get_dir['dir_list'] is None:
                    return []
                else:
                    return get_dir['dir_list']
        return []

    def handle_getbindid(self, sock):
        d = {'session': self.user_session}
        req_getbindid = "getbindid\n{}\0".format(json.dumps(d)).encode(encoding='gbk')
        write2sock(sock, req_getbindid, len(req_getbindid))
        res = recv_pack(sock).decode(encoding='gbk')
        res_body = res.split('\n')[1].split('\0')[0]
        res_body = json.loads(res_body)

        if 1 <= res_body['bindid'] <= 3:
            self.is_bind = res_body['bindid']
        else:
            self.is_bind = 0

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
            while not bind_id.isdigit() or int(bind_id) > 3 or int(bind_id) < 1:
                bind_id = input('������Ŀ¼�ķ�Χ��1~3��������ѡ��')

            # �γ�setbind�������Ͳ�����res
            d = {"session": self.user_session, "bindid": int(bind_id)}
            bind_pack = "setbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
            print('test [bind_pack]', bind_pack)
            # self.sock.send(bind_pack)
            write2sock(self.sock, bind_pack, len(bind_pack))
            bind_res = recv_pack(self.sock).decode(encoding='gbk')
            print('test [bind_res]', bind_res)

            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)
            # print('test', bind_res_body)

            if bind_res_body['error'] == 0:
                self.bind_path_prefix = path_translate(input_path)
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
            # self.sock.send(bind_pack)
            write2sock(self.sock, bind_pack, len(bind_pack))
            bind_res = recv_pack(self.sock).decode(encoding='gbk')
            # print('test [bind_res]', bind_res)

            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)
            # print('test [bind_res_body]', bind_res_body)

            if bind_res_body['error'] == 0:
                print('msg:%s' % bind_res_body['msg'])
                # self.bind_path_prefix = ''
                break
            else:
                print('�Ӵ���ʧ��\n msg:%s' % bind_res_body['msg'])

    def add_to_task_que(self, task_id, task_type, f_path, f_name, f_size, f_mtime, md5, cnt, total):
        d = {
            'task_type': task_type,
            'task_id': task_id,
            'f_path': f_path,
            'f_name': f_name,
            'f_size': f_size,
            'f_mtime': f_mtime,
            'md5': md5,
            'cnt': cnt,
            'total': total
        }
        self.task_queue[task_id] = d

    def first_sync(self):
        """
        �������ƣ�first_sync
        ����������self
        ��������ֵ����
        �������ܣ�1. ���г���ͬ������Ҫ����������
                2. ��������������getDir��Ȼ��������Ľ��
        """
        # �������ݿ�
        self.db_connect()

        # print('\n...[����ͬ��] ��ʼ...')
        # 1. ɨ�豾�������ļ������������ļ��Լ��ļ��е�upload api
        # ����local_file_map��key:"path/file_name", value:"md5"��֮�������˶Ա�ʱʹ��
        local_file_map = {}
        s_path = self.bind_path_prefix

        # print('\n...[����ͬ��] �ϴ����������ļ���...')
        for path, dirs, files in os.walk(s_path):
            sub_path = '/' + path[len(s_path):] + '/'
            for file in files:
                # �����ϴ����м��ļ�
                if not is_tmp_file(file):
                    relative_path = sub_path + file
                    # print('relative_path', relative_path)
                    real_path = os.path.join(path, file)
                    md5 = get_file_md5(real_path)
                    size = os.path.getsize(real_path)
                    mtime = os.path.getmtime(real_path)
                    mtime = int(mtime)

                    # �����ϴ�����
                    if size > 0:
                        sub_path = add_path_bound(sub_path)
                        self.gen_task_download_upload(task_type='upload', f_path=sub_path, f_name=file,
                                                    f_size=size, md5=md5, f_mtime=mtime)

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

        # print('\n...[����ͬ��] �ӷ��������������ļ���...')
        """
        local_file_map = {"./txt": "md5", "./txt2": "md5"}
        """
        # ��local_file��remote_dir_list���жԱ�, ������������ÿһ���ļ�remote_file
        if remote_dir_list is not []:
            # num = len(remote_dir_list)
            # now = 0
            for remote_file in remote_dir_list:
                # now += 1
                # print('\n..��ǰ{0}������{1}'.format(now, num))
                # ��map���ҵ�keyΪremote_file['filename']����
                if not is_tmp_file(remote_file['filename']):
                    remote_relative_path = remote_file['path'] + remote_file['filename']
                    local_md5 = local_file_map.get(remote_relative_path, -1)
                    # ������ؾ���ͬ���ļ�
                    if local_md5 != -1:
                        # ������ص�ͬ���ļ���md5��ͬ
                        if local_md5 != remote_file['md5']:
                            # task_type, f_path, f_name, f_size, md5
                            self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'],
                                                        f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                            
                    else:   #û��remote_file��Ӧ����
                        # task_type, f_path, f_name, f_size, md5
                        # print('f_size:', remote_file['size'])
                        self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'], f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                        
        # ���ݿ�Ͽ�����
        self.db_disconnect()
        # print('\n...[����ͬ��] ���...')

    def not_first_sync(self):
        # �������ݿ�
        self.db_connect()

        # print('\n...[�ǳ���ͬ��] ��ʼ...')
        # 1. �����ص������ļ���db���жԱȣ�ɨ�豾�������ļ�
        # ͨ��db��ȡ���ص����ݿ����е�����, �����ڴ���ά��һ����¼db��Ϣ��map
        db_map = self.db_select()
        # print(db_map)

        # self.test_task()

        # 1.1 ��ʼɨ�豾�������ļ�
        # print('\n...[�ǳ���ͬ��] ɨ�豾�������ļ����ϴ�������Ϣ��...')
        # ά��һ��list����map��¼�����ļ�����Ϣ���ļ�·�����ļ��������ڴ�
        local_file_map = {}    # ��¼�ļ������Ӧ��md5
        s_path = self.bind_path_prefix
        for path, dirs, files in os.walk(s_path):
            sub_path = '/' + path[len(s_path):] + '/'
            for file in files:
                if not is_tmp_file(file):
                    # ������Ե�ַ����ʵ��ַ
                    relative_path_file = sub_path + file
                    real_path = os.path.join(path, file)

                    # ����file�������Ϣ��size, mtime
                    size = os.path.getsize(real_path)
                    mtime = os.path.getmtime(real_path)
                    mtime = int(mtime)
                    if size > 0:
                        if relative_path_file in db_map.keys():
                            db_map_item = db_map[relative_path_file]
                            if size == db_map_item['size'] and mtime == db_map_item['mtime']:
                                local_file_map[relative_path_file] = db_map_item['md5']
                            else:
                                md5 = get_file_md5(real_path)
                                local_file_map[relative_path_file] = md5
                                if md5 != db_map_item['md5']:
                                    self.gen_task_download_upload(task_type='upload', f_path=sub_path, f_name=file, f_size=size, f_mtime=mtime, md5=md5)
                                self.db_update(relative_path_file=relative_path_file, size=size, mtime=mtime, md5=md5)     # need
                        else:
                            # ���ļ���Ϣ�Ǽǽ���local_file_map
                            md5 = get_file_md5(real_path)
                            local_file_map[relative_path_file] = md5
                            self.gen_task_download_upload(task_type='upload', f_path=sub_path, f_name=file, f_size=size, f_mtime=mtime, md5=md5)
                            

        # 1.2 ɨ��db�е����м�¼
        # print('\n...[�ǳ���ͬ��] ɨ���ϴ��˳����ļ���¼��ɾ�������������ļ���...')
        # now = 0
        # num = len(db_map.keys())
        for db_file_path_name in db_map.keys():
            # now += 1
            # print('\n..��ǰ{0}������{1}'.format(now, num))
            if (not is_tmp_file(db_file_path_name)) and (db_file_path_name not in local_file_map.keys()):
                task_dict = {}
                for task in self.task_queue.values():
                    task_path_name = task['f_path'] + task['f_name']
                    task_id = task['task_id']
                    if task_path_name == db_file_path_name:

                        # self.task_queue.pop(task_id)
                        req_list = list(self.req_queue.queue)
                        # print('req_1', req_list)
                        self.req_queue = queue.Queue()
                        for req in req_list:
                            req_body = req.split('\n')[1].split('\0')[0]
                            req_body = json.loads(req_body)
                            # print('req_', req_body)
                            if 'taskid' in req_body.keys():
                                if req_body['taskid'] != task_id:
                                    self.req_queue.put(req)
                    else:
                        task_dict[task_id] = task

                self.task_queue = task_dict
                self.handle_deleteFile(db_file_path_name)
                self.db_delete(db_file_path_name)
        # db_map = self.db_select()
        # print('db2', db_map)
        
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
        # print(remote_dir_list)
        # print('\n...[�ǳ���ͬ��] ɨ������������ļ����ӷ������������ļ���...')

        if remote_dir_list is not []:
            # now += 1
            # print('\n..��ǰ{0}������{1}'.format(now, num))
            for remote_file in remote_dir_list:
                if not is_tmp_file(remote_file['filename']):
                    # ��map���ҵ�keyΪremote_file['filename']����
                    remote_relative_path = remote_file['path'] + remote_file['filename']
                    local_md5 = local_file_map.get(remote_relative_path, -1)
                    # ������ؾ���ͬ���ļ�
                    if local_md5 != -1:
                        # ������ص�ͬ���ļ���md5��ͬ
                        if local_md5 != remote_file['md5']:
                            # task_type, f_path, f_name, f_size, f_mtime, md5
                            self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'],
                                                        f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                            
                    else:  # û��remote_file��Ӧ����
                        # task_type, f_path, f_name, f_size, md5
                        self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'],
                                                    f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                        
        # ���ݿ�Ͽ�����
        self.db_disconnect()
        # print('\n...[�ǳ���ͬ��] ���...')

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
        per_bind_name = "{0}{1}bind".format(self.init_path, str(self.user_name))
        if os.path.exists(per_bind_name):
            f = open(per_bind_name, 'r')
            data = f.read(self.BUF_SIZE)
            f.close()
            if data:
                json_data = json.loads(data)
                self.bind_path_prefix = json_data['path']
                return True
        return False


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
        per_bind_name = "{0}{1}bind".format(self.init_path, str(self.user_name))
        # w��ʽÿ�ζ����ǣ�����Ԥ��
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
        prefix = prefix_to_filename(self.bind_path_prefix)
        per_queue_name = "{0}{1}{2}queue".format(self.init_path, str(self.user_name), prefix)
        if os.path.exists(per_queue_name):
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
                for key, value in task_map.items():
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
        for v in self.res_queue.values():
            self.req_queue.put(v)

        req_list = list(self.req_queue.queue)
        task_map = self.task_queue
        d = {'req': req_list, 'task': task_map}
        d_str = json.dumps(d)

        prefix = prefix_to_filename(self.bind_path_prefix)
        per_queue_name = "{0}{1}{2}queue".format(self.init_path, str(self.user_name), prefix)
        f = open(per_queue_name, 'w')
        f.write(d_str)
        f.close()
        # print('�־û�req�Լ�task queue��Ϣ���ļ�%s<����>��' % per_queue_name)

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
        prefix = prefix_to_filename(self.bind_path_prefix)
        self.per_db_name = "{0}{1}{2}{3}db.db".format(self.init_path, str(self.user_name), prefix, self.is_bind)
        if not os.path.exists(self.per_db_name):
            self.db_create_local_file_table()
            self.first_sync()
        else:
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
        prefix = prefix_to_filename(self.bind_path_prefix)
        self.per_db_name = "{0}{1}{2}db.db".format(self.init_path, str(self.user_name), prefix)

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
            new_body['queueid'] = body['queueid']
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

        pack = "{0}\n{1}\0".format(head, json.dumps(new_body)).encode(encoding='gbk')
        # print('req2server:', pack)
        # self.sock.send(pack)
        write2sock(self.sock, pack, len(pack))

    # sender����
    def sender(self):
        print('...sender begin...')
        while self.is_die == 0:
            rs, ws, es = select.select([], [self.sock], [])
            if self.sock in ws:
                # print('sender writable', self.req_queue.empty())
                if not self.req_queue.empty():
                    init_req = self.req_queue.get()
                    # print(init_req)
                    head, body = decode_req(init_req)
                    body = json.loads(body)
                    que_id = body['queueid']

                    if head == 'uploadChunk':
                        off, size = body['offset'], body['chunksize']
                        path, filename = body.pop('path'), body.pop('filename')
                        file_path = "{0}{1}{2}".format(self.bind_path_prefix[:-1], path, filename)

                        if os.path.exists(file_path):
                            d = {
                                'session': self.user_session,
                                'vfile_id': body['vfile_id'],
                                'queueid': que_id,
                                'chunkid': body['chunkid'],
                                'offset': body['offset'],
                                'chunksize': body['chunksize']
                            }

                            # send header & body
                            req_send = "{0}\n{1}\0".format(head, json.dumps(d)).encode(encoding='gbk')
                            # self.sock.send(req_send)
                            write2sock(self.sock, req_send, len(req_send))

                            # send con
                            # file chunk
                            # now_read = 0
                            f = open(file_path, 'rb')
                            file2sock(self.sock, f, off, size)
                            f.close()
                            # while now_read < size:
                            #     f.seek(off + now_read)
                            #     data = f.read(size - now_read)
                            #     if sock_ready_to_write(self.sock):
                            #         n = self.sock.send("{}".format(data).encode())
                            #         if n == 0:
                            #             break
                            #         now_read += n

                            # print('\n\n{}\n\n'.format(now_read))
                            # self.sock.send(b'\0')

                            self.res_queue[que_id] = init_req
                            # print('\n�յ�һ��res,[res_queue]', self.res_queue)
                        else:
                            print('<sender>:���ļ�ʧ��, {}������'.format(file_path))
                    else:
                        self.req2server(head, body)
                        self.res_queue[que_id] = init_req
                        # print('\n�յ�һ��res,[res_queue]', self.res_queue)

    def task_queue_update_cnt(self, task_id, cnt, total):
        if task_id in self.task_queue.keys():
            self.task_queue[task_id]['cnt'] = cnt
            self.task_queue[task_id]['total'] = total

    def task_queue_add_cnt(self, task_id):
        
        if task_id in self.task_queue.keys():
            self.task_queue[task_id]['cnt'] += 1
            if self.task_queue[task_id]['cnt'] >= self.task_queue[task_id]['total']:
                print(self.task_queue[task_id])
                self.complete_queue[task_id] = self.task_queue.pop(task_id)
               
                return True
            print(self.task_queue[task_id])
        return False

    # def manage_api_download(self, Done, task_id, path, filename, offset, chunksize, file_content):
    #     file_path = "{0}{1}{2}".format(self.bind_path_prefix[:-1], path, filename)
    #     try:
    #         f = open(file_path, 'ab+')
    #         f.seek(offset)
    #         f.write(file_content[0:chunksize-1])
    #         f.close()
    #     except IOError:
    #         print('<download>д���ļ�ʧ�ܣ�')

    #     if Done:
    #         mtime = self.complete_queue[task_id]['f_mtime']
    #         os.utime(file_path, (mtime, mtime))
            


    # receiver����
    def receiver(self):
        print('\n... recver begin ...')
        while self.is_die == 0:
            rs, ws, es = select.select([self.sock], [], [], 1.0)
            if self.sock in rs:
                init_res = recv_pack(self.sock).decode(encoding='gbk')
                # print('\nrecver�յ�ԭʼ����', init_res.encode())

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
                if head == 'uploadFileRes' and error == 4:
                    """
                    ���������error, msg, queueid, vfile_id
                    """
                    print_info('\nrecver', error, '�ϴ��ļ�������Ϣ��{}'.format(body_json['msg']))

                    # err=4 �ļ��ڷ����������ڣ���Ҫ�ϴ�, ��res_que�еĶ�Ӧɾ��
                    req = self.res_queue.pop(que_id)
                    # ����req���õ�filename�Լ�path
                    req_body = req.split('\n')[1].split('\0')[0]
                    req_body = json.loads(req_body)
                    path, filename, taskid = req_body['path'], req_body['filename'], req_body['taskid']
                    print_info('recver', error, '�ϴ��ļ�{0} {1} ������Ϣ��{2}'.format(path, filename, body_json['msg']))
                    
                    # ��λ�ļ�����ȡ��С
                    fpath = "{0}{1}{2}".format(self.bind_path_prefix[:-1], path, filename)
                    fsize = os.path.getsize(fpath)

                    # �γ����ɸ�req uploadChunk����
                    off = 0
                    chunk_id = 0
                    s = fsize
                    while s > 0:
                        if s > self.BUF_SIZE:
                            chunk_size = self.BUF_SIZE
                        else:
                            chunk_size = s
                        que_id = self.get_que_id()
                        self.gen_req_uploadChunk(body_json['vfile_id'], taskid, que_id, chunk_id, off, s, path, filename)
                        off += chunk_size
                        chunk_id += 1
                        s -= chunk_size

                    # �γ�req��upChunk����֮�󣬽�task_que�е�cnt��total�޸�
                    self.task_queue_update_cnt(taskid, 0, chunk_id)
                    

                else:
                    if error == 0:
                        # ��res_que�еĶ�Ӧɾ��, �������õ�taskid
                        req = self.res_queue.pop(que_id)
                        # print('req!', req)
                        req_body = req.split('\n')[1].split('\0')[0]
                        req_body = json.loads(req_body)
                        # print('req/', req_body)

                        # ������task_id�������Ӧ��task��cnt�����޸�
                        if head == 'downloadFileRes':
                            # ����body�������ݰ���offд�뱾�أ����д������Ҫ����mtime
                            # f = open(req_body)
                            file_path = "{0}{1}{2}".format(self.bind_path_prefix[:-1], req_body['path'], req_body['filename'])
                            f = open(file_path, 'rb+')
                            res = sock2file(self.sock, f, req_body['offset'], req_body['chunksize'])
                            f.close()

                            print_info('recver', error, '�����ļ�{0} {1}������Ϣ:{2}'.format(req_body['path'], req_body['filename'], body_json['msg']))
                            print(req)

                            task_id = req_body['taskid']

                            if res == 0:
                                print('[receiver] [error = 0] current socket is not readable')
                                if task_id in self.task_queue.keys():
                                    if not Done:
                                        task = self.task_queue[task_id]
                                    else:
                                        task = self.complete_queue[task_id]
                                    self.gen_task_download_upload(task['task_type'], task['f_path'],
                                                                    task['f_name'], task['f_size'], task['f_mtime'], task['md5'])
                            elif res == -1:
                                print_info('recver', -1, 'read from socket error!')
                            
                            # ��Ӧtask_que�е�cnt+1
                            Done = self.task_queue_add_cnt(task_id)

                            if Done:
                                # ��ȡ�ļ���Ϣ
                                mtime = self.complete_queue[task_id]['f_mtime']
                                size = self.complete_queue[task_id]['f_size']
                                md5 = self.complete_queue[task_id]['md5']

                                # ȥ���ļ����ر�־
                                real_file_path = os.path.splitext(file_path)[0]     # ȥ��.download
                                if not os.path.exists(real_file_path):
                                    os.rename(file_path, real_file_path)

                                # note!
                                # self.test_task()
                                if os.path.exists(file_path):
                                    os.remove(file_path)
                                # �޸��ļ�mtime
                                os.utime(real_file_path, (mtime, mtime))

                                # ��db�в��������ļ�
                                tmp_sql_conn = sqlite3.connect(self.per_db_name)  # �����ڣ�ֱ�Ӵ���
                                self.db_insert(tmp_sql_conn, relative_path=real_file_path, size=size, mtime=mtime, md5=md5)
                                tmp_sql_conn.close()

                            # con = recv_pack(self.sock).decode(encoding='gbk')
                            # self.manage_api_download(Done, task_id, req_body['path'], req_body['filename'],
                            #                          req_body['offset'], req_body['chunksize'], con)

                        elif head == 'uploadFileRes' or head == 'uploadChunkRes':
                            print_info('recver', error, '�ϴ��ļ�{0} {1}������Ϣ:{2}'.format(req_body['path'], req_body['filename'], body_json['msg']))
                            task_id = req_body['taskid']
                            # ��Ӧtask_que�е�cnt+1
                            self.task_queue_add_cnt(task_id)

                    elif error == 1:
                        print_info('recver', error, '�ϴ��ļ�{0} {1}������Ϣ:{2}'.format(req_body['path'], req_body['filename'], body_json['msg']))
                        # ��res_que�ж�Ӧ��que_id�������ٴη���req_que��
                        self.req_queue.put(self.res_queue.pop(que_id))

    def sync_timer(self):
        # ֻ�Ǽ򵥵Ĳ���һ����ʱ�źţ�����Ҫ��ȷ�Ķ�ʱ����˿���ʹ��sleep
        while self.is_die == 0:
            self.update_flag += 1
            time.sleep(1)


    def IO_control(self):
        # ���̴߳������IO
        while self.is_die == 0:
            io_str = input('...������ָ��...\n')
            print('[input] {}'.format(io_str))
            if io_str == "logout":
                self.is_die = 1

            elif io_str == "exit":
                self.is_die = 1

            elif io_str == "op3":
                print('op3')

            else:
                print('���������ָ��..')

    def handle_remotebind(self):# if is_bind=0, bind; not = 0, ask if disbind
        if self.is_bind == 0:
            # bind remote
            print('\n��󶨷�����Ŀ¼...')
            bind_id = input('��������ѡĿ¼��1��2��3\n��ѡ��Ҫ�󶨵�Ŀ¼��')
            while not bind_id.isdigit() or int(bind_id) > 3 or int(bind_id) < 1:
                bind_id = input('������Ŀ¼�ķ�Χ��1~3��������ѡ��')

            # �γ�setbind�������Ͳ�����res
            d = {"session": self.user_session, "bindid": int(bind_id)}
            bind_pack = "setbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
            # print('test [bind_pack]', bind_pack)
            # self.sock.send(bind_pack)
            write2sock(self.sock, bind_pack, len(bind_pack))
            bind_res = recv_pack(self.sock).decode(encoding='gbk')
            # print('test [bind_res]', bind_res)

            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)
            # print('test', bind_res_body)

            if bind_res_body['error'] == 0:
                self.is_bind = bind_res_body['bindid']
                print('msg:%s' % bind_res_body['msg'])
                return
            else:
                print('��ʧ��,bindid:%s\n msg:%s' % (bind_res_body['bindid'], bind_res_body['msg']))
                exit(0)
        else:
            # is disbind
            res = is_cancel_bind()
            if res:
                self.handle_cancel_bind()
                print('\n����󶨳ɹ��������µ�¼')
                exit(0)

    def handle_localbind(self):
        input_path = input('ȷ�����ذ󶨵�Ŀ¼:')
        while not os.path.exists(input_path):
            print('ѡ���Ŀ¼��%s������' % input_path)
            input_path = input('���ٴ�ȷ�����ذ󶨵�Ŀ¼:')
        self.bind_path_prefix = path_translate(input_path)
        self.write_bind_persistence()

    def run(self):
        """
        �������ƣ�run
        ����������self
        �������ܣ��������������е���Ҫ����
        """

        # �����½��ע���¼���ֱ����½�ɹ��Ž�������
        self.login_signup()

        # getbindid
        self.handle_getbindid(self.sock)

        # if is_bind=0, bind; not = 0, ask if disbind
        self.handle_remotebind()

        # ������bindid��>0��

        # ��bind�ĳ־û����ݶ�ȡ����
        res = self.read_bind_persistence()
        # print(self.bind_path_prefix)
        if not res:
            self.handle_localbind()

        signal.signal(signal.SIGINT, self.handler_signal)

        # ����queue�ĳ־û��ļ�������ϴ�δ��ɻ��ٴδ�������
        self.read_queue_persistence()
        # ����db�ĳ־û��ļ������бȶԲ�ͬ��
        self.read_db_persistence()

        # ��������Ϊ������
        self.sock.setblocking(False)

        # �����̲߳�����
        t_sender = threading.Thread(target=self.sender)
        t_receiver = threading.Thread(target=self.receiver)
        t_timer = threading.Thread(target = self.sync_timer)

        t_sender.start()
        t_receiver.start()
        t_timer.start()

        t_io = threading.Thread(target=self.IO_control)
        t_io.start()

        print('���߳�ѭ����ʼ...')
        # ���̴߳���ȫ����Ӧ
        print()
        while True:
            # print('update_flag:', self.update_flag)
            if self.update_flag >= 10:
                self.not_first_sync()
                self.update_flag = -1
            # print('is_die:', self.is_die)
            # if self.update_flag == 1:
            #     self.update_flag = 0
            #     print('\n����ͬ��ing...')
            #     self.not_first_sync()

            if self.is_die == 1:
                t_sender.join()
                print('sender end')
                t_receiver.join()
                print('recv end')
                t_io.join()
                print('io end')
                t_timer.join()
                print('timer end')

                # ���ն��־û��ļ��ķ�˳����г־û�
                # self.write_db_persistence()
                self.write_queue_persistence()
                self.write_bind_persistence()
                print('persistence end')

                # �˳���½�������˳���½��api
                self.handle_logout()
                print('send log out')

                self.sock.close()
                print('close socket')
                sys.exit(0)

            time.sleep(1)


if __name__ == '__main__':
    client = Client()
    client.run()