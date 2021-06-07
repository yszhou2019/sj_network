TARGET = test2
CC = gcc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

$(TARGET):test2.o libtest1.a
	$(CC) $^ -o $@ 

static: test1.o
	ar rcs libtest1.a $<
#libtest1.a:test1.o
#	ar rcs $@ $^

%.o: %.c
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TARGET) libtest1.a
