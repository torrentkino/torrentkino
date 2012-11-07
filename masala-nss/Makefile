#DEB_BUILD_HARDENING_PIE:=0
#include /usr/share/hardening-includes/hardening.make

export CC = gcc
export CFLAGS = -O2
CFLAGS += -std=gnu99 -fPIC -DPIC
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DNSS
#CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
#CFLAGS += $(HARDENING_CFLAGS)
export POST_LINKING = -lpthread -lpolarssl
#POST_LINKING += $(shell dpkg-buildflags --get LDFLAGS)
#POST_LINKING += $(HARDENING_LDFLAGS)

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) libnss_masala.so.2

nss-masala.o: ../src/nss-masala.c ../src/nss-masala.h
	$(CC) $(CFLAGS) -c ../src/nss-masala.c -o nss-masala.o

sha1.o: ../src/sha1.c ../src/sha1.h
	$(CC) $(CFLAGS) -c ../src/sha1.c

libnss_masala.so.2: nss-masala.o sha1.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,libnss_masala.so.2 nss-masala.o sha1.o -o libnss_masala.so.2 $(POST_LINKING)

clean:
	rm -f *.o libnss_masala.so.2

install:
	strip libnss_masala.so.2
