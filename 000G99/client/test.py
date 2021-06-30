import os
import time
import client
import threading


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

gen_req_deleteFile('/root/path/file')

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



def recv_pack(mres):
    res = memoryview(mres)
    off = 0
    ch = 'd'
    while ch != b'\0':
        res[off] = ch
        off += 1
        if off == 1000:
            ch = b'\0'
        else:
            ch = 'x'
    res[off] += b'\0'
    return off


def write2sock(sock, data, n):
    left = n
    ptr = 0
    while left > 0:
        nwritten = sock.write(data[ptr: ptr+left])
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


if __name__ == '__main__':

    # send con
    f = open('jjj', 'w')
    data = 'hi world'
    n = write2sock(f, data, len(data))
    print(n, len(data))
    f.close()
    f = open('jjj', 'r')
    print(f.read(n))

    # f_name = './test2.py'
    # now_read = 0
    # off = 0
    # f = open(f_name, 'r', encoding='gbk')
    # size = os.path.getsize(f_name)
    #
    # f_2 = open('abc', 'w')
    #
    # while now_read < size:
    #     f.seek(off + now_read)
    #     data = f.read(size - now_read)
    #     n = f_2.write("{}".format(data))
    #     if n == 0:
    #         break
    #     now_read += n
    # print(now_read)
    #
    # client = client.Client()
    # # 处理登陆和注册事件，直到登陆成功才结束函数
    # client.login_signup()
    #
    # # 将bind的持久化内容读取本地
    # client.read_bind_persistence()
    #
    # # 处理绑定目录
    # is_cancel = False
    # if client.is_bind:
    #     is_cancel = client.is_cancel_bind()
    #     if is_cancel:
    #         client.handle_cancel_bind()
    # if (not client.is_bind) or is_cancel:  # 目前未绑定或者绑定之后选择取消绑定
    #     client.handle_bind()
    #
    # # 处理queue的持久化文件，如果上次未完成会再次创建队列
    # client.read_queue_persistence()
    # # 处理db的持久化文件，进行比对并同步
    # client.read_db_persistence()
    #
    # # 设置连接为非阻塞
    # client.sock.setblocking(False)
    #
    # # 创建线程并运行
    # t_sender = threading.Thread(target=client.sender)
    # t_receiver = threading.Thread(target=client.receiver)
    # t_sync_timer = threading.Thread(target=client.sync_timer())
    # t_sender.run()
    # t_receiver.run()
    # t_sync_timer.run()

    while True:
        time.sleep(1)

