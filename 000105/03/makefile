TARGET = mysql_demo
CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ -static -L/usr/lib64 -lmariadbclient -lz -lssl -lcrypto -ldl -lpthread  -I/usr/include/mysql

%.o: %.cpp
	$(CC) -c $< -I/usr/include/mysql

clean:
	rm -f $(OBJS) $(TARGET)
