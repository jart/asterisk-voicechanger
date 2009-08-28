#
# Asterisk Voice Changer Makefile
#
# Copyright (C) 2005-2009 J.A. Roberts Tunney
#
# J.A. Roberts Tunney <jtunney@lobstertech.com>
#
# This program is free software, distributed under the terms of
# the GNU General Public License
#

CC          = gcc

PREFIX      = /usr
MODULES_DIR = $(PREFIX)/lib/asterisk/modules

MODS        = app_voicechanger.so
CFLAGS      = -O -g -D_GNU_SOURCE -shared -fpic
LDFLAGS     = -lsoundtouch4c

include Makefile.inc
