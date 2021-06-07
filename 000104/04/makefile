CC	=clang++
# 解决clang对于中文编码的报错
CFLAGS	=-Wno-invalid-source-encoding 


SRCS =$(wildcard *.cpp)
OBJS =$(patsubst %.cpp, %.o, $(SRCS))
TRGS =$(patsubst %.cpp, %, $(SRCS))

all:$(TRGS)

# 可执行文件依赖于.o文件
$(TRGS):%:%.o
	$(CC) $(CFLAGS) -o $@ $^

# .o文件依赖于源文件
%.o:%.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(TRGS)
