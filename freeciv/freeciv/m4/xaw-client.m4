# Try to configure the XAW client (gui-xaw)

# FC_XAW_CLIENT
# Test for X and XAW libraries needed for gui-xaw

AC_DEFUN([FC_XAW_CLIENT],
[
  AC_REQUIRE([AC_PATH_XTRA])
  if test "x$gui_xaw" = "xyes" || test "x$client" = "xall" ||
     test "x$client" = "xauto" ; then
    dnl Checks for X:
    AC_PATH_XTRA

    dnl Determine the Xfuncproto control definitions:
    FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
    if test -n "$fc_x_proto_value"; then
      AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value, [Xfuncproto])
    fi
    FC_CHECK_X_PROTO_DEFINE(NARROWPROTO)
    if test -n "$fc_x_proto_value"; then
      AC_DEFINE_UNQUOTED(NARROWPROTO, $fc_x_proto_value, [Narrowproto])
    fi

    PKG_PROG_PKG_CONFIG

    dnl Check for libpng
    PKG_CHECK_MODULES([PNG], [libpng],
    [
      X_LIBS="$X_LIBS $PNG_LIBS"
      X_CFLAGS="$X_CFLAGS $PNG_CFLAGS"
    ],
    [
      AC_CHECK_LIB([png12], [png_read_image], [X_LIBS="$X_LIBS -lpng12 -lm"],
      [
        AC_CHECK_LIB([png], [png_read_image], [X_LIBS="$X_LIBS -lpng -lm"],
        [
          FC_NO_CLIENT([xaw], [Could not find PNG library.])
          no_png=yes
        ])
      ])
      AC_CHECK_HEADER([png.h],,
      [
        FC_NO_CLIENT([xaw], [libpng found but not png.h.
You may need to install a libpng \"development\" package.])
        no_png=yes
      ])
    ])

    dnl Try to get additional Xpm paths:
    FC_XPM_PATHS

    if test "$xpm_incdir" != "no"; then
      X_CFLAGS="$X_CFLAGS -I$xpm_incdir"
    fi
    if test "$xpm_libdir" != "no"; then
      X_LIBS="$X_LIBS -L$xpm_libdir"
      dnl Try using R values set in AC_PATH_XTRA:
      if test "$ac_R_nospace" = "yes"; then
        X_LIBS="$X_LIBS -R$xpm_libdir"
      elif test "$ac_R_space" = "yes"; then
        X_LIBS="$X_LIBS -R $xpm_libdir"
      fi
      dnl Some sites may put xpm.h in a directory whose parent isn't "X11"
      if test "x$xpm_h_no_x11" = "xyes"; then
        AC_DEFINE(XPM_H_NO_X11, 1, [XPM support])
      fi
    fi

    dnl Checks for X libs:
    fc_save_X_LIBS="$X_LIBS"
    X_LIBS="$X_LIBS $X_PRE_LIBS"
    FC_CHECK_X_LIB(X11, XOpenDisplay, , haveX11=no)
    if test "x$haveX11" != "xno"; then
      FC_CHECK_X_LIB(Xext, XShapeCombineMask)

      dnl Insert X_PRE_LIBS (eg -lSM -lICE) into X_EXTRA_LIBS here:
      X_EXTRA_LIBS="$X_PRE_LIBS $X_EXTRA_LIBS"
      X_LIBS="$fc_save_X_LIBS"

      FC_CHECK_X_LIB(Xt, main)
      FC_CHECK_X_LIB(Xmu, main)
      FC_CHECK_X_LIB(Xpm, XpmReadFileToPixmap, , haveXpm=no)
      if test "x$haveXpm" != "xno"; then
	dnl Xaw or Xaw3d:
	if test -n "$WITH_XAW3D"; then
	  FC_CHECK_X_LIB(Xaw3d, main, , AC_MSG_ERROR(did not find Xaw3d library))
        else
	  FC_CHECK_X_LIB(Xaw, main, , AC_MSG_ERROR(did not find Xaw library))
	fi

	GUI_xaw_CFLAGS="$X_CFLAGS"
	GUI_xaw_LIBS="$X_LIBS $X_EXTRA_LIBS"

	found_client=yes
      fi
    fi

    if test "x$found_client" = "xyes" && test "x$no_png" != "xyes"; then
      gui_xaw=yes
      if test "x$client" = "xauto" ; then
        client=yes
      fi
    elif test "x$gui_xaw" = "xyes"; then
      if test "x$haveXpm" = "xno"; then
	AC_MSG_ERROR(specified client 'xaw' not configurable -- need Xpm library and development headers; perhaps try/adjust --with-xpm-lib)
      else
	AC_MSG_ERROR(specified client 'xaw' not configurable -- need X11 libraries and development headers; perhaps try/adjust --x-libraries)
      fi
    fi
  fi
])
