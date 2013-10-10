CODENAME = tk
export CC = gcc
export CFLAGS = -O2 -std=gnu99
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DTORRENTKINO
#CFLAGS += -g -DDEBUG
export LDFLAGS =
OBJS=tkcli.o str.o malloc.o file.o ben.o list.o fail.o

.PHONY: all clean install

all: $(CODENAME)

%.o : ../src/%.c ../src/%.h
	$(CC) $(CFLAGS) -c $<

$(CODENAME): $(OBJS)
	$(CC) $(OBJS) -o $(CODENAME) $(LDFLAGS)

clean:
	rm -f *.o $(CODENAME)

install:
	strip $(CODENAME)
	cp $(CODENAME) $(DESTDIR)/usr/bin/
