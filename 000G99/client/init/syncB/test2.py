# coding=gbk
import os
import time
import client
import sqlite3
import json
import threading
import hashlib


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


def search():
    s_path = input('input path:')
    for path, dirs, files in os.walk(s_path):
        path_2 = path[len(s_path):] + '/'
        print("path:{0}, dirs:{1}, files:{2}".format(path_2, dirs, files))

def create_file(prefix, path, file):
    path_list = path.split('/')
    print(path_list)


def gen_down():
    off = 0
    chunk_id = 0
    s = 4000000000
    while s > 0:
        if s > 4 * 1024 * 1024:
            chunk_size = 4 * 1024 * 1024
        else:
            chunk_size = s
        que_id = 7
        off += chunk_size
        chunk_id += 1
        s -= chunk_size
        print(chunk_size, off, chunk_id)

    total = chunk_id + 1
    print(total)

def gen_req_deleteFile(path_file):
    file = path_file.split('/')[-1]
    path = path_file[0:-len(file)]
    print(file, path)

# gen_req_deleteFile('/root/path/file')

# print(time.ctime(os.path.getmtime('./search_path.py')))
# gen_down()

"""    client = client.Client()
    client.per_db_name = './client.db'
    client.db_init()
    res = client.db_select()
    print(res)
    client.db_update('jjsxj0', 9099, 999, 'iii')
    res = client.db_select()
    print(res)"""

def runA():
    while True:
        time.sleep(1)
        print("A")

def runB():
    while True:
        time.sleep(1)
        print("B")

def sync():
    print('aa')
    sync_timer()

def sync_timer():
    t = threading.Timer(3, sync)
    t.start()

"""    client.init_path = './'
    client.read_bind_persistence()
    client.read_queue_persistence()
    client.test_req()

    # 设置连接为非阻塞
    client.sock.setblocking(False)

    t_sender = threading.Thread(target=client.sender)
    t_recv = threading.Thread(target=client.receiver)

    t_sender.start()
    t_recv.start()

    t_sender.join()
    t_recv.join()"""


def test_login(name, pwd):
    d = {
        'username': name,
        'password': pwd
    }
    login_req = "login\n{0}\0".format(json.dumps(d))
    return login_req


def test_bind(session, bind_id):
    d = {"session": session, "bindid": int(bind_id)}
    bind_pack = "setbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
    return bind_pack

def test_disbind(user_session):
    d = {"session": user_session}
    bind_pack = "disbind\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
    return bind_pack

def test_signup(account, password):
    d = {
        'username': account,
        'password': password
    }
    tmpStr = "signup\n{0}\0".format(json.dumps(d))
    return tmpStr

def gen_req_uploadFile(self, path, filename, size, md5, task_id, mtime):
    que_id = self.get_que_id()
    d = {"session": self.user_session, "taskid": int(task_id), "queueid": que_id,
         "filename": filename, "path": path, "md5": md5, "size": size, "mtime": mtime}

    req_uploadFile = "uploadFile\n{0}\0".format(json.dumps(d))
    print(req_uploadFile)
    return req_uploadFile

def gen_req_uploadChunk(self, v_id, t_id, q_id, c_id, off, size, path, filename):
    d = {'session': self.user_session, 'vfile_id': v_id, 'taskid': t_id, 'queueid': q_id, 'chunkid': c_id,
         'offset': off, 'chunksize': size, 'path': path, 'filename': filename}
    req_up = "uploadChunk\n{0}\0".format(json.dumps(d))
    print(req_up)
    return req_up

def gen_req_uploadChunk(self, v_id, t_id, q_id, c_id, off, size, path, filename):
    d = {'session': self.user_session, 'vfile_id': v_id, 'taskid': t_id, 'queueid': q_id, 'chunkid': c_id,
         'offset': off, 'chunksize': size, 'path': path, 'filename': filename}
    req_up = "uploadChunk\n{0}\0".format(json.dumps(d))
    print(req_up)
    return req_up

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
        print(req)
        self.sock.send(req.encode())
        print(self.recv_pack())
        # self.req_queue.put(req)
        off += chunk_size
        chunk_id += 1
        s -= chunk_size
    return chunk_id


def test(client):
    # login
    # suc

    client.sock.send(test_login('user_9', 'userU90512').encode())
    print(client.recv_pack())
    # user not f
    client.sock.send(test_login('user_10', 'userU90512').encode())
    print(client.recv_pack())
    # pwd err
    client.sock.send(test_login('user_9', '123').encode())
    print(client.recv_pack())

    # signup
    # user exist
    client.sock.send(test_signup('user_9', '123').encode())
    print(client.recv_pack())
    # suc
    client.sock.send(test_signup('user_101', '123').encode())
    print(client.recv_pack())


    # setbind
    # no session
    """    client.sock.send(test_bind('123', '3'))
    print(client.recv_pack())
    # suc
    client.sock.send(test_bind('i2D6I9WO1N6jvoo9hy32lSQNi0e895Rg', '3'))
    print(client.recv_pack())
    client.sock.send(test_bind('i2D6I9WO1N6jvoo9hy32lSQNi0e895Rg', '2'))
    print(client.recv_pack())"""

    # disbind
    """client.sock.send(test_bind('i2D6I9WO1N6jvoo9hy32lSQNi0e895Rg', '3'))
    print(client.recv_pack())
    client.sock.send(test_disbind('999'))
    print(client.recv_pack())
    client.sock.send(test_disbind('222'))
    print(client.recv_pack())"""

    # getdir
    # d = {"session": 'i2D6I9WO1N6jvoo9hy32lSQNi0e895Rg'}
    # getDir = "getdir\n{0}\0".format(json.dumps(d)).encode(encoding='gbk')
    # client.sock.send(getDir)
    # print(getDir)
    # get_dir_res = client.recv_pack()
    # print('test')
    # print(get_dir_res)
    # client.handle_getDir()

    # logout

    # unit test for upload and download
    # uploadFile
    # no session; no bind; md5 exit; md5 not exist
    # self, path, filename, size, md5, task_id, mtime

    # uploadFile Chunk
    # session have bind
    # client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    # client.sock.send(gen_req_uploadChunk(client, ).encode())
    # print(client.recv_pack())

    # no session
    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_sesd file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(ooooclient, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())# md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())# md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3ooR7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())# md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())# md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())# md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())# md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""    """client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jjj', 1, 1999).encode())
    print(client.recv_pack())

    # md5 not exist need to upload file
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'jj99j', 1, 1999).encode())
    print(client.recv_pack())


    # md5 exist
    client.sock.send(gen_req_uploadFile(client, '/root/', 'abc123', 800, 'kk', 1, 1999).encode())
    print(client.recv_pack())"""

    #upload chunk
    # no session
    # gen_req_uploadChunk(self, v_id, t_id, q_id, c_id, off, size, path, filename):
    """client.sock.send(gen_req_uploadChunk(client, 19, 3, 3, 0, 0, 300, '/root/', 'abc123').encode())
    print(client.recv_pack())

    # have session but not bind
    client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    client.sock.send(gen_req_uploadChunk(client, 19, 3, 3, 0, 0, 300, '/root/', 'abc123').encode())
    print(client.recv_pack())

    # suc
    client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    print((gen_req_uploadChunk(client, 20, 3, 3, 0, 0, 800, '/root/', 'abc123') + 'abcabcacbcc\0').encode())
    client.sock.send((gen_req_uploadChunk(client, 20, 3, 3, 0, 0, 800, '/root/', 'abc123') + 'abcabcacbcc\0').encode())
    print(client.recv_pack())"""

    # downloadFile : self, f_path, f_name, f_size, md5, task_id
    # not bind
    # client.user_session = 'B29j533NLLJ58s6u9NO5EFg35BQg5ZWA'
    # gen_req_download(client, '/', 'txt', 500, 'kk', 9)
    # client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    # gen_req_download(client, '/', 'txt', 500, 'kk', 9)
    # downloadFile

def add_path_bound(path):
    if path[0] != '/':
        path = '/' + path
    if path[-1] != '/':
        path += '/'
    return path


def test_upload(client, f_path, f_name, real_path, m_d5):

    # 模拟登陆
    client.user_session = 'hYf5KlMB6sr02Ev4YF94tbOB8G8IUngw'
    client.user_name = 'zxh'

    # 模拟绑定
    client.is_bind = 1
    client.bind_path_prefix = os.path.abspath('.')

    # 假设同步完成
    # 设置连接为非阻塞
    client.sock.setblocking(False)

    # 创建upload和download的task
    # task_type, f_path, f_name, f_size, f_mtime, md5
    # f_path = '/'
    # f_name = 'test.py'

    # 假设绑定的是当前目录
    client.bind_path_prefix = add_path_bound(os.path.abspath('.'))
    #
    # real_path = os.path.abspath('./test.py')
    size = os.path.getsize(real_path)
    mtime = os.path.getmtime(real_path)
    mtime = int(mtime)
    md5 = m_d5
    print('...gen_task_download_upload...')
    client.gen_task_download_upload('upload', f_path, f_name, size, mtime, md5)
    # 生成task任务
    # 查看task和req队列
    print('\n下面生成task以及req队列\n')
    client.test_task()
    client.test_req()

    # ff62bf2e9270b02f475ca0330e1188b2
    # 16033
    # client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    # client.user_name = 'zxh'
    # print(client.bind_path_prefix)
    # f_path = '/txtt/'
    # f_name = 'test3.py'
    # client.gen_task_download_upload('download', f_path, f_name, size, mtime, '20210701')

    # sender和recver再运行的时候，进行输出
    print('...run server & recver...')
    t_sender = threading.Thread(target=client.sender)
    t_receiver = threading.Thread(target=client.receiver)
    t_sender.start()
    t_receiver.start()

    time.sleep(3)
    print('\n任务完成，最终task队列为：')
    client.test_task()
    print('\ncom_queue为：')
    print('[com]', client.complete_queue)


def test_download(client,f_path, f_name, real_path, _md5):
    # 模拟登陆
    client.user_session = 'hYf5KlMB6sr02Ev4YF94tbOB8G8IUngw'
    client.user_name = 'zxh'

    # 模拟绑定
    client.is_bind = 1
    client.bind_path_prefix = os.path.abspath('.')

    # 假设同步完成
    # 设置连接为非阻塞
    client.sock.setblocking(False)

    # 假设绑定的是当前目录
    client.bind_path_prefix = add_path_bound(os.path.abspath('.'))

    # real_path = os.path.abspath('./test.py')
    size = os.path.getsize(real_path)
    mtime = os.path.getmtime(real_path)
    mtime = int(mtime)
    md5 = _md5

    client.gen_task_download_upload('download', f_path, f_name, size, mtime, md5)

    # sender和recver再运行的时候，进行输出
    print('...run server & recver...')
    t_sender = threading.Thread(target=client.sender)
    t_receiver = threading.Thread(target=client.receiver)
    t_sender.start()
    t_receiver.start()

    time.sleep(3)
    print('\n任务完成，最终task队列为：')
    client.test_task()
    print('\ncom_queue为：')
    print('[com]', client.complete_queue)

if __name__ == '__main__':
    print(time.ctime(1624706304))
    client = client.Client()
    client.handle_getDir()
    # client.login_signup()
    # md5不存在，应该上传并成功
    # test_upload(client, '/test/', 'upload.mp4', os.path.abspath('./test/upload.mp4'), '2100708890909009900')
    # md5存在，秒传成功
    # test_upload(client, '202107010')

    # md5 存在，开始下载
    test_download(client, '/test/', 'download2.mp4', os.path.abspath('./test/upload.mp4'), '2100708890909009900')
    # client.login_signup()
    # # 模拟登陆
    # client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    # client.user_name = 'zxh'
    #
    # # 模拟绑定
    # client.is_bind = 1
    # client.bind_path_prefix = os.path.abspath('.')
    #
    # # 假设同步完成
    # # 设置连接为非阻塞
    # client.sock.setblocking(False)
    #
    # # 创建upload和download的task
    # # task_type, f_path, f_name, f_size, f_mtime, md5
    # f_path = '/'
    # f_name = 'test2.py'
    #
    # # 假设绑定的是当前目录
    # client.bind_path_prefix = add_path_bound(os.path.abspath('.'))
    #
    # real_path = os.path.abspath('./test2.py')
    # size = os.path.getsize(real_path)
    # mtime = os.path.getmtime(real_path)
    # mtime = int(mtime)
    # md5 = '20210701'
    # print('...gen_task_download_upload...')
    # client.gen_task_download_upload('upload', f_path, f_name, size, mtime, md5)
    # # 生成task任务
    # # 查看task和req队列
    # client.test_task()
    # client.test_req()
    #
    # # ff62bf2e9270b02f475ca0330e1188b2
    # # 16033
    # client.user_session = '3FslNwoD4oIg3R7Qj5ZscJS0rH0zuTPU'
    # client.user_name = 'zxh'
    # print(client.bind_path_prefix)
    # f_path = '/txtt/'
    # f_name = 'test3.py'
    # client.gen_task_download_upload('download', f_path, f_name, size, mtime, '20210701')
    #
    # # sender和recver再运行的时候，进行输出
    # print('...run server & recver...')
    # t_sender = threading.Thread(target=client.sender)
    # t_receiver = threading.Thread(target=client.receiver)
    # t_sender.start()
    # t_receiver.start()

    #
    # time.sleep(1)
    # client.test_task()
    # print('c', client.complete_queue)



    while True:
        time.sleep(1)

