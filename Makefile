CC=gcc
CFLAGS=-g -O2 -Wall
LDLIBS=-lpthread
OBJS=main.o connection.o request.o response.o

all: server

server: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@

$(OBJS): server.h

clean:
	rm -rf *.o *~ *.dSYM server