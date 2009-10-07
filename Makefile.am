
EXTRA_DIST      = bootstrap.sh
SUBDIRS         = 
AUTOMAKE_OPTS   = gnu
ACLOCAL_AMFLAGS = -I m4
#libdir          = $(AST_MOD_DIR)

AUX_DIST = \
	config/config.guess \
	config/config.sub \
	config/depcomp \
	config/install-sh \
	config/ltmain.sh \
	config/missing

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure config.h.in \
	$(AUX_DIST)

AM_CC				= gcc
AM_CFLAGS			= -Wall
AM_LDFLAGS			= -module
AM_LIBTOOLFLAGS			= 
lib_LTLIBRARIES			= app_voicechanger.la

app_voicechanger_la_SOURCES	= app_voicechanger.c
app_voicechanger_la_CFLAGS	= $(AM_CFLAGS) -D AST_MODULE=\"app_voicechanger\"
app_voicechanger_la_LIBADD	= -lsoundtouch4c

# create a symlink in /usr/lib/asterisk/modules to real library
install-exec-hook:
	ln -sf $(libdir)/app_voicechanger.so $(AST_MOD_DIR)/app_voicechanger.so
	ldconfig

# generate ChangeLog using mercurial
dist-hook:
	hg log --style changelog >$(distdir)/ChangeLog

upgrade-code:
	hg pull
	hg update

upgrade: clean upgrade-code all
	@echo "------------------------------------------------------------"
	@echo "You're now up to date!"
	@echo "You might want to run 'sudo make install' now :)	"
	@echo "------------------------------------------------------------"

# controlling asterisk
start: install
	asterisk -rx "module load app_voicechanger.so"
stop:
	asterisk -rx "module unload app_voicechanger.so"
restart: install stop start
