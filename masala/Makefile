export CC = gcc
export CFLAGS = -Wall -Wwrite-strings -pedantic
CFLAGS += -DMASALA
CFLAGS += -DOPENSSL
export OPTIMISATION = -O2
#SEC_LINKING  = -static
PRE_LINKING  = -L/opt/lib/ -Wl,--as-needed
POST_LINKING = -lpthread
POST_LINKING += -lcrypto
#POST_LINKING += -lpolarssl
SUBDIRS =

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) masala

aes.o: ../src/aes.c ../src/aes.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/aes.c

ben.o: ../src/ben.c ../src/ben.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/ben.c

cache.o: ../src/cache.c ../src/cache.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/cache.c

random.o: ../src/random.c ../src/random.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/random.c

conf.o: ../src/conf.c ../src/conf.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/conf.c

unix.o: ../src/unix.c ../src/unix.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/unix.c

log.o: ../src/log.c ../src/log.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/log.c

file.o: ../src/file.c ../src/file.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/file.c

lookup.o: ../src/lookup.c ../src/lookup.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/lookup.c

hash.o: ../src/hash.c ../src/hash.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/hash.c

hex.o: ../src/hex.c ../src/hex.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/hex.c

list.o: ../src/list.c ../src/list.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/list.c

malloc.o: ../src/malloc.c ../src/malloc.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/malloc.c

opts.o: ../src/opts.c ../src/opts.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/opts.c

str.o: ../src/str.c ../src/str.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/str.c

udp.o: ../src/udp.c ../src/udp.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/udp.c

thrd.o: ../src/thrd.c ../src/thrd.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/thrd.c

main.o: ../src/main.c ../src/main.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/main.c

p2p.o: ../src/p2p.c ../src/p2p.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/p2p.c

node_p2p.o: ../src/node_p2p.c ../src/node_p2p.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/node_p2p.c

bucket.o: ../src/bucket.c ../src/bucket.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/bucket.c

send_p2p.o: ../src/send_p2p.c ../src/send_p2p.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/send_p2p.c

sha1.o: ../src/sha1.c ../src/sha1.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/sha1.c

neighboorhood.o: ../src/neighboorhood.c ../src/neighboorhood.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/neighboorhood.c

time.o: ../src/time.c ../src/time.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/time.c

masala: \
	main.o conf.o unix.o log.o file.o lookup.o hash.o hex.o list.o malloc.o opts.o str.o thrd.o ben.o udp.o random.o send_p2p.o sha1.o node_p2p.o bucket.o neighboorhood.o cache.o aes.o time.o p2p.o
	$(CC) $(SEC_LINKING) $(PRE_LINKING) \
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
