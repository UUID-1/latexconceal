.POSIX:

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

CC = c99
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -DNDEBUG
CFLAGS = $(CPPFLAGS) -Wall -Wpedantic -Os -s

SRC = \
      main.c \
      misc.c \
      rules.c \
      restore.c \
      strmap.c \
      util.c \
      vec.c

OBJ = $(SRC:.c=.o)

all: unitex

unitex: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: strmap.h util.h vec.h misc.h rules.h restore.h
misc.o: strmap.h util.h vec.h misc.h
parserules.o: strmap.h util.h vec.h misc.h rules.h
restore.o: strmap.h vec.h misc.h restore.h
strmap.o: strmap.h util.h
util.o: util.h
vec.o: util.h vec.h vec.c.tmpl

vec.h: vec.h.tmpl
	touch -r $< $@
 
.PHONY: install
install: unitex
	mkdir -p $(BINDIR)
	cp -f unitex $(BINDIR)
	chmod 755 $(BINDIR)/unitex

.PHONY: uninstall
uninstall:
	rm -f $(BINDIR)/unitex

.PHONY: clean
clean:
	rm -fr *.o unitex
