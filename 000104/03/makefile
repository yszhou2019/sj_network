CC	=clang
# 解决clang对于中文编码的报错
CFLAGS	=-Wno-invalid-source-encoding 


SRCS =$(wildcard *.c)
OBJS =$(patsubst %.c, %.o, $(SRCS))
TRGS =$(patsubst %.c, %, $(SRCS))

all:$(TRGS)

# 可执行文件依赖于.o
$(TRGS):%:%.o
	$(CC) $(CFLAGS) -o $@ $^

# .o依赖于源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(TRGS)
