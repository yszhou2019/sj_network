#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"
#include <signal.h>
#include <sys/prctl.h>
#include <stdlib.h> // header for random
#include <string.h>
#include <mysql.h>	// mysql特有

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

int SQLaccess(){
    MYSQL     *mysql;   
    MYSQL_RES *result;   
    MYSQL_ROW  row;

    if ((mysql = mysql_init(NULL))==NULL) {
    	return -1;
    }

    if (mysql_real_connect(mysql,"localhost","root", "root123","demo",0, NULL, 0)==NULL) {
    	return -1;
    }

    mysql_set_character_set(mysql, "gbk"); 

    if (mysql_query(mysql, "select * from student")) {
    	return -1;
    }

    if ((result = mysql_store_result(mysql))==NULL) {
    	return -1;
    }

    while((row=mysql_fetch_row(result))!=NULL) {
    }

    mysql_free_result(result);   

    mysql_close(mysql);   

    return 0;
}

int sub(){
    SQLaccess();
    for(;;)
        sleep(1);
}

int main(int argc, char* argv[]){
	if(argc!=5){
		printf("incorrect arg number, exit\n");
		exit(0);
	}
    int fnum;
    int pnum;
    for (int i = 1; i <= 2; ++i){
        if(!strcmp(argv[2*i-1], "--fnum")){
            StrToNum(argv[2*i], &fnum);
        }
        else if(!strcmp(argv[2*i-1], "--pnum")){
            StrToNum(argv[2*i], &pnum);
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
            i--;
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
            sub();
        }else{
            printf("%d %d 1752240 main\n",getppid(),getpid());
            fflush(stdout);
            sleep(5);
        }
    }
    return 0;
}