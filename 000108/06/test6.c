#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"
#include <signal.h>
#include <sys/prctl.h>
#include <stdlib.h> // header for random
#include <string.h>

void StrToNum(char *par, int *val){
    int ret = 0, len = strlen(par);
    for (int i = 0; i < len; ++i){
        if(par[i] < '0'||par[i] > '9'){
            return;
        }
        ret = ret * 10 + par[i] - '0';
    }
    *val = ret;
    return;
}

int sub(int size,int need_init){
    char *str=(char*)malloc(size*1024*sizeof(char));
    if(str==NULL){
        printf("内存不足，分裂失败\n");
        fflush(stdout);
        exit(0);
    }
    if(need_init){
        for(int i=0;i<size*1024;i++){
            str[i]=random()%256;
        }
    }
    for(;;)
        sleep(1);
}

int main(int argc, char* argv[]){
	if(argc!=9){
		printf("incorrect arg number, exit\n");
		exit(0);
	}
    int fnum;
    int size;
    int pnum;
    int init;
    for (int i = 1; i <= 4; ++i){
        if(!strcmp(argv[2*i-1], "--fnum")){
            StrToNum(argv[2*i], &fnum);
        }
        else if(!strcmp(argv[2*i-1], "--size")){
            StrToNum(argv[2*i], &size);
        }
        else if(!strcmp(argv[2*i-1], "--pnum")){
            StrToNum(argv[2*i], &pnum);
        }
        else if(!strcmp(argv[2*i-1], "--init")){
            if(!strcmp(argv[2*i], "yes")||!strcmp(argv[2*i], "y")||!strcmp(argv[2*i], "Y")){
                init=1;
            }else{
                init=0;
            }
        }
    }
    my_daemon(1,1);
    int pid;
    for(int i=0;i<fnum;i++){
        pid=fork();
        if(pid==0){
			prctl(PR_SET_PDEATHSIG, SIGKILL); // child exit when daemon process is killed
            break;
        }else if(pid == -1){
			printf("分裂失败 等待\n");
			fflush(stdout);
			sleep(1);
		}else{
            if((i+1)%pnum==0){
			    printf("已分裂 %d 个进程\n",i+1);
            }
			fflush(stdout);
        }
    }
    while(1){
        if(pid==0){
            sub(size, init);
            return 0;
        }else{
            printf("%d %d 1752240 main\n",getppid(),getpid());
            fflush(stdout);
            sleep(5);
        }
    }
    return 0;
}