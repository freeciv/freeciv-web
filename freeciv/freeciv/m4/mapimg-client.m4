# Try to configure the GTK+-2.0 client (gui-gtk-2.0-mapimg)

# FC_GTK_CLIENT
# Test for GTK+-2.0 libraries needed for gui-gtk-2.0-mapimg

AC_DEFUN([FC_MAPIMG_CLIENT],
[
  if test "x$gui_mapimg" = "xyes" || test "x$client" = "xauto" ||
     test "x$client" = "xall" ; then
    AM_PATH_GTK_2_0(2.4.0,
      [
        gui_mapimg=yes
        # this is _not_ a full client!
        GUI_mapimg_CFLAGS="$GTK_CFLAGS"
        GUI_mapimg_LIBS="$GTK_LIBS"
        if test "x$MINGW32" = "xyes"; then
          dnl Required to compile gtk2 on Windows platform
          GUI_mapimg_CFLAGS="$GUI_mapimg_CFLAGS -mms-bitfields"
          GUI_mapimg_LDFLAGS="$GUI_mapimg_LDFLAGS -mwindows"
        fi
      ],
      [
        FC_NO_CLIENT([mapimg], [GTK+-2.0 libraries not found])
      ])
  fi
])
