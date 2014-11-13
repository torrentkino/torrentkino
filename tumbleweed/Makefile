export CODENAME = tumbleweed
export CC = gcc
export CFLAGS = -O2 -std=gnu99 -Wall -Wwrite-strings -pedantic
CFLAGS += -DTUMBLEWEED -DIPV6
CFLAGS += -DMAGIC
#CFLAGS += -g
LDFLAGS = -lpthread
LDFLAGS += -lmagic
OBJS = conf.o fail.o file.o hash.o http.o ip.o list.o log.o \
	malloc.o mime.o node_tcp.o response.o \
	send_tcp.o str.o tcp.o thrd.o tumbleweed.o unix.o \
	worker.o

.PHONY: all clean install

all: $(CODENAME)

%.o : ../src/web/%.c ../src/web/%.h
	$(CC) $(CFLAGS) -c $<

%.o : ../src/shr/%.c ../src/shr/%.h
	$(CC) $(CFLAGS) -c $<

$(CODENAME): $(OBJS)
	$(CC) $(OBJS) -o $(CODENAME) $(LDFLAGS)

clean:
	rm -f *.o $(CODENAME)

install:
	strip $(CODENAME)
	cp $(CODENAME) $(DESTDIR)/usr/bin/
