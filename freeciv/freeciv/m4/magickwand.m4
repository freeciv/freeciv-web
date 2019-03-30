# Check for MagickWand (ImageMagick)
#
# FC_CHECK_MAGICKWAND([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])

AC_DEFUN([FC_CHECK_MAGICKWAND],
[
  AC_ARG_WITH([magickwand],
    AS_HELP_STRING([--with-magickwand[=DIR]], [Imagemagick installation directory (optional)]),
[magickwand_dir="$withval"], [magickwand_dir=""])

  WAND_CONFIG_PATH=""
  if test "x$magickwand_dir" = "x" ; then
    PKG_CHECK_MODULES([WAND], [MagickWand], [wand=yes], [wand=no])

    if test "x$wand" = "xno" ; then
      AC_MSG_CHECKING([for MagickWand-config in default path])

      for i in /usr/local /usr;
      do
        test -r $i/bin/${host}-MagicWand-config &&
        WAND_CONFIG_PATH=$i/bin && WAND_CONFIG_NAME=${host}-MagickWand-config &&
        break
      done

      if test x$WAND_CONFIG_PATH = x ; then
        for i in /usr/local /usr;
        do
          test -r $i/bin/MagickWand-config &&
          WAND_CONFIG_PATH=$i/bin && WAND_CONFIG_NAME=MagickWand-config && break
        done
      fi
    fi
  else
    AC_MSG_CHECKING([for MagickWand-config])

    if test -r $magickwand_dir/${host}-MagickWand-config &&
    WAND_CONFIG_PATH=$magickwand_dir && WAND_CONFIG_NAME=${host}-MagickWand-config
    then
      :
    else
      test -r $magickwand_dir/MagickWand-config &&
      WAND_CONFIG_PATH=$magickwand_dir && WAND_CONFIG_NAME=MagickWand-config
    fi
  fi

  if test "x$wand" != "xyes" ; then
    if test -z "$WAND_CONFIG_PATH" ; then
      AC_MSG_RESULT([no])
    else
      AC_MSG_RESULT([found in $WAND_CONFIG_PATH])

      AC_MSG_CHECKING([for $WAND_CONFIG_NAME --cflags])
      WAND_CFLAGS="`$WAND_CONFIG_PATH/$WAND_CONFIG_NAME --cflags`"
      AC_MSG_RESULT([$WAND_CFLAGS])

      AC_MSG_CHECKING([for $WAND_CONFIG_NAME --libs])
      WAND_LIBS="`$WAND_CONFIG_PATH/$WAND_CONFIG_NAME --libs`"
      AC_MSG_RESULT([$WAND_LIBS])

      wand=yes
    fi
  fi

  if test "x$wand" = "xyes" ; then
    dnl
    dnl MagickWand uses -lbz2 (at least on opensuse) - test it
    dnl
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $WAND_CFLAGS"
    LIBS="$WAND_LIBS $LIBS"

    AC_MSG_CHECKING([for all development tools needed for MagickWand])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <wand/magick_wand.h>]],
[MagickWand *mw = NewMagickWand();])], [AC_MSG_RESULT([yes])],
[AC_MSG_RESULT([no])
wand=no
AC_MSG_WARN([MagickWand deactivated due to missing development packages.])])

    dnl
    dnl reset variables to old values
    dnl
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
  fi

  AC_SUBST([WAND_CFLAGS])
  AC_SUBST([WAND_LIBS])

  if test "x$wand" = "xyes" ; then
    ifelse([$1], , :, [$1])
  else
    ifelse([$2], , :, [$2])
  fi
])
