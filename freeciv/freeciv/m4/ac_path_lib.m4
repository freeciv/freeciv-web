dnl @synopsis AC_PATH_LIB(LIBRARY [, MINIMUM-VERSION [, HEADERS [, CONFIG-SCRIPT [, MODULES [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]]]]])
dnl
dnl Runs a LIBRARY-config script and defines LIBRARY_CFLAGS and LIBRARY_LIBS,
dnl saving you from writing your own macro specific to your library.
dnl
dnl The defaults:
dnl  $1 = LIBRARY             e.g. gtk, ncurses
dnl  $2 = MINIMUM-VERSION     x.y.z format e.g. 4.2.1
dnl                           Add ' -nocheck' e.g. '4.2.1 -nocheck' to avoid
dnl                           checking version with library-defined version
dnl                           numbers (see below) i.e. --version only
dnl  $3 = HEADER              Header to include in test program if not
dnl                           called LIBRARY/LIBRARY.h
dnl  $4 = CONFIG-SCRIPT       Name of libconfig script if not
dnl                           LIBRARY-config
dnl  $5 = MODULES             List of module names to pass to LIBRARY-config.
dnl                           It is probably best to use only one, to avoid
dnl                           two version numbers being reported.
dnl  $6 = ACTION-IF-FOUND     Shell script to run if library is found
dnl  $7 = ACTION-IF-NOT-FOUND Shell script to run if library is not found
dnl
dnl LIBRARY-config must support `--cflags' and `--libs' args.
dnl If MINIMUM-VERSION is specified, LIBRARY-config should also support the
dnl `--version' arg, and have version information embedded in its header
dnl as detailed below:
dnl
dnl Macros:
dnl  #define LIBRARY_MAJOR_VERSION       (@LIBRARY_MAJOR_VERSION@)
dnl  #define LIBRARY_MINOR_VERSION       (@LIBRARY_MINOR_VERSION@)
dnl  #define LIBRARY_MICRO_VERSION       (@LIBRARY_MICRO_VERSION@)
dnl
dnl Version numbers (defined in the library):
dnl  extern const unsigned int library_major_version;
dnl  extern const unsigned int library_minor_version;
dnl  extern const unsigned int library_micro_version;
dnl
dnl If the above are not defined, then use ' -nocheck'.
dnl
dnl If the `--with-library-[exec-]prefix' arguments to ./configure are
dnl given, it must also support `--prefix' and `--exec-prefix'.
dnl (In other words, it must be like gtk-config.)
dnl
dnl If modules are to be used, LIBRARY-config must support modules.
dnl 
dnl For example:
dnl
dnl  `AC_PATH_LIB(foo, 1.0.0)'
dnl
dnl would run `foo-config --version' and check that it is at least 1.0.0
dnl
dnl If so, the following would then be defined:
dnl
dnl  FOO_CFLAGS  to `foo-config --cflags`
dnl  FOO_LIBS    to `foo-config --libs`
dnl  FOO_VERSION to `foo-config --version`
dnl
dnl Based on `AM_PATH_GTK' (gtk.m4) by Owen Taylor, and `AC_PATH_GENERIC'
dnl (ac_path_generic.m4) by Angus Lees <gusl@cse.unsw.edu.au>
dnl
dnl @version $Id: ac_path_lib.m4 4430 2002-04-13 13:52:03Z rfalke $
dnl @author Roger Leigh <roger@whinlatter.uklinux.net>
dnl
AC_DEFUN([AC_PATH_LIB],[# check for presence of lib$1
dnl
dnl we're going to need uppercase, lowercase and user-friendly versions of the
dnl string `LIBRARY', and long and cache variants.
dnl
m4_pushdef([UP], m4_translit([$1], [a-z], [A-Z]))dnl
m4_pushdef([DOWN], m4_translit([$1], [A-Z], [a-z]))dnl
m4_pushdef([LDOWN], ac_path_lib_[]DOWN)dnl
m4_pushdef([LUP], ac_path_lib_[]UP)dnl
m4_pushdef([CACHEDOWN], ac_cv_path_lib_[]DOWN)dnl
m4_pushdef([CACHEUP], ac_cv_path_lib_[]UP)dnl
LDOWN[]_header="m4_default([$3], [$1/$1.h])"
LDOWN[]_config="m4_default([$4], [$1-config])"
dnl
dnl get the cflags and libraries from the LIBRARY-config script
dnl
AC_ARG_WITH(DOWN-prefix,
            AC_HELP_STRING([--with-DOWN-prefix=PFX],
	                   [prefix where UP is installed (optional)]),
            [LDOWN[]_config_prefix="$withval"],
	    [LDOWN[]_config_prefix=""])dnl
AC_ARG_WITH(DOWN-exec-prefix,
            AC_HELP_STRING([--with-DOWN-exec-prefix=PFX],
	                   [exec-prefix where UP is installed (optional)]),
            [LDOWN[]_config_exec_prefix="$withval"],
	    [LDOWN[]_config_exec_prefix=""])dnl
AC_ARG_ENABLE(DOWN[]test,
              AC_HELP_STRING([--disable-DOWN[]test],
	                     [do not try to compile and run a test UP program]),
              [LDOWN[]_test_enabled="no"],
              [LDOWN[]_test_enabled="yes"])dnl
dnl
dnl set up LIBRARY-config script parameters
dnl
m4_if([$5], , ,
      [LDOWN[]_config_args="$LDOWN[]_config_args $5"])
LDOWN[]_min_version=`echo "$2" | sed -e 's/ -nocheck//'`
m4_if([$2], , ,[if test "$LDOWN[]_min_version" = "$2" ; then
                  LDOWN[]_version_test_enabled="yes"
                fi])
if test x$LDOWN[]_config_exec_prefix != x ; then
  LDOWN[]_config_args="$LDOWN[]_config_args --exec-prefix=$LDOWN[]_config_exec_prefix"
fi
if test x$LDOWN[]_config_prefix != x ; then
  LDOWN[]_config_args="$LDOWN[]_config_args --prefix=$LDOWN[]_config_prefix"
fi
dnl
dnl find LIBRARY-config script
dnl
AC_PATH_PROG(UP[]_CONFIG, $LDOWN[]_config, no)dnl
dnl
dnl run the test, but cache results so it can be skipped next time
dnl
if test x$UP[]_CONFIG = xno ; then
  LDOWN[]_present_avoidcache="no"
else
  LDOWN[]_present_avoidcache="yes"
dnl
dnl set up variables, caching those needed later
dnl
  AC_CACHE_CHECK([for UP CFLAGS],
                 [CACHEDOWN[]_cflags],
                 [CACHEDOWN[]_cflags=`$UP[]_CONFIG $LDOWN[]_config_args --cflags`])
  AC_CACHE_CHECK([for UP LIBS],
                 [CACHEDOWN[]_libs],
                 [CACHEDOWN[]_libs=`$UP[]_CONFIG $LDOWN[]_config_args --libs`])
  AC_CACHE_CHECK([for UP version],
                 [CACHEDOWN[]_version],
                 [CACHEDOWN[]_version=`$UP[]_CONFIG $LDOWN[]_config_args --version`])
  UP[]_CFLAGS="$CACHEDOWN[]_cflags"
  UP[]_LIBS="$CACHEDOWN[]_libs"
  UP[]_VERSION="$CACHEDOWN[]_version"
  LDOWN[]_config_major_version=`$UP[]_CONFIG $LDOWN[]_config_args --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  LDOWN[]_config_minor_version=`$UP[]_CONFIG $LDOWN[]_config_args --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  LDOWN[]_config_micro_version=`$UP[]_CONFIG $LDOWN[]_config_args --version | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  LDOWN[]_min_major_version=`echo "$LDOWN[]_min_version" | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  LDOWN[]_min_minor_version=`echo "$LDOWN[]_min_version" | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  LDOWN[]_min_micro_version=`echo "$LDOWN[]_min_version" | \
      sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
fi
dnl
dnl remove error log from previous runs
dnl
rm -f error.DOWN[]test
dnl
dnl now check if the installed UP is sufficiently new. (Also sanity
dnl checks the results of DOWN-config to some extent
dnl
AC_CACHE_CHECK([for UP - m4_if([$2], ,
                               [any version],
                               [version >= $LDOWN[]_min_version])],
               [CACHEDOWN[]_present],
[CACHEDOWN[]_present="$LDOWN[]_present_avoidcache"
if test x$CACHEDOWN[]_present = xyes -a x$LDOWN[]_test_enabled = xyes -a \
    x$LDOWN[]_version_test_enabled = xyes ; then
  ac_save_CFLAGS="$CFLAGS"
  ac_save_LIBS="$LIBS"
  CFLAGS="$CFLAGS $UP[]_CFLAGS"
  LIBS="$UP[]_LIBS $LIBS"
dnl
dnl now check if the installed UP is sufficiently new. (Also sanity
dnl checks the results of DOWN-config to some extent
dnl
  rm -f conf.DOWN[]test
  AC_TRY_RUN([
#include <$]LDOWN[_header>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;
  FILE *errorlog;

  if ((errorlog = fopen("error.]DOWN[test", "w")) == NULL) {
     exit(1);
   }

  system ("touch conf.]DOWN[test");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$]LDOWN[_min_version");
  if (!tmp_version) {
     exit(1);
   }
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     fprintf(errorlog, "*** %s: bad version string\n", "$]LDOWN[_min_version");
     exit(1);
   }

  if ((]DOWN[_major_version != $]LDOWN[_config_major_version) ||
      (]DOWN[_minor_version != $]LDOWN[_config_minor_version) ||
      (]DOWN[_micro_version != $]LDOWN[_config_micro_version))
    {
      fprintf(errorlog, "*** '$]UP[_CONFIG --version' returned %d.%d.%d, but \n", \
        $]LDOWN[_config_major_version, $]LDOWN[_config_minor_version, \
	$]LDOWN[_config_micro_version);
      fprintf(errorlog, "*** ]UP[ (%d.%d.%d) was found!\n", \
        ]DOWN[_major_version, ]DOWN[_minor_version, ]DOWN[_micro_version);
      fprintf(errorlog, "***\n");
      fprintf(errorlog, "*** If $]LDOWN[_config was correct, then it is best to remove\n");
      fprintf(errorlog, "*** the old version of ]UP[.  You may also be able to\n");
      fprintf(errorlog, "*** fix the error by modifying your LD_LIBRARY_PATH enviroment\n");
      fprintf(errorlog, "*** variable, or by editing /etc/ld.so.conf.  Make sure you have\n");
      fprintf(errorlog, "*** run ldconfig if that is required on your system.\n");
      fprintf(errorlog, "*** If $]LDOWN[_config was wrong, set the environment\n");
      fprintf(errorlog, "*** variable ]UP[_CONFIG to point to the correct copy of\n");
      fprintf(errorlog, "*** $]LDOWN[_config, and remove the file config.cache\n");
      fprintf(errorlog, "*** before re-running configure.\n");
    } 
#if defined (]UP[_MAJOR_VERSION) && defined (]UP[_MINOR_VERSION) && defined (]UP[_MICRO_VERSION)
  else if ((]DOWN[_major_version != ]UP[_MAJOR_VERSION) ||
	   (]DOWN[_minor_version != ]UP[_MINOR_VERSION) ||
           (]DOWN[_micro_version != ]UP[_MICRO_VERSION))
    {
      fprintf(errorlog, "*** ]UP[ header files (version %d.%d.%d) do not match\n",
	     ]UP[_MAJOR_VERSION, ]UP[_MINOR_VERSION, ]UP[_MICRO_VERSION);
      fprintf(errorlog, "*** library (version %d.%d.%d)\n",
	     ]DOWN[_major_version, ]DOWN[_minor_version, ]DOWN[_micro_version);
    }
#endif /* defined (]UP[_MAJOR_VERSION) ... */
  else
    {
      if ((]DOWN[_major_version > major) ||
        ((]DOWN[_major_version == major) && (]DOWN[_minor_version > minor)) ||
        ((]DOWN[_major_version == major) && (]DOWN[_minor_version == minor) && (]DOWN[_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        fprintf(errorlog, "*** An old version of ]UP[ (%d.%d.%d) was found.\n",
               ]DOWN[_major_version, ]DOWN[_minor_version, ]DOWN[_micro_version);
        fprintf(errorlog, "*** You need a version of ]UP[ newer than %d.%d.%d.\n",
	       major, minor, micro);
        /*fprintf(errorlog, "*** The latest version of ]UP[ is always available from ftp://ftp.my.site\n");*/
        fprintf(errorlog, "***\n");
        fprintf(errorlog, "*** If you have already installed a sufficiently new version, this\n");
        fprintf(errorlog, "*** error probably means that the wrong copy of the $]LDOWN[_config\n");
        fprintf(errorlog, "*** shell script is being found.  The easiest way to fix this is to\n");
        fprintf(errorlog, "*** remove the old version of ]UP[, but you can also set the\n");
        fprintf(errorlog, "*** ]UP[_CONFIG environment variable to point to the correct\n");
	fprintf(errorlog, "*** copy of $]LDOWN[_config.  (In this case, you will have to\n");
        fprintf(errorlog, "*** modify your LD_LIBRARY_PATH environment variable, or edit\n");
        fprintf(errorlog, "*** /etc/ld.so.conf so that the correct libraries are found at\n");
	fprintf(errorlog, "*** run-time.)\n");
      }
    }
  return 1;
}
],, [CACHEDOWN[]_present="no"],
    [echo $ac_n "cross compiling; assumed OK... $ac_c" >>error.]DOWN[test])
  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"
else
dnl
dnl check version just using --version values
dnl
  m4_if([$2], , ,[
  if test x$LDOWN[]_present_avoidcache = xyes ; then
    if test \
        "$LDOWN[]_config_major_version" -lt "$LDOWN[]_min_major_version" -o \
        "$LDOWN[]_config_major_version" -eq "$LDOWN[]_min_major_version" -a \
        "$LDOWN[]_config_minor_version" -lt "$LDOWN[]_min_minor_version" -o \
        "$LDOWN[]_config_major_version" -eq "$LDOWN[]_min_major_version" -a \
        "$LDOWN[]_config_minor_version" -eq "$LDOWN[]_min_minor_version" -a \
        "$LDOWN[]_config_micro_version" -lt "$LDOWN[]_min_micro_version" ; then
      CACHEDOWN[]_present="no"
      LDOWN[]_version_test_error="yes"
      echo "*** '$UP[]_CONFIG --version' returned $LDOWN[]_config_major_version.$LDOWN[]_config_minor_version.$LDOWN[]_config_micro_version, but" >>error.]DOWN[test
      echo "*** UP (>= $LDOWN[]_min_version) was needed." >>error.]DOWN[test
      echo "***" >>error.]DOWN[test
      echo "*** If $]LDOWN[_config was wrong, set the environment" >>error.]DOWN[test
      echo "*** variable ]UP[_CONFIG to point to the correct copy of" >>error.]DOWN[test
      echo "*** $]LDOWN[_config, and remove the file config.cache" >>error.]DOWN[test
      echo "*** before re-running configure." >>error.]DOWN[test
    else
      CACHEDOWN[]_present="yes"
    fi
  fi])
dnl
dnl if the user allowed it, try linking with the library
dnl
  if test x$LDOWN[]_test_enabled = xyes ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $UP[]_CFLAGS"
    LIBS="$UP[]_LIBS $LIBS"
    AC_TRY_LINK([
#include <stdio.h>
],,,            [CACHEDOWN[]_present="no"
                 if test x$LDOWN[]_version_test_error = xyes ; then
                   echo "***" >>error.]DOWN[test
                 fi
                 echo "*** The test program failed to compile or link.  See the file" >>error.]DOWN[test
                 echo "*** config.log for the exact error that occured.  This usually" >>error.]DOWN[test
                 echo "*** means UP was not installed, was incorrectly installed" >>error.]DOWN[test
                 echo "*** or that you have moved UP since it was installed.  In" >>error.]DOWN[test
                 echo "*** the latter case, you may want to edit the $LDOWN[]_config" >>error.]DOWN[test
                 echo "*** script: $UP[]_CONFIG" >>error.]DOWN[test])
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
  fi  
fi
dnl
dnl the test failed; try to diagnose the cause of failure
dnl
if test x$CACHEDOWN[]_present = xno ; then
  if test x$UP[]_CONFIG = xno ; then
    echo "*** The $LDOWN[]_config script installed by UP could not be found" >>error.]DOWN[test
    echo "*** If UP was installed in PREFIX, make sure PREFIX/bin is in" >>error.]DOWN[test
    echo "*** your path, or set the UP[]_CONFIG environment variable to the" >>error.]DOWN[test
    echo "*** full path to $LDOWN[]_config." >>error.]DOWN[test
  else
    if test -f conf.DOWN[]test ; then
      :
    elif test x$LDOWN[]_version_test_enabled = xyes ; then
      echo "*** Could not run UP test program, checking why..." >>error.]DOWN[test
      echo "***" >>error.]DOWN[test
      CFLAGS="$CFLAGS $UP[]_CFLAGS"
      LIBS="$LIBS $UP[]_LIBS"
      AC_TRY_LINK([
#include <$]LDOWN[_header>
#include <stdio.h>
],      [ return ((]DOWN[_major_version) || (]DOWN[_minor_version) || (]DOWN[_micro_version)); ],
        [ echo "*** The test program compiled, but did not run.  This usually" >>error.]DOWN[test
          echo "*** means that the run-time linker is not finding UP or finding" >>error.]DOWN[test
          echo "*** finding the wrong version of UP.  If it is not finding" >>error.]DOWN[test
          echo "*** UP, you'll need to set your LD_LIBRARY_PATH environment" >>error.]DOWN[test
          echo "*** variable, or edit /etc/ld.so.conf to point to the installed" >>error.]DOWN[test
          echo "*** location.  Also, make sure you have run ldconfig if that is" >>error.]DOWN[test
	  echo "*** required on your system." >>error.]DOWN[test
	  echo "***" >>error.]DOWN[test
          echo "*** If you have an old version installed, it is best to remove" >>error.]DOWN[test
	  echo "*** it, although you may also be able to get things to work by" >>error.]DOWN[test
	  echo "*** modifying LD_LIBRARY_PATH" >>error.]DOWN[test],
        [ echo "*** The test program failed to compile or link.  See the file" >>error.]DOWN[test
	  echo "*** config.log for the exact error that occured.  This usually" >>error.]DOWN[test
          echo "*** means UP was incorrectly installed or that you have" >>error.]DOWN[test
          echo "*** moved UP since it was installed.  In the latter case," >>error.]DOWN[test
	  echo "*** you may want to edit the $LDOWN[]_config script:" >>error.]DOWN[test
	  echo "*** $UP[]_CONFIG" >>error.]DOWN[test])
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi
  fi
  UP[]_CFLAGS=""
  UP[]_LIBS=""
  UP[]_VERSION=""
  m4_if([$7], , :, [$7])
fi])
dnl
dnl print the error log (after AC_CACHE_CHECK completes)
dnl
if test -f error.DOWN[]test ; then
  cat error.DOWN[]test
fi
dnl
dnl define variables and run extra code
dnl
UP[]_CFLAGS="$CACHEDOWN[]_cflags"
UP[]_LIBS="$CACHEDOWN[]_libs"
UP[]_VERSION="$CACHEDOWN[]_version"
AC_SUBST(UP[]_CFLAGS)dnl
AC_SUBST(UP[]_LIBS)dnl
AC_SUBST(UP[]_VERSION)dnl
# Run code which depends upon the test result.
if test x$CACHEDOWN[]_present = xyes ; then
  m4_if([$6], , :, [$6])     
else
  UP[]_CFLAGS=""
  UP[]_LIBS=""
  UP[]_VERSION=""
  m4_if([$7], , :, [$7])
fi
dnl
dnl clean up temporary files
dnl
rm -f conf.DOWN[]test
rm -f error.DOWN[]test
dnl
dnl pop the macros we defined earlier
dnl
m4_popdef([UP])dnl
m4_popdef([DOWN])dnl
m4_popdef([LDOWN])dnl
m4_popdef([LUP])dnl
m4_popdef([CACHEDOWN])dnl
m4_popdef([CACHEUP])dnl
])
