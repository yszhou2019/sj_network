
client������һЩ��������̽�������⣬���������Դ�����⣬��������˼·����������ļ�����

## 

21/6/25��
~~��client���ͺ�����header֮���յ���response�޷�����Ϊclass~~



21/6/28��

1. ~~д��recver(��)~~

2. ~~res_queue�Լ�task�Լ�com�����޸�Ϊͨ��python::map{}ʵ�֣���ʽΪ��~~

~~res_queue={que_id:"json",}~~

~~��Ҫ�޸�res_queue����صĲ�������������~~

3. ͬ����ʱ��size=0�Ĳ�����
4. �޸�һ��path��filename�������ʽ��path���һ����һ��'/'����filename����һ���ɸɾ���������



#### client to do list:

����db������������md5����path+filename

���ص�½֮����ͬ��queue��ʱ�򣬽�session����

~~�޸�res_queue����ز���~~

1. first_sync

   1.1 ɨ�豾�ز��ϴ�

   ɨ�������

   ![image-20210628194713579](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210628194713579.png)

   ʹ��os.path.walk

   �ڴ���Ŀ¼ʱ��ʹ�ü�ȥlen�ķ�ʽ��

   ![image-20210628194749228](C:\Users\63450\AppData\Roaming\Typora\typora-user-images\image-20210628194749228.png)

   

   1.2 ɨ�豾���������������

   �ϴ����ȴ���task���ٴ���req

   ���أ����ڱ��ش���һ�����ļ�������db����һ����¼

2. not_first_sync

   ͬ������ʱ�����������е�api

3. ~~sender~~

   ~~3.1 que_id�Լ�����id�ĳ�ʼ���Լ��־û�~~

4. ~~recver~~

5. handle_login

6. handle_logout

7. handle_signup

8. ~~is_cancel_bind~~

9. ~~handle_bind~~

10. session�����ٺ�clientִ��logout������

11. ~~api�����˸��ģ���Ҫ�޸�~~

12. ~~insert_into_db~~

13. ~~update_db~~

д��֮�󣬽��е�Ԫ����





req_queue��ʽ����Ҫ�ο�api�ĵ��к���+���ֲ���

res_queue��ʽ��

res_queue = {

que_id_1 :"uploadFile\n{
	"session":"32��char",
	"taskid":,
	"queueid":,
	"filename": ,
	"path": ,
	"md5": ,
	"size": ,
	"mtime": ,
}\0",

que_id_2: {uploadFile
	"session":"32��char",
	"vfile_id":
	"md5":
	"taskid":,
	"queueid":,
	"chunkid":,
	"offset": ,
	"chunksize": 
},

que_id_3:{downloadFile\n
"session":"32��char",
"filename":
"path":
"md5":
"taskid"
"queueid":,
"chunkid",
"offset":,
"chunksize":
}

que_id: init_req(ԭ��ɶ����ɶ��)

}



task_queue��

{

id_1:{},

id_2:{}

}





#### ��Ԫ����

1. db

   ~~1.1 db_insert~~

   ~~1.2 db_select~~

   1.3 db_update

   1.4 db_delete

2. gen_req:������pack����req���У�req������Ҫ��pack�����ȥ�������ٷ��ͣ�

   2.1 gen_req_createDir

   2.2 gen_req_uploadFile

   2.3 gen_req_deleteFile

   2.4 gen_req_download

3. gen_task:

   3.1 gen_task_upload_download

4. handle:(��������µ�c/s����)

   3.1 handle_getDir

   3.2 handle_bind

   3.3 handle_cancel_bind

5. persistence:

   5.1 bind_persistence

   5.2 queue_persistence

   5.3 db_persistence

6. �����߳�
7. ����sender��recver



?	



