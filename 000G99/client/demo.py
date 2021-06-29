# coding=gbk
import socket
import threading
import sys
import sqlite3
import os
import json
from api.download_header import *
from api.upload_header import *
from utils import *


def log_in():
    """ ��½ͬ���� """
    print('log in')


def sign_up():
    """ ע���˺� """
    print('sign up')


def receive(socket, signal):
    while signal:
        try:
            data = socket.recv(32)
            print(str(data.decode("utf-8")))
        except:
            print("You have been disconnected from the server")
            signal = False
            break


def read_config(file_name):
    """
    �������ļ��ж�ȡ���ӷ�������ip��port
    """
    f = open(file_name, 'r')
    text = f.read()
    f.close()
    j_ = json.loads(text)
    return j_['host'], j_['port']


def create_socket(host, port):
    """
    �������ܣ�����host��port����sock
    ���������str host, int port
    �����߼���1. ����socket��
         2. ����host��port���ж��Ƿ����ӳɹ�
         3. ���÷��������˿ڸ���
    """
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print("try to connect to %s:%s" % (host, port))
        sock.connect((host, port))
    except socket.error:
        print("Could not make a connection to the server")
        sys.exit(0)
    return sock


def op_upload(file_name):

    op_f_name = file_name

    # 1. �ж��ļ��Ƿ����
    if not os.path.exists(op_f_name):
        print('file:%s not exit.' % op_f_name)
        exit(1)

    # 2. �����ļ���С���зֿ飬������upload_header
    f_size = os.path.getsize(op_f_name)
    up_header_ = Up_Req_Item(op_f_name, f_size)
    print(up_header_.cls2json())

    # 3. ��ȡ�ļ�����ȡ�ļ���Ϣ����header���ļ�ƴ��
    f = open(op_f_name, 'rb')
    f_content = f.read(f_size)
    f.close()
    content = up_header_.cls2json().encode(encoding='gbk') + f_content

    # 4. ��ȡhost��port����������
    host_, port_ = read_config('./config.cfg')
    s = create_socket(host_, port_)

    print('send content:\n%s' % content)
    s.send(content)
    # res = s.recv(1024)
    # print('return :%s ' % res.decode(encoding='gbk'))
    # s.send(f_content)
    s.close()


def op_download(file_name):
    """
    �������ܣ�����ĳ���ļ�
    ����������ļ�����������д���ɱ�����Ҳ�����Ƕ�ȡ�����У����㣩
    �����߼���1. client����json��ʽ��header
         2. client��ȡserver��res����ȡ�ļ�size
         3. ��ȡָ�����ȵ��ֽڣ�д���ļ���
    """
    # 1. ����download_header
    op_f_name = file_name
    down_header = Down_Req_Item(op_f_name)
    print(down_header.cls2json())

    # 2. ��ȡhost��port����������
    host_, port_ = read_config('./config.cfg')
    s = create_socket(host_, port_)

    # 3. ����down_header������down_response
    s.send(down_header.cls2json().encode(encoding='gbk'))
    str_json = s.recv(1024)
    str_ = str_json.decode(encoding='gbk')
    str_ = str_[0:-1]   # ȥ��β��'\0'

    down_res = Down_Res.json2cls(str_)
    size = down_res.size

    # 4. ��socket����size��С
    f_content = s.recv(size)
    s.close()

    # 5. �ڱ��������ļ�
    f = open(op_f_name+'_recv', 'wb')
    f.write(f_content)

class db:
    def __init__(self):
        self.conn = 0

    def con(self):
        self.conn = sqlite3.connect('ab.db')

    def run(self):
        self.con()
        a = self.conn.cursor()


db = db()
db.run()

if __name__ == '__main__':
    get_dir_res = """getdirRes\n[{"filename":1,"path":1,"size":1,"md5":1,"modtime":1}]\0""".encode(encoding='gbk')

    get_dir_str = get_dir_res.decode(encoding='gbk')
    print('1',get_dir_str)
    get_dir_head = get_dir_str.split('\n')[0]
    print('2',get_dir_head)
    if get_dir_head != 'getdirRes':
        print(get_dir_head)
        print('����getDirʧ�ܣ�')
        exit(1)

    # ��֤�ɹ�����strתΪjson
    get_dir_body = get_dir_str.split('\n')[1]
    get_dir_body = get_dir_body.split('\0')[0]
    get_dir_list = json.loads(get_dir_body)
    for ii in get_dir_list:
        print(ii['filename'])
    #
    # session = ''
    # while True:
    #     op_str = input('�����������')
    #     op_li = op_str.split()
    #     op = op_li[0]
    #
    #     if op == 'exit':
    #         break
    #     elif op == 'upload' and len(op_li) == 2:
    #         op_upload(op_li[1])
    #     elif op == 'download' and len(op_li) == 2:
    #         op_download(op_li[1])
    #     else:
    #         print('��������������Լ���������...')

    # # ��ȡhost��port
    # host, port = read_config('./config.cfg')
    #
    # # ����socket
    # sock = create_socket(host, port+1)
    # # ����Ϊ������
    # # sock.setblocking(False)
    #
    # sock.send(b'Hello')
    # print('send suc')
    #
    # buf = sock.recv(1024)
    # print(buf)
    #
    # up_header = Up_Req_Item('upload', 'txt', 100)
    # print(up_header.cls2json())

    # # Create new thread to wait for data
    # receiveThread = threading.Thread(target=receive, args=(sock, True))
    # receiveThread.start()
    #
    # # Send data to server
    # # str.encode is used to turn the string message into bytes so it can be sent across the network
    # while True:
    #     message = input()
    #     sock.sendall(str.encode(message))
