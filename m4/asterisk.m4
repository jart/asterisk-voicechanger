#  -*-Autoconf-*-

AC_DEFUN([AC_PROG_ASTERISK], [

AC_PATH_PROG([ASTERISK], [asterisk], :)

AC_CHECK_HEADER([asterisk/module.h],
        AC_DEFINE(HAS_ASTERISK, 1, Asterisk with headers is installed),
        AC_MSG_ERROR(Please install Asterisk! apt-get install asterisk-dev),
        [#include <asterisk.h>])

AC_MSG_CHECKING(asterisk version)
case `${ASTERISK} -V` in
    Asterisk\ 1.2.*)
        AC_MSG_RESULT(1.2.x)
        AC_MSG_ERROR(Asterisk 1.2.x is not supported anymore sorries)
        ;;
    Asterisk\ 1.4.*)
        AC_MSG_RESULT(1.4.x)
        AC_DEFINE(HAS_ASTERISK_V14, 1, Asterisk 1.4.x installed)
        ;;
    Asterisk\ 1.6.*)
        AC_MSG_RESULT(1.6.x)
        AC_DEFINE(HAS_ASTERISK_V16, 1, Asterisk 1.6.x installed)
        ;;
    *)
        AC_MSG_RESULT(wtf)
        AC_MSG_ERROR(Unrecognized version of Asterisk)
        ;;
esac

AC_MSG_CHECKING(asterisk modules dir)
if test -d /usr/lib/asterisk/modules; then
    AC_MSG_RESULT(/usr/lib/asterisk/modules)
    AST_MOD_DIR="/usr/lib/asterisk/modules"
else
    AC_MSG_ERROR([Asterisk modules directory not found!])
fi
AC_SUBST(AST_MOD_DIR)

])
