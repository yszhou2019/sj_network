import os
import time


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
