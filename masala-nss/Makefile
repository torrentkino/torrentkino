#DEB_BUILD_HARDENING_PIE:=0
#include /usr/share/hardening-includes/hardening.make

export CC = gcc
export CFLAGS = -O2
CFLAGS += -std=gnu99 -fPIC -DPIC
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DNSS
#CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
#CFLAGS += $(HARDENING_CFLAGS)
export POST_LINKING = -lpthread
#POST_LINKING += $(shell dpkg-buildflags --get LDFLAGS)
#POST_LINKING += $(HARDENING_LDFLAGS)
OBJS=masala-nss.o

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) libnss_masala.so.2

%.o : ../src/%.c ../src/%.h
	$(CC) $(CFLAGS) -c $<

libnss_masala.so.2: $(OBJS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,libnss_masala.so.2 $(OBJS) -o libnss_masala.so.2 $(POST_LINKING)

clean:
	rm -f *.o libnss_masala.so.2

install:
	strip libnss_masala.so.2
