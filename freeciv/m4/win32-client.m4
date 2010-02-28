# Try to configure the Win32 client (gui-win32)

# FC_WIN32_CLIENT
# Test for Win32 and needed libraries for gui-win32

AC_DEFUN([FC_WIN32_CLIENT],
[
  if test "x$gui_win32" = "xyes" || test "x$client" = "xauto" ||
     test "x$client" = "xall" ; then
    if test "$MINGW32" = "yes"; then
    
      PKG_PROG_PKG_CONFIG
      
      dnl Check for libpng
      PKG_CHECK_MODULES([PNG], [libpng],
      [
        GUI_win32_LIBS="-lwsock32 -lcomctl32 -mwindows $PNG_LIBS"
        GUI_win32_CFLAGS="$PNG_CFLAGS"
      ],
      [
        AC_CHECK_LIB([z], [gzgets],
        [
          AC_CHECK_HEADER([zlib.h],
          [
            AC_CHECK_LIB([png12], [png_read_image],,
            [
              AC_CHECK_LIB([png], [png_read_image],,
              [
                FC_NO_CLIENT([win32], [libpng is needed])
              ])
            ])

            AC_CHECK_HEADER([png.h],
            [
              found_client=yes
	      gui_win32=yes
              if test "x$client" = "xauto" ; then
                client=yes
              fi
              GUI_win32_LIBS="-lwsock32 -lcomctl32  -lpng -mwindows"
            ],
            [
              FC_NO_CLIENT([win32], [libpng-dev is needed])
            ])
          ],
          [
            FC_NO_CLIENT([win32], [zlib-dev is needed])
          ])
        ],
        [
          FC_NO_CLIENT([win32], [zlib is needed])
        ])
      ])
    else
      FC_NO_CLIENT([win32], [mingw32 is needed])
    fi
  fi
])
