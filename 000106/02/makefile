TARGET = test2
CC = gcc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

$(TARGET):test2.o  
	$(CC) $^ -o $@ -L/usr/lib64/u1752240 -ltest2
	# -L指明动态链接库的路径 -l指明动态链接库的名字
#	$(CC) $^ -o $@ -Wl,-z,origin -Wl,-rpath='/$$ORIGIN'

install:
	cp libtest2.so /usr/lib64/u1752240/libtest2.so

dynamic: test1.c
	$(CC) -shared -fpic $^ -o libtest2.so

#libtest2.so:test1.c
#	$(CC) -shared -fpic $^ -o $@

%.o: %.c
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TARGET) libtest2.so
