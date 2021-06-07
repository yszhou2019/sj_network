#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"
#include <signal.h>
#include <sys/prctl.h>

int main(){
    my_daemon(1,1);
    int pid;
    for(int i=0;i<10;i++){
        pid=fork();
        if(pid==0){
			prctl(PR_SET_PDEATHSIG, SIGKILL); // child exit when daemon process is killed
            break;
        }else{
            sleep(3);
        }
    }
    while(1){
        if(pid==0){
            printf("%d %d 1752240 sub\n",getppid(),getpid());
            fflush(stdout);
            sleep(25);
        }else{
            printf("%d %d 1752240 main\n",getppid(),getpid());
            fflush(stdout);
            sleep(5);
        }
    }
    return 0;
}