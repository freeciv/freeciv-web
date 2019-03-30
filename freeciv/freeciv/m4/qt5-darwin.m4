# Detect Qt5 headers and libraries and set flag variables for Mac OS X 10.10+

AC_DEFUN([FC_QT5_DARWIN],
[
  AC_ARG_WITH([qt5_framework_bin],
    AS_HELP_STRING([--with-qt5-framework-bin], [path to binares of Qt5 framework (MacOS X, autodetected if wasn't specified)]))

  AC_CHECK_PROG([QTPATHS], [qtpaths], [qtpaths], [no])

  if test "x$QTPATHS" != "xno" ; then

    AC_MSG_CHECKING([Qt5 framework])

    if test "x$qt5_framework_bin" = "x" ; then
      qt5_framework_bin="$($QTPATHS --binaries-dir)"
    fi
    qt5_path="$($QTPATHS --install-prefix)"

    if test "x$qt5_path" != "x" ; then
      AC_LANG_PUSH([C++])
      FC_QT5_DARWIN_COMPILETEST([$qt5_path])
      if test "x$qt5_headers" = "xyes" ; then
        FC_QT5_DARWIN_LINKTEST([$qt5_path])
      else
        fc_qt5_usable=false
      fi
      AC_LANG_POP([C++])

      if test "x$qt5_libs" = "xyes" ; then
        MOCCMD="$qt5_framework_bin/moc"
        AS_IF([test -x $MOCCMD], [fc_qt5_usable=true], [fc_qt5_usable=false])
      else
        fc_qt5_usable=false
      fi
    fi

    if test "x$fc_qt5_usable" = "xtrue" ; then
      AC_MSG_RESULT([found])
    else
      AC_MSG_RESULT([not found])
    fi
  fi
])

dnl Test if Qt5 headers are found from given path
AC_DEFUN([FC_QT5_DARWIN_COMPILETEST],
[
  CPPFADD=" -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I$1/lib/QtWidgets.framework/Versions/5/Headers -I$1/lib/QtGui.framework/Versions/5/Headers -I$1/lib/QtCore.framework/Versions/5/Headers -I. -I$1/mkspecs/macx-clang -F$1/lib "

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

dnl Test Qt application linking with current flags
AC_DEFUN([FC_QT5_DARWIN_LINKTEST],
[
  LIBSADD=" -F$1/lib -framework QtWidgets -framework QtGui -framework QtCore -framework DiskArbitration -framework IOKit -framework OpenGL -framework AGL"

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
