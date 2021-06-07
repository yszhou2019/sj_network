CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TRGS = $(patsubst %.cpp, %, $(SRCS))
FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))
CLIENTS = $(call FILTER_OUT,server, $(TRGS))
SERVERS = $(call FILTER_OUT,client, $(TRGS))

all:$(CLIENTS) $(SERVERS)

# fake:
#   	@echo $(wildcard *client*)
# 	@echo "targets" $(TRGS)
# 	@echo "clients" $(CLIENTS)
# 	@echo "targets" $(TRGS)
# 	@echo "servers" $(SERVERS)

$(CLIENTS):%:%.o
	$(CC) -o $@ $^
	cp $@ /mnt/client/$(addprefix u1752240_, $@)

$(SERVERS):%:%.o
	$(CC) -o $@ $^
	cp $@ ../$(addprefix u1752240_, $@)

# all:$(TRGS)

# $(TRGS):%:%.o
# 	$(CC) -o $@ $^
# 	cp $@ /mnt/client/$(addprefix u1752240_, $@)

%.o: %.cpp
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TRGS)