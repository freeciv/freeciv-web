# Try to configure the GTK+-3.x client (gui-gtk-3.9x)

# FC_GTK3X_CLIENT
# Test for GTK+-3.92 libraries needed for gui-gtk-3.9x

AC_DEFUN([FC_GTK3X_CLIENT],
[
  # Add check "x$client" = "xauto"  when this becomes supported client
  if test "x$gui_gtk3x" = "xyes" ||
     test "x$client" = "xall" ; then
    PKG_CHECK_MODULES([GTK3X], [gtk+-4.0 >= 3.92.0],
      [
        GTK3X_CFLAGS="$GTK3X_CFLAGS -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_92"
        GTK3X_CFLAGS="$GTK3X_CFLAGS -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_52"
        gui_gtk3x=yes
        if test "x$client" = "xauto" ; then
          client=yes
        fi
        gui_gtk3x_cflags="$GTK3X_CFLAGS"
        gui_gtk3x_libs="$GTK3X_LIBS"
        if test "x$MINGW" = "xyes"; then
          dnl Required to compile gtk3 on Windows platform
          gui_gtk3x_cflags="$gui_gtk3x_cflags -mms-bitfields"
          gui_gtk3x_ldflags="$gui_gtk3x_ldflags -mwindows"
        fi
      ],
      [
        FC_NO_CLIENT([gtk3x], [GTK+-3.9x libraries not found])
      ])
  fi
])
