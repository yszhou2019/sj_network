TARGET	=test
CC	=g++
#CFLAGS	=-MMD 

SRCS =$(wildcard *.cpp)
OBJS =$(SRCS:.cpp=.o)
DEPS =test.h

#all:$(TARGET)

# 可执行文件依赖于.o文件
$(TARGET):$(OBJS)
	$(CC) -o $@ $^

# .o依赖于对应的cpp和头文件
%.o: %.cpp $(DEPS)
	$(CC) -c $< 

clean:
	rm -f $(OBJS) $(TARGET)
