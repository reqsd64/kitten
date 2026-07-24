CC ?= cc
CFLAGS ?= -O2
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

prefix ?= /usr/local
PREFIX ?= $(prefix)
DESTDIR ?=
bindir = $(PREFIX)/bin
mandir = $(PREFIX)/share/man
infodir = $(PREFIX)/share/info

INSTALL ?= install
MAKEINFO ?= makeinfo
RM ?= rm -f

WARNINGS ?= -Wall -Wextra -Wpedantic
SOURCES = src/main.c src/walk.c src/output.c
HEADERS = src/kitten.h

all: kitten

kitten: $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -std=c99 -o $@ $(SOURCES) $(LDFLAGS) $(LDLIBS)

test: kitten
	sh test/cli.sh ./kitten

info: doc/kitten.info

doc/kitten.info: doc/kitten.texi
	$(MAKEINFO) -o $@ doc/kitten.texi

install: kitten
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 kitten $(DESTDIR)$(bindir)/kitten
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 doc/kitten.1 $(DESTDIR)$(mandir)/man1/kitten.1

install-info: info
	$(INSTALL) -d $(DESTDIR)$(infodir)
	$(INSTALL) -m 644 doc/kitten.info $(DESTDIR)$(infodir)/kitten.info

uninstall:
	$(RM) $(DESTDIR)$(bindir)/kitten
	$(RM) $(DESTDIR)$(mandir)/man1/kitten.1
	$(RM) $(DESTDIR)$(infodir)/kitten.info

clean:
	$(RM) kitten doc/kitten.info

.PHONY: all test info install install-info uninstall clean
