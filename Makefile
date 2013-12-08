SUBDIRS = torrentkino6 torrentkino4 tknss tkcli tumbleweed

.PHONY : all clean install docs debian ubuntu $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) install -C $$dir; \
	done

docs:
	./bin/manpage.sh

debian:
	./bin/debian.sh

ubuntu:
	./bin/debian.sh

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
	rm -f *.changes
	rm -f *.tar.gz
	rm -f *.deb
	rm -f *.dsc
