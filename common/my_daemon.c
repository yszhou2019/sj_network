#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>


int my_daemon(int nochdir,int noclose)
{
    int fd;

    switch (fork()) {
        case -1:
            printf("fork() failed\n");
            return -1;

        case 0:
            break;

        default:
            exit(0);
    }
    
    /*
    pid_t setsid(void);
    ���̵���setsid()�ɽ���һ���¶Ի��ڼ䡣
    ������ô˺����Ľ��̲���һ����������鳤����˺�������һ���¶Ի��ڣ����Ϊ��
        1���˽��̱�ɸ��¶Ի��ڵĶԻ����׽��̣�session leader���Ի����׽����Ǵ����öԻ��ڵĽ��̣���
           �˽����Ǹ��¶Ի����е�Ψһ���̡�
        2���˽��̳�Ϊһ���½�������鳤���̡��½�����ID���ǵ��ý��̵Ľ���ID��
        3���˽���û�п����նˡ�����ڵ���setsid֮ǰ�ν�����һ�������նˣ���ô������ϵҲ�������
    ������ý����Ѿ���һ����������鳤����˺������ش���Ϊ�˱�֤���������������ͨ���ȵ���fork()��
    Ȼ��ʹ�丸������ֹ�����ӽ��̼���ִ�С���Ϊ�ӽ��̼̳��˸����̵Ľ�����ID�����ӽ��̵Ľ���ID������
    ����ģ����߲�������ȣ�������ͱ�֤���ӽ��̲���һ����������鳤��
    */
    if (setsid() == -1) {
        printf("setsid() failed\n");
        return -1;
    }
    
    switch (fork()) {
        case -1:
            printf("fork() failed\n");
            return -1;

        case 0:
            break;

        default:
            exit(0);
    }

    umask(0);
    if(nochdir==0){
        chdir("/");
    }
    

    if(noclose==0){
        // close(0);
        // fd0=open("/dev/null",O_RDWR);//�رձ�׼���� ����׼�������׼����
        // dup2(fd0,1);
        // dup2(fd0,2);
        long maxfd;
        if ((maxfd = sysconf(_SC_OPEN_MAX)) != -1)
        {
            for (fd = 0; fd < maxfd; fd++)
            {
                close(fd);
            }
        }
        fd = open("/dev/null", O_RDWR);
        if (fd == -1) {
            printf("open(\"/dev/null\") failed\n");
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            printf("dup2(STDIN) failed\n");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            printf("dup2(STDOUT) failed\n");
            return -1;
        }

        if (dup2(fd, STDERR_FILENO) == -1) {
            printf("dup2(STDERR) failed\n");
            return -1;
        }

        if (fd > STDERR_FILENO) {
            if (close(fd) == -1) {
                printf("close() failed\n");
                return -1;
            }
        }
    }
    
    /*
    // Standard file descriptors.
    #define STDIN_FILENO    0   // Standard input.
    #define STDOUT_FILENO   1   // Standard output.
    #define STDERR_FILENO   2   // Standard error output.
    */
    
    /*
    int dup2(int oldfd, int newfd);
    dup2()�������Ʋ���oldfd��ָ���ļ�������������������������newfd��һ�鷵�ء�
    ���newfd�Ѿ��򿪣����Ƚ���رա�
    ���oldfdΪ�Ƿ���������dup2()���ش��󣬲���newfd���ᱻ�رա�
    ���oldfdΪ�Ϸ�������������newfd��oldfd��ȣ���dup2()�����κ��£�ֱ�ӷ���newfd��
    */


    return 0;
}
