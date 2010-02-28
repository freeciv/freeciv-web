AC_DEFUN([FC_CHECK_SOUND],[
 AC_ARG_ENABLE(sdl-mixer,
   [  --disable-sdl-mixer     do not try to use the SDL mixer],
  [case "${enableval}" in
   yes) USE_SOUND_SDL=yes ;;
   no)  USE_SOUND_SDL=no ;;
   *)   AC_MSG_ERROR([bad value ${enableval} for --enable-sdl-mixer]) ;;
   esac], [USE_SOUND_SDL=maybe])

 if test "x$USE_SOUND_SDL" != "xno" ; then
  dnl Add SDL support to client
  SDL_VERSION=1.0.0
  AM_PATH_SDL($SDL_VERSION, SDL=yes, SDL=no)
  if test "x$SDL" != "xno"; then
    AC_CHECK_HEADER(SDL/SDL_mixer.h, SDL_mixer_h=1, SDL_mixer_h=0)
    AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio, SDL_mixer=yes)
    AC_MSG_CHECKING(building SDL_mixer support)
    if test "x$SDL_mixer_h" = "x1"; then
      if test "x$SDL_mixer" = "xyes"; then
        SOUND_CFLAGS="$SOUND_CFLAGS $SDL_CFLAGS"
        SOUND_LIBS="$SOUND_LIBS $SDL_LIBS -lSDL_mixer"
        AC_DEFINE(AUDIO_SDL, 1, [SDL_Mixer support])
        AC_MSG_RESULT(yes)
        SOUND_SDL_OK=true
      else
        AC_MSG_RESULT([no, found header but not library!])
      fi
    else
      AC_MSG_RESULT([no, install SDL_mixer first: http://www.libsdl.org/projects/SDL_mixer/index.html])
      SDL_mixer="xno"
    fi
  fi
  if test "x$USE_SOUND_SDL" = "xyes" && test "x$SOUND_SDL_OK" != "xtrue" ; then
    AC_MSG_ERROR([SDL mixer support requested, but cannot be compiled in])
  fi
 fi
])
