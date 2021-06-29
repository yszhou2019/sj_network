CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TRGS = $(patsubst %.cpp, %, $(SRCS))

INCLUDE = -I../../include
ETC = ../../common/my_etc.cpp
TOOLS = my_etc.o



all:$(TOOLS) $(TRGS)

$(TOOLS):
	$(CC) $(INCLUDE) -c $(ETC) `mysql_config --libs --cflags`

$(TRGS):%:%.o $(TOOLS)
	$(CC) -o $@ $^ `mysql_config --libs --cflags` -L/usr/lib64 -lmariadbclient -lssl -lcrypto -ldl -lpthread
	cp $@ /test/server/ 

%.o: %.cpp
	$(CC) $(INCLUDE) -c $<  `mysql_config --libs --cflags`

clean:
	rm -f $(OBJS) $(TRGS) $(TOOLS)