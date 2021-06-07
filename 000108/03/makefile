CC	=gcc


SRCS =$(wildcard *.c)\
	../../common/my_daemon.c
OBJS =$(patsubst %.c, %.o, $(SRCS))
TRGS =test3-1 test3-2
INC  =-I../../common
all:$(TRGS)

# 可执行文件依赖于.o
$(TRGS):%:%.o
	$(CC) $(CFLAGS) -o $@ $^

# .o依赖于源文件
%.o: %.c
	$(CC) $(INC) $(CFLAGS) -c $^

install:$(TRGS)
	cp $(TRGS) ..

uninstall:
	@for tar in $(TRGS); do \
		rm -f ../$$tar; \
	done

clean:
	rm -f $(OBJS) $(TRGS)
