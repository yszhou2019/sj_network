
## 连接server

server地址我稍后写出来


## 阶段任务



### phase_0 

server地址 47.102.201.228:4500


大致描述：
首先完成文件的网络传输，可以指定文件进行上传下载
client和server连接之后可以指定文件进行上传和下载


具体过程：
首先，client连接server，可以进行指定文件的上传和下载（按照指令模式，比如 `upload filename` `download filename`）
运行之后，client把server的ip和port直接写到代码里面，然后读取命令行参数并执行


`upload filename`：会创建并连接之后，client先发送json格式的header，然后直接读取文件写入socket即可
发送内容（json方式发送，内容暂时先简单一点好了，不考虑目录、修改时间、md5什么的），读取完毕发送

`download filename`：创建连接之后，client先发送json格式的header，然后一直读取，直到

### upload

**client发送了json格式的header之后，直接读取文件写入socket即可**
```json
{
    "type": "upload",
    "filename": "txt",
    "size": 100 // 字节数量，用以读取指定长度的字节
}
```

### download

client的逻辑：
client创建并连接socket，然后client写入json格式的请求header，然后client再从socket读取一个json格式的响应(包含文件的size)，然后client一直读取socket并在本地创建文件并写入，直到读取够指定的字节数量

server的逻辑：
`download filename`连接完毕后，server收到请求，然后读取对应的文件，先将json格式的响应写入socket，然后再将文件的数据写入socket

```json
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
```



### phase_1


业务逻辑部分（登录，注册，退出登录） （**逻辑部分最好要有，和数据库保持一致**）

- [ ] 客户端的基本实现（基本的业务逻辑）
- [ ] 服务器的基本实现（基本的业务逻辑）

完成一个不考虑分块传输的网络同步盘（必须要完成）

- [ ] 创建上传任务，执行
- [ ] 创建下载任务，执行
- [ ] 获取server的目录
- [ ] 客户端本地监听文件
- [ ] client扫描本地的目录








### phase_2

分块上传/下载

- [ ] 分块上传的实现



### phase_2

- [ ] 多客户端的实现





## 一些基本的共识

### 前后端语言

>server: C++
>
>client: python
>
>



### 文件分块（暂时不用实现）

>文件分块的目的：
>仅仅是为了多进程 or 多线程，有可能会加速整体文件的上传
>并不是为了局部修改，增量更新这些功能（不现实）
>
>客户端分块上传的时候，server如何保存？
>（预先设置好文件体积，传输文件块完毕，直接将数据写入偏移位置即可）
>
>分块上传的进度记录，保存在对应vfile的chunks字段中，如下
>
>```json
>{
>"offset": 0,
>"size": 1024,
>"done": true,
>},
>{
>"offset": 1024,
>"size": 1024,
>"done": true,
>},
>{
>"offset": 2048,
>"size": 1024,
>"done": false,
>}
>```
>
>
>
>分块同步的想法
>
>上传
>
>>场景：客户端上传文件，上传某个文件
>>
>>细节：上传的时候，客户端对文件进行切片，4MB大小一块，一个文件生成一个任务数组，每个数组记录了offset和对应的size，每个块上传完毕之后，客户端会接收到服务器对应的res，服务器会将对应的chunk信息写入到数据库中，所有块接收完毕之后，向服务器发送请求，代表这个文件传送完毕
>>
>>客户端暂停后退出客户端，再次打开：
>>客户端需要保存所有的上传任务，以及正在上传中的文件的分块信息
>
>下载
>
>>场景：客户端下载文件
>>
>>细节：下载，服务器下载
>>
>>客户端暂停后退出客户端，再次打开：
>>客户端需要把所有的任务列表保存到本地，当前任务的分块信息也是保存到本地



### 服务器传送的目录信息放在那里

>首先客户端会将**本地的扫描记录保存一遍记录在本地**，另外每次打开客户端会重新扫描（自己和自己对比，进行一些上传任务/下载任务的创建 与 执行）
>然后获取服务器的目录，进行本地目录和服务器目录的比对，进行任务的创建和执行
>
>



### 登录凭证

>仍然是要登录凭证的，每次都需要登录
>
>





## 数据库



### session表

session这个表，完成session->uid，用来保存客户端的凭证（当客户端退出，客户端发出销毁信息，服务器销毁对应的凭证）

多个客户端登录的情况

>必然是有先后的，先登录的客户端会让server创建新的session，后登录的客户端会拥有相同的session
>
>server这里的login逻辑：
>先验证username和pwd，验证通过之后，如果有session那么返回session，没有则生成session并添加到表中





## 服务器

worker进程负责接收请求、解析请求、执行并返回响应





## 客户端

基本逻辑

>先登录（用一个socket），登录成功之后，保存session到内存中，然后**预先创建若干socket**，并且之后复用（比如客户端如果要并行传输的话，socket资源是固定不变的），直到客户端程序关闭，才关闭socket
>
>
>
>之后，扫描本地 / 扫描server传送过来的目录，创建上传任务/下载任务，执行...
>
>（每次调用socket传输的时候，需要携带session字段，不再需要携带username和pwd字段）
>
>



## 通信规范



### 基本的通信类型

| 操作类型                 | type      |
| ------------------------ | --------- |
| 注册账号                 | signup    |
| 登录账号                 | login     |
| 更改密码                 | changepwd |
| 退出登录                 | logout    |
| 获取服务器的文件目录结构 | dirinfo   |
| 上传某个文件（或者片段） | upload    |
| 下载某个文件（或者片段） | download  |
|                          |           |



### 注册账号

发送请求

```json
{
    "type": "signup",
    "username": "admin0606",
    "password": "123456",
}
```



成功响应

```json

```



失败响应

```json

```





### 登录账号

请求

```json
{
    "type": "login",
    "username": "admin0606",
    "password": "123456",
    
}
```

成功响应



失败响应



### 更改密码

请求

```json
{
    "type": "changepwd",
    "session": "xxxxxxxxx(32)"
}
```

成功响应

失败响应



### 退出登录

```json
{
    "type": "logout",
    "session": "xxxxxxxxx(32)"
    
}
```





### 获取服务器的文件目录结构

请求

```json
{
    "type": "dirinfo",
    // 所有的目录
    "dirs": 
    {
        {
        	"dir_id": ,
        	"dir_path": ,
    	},
        {
        	"dir_id": ,
        	"dir_path": ,
    	},
    },
	// 所有的文件信息
    "files":
    {
        {
        	"vfile_id": ,
        	"vfile_dir_id": ,
        	"vfile_name": ,
        	"vfile_size": ,
        	"vfile_md5": ,
        	"vfile_server_mtime": ,
    	}
    },
}
```



成功响应

```json

```



失败响应

```json

```



### 上传某个文件（或者片段）

请求

```json

```



成功响应

```json

```



失败响应

```json

```



### 下载某个文件（或者片段）

### 

请求

```json

```



成功响应

```json

```



失败响应

```json

```

