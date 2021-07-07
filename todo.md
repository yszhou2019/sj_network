- [x] TODO : server运行的时候，epoll长时间没有触发，会导致数据库断开连接
- [ ]   似乎server还有可能down掉
- [x]   类型ll的替换 -> size
- [x]   时间 -> int
- [x]   API downloadFile
- [x]   日志系统
- [x]   ~~pfile库~~
- [x]   type + json + [binary data] 要一块发送（不需要一块发送，如果超时1秒，就放弃读取数据）
- [x]   该不该阻塞呢？
- [ ]   守护进程
- [x]   创建数据库
- [x]   能够编译运行
- [ ]   rio？



服务器现在的 sock <-> file 已经是rio了

服务器接收header 发送header
发送的时候，可以用sendn来保证rio
接收的时候，由于数据比较小，应该可以保证是rio（其实只需要保证 read_until 的rio即可）
关键是，接收 type 和 json 的时候，rio没有保证

服务器接收json，都是小数据（虽然是小数据，但也应该通过read_until来保证读取内容的正确性）

服务器发送json，有大数据，但是可以通过sendn来解决



日志

>
>
>写到 /home/G1752240/log下
>
>注册，注册成功，进行记录
>登录，登录成功，进行记录，写入日志
>
>绑定，日志写入绑定





宿舍

55M，

4M传输时间80s，平均每秒51K

4M下载时间4s，平均每秒1024K

图书馆1s上传1MB