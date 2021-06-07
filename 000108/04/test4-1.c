#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"

int main(){
    my_daemon(1,1);
    int pid;
    for(int i=0;i<10;i++){
        pid=fork();
        if(pid==0){
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