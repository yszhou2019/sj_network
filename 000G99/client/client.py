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

    # need
    def first_sync(self):
        print("aa")

    def not_first_sync(self):
        print("aa")

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
        per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        if not os.path.exists(per_db_name):
            self.create_local_file_table(per_db_name)
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











