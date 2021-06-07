#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <stdlib.h> // header for random
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#define MAXLINE 2048

// without setsid
int my_daemon_spec(int nochdir,int noclose){
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
    进程调用setsid()可建立一个新对话期间。
    如果调用此函数的进程不是一个进程组的组长，则此函数创建一个新对话期，结果为：
        1、此进程变成该新对话期的对话期首进程（session leader，对话期首进程是创建该对话期的进程）。
           此进程是该新对话期中的唯一进程。
        2、此进程成为一个新进程组的组长进程。新进程组ID就是调用进程的进程ID。
        3、此进程没有控制终端。如果在调用setsid之前次进程有一个控制终端，那么这种联系也被解除。
    如果调用进程已经是一个进程组的组长，则此函数返回错误。为了保证不处于这种情况，通常先调用fork()，
    然后使其父进程终止，而子进程继续执行。因为子进程继承了父进程的进程组ID，而子进程的进程ID则是新
    分配的，两者不可能相等，所以这就保证了子进程不是一个进程组的组长。
    */

    // 不完全失去对终端的控制
    // if (setsid() == -1) {
    //     printf("setsid() failed\n");
    //     return -1;
    // }
    
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
        // fd0=open("/dev/null",O_RDWR);//关闭标准输入 ，标准输出，标准错误
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
    dup2()用来复制参数oldfd所指的文件描述符，并将它拷贝至参数newfd后一块返回。
    如果newfd已经打开，则先将其关闭。
    如果oldfd为非法描述符，dup2()返回错误，并且newfd不会被关闭。
    如果oldfd为合法描述符，并且newfd与oldfd相等，则dup2()不做任何事，直接返回newfd。
    */


    return 0;
}

extern char **environ;
static char **g_main_Argv = NULL;	/* pointer to argument vector */
static char *g_main_LastArgv = NULL; /* end of argv */

int readconf();

// wait if any subprocess exit
void handler(int signo) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0){
		printf("child %d exited with signal %d\n", pid, signo);
		fflush(stdout);
	}
}

// save the initial env args, because rewrite argv[0] (daemon name) may cover the args memory behind
void setproctitle_init(int argc, char **argv, char **envp) {
	int i;

	for (i = 0; envp[i] != NULL; i++) // calc envp num
		continue;
	environ = (char **)malloc(sizeof(char *) * (i + 1)); // malloc envp pointer

	for (i = 0; envp[i] != NULL; i++){
		environ[i] = malloc(sizeof(char) * strlen(envp[i]));
		strcpy(environ[i], envp[i]);
	}
	environ[i] = NULL;

	g_main_Argv = argv;
	if (i > 0)
		g_main_LastArgv = envp[i - 1] + strlen(envp[i - 1]);
	else
		g_main_LastArgv = argv[argc - 1] + strlen(argv[argc - 1]);
}

int main(int argc, char* argv[]){
    char pname_origin[20];
	strcpy(pname_origin, argv[0]); // daemon origin name

	time_t start_t, now_t;
	time(&start_t);
	int diff_t;

    my_daemon_spec(1,1);
	signal(SIGCHLD, handler); // if parent process recv SIGCHLD, then call handler()

	char argv_buf[MAXLINE] = {0}; // save argv paramters
	int i;
	for (i = 1; i < argc; i++) {
		strcat(argv_buf, argv[i]);
		strcat(argv_buf, " ");
	}
	setproctitle_init(argc, argv, environ); // save a copy of env args

	char pname_now[MAXLINE];
	char comm_name[MAXLINE];
	time(&now_t);
	diff_t = difftime(now_t, start_t);
	snprintf(pname_now, 60, "%s [main %02d:%02d:%02d]", pname_origin, diff_t / 3600, (diff_t % 3600 - diff_t % 60) / 60, diff_t % 60);
	snprintf(comm_name, 20, "%s [main]",pname_origin);
	prctl(PR_SET_NAME, comm_name); // rename pname [ps]
	strcpy(g_main_Argv[0], pname_now); // rename pname [ps -ef]

	int sub_num = readconf();
	printf("fork %d subprocess, now begin\n", sub_num);
	fflush(stdout);

	int pid;
	int subpid[10];

    // fork n sub_process
	for (i = 0; i < sub_num; i++){
		pid = fork();
        if(pid==-1){
            printf("fork failed/n");
            fflush(stdout);
            exit(0);
        }else if(pid==0){
            prctl(PR_SET_PDEATHSIG, SIGHUP); // exit when daemon is killed
            time(&start_t);
            time(&now_t);
            diff_t = difftime(now_t, start_t);
            snprintf(pname_now, 60, "%s [Sub-%02d %02d:%02d:%02d]", pname_origin, i, diff_t / 3600, (diff_t % 3600 - diff_t % 60) / 60, diff_t % 60);
            snprintf(comm_name, 20, "%s [sub-%02d]", pname_origin, i);
            prctl(PR_SET_NAME, comm_name); // rename subpname [ps]
            strcpy(g_main_Argv[0], pname_now); //rename subpname [ps -ef]
        }
		if (pid == 0)
			break;
		else{
			subpid[i] = pid;
			time(&now_t);
			diff_t = difftime(now_t, start_t);
			snprintf(pname_now, 60, "%s [main %02d:%02d:%02d]", pname_origin, diff_t / 3600, (diff_t % 3600 - diff_t % 60) / 60, diff_t % 60);
			strcpy(g_main_Argv[0], pname_now); // rename pname [ps -ef]
		}
		sleep(1);
	}

	while (1){
		if (pid == 0){
			time(&now_t);
			diff_t = difftime(now_t, start_t);
            snprintf(comm_name, 20, "%s [sub-%02d]", pname_origin, i);
            prctl(PR_SET_NAME, comm_name); // rename subpname [ps]
			snprintf(pname_now, 60, "%s [Sub-%02d %02d:%02d:%02d]", pname_origin, i, diff_t / 3600, (diff_t % 3600 - diff_t % 60) / 60, diff_t % 60);
			strcpy(g_main_Argv[0], pname_now); // rename subpname [ps -ef]
		}else{
			time(&now_t);
			diff_t = difftime(now_t, start_t);
			snprintf(pname_now, 60, "%s [main %02d:%02d:%02d]", pname_origin, diff_t / 3600, (diff_t % 3600 - diff_t % 60) / 60, diff_t % 60);
			strcpy(g_main_Argv[0], pname_now); // rename pname [ps -ef]
			for (i = 0; i < sub_num; i++){
				if (kill(subpid[i], 0) == -1){
					pid = fork();
					if (pid == 0){
						prctl(PR_SET_PDEATHSIG, SIGHUP);
						time(&start_t);
						break;
					}else{
						subpid[i] = pid;
                        sleep(1);
					}
				}
			}
		}
		fflush(stdout);
		sleep(1);
	};
	for (i = 0; environ[i] != NULL; i++)
		free(environ[i]);
	return 0;
}