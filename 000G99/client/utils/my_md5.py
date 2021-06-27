import hashlib


def get_file_md5(path, f_name):
    """
    输入参数：(1)path 文件路径
            (2)f_name 文件名
    输出参数：文件的md5
    """
    buf_size = 4096
    m = hashlib.md5()
    with open(f_name, 'rb') as fobj:
        while True:
            data = fobj.read(buf_size)
            if not data:
                break
            m.update(data)
    return m.hexdigest()