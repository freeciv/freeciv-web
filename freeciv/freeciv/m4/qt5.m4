# Detect Qt5 headers and libraries and set flag variables

AC_ARG_VAR([MOCCMD], [QT 5 moc command (autodetected it if not set)])

AC_DEFUN([FC_QT5],
[
  if test "x$fc_qt5_usable" = "x" ; then
    case $host_os in 
    darwin*) FC_QT5_DARWIN;;
    *) FC_QT5_GENERIC;;
    esac
  fi
])
 

AC_DEFUN([FC_QT5_GENERIC],
[
  AC_LANG_PUSH([C++])

  AC_MSG_CHECKING([Qt5 headers])

  AC_ARG_WITH([qt5-includes],
    AS_HELP_STRING([--with-qt5-includes], [path to Qt5 includes]),
              [FC_QT5_COMPILETEST([$withval])],
[POTENTIAL_PATHS="/usr/include /usr/include/qt5 /usr/include/qt"

  # search multiarch paths too (if the multiarch tuple can be found)
  FC_MULTIARCH_TUPLE()
  AS_IF(test "x$MULTIARCH_TUPLE" != "x",
    POTENTIAL_PATHS="$POTENTIAL_PATHS /usr/include/$MULTIARCH_TUPLE/qt5")

  dnl First test without any additional include paths to see if it works already
  FC_QT5_COMPILETEST
  for TEST_PATH in $POTENTIAL_PATHS
  do
    if test "x$qt5_headers" != "xyes" ; then
      FC_QT5_COMPILETEST($TEST_PATH)
    fi
  done])

  if test "x$qt5_headers" = "xyes" ; then
    AC_MSG_RESULT([found])

    AC_MSG_CHECKING([Qt5 libraries])
    AC_ARG_WITH([qt5-libs],
      AS_HELP_STRING([--with-qt5-libs], [path to Qt5 libraries]),
                [FC_QT5_LINKTEST([$withval])],
[POTENTIAL_PATHS="/usr/lib/qt5 /usr/lib/qt"

    # search multiarch paths too (if the multiarch tuple can be found)
    FC_MULTIARCH_TUPLE()
    AS_IF(test "x$MULTIARCH_TUPLE" != "x",
      POTENTIAL_PATHS="$POTENTIAL_PATHS /usr/lib/$MULTIARCH_TUPLE/qt5")

    dnl First test without any additional library paths to see if it works already
    FC_QT5_LINKTEST
    for TEST_PATH in $POTENTIAL_PATHS
    do
      if test "x$qt5_libs" != "xyes" ; then
        FC_QT5_LINKTEST($TEST_PATH)
      fi
    done])
  fi

  if test "x$qt5_libs" = "xyes" ; then
    AC_MSG_RESULT([found])
    AC_MSG_CHECKING([for Qt >= 5.2])
    FC_QT52_CHECK
  fi

  AC_LANG_POP([C++])
  if test "x$fc_qt52" = "xyes" ; then
    AC_MSG_RESULT([ok])
    FC_QT5_VALIDATE_MOC([fc_qt5_usable=true], [fc_qt5_usable=false])
  else
    AC_MSG_RESULT([not found])
    fc_qt5_usable=false
  fi
])

dnl Test if Qt headers are found from given path
AC_DEFUN([FC_QT5_COMPILETEST],
[
  if test "x$1" != "x" ; then
    CPPFADD=" -I$1 -I$1/QtCore -I$1/QtGui -I$1/QtWidgets"
  else
    CPPFADD=""
  fi

  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="${CPPFLAGS}${CPPFADD}"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <QApplication>]],
[[int a; QApplication app(a, 0);]])],
    [qt5_headers=yes
     FC_QT5_CPPFLAGS="${FC_QT5_CPPFLAGS}${CPPFADD}"],
    [CXXFLAGS_SAVE="${CXXFLAGS}"
     CXXFLAGS="${CXXFLAGS} -fPIC"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <QApplication>]],
[[int a; QApplication app(a, 0);]])],
      [qt5_headers=yes
       FC_QT5_CPPFLAGS="${FC_QT5_CPPFLAGS}${CPPFADD}"
       FC_QT5_CXXFLAGS="${FC_QT5_CXXFLAGS} -fPIC"])
     CXXFLAGS="${CXXFLAGS_SAVE}"])

  CPPFLAGS="$CPPFLAGS_SAVE"
])

dnl Check if the included version of Qt is at least Qt5.2
dnl Output: fc_qt52=yes|no
AC_DEFUN([FC_QT52_CHECK],
[
  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $FC_QT5_CPPFLAGS"
  CXXFLAGS_SAVE="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $FC_QT5_CXXFLAGS"
  LIBS_SAVE="$LIBS"
  LIBS="${LIBS}${LIBSADD}"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <QtCore>]],[[
      #if QT_VERSION < 0x050200
        fail
      #endif
    ]])],
    [fc_qt52=yes],
    [fc_qt52=no])
  LIBS="$LIBS_SAVE"
  CPPFLAGS="${CPPFLAGS_SAVE}"
  CXXFLAGS="${CXXFLAGS_SAVE}"
])


dnl Test Qt application linking with current flags
AC_DEFUN([FC_QT5_LINKTEST],
[
  if test "x$1" != "x" ; then
    LIBSADD=" -L$1 -lQt5Gui -lQt5Core -lQt5Widgets"
  else
    LIBSADD=" -lQt5Gui -lQt5Core -lQt5Widgets"
  fi

  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $FC_QT5_CPPFLAGS"
  CXXFLAGS_SAVE="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $FC_QT5_CXXFLAGS"
  LIBS_SAVE="$LIBS"
  LIBS="${LIBS}${LIBSADD}"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <QApplication>]],
[[int a; QApplication app(a, 0);]])],
[qt5_libs=yes
 FC_QT5_LIBS="${FC_QT5_LIBS}${LIBSADD}"])
 LIBS="$LIBS_SAVE"
 CPPFLAGS="${CPPFLAGS_SAVE}"
 CXXFLAGS="${CXXFLAGS_SAVE}"
])

dnl If $1 is Qt 5's moc command then $2 else $3
AC_DEFUN([FC_QT5_IF_QT5_MOC],
  AS_IF([test "`$1 -v 2<&1 | grep -o 'Qt [[[0-9]]]\+'`" = "Qt 5" ||
         test "`$1 -v 2<&1 | grep -o 'moc [[[0-9]]]\+'`" = "moc 5" ],
    [$2], [$3]))

dnl Set MOCCMD to $1 if it is the Qt 5 "moc". If not run $2 parameter.
AC_DEFUN([FC_QT5_TRY_MOC],
  [FC_QT5_IF_QT5_MOC([$1], [MOCCMD="$1"], [$2])])


dnl If a usable moc command is found do $1 else do $2
AC_DEFUN([FC_QT5_VALIDATE_MOC], [
  AC_MSG_CHECKING([the Qt 5 moc command])

  dnl Try to find a Qt 5 'moc' if MOCCMD isn't set.
  dnl Test that the supplied MOCCMD is a Qt 5 'moc' if it is set.
  AS_IF([test "x$MOCCMD" = "x"],
    [FC_QT5_TRY_MOC([moc],
      [FC_QT5_TRY_MOC([qtchooser -run-tool=moc -qt=5],
        [MOCCMD=""])])],
    [FC_QT5_TRY_MOC([$MOCCMD],
      AC_MSG_ERROR(["MOCCMD set to a bad value ($MOCCMD)"]))])

  dnl If no Qt 5 'moc' was found do $2, else do $1
  AS_IF([test "x$MOCCMD" = "x"],
    [AC_MSG_RESULT([not found]); $2],
    [AC_MSG_RESULT([$MOCCMD]); $1])])

dnl Put the multiarch tuple of the host architecture in $MULTIARCH_TUPLE if it
dnl can be found.
AC_DEFUN([FC_MULTIARCH_TUPLE], [
  # GCC has the --print-multiarch option
  AS_IF(test "x$GCC" = "xyes", [
    # unless it is an old version
    AS_IF(($CC --print-multiarch >/dev/null 2>/dev/null),
      [MULTIARCH_TUPLE=`$CC --print-multiarch`],
      [MULTIARCH_TUPLE="$host_cpu-$host_os"])],
    [MULTIARCH_TUPLE="$host_cpu-$host_os"])])
