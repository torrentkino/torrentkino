SUBDIRS = tk6 tk4

.PHONY : all clean install docs sync debian ubuntu $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	for dir in $(SUBDIRS); do \
		$(MAKE) install -C $$dir; \
	done

docs:
	./bin/docs.sh torrentkino

sync:
	./bin/sync.sh torrentkino

debian:
	./bin/debian.sh torrentkino

ubuntu:
	./bin/debian.sh torrentkino

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done
