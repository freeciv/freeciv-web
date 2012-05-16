# Try to configure the SDL client (gui-sdl)

dnl FC_SDL_CLIENT
dnl Test for SDL and needed libraries for gui-sdl

AC_DEFUN([FC_SDL_CLIENT],
[
  if test "x$gui_sdl" = "xyes" || test "x$client" = "xall" ||
     test "x$client" = "xauto" ; then
    AM_PATH_SDL([1.1.4], [sdl_found="yes"], [sdl_found="no"])
    if test "$sdl_found" = yes; then
      ac_save_CPPFLAGS="$CPPFLAGS"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
      AC_CHECK_LIB([SDL_image], [IMG_Load],
                   [sdl_image_found="yes"], [sdl_image_found="no"])
      if test "$sdl_image_found" = "yes"; then
        AC_CHECK_HEADER([SDL/SDL_image.h],
                        [sdl_image_h_found="yes"], [sdl_image_h_found="no"])
    	if test "$sdl_image_h_found" = yes; then
          CPPFLAGS="$ac_save_CPPFLAGS"
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
	  AC_CHECK_FT2([2.1.3], [freetype_found="yes"],[freetype_found="no"])
          if test "$freetype_found" = yes; then
	    GUI_sdl_CFLAGS="$SDL_CFLAGS $FT2_CFLAGS"
	    GUI_sdl_LIBS="-lSDL_image $SDL_LIBS $FT2_LIBS"
	    found_client=yes
          elif test "x$gui_sdl" = "xyes"; then
            AC_MSG_ERROR([specified client 'sdl' not configurable (FreeType2 >= 2.1.3 is needed (www.freetype.org))])
          fi    
	elif test "x$gui_sdl" = "xyes"; then
	    AC_MSG_ERROR([specified client 'sdl' not configurable (SDL_image-devel is needed (www.libsdl.org))])
	fi
      elif test "x$gui_sdl" = "xyes"; then
        AC_MSG_ERROR([specified client 'sdl' not configurable (SDL_image is needed (www.libsdl.org))])
      fi
    fi

    if test "$found_client" = yes; then
      gui_sdl=yes
      if test "x$client" = "xauto" ; then
        client=yes
      fi

      dnl Check for libiconv (which is usually included in glibc, but may
      dnl be distributed separately).
      AM_ICONV
      AM_LIBCHARSET
      AM_LANGINFO_CODESET
      GUI_sdl_LIBS="$LIBICONV $GUI_sdl_LIBS"

      dnl Check for some other libraries - needed under BeOS for instance.
      dnl These should perhaps be checked for in all cases?
      AC_CHECK_LIB(socket, connect, GUI_sdl_LIBS="-lsocket $GUI_sdl_LIBS")
      AC_CHECK_LIB(bind, gethostbyaddr, GUI_sdl_LIBS="-lbind $GUI_sdl_LIBS")

    elif test "x$gui_sdl" = "xyes"; then
      AC_MSG_ERROR([specified client 'sdl' not configurable (SDL >= 1.1.4 is needed (www.libsdl.org))])
    fi
  fi
])
