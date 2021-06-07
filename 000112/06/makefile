CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TRGS = $(patsubst %.cpp, %, $(SRCS))
FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))
CLIENTS = $(call FILTER_OUT,server, $(TRGS))
SERVERS = $(call FILTER_OUT,client, $(TRGS))

INCLUDE = -I../../include/
ETC = ../../common/my_etc.cpp
TOOLS = my_etc.o



all:$(TOOLS) $(CLIENTS) $(SERVERS)

$(TOOLS):
	$(CC) $(INCLUDE) -c $(ETC)

$(CLIENTS):%:%.o $(TOOLS)
	$(CC) -o $@ $^
	cp $@ /mnt/client/$(addprefix u1752240_, $@)

$(SERVERS):%:%.o $(TOOLS)
	$(CC) -o $@ $^
	cp $@ ../$(addprefix u1752240_, $@)

%.o: %.cpp
	$(CC) $(INCLUDE) -c $<

clean:
	rm -f $(OBJS) $(TRGS) $(TOOLS)