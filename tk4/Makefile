export CODENAME = tk4
export CC = gcc

export CFLAGS_MIN = -O2 -std=gnu99
CFLAGS_MIN += -DTORRENTKINO -DIPV4
CFLAGS_MIN += -Wall -Wwrite-strings
#CFLAGS_MIN += -g

export LDFLAGS = -lpthread

OBJS = ben.o bucket.o cache.o conf.o dns.o fail.o \
	file.o hash.o hex.o identity.o  ip.o value.o list.o \
	log.o lookup.o malloc.o torrentkino.o \
	neighbourhood.o node_udp.o p2p.o random.o resolver.o send_udp.o \
	sha1.o str.o thrd.o time.o token.o transaction.o \
	udp.o unix.o worker.o

# PolarSSL Support
#CFLAGS_MIN += -DPOLARSSL
#LDFLAGS += -lpolarssl
#OBJS += aes.o

export CFLAGS_EXT = $(CFLAGS_MIN) -pedantic

.PHONY: all clean install

all: $(CODENAME)

%.o : ../src/p2p/%.c ../src/p2p/%.h
	$(CC) $(CFLAGS_EXT) -c $<

%.o : ../src/dns/%.c ../src/dns/%.h
	$(CC) $(CFLAGS_EXT) -c $<

%.o : ../src/shr/%.c ../src/shr/%.h
	$(CC) $(CFLAGS_EXT) -c $<

sha1-linus.o : ../src/ext/sha1-linus.c ../src/ext/sha1-linus.h
	$(CC) $(CFLAGS_MIN) -c ../src/ext/sha1-linus.c

$(CODENAME): $(OBJS) sha1-linus.o
	$(CC) $(OBJS) sha1-linus.o -o $(CODENAME) $(LDFLAGS)

clean:
	rm -f *.o $(CODENAME)

install:
	strip $(CODENAME)
	mkdir -p $(DESTDIR)/usr/bin/
	cp $(CODENAME) $(DESTDIR)/usr/bin/
