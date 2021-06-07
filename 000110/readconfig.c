#include <stdio.h>
#include <stdlib.h>
int readconf()
{
	int i;
	FILE *fp;
	fp = fopen("/etc/stu/stu_1752240.conf", "r");
	fscanf(fp, "子进程数量=%d", &i);
	fclose(fp);

	if (i > 20 || i < 5)
		i = 10;
	return i;
}
