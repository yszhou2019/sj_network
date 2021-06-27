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
    """ 下载请求的单元 """
    
    def __init__(self, filename, t_='download'):
        self.type = t_
        self.filename = filename
    
    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['filename'], d_['type'])


class Down_Req:
    """
    发送文件请求
    """
    list_= []


class Down_Res:
    def __init__(self, size):
        self.size = size

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['size'])


class Up_Req_Item:
    """ 上传请求的单元 """

    def __init__(self, filename, size, t_='upload'):
        self.type = t_
        self.filename = filename
        self.size = size

    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['type'], d_['filename'], d_['size'])
    

def log_in():
    """ 登陆同步盘 """
    print('log in')


def sign_up():
    """ 注册账号 """
    print('sign up')



'''
函数功能：上传某个文件
输入参数：文件名（可以是写死成变量，也可以是读取命令行，看你）
具体逻辑： 1. 根据文件名生成json格式的header，然后写入socket
          2. 写入文件的数据 

client发送header:          
{
    "type": "upload",
    "filename": "txt",
    "size": 100 // 字节数量，用以server读取指定长度的字节
}
'''
# def upload():
# 用之前创建好的socket

# 首先发送json格式的请求体

# 然后发送文件的数据

'''

// client的请求
{
    // 指定下载服务器上的某个文件(刚刚上传的一样)
    "type": "download",
    "filename": "txt"
}
// server的响应
{
    "size": 100,
}
'''


# def download():


# def demo_client():
# step 1 : socket连接指定的server的ip和port
# connect(ip,port)
#
# # step 2: 解析命令行参数
# if arg[0]=="upload"
#     upload()
# elif arg[0] == "download"
#     download()
#
# # step 2: 或者说直接在程序里面写死
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
    从配置文件中读取连接服务器的ip和port
    """
    f = open(file_name, 'r')
    text = f.read()
    f.close()
    j_ = json.loads(text)
    return j_['host'], j_['port']


def create_socket(host, port):
    """
    函数功能：根据host和port创建sock
    输入参数：str host, int port
    具体逻辑：1. 创建socket，
         2. 连接host和port，判断是否连接成功
         3. 设置非阻塞，端口复用
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

    # 1. 判断文件是否存在
    if not os.path.exists(op_f_name):
        print('file:%s not exit.' % op_f_name)
        exit(1)

    # 2. 根据文件大小进行分块，并生成upload_header
    f_size = os.path.getsize(op_f_name)
    up_header_ = Up_Req_Item(op_f_name, f_size)
    print(up_header_.cls2json())

    # 3. 读取host和port，建立连接
    host_, port_ = read_config('./config.cfg')
    s = create_socket(host_, port_)

    # 4. 发送header到echo服务器，检查
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
    函数功能：下载某个文件
    输入参数：文件名（可以是写死成变量，也可以是读取命令行，看你）
    具体逻辑：1. client生成json格式的header
         2. client读取server的res，获取文件size
         3. 读取指定长度的字节，写入文件中
    """
    # 1. 构造download_header
    op_f_name = file_name
    down_header = Down_Req_Item(op_f_name)
    print(down_header.cls2json())

    # 2. 读取host和port，建立连接
    host_, port_ = read_config('./config.cfg')
    s = create_socket(host_, port_)

    # 3. 发送down_header并接收down_response
    s.send(down_header.cls2json().encode(encoding='gbk'))
    str_json = s.recv(1024)
    down_res = Down_Res.json2cls(str_json)
    size = down_res.size

    # 4. 从socket接收size大小
    f_content = s.recv(size)
    s.close()

    # 5. 在本地生成文件
    f = open(op_f_name, 'wb')
    f.write(f_content)
    print('down')


if __name__ == '__main__':
    while True:
        op_str = input('请输入操作：')
        op_li = op_str.split()
        op = op_li[0]

        if op == 'exit':
            break
        elif op == 'upload' and len(op_li) == 2:
            op_upload(op_li[1])
        elif op == 'download' and len(op_li) == 2:
            op_download(op_li[1])
        else:
            print('请检查输入的命令以及参数个数...')

    # # 读取host和port
    # host, port = read_config('./config.cfg')
    #
    # # 创建socket
    # sock = create_socket(host, port+1)
    # # 设置为非阻塞
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
