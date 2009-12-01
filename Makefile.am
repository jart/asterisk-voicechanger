
SUBDIRS         = 
AUTOMAKE_OPTS   = gnu
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST      = bootstrap.sh

AUX_DIST = \
	config/config.guess \
	config/config.sub \
	config/depcomp \
	config/install-sh \
	config/ltmain.sh \
	config/missing

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure config.h.in \
	config.h.in~ $(AUX_DIST)

AM_CC				= gcc
AM_CFLAGS			= -Wall
AM_LDFLAGS			= -module
AM_LIBTOOLFLAGS			= 
lib_LTLIBRARIES			= app_voicechanger.la

app_voicechanger_la_SOURCES	= app_voicechanger.c
app_voicechanger_la_CFLAGS	= $(AM_CFLAGS) -D AST_MODULE=\"app_voicechanger\"
app_voicechanger_la_LIBADD	= -lsoundtouch4c

# create a symlink in /usr/lib/asterisk/modules to real library only
# if not running distcheck.  there *has* to be a less ugly way to do
# this
install-exec-hook:
	@case "$(abs_builddir)" in \
	  */_build*) \
	    ;; \
	  *) \
	    echo ""; \
	    echo "*** Installing app_voicechanger.so to $(AST_MOD_DIR) ***"; \
	    echo ""; \
	    $(LN_S) -f $(libdir)/app_voicechanger.so $(AST_MOD_DIR)/app_voicechanger.so; \
	    if test -n "$(LDCONFIG)"; then $(LDCONFIG); fi; \
	    ;; \
	esac;

uninstall-hook:
	@case "$(abs_builddir)" in \
	  */_build*) \
	    ;; \
	  *) \
	    rm -rf $(AST_MOD_DIR)/app_voicechanger.so; \
	    if test -n "$(LDCONFIG)"; then $(LDCONFIG); fi; \
	    ;; \
	esac;

# generate ChangeLog using mercurial
dist-hook:
	if test -d $(top_srcdir)/.hg; then \
	    $(MERCURIAL) log --style changelog >$(distdir)/ChangeLog; \
	fi;

upgrade-code:
	$(MERCURIAL) pull
	$(MERCURIAL) update

upgrade: clean upgrade-code all
	@echo "------------------------------------------------------------"
	@echo "You're now up to date!"
	@echo "You might want to run 'sudo make install' now :)	"
	@echo "------------------------------------------------------------"

# controlling asterisk
start: install
	$(ASTERISK) -rx "module load app_voicechanger.so"
stop:
	$(ASTERISK) -rx "module unload app_voicechanger.so"
restart: install stop start
