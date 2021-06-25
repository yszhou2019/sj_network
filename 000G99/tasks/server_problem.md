
server遇到的一些可以商讨探索的问题，或者自问自答的问题，方便整理思路，放在这个文件里面

## server向client发送数据，或者client向server发送数据的时候，该不该把数据放在json里面?

>用不着，json作为req和res的header，具体的数据可以之后再读取
>
>
>


## 服务器在处理客户端

>server先从最简单的来，epoll_wait等待连接，等待客户端发送的请求
>获取请求之后，识别，解析，执行，返回即可
>在获取请求(比如接收客户端上传到服务器的文件的时候，需要创建文件，然后读取socket，写入字节即可)
>
>所以说，server accept返回的socket应该采用阻塞还是非阻塞? 先采用非阻塞吧
>
>
>只不过，需要一个健壮的readn和sendn，socket应该是非阻塞的
>(我看到的，这个server的demo，采用的是阻塞socket)