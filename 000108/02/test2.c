#include <stdio.h>
#include <unistd.h>
#include "my_daemon.c"
int main(int argc, char** argv)
{
    my_daemon(1,1);
    
    while (1)
    {
        printf("1752240 ÷‹‘∆Àß\n");
        fflush(stdout);
	    sleep(5);
    }
    
    return 0;
}
