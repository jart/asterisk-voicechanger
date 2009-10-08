# -*- autoconf -*-

AC_PREREQ(2.59)
AC_INIT(asterisk-voicechanger, 0.8b, [jtunney@lobstertech.com])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(m4)
AM_INIT_AUTOMAKE(asterisk-voicechanger,0.8b)
AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([app_voicechanger.c])
AC_LANG(C)
AC_GNU_SOURCE
AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_ASTERISK
AC_LIB_SOUNDTOUCH4C

# stuff
AC_PATH_PROG([LDCONFIG], [ldconfig], :)
AC_SUBST(LDCONFIG)
AC_PATH_PROG([MERCURIAL], [hg], :)
AC_SUBST(MERCURIAL)

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
