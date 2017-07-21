PROGNAME = json_parser
PREFIX   = ${HOME}/.local/bin
CC       = gcc
CFLAGS  += $(shell pkg-config --cflags json-c)
CFLAGS  += -Wall -Wextra
LDFLAGS += $(shell pkg-config --libs json-c)

install: $(PROGNAME)
	install -d $(PREFIX)
	install $< $(PREFIX)
	make clean

uninstall:
	rm $(PREFIX)/$(PROGNAME)

.PHONY: clean

clean: 

