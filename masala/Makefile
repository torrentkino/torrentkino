#include /usr/share/hardening-includes/hardening.make

export CC = gcc
export CFLAGS = -O2
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DMASALA
#CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
#CFLAGS += $(HARDENING_CFLAGS)
export POST_LINKING = -lpthread -lpolarssl
#POST_LINKING += $(shell dpkg-buildflags --get LDFLAGS)
#POST_LINKING += $(HARDENING_LDFLAGS)
#POST_LINKING += -lpolarssl
SUBDIRS =

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) masala

aes.o: ../src/aes.c ../src/aes.h
	$(CC) $(CFLAGS) -c ../src/aes.c

ben.o: ../src/ben.c ../src/ben.h
	$(CC) $(CFLAGS) -c ../src/ben.c

cache.o: ../src/cache.c ../src/cache.h
	$(CC) $(CFLAGS) -c ../src/cache.c

random.o: ../src/random.c ../src/random.h
	$(CC) $(CFLAGS) -c ../src/random.c

conf.o: ../src/conf.c ../src/conf.h
	$(CC) $(CFLAGS) -c ../src/conf.c

unix.o: ../src/unix.c ../src/unix.h
	$(CC) $(CFLAGS) -c ../src/unix.c

log.o: ../src/log.c ../src/log.h
	$(CC) $(CFLAGS) -c ../src/log.c

file.o: ../src/file.c ../src/file.h
	$(CC) $(CFLAGS) -c ../src/file.c

lookup.o: ../src/lookup.c ../src/lookup.h
	$(CC) $(CFLAGS) -c ../src/lookup.c

hash.o: ../src/hash.c ../src/hash.h
	$(CC) $(CFLAGS) -c ../src/hash.c

hex.o: ../src/hex.c ../src/hex.h
	$(CC) $(CFLAGS) -c ../src/hex.c

list.o: ../src/list.c ../src/list.h
	$(CC) $(CFLAGS) -c ../src/list.c

malloc.o: ../src/malloc.c ../src/malloc.h
	$(CC) $(CFLAGS) -c ../src/malloc.c

opts.o: ../src/opts.c ../src/opts.h
	$(CC) $(CFLAGS) -c ../src/opts.c

str.o: ../src/str.c ../src/str.h
	$(CC) $(CFLAGS) -c ../src/str.c

udp.o: ../src/udp.c ../src/udp.h
	$(CC) $(CFLAGS) -c ../src/udp.c

thrd.o: ../src/thrd.c ../src/thrd.h
	$(CC) $(CFLAGS) -c ../src/thrd.c

main.o: ../src/main.c ../src/main.h
	$(CC) $(CFLAGS) -c ../src/main.c

p2p.o: ../src/p2p.c ../src/p2p.h
	$(CC) $(CFLAGS) -c ../src/p2p.c

node_p2p.o: ../src/node_p2p.c ../src/node_p2p.h
	$(CC) $(CFLAGS) -c ../src/node_p2p.c

bucket.o: ../src/bucket.c ../src/bucket.h
	$(CC) $(CFLAGS) -c ../src/bucket.c

send_p2p.o: ../src/send_p2p.c ../src/send_p2p.h
	$(CC) $(CFLAGS) -c ../src/send_p2p.c

sha1.o: ../src/sha1.c ../src/sha1.h
	$(CC) $(CFLAGS) -c ../src/sha1.c

neighboorhood.o: ../src/neighboorhood.c ../src/neighboorhood.h
	$(CC) $(CFLAGS) -c ../src/neighboorhood.c

time.o: ../src/time.c ../src/time.h
	$(CC) $(CFLAGS) -c ../src/time.c

masala: \
	main.o conf.o unix.o log.o file.o lookup.o hash.o hex.o list.o malloc.o opts.o str.o thrd.o ben.o udp.o random.o send_p2p.o sha1.o node_p2p.o bucket.o neighboorhood.o cache.o aes.o time.o p2p.o
	$(CC) \
	main.o conf.o unix.o log.o file.o lookup.o hash.o hex.o list.o malloc.o opts.o str.o thrd.o ben.o udp.o random.o send_p2p.o sha1.o node_p2p.o bucket.o neighboorhood.o cache.o aes.o time.o p2p.o \
	-o masala $(POST_LINKING)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
	rm -f *.o masala

install:
	strip masala
