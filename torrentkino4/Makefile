export CODENAME = torrentkino4
export CC = gcc

export CFLAGS_MIN = -O2 -std=gnu99
CFLAGS_MIN += -DTORRENTKINO -DIPV4
CFLAGS_MIN += -Wall -Wwrite-strings
#CFLAGS_MIN += -g

export LDFLAGS = -lpthread

OBJS = ben.o bucket.o cache.o conf.o fail.o \
	file.o hash.o hex.o info_hash.o list.o \
	log.o lookup.o malloc.o torrentkino.o \
	neighbourhood.o node_udp.o opts.o p2p.o random.o send_udp.o \
	sha1.o str.o thrd.o time.o token.o transaction.o \
	udp.o unix.o worker.o

# PolarSSL Support
#CFLAGS_MIN += -DPOLARSSL
#LDFLAGS += -lpolarssl
#OBJS += aes.o

export CFLAGS_EXT = $(CFLAGS_MIN) -pedantic

.PHONY: all clean install

all: $(CODENAME)

sha1-linus.o : ../src/sha1-linus.c ../src/sha1-linus.h
	$(CC) $(CFLAGS_MIN) -c ../src/sha1-linus.c

%.o : ../src/%.c ../src/%.h
	$(CC) $(CFLAGS_EXT) -c $<

$(CODENAME): $(OBJS) sha1-linus.o
	$(CC) $(OBJS) sha1-linus.o -o $(CODENAME) $(LDFLAGS)

clean:
	rm -f *.o $(CODENAME)

install:
	strip $(CODENAME)
	cp $(CODENAME) $(DESTDIR)/usr/bin/
