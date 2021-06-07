#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"
#include <signal.h>
#include <sys/prctl.h>
#include <stdlib.h> // header for random
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/wait.h>

int CntSplit=0;
int CntCollect=0;

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

int sub(int size){
    char *str=(char*)malloc(size*1024*sizeof(char));
    if(str==NULL){
        printf("内存不足，分裂失败\n");
        fflush(stdout);
        exit(0);
    }
    for(int i=0;i<size*1024;i++){
        str[i]=random()%256;
    }
    sleep(20);
    exit(0);
}

void handler(int sig){
    int pid;
    while((pid=waitpid(-1,NULL,WNOHANG))>0){ // async wait for signals
        CntCollect++;
        printf("子进程 pid = %d , signal = %d ,已回收 %d \n",pid,sig,CntCollect);
        fflush(stdout);
    };
}

int main(int argc, char* argv[]){
	if(argc!=7){
		printf("incorrect arg number, exit\n");
		exit(0);
	}
    int fnum;
    int size;
    int ptime;
    for (int i = 1; i <= 3; ++i){
        if(!strcmp(argv[2*i-1], "--fnum")){
            StrToNum(argv[2*i], &fnum);
        }
        else if(!strcmp(argv[2*i-1], "--size")){
            StrToNum(argv[2*i], &size);
        }
        else if(!strcmp(argv[2*i-1], "--ptime")){
            StrToNum(argv[2*i], &ptime);
        }
    }
    my_daemon(1,1);
    signal(SIGCHLD, handler); // register daemon, waits for sig from child, and calls handler
    struct timeval tStart;
    struct timeval tEnd;
    gettimeofday(&tStart,NULL);
    int time;
    int pid;
    for(int i=0;i<fnum;i++){
        pid=fork();
        if(pid==0){
			prctl(PR_SET_PDEATHSIG, SIGKILL); // child exit when daemon process is killed
            break;
        }else{
            if(pid==-1){
                // printf("分裂失败 退出\n");
                // fflush(stdout);
                // return 0;
                i--;
                sleep(1);
            }else{
                CntSplit++; // increment
            }
            gettimeofday(&tEnd,NULL);
            time=(int)(tEnd.tv_sec-tStart.tv_sec);
            if(time%ptime==0){
			    printf("已分裂 %d 个子进程，已回收 %d 个子进程\n",CntSplit,CntCollect);
			    fflush(stdout);
            }
        }
    }
    gettimeofday(&tEnd,NULL);
    time=(int)(tEnd.tv_sec-tStart.tv_sec);
    while(1){
        if(pid==0){
            sub(size);
        }else{
            if(CntSplit==fnum&&CntCollect==fnum){
                printf("分裂/回收 %d 个子进程完成，用时 %d 秒\n",fnum,time);
            }
            fflush(stdout);
            sleep(5);
        }
    }
    return 0;
}