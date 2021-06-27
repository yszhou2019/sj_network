import json


class Download:
    """ 下载请求头 """
    def __init__(self, session, filename, path,
                 md5, offset, chunksize):
        self.session = session
        self.filename = filename
        self.path = path
        self.md5 = md5
        self.offset = offset
        self.chunksize = chunksize

    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    def cls2byte(self):
        str_ = 'download\n'+json.dumps(self.__dict__)+'\0'
        return str_.encode(encoding='gbk')


class Download_Res1:
    def __init__(self, error, msg):
        self.error = error
        self.msg = msg

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['error'], d_['msg'])


class Download_Res:
    """ 下载请求响应 """
    def __init__(self, error, msg, filename="",
                 path="", offset="", chunksize=""):
        self.error = error
        self.msg = msg
        self.filename = filename
        self.path = path
        self.offset = offset
        self.chunksize = chunksize

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['error'], d_['msg'], d_['filename'], d_['path'], d_['offset'], d_['chunksize'])


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


class Down_Res:
    def __init__(self, size):
        self.size = size

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['size'])