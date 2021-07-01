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


def add_path_bound(path):
    if path[0] != '/':
        path = '/' + path
    if path[-1] != '/':
        path += '/'
    return path


def get_file_md5(f_name):
    """
    输入参数：(1)path 文件路径
            (2)f_name 文件名
    输出参数：文件的md5
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
    从配置文件中读取连接服务器的ip和port
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
        print("try to connect to %s:%s" % (host_, port_))
        sock.connect((host_, port_))
    except socket.error:
        print("Could not make a connection to the server")
        sys.exit(0)
    print('connect success!')
    return sock


def decode_req(req):
    """
    输入参数：req(str), format: req:"head\n{body}\0"
    输出参数: 1. head(str)
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
    # 遍历path_list，生成不存在的path
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
        print("密码至少由8位组成。")
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
        print("密码至少包含数字、大写、小写字母。")
        return False


def is_cancel_bind():
    """
    函数名称：is_cancel_bind
    函数参数：self
    函数功能：让用户选择是否要取消绑定目录
    """
    cancel_str = input("请确定是否需要解除绑定？y/Y解除绑定，n/N不接触绑定")
    if cancel_str == 'y' or cancel_str == 'Y':
        cancel_str_2 = input("输入y/Y再次确认")
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


# 不一定需要
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
    rs, ws, es = select.select([], [sock], [], 1.0)
    if sock in ws:
        return True
    return False


def sock_ready_to_read(sock):
    rs, ws, es = select.select([sock], [], [], 1.0)
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
    调用结束记得加尾0
    """
    cnt = 0
    size = chunksize
    # r_pipe, w_pipe = os.pipe()
    while cnt != chunksize:
        ready = sock_ready_to_read(sock)
        if not ready:
            return 0

        # r_bytes = splice(sock.fileno(), r_pipe, offset=off, nbytes=size)
        # w_bytes = splice(w_pipe, file.fileno(), offset=off, nbytes=size)
        # print('read {0} bytes from sock, and write {1} bytes to file'.format(r_bytes, w_bytes))
        data = sock.recv(size)
        file.seek(off)
        w_bytes = file.write(data)
        # print(data, size, w_bytes)
        if w_bytes == 0:
            break
        if w_bytes == -1:
            return -1
        cnt += w_bytes
        off += w_bytes
        size -= w_bytes

    ch = sock.recv(1)
    # while ch != b'\0':
    #     ch = sock.recv(1)
    return 1


class Client:
    def __init__(self):
        self.init_path = './init/'  # 存储需要存放文件的目录
        self.user_name = ''  # 用户账号
        self.user_pwd = ''  # 用户密码
        self.user_session = ''  # 用户session
        self.BUF_SIZE = 4194304  #
        self.GET_DIR_SIZE = 4194304     # 4M
        self.is_die = 0  # 是否需要暂停线程
        self.is_bind = 0  # 用户是否已经绑定目录
        self.bind_path_prefix = ''  # 当前本地绑定目录
        self.sql_conn = 0  # 连接数据库
        self.req_queue = queue.Queue()  # request请求队列，分裂后的（子任务）
        self.res_queue = {}  # response等待响应队列，从request队列里面get出来再放在res队列
        self.task_queue = {}  # 任务队列，存储上传、下载等等用户任务（大任务）
        self.complete_queue = {}  # 任务完成队列，完成后从task里面放进来
        self.per_db_name = ''
        self.que_id = 0
        self.chunk_id = 0
        self.task_id = 0
        self.table_name = 'local_file_table'
        self.mutex = threading.Lock()
        self.update_flag = 0

        # 初始化客户端
        print('初始化客户端...')

        # 0. 创建目录
        self.init_path = path_translate(self.init_path)
        create_path(self.init_path)

        # 1. 建立sock连接
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # 连接server
        host_, port_ = read_config('./config.cfg')
        try:
            print("try to connect to %s:%s" % (host_, port_))
            self.sock.connect((host_, port_))
        except socket.error:
            print("Could not make a connection to the server")
            sys.exit(0)

        print('connect success!\n 初始化完成！')

    # need :
    def handle_login(self):
        """
        函数名称：handle_login
        函数参数：self
        函数返回值：bool，登陆成功为True，错误为False，需要输出错误原因
        函数功能：让用户输入账号和密码进行登陆
        函数步骤：1. 要求用户输入账号和密码
                2. 将账号和密码形成请求包，通过self.sock发送（阻塞）
                3. 接收response，解析res，根据error判断失败成功
                4.  成功则打印消息，更改全局self.user信息以及session信息，return True；
                    失败则打印消息，return False
        请求格式：signup\n{"username": "root","password": "root123"}\0
                （需要str转bytes格式，以API文档为准）
        """
        print('login')
        # 大致重要的操作
        name = input('请输入账号：')
        pwd = getpass.getpass('请输入密码：')
        d = {
            'username': name,
            'password': pwd
        }
        login_req = "login\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')

        # 默认sock在调用函数之前连接成功
        # self.sock.send(login_req.encode(encoding='gbk'))
        write2sock(self.sock, login_req, len(login_req))
        res_bytes = self.recv_pack()
        res = res_bytes.decode(encoding='gbk')
        # print(res)

        res_head = res.split('\n')[0]
        res_body = res.split('\n')[1]
        res_body = json.loads(res_body[0:-1])

        if res_head == 'loginRes':
            if res_body['error']:
                # 处理失败的操作
                print('登录失败！')
            else:
                # 处理成功的操作
                print('登录成功！')
                self.user_session = res_body['session']
                self.user_name = name
                self.user_pwd = pwd
                return True

        # 这里统一打印错误结果
        print(res_body['msg'])
        return False

    def handle_logout(self):
        """
        函数名称：handle_logout
        函数参数：self
        函数返回值：bool，退出成功为True，失败为False，需要输出错误原因
        函数步骤：1. 形成退出请求包，并通过sock发送
                2. 接收sock的res，并解析，根据error判断退出成功失败
                3.  成功，打印信息，返回True
                    失败，打印信息（失败原因），返回False
        """
        print('loginout')

        d = {'session': self.user_session}
        request = "logout\n{0}\0".format(json.dumps(d))

        # 默认sock在调用函数之前连接成功
        request = request.encode(encoding='gbk')
        write2sock(self.sock, request, len(request))

        res_bytes = self.recv_pack()
        res = res_bytes.decode(encoding='gbk')

        res_head = res.split('\n')[0]
        res_body = res.split('\n')[1]
        res_body = json.loads(res_body[0:-1])

        if res_head == 'logoutRes':
            if res_body['error']:
                # 处理失败的操作
                print('登出失败！')
            else:
                # 处理成功的操作
                print('登出成功！')
                self.user_session = ''
                self.user_name = ''
                self.user_pwd = ''
                return True

        # 这里统一打印错误结果
        print(res_body['msg'])
        return False

    def handle_signup(self):
        """
        函数名称：handle_signup
        函数参数：self
        函数功能：bool，成功为True，失败为False，需要输出错误原因
        函数步骤：1. 要求用户输入账号和密码
                 2. 在客户端判断密码的安全性，不安全则重新输入密码（也可以要求重新输入账号）
                 2. 形成注册请求包，并通过sock发送
                 2. 接收sock的res，并解析，判断退出成功失败
                 3.  成功，打印信息，返回True
                失败，打印信息（失败原因），返回False
        """
        print('signup:')
        account = input('请输入账号：')
        password = getpass.getpass('请输入密码：')

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

        result = self.recv_pack()
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
            start_input = input("请输入login或者signup...")
            if start_input == "login":
                login_res = self.handle_login()
                if not login_res:  # 登陆失败
                    print('请重新登录...')
                    continue
                else:  # 登陆成功
                    print('登录成功...')
                    break

            elif start_input == "signup":
                signup_res = self.handle_signup()
                if not signup_res:
                    print('请重新注册...')
                    continue
                else:
                    print('注册成功...')
            else:
                print('请检查输入指令，包括拼写、大小写..')

    def init_queue(self):
        """ 初始化队列 """
        if self.req_queue.not_empty:
            self.req_queue = queue.Queue()
        self.res_queue = {}
        self.task_queue = {}
        self.complete_queue = {}

    def get_que_id(self):
        # 主线程和recver线程都会调用，所以要加锁
        self.mutex.acquire()
        que_id = self.que_id
        if self.que_id >= 10000:
            self.que_id = 0
        else:
            self.que_id += 1
        self.mutex.release()
        return que_id

    def get_task_id(self):
        # 都是主线程调用这个函数
        task_id = self.task_id
        if self.task_id >= 10000:
            self.task_id = 0
        else:
            self.task_id += 1
        return task_id

    """
=================        db相关的操作     ===============================
    """
    def db_select(self):
        """
        函数名称：db_select
        函数参数：null
        函数功能：将数据库的内容筛选，并转化为一下格式：
            db_map = {"./txt": {"size":5, "mtime":'hhhh-yy-dd', "md1"}, "./txt2": {"size":5, "mtime":'hhhh-yy-dd', "md2"}}
        """
        cur = self.sql_conn.cursor()
        res = cur.execute('select * from {0}'.format(self.table_name))
        db_file = res.fetchall()  # 同样变成字典样式
        # 构造一个map，key:"path/file_name", value:(size,mtime,md5)
        db_map = {local_file['path_file_name']: {'size': local_file['size'], 'mtime': local_file['mtime'],
                                                'md5': local_file['md5']}
                  for local_file in db_file}
        return db_map

    def db_init(self):
        """
        函数名称：db_init
        函数参数：null
        函数功能：进行数据库的连接，并将数据库返回值设置为dict的格式
        """
        # if not os.path.exists(self.per_db_name):
        #
        # print(self.per_db_name)
        self.sql_conn = sqlite3.connect(self.per_db_name)  # 不存在，直接创建
        self.sql_conn.row_factory = dict_factory

    def db_create_local_file_table(self):
        """
        函数名称：db_create_local_file_table
        函数参数：null
        函数功能：初始化client时，如果当前bind的目录没有db文件，也就是第一次连接目录时，需要创建表
        """
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

    def db_insert(self, relative_path, size, mtime, md5):
        """
        函数名称：db_insert
        函数参数：path_file_name, size, mtime, md5 (path_file_name is a relative path)
        函数功能：像数据库中插入一条数据
        """
        cur = self.sql_conn.cursor()
        try:
            sql = "INSERT INTO {0} VALUES ('{1}', {2}, {3}, '{4}')".format(self.table_name, relative_path, size, mtime, md5)
            # print(sql)
            cur.execute(sql)
            self.sql_conn.commit()
        except:
            self.sql_conn.rollback()

    def db_update(self, relative_path_file, size, mtime, md5):
        """
        函数名称：db_update
        函数参数：relative_path_file, size, mtime, md5
        函数功能：以relative_path_file为主键，对db的内容进行更新
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
        函数名称：db_delete
        函数参数：path_file_name
        函数功能：以path_file_name为主键，将对应的内容删除
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
=================        生成req队列需要的pack的相关操作     ===============================
    gen的含义为generate，因此命名的函数只涉及产生pack，并将pack放到req_queue里面，不涉及将包发送
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

    def gen_req_deleteFile(self, path_file):
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
        req_deleteFile = "deleteFile\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_deleteFile)

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

    def gen_req_uploadChunk(self, v_id, t_id, q_id, c_id, off, size, path, filename):
        d = {'session': self.user_session, 'vfile_id': v_id, 'taskid': t_id, 'queueid': q_id, 'chunkid': c_id,
             'offset': off, 'chunksize': size, 'path': path, 'filename': filename}
        req_up = "uploadChunk\n{0}\0".format(json.dumps(d))
        self.req_queue.put(req_up)

    # 'download', 'sub_path', 'file', 5000000, 'md5', 800000)
    def gen_task_download_upload(self, task_type, f_path, f_name, f_size, f_mtime, md5):
        """
        函数名称：gen_task_download_upload
        函数参数：self, task_type, f_path, f_name, f_size, md5
        函数返回值：无
        函数功能：1. 生成下载和上传任务
                2. 对于下载任务，(1)根据文件的大小生成若干个下载的req包到req_que中，
                                (2)然后生成task,对应cnt=0，total=num(req)，(3)然后根据路径本地建立文件夹以及文件
                3. 对于上传任务，(1) 生成一个上传的req包到req_que中，
                                (2)生成一个task，对应cnt=0,total=1。只有当recv接收到req的res之后并且秒传失败，才将total置为num(req)
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

        if task_type == 'download':
            create_file(self.bind_path_prefix, f_path, f_name, f_size)
            total = self.gen_req_download(f_path, f_name, f_size, md5, task_id)
            self.add_to_task_que(task_id, task_type, f_path, f_name, f_size, f_mtime, md5, 0, total)

        elif task_type == 'upload':
            self.gen_req_uploadFile(f_path, f_name, f_size, md5, task_id, f_mtime)
            self.add_to_task_que(task_id, task_type, f_path, f_name, f_size, f_mtime, md5, 0, 1)

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
=================        生成req队列需要的pack的相关操作     ===============================
    gen的含义为generate，因此命名的函数只涉及产生pack，并将pack放到req_queue里面，不涉及将包发送
    """
    def handle_getDir(self):
        d = {"session": self.user_session}
        getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        # self.sock.send(getDir)
        write2sock(self.sock, getDir, len(getDir))
        get_dir_res = self.recv_pack()
        # print('test [get_dir_res]')
        # print(get_dir_res)

        # 2.2 验证getDir的res
        get_dir_str = get_dir_res.decode(encoding='gbk')
        get_dir_head = get_dir_str.split('\n')[0]
        if get_dir_head == 'getdirRes':
            # 2.3 验证成功，将str转为json, 得到服务器目录的所有文件的list
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

    def handle_bind(self):
        """
        函数名称：handle_bind
        函数参数：self
        函数功能：处理绑定同步目录，主要步骤包括：
                1. 确定本地绑定的目录，修改全局变量
                2. 确定服务器需要绑定的目录
                3. 更新全局变量
        """
        while True: # 直到绑定成功才退出循环
            input_path = input('确定本地绑定的目录:')
            while not os.path.exists(input_path):
                print('选择的目录：%s不存在' % input_path)
                input_path = input('请再次确定本地绑定的目录:')

            bind_id = input('服务器可选目录：1，2，3\n请选择要绑定的目录：')
            while int(bind_id) > 3 or int(bind_id) < 1:
                bind_id = input('服务器目录的范围：1~3，请重新选择：')

            # 形成setbind包，发送并解析res
            d = {"session": self.user_session, "bindid": int(bind_id)}
            bind_pack = "setbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
            print('test [bind_pack]', bind_pack)
            # self.sock.send(bind_pack)
            write2sock(self.sock, bind_pack, len(bind_pack))
            bind_res = self.recv_pack().decode(encoding='gbk')
            print('test [bind_res]', bind_res)

            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)
            # print('test', bind_res_body)

            if bind_res_body['error'] == 0:
                self.bind_path_prefix = path_translate(input_path)
                print('msg:%s' % bind_res_body['msg'])
                break
            else:
                print('绑定失败,bindid:%s\n msg:%s' % (bind_res_body['bindid'], bind_res_body['msg']))

    def handle_cancel_bind(self):
        """
        函数名称：handle_cancel_bind
        函数参数：self
        函数功能：处理取消绑定的请求，发送取消绑定的包
        """
        while True: # 直到绑定成功才退出循环
            # 形成disbind包，发送并解析res
            d = {"session": self.user_session}
            bind_pack = "disbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
            # self.sock.send(bind_pack)
            write2sock(self.sock, bind_pack, len(bind_pack))
            bind_res = self.recv_pack().decode(encoding='gbk')
            print('test [bind_res]', bind_res)

            bind_res_body = bind_res.split('\n')[1].split('\0')[0]
            bind_res_body = json.loads(bind_res_body)
            print('test [bind_res_body]', bind_res_body)

            if bind_res_body['error'] == 0:
                print('msg:%s' % bind_res_body['msg'])
                self.bind_path_prefix = ''
                break
            else:
                print('接触绑定失败\n msg:%s' % bind_res_body['msg'])

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
        函数名称：first_sync
        函数参数：self
        函数返回值：无
        函数功能：1. 进行初次同步，主要任务是下载
                2. 操作：首先请求getDir，然后处理请求的结果
        """

        # 1. 扫描本地所有文件，生成所有文件以及文件夹的upload api
        # 构造local_file_map，key:"path/file_name", value:"md5"，之后与服务端对比时使用
        local_file_map = {}
        s_path = self.bind_path_prefix

        for path, dirs, files in os.walk(s_path):
            sub_path = path[len(s_path):] + '/'
            for file in files:
                # 计算md5
                # print('test', path, file)
                # print('os_path', os.path.join(path, file))
                relative_path = sub_path + file
                # print('relative_path', relative_path)
                real_path = os.path.join(path, file)
                md5 = get_file_md5(real_path)
                size = os.path.getsize(real_path)
                mtime = os.path.getmtime(real_path)
                mtime = int(mtime)

                # 生成上传任务
                sub_path = add_path_bound(sub_path)
                self.gen_task_download_upload(task_type='upload', f_path=sub_path, f_name=file,
                                              f_size=size, md5=md5, f_mtime=mtime)

                # 将文件信息登记进入local_file_map
                local_file_map[relative_path] = md5

        # 2. 请求服务端的所有数据，遍历服务端的所有数据
        # 2.1 发送getdir请求
        """
        list = [{
            "filename":,
            "path":,（两个表在服务器进行连接）
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
        # 将local_file与remote_dir_list进行对比, 遍历服务器的每一个文件remote_file
        if remote_dir_list is not []:
            for remote_file in remote_dir_list:
                # 在map中找到key为remote_file['filename']的项
                remote_relative_path = remote_file['path'] + remote_file['filename']
                local_md5 = local_file_map.get(remote_relative_path, -1)
                # 如果本地具有同名文件
                if local_md5 != -1:
                    # 如果本地的同名文件的md5不同
                    if local_md5 != remote_file['md5']:
                        # task_type, f_path, f_name, f_size, md5
                        self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'],
                                                      f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                        # 将需要下载的文件信息写入db
                        self.db_insert(relative_path=remote_relative_path, size=remote_file['size'], mtime=remote_file['mtime'], md5=remote_file['md5'])

                else:   #没有remote_file对应的文
                    # task_type, f_path, f_name, f_size, md5
                    print('f_size:', remote_file['size'])
                    self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'], f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                    # 将需要下载的文件信息写入db
                    self.db_insert(relative_path=remote_relative_path, size=remote_file['size'], mtime=remote_file['mtime'], md5=remote_file['md5'])

    def not_first_sync(self):
        # 1. 将本地的所有文件与db进行对比，扫描本地所有文件
        # 通过db获取本地的数据库所有的内容, 并在内存中维护一个记录db信息的map
        db_map = self.db_select()
        # db_map = {"./txt": {"size":5, "mtime":'hhhh-yy-dd', "md1"}, "./txt2": (6, 'hhhh-yy-dd', "md2")}

        # 1.1 开始扫描本地所有文件
        # 维护一个list或者map记录本地文件的信息（文件路径、文件名）到内存
        local_file_map = {}    # 记录文件名与对应的md5
        s_path = self.bind_path_prefix
        for path, dirs, files in os.walk(s_path):
            sub_path = path[len(s_path):] + '/'
            for file in files:
                # 计算相对地址与真实地址
                relative_path_file = sub_path + file
                real_path = os.path.join(path, file)

                # 计算file的相关信息：size, mtime
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
                            self.gen_task_download_upload(task_type='upload', f_path=sub_path, f_name=file, f_size=size, f_mtime=mtime, md5=md5)
                        self.db_update(relative_path_file=relative_path_file, size=size, mtime=mtime, md5=md5)     # need
                else:
                    # 将文件信息登记进入local_file_map
                    md5 = get_file_md5(real_path)
                    local_file_map[relative_path_file] = md5
                    self.gen_task_download_upload(task_type='upload', f_path=sub_path, f_name=file, f_size=size, f_mtime=mtime, md5=md5)
                    self.db_insert(relative_path=relative_path_file, size=size, mtime=mtime, md5=md5)   # need

        # 1.2 扫描db中的所有记录
        for db_file_path_name in db_map.keys():
            if db_file_path_name not in local_file_map.keys():
                task_dict = {}
                for task in self.task_queue.values():
                    task_path_name = task['f_path'] + task['f_name']
                    task_id = task['task_id']
                    if task_path_name == db_file_path_name:

                        # self.task_queue.pop(task_id)
                        req_list = list(self.req_queue.queue)
                        print('req_1', req_list)
                        self.req_queue = queue.Queue()
                        for req in req_list:
                            req_body = req.split('\n')[1].split('\0')[0]
                            req_body = json.loads(req_body)
                            # print('req_', req_body)
                            if req_body['taskid'] != task_id:
                                self.req_queue.put(req)
                    else:
                        task_dict[task_id] = task

                self.task_queue = task_dict
                self.gen_req_deleteFile(db_file_path_name)
                self.db_delete(db_file_path_name)

        # 1.3 请求服务端的所有数据，遍历服务端的所有数据
        # 发送getdir请求
        remote_dir_list = self.handle_getDir()
        """
        list = [{
            "filename":,
            "path":,（两个表在服务器进行连接）
            "size":
            "md5"
            "modtime"
        },
        {},...,
        {}]
        """
        # 将local_file与remote_dir_list进行对比, 遍历服务器的每一个文件remote_file
        print(remote_dir_list)
        if remote_dir_list is not []:
            for remote_file in remote_dir_list:
                # 在map中找到key为remote_file['filename']的项
                remote_relative_path = remote_file['path'] + remote_file['filename']
                local_md5 = local_file_map.get(remote_relative_path, -1)
                # 如果本地具有同名文件
                if local_md5 != -1:
                    # 如果本地的同名文件的md5不同
                    if local_md5 != remote_file['md5']:
                        # task_type, f_path, f_name, f_size, f_mtime, md5
                        self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'],
                                                      f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                        # 将需要下载的文件信息写入db
                        self.db_insert(relative_path=remote_relative_path, size=remote_file['size'], mtime=remote_file['mtime'], md5=remote_file['md5'])

                else:  # 没有remote_file对应的文
                    # task_type, f_path, f_name, f_size, md5
                    self.gen_task_download_upload(task_type='download', f_path=remote_file['path'], f_name=remote_file['filename'],
                                                  f_size=remote_file['size'], f_mtime=remote_file['mtime'], md5=remote_file['md5'])
                    # 将需要下载的文件信息写入db
                    self.db_insert(relative_path=remote_relative_path, size=remote_file['size'], mtime=remote_file['mtime'], md5=remote_file['md5'])

    def read_bind_persistence(self):
        """
        函数名称：read_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 判断bind持久化文件是否存在，
                    持久化文件不存在：is_bind = 0
                3. 存在，并根据user_name将持久化bind文件读取，获得is_bind以及bind_path信息
        """
        # 2.1 绑定信息
        per_bind_name = "{0}{1}bind".format(self.init_path, str(self.user_name))
        if os.path.exists(per_bind_name):
            f = open(per_bind_name, 'r')
            data = f.read(self.BUF_SIZE)
            f.close()
            if data:
                json_data = json.loads(data)
                self.is_bind = json_data['is_bind']
                self.bind_path_prefix = json_data['path']
            else:
                self.is_bind = 0
        else:
            self.is_bind = 0

    def write_bind_persistence(self):
        """
        函数名称：write_bind_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 判断bind持久化文件是否存在，
                    持久化文件不存在：is_bind = 0
                3. 存在，并根据user_name将持久化bind文件读取，获得is_bind以及bind_path信息
        """
        # 2.1 绑定信息
        per_bind_name = "{0}{1}bind".format(self.init_path, str(self.user_name))
        f = open(per_bind_name, 'w')
        d = {'is_bind': self.is_bind, 'path': self.bind_path_prefix}
        d_str = json.dumps(d)
        f.write(d_str)
        f.close()

    def read_queue_persistence(self):
        """
        函数名称：read_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_name以及bind信息确定queue文件是否存在
                2. 不存在，说明第一次绑定该本地目录，queue不做处理，为空
                3. 存在，并根据user_name将持久化queue读入内存，初始化queue
        """
        # 1. 清空全局的queue
        self.init_queue()

        # 2. 判断持久化文件是否存在（持久化文件：1.绑定信息，2.queue信息，3.db信息）
        # 2.2 queue信息
        prefix = prefix_to_filename(self.bind_path_prefix)
        per_queue_name = "{0}{1}{2}queue".format(self.init_path, str(self.user_name), prefix)
        if os.path.exists(per_queue_name):
            f = open(per_queue_name, 'r')
            data = f.read(self.BUF_SIZE)
            f.close()
            if data:
                json_data = json.loads(data)
                # queue的形式 "op_type\n{content}\0"
                # important!
                req_json = json_data['req']
                task_map = json_data['task']
                for req_item in req_json:
                    self.req_queue.put(req_item)
                for key, value in task_map.items():
                    self.task_queue[key] = value

    def write_queue_persistence(self):
        """
        函数名称：write_queue_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_name以及bind信息确定queue文件是否存在
                2. 不存在，说明第一次绑定该本地目录，queue不做处理，为空
                3. 存在，并根据user_name将持久化queue读入内存，初始化queue
        """
        # 2.2 queue信息
        print('<开始>持久化req以及task queue信息到文件')
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
        print('持久化req以及task queue信息到文件%s<结束>！' % per_queue_name)

    def read_db_persistence(self):
        """
        函数名称：read_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_name以及bind信息确定db文件是否存在
                2. 不存在，说明第一次绑定该本地目录，调用first_sync
                3. 存在，调用not_first_sync
        """
        # 2. 判断持久化文件是否存在（持久化文件：1.绑定信息，2.queue信息，3.db信息）
        # 2.2 db信息
        prefix = prefix_to_filename(self.bind_path_prefix)
        self.per_db_name = "{0}{1}{2}db.db".format(self.init_path, str(self.user_name), prefix)
        if not os.path.exists(self.per_db_name):
            self.db_init()
            self.db_create_local_file_table()
            self.first_sync()
        else:
            self.db_init()
            self.not_first_sync()

    # 有可能这个操作不需要
    def write_db_persistence(self):
        """
        函数名称：write_db_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_name以及bind信息确定db文件是否存在
                2. 不存在，说明第一次绑定该本地目录，调用first_sync
                3. 存在，调用not_first_sync
        """
        # 2. 判断持久化文件是否存在（持久化文件：1.绑定信息，2.queue信息，3.db信息）
        # 2.2 db信息
        prefix = prefix_to_filename(self.bind_path_prefix)
        self.per_db_name = "{0}{1}{2}db.db".format(self.init_path, str(self.user_name), prefix)

    def req2server(self, head, body):
        new_body = {}
        if head =="uploadFile":
            """
            输入参数：body(dict):{"session":"32个char","taskid":,"queueid":,"filename": ,"path": ,"md5": ,"size": ,"mtime": ,}
            输出参数：去掉taskid，并添加包头以及尾0，发送包
            """
            # 这样写是为了方便添加和删除参数
            new_body['session'] = body['session']
            new_body['queueid'] = body['queueid']
            new_body['filename'] = body['filename']
            new_body['path'] = body['path']
            new_body['md5'] = body['md5']
            new_body['size'] = body['size']
            new_body['mtime'] = body['mtime']

        elif head == 'downloadFile':
            """
            输入参数：{"session":"32个char","filename":"path":"md5":"taskid""queueid":,"chunk","offset":,"chunksize":}
            输出参数：去掉filename, path, taskid, chunkid
            """
            new_body['session'] = body['session']
            new_body['md5'] = body['md5']
            new_body['queueid'] = body['queueid']
            new_body['offset'] = body['offset']
            new_body['chunksize'] = body['chunksize']

        elif head == 'deleteFile':
            """
            输入参数：{"session":,"filename":,"path":,"queueid":,}
            输出参数：还是这4个
            """
            new_body['session'] = body['session']
            new_body['filename'] = body['filename']
            new_body['path'] = body['path']
            new_body['queueid'] = body['queueid']

        elif head == 'createDir':
            """
            输入参数：{"session":,"prefix":,"dirname":,"queueid":,}
            输出参数：还是这4个
            """
            new_body['session'] = body['session']
            new_body['prefix'] = body['prefix']
            new_body['dirname'] = body['dirname']
            new_body['queueid'] = body['queueid']

        elif head == 'deleteDir':
            """
            输入参数：{"session":,"prefix":,"dirname":,"queueid":,}
            输出参数：还是这4个
            """
            new_body['session'] = body['session']
            new_body['prefix'] = body['prefix']
            new_body['dirname'] = body['dirname']
            new_body['queueid'] = body['queueid']

        pack = "{0}\n{1}\0".format(head, json.dumps(new_body)).encode(encoding='gbk')
        # print('req2server:', pack)
        # self.sock.send(pack)
        write2sock(self.sock, pack, len(pack))

    # sender进程
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
                            # print('\n收到一份res,[res_queue]', self.res_queue)
                        else:
                            print('<sender>:打开文件失败, {}不存在'.format(file_path))
                    else:
                        self.req2server(head, body)
                        self.res_queue[que_id] = init_req
                        # print('\n收到一份res,[res_queue]', self.res_queue)

    def task_queue_update_cnt(self, task_id, cnt, total):
        self.task_queue[task_id]['cnt'] = cnt
        self.task_queue[task_id]['total'] = total

    def task_queue_add_cnt(self, task_id):
        self.task_queue[task_id]['cnt'] += 1
        if self.task_queue[task_id]['cnt'] >= self.task_queue[task_id]['total']:
            self.complete_queue[task_id] = self.task_queue.pop(task_id)
            # print('\ntask完成!\n')
            # self.test_task()
            # print(self.complete_queue)
            return True
        return False

    def manage_api_download(self, Done, task_id, path, filename, offset, chunksize, file_content):
        file_path = "{0}{1}{2}".format(self.bind_path_prefix[:-1], path, filename)
        try:
            f = open(file_path, 'ab+')
            f.seek(offset)
            f.write(file_content[0:chunksize-1])
        except IOError:
            print('<download>写入文件失败！')

        if Done:
            mtime = self.complete_queue[task_id]['f_mtime']
            os.utime(file_path, (mtime, mtime))

    def recv_pack(self):
        res = b''
        ch = self.sock.recv(1)
        while ch != b'\0':
            res += ch
            # print(ch)
            ch = self.sock.recv(1)
        res += ch
        return res

    # receiver进程
    def receiver(self):
        print('\n... recver begin ...')
        while self.is_die == 0:
            rs, ws, es = select.select([self.sock], [], [])
            if self.sock in rs:
                init_res = self.recv_pack().decode(encoding='gbk')
                # print('\nrecver收到原始数据', init_res.encode())

                # 解析接收数据的res头以及res_body并转化为json的dict格式
                head = init_res.split('\n')[0]
                body_con = init_res.split('\n')[1]
                body = body_con.split('\0')[0]
                body_json = json.loads(body)

                # 记录需要的error信息和queue_id信息
                error = body_json['error']
                que_id = body_json['queueid']

                # 文件秒传失败，需要上传文件的详细内容
                # 需要修改task的cnt和total
                if head == 'uploadFileRes' and error == 4:
                    """
                    输入参数：error, msg, queueid, vfile_id
                    """
                    # print(body_json['msg'])

                    # err=4 文件在服务器不存在，需要上传, 将res_que中的对应删除
                    req = self.res_queue.pop(que_id)
                    # 解析req，得到filename以及path
                    req_body = req.split('\n')[1].split('\0')[0]
                    req_body = json.loads(req_body)
                    path, filename, taskid = req_body['path'], req_body['filename'], req_body['taskid']

                    # 定位文件并获取大小
                    fpath = "{0}{1}{2}".format(self.bind_path_prefix[:-1], path, filename)
                    fsize = os.path.getsize(fpath)

                    # 形成若干个req uploadChunk请求
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

                    # 形成req的upChunk请求之后，将task_que中的cnt和total修改
                    self.task_queue_update_cnt(taskid, 0, chunk_id)
                    # self.test_req()
                    # self.test_task()

                else:
                    if error == 0:
                        # 将res_que中的对应删除, 并解析得到taskid
                        req = self.res_queue.pop(que_id)
                        # print('req!', req)
                        req_body = req.split('\n')[1].split('\0')[0]
                        req_body = json.loads(req_body)
                        # print('req/', req_body)

                        # 将含有task_id的任务对应的task的cnt进行修改
                        if head == 'downloadFileRes':
                            task_id = req_body['taskid']
                            # 对应task_que中的cnt+1
                            Done = self.task_queue_add_cnt(task_id)

                            # 解析body，将数据按照off写入本地，如果写完则需要更新mtime
                            # f = open(req_body)
                            file_path = "{0}{1}{2}".format(self.bind_path_prefix[:-1], req_body['path'], req_body['filename'])
                            f = open(file_path, 'wb+')
                            res = sock2file(self.sock, f, req_body['offset'], req_body['chunksize'])
                            if res == 0:
                                print('[error = 0] current socket is not readable')
                                if not Done:
                                    task = self.task_queue[task_id]
                                else:
                                    task = self.complete_queue[task_id]
                                self.gen_task_download_upload(task['task_type'], task['f_path'],
                                                              task['f_name'], task['f_size'], task['f_mtime'], task['md5'])
                            elif res == -1:
                                print('[error = -1] read from socket error!')

                            if Done:
                                # print('修改时间！！！！')
                                mtime = self.complete_queue[task_id]['f_mtime']
                                os.utime(file_path, (mtime, mtime))
                            # con = self.recv_pack().decode(encoding='gbk')
                            # self.manage_api_download(Done, task_id, req_body['path'], req_body['filename'],
                            #                          req_body['offset'], req_body['chunksize'], con)

                        elif head == 'uploadFileRes' or head == 'uploadChunkRes':
                            task_id = req_body['taskid']
                            # 对应task_que中的cnt+1
                            self.task_queue_add_cnt(task_id)

                    elif error == 1:
                        # 将res_que中对应的que_id的请求再次放入req_que中
                        self.req_queue.put(self.res_queue.pop(que_id))

    def sync(self):
        if self.is_die == 0:
            # print('aa')
            if self.update_flag == 0:
                self.update_flag = 1
            self.sync_timer()

    def sync_timer(self):
        t = threading.Timer(30, self.sync)
        if self.is_die == 0:
            t.start()
        else:
            t.cancel()

    def run(self):
        """
        函数名称：run
        函数参数：self
        函数功能：描述主函数运行的主要流程
        """

        # 处理登陆和注册事件，直到登陆成功才结束函数
        self.login_signup()

        # 将bind的持久化内容读取本地
        self.read_bind_persistence()

        # 处理绑定目录
        is_cancel = False
        if self.is_bind:
            is_cancel = is_cancel_bind()
            if is_cancel:
                self.handle_cancel_bind()
        if (not self.is_bind) or is_cancel:  # 目前未绑定或者绑定之后选择取消绑定
            self.handle_bind()

        # 处理queue的持久化文件，如果上次未完成会再次创建队列
        self.read_queue_persistence()
        # 处理db的持久化文件，进行比对并同步
        self.read_db_persistence()

        # 设置连接为非阻塞
        self.sock.setblocking(False)

        # 创建线程并运行
        t_sender = threading.Thread(target=self.sender)
        t_receiver = threading.Thread(target=self.receiver)

        # t_sync_timer = threading.Thread(target=self.sync_timer)
        t_sender.start()
        t_receiver.start()
        # t_sync_timer.run()

        self.sync_timer()

        # 主线程处理键盘IO
        while True:
            if self.update_flag == 1:
                self.update_flag = 0
                print('\n定期同步ing...')
                self.not_first_sync()
                continue
            io_str = input('请输入指令...')
            if io_str == "log_out":
                self.is_die = 1
                t_sender.join()
                t_receiver.join()

                # 按照读持久化文件的反顺序进行持久化
                self.write_db_persistence()
                self.write_queue_persistence()
                self.write_bind_persistence()

                # 退出登陆，发送退出登陆的api
                self.handle_logout()
                self.sock.close()
                exit(1)

            elif io_str == "exit":
                self.is_die = 1
                t_sender.join()
                t_receiver.join()

                # 按照读持久化文件的反顺序进行持久化
                self.write_db_persistence()
                self.write_queue_persistence()
                self.write_bind_persistence()

                # 退出登陆，发送退出登陆的api
                self.handle_logout()
                self.sock.close()
                exit(1)

            elif io_str == "op3":
                print('op3')

            else:
                print('请检查输入的指令..')


if __name__ == '__main__':
    client = Client()
    client.run()
