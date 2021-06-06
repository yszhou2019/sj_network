CC = g++

SRCS = proxy.cpp client.cpp
OBJS = $(SRCS:.cpp=.o)
TRGS = $(patsubst %.cpp, %, $(SRCS))

INCLUDE = -I../../include
ETC = ../../common/my_etc.cpp
TOOLS = my_etc.o



all:$(TOOLS) $(TRGS)

$(TOOLS):
	$(CC) $(INCLUDE) -c $(ETC)

$(TRGS):%:%.o $(TOOLS)
	$(CC) -o $@ $^

%.o: %.cpp
	$(CC) $(INCLUDE) -c $<

clean:
	rm -f $(OBJS) $(TRGS) $(TOOLS)