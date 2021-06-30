CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TRGS = $(patsubst %.cpp, %, $(SRCS))

INCLUDE = -I./include
ETC = ./common/my_etc.cpp
TOOLS = my_etc.o



all: $(TOOLS) $(TRGS)

install:
	mkdir -p $(DIRROOT)/home/G1752240
	cp ./conf/u1752240.conf $(DIRROOT)/home/G1752240/u1752240.conf
	mkdir -p $(DIRROOT)/home/G1752240/log
	mkdir -p $(DIRROOT)/home/G1752240/realfile

$(TOOLS):
	$(CC) $(INCLUDE) -c $(ETC) `mysql_config --libs --cflags`

$(TRGS):%:%.o $(TOOLS)
	$(CC) -o $@ $^ `mysql_config --libs --cflags` -L./lib -lmariadbclient -L/usr/lib64 -lssl -lcrypto -ldl -lpthread

%.o: %.cpp
	$(CC) $(INCLUDE) -c $<  `mysql_config --libs --cflags`

clean:
	rm -f $(OBJS) $(TRGS) $(TOOLS)