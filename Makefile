#
# Asterisk Voice Changer Makefile
#
# Copyright (C) 2005-2006 J.A. Roberts Tunney
#
# J.A. Roberts Tunney <jtunney@lobstertech.com>
#
# This program is free software, distributed under the terms of
# the GNU General Public License
#

.EXPORT_ALL_VARIABLES:

MODS=app_voicechangedial.so

CC=cc
CFLAGS=-O -g -D_GNU_SOURCE -shared -fpic

PREFIX=/usr
MODULES_DIR=$(PREFIX)/lib/asterisk/modules

NAME=$(shell basename `pwd`)
OSARCH=$(shell uname -s)

ifeq (${OSARCH},Darwin)
    SOLINK=-dynamic -bundle -undefined suppress -force_flat_namespace
else
    SOLINK=-fpic -shared -Xlinker -x -pthread
endif
ifeq (${OSARCH},SunOS)
    SOLINK=-shared -fpic -L/usr/local/ssl/lib
endif

SOLINK+=-lsoundtouch4c

all: depend $(MODS)

install: all
	for x in $(MODS); do install -m 755 $$x $(MODULES_DIR) ; done

installbin:
	for x in $(MODS); do install -m 755 $$x $(MODULES_DIR) ; done

uninstall:
	for x in $(MODS); do rm -f $(MODULES_DIR)/$$x ; done

clean:
	rm -f *.so *.o .depend $(NAME).tar.gz

objclean:
	rm -f *.o .depend $(NAME).tar.gz

dist: clean
	tar -C .. -cvzf /tmp/$(NAME).tar.gz $(NAME)
	mv /tmp/$(NAME).tar.gz .

start: install
	for x in $(MODS); do asterisk -rx "load $$x" ; done

stop:
	for x in $(MODS); do asterisk -rx "unload $$x" ; done

restart: all stop start

%.so : %.o
	$(CC) $(SOLINK) -o $@ $<

ifneq ($(wildcard .depend),)
include .depend
endif

depend: .depend

.depend:
	./mkdep $(CFLAGS) $(shell ls *.c)
