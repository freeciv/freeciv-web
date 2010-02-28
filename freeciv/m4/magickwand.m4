# Check for Freeciv MagickWand support used to create png images of the map
#
# Called without any parameters.

AC_DEFUN([FC_CHECK_MAGICKWAND],
[
  AC_ARG_WITH(magickwand,
              [  --with-magickwand[=DIR]   Imagemagick installation directory (optional)],
              magickwand_dir="$withval", magickwand_dir="")

  WAND_CONFIG_PATH=""
  if test x$magickwand_dir = x ; then
    AC_MSG_CHECKING([for MagickWand-config in default path])

    for i in /usr/local /usr;
    do
      test -r $i/bin/MagickWand-config && WAND_CONFIG_PATH=$i/bin && break
    done
  else
    AC_MSG_CHECKING([for MagickWand-config])

    test -r $magickwand_dir/MagickWand-config && WAND_CONFIG_PATH=$magickwand_dir
  fi

  if test -z "$WAND_CONFIG_PATH"; then
    AC_MSG_RESULT([not found - falling back to ppm images])
    wand=no
  else
    AC_MSG_RESULT([found in $WAND_CONFIG_PATH])
    wand=yes
  fi

  if test x$wand = xyes ; then
    AC_MSG_CHECKING([for MagickWand-config --cflags])
    WAND_CFLAGS="`$WAND_CONFIG_PATH/MagickWand-config --cflags`"
    AC_MSG_RESULT([$WAND_CFLAGS])

    AC_MSG_CHECKING([for MagickWand-config --libs])
    WAND_LIBS="`$WAND_CONFIG_PATH/MagickWand-config --libs`"
    AC_MSG_RESULT([$WAND_LIBS])

    AC_SUBST(WAND_CFLAGS)
    AC_SUBST(WAND_LIBS)

    AC_DEFINE(HAVE_WAND,1,[ ])
  fi
])
