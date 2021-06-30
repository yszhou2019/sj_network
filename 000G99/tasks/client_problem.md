
client遇到的一些可以商讨探索的问题，或者自问自答的问题，方便整理思路，放在这个文件里面

## 

21/6/25：
~~在client发送好下载header之后，收到的response无法解析为class~~



21/6/28：

1. ~~写完recver(√)~~

2. ~~res_queue以及task以及com队列修改为通过python::map{}实现，格式为：~~

~~res_queue={que_id:"json",}~~

~~需要修改res_queue的相关的操作（！！！）~~

3. 同步的时候size=0的不操作
4. 修改一下path和filename的组合形式：path最后一定是一个'/'，而filename就是一个干干净净的名字



#### client to do list:

本地db的主键不能用md5，用path+filename

本地登陆之后，在同步queue的时候，将session更新

~~修改res_queue的相关操作~~

1. first_sync

   1.1 扫描本地并上传

   扫描操作：

   ![image-20210628194713579](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210628194713579.png)

   使用os.path.walk

   在处理目录时，使用减去len的方式：

   ![image-20210628194749228](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210628194749228.png)

   

   1.2 扫描本地与服务器并下载

   上传，先创建task，再创建req

   下载，先在本地创建一个空文件，再在db创建一个记录

2. not_first_sync

   同步操作时，创建了所有的api

3. ~~sender~~

   ~~3.1 que_id以及其他id的初始化以及持久化~~

4. ~~recver~~

5. ~~handle_login~~

6. ~~handle_logout~~

7. ~~handle_signup~~

8. ~~is_cancel_bind~~

9. ~~handle_bind~~

10. session被销毁后，client执行logout的命令

11. ~~api发生了更改，需要修改~~

12. ~~insert_into_db~~

13. ~~update_db~~

写完之后，进行单元测试





req_queue格式：需要参考api文档中横线+文字部分

res_queue格式：

res_queue = {

que_id_1 :"uploadFile\n{
	"session":"32个char",
	"taskid":,
	"queueid":,
	"filename": ,
	"path": ,
	"md5": ,
	"size": ,
	"mtime": ,
}\0",

que_id_2: {uploadFile
	"session":"32个char",
	"vfile_id":
	"md5":
	"taskid":,
	"queueid":,
	"chunkid":,
	"offset": ,
	"chunksize": 
},

que_id_3:{downloadFile\n
"session":"32个char",
"filename":
"path":
"md5":
"taskid"
"queueid":,
"chunkid",
"offset":,
"chunksize":
}

que_id: init_req(原来啥样就啥样)

}



task_queue：

{

id_1:{},

id_2:{}

}





#### 单元测试

1. db

   ~~1.1 db_insert~~

   ~~1.2 db_select~~

   ~~1.3 db_update~~

   ~~1.4 db_delete~~

2. gen_req:（生成pack进入req队列，req队列需要将pack拆包并去除数据再发送）

   ~~2.1 gen_req_createDir~~

   ~~2.2 gen_req_uploadFile~~

   ~~2.3 gen_req_deleteFile~~

   ~~2.4 gen_req_download~~

3. gen_task:

   ~~3.1 gen_task_upload_download~~

4. handle:(阻塞情况下的c/s交互)

   ~~3.1 handle_getDir~~

   ~~3.2 handle_bind~~

   ~~3.3 handle_cancel_bind~~

5. persistence:

   ~~5.1 bind_persistence~~

   ~~5.2 queue_persistence~~

   5.3 db_persistence

6. 测试线程定时器
7. 测试sender和recver



发现的问题：

1. ~~传给server的数据没有经过删减~~

2. ~~一下传给server三个包~~

   ![image-20210630032827531](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210630032827531.png)

?	



test：检测简单的通信

1. login signup

   ![image-20210630141328084](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210630141328084.png)

2. bind

   ![image-20210630141855514](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210630141855514.png)

3. getdir

   handle_getDir有问题

4. uploadFile和downloadFile

   ![image-20210630160700275](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210630160700275.png)

   ![image-20210630163441780](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210630163441780.png)

   ![image-20210630163658378](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210630163658378.png)



test：检测逻辑问题，



sender和recver不能同时启动的原因：

1. target=fun，没有括号
2. 不能用run()，要用start

发送和接收出现数量不匹配的情况：

1. recv和send的系统调用都是不可靠的，正确的做法是将待读取和写入的数据保存到buffer中。
2. 对于较大的数据，调用linux的零拷贝系统调用，避免过多的io

