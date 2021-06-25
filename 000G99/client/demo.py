# coding=gbk
import socket
import threading
import sys
import os
import json

'''
    res = Response('upload', 'test.txt', '100')
    print(res)
    print(res.cls2json())

    res2 = Response.json2cls(res.cls2json())
    print(res2.cls2json())
'''


class Down_Req_Item:
    """ ��������ĵ�Ԫ """
    
    def __init__(self, filename, t_='download'):
        self.type = t_
        self.filename = filename
    
    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    # ��ѡ���캯��
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['filename'], d_['type'])


class Down_Req:
    """
    �����ļ�����
    """
    list_= []


class Down_Res:
    def __init__(self, size):
        self.size = size

    # ��ѡ���캯��
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['size'])


class Up_Req_Item:
    """ �ϴ�����ĵ�Ԫ """

    def __init__(self, filename, size, t_='upload'):
        self.type = t_
        self.filename = filename
        self.size = size

    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    # ��ѡ���캯��
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['type'], d_['filename'], d_['size'])
    

def log_in():
    """ ��½ͬ���� """
    print('log in')


def sign_up():
    """ ע���˺� """
    print('sign up')



'''
�������ܣ��ϴ�ĳ���ļ�
����������ļ�����������д���ɱ�����Ҳ�����Ƕ�ȡ�����У����㣩
�����߼��� 1. �����ļ�������json��ʽ��header��Ȼ��д��socket
          2. д���ļ������� 

client����header:          
{
    "type": "upload",
    "filename": "txt",
    "size": 100 // �ֽ�����������server��ȡָ�����ȵ��ֽ�
}
'''
# def upload():
# ��֮ǰ�����õ�socket

# ���ȷ���json��ʽ��������

# Ȼ�����ļ�������

'''

// client������
{
    // ָ�����ط������ϵ�ĳ���ļ�(�ո��ϴ���һ��)
    "type": "download",
    "filename": "txt"
}
// server����Ӧ
{
    "size": 100,
}
'''


# def download():


# def demo_client():
# step 1 : socket����ָ����server��ip��port
# connect(ip,port)
#
# # step 2: ���������в���
# if arg[0]=="upload"
#     upload()
# elif arg[0] == "download"
#     download()
#
# # step 2: ����˵ֱ���ڳ�������д��
# upload("txt")

# download("txt")

# Wait for incoming data from server
# .decode is used to turn the message in bytes to a string
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
        sock.setblocking(False)
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

    # 3. ��ȡhost��port����������
    host_, port_ = read_config('./config.cfg')
    s = create_socket(host_, port_)

    # 4. ����header��echo�����������
    s.send(up_header_.cls2json().encode(encoding='gbk'))
    res = s.recv(1024)
    print('response:%' % res)

    f = open(op_f_name, 'rb')
    f_content = f.read(f_size)
    print('file content:%s' % f_content)
    s.send(up_header_.cls2json().encode(encoding='gbk'))
    res = s.recv(1024)
    print('response:%' % res)
    s.send(f_content)
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
    down_res = Down_Res.json2cls(str_json)
    size = down_res.size

    # 4. ��socket����size��С
    f_content = s.recv(size)
    s.close()

    # 5. �ڱ��������ļ�
    f = open(op_f_name, 'wb')
    f.write(f_content)
    print('down')


if __name__ == '__main__':
    while True:
        op_str = input('�����������')
        op_li = op_str.split()
        op = op_li[0]

        if op == 'exit':
            break
        elif op == 'upload' and len(op_li) == 2:
            op_upload(op_li[1])
        elif op == 'download' and len(op_li) == 2:
            op_download(op_li[1])
        else:
            print('��������������Լ���������...')

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
