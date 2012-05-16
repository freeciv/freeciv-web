# Try to configure the GTK+-2.0 client (gui-gtk-2.0)

# FC_GTK_CLIENT
# Test for GTK+-2.0 libraries needed for gui-gtk-2.0

AC_DEFUN([FC_GTK2_CLIENT],
[
  if test "x$gui_gtk2" = "xyes" || test "x$client" = "xauto" ||
     test "x$client" = "xall" ; then
    AM_PATH_GTK_2_0(2.4.0,
      [
        gui_gtk2=yes
        if test "x$client" = "xauto" ; then
          client=yes
        fi
        GUI_gtk2_CFLAGS="$GTK_CFLAGS"
        GUI_gtk2_LIBS="$GTK_LIBS"
        if test "x$MINGW32" = "xyes"; then
          dnl Required to compile gtk2 on Windows platform
          GUI_gtk2_CFLAGS="$GUI_gtk2_CFLAGS -mms-bitfields"
          GUI_gtk2_LDFLAGS="$GUI_gtk2_LDFLAGS -mwindows"
        fi
      ],
      [
        FC_NO_CLIENT([gtk2], [GTK+-2.0 libraries not found])
      ])
  fi
])
