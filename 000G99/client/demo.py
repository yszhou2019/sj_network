
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
def upload():
    # 用之前创建好的socket

    # 首先发送json格式的请求体

    # 然后发送文件的数据



'''
函数功能：下载某个文件
输入参数：文件名（可以是写死成变量，也可以是读取命令行，看你）
具体逻辑：1. client生成json格式的header
         2. client读取server的res，获取文件size
         3. 读取指定长度的字节，写入文件中
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
def download():


def demo_client():
    # step 1 : socket连接指定的server的ip和port
    connect(ip,port)

    # step 2: 解析命令行参数
    if arg[0]=="upload"
        upload()
    elif arg[0] == "download"
        download()

    # step 2: 或者说直接在程序里面写死
    upload("txt")

    # download("txt")