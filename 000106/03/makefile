TARGET = test2
CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

$(TARGET):test2.o libtest3.a
	$(CC) $^ -o $@ 

static: test1.o
	ar rcs libtest3.a $^


#libtest3.a:test1.o
#	ar rcs $@ $^

%.o: %.cpp
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TARGET) libtest3.a
