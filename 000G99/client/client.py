# coding=gbk
import socket
import threading
import sys
import os
import json
import queue
import sqlite3
import threading
from api.download_header import *
from api.upload_header import *
from utils import *


def read_config(file_name):
    """
    从配置文件中读取连接服务器的ip和port
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

def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d


class Client:
    def __init__(self):
        self.init_path = './init/'              # 存储需要存放文件的目录
        self.user_id = 0                        # 用户id
        self.user_name = ''                     # 用户账号
        self.user_pwd = ''                      # 用户密码
        self.user_session = ''                  # 用户session
        self.BUF_SIZE = 4096                    # const(fake) var buf_size
        self.is_die = 0                         # 是否需要暂停线程
        self.is_bind = 0                        # 用户是否已经绑定目录
        self.bind_path_prefix = ''              # 当前本地绑定目录
        self.sql_conn = 0                       # 连接数据库
        self.req_queue = queue.Queue()          # request请求队列，分裂后的（子任务）
        self.res_queue = queue.Queue()          # response等待响应队列，从request队列里面get出来再放在res队列
        self.task_queue = queue.Queue()         # 任务队列，存储上传、下载等等用户任务（大任务）
        self.complete_queue = queue.Queue()     # 任务完成队列，完成后从task里面放进来
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)     # 连接server
        self.per_db_name = ''

    # need :
    def handle_login(self):
        """
        函数名称：handle_login
        函数参数：self
        函数返回值：bool，登陆成功为True，错误为False，需要输出错误原因
        函数功能：让用户输入账号和密码进行登陆
        """
        print('login')
        return True

    def handle_logout(self):
        """
        函数名称：handle_login
        函数参数：self
        函数返回值：bool，登陆成功为True，错误为False，需要输出错误原因
        函数功能：让用户输入账号和密码进行登陆
        """
        print('login')
        return True

    # need :
    def handle_signup(self):
        """
        函数名称：run
        函数参数：self
        函数功能：描述主函数运行的主要流程
        """
        print('signup')
        return True

    def login_signup(self):
        while True:
            start_input = input("请输入login或者signup...")
            if start_input == "login":
                login_res = self.handle_login()
                if not login_res:   # 登陆失败
                    print('请重新登录...')
                    continue
                else:               # 登陆成功
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
        if self.res_queue.not_empty:
            self.res_queue = queue.Queue()
        if self.task_queue.not_empty:
            self.task_queue = queue.Queue()
        if self.complete_queue.not_empty:
            self.complete_queue = queue.Queue()

    def first_sync(self):
        """
        函数名称：first_sync
        函数参数：self
        函数返回值：无
        函数功能：1. 进行初次同步，主要任务是下载
                2. 操作：首先请求getDir，然后处理请求的结果
        """
        # 1. 扫描本地所有文件，生成所有文件的upload api
        # 构造一个map，key:"path/file_name", value:md5，之后与服务端对比时使用
        map = {}
        for local_file in cur_dir:
            upload_header = "uploadFile\n"+ local_file_info_json + '\0'.encode()
            self.req_queue.put(upload_header)
            map.insert("{0}/{1}".format(local_file['relative_path'],
                                local_file['file_name']): local_file['md5'])


        # 2. 请求服务端的所有数据，遍历服务端的所有数据
        # 2.1 发送getdir请求
        d = {"session": self.user_session}
        getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        self.sock = get_connect()
        self.sock.send(getDir)
        get_dir_res = self.sock.recv(self.BUF_SIZE)

        # 2.2 验证getDir的res
        get_dir_str = get_dir_res.decode(encoding='gbk')
        get_dir_head = get_dir_str.split('\n')[0]
        if get_dir_head != 'getdirRes':
            print('请求getDir失败！')
            exit(1)

        # 2.3 验证成功，将str转为json, 得到服务器目录的所有文件的list
        get_dir_body = get_dir_str.split('\n')[1]
        get_dir_body = get_dir_body.split('\0')[0]
        get_dir_list = json.loads(get_dir_body)
        """
        {
            "filename":,
            "path":,（两个表在服务器进行连接）
            "size":
            "md5"
            "modtime"
        }
        """

        """
        map = {"./txt": "md1", "./txt2": "md2"}
        """
        # 2.4 将local_file与get_dir_list进行对比, 遍历服务器的每一个文件remote_file
        for remote_file in get_dir_list:
            # 在map中找到key为remote_file['filename']的项
            remote_md5 = map.get(remote_file['filename'], -1)
            if remote_md5 == -1 or remote_md5 != remote_file['md5']:
                # 将download请求放到task队列
                # 生成task_id，记录
                download_to_task()
                # 将带有task_id的download请求放到req队列
                download_to_req()
                # 将需要下载的文件信息写入db
                add_to_db()

    def not_first_sync(self):
        # 1. 将本地的所有文件与db进行对比，扫描本地所有文件
        # 通过db获取本地的数据库所有的内容, 并在内存中维护一个记录db信息的map
        self.sql_conn.row_factory = dict_factory
        cur = self.sql_conn.cursor()
        res = cur.execute('select * from local_file_table')
        db_file = res.fetchall()  # 同样变成字典样式
        # 构造一个map，key:"path/file_name", value:(size,mtime,md5)
        map = {"{0}/{1}".format(local_file['relative_path'],
                                local_file['file_name']): (local_file['size'],local_file['mtime'],local_file['md5']) for local_file in db_file}
        """
        map = {"./txt": (5, 'hhhh-yy-dd', "md1"), "./txt2": (6, 'hhhh-yy-dd', "md2")}
        """

        # 1.1 开始扫描本地所有文件
        # 维护一个list或者map记录本地文件的信息（文件路径、文件名）到内存
        list_or_map = {}    # 记录文件名与对应的md5
        for local_file in cur_dir:
            f = map.get(strcat(local_file.path,local_file.fname),-1)
            if f != -1: # 如果存在，对比size以及mtime，不能对比md5，因为尽量减少计算本地md5的花销
                if local_file.size != f[0] or local_file.mtime!= f[1]:
                    # 说明此时同名的文件已经进行了修改，需要重算md5
                    local_file_md5 = calculate(local_file)
                    if local_file_md5 != f[2]:
                        # 说明修改了真正的内容，需要上传
                        upload_header = "uploadFile\n" + local_file_info_json + '\0'.encode()
                        self.req_queue.put(upload_header)
                    # 不管是否修改真正的内容，size和mtime已经不同，所以需要更新
                    self.sql_conn.execute('update [table] set md5 = % , size = % , mtime = % where path_filename = %' % (local_file_md5, path, filename))
            # 如果当前扫描的文件在db不存在重名
            # 计算本地md5
            local_file_md5 = calculate(local_file)
            # 增加上传请求
            upload_header = "uploadFile\n" + local_file_info_json + '\0'.encode()
            self.req_queue.put(upload_header)
            # 插入db的path, name, size, mtime, md5
            self.sql_conn.execute('insert into ...')

        # 1.2 扫描db中的所有记录
        for item in map:
            if item.first not in local_map:
                # 当前db的记录在本地不存在
                # 检查task队列是否存在对应的上传or下载任务
                task_list = list(self.task_queue.queue)
                for task in task_list:
                    if task.f_path && task.f_name:
                        # 获得需要删除req的对应的task_id
                        task_id = task.task_id
                        # 弹出所有的req_queue到一个list
                        req_list = get_req_queue_to_list()
                        # 遍历一遍list，将task_id对应的删除
                        delete_task_id_in_req_list(req_list, task_id)
                        # 将删除后的req_list的内容重新put到queue中
                        for req in req_list:
                            self.req_queue.put(req)
                # 告知server删除对应的记录
                req_delete = 'deleteFile\n' + file_info + '\0'
                self.req_queue.put(req_delete)
                # 删除db中的记录
                self.sql_conn.execute('delete 对应的记录')

        # 1.3 请求服务端的所有数据，遍历服务端的所有数据
        # 发送getdir请求
        d = {"session": self.user_session}
        getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
        self.sock = get_connect()
        self.sock.send(getDir)
        get_dir_res = self.sock.recv(self.BUF_SIZE)

        # 验证getDir的res
        get_dir_str = get_dir_res.decode(encoding='gbk')
        get_dir_head = get_dir_str.split('\n')[0]
        if get_dir_head != 'getdirRes':
            print('请求getDir失败！')
            exit(1)

        # 验证成功，将str转为json, 得到服务器目录的所有文件的list
        get_dir_body = get_dir_str.split('\n')[1]
        get_dir_body = get_dir_body.split('\0')[0]
        get_dir_list = json.loads(get_dir_body)
        """{
            "filename":,
            "path":,（两个表在服务器进行连接）
            "size":
            "md5"
            "modtime"
        }"""

        # 将与get_dir_list进行对比, 遍历服务器的每一个文件remote_file
        for remote_file in get_dir_list:
            # 在map中找到key为remote_file['filename']的项
            remote_md5 = map.get(remote_file['filename'], -1)
            if remote_md5 == -1 or remote_md5 != remote_file['md5']:
                download_to_req()
                download_to_task()
                add_to_db()

    def read_bind_persistence(self):
        """
        函数名称：read_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 判断bind持久化文件是否存在，
                    持久化文件不存在：is_bind = 0
                3. 存在，并根据user_id将持久化bind文件读取，获得is_bind以及bind_path信息
        """
        # 2.1 绑定信息
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
        函数名称：write_bind_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 判断bind持久化文件是否存在，
                    持久化文件不存在：is_bind = 0
                3. 存在，并根据user_id将持久化bind文件读取，获得is_bind以及bind_path信息
        """
        # 2.1 绑定信息
        per_bind_name = "{0}/{1}_bind".format(self.init_path, str(self.user_id))
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
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_id以及bind信息确定queue文件是否存在
                2. 不存在，说明第一次绑定该本地目录，queue不做处理，为空
                3. 存在，并根据user_id将持久化queue读入内存，初始化queue
        """
        # 1. 清空全局的queue
        self.init_queue()

        # 2. 判断持久化文件是否存在（持久化文件：1.绑定信息，2.queue信息，3.db信息）
        # 2.2 queue信息
        per_queue_name = "{0}/{1}_{2}_queue".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        f = open(per_queue_name, 'r')
        data = f.read(self.BUF_SIZE)
        f.close()
        if data:
            json_data = json.loads(data)
            # queue的形式 "op_type\n{content}\0"
            # important!
            req_json = json_data['req']
            task_json = json_data['task']
            for req_item in req_json:
                self.req_queue.put(req_item)
            for task_item in task_json:
                self.task_queue.put(task_item)

    def write_queue_persistence(self):
        """
        函数名称：write_queue_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_id以及bind信息确定queue文件是否存在
                2. 不存在，说明第一次绑定该本地目录，queue不做处理，为空
                3. 存在，并根据user_id将持久化queue读入内存，初始化queue
        """
        # 2.2 queue信息
        print('<开始>持久化req以及task queue信息到文件')
        while not self.res_queue.empty():
            item = self.res_queue.get()
            self.req_queue.put(item)

        req_list = list(self.req_queue.queue)
        task_list = list(self.task_queue.queue)
        d = {'req': req_list, 'task': task_list}
        d_str = json.dumps(d)

        per_queue_name = "{0}/{1}_{2}_queue".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        f = open(per_queue_name, 'r')
        f.write(d_str)
        f.close()
        print('持久化req以及task queue信息到文件%s<结束>！' % per_queue_name)

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
        self.sql_conn = sqlite3.connect(per_db_name)  # 不存在，直接创建
        cur = self.sql_conn.cursor()
        cur.execute(sql)
        # self.sql_conn.close()

    def read_db_persistence(self):
        """
        函数名称：read_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_id以及bind信息确定db文件是否存在
                2. 不存在，说明第一次绑定该本地目录，调用first_sync
                3. 存在，调用not_first_sync
        """
        # 2. 判断持久化文件是否存在（持久化文件：1.绑定信息，2.queue信息，3.db信息）
        # 2.2 db信息
        self.per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        if not os.path.exists(self.per_db_name):
            self.create_local_file_table(self.per_db_name)
            self.first_sync()
        else:
            # 可能需要fetchall
            self.not_first_sync()

    # 有可能这个操作不需要
    def write_db_persistence(self):
        """
        函数名称：write_db_persistence
        函数参数：self
        函数返回值：无
        函数功能：1. 此时已经登陆，并且绑定完毕，根据user_id以及bind信息确定db文件是否存在
                2. 不存在，说明第一次绑定该本地目录，调用first_sync
                3. 存在，调用not_first_sync
        """
        # 2. 判断持久化文件是否存在（持久化文件：1.绑定信息，2.queue信息，3.db信息）
        # 2.2 db信息
        per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))

    def is_cancel_bind(self):
        cancel_str = input("请确定是否需要解除绑定？y/Y解除绑定，n/N不接触绑定")
        if cancel_str == 'y' or cancel_str == 'Y':
            cancel_str_2 = input("输入y/Y再次确认")
            if cancel_str_2 == 'y' or cancel_str_2 == 'Y':
                return True
        return False

    # need ：
    def handle_bind(self):
        """
        函数名称：handle_bind
        函数参数：self
        函数功能：处理绑定同步目录，主要步骤包括：
                1. 确定本地绑定的目录，修改全局变量
                2. 确定服务器需要绑定的目录
        """
        path = input('确定本地绑定的目录')
        check(path)
        bind_id = input('服务器可选目录：1，2，3\n请选择要绑定的目录：')
        check(bind_id)
        send(bind_header)
        recv(bind_res)

    def sender(self):
        while True:
            print('im sender')

    def receiver(self):
        while True:
            print('im recver')

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
            is_cancel = self.is_cancel_bind()
            if is_cancel:
                self.handle_cancel_bind()
        if (not self.is_bind) or is_cancel:     # 目前未绑定或者绑定之后选择取消绑定
            self.handle_bind()

        # 处理queue的持久化文件，如果上次未完成会再次创建队列
        self.read_queue_persistence()
        # 处理db的持久化文件，进行比对并同步
        self.read_db_persistence()

        # 建立连接
        conn = 'aaa'

        # 创建线程并运行
        t_sender = threading.Thread(target=self.sender())
        t_receiver = threading.Thread(target=self.receiver())
        t_sync_timer = threading.Thread(target=self.sync_timer())
        t_sender.run()
        t_receiver.run()
        t_sync_timer.run()

        # 主线程处理键盘IO
        while True:
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
                exit(1)

            elif io_str == "op2":
                print('op2')

            elif io_str == "op3":
                print('op3')

            else:
                print('请检查输入的指令..')


if __name__ == '__main__':
    client = Client()
    client.run()











