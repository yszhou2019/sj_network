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
        self.init_path = './init/'              # �洢��Ҫ����ļ���Ŀ¼
        self.user_id = 0                        # �û�id
        self.user_name = ''                     # �û��˺�
        self.user_pwd = ''                      # �û�����
        self.user_session = ''                  # �û�session
        self.BUF_SIZE = 4096                    # const(fake) var buf_size
        self.is_die = 0                         # �Ƿ���Ҫ��ͣ�߳�
        self.is_bind = 0                        # �û��Ƿ��Ѿ���Ŀ¼
        self.bind_path_prefix = ''              # ��ǰ���ذ�Ŀ¼
        self.sql_conn = 0                       # �������ݿ�
        self.req_queue = queue.Queue()          # request������У����Ѻ�ģ�������
        self.res_queue = queue.Queue()          # response�ȴ���Ӧ���У���request��������get�����ٷ���res����
        self.task_queue = queue.Queue()         # ������У��洢�ϴ������صȵ��û����񣨴�����
        self.complete_queue = queue.Queue()     # ������ɶ��У���ɺ��task����Ž���

    # need :
    def handle_login(self):
        """
        �������ƣ�handle_login
        ����������self
        ��������ֵ��bool����½�ɹ�ΪTrue������ΪFalse����Ҫ�������ԭ��
        �������ܣ����û������˺ź�������е�½
        """
        print('login')
        return True

    def handle_logout(self):
        """
        �������ƣ�handle_login
        ����������self
        ��������ֵ��bool����½�ɹ�ΪTrue������ΪFalse����Ҫ�������ԭ��
        �������ܣ����û������˺ź�������е�½
        """
        print('login')
        return True

    # need :
    def handle_signup(self):
        """
        �������ƣ�run
        ����������self
        �������ܣ��������������е���Ҫ����
        """
        print('signup')
        return True

    def login_signup(self):
        while True:
            start_input = input("������login����signup...")
            if start_input == "login":
                login_res = self.handle_login()
                if not login_res:   # ��½ʧ��
                    print('�����µ�¼...')
                    continue
                else:               # ��½�ɹ�
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
            task_json = json_data['task']
            for req_item in req_json:
                self.req_queue.put(req_item)
            for task_item in task_json:
                self.task_queue.put(task_item)

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
        per_db_name = "{0}/{1}_{2}_db.db".format(self.init_path, str(self.user_id), str(self.bind_path_prefix))
        if not os.path.exists(per_db_name):
            self.create_local_file_table(per_db_name)
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
        check(path)
        bind_id = input('��������ѡĿ¼��1��2��3\n��ѡ��Ҫ�󶨵�Ŀ¼��')
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
        if (not self.is_bind) or is_cancel:     # Ŀǰδ�󶨻��߰�֮��ѡ��ȡ����
            self.handle_bind()

        # ����queue�ĳ־û��ļ�������ϴ�δ��ɻ��ٴδ�������
        self.read_queue_persistence()
        # ����db�ĳ־û��ļ������бȶԲ�ͬ��
        self.read_db_persistence()

        # ��������
        conn = 'aaa'

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











