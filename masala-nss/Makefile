export CC = gcc
export CFLAGS = -std=gnu99 -Wall -Wwrite-strings -pedantic -fPIC -DPIC -DOPENSSL
export POST_LINKING = -lcrypto
export OPTIMISATION = -O2

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS) libnss_masala.so.2

nss-masala.o: ../src/nss-masala.c ../src/nss-masala.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/nss-masala.c -o nss-masala.o

sha1.o: ../src/sha1.c ../src/sha1.h
	$(CC) $(CFLAGS) $(OPTIMISATION) -c ../src/sha1.c

libnss_masala.so.2: nss-masala.o sha1.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,libnss_masala.so.2 nss-masala.o sha1.o -o libnss_masala.so.2 $(POST_LINKING)

clean:
	rm -f *.o libnss_masala.so.2

install:
	strip libnss_masala.so.2
