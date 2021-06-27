from api.upload_header import *
from utils.my_md5 import *
from utils.settings import *
import socket
import queue
import os

class User:
    def __init__(self, session):
        self.session = session


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
        print("try to connect to %s:%s" % (host, port))
        sock.connect((host, port))
    except socket.error:
        return -1
    return sock



def upload(session, filename, path, size, mtime):
    # settings = settings()
    f_md5 = get_file_md5(path, filename)
    upload = Upload(session, filename, path, f_md5, size, mtime)

    host_, port_ = read_config('../config.cfg')
    sock = create_socket(host_, port_)
    if sock == -1:
        print('can not connect to %s:%s' % (host_, port_))
    sock.send(upload.cls2byte())

    upload_recv = sock.recv(1024).decode(encoding='gbk')
    sp_res = upload_recv.split('\n')
    if sp_res[0] == 'uploadRes':
        upload_res_body = sp_res[1].split('\0')[0]
        upload_res = json.loads(upload_res_body)
        if upload_res['error'] == 0:
            return
        else: # 文件不存在需要分块上传
            vfile_id = upload_res['vfile_id']
            f_size = os.path.getsize(path+'/'+filename)
            off = 0
            while f_size > 0:
                chunksize = f_size % 4096


class Task:
    def __init__(self, cls, atr):
        self.cls = cls
        self.atr = atr

    def get_dict(self):
        return json.loads(self.atr)

t_queue = queue.Queue()
def upload():
    dict = {
        "session":32,
        "filename":"txt"
    }
    t = Task(dict)


upload()


def log_out():
    this_is_die = 1
    if this_t1_join==1 and this_t1_join==1 and this_t1_join==1:
        persistence_db()
        persistence_queue()
