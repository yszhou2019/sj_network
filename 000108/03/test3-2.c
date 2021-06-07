#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"
#include <sys/wait.h>

int main(){
    my_daemon(1,1);
    int pid;
    for(int i=0;i<10;i++){
        pid=fork();
        if(pid==0){ // son
            pid=fork();
            if(pid==0){ // grandson
                break;
            }else{ // son
                return 0;
            }
        }else{
            sleep(3);
        }
        waitpid(pid, NULL, 0);
    }
    int cnt=0;
    while(1){
        if(pid==0){
            printf("%d %d 1752240 sub\n",getppid(),getpid());
            fflush(stdout);
            sleep(25);
            cnt++;
            if(cnt==3){
                break;
            }
        }else{
            printf("%d %d 1752240 main\n",getppid(),getpid());
            fflush(stdout);
            sleep(5);
        }
    }
    return 0;
}
