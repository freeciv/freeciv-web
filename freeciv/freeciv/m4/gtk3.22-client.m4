# Try to configure the GTK+-3.22 client (gui-gtk-3.22)

# FC_GTK3X_CLIENT
# Test for GTK+-3.0 libraries needed for gui-gtk-3.22

AC_DEFUN([FC_GTK3_22_CLIENT],
[
  if test "x$gui_gtk3_22" = "xyes" ||
     test "x$client" = "xall" || test "x$client" = "xauto" ; then
    PKG_CHECK_MODULES([GTK3_22], [gtk+-3.0 >= 3.22.0],
      [
        GTK3_22_CFLAGS="$GTK3_22_CFLAGS -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_20"
        GTK3_22_CFLAGS="$GTK3_22_CFLAGS -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_50"
        gui_gtk3_22=yes
        if test "x$client" = "xauto" ; then
          client=yes
        fi
        gui_gtk3_22_cflags="$GTK3_22_CFLAGS"
        gui_gtk3_22_libs="$GTK3_22_LIBS"
        if test "x$MINGW" = "xyes"; then
          dnl Required to compile gtk3 on Windows platform
          gui_gtk3_22_cflags="$gui_gtk3_22_cflags -mms-bitfields"
          gui_gtk3_22_ldflags="$gui_gtk3_22_ldflags -mwindows"
        fi
      ],
      [
        FC_NO_CLIENT([gtk3.22], [GTK+-3.0 libraries not found])
      ])
  fi
])
