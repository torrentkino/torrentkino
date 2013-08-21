#include /usr/share/hardening-includes/hardening.make

CODENAME = masala-cli
export CC = gcc
export CFLAGS = -O2 -std=gnu99
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DMASALA
#CFLAGS += $(shell dpkg-buildflags --get CFLAGS)
#CFLAGS += $(HARDENING_CFLAGS)
#CFLAGS += -g -DDEBUG
export POST_LINKING =
#POST_LINKING += -lpolarssl
#POST_LINKING += $(shell dpkg-buildflags --get LDFLAGS)
#POST_LINKING += $(HARDENING_LDFLAGS)
SUBDIRS =
OBJS=masala-cli.o str.o malloc.o file.o ben.o list.o fail.o

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
