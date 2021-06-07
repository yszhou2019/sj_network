CC	=gcc


SRCS =test8.cpp ../../common/my_daemon.c
OBJS =test8.o my_daemon.o
TRG1 =test8-1
TRG2 =test8-2
TRGS =$(TRG1) $(TRG2)
INC  =-I../../common -I/usr/include/mysql
all:$(TRGS)

$(TRG1):$(OBJS)
	$(CC) -o $@ $^ -L/usr/lib64/mysql -lmysqlclient 

$(TRG2):$(OBJS)
	$(CC) -o $@ $^ -static -L/usr/lib64 -lmariadbclient -lz -lssl -lcrypto -ldl -lpthread  -I/usr/include/mysql 

$(OBJS):$(SRCS)
	$(CC) $(INC) -c $^

install:$(TRGS)
	cp $(TRGS) ..

uninstall:
	@for tar in $(TRGS); do \
		rm -f ../$$tar; \
	done

clean:
	rm -f $(OBJS) $(TRG1) $(TRG2)
