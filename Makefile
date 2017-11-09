PROGNAME = json_parser
PREFIX   = ${HOME}/.local/bin
CC       = gcc
CFLAGS  += $(shell pkg-config --cflags json-c libcurl)
CFLAGS  += -Wall -Wextra -pedantic -ggdb
LDFLAGS += $(shell pkg-config --libs json-c libcurl)

dev: $(PROGNAME) gtr.h
	$(PWD)/$(PROGNAME) t
install: $(PROGNAME)
	install -d $(PREFIX)
	install $< $(PREFIX)
	make clean

cu: gtr.h


uninstall:
	rm $(PREFIX)/$(PROGNAME)

.PHONY: clean

clean: 

