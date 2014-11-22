export CODENAME = libnss_tk.so.2
export CC = gcc
export CFLAGS = -O2
CFLAGS += -std=gnu99 -fPIC -DPIC
CFLAGS += -Wall -Wwrite-strings -pedantic
CFLAGS += -DTORRENTKINO -DNSS
export LDFLAGS = -lpthread
OBJS=tknss.o str.o malloc.o list.o fail.o random.o

.PHONY: all clean install

all: $(CODENAME)

%.o : ../src/nss/%.c ../src/nss/%.h
	$(CC) $(CFLAGS) -c $<

%.o : ../src/shr/%.c ../src/shr/%.h
	$(CC) $(CFLAGS) -c $<

$(CODENAME): $(OBJS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(CODENAME) $(OBJS) -o $(CODENAME) $(LDFLAGS)

clean:
	rm -f *.o $(CODENAME)

install:
	strip $(CODENAME)
	cp $(CODENAME) $(DESTDIR)/lib/
