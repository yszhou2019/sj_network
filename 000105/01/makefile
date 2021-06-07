TARGET = test
CC = gcc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

$(TARGET):$(OBJS)
	$(CC) -static -o $@ $^

%.o: %.c
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
