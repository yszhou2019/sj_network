
'''
�������ܣ��ϴ�ĳ���ļ�
����������ļ�����������д���ɱ�����Ҳ�����Ƕ�ȡ�����У����㣩
�����߼��� 1. �����ļ�������json��ʽ��header��Ȼ��д��socket
          2. д���ļ������� 

client����header:          
{
    "type": "upload",
    "filename": "txt",
    "size": 100 // �ֽ�����������server��ȡָ�����ȵ��ֽ�
}
'''
def upload():
    # ��֮ǰ�����õ�socket

    # ���ȷ���json��ʽ��������

    # Ȼ�����ļ�������



'''
�������ܣ�����ĳ���ļ�
����������ļ�����������д���ɱ�����Ҳ�����Ƕ�ȡ�����У����㣩
�����߼���1. client����json��ʽ��header
         2. client��ȡserver��res����ȡ�ļ�size
         3. ��ȡָ�����ȵ��ֽڣ�д���ļ���
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
'''
def download():


def demo_client():
    # step 1 : socket����ָ����server��ip��port
    connect(ip,port)

    # step 2: ���������в���
    if arg[0]=="upload"
        upload()
    elif arg[0] == "download"
        download()

    # step 2: ����˵ֱ���ڳ�������д��
    upload("txt")

    # download("txt")