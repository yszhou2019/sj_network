
## ����server

server��ַ���Ժ�д����


## �׶�����



### phase_0 

server��ַ 47.102.201.228:4500


����������
��������ļ������紫�䣬����ָ���ļ������ϴ�����
client��server����֮�����ָ���ļ������ϴ�������


������̣�
���ȣ�client����server�����Խ���ָ���ļ����ϴ������أ�����ָ��ģʽ������ `upload filename` `download filename`��
����֮��client��server��ip��portֱ��д���������棬Ȼ���ȡ�����в�����ִ��


`upload filename`���ᴴ��������֮��client�ȷ���json��ʽ��header��Ȼ��ֱ�Ӷ�ȡ�ļ�д��socket����
�������ݣ�json��ʽ���ͣ�������ʱ�ȼ�һ����ˣ�������Ŀ¼���޸�ʱ�䡢md5ʲô�ģ�����ȡ��Ϸ���

`download filename`����������֮��client�ȷ���json��ʽ��header��Ȼ��һֱ��ȡ��ֱ��

### upload

**client������json��ʽ��header֮��ֱ�Ӷ�ȡ�ļ�д��socket����**
```json
{
    "type": "upload",
    "filename": "txt",
    "size": 100 // �ֽ����������Զ�ȡָ�����ȵ��ֽ�
}
```

### download

client���߼���
client����������socket��Ȼ��clientд��json��ʽ������header��Ȼ��client�ٴ�socket��ȡһ��json��ʽ����Ӧ(�����ļ���size)��Ȼ��clientһֱ��ȡsocket���ڱ��ش����ļ���д�룬ֱ����ȡ��ָ�����ֽ�����

server���߼���
`download filename`������Ϻ�server�յ�����Ȼ���ȡ��Ӧ���ļ����Ƚ�json��ʽ����Ӧд��socket��Ȼ���ٽ��ļ�������д��socket

```json
// client������
{
    // ָ�����ط������ϵ�ĳ���ļ�(�ո��ϴ���һ��)
    "type": "download",
    "filename": "txt"
}

// server����Ӧ
{
    "size": 100,
}
```



### phase_1


ҵ���߼����֣���¼��ע�ᣬ�˳���¼�� ��**�߼��������Ҫ�У������ݿⱣ��һ��**��

- [ ] �ͻ��˵Ļ���ʵ�֣�������ҵ���߼���
- [ ] �������Ļ���ʵ�֣�������ҵ���߼���

���һ�������Ƿֿ鴫�������ͬ���̣�����Ҫ��ɣ�

- [ ] �����ϴ�����ִ��
- [ ] ������������ִ��
- [ ] ��ȡserver��Ŀ¼
- [ ] �ͻ��˱��ؼ����ļ�
- [ ] clientɨ�豾�ص�Ŀ¼








### phase_2

�ֿ��ϴ�/����

- [ ] �ֿ��ϴ���ʵ��



### phase_2

- [ ] ��ͻ��˵�ʵ��





## һЩ�����Ĺ�ʶ

### ǰ�������

>server: C++
>
>client: python
>
>



### �ļ��ֿ飨��ʱ����ʵ�֣�

>�ļ��ֿ��Ŀ�ģ�
>������Ϊ�˶���� or ���̣߳��п��ܻ���������ļ����ϴ�
>������Ϊ�˾ֲ��޸ģ�����������Щ���ܣ�����ʵ��
>
>�ͻ��˷ֿ��ϴ���ʱ��server��α��棿
>��Ԥ�����ú��ļ�����������ļ�����ϣ�ֱ�ӽ�����д��ƫ��λ�ü��ɣ�
>
>�ֿ��ϴ��Ľ��ȼ�¼�������ڶ�Ӧvfile��chunks�ֶ��У�����
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
>�ֿ�ͬ�����뷨
>
>�ϴ�
>
>>�������ͻ����ϴ��ļ����ϴ�ĳ���ļ�
>>
>>ϸ�ڣ��ϴ���ʱ�򣬿ͻ��˶��ļ�������Ƭ��4MB��Сһ�飬һ���ļ�����һ���������飬ÿ�������¼��offset�Ͷ�Ӧ��size��ÿ�����ϴ����֮�󣬿ͻ��˻���յ���������Ӧ��res���������Ὣ��Ӧ��chunk��Ϣд�뵽���ݿ��У����п�������֮����������������󣬴�������ļ��������
>>
>>�ͻ�����ͣ���˳��ͻ��ˣ��ٴδ򿪣�
>>�ͻ�����Ҫ�������е��ϴ������Լ������ϴ��е��ļ��ķֿ���Ϣ
>
>����
>
>>�������ͻ��������ļ�
>>
>>ϸ�ڣ����أ�����������
>>
>>�ͻ�����ͣ���˳��ͻ��ˣ��ٴδ򿪣�
>>�ͻ�����Ҫ�����е������б����浽���أ���ǰ����ķֿ���ϢҲ�Ǳ��浽����



### ���������͵�Ŀ¼��Ϣ��������

>���ȿͻ��˻Ὣ**���ص�ɨ���¼����һ���¼�ڱ���**������ÿ�δ򿪿ͻ��˻�����ɨ�裨�Լ����Լ��Աȣ�����һЩ�ϴ�����/��������Ĵ��� �� ִ�У�
>Ȼ���ȡ��������Ŀ¼�����б���Ŀ¼�ͷ�����Ŀ¼�ıȶԣ���������Ĵ�����ִ��
>
>



### ��¼ƾ֤

>��Ȼ��Ҫ��¼ƾ֤�ģ�ÿ�ζ���Ҫ��¼
>
>





## ���ݿ�



### session��

session����������session->uid����������ͻ��˵�ƾ֤�����ͻ����˳����ͻ��˷���������Ϣ�����������ٶ�Ӧ��ƾ֤��

����ͻ��˵�¼�����

>��Ȼ�����Ⱥ�ģ��ȵ�¼�Ŀͻ��˻���server�����µ�session�����¼�Ŀͻ��˻�ӵ����ͬ��session
>
>server�����login�߼���
>����֤username��pwd����֤ͨ��֮�������session��ô����session��û��������session�����ӵ�����





## ������

worker���̸���������󡢽�������ִ�в�������Ӧ





## �ͻ���

�����߼�

>�ȵ�¼����һ��socket������¼�ɹ�֮�󣬱���session���ڴ��У�Ȼ��**Ԥ�ȴ�������socket**������֮���ã�����ͻ������Ҫ���д���Ļ���socket��Դ�ǹ̶�����ģ���ֱ���ͻ��˳���رգ��Źر�socket
>
>
>
>֮��ɨ�豾�� / ɨ��server���͹�����Ŀ¼�������ϴ�����/��������ִ��...
>
>��ÿ�ε���socket�����ʱ����ҪЯ��session�ֶΣ�������ҪЯ��username��pwd�ֶΣ�
>
>



## ͨ�Ź淶



### ������ͨ������

| ��������                 | type      |
| ------------------------ | --------- |
| ע���˺�                 | signup    |
| ��¼�˺�                 | login     |
| ��������                 | changepwd |
| �˳���¼                 | logout    |
| ��ȡ���������ļ�Ŀ¼�ṹ | dirinfo   |
| �ϴ�ĳ���ļ�������Ƭ�Σ� | upload    |
| ����ĳ���ļ�������Ƭ�Σ� | download  |
|                          |           |



### ע���˺�

��������

```json
{
    "type": "signup",
    "username": "admin0606",
    "password": "123456",
}
```



�ɹ���Ӧ

```json

```



ʧ����Ӧ

```json

```





### ��¼�˺�

����

```json
{
    "type": "login",
    "username": "admin0606",
    "password": "123456",
    
}
```

�ɹ���Ӧ



ʧ����Ӧ



### ��������

����

```json
{
    "type": "changepwd",
    "session": "xxxxxxxxx(32)"
}
```

�ɹ���Ӧ

ʧ����Ӧ



### �˳���¼

```json
{
    "type": "logout",
    "session": "xxxxxxxxx(32)"
    
}
```





### ��ȡ���������ļ�Ŀ¼�ṹ

����

```json
{
    "type": "dirinfo",
    // ���е�Ŀ¼
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
	// ���е��ļ���Ϣ
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



�ɹ���Ӧ

```json

```



ʧ����Ӧ

```json

```



### �ϴ�ĳ���ļ�������Ƭ�Σ�

����

```json

```



�ɹ���Ӧ

```json

```



ʧ����Ӧ

```json

```



### ����ĳ���ļ�������Ƭ�Σ�

### 

����

```json

```



�ɹ���Ӧ

```json

```



ʧ����Ӧ

```json

```
