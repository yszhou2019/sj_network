# 网络同步盘server端说明文档





## 安装说明

### 导入数据库

首先确保创建数据库`db1752240`，用户`u1752240`，密码`u1752240`

然后导入数据库

```bash
mysql -uu1752240 -pu1752240 < backup.sql
```



编译服务器

```bash
tar xf server.tar.bz2
cd server
make install
make
```



运行服务器

```shell
./server --serverport <指定端口>
```







## 数据库结构

<img src="https://yszhou.oss-cn-beijing.aliyuncs.com/img/20210707180831.png" alt="image-20210707180831639" style="zoom:80%;" />



要点：

user_session表，用途，映射session到uid

user表，user_bindid字段，保存当前作为同步盘的服务器绑定目录的id

physical_file表，保存对应实际文件的上传情况
其中，字段pfile_chunks，保存的是序列化的json，而对应的json实际上是所有chunk的上传情况（chunk序号，chunksize，是否上传完毕）



## 整体结构

服务器运行之后，自动连接数据库，以守护进程方式运行。

新连接fork之后，交给子进程进行处理。

子进程进入循环后，不断循环，读取数据，解析请求，执行命令，返回相应... 

直到发现连接断开，或者非法请求，那么子进程结束运行。




## 主要功能的流程



### 文件的上传和下载

都采用分块方式，一块大小最多4MB



客户端上传文件，chunks的信息保存在了服务器的pfile表中
客户端下载文件，chunks的信息保存在客户端中，并持久化到客户端硬盘



### 上传文件

<img src="https://yszhou.oss-cn-beijing.aliyuncs.com/img/20210707213700.png" alt="image-20210707213700199" style="zoom:80%;" />



### 上传文件块

<img src="https://yszhou.oss-cn-beijing.aliyuncs.com/img/20210707213719.png" alt="image-20210707213719652" style="zoom:80%;" />



### 下载文件块

<img src="https://yszhou.oss-cn-beijing.aliyuncs.com/img/20210707211637.png" alt="image-20210707211637713" style="zoom:80%;" />



### 获取远程目录

注意的地方，只能获取上传完毕的文件信息，未上传完毕的文件信息不予返回



<img src="https://yszhou.oss-cn-beijing.aliyuncs.com/img/20210707211348.png" alt="image-20210707211348022" style="zoom:80%;" />







# 客户端简单介绍

## 比对算法

本次扫描结果与之前的db文件比对 => 产生上传任务

之前的db与本次扫描结果比对 => 产生删除任务

本地与服务器目录比对 => 产生下载任务



产生的任务全部添加到`req`队列中



## 线程安全的队列

`req`队列，保存的是待发送的请求

`res`队列，保存的是已经发送但是还没有收到回复的请求。收到回复之后，则将任务从`res`队列中删除



## 客户端的持久化

每个userid + bindid 唯一对应一个db文件（也就是目录文件）以及队列文件



## 留下的扩展

由于两个队列可以保证线程安全，因此可以启动多个sender和receiver线程，从而加速客户端的用户体验。





