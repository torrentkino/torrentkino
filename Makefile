SUBDIRS = masala masala-nss

.PHONY : all clean install $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) install -C $$dir; \
	done

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
	rm -f *.changes
	rm -f *.tar.gz
	rm -f *.deb
	rm -f *.dsc
