#  -*-Autoconf-*-

AC_DEFUN([AC_LIB_SOUNDTOUCH4C], [

AC_CHECK_HEADER([soundtouch4c.h], ,
        AC_MSG_ERROR([Please install libsoundtouch4c: http://bitbucket.org/jart/soundtouch4c]))

AC_CHECK_LIB([soundtouch4c], [SoundTouch_construct],
        AC_DEFINE(HAVE_LIBSOUNDTOUCH4C, 1, [libsoundtouch4c library is installed]),
        AC_MSG_ERROR([Please install libsoundtouch4c: http://bitbucket.org/jart/soundtouch4c]))

])
