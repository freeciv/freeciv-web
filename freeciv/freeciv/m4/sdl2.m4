# Configure paths for SDL2
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor
#
# Changelog:
# * also look for SDL2.framework under Mac OS X
# * Taken to Freeciv from SDL release 2.0.8 - modified to work together with sdl1.2

# serial 1.0.2

dnl AM_PATH_SDL2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL2, and define SDL2_CFLAGS and SDL2_LIBS
dnl
AC_DEFUN([AM_PATH_SDL2],
[dnl 
dnl Get the cflags and libraries from the sdl2-config script
dnl
AC_ARG_WITH([sdl2-prefix],
  AS_HELP_STRING([--with-sdl2-prefix=PFX], [Prefix where SDL2 is installed (optional)]),
[sdl2_prefix="$withval"], [sdl2_prefix=""])

AC_ARG_WITH([sdl2-exec-prefix],
  AS_HELP_STRING([--with-sdl2-exec-prefix=PFX], [Exec prefix where SDL2 is installed (optional)]),
[sdl2_exec_prefix="$withval"], [sdl2_exec_prefix=""])

AC_ARG_ENABLE([sdl2test],
  AS_HELP_STRING([--disable-sdl2test], [Do not try to compile and run a test SDL2 program]),
[], [enable_sdl2test=yes])

AC_ARG_ENABLE([sdl2framework],
  AS_HELP_STRING([--disable-sdl2framework], [Do not search for SDL2.framework]),
[], [search_sdl2_framework=yes])

AC_ARG_VAR(SDL2_FRAMEWORK, [Path to SDL2.framework])

  min_sdl2_version=ifelse([$1], ,2.0.0,$1)

  if test "x$sdl2_prefix$sdl2_exec_prefix" = x ; then
    PKG_CHECK_MODULES([SDL2], [sdl2 >= $min_sdl2_version],
           [sdl2_pc=yes],
           [sdl2_pc=no])
  else
    sdl2_pc=no
    if test x$sdl2_exec_prefix != x ; then
      sdl2_config_args="$sdl2_config_args --exec-prefix=$sdl2_exec_prefix"
      if test x${SDL2_CONFIG+set} != xset ; then
        SDL2_CONFIG=$sdl2_exec_prefix/bin/sdl2-config
      fi
    fi
    if test x$sdl2_prefix != x ; then
      sdl2_config_args="$sdl2_config_args --prefix=$sdl2_prefix"
      if test x${SDL2_CONFIG+set} != xset ; then
        SDL2_CONFIG=$sdl2_prefix/bin/sdl2-config
      fi
    fi
  fi

  if test "x$sdl2_pc" = xyes ; then
    no_sdl2=""
    SDL2_CONFIG="pkg-config sdl2"
  else
    as_save_PATH="$PATH"
    if test "x$prefix" != xNONE && test "$cross_compiling" != yes; then
      PATH="$prefix/bin:$prefix/usr/bin:$PATH"
    fi
    AC_PATH_PROG(SDL2_CONFIG, sdl2-config, no, [$PATH])
    PATH="$as_save_PATH"
    no_sdl2=""

    if test "$SDL2_CONFIG" = "no" -a "x$search_sdl2_framework" = "xyes"; then
      AC_MSG_CHECKING(for SDL2.framework)
      if test "x$SDL2_FRAMEWORK" != x; then
        sdl2_framework=$SDL2_FRAMEWORK
      else
        for d in / ~/ /System/; do
          if test -d "$dLibrary/Frameworks/SDL2.framework"; then
            sdl2_framework="$dLibrary/Frameworks/SDL2.framework"
          fi
        done
      fi

      if test -d $sdl2_framework; then
        AC_MSG_RESULT($sdl2_framework)
        sdl2_framework_dir=`dirname $sdl2_framework`
        SDL_CFLAGS="-F$sdl2_framework_dir -Wl,-framework,SDL2 -I$sdl2_framework/include"
        SDL_LIBS="-F$sdl2_framework_dir -Wl,-framework,SDL2"
      else
        no_sdl2=yes
      fi
    fi

    if test "$SDL2_CONFIG" != "no"; then
      if test "x$sdl2_pc" = "xno"; then
        AC_MSG_CHECKING(for SDL2 - version >= $min_sdl2_version)
        SDL_CFLAGS=`$SDL2_CONFIG $sdl2_config_args --cflags`
        SDL_LIBS=`$SDL2_CONFIG $sdl2_config_args --libs`
      fi

      sdl2_major_version=`$SDL2_CONFIG $sdl2_config_args --version | \
             sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
      sdl2_minor_version=`$SDL2_CONFIG $sdl2_config_args --version | \
             sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
      sdl2_micro_version=`$SDL2_CONFIG $sdl2_config_args --version | \
             sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
      if test "x$enable_sdl2test" = "xyes" ; then
        ac_save_CFLAGS="$CFLAGS"
        ac_save_CXXFLAGS="$CXXFLAGS"
        ac_save_LIBS="$LIBS"
        CFLAGS="$CFLAGS $SDL2_CFLAGS"
        CXXFLAGS="$CXXFLAGS $SDL2_CFLAGS"
        LIBS="$LIBS $SDL2_LIBS"
dnl
dnl Now check if the installed SDL2 is sufficiently new. (Also sanity
dnl checks the results of sdl2-config to some extent
dnl
      rm -f conf.sdl2test
      AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdl2test");
  */
  { FILE *fp = fopen("conf.sdl2test", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl2_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl2_version");
     exit(1);
   }

   if (($sdl2_major_version > major) ||
      (($sdl2_major_version == major) && ($sdl2_minor_version > minor)) ||
      (($sdl2_major_version == major) && ($sdl2_minor_version == minor) && ($sdl2_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl2-config --version' returned %d.%d.%d, but the minimum version\n", $sdl2_major_version, $sdl2_minor_version, $sdl2_micro_version);
      printf("*** of SDL2 required is %d.%d.%d. If sdl2-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl2-config was wrong, set the environment variable SDL2_CONFIG\n");
      printf("*** to point to the correct copy of sdl2-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

]])], [], [no_sdl2=yes], [echo $ac_n "cross compiling; assumed OK... $ac_c"])
        CFLAGS="$ac_save_CFLAGS"
        CXXFLAGS="$ac_save_CXXFLAGS"
        LIBS="$ac_save_LIBS"

      fi
      if test "x$sdl2_pc" = "xno"; then
        if test "x$no_sdl2" = "xyes"; then
          AC_MSG_RESULT(no)
        else
          AC_MSG_RESULT(yes)
        fi
      fi
    fi
  fi
  if test "x$no_sdl2" = x ; then
     ifelse([$2], , :, [$2])
  else
     if test "$SDL2_CONFIG" = "no" ; then
       echo "*** The sdl2-config script installed by SDL2 could not be found"
       echo "*** If SDL2 was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL2_CONFIG environment variable to the"
       echo "*** full path to sdl2-config."
     else
       if test -f conf.sdl2test ; then
        :
       else
          echo "*** Could not run SDL2 test program, checking why..."
          CFLAGS="$CFLAGS $SDL2_CFLAGS"
          CXXFLAGS="$CXXFLAGS $SDL2_CFLAGS"
          LIBS="$LIBS $SDL2_LIBS"
          AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
]], [[ return 0; ]])],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL2 or finding the wrong"
          echo "*** version of SDL2. If it is not finding SDL2, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occurred. This usually means SDL2 was incorrectly installed"
          echo "*** or that you have moved SDL2 since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl2-config script: $SDL2_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          CXXFLAGS="$ac_save_CXXFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL2_CFLAGS=""
     SDL2_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL2_CFLAGS)
  AC_SUBST(SDL2_LIBS)
  rm -f conf.sdl2test
])
