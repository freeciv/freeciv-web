# Freeciv testing support

#
#
AC_DEFUN([FC_TESTMATIC],
[
AC_ARG_ENABLE([testmatic],
  AS_HELP_STRING([--enable-testmatic],
        [Produce logging useful in freeciv testing integration]),
[case "${enableval}" in
      yes) enable_testmatic=yes ;;
      no)  enable_testmatic=no ;;
      *)   AC_MSG_ERROR([bad value ${enableval} for --enable-testmatic]) ;;
esac], [enable_testmatic=no])

  if test x$enable_testmatic = xyes ; then
    AC_DEFINE([FREECIV_TESTMATIC], [1], [Testmatic integration enabled])
  fi
])
