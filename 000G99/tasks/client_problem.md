
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

   �ϴ�/����ʱ���ȴ���task�ٴ���req

2. not_first_sync

   ͬ������ʱ�����������е�api

3. ~~sender~~

   ~~3.1 que_id�Լ�����id�ĳ�ʼ���Լ��־û�~~

4. ~~recver~~

5. handle_login

6. handle_logout

7. handle_signup

8. is_cancel_bind

9. handle_bind

10. session�����ٺ�clientִ��logout������

11. api�����˸��ģ���Ҫ�޸�





res_queue��ʽ��

res_queue = {

que_id_1 : {uploadFile
	"session":"32��char",
	"taskid":,
	"queueid":,
	"filename": ,
	"path": ,
	"md5": ,
	"size": ,
	"mtime": ,
},

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

}