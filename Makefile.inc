#
# Asterisk Module Makefile Include
#
# Copyright (C) 2005-2007 J.A. Roberts Tunney
#
# J.A. Roberts Tunney <jtunney@lobstertech.com>
#
# This program is free software, distributed under the terms of
# the GNU General Public License
#

NAME        = $(shell basename `pwd`)
OSARCH      = $(shell uname -s)
ASTVERSION  = $(shell /usr/sbin/asterisk -V | cut -c 10-12)

ifeq (${OSARCH},Darwin)
    LDFLAGS+=-dynamic -bundle -undefined suppress -force_flat_namespace
else
    LDFLAGS+=-fpic -shared -Xlinker -x -pthread
endif
ifeq (${OSARCH},SunOS)
    LDFLAGS+=-shared -fpic -L/usr/local/ssl/lib
endif
ifeq (${ASTVERSION},1.2)
    CFLAGS+=-D_ASTERISK_1_2_
endif

all: sanity depend $(MODS)

sanity:
	./sanity.sh

install: all
	for x in $(MODS); do install -m 755 $$x $(MODULES_DIR) ; done

installbin:
	for x in $(MODS); do install -m 755 $$x $(MODULES_DIR) ; done

uninstall:
	for x in $(MODS); do rm -f $(MODULES_DIR)/$$x ; done

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

%.so: %.o
	$(CC) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.so *.o .depend $(NAME).tar.gz

ifneq ($(wildcard .depend),)
    include .depend
endif

depend: .depend

.depend: *.[ch]
	$(CC) $(CFLAGS) -MM $(wildcard *.c) >$@
