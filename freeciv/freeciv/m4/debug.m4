AC_DEFUN([FC_DEBUG], [
AC_ARG_ENABLE(debug,
[  --enable-debug[[=no/some/yes]]  turn on debugging [[default=some]]],
[case "${enableval}" in   
  yes) enable_debug=yes ;;
  some) enable_debug=some ;;
  no)  enable_debug=no ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac], [enable_debug=some])

dnl -g is added by AC_PROG_CC if the compiler understands it
if test "x$enable_debug" = "xyes"; then
  AC_DEFINE([DEBUG], [1], [Extra debugging support])
  FC_C_FLAGS([-Werror], [], [EXTRA_DEBUG_CFLAGS])
  FC_CXX_FLAGS([-Werror], [], [EXTRA_DEBUG_CXXFLAGS])
else
  if test "x$enable_debug" = "xno"; then
    AC_DEFINE([NDEBUG], [1], [No debugging support at all])
    FC_C_FLAGS([-O3 -fomit-frame-pointer], [], [EXTRA_DEBUG_CFLAGS])
    FC_CXX_FLAGS([-O3 -fomit-frame-pointer], [], [EXTRA_DEBUG_CXXFLAGS])
  fi
fi
])
