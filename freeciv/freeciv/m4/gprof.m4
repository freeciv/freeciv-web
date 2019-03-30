AC_DEFUN([FC_GPROF], [
AC_ARG_ENABLE([gprof],
  AS_HELP_STRING([--enable-gprof], [turn on profiling [default=no]]),
[case "${enableval}" in
  yes) enable_gprof=yes ;;
  no)  enable_gprof=no ;;
  *)   AC_MSG_ERROR([bad value ${enableval} for --enable-gprof]) ;;
esac], [enable_gprof=no])

dnl -g is added by AC_PROG_CC if the compiler understands it
if test "x$enable_gprof" = "xyes"; then
  FC_C_FLAGS([-pg -no-pie], [], [EXTRA_DEBUG_CFLAGS])
  FC_LD_FLAGS([-pg], [], [EXTRA_DEBUG_LDFLAGS])
  if test "x$cxx_works" = "xyes" ; then
    FC_CXX_FLAGS([-pg], [], [EXTRA_DEBUG_CXXFLAGS])
  fi
fi
])
