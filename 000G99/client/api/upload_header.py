import json


class Upload:
    """ 上传请求头 """
    def __init__(self, session, filename, path,
                 md5, size, mtime):
        self.session = session
        self.filename = filename
        self.path = path
        self.md5 = md5
        self.size = size
        self.mtime = mtime

    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    def cls2byte(self):
        str_ = 'upload\n'+json.dumps(self.__dict__)+'\0'
        return str_.encode(encoding='gbk')


class Upload_Chunk:
    """ 上传请求头 """

    def __init__(self, session, vfile_id, md5, offset,
                 chunksize):
        self.session = session
        self.vfile_id = vfile_id
        self.md5 = md5
        self.offset = offset
        self.chunksize = chunksize

    def cls2json(self):
        """ convert Response class to Json(str) """
        return json.dumps(self.__dict__)

    def cls2byte(self):
        str_ = 'uploadChunk\n'+json.dumps(self.__dict__)+'\0'
        return str_.encode(encoding='gbk')


class Upload_Res:
    """ 上传请求响应 """

    def __init__(self, error, msg, vfile_id):
        self.error = error
        self.msg = msg
        self.vfile_id = vfile_id

    # 可选构造函数
    @classmethod
    def json2cls(cls, j_):
        d_ = json.loads(j_)
        return cls(d_['error'], d_['msg'], d_['vfile_id'])


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


