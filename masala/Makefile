#include /usr/share/hardening-includes/hardening.make

export CC = gcc
export CFLAGS = -O2 -std=gnu99
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DMASALA
#CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
#CFLAGS += $(HARDENING_CFLAGS)
export POST_LINKING = -lpthread -lpolarssl
#POST_LINKING += $(shell dpkg-buildflags --get LDFLAGS)
#POST_LINKING += $(HARDENING_LDFLAGS)
SUBDIRS =
OBJS=aes.o announce.o ben.o bucket.o cache.o conf.o \
	database.o file.o hash.o hex.o list.o \
	log.o lookup.o main.o malloc.o neighboorhood.o \
	node_p2p.o opts.o p2p.o random.o send_p2p.o \
	sha1.o str.o thrd.o time.o udp.o unix.o

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) masala

%.o : ../src/%.c ../src/%.h
	$(CC) $(CFLAGS) -c $<

masala: $(OBJS)
	$(CC) $(OBJS) -o masala $(POST_LINKING)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
	rm -f *.o masala

install:
	strip masala
