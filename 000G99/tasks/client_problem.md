
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

   上传/下载时，先创建task再创建req

2. not_first_sync

   同步操作时，创建了所有的api

3. ~~sender~~

   ~~3.1 que_id以及其他id的初始化以及持久化~~

4. ~~recver~~

5. handle_login

6. handle_logout

7. handle_signup

8. is_cancel_bind

9. handle_bind

10. session被销毁后，client执行logout的命令

11. api发生了更改，需要修改





res_queue格式：

res_queue = {

que_id_1 : {uploadFile
	"session":"32个char",
	"taskid":,
	"queueid":,
	"filename": ,
	"path": ,
	"md5": ,
	"size": ,
	"mtime": ,
},

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

}