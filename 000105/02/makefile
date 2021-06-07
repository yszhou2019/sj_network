TARGET = test
CC = g++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

$(TARGET):$(OBJS)
	$(CC) -static -o $@ $^

%.o: %.cpp
	$(CC) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
