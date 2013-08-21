#include /usr/share/hardening-includes/hardening.make

CODENAME = masala
export CC = gcc
export CFLAGS = -O2 -std=gnu99
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DMASALA
#CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
#CFLAGS += $(HARDENING_CFLAGS)
#CFLAGS += -g -DDEBUG
export POST_LINKING = -lpthread
POST_LINKING += -lpolarssl
#POST_LINKING += $(shell dpkg-buildflags --get LDFLAGS)
#POST_LINKING += $(HARDENING_LDFLAGS)
SUBDIRS =
OBJS=aes.o ben.o bucket.o cache.o conf.o fail.o \
	file.o hash.o hex.o info_hash.o list.o \
	log.o lookup.o malloc.o masala-srv.o \
	neighbourhood.o node.o opts.o p2p.o random.o send_p2p.o \
	sha1.o str.o thrd.o time.o token.o transaction.o \
	udp.o unix.o

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) $(CODENAME)

%.o : ../src/%.c ../src/%.h
	$(CC) $(CFLAGS) -c $<

$(CODENAME): $(OBJS)
	$(CC) $(OBJS) -o $(CODENAME) $(POST_LINKING)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
	rm -f *.o $(CODENAME)

install:
	strip $(CODENAME)
