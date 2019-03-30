# Try to configure the SDL2 client (gui-sdl2)

dnl FC_SDL2_CLIENT
dnl Test for SDL and needed libraries for gui-sdl2

AC_DEFUN([FC_SDL2_CLIENT],
[
  if test "x$gui_sdl2" = "xyes" || test "x$client" = "xall" ||
     test "x$client" = "xauto" ; then
    if test "x$SDL_mixer" = "xsdl" ; then
      if test "x$gui_sdl2" = "xyes"; then
        AC_MSG_ERROR([specified client 'sdl2' not configurable (cannot use SDL_mixer with it))])
      fi
      sdl2_found=no
    else
      AM_PATH_SDL2([2.0.0], [sdl2_found="yes"], [sdl2_found="no"])
    fi
    if test "$sdl2_found" = yes; then
      gui_sdl2_cflags="$SDL2_CFLAGS"
      gui_sdl2_libs="$SDL2_LIBS"
      FC_SDL2_PROJECT([SDL2_image], [IMG_Load], [SDL_image.h])
      if test "x$sdl2_h_found" = "xyes" ; then
        FC_SDL2_PROJECT([SDL2_ttf], [TTF_OpenFont], [SDL_ttf.h])
      else
        missing_2_project="SDL2_image"
      fi
      if test "x$sdl2_h_found" = "xyes" ; then
        FC_SDL2_PROJECT([SDL2_gfx], [rotozoomSurface], [SDL2_rotozoom.h])
      elif test "x$missing_2_project" = "x" ; then
        missing_2_project="SDL2_ttf"
      fi
      if test "x$sdl2_h_found" = "xyes" ; then
        dnl Version number here is as specified by libtool, not the main
        dnl freetype version number.
        dnl See http://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/VERSIONS.TXT
        PKG_CHECK_MODULES([FT2], [freetype2 >= 7.0.1], [freetype_found="yes"], [freetype_found="no"])
        if test "$freetype_found" = yes; then
          gui_sdl2_cflags="$gui_sdl2_cflags $FT2_CFLAGS"
          gui_sdl2_libs="$gui_sdl2_libs $FT2_LIBS"
          found_sdl2_client=yes
        elif test "x$gui_sdl2" = "xyes"; then
          AC_MSG_ERROR([specified client 'sdl2' not configurable (FreeType2 >= 2.1.3 is needed (www.freetype.org))])
        fi
      elif test "x$gui_sdl2" = "xyes"; then
        if test "x$missing_2_project" = "x" ; then
          missing_2_project="SDL2_gfx"
        fi
        if test "x$sdl2_lib_found" = "xyes" ; then
          missing_type="-devel"
        else
          missing_type=""
        fi
        AC_MSG_ERROR([specified client 'sdl2' not configurable (${missing_2_project}${missing_type} is needed (www.libsdl.org))])
      fi
    fi

    if test "$found_sdl2_client" = yes; then
      gui_sdl2=yes
      if test "x$client" = "xauto" ; then
        client=yes
      fi

      dnl Check for libiconv (which is usually included in glibc, but may
      dnl be distributed separately).
      AM_ICONV
      FC_LIBCHARSET
      AM_LANGINFO_CODESET
      gui_sdl2_libs="$LIBICONV $gui_sdl2_libs"

      dnl Check for some other libraries - needed under BeOS for instance.
      dnl These should perhaps be checked for in all cases?
      AC_CHECK_LIB(socket, connect, gui_sdl2_libs="-lsocket $gui_sdl2_libs")
      AC_CHECK_LIB(bind, gethostbyaddr, gui_sdl2_libs="-lbind $gui_sdl2_libs")

    elif test "x$gui_sdl2" = "xyes"; then
      AC_MSG_ERROR([specified client 'sdl2' not configurable (SDL2 >= 2.0.0 is needed (www.libsdl.org))])
    fi
  fi
])

AC_DEFUN([FC_SDL2_PROJECT],
[
  ac_save_CPPFLAGS="$CPPFLAGS"
  ac_save_CFLAGS="$CFLAGS"
  ac_save_LIBS="$LIBS"
  CPPFLAGS="$CPPFLAGS $SDL2_CFLAGS"
  CFLAGS="$CFLAGS $SDL2_CFLAGS"
  LIBS="$LIBS $SDL2_LIBS"
  AC_CHECK_LIB([$1], [$2],
               [sdl2_lib_found="yes"], [sdl2_lib_found="no"
sdl2_h_found="no"])
  if test "x$sdl2_lib_found" = "xyes" ; then
    sdl2_h_found="no"
    if test x$sdl_headers_without_path != xyes ; then
      AC_CHECK_HEADER([SDL2/$3],
                      [sdl2_h_found="yes"
gui_sdl2_libs="${gui_sdl2_libs} -l$1"], [])
    fi
    if test x$sdl2_h_found = xno ; then
      AC_CHECK_HEADER([$3], [sdl2_h_found="yes"
gui_sdl2_libs="${gui_sdl2_libs} -l$1"
if test x$sdl_headers_without_path != xyes ; then
  AC_DEFINE([SDL2_PLAIN_INCLUDE], [1], [sdl2 headers must be included without path])
  sdl_headers_without_path=yes
fi], [])
    fi
  fi
  CPPFLAGS="$ac_save_CPPFLAGS"
  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"
])
