SUBDIRS = torrentkino6 torrentkino4 tknss tkcli tumbleweed

.PHONY : all clean install manpage $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) install -C $$dir; \
	done

manpage:
	ronn < docs/torrentkino.md > debian/docs/torrentkino6.1
	ronn < docs/torrentkino.md > debian/docs/torrentkino4.1
	ronn < docs/torrentkino.md > debian/docs/tk.1
	ronn < docs/tumbleweed.md > debian/docs/tumbleweed.1

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
	rm -f *.changes
	rm -f *.tar.gz
	rm -f *.deb
	rm -f *.dsc
