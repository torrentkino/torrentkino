# Torrentkino (IPv6)
SUBDIRS = tk6 nss6

# Torrentkino (IPv4)
SUBDIRS += tk4 nss4

# Tumbleweed (Simple Webserver)
SUBDIRS += tumbleweed

.PHONY : all clean install docs sync debian ubuntu $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	mkdir -p $(DESTDIR)/usr/share/tk
	mkdir -p $(DESTDIR)/var/lib/tk
	mkdir -p $(DESTDIR)/var/run/tk
	cp -f systemd/tk4.service $(DESTDIR)/lib/systemd/system/
	cp -f systemd/tk6.service $(DESTDIR)/lib/systemd/system/
	for dir in $(SUBDIRS); do \
		$(MAKE) install -C $$dir; \
	done

docs:
	./bin/docs.sh

debian:
	./bin/debian.sh

ubuntu:
	./bin/debian.sh

indent:
	./bin/indent.sh

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
