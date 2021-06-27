
client遇到的一些可以商讨探索的问题，或者自问自答的问题，方便整理思路，放在这个文件里面

## 

21/6/25：
在client发送好下载header之后，收到的response无法解析为class

#### client to do list:

本地db的主键不能用md5，用path+filename

本地登陆之后，在同步queue的时候，将session更新

1. first_sync

   1.1 扫描本地并上传

   1.2 扫描本地与服务器并下载

   上传/下载时，先创建task再创建req

2. not_first_sync

3. sender

4. recver

5. handle_login

6. handle_logout

7. handle_signup

8. is_cancel_bind

9. handle_bind

10. session被销毁后，client执行logout的命令