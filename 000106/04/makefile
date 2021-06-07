TARGET = test2
CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

$(TARGET):test2.o
	$(CC) $^ -o $@ -L/usr/lib64/u1752240 -ltest4
#	$(CC) $^ -o $@ -Wl,-z,origin -Wl,-rpath='/$$ORIGIN'

install:
	cp libtest4.so /usr/lib64/u1752240/libtest4.so

dynamic: test1.cpp
	$(CC) -shared -fpic $^ -o libtest4.so

#libtest4.so:test1.cpp
#	$(CC) -shared -fpic $^ -o $@

%.o: %.cpp
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TARGET) libtest4.so
