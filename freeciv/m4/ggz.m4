dnl ======================================
dnl GGZ Gaming Zone - Configuration Macros
dnl ======================================
dnl
dnl Copyright (C) 2001 - 2007 Josef Spillner, josef@ggzgamingzone.org
dnl This file has heavily been inspired by KDE's acinclude :)
dnl It is published under the conditions of the GNU General Public License.
dnl
dnl ======================================
dnl
dnl This file is common to most GGZ modules, and should be kept in sync
dnl between them all.  The master copy resides with libggz.
dnl Currently the following modules use it:
dnl   kde-games, kde-client, gtk-games, gtk-client, utils, grubby,
dnl   ggz-client-libs, ggzd, gnome-client, txt-client, sdl-games, libggz
dnl See /docs/ggz-project/buildsystem for documentation.
dnl
dnl ======================================
dnl
dnl History:
dnl   See the SVN log for a full history.
dnl
dnl ------------------------------------------------------------------------
dnl Content of this file:
dnl ------------------------------------------------------------------------
dnl High-level macros:
dnl   AC_GGZ_CHECK - Checks for presence of GGZ client and server libraries.
dnl                  GGZ users can call this macro to determine at compile
dnl                  time whether to include GGZ support.  Server and client
dnl                  are checked separately.  GGZ_SERVER and GGZ_CLIENT are
dnl                  defined in config.h, and created as conditionals in
dnl                  the Makefiles.
dnl   AC_GGZ_CHECK_SERVER - The same, but server libs only.
dnl
dnl Low-level macros:
dnl   AC_GGZ_INIT - initialization and paths/options setup
dnl   AC_GGZ_VERSION - ensure a minimum version of GGZ
dnl   AC_GGZ_LIBGGZ - find the libggz headers and libraries
dnl   AC_GGZ_GGZCORE - find the ggzcore headers and libraries
dnl   AC_GGZ_CONFIG - find the ggz-config tool and set up configuration
dnl   AC_GGZ_GGZMOD - find the ggzmod library
dnl   AC_GGZ_GGZDMOD - find the ggzdmod library
dnl   AC_GGZ_SERVER - set up game and room path for ggzd game servers
dnl
dnl   Each macro takes two arguments:
dnl     1.  Action-if-found (or empty for no action).
dnl     2.  Action-if-not-found (or empty for error, or "ignore" to ignore).
dnl
dnl Internal functions:
dnl   AC_GGZ_ERROR - user-friendly error messages
dnl   AC_GGZ_FIND_FILE - macro for convenience (thanks kde)
dnl   AC_GGZ_REMOVEDUPS - eliminate duplicate list elements
dnl

# Version number of this script.
# First part is upstream (ggz) version and second Freeciv modifications.
# serial 0014.2

dnl ------------------------------------------------------------------------
dnl Find a directory containing a single file
dnl Synopsis: AC_GGZ_FIND_FILE(file, directorylist, <returnvar>)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_FIND_FILE],
[
$3=NO
for i in $2;
do
  for j in $1;
  do
    echo "configure: __oline__: $i/$j" >&AC_FD_CC
    if test -r "$i/$j"; then
      echo "taking that" >&AC_FD_CC
      $3=$i
      break 2
    fi
  done
done
])

dnl ------------------------------------------------------------------------
dnl Remove duplicate entries in a list, and remove all NO's
dnl Synopsis: AC_GGZ_REMOVEDUPS(list, <returnlist>)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_REMOVEDUPS],
[
ret=""
for i in $1; do
  add=yes
  for j in $ret; do
    if test "x$i" = "x$j"; then
      add=no
    fi
  done
  if test "x$i" = "xNO"; then
    add=no
  fi
  if test "x$add" = "xyes"; then
  ret="$ret $i"
  fi
done
$2=$ret
])

dnl ------------------------------------------------------------------------
dnl User-friendly error messages
dnl Synopsis: AC_GGZ_ERROR(libraryname, headerdirlist, libdirlist)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_ERROR],
[
  AC_MSG_WARN([no
  The library '$1' does not seem to be installed correctly.
  Headers searched in: $2
  Libraries searched in: $3
  Please read QuickStart.GGZ in order to fix this.
  ])
  exit 1
])

dnl ------------------------------------------------------------------------
dnl Initialization, common values and such
dnl Synopsis: AC_GGZ_INIT([export], [defaults])
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_INIT],
[
if test "x$prefix" = "xNONE"; then
  prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = "xNONE"; then
  exec_prefix='${prefix}'
fi

ac_ggz_prefix=""
AC_ARG_WITH(ggz-dir,
    AC_HELP_STRING([--with-ggz-dir=DIR], [Path to GGZ Gaming Zone]),
    [  ac_ggz_prefix="$withval"
    ])

if test "x$ac_ggz_prefix" != "xno" ; then
  if test "x${prefix}" = "xNONE"; then
    ac_ggz_prefix_incdir="${ac_default_prefix}/include"
    ac_ggz_prefix_libdir="${ac_default_prefix}/lib"
    ac_ggz_prefix_bindir="${ac_default_prefix}/bin"
    ac_ggz_prefix_etcdir="${ac_default_prefix}/etc"
  else
    unq_includedir="${includedir}"
    unq_libdir="${libdir}"
    unq_bindir="${bindir}"
    unq_sysconfdir="${sysconfdir}"

    eval unq_includedir=`echo $unq_includedir`
    eval unq_includedir=`echo $unq_includedir`
    eval unq_libdir=`echo $unq_libdir`
    eval unq_libdir=`echo $unq_libdir`
    eval unq_bindir=`echo $unq_bindir`
    eval unq_bindir=`echo $unq_bindir`
    eval unq_sysconfdir=`echo $unq_sysconfdir`
    eval unq_sysconfdir=`echo $unq_sysconfdir`

    ac_ggz_prefix_incdir="${unq_includedir}"
    ac_ggz_prefix_libdir="${unq_libdir}"
    ac_ggz_prefix_bindir="${unq_bindir}"
    ac_ggz_prefix_etcdir="${unq_sysconfdir}"
  fi
  ac_ggz_stdinc="$ac_ggz_prefix_incdir"
  ac_ggz_stdlib="$ac_ggz_prefix_libdir"
  ac_ggz_stdbin="$ac_ggz_prefix_bindir"
  ac_ggz_stdetc="$ac_ggz_prefix_etcdir/ggzd"
  if test "x$ac_ggz_prefix" != "x"; then
    ac_ggz_stdinc="$ac_ggz_stdinc $ac_ggz_prefix/include"
    ac_ggz_stdlib="$ac_ggz_stdlib $ac_ggz_prefix/lib $ac_ggz_prefix/lib64"
    ac_ggz_stdbin="$ac_ggz_stdbin $ac_ggz_prefix/bin"
    ac_ggz_stdetc="$ac_ggz_stdetc $ac_ggz_prefix/etc/ggzd"
  fi
  if test "x$1" = "xdefaults" || test "x$2" = "xdefaults"; then
    ac_ggz_stdinc="$ac_ggz_stdinc /usr/local/include /usr/include"
    ac_ggz_stdlib="$ac_ggz_stdlib /usr/local/lib /usr/local/lib64 /usr/lib /usr/lib64"
    ac_ggz_stdbin="$ac_ggz_stdbin /usr/local/bin /usr/bin"
    ac_ggz_stdetc="$ac_ggz_stdetc /usr/local/etc/ggzd /etc/ggzd"
  fi
  if test "x$1" = "xexport" || test "x$2" = "xexport"; then
    CPPFLAGS="$CPPFLAGS -I ${ac_ggz_prefix_incdir} -I /usr/local/include"
    LDFLAGS="$LDFLAGS -L${ac_ggz_prefix_libdir} -L/usr/local/lib"
  fi

  save_cflags=$CFLAGS
  save_cxxflags=$CXXFLAGS
  if test "x$GCC" = xyes; then
	CFLAGS="-Wall -Werror"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[void signedness(void){char c;if(c==-1)c=0;}]])],
		[],
		[save_cflags="$save_cflags -fsigned-char"
		 save_cxxflags="$save_cxxflags -fsigned-char"])
  else
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#if defined(__SUNPRO_C) || (__SUNPRO_C >= 0x550)
		#else
		# include "Error: Only GCC and Sun Studio are supported compilers."
		#endif]], [[]])],
		[save_cflags="$save_cflags -xchar=signed"
		 save_cxxflags="$save_cxxflags -xchar=signed"],
		[])

  fi
  CFLAGS=$save_cflags
  CXXFLAGS=$save_cxxflags
fi
])

dnl ------------------------------------------------------------------------
dnl Ensure that a minimum version of GGZ is present
dnl Synopsis: AC_GGZ_VERSION(major, minor, micro,
dnl                          action-if-found, action-if-not-found)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_VERSION],
[
	major=$1
	minor=$2
	micro=$3

	testprologue="#include <ggz.h>"
	testbody=""
	testbody="$testbody if(LIBGGZ_VERSION_MAJOR > $major) return 0;"
	testbody="$testbody if(LIBGGZ_VERSION_MAJOR < $major) return -1;"
	testbody="$testbody if(LIBGGZ_VERSION_MINOR > $minor) return 0;"
	testbody="$testbody if(LIBGGZ_VERSION_MINOR < $minor) return -1;"
	testbody="$testbody if(LIBGGZ_VERSION_MICRO > $micro) return 0;"
	testbody="$testbody if(LIBGGZ_VERSION_MICRO < $micro) return -1;"
	testbody="$testbody return 0;"

	save_libs=$LIBS
	save_ldflags=$LDFLAGS
	save_cppflags=$CPPFLAGS
	save_ldlibrary_path=$LD_LIBRARY_PATH
	LDFLAGS=$LIBGGZ_LDFLAGS
	LIBS=$LIB_GGZ
	CPPFLAGS=$LIBGGZ_INCLUDES
	LD_LIBRARY_PATH=$save_ldlibrary_path:$libggz_libraries
	export LD_LIBRARY_PATH

	AC_MSG_CHECKING([for GGZ library version: $major.$minor.$micro])
	AC_RUN_IFELSE(
		[AC_LANG_PROGRAM([[$testprologue]], [[$testbody]])],
		[ac_ggz_version_check=yes],
		[ac_ggz_version_check=no],
		[ac_ggz_version_check="skipped due to cross-compiling"]
	)
	if test "$ac_ggz_version_check" = "no"; then
		AC_MSG_RESULT([no])
		if test "x$5" = "x"; then
			AC_MSG_ERROR([The GGZ version is too old. Version $major.$minor.$micro is required.])
		fi
		$5
	else
		AC_MSG_RESULT($ac_ggz_version_check)
		$4
	fi

	LIBS=$save_libs
	LDFLAGS=$save_ldflags
	CPPFLAGS=$save_cppflags
	LD_LIBRARY_PATH=$save_ldlibrary_path
])

dnl ------------------------------------------------------------------------
dnl Try to find the libggz headers and libraries.
dnl $(LIBGGZ_LDFLAGS) will be -L ... (if needed)
dnl and $(LIBGGZ_INCLUDES) will be -I ... (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_LIBGGZ],
[
AC_MSG_CHECKING([for GGZ library: libggz])

ac_libggz_includes=NO ac_libggz_libraries=NO
libggz_libraries=""
libggz_includes=""

AC_ARG_WITH(libggz-dir,
    AC_HELP_STRING([--with-libggz-dir=DIR],[libggz installation prefix]),
    [  ac_libggz_includes="$withval"/include
       ac_libggz_libraries="$withval"/lib
    ])
AC_ARG_WITH(libggz-includes,
    AC_HELP_STRING([--with-libggz-includes=DIR],
                   [where the libggz includes are]),
    [  ac_libggz_includes="$withval"
    ])
AC_ARG_WITH(libggz-libraries,
    AC_HELP_STRING([--with-libggz-libraries=DIR],[where the libggz libs are]),
    [  ac_libggz_libraries="$withval"
    ])

AC_CACHE_VAL(ac_cv_have_libggz,
[
libggz_incdirs="$ac_libggz_includes $ac_ggz_stdinc"
AC_GGZ_REMOVEDUPS($libggz_incdirs, libggz_incdirs)
libggz_header=ggz.h

AC_GGZ_FIND_FILE($libggz_header, $libggz_incdirs, libggz_incdir)
ac_libggz_includes="$libggz_incdir"

libggz_libdirs="$ac_libggz_libraries $ac_ggz_stdlib"
AC_GGZ_REMOVEDUPS($libggz_libdirs, libggz_libdirs)

libggz_libdir=NO
for dir in $libggz_libdirs; do
  try="ls -1 $dir/libggz.la $dir/libggz.so"
  if test -n "`$try 2> /dev/null`"; then libggz_libdir=$dir; break; else echo "tried $dir" >&AC_FD_CC ; fi
done

ac_libggz_libraries="$libggz_libdir"

if test "$ac_libggz_includes" = NO || test "$ac_libggz_libraries" = NO; then
  ac_cv_have_libggz="have_libggz=no"
  ac_libggz_notfound=""
else
  have_libggz="yes"
fi
])

eval "$ac_cv_have_libggz"

if test "$have_libggz" != yes; then
  if test "x$2" = "xignore"; then
    AC_MSG_RESULT([$have_libggz (ignored)])
  else
    AC_MSG_RESULT([$have_libggz])
    if test "x$2" = "x"; then
      AC_GGZ_ERROR(libggz, $libggz_incdirs, $libggz_libdirs)
    fi

    # perform actions given by argument 2.
    $2
  fi
else
  ac_cv_have_libggz="have_libggz=yes \
    ac_libggz_includes=$ac_libggz_includes ac_libggz_libraries=$ac_libggz_libraries"
  AC_MSG_RESULT([$have_libggz (libraries $ac_libggz_libraries, headers $ac_libggz_includes)])

  libggz_libraries="$ac_libggz_libraries"
  libggz_includes="$ac_libggz_includes"

  AC_SUBST(libggz_libraries)
  AC_SUBST(libggz_includes)

  LIBGGZ_INCLUDES="-I $libggz_includes"
  LIBGGZ_LDFLAGS="-L$libggz_libraries"

  AC_SUBST(LIBGGZ_INCLUDES)
  AC_SUBST(LIBGGZ_LDFLAGS)

  LIB_GGZ='-lggz'
  AC_SUBST(LIB_GGZ)

  # perform actions given by argument 1.
  $1
fi

])

dnl ------------------------------------------------------------------------
dnl Try to find the ggzcore headers and libraries.
dnl $(GGZCORE_LDFLAGS) will be -L ... (if needed)
dnl and $(GGZCORE_INCLUDES) will be -I ... (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_GGZCORE],
[
AC_MSG_CHECKING([for GGZ library: ggzcore])

ac_ggzcore_includes=NO ac_ggzcore_libraries=NO
ggzcore_libraries=""
ggzcore_includes=""

AC_ARG_WITH(ggzcore-dir,
    AC_HELP_STRING([--with-ggzcore-dir=DIR],[ggzcore installation prefix]),
    [  ac_ggzcore_includes="$withval"/include
       ac_ggzcore_libraries="$withval"/lib
    ])
AC_ARG_WITH(ggzcore-includes,
    AC_HELP_STRING([--with-ggzcore-includes=DIR],
                   [where the ggzcore includes are]),
    [  ac_ggzcore_includes="$withval"
    ])
AC_ARG_WITH(ggzcore-libraries,
    AC_HELP_STRING([--with-ggzcore-libraries=DIR],
                   [where the ggzcore libs are]),
    [  ac_ggzcore_libraries="$withval"
    ])

AC_CACHE_VAL(ac_cv_have_ggzcore,
[
ggzcore_incdirs="$ac_ggzcore_includes $ac_ggz_stdinc"
AC_GGZ_REMOVEDUPS($ggzcore_incdirs, ggzcore_incdirs)
ggzcore_header=ggzcore.h

AC_GGZ_FIND_FILE($ggzcore_header, $ggzcore_incdirs, ggzcore_incdir)
ac_ggzcore_includes="$ggzcore_incdir"

ggzcore_libdirs="$ac_ggzcore_libraries $ac_ggz_stdlib"
AC_GGZ_REMOVEDUPS($ggzcore_libdirs, ggzcore_libdirs)

ggzcore_libdir=NO
for dir in $ggzcore_libdirs; do
  try="ls -1 $dir/libggzcore.la $dir/libggzcore.so"
  if test -n "`$try 2> /dev/null`"; then ggzcore_libdir=$dir; break; else echo "tried $dir" >&AC_FD_CC ; fi
done

ac_ggzcore_libraries="$ggzcore_libdir"

if test "$ac_ggzcore_includes" = NO || test "$ac_ggzcore_libraries" = NO; then
  ac_cv_have_ggzcore="have_ggzcore=no"
  ac_ggzcore_notfound=""
else
  have_ggzcore="yes"
fi
])

eval "$ac_cv_have_ggzcore"

if test "$have_ggzcore" != yes; then
  if test "x$2" = "xignore"; then
    AC_MSG_RESULT([$have_ggzcore (intentionally ignored)])
  else
    AC_MSG_RESULT([$have_ggzcore])
    if test "x$2" = "x"; then
      AC_GGZ_ERROR(ggzcore, $ggzcore_incdirs, $ggzcore_libdirs)
    fi

    # Perform actions given by argument 2.
    $2
  fi
else
  ac_cv_have_ggzcore="have_ggzcore=yes \
    ac_ggzcore_includes=$ac_ggzcore_includes ac_ggzcore_libraries=$ac_ggzcore_libraries"
  AC_MSG_RESULT([$have_ggzcore (libraries $ac_ggzcore_libraries, headers $ac_ggzcore_includes)])

  ggzcore_libraries="$ac_ggzcore_libraries"
  ggzcore_includes="$ac_ggzcore_includes"

  AC_SUBST(ggzcore_libraries)
  AC_SUBST(ggzcore_includes)

  GGZCORE_INCLUDES="-I $ggzcore_includes"
  GGZCORE_LDFLAGS="-L$ggzcore_libraries"

  AC_SUBST(GGZCORE_INCLUDES)
  AC_SUBST(GGZCORE_LDFLAGS)

  LIB_GGZCORE='-lggzcore'
  AC_SUBST(LIB_GGZCORE)

  # Perform actions given by argument 1.
  $1
fi

])

dnl ------------------------------------------------------------------------
dnl Try to find the ggz-config binary.
dnl Sets GGZ_CONFIG to the path/name of the program.
dnl Sets also: ggz_gamedir, ggz_datadir etc.
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_CONFIG],
[
AC_MSG_CHECKING([for GGZ configuration tool: ggz-config])

ac_ggz_config=NO
ggz_config=""

AC_ARG_WITH(ggzconfig,
    AC_HELP_STRING([--with-ggzconfig=DIR],[path to ggz-config]),
    [  ac_ggz_config="$withval"
    ])

ac_ggz_config_orig=$ac_ggz_config

AC_CACHE_VAL(ac_cv_have_ggzconfig,
[
ggz_config_dirs="$ac_ggz_config $ac_ggz_stdbin"

AC_GGZ_FIND_FILE(ggz-config, $ggz_config_dirs, ggz_config_dir)
ac_ggz_config="$ggz_config_dir"

if test "$ac_ggz_config" = NO; then
  ac_cv_have_ggzcore="have_ggz_config=no"
  ac_ggz_config_notfound=""
  have_ggz_config="no"
else
  have_ggz_config="yes"
fi
])

eval "$ac_cv_have_ggz_config"

if test "$have_ggz_config" != yes; then
  if test "x$2" = "xignore"; then
    AC_MSG_RESULT([$have_ggz_config (intentionally ignored)])
    GGZ_CONFIG="true"
    ggzexecmoddir="\${libdir}/ggz"
    ggzdatadir="\${datadir}/ggz"
    AC_SUBST(GGZ_CONFIG)
    AC_SUBST(ggzexecmoddir)
    AC_SUBST(ggzdatadir)
    AC_DEFINE_UNQUOTED(GAMEDIR, "${libdir}/ggz", [Path where to install the games])
    AC_DEFINE_UNQUOTED(GGZDATADIR, "${datadir}/ggz", [Path where the games should look for their data files])
  else
    AC_MSG_RESULT([$have_ggz_config])
    if test "x$2" = "x"; then
      AC_MSG_ERROR([ggz-config not found. Please check your installation! ])
    fi

    # Perform actions given by argument 2.
    $2
  fi
else
  pathto_app=`echo $prefix/bin/ | tr -s "/"`
  pathto_ggz=`echo $ac_ggz_config/ | tr -s "/"`

  if test "$ac_ggz_config_orig" != "NO"; then
    pathto_app=$pathto_ggz
  fi

  if test "x$pathto_app" != "x$pathto_ggz"; then
    AC_MSG_RESULT([$have_ggz_config (dismissed due to different prefix)])
    GGZ_CONFIG="true"
    ggzexecmoddir="\${libdir}/ggz"
    ggzdatadir="\${datadir}/ggz"
    AC_SUBST(GGZ_CONFIG)
    AC_SUBST(ggzexecmoddir)
    AC_SUBST(ggzdatadir)
    AC_DEFINE_UNQUOTED(GGZMODULECONFDIR, "${prefix}/etc", [Path where the game registry is located])
    AC_DEFINE_UNQUOTED(GAMEDIR, "${libdir}/ggz", [Path where to install the games])
    AC_DEFINE_UNQUOTED(GGZDATADIR, "${datadir}/ggz", [Path where the games should look for their data files])
  else
    ac_cv_have_ggz_config="have_ggz_config=yes \
      ac_ggz_config=$ac_ggz_config"
    AC_MSG_RESULT([$ac_ggz_config/ggz-config])

    ggz_config="$ac_ggz_config"
    AC_SUBST(ggz_config)

    AC_ARG_ENABLE([noregistry],
      AC_HELP_STRING([--enable-noregistry], [Do not register game modules.]),
      [enable_noregistry=yes], [enable_noregistry=no])

    GGZ_CONFIG="${ggz_config}/ggz-config"
    if test "$enable_noregistry" = yes; then
      GGZ_CONFIG="$GGZ_CONFIG --noregistry=$enableval"
    fi
    AC_SUBST(GGZ_CONFIG)

    ggzmoduleconfdir=`$GGZ_CONFIG --configdir`
    AC_DEFINE_UNQUOTED(GGZMODULECONFDIR, "${ggzmoduleconfdir}", [Path where the game registry is located])
    ggzexecmoddir=`$GGZ_CONFIG --gamedir`
    AC_DEFINE_UNQUOTED(GAMEDIR, "${ggzexecmoddir}", [Path where to install the games])
    ggzdatadir=`$GGZ_CONFIG --datadir`
    AC_DEFINE_UNQUOTED(GGZDATADIR, "${ggzdatadir}", [Path where the games should look for their data files])
    packagesrcdir=`cd $srcdir && pwd`
    AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, "${packagesrcdir}", [Path where the source is located])

    if test "$ggzmoduleconfdir" = ""; then
      AC_MSG_ERROR([ggz-config is unusable. Maybe LD_LIBRARY_PATH needs to be set.])
    fi

    AC_SUBST(ggzmoduleconfdir)
    AC_SUBST(ggzexecmoddir)
    AC_SUBST(ggzdatadir)
    AC_SUBST(packagesrcdir)

    # Perform actions given by argument 1.
    $1
  fi
fi

])

dnl ------------------------------------------------------------------------
dnl Try to find the ggzmod headers and libraries.
dnl $(GGZMOD_LDFLAGS) will be -L ... (if needed)
dnl and $(GGZMOD_INCLUDES) will be -I ... (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_GGZMOD],
[
AC_MSG_CHECKING([for GGZ library: ggzmod])

ac_ggzmod_includes=NO ac_ggzmod_libraries=NO
ggzmod_libraries=""
ggzmod_includes=""

AC_ARG_WITH(ggzmod-dir,
    AC_HELP_STRING([--with-ggzmod-dir=DIR],[ggzmod installation prefix]),
    [  ac_ggzmod_includes="$withval"/include
       ac_ggzmod_libraries="$withval"/lib
    ])
AC_ARG_WITH(ggzmod-includes,
    AC_HELP_STRING([--with-ggzmod-includes=DIR],
                   [where the ggzmod includes are]),
    [  ac_ggzmod_includes="$withval"
    ])
AC_ARG_WITH(ggzmod-libraries,
    AC_HELP_STRING([--with-ggzmod-libraries=DIR],
                   [where the ggzmod libs are]),
    [  ac_ggzmod_libraries="$withval"
    ])

AC_CACHE_VAL(ac_cv_have_ggzmod,
[
ggzmod_incdirs="$ac_ggzmod_includes $ac_ggz_stdinc"
AC_GGZ_REMOVEDUPS($ggzmod_incdirs, ggzmod_incdirs)
ggzmod_header=ggzmod.h

AC_GGZ_FIND_FILE($ggzmod_header, $ggzmod_incdirs, ggzmod_incdir)
ac_ggzmod_includes="$ggzmod_incdir"

ggzmod_libdirs="$ac_ggzmod_libraries $ac_ggz_stdlib"
AC_GGZ_REMOVEDUPS($ggzmod_libdirs, ggzmod_libdirs)

ggzmod_libdir=NO
for dir in $ggzmod_libdirs; do
  try="ls -1 $dir/libggzmod.la $dir/libggzmod.so"
  if test -n "`$try 2> /dev/null`"; then ggzmod_libdir=$dir; break; else echo "tried $dir" >&AC_FD_CC ; fi
done

ac_ggzmod_libraries="$ggzmod_libdir"

if test "$ac_ggzmod_includes" = NO || test "$ac_ggzmod_libraries" = NO; then
  ac_cv_have_ggzmod="have_ggzmod=no"
  ac_ggzmod_notfound=""
else
  have_ggzmod="yes"
fi
])

eval "$ac_cv_have_ggzmod"

if test "$have_ggzmod" != yes; then
  if test "x$2" = "xignore"; then
    AC_MSG_RESULT([$have_ggzmod (intentionally ignored)])
  else
    AC_MSG_RESULT([$have_ggzmod])
    if test "x$2" = "x"; then
      AC_GGZ_ERROR(ggzmod, $ggzmod_incdirs, $ggzmod_libdirs)
    fi

    # Perform actions given by argument 2.
    $2
  fi
else
  ac_cv_have_ggzmod="have_ggzmod=yes \
    ac_ggzmod_includes=$ac_ggzmod_includes ac_ggzmod_libraries=$ac_ggzmod_libraries"
  AC_MSG_RESULT([$have_ggzmod (libraries $ac_ggzmod_libraries, headers $ac_ggzmod_includes)])

  ggzmod_libraries="$ac_ggzmod_libraries"
  ggzmod_includes="$ac_ggzmod_includes"

  AC_SUBST(ggzmod_libraries)
  AC_SUBST(ggzmod_includes)

  GGZMOD_INCLUDES="-I $ggzmod_includes"
  GGZMOD_LDFLAGS="-L$ggzmod_libraries"

  AC_SUBST(GGZMOD_INCLUDES)
  AC_SUBST(GGZMOD_LDFLAGS)

  LIB_GGZMOD='-lggzmod'
  AC_SUBST(LIB_GGZMOD)

  # Perform actions given by argument 1.
  $1
fi

])

dnl ------------------------------------------------------------------------
dnl Try to find the ggzdmod headers and libraries.
dnl $(GGZDMOD_LDFLAGS) will be -L ... (if needed)
dnl and $(GGZDMOD_INCLUDES) will be -I ... (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_GGZDMOD],
[
AC_MSG_CHECKING([for GGZ library: ggzdmod])

ac_ggzdmod_includes=NO ac_ggzdmod_libraries=NO
ggzdmod_libraries=""
ggzdmod_includes=""

AC_ARG_WITH(ggzdmod-dir,
    AC_HELP_STRING([--with-ggzdmod-dir=DIR], [ggzdmod installation prefix]),
    [  ac_ggzdmod_includes="$withval"/include
       ac_ggzdmod_libraries="$withval"/lib
    ])
AC_ARG_WITH(ggzdmod-includes,
    AC_HELP_STRING([--with-ggzdmod-includes=DIR], 
                   [where the ggzdmod includes are]),
    [  ac_ggzdmod_includes="$withval"
    ])
AC_ARG_WITH(ggzdmod-libraries,
    AC_HELP_STRING([--with-ggzdmod-libraries=DIR],
                   [where the ggzdmod libs are]),
    [  ac_ggzdmod_libraries="$withval"
    ])

AC_CACHE_VAL(ac_cv_have_ggzdmod,
[
ggzdmod_incdirs="$ac_ggzdmod_includes $ac_ggz_stdinc"
AC_GGZ_REMOVEDUPS($ggzdmod_incdirs, ggzdmod_incdirs)
ggzdmod_header=ggzdmod.h

AC_GGZ_FIND_FILE($ggzdmod_header, $ggzdmod_incdirs, ggzdmod_incdir)
ac_ggzdmod_includes="$ggzdmod_incdir"

ggzdmod_libdirs="$ac_ggzdmod_libraries $ac_ggz_stdlib"
AC_GGZ_REMOVEDUPS($ggzdmod_libdirs, ggzdmod_libdirs)

ggzdmod_libdir=NO
for dir in $ggzdmod_libdirs; do
  try="ls -1 $dir/libggzdmod.la $dir/libggzdmod.so"
  if test -n "`$try 2> /dev/null`"; then ggzdmod_libdir=$dir; break; else echo "tried $dir" >&AC_FD_CC ; fi
done

ac_ggzdmod_libraries="$ggzdmod_libdir"

if test "$ac_ggzdmod_includes" = NO || test "$ac_ggzdmod_libraries" = NO; then
  ac_cv_have_ggzdmod="have_ggzdmod=no"
  ac_ggzdmod_notfound=""
else
  have_ggzdmod="yes"
fi
])

eval "$ac_cv_have_ggzdmod"

if test "$have_ggzdmod" != yes; then
  if test "x$2" = "xignore"; then
    AC_MSG_RESULT([$have_ggzdmod (intentionally ignored)])
  else
    AC_MSG_RESULT([$have_ggzdmod])
    if test "x$2" = "x"; then
      AC_GGZ_ERROR(ggzdmod, $ggzdmod_incdirs, $ggzdmod_libdirs)
    fi

    # Perform actions given by argument 2.
    $2
  fi
else
  ac_cv_have_ggzdmod="have_ggzdmod=yes \
    ac_ggzdmod_includes=$ac_ggzdmod_includes ac_ggzdmod_libraries=$ac_ggzdmod_libraries"
  AC_MSG_RESULT([$have_ggzdmod (libraries $ac_ggzdmod_libraries, headers $ac_ggzdmod_includes)])

  ggzdmod_libraries="$ac_ggzdmod_libraries"
  ggzdmod_includes="$ac_ggzdmod_includes"

  AC_SUBST(ggzdmod_libraries)
  AC_SUBST(ggzdmod_includes)

  GGZDMOD_INCLUDES="-I $ggzdmod_includes"
  GGZDMOD_LDFLAGS="-L$ggzdmod_libraries"

  AC_SUBST(GGZDMOD_INCLUDES)
  AC_SUBST(GGZDMOD_LDFLAGS)

  LIB_GGZDMOD='-lggzdmod'
  AC_SUBST(LIB_GGZDMOD)

  # Perform actions given by argument 1.
  $1
fi

])

dnl ------------------------------------------------------------------------
dnl Try to find the ggz-gtk headers and libraries.
dnl $(GGZGTK_LDFLAGS) will be -L ... (if needed)
dnl and $(GGZGTK_INCLUDES) will be -I ... (if needed)
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_GTK],
[
AC_MSG_CHECKING([for GGZ library: ggz-gtk])

ac_ggz_gtk_includes=NO ac_ggz_gtk_libraries=NO
ggz_gtk_libraries=""
ggz_gtk_includes=""

AC_ARG_WITH(ggz-gtk-dir,
    AC_HELP_STRING([--with-ggz-gtk-dir=DIR], [ggz-gtk installation prefix]),
    [  ac_ggz_gtk_includes="$withval"/include
       ac_ggz_gtk_libraries="$withval"/lib
    ])
AC_ARG_WITH(ggz-gtk-includes,
    AC_HELP_STRING([--with-ggz-gtk-includes=DIR], 
                   [where the ggz-gtk includes are]),
    [  ac_ggz_gtk_includes="$withval"
    ])
AC_ARG_WITH(ggz-gtk-libraries,
    AC_HELP_STRING([--with-ggz-gtk-libraries=DIR],
                   [where the ggz-gtk libs are]),
    [  ac_ggz_gtk_libraries="$withval"
    ])

AC_CACHE_VAL(ac_cv_have_ggz_gtk,
[
ggz_gtk_incdirs="$ac_ggz_gtk_includes $ac_ggz_stdinc"
AC_GGZ_REMOVEDUPS($ggz_gtk_incdirs, ggz_gtk_incdirs)
ggz_gtk_header=ggz-gtk.h

AC_GGZ_FIND_FILE($ggz_gtk_header, $ggz_gtk_incdirs, ggz_gtk_incdir)
ac_ggz_gtk_includes="$ggz_gtk_incdir"

ggz_gtk_libdirs="$ac_ggz_gtk_libraries $ac_ggz_stdlib"
AC_GGZ_REMOVEDUPS($ggz_gtk_libdirs, ggz_gtk_libdirs)

ggz_gtk_libdir=NO
for dir in $ggz_gtk_libdirs; do
  try="ls -1 $dir/libggz-gtk.la $dir/libggz-gtk.so"
  if test -n "`$try 2> /dev/null`"; then ggz_gtk_libdir=$dir; break; else echo "tried $dir" >&AC_FD_CC ; fi
done

ac_ggz_gtk_libraries="$ggz_gtk_libdir"

if test "$ac_ggz_gtk_includes" = NO || test "$ac_ggz_gtk_libraries" = NO; then
  ac_cv_have_ggz_gtk="have_ggz_gtk=no"
  ac_ggz_gtk_notfound=""
else
  have_ggz_gtk="yes"
fi
])

eval "$ac_cv_have_ggz_gtk"

if test "$have_ggz_gtk" != yes; then
  if test "x$2" = "xignore"; then
    AC_MSG_RESULT([$have_ggz_gtk (intentionally ignored)])
  else
    AC_MSG_RESULT([$have_ggz_gtk])
    if test "x$2" = "x"; then
      AC_GGZ_ERROR(ggz-gtk, $ggz_gtk_incdirs, $ggz_gtk_libdirs)
    fi

    # Perform actions given by argument 2.
    $2
  fi
else
  ac_cv_have_ggz_gtk="have_ggz_gtk=yes \
    ac_ggz_gtk_includes=$ac_ggz_gtk_includes ac_ggz_gtk_libraries=$ac_ggz_gtk_libraries"
  AC_MSG_RESULT([$have_ggz_gtk (libraries $ac_ggz_gtk_libraries, headers $ac_ggz_gtk_includes)])

  ggz_gtk_libraries="$ac_ggz_gtk_libraries"
  ggz_gtk_includes="$ac_ggz_gtk_includes"

  AC_SUBST(ggz_gtk_libraries)
  AC_SUBST(ggz_gtk_includes)

  GGZ_GTK_INCLUDES="-I $ggz_gtk_includes"
  GGZ_GTK_LDFLAGS="-L$ggz_gtk_libraries"

  AC_SUBST(GGZ_GTK_INCLUDES)
  AC_SUBST(GGZ_GTK_LDFLAGS)

  LIB_GGZ_GTK='-lggz-gtk'
  AC_SUBST(LIB_GGZ_GTK)

  # Perform actions given by argument 1.
  $1
fi
])

dnl ------------------------------------------------------------------------
dnl Setup the game server configuration.
dnl Sets ggzdconfdir (ggzd configuration).
dnl Sets ggzddatadir (for game server data).
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_SERVER],
[
AC_MSG_CHECKING([for GGZ server: ggzd])
AC_ARG_WITH(ggzd-confdir,
    AC_HELP_STRING([--with-ggzd-confdir=DIR], [directory for room/game data]),
[ ac_ggzd_confdir="$withval"
])

AC_CACHE_VAL(ac_cv_have_ggzdconf,
[
	if test "x$1" = "xforce"; then
		if test "x$ac_ggzd_confdir" = "x"; then
			ggzdconfdirs="$ac_ggz_stdetc"
		else
			ggzdconfdirs="$ac_ggzd_confdir"
		fi
	else
		ggzdconfdirs="$ac_ggzd_confdir $ac_ggz_stdetc"
	fi

	ggzdconfdir=NONE
	for dir in $ggzdconfdirs; do
		if test -n "`ls -d $dir/rooms 2> /dev/null`"; then
			if test -n "`ls -d $dir/rooms 2> /dev/null`"; then
				ggzdconfdir=$dir; break;
			else
				echo "tried $dir" >&AC_FD_CC;
			fi
		else
			echo "tried $dir" >&AC_FD_CC;
		fi
	done

	if test "x$ggzdconfdir" = "xNONE"; then
		have_ggzdconf="no"
	else
		have_ggzdconf="yes"
	fi
])

eval "$ac_cv_have_ggzdconf"

if test "$have_ggzdconf" != yes; then
	if test "x$2" = "xignore"; then
	  AC_MSG_RESULT([$have_ggzdconf (intentionally ignored)])
	elif test "x$2" = "xforce"; then
	  if test "x$ac_ggzd_confdir" = "x"; then
	    ggzdconfdir="\${prefix}/etc/ggzd"
	  else
	    ggzdconfdir=$ac_ggzd_confdir
	  fi
	  AC_MSG_RESULT([$have_ggzdconf (but forced to ${ggzdconfdir})])
	else
	  AC_MSG_RESULT([$have_ggzdconf])
	  if test "x$2" = "x"; then
	    AC_MSG_ERROR([GGZ server configuration not found. Please check your installation! ])
	  fi

	  # Perform actions given by argument 2.
	  $2
	fi
else
	prefixed=0
	if test "x${prefix}" != "xNONE" && test "x${prefix}" != "x${ac_default_prefix}"; then
		if test "x$ac_ggzd_confdir" = "x"; then
			prefixed=1
		fi
	fi
	if test "x$ggzdconfdir" != "x${prefix}/etc/ggzd" && test "x$prefixed" = "x1"; then
		AC_MSG_RESULT([$have_ggzdconf ($ggzdconfdir, but using ${prefix}/etc/ggzd nevertheless)])
		ggzdconfdir="\${prefix}/etc/ggzd"
	else
		AC_MSG_RESULT([$have_ggzdconf ($ggzdconfdir)])
	fi
fi

if test "$have_ggzdconf" = yes || test "x$2" = "xforce"; then
	AC_SUBST(ggzdconfdir)

	ggzddatadir=${datadir}/${PACKAGE}
	AC_DEFINE_UNQUOTED(GGZDDATADIR, "${ggzddatadir}", [Game server data directory])
	AC_SUBST(ggzddatadir)

	if test "x${libdir}" = 'x${exec_prefix}/lib'; then
	  if test "x${exec_prefix}" = "xNONE"; then
	    if test "x${prefix}" = "xNONE"; then
	      ggzdexecmoddir="\${ac_default_prefix}/lib/ggzd"
	      ggzdexecmodpath="${ac_default_prefix}/lib/ggzd"
	    else
	      ggzdexecmoddir="\${prefix}/lib/ggzd"
	      ggzdexecmodpath="${prefix}/lib/ggzd"
	    fi
	  else
	    ggzdexecmoddir="\${exec_prefix}/lib/ggzd"
	    ggzdexecmodpath="${exec_prefix}/lib/ggzd"
	  fi
	else
	  ggzdexecmoddir="\${libdir}/ggzd"
	  ggzdexecmodpath="${libdir}/ggzd"
	fi
	AC_SUBST(ggzdexecmoddir)
	AC_SUBST(ggzdexecmodpath)

	# Perform actions given by argument 1.
	$1
fi

])

dnl ------------------------------------------------------------------------
dnl AC_GGZ_CHECK_SERVER
dnl   Check for presence of GGZ server libraries.
dnl
dnl   Simply call this function in programs that use GGZ.  GGZ_SERVER will
dnl   be #defined in config.h, and created as a conditional
dnl   in Makefile.am files, if server libraries are present.
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_CHECK_SERVER],
[
  AC_GGZ_LIBGGZ([try_ggz="yes"], [try_ggz="no"])
  if test "$try_ggz" = "yes"; then
    # For now, version 0.0.14 is required.  This could be an additional
    # parameter.
    AC_GGZ_VERSION([0], [99], [4], [], [try_ggz=no])
  fi

  ggz_server="no"
  AC_ARG_WITH(ggz-server,
              AC_HELP_STRING([--with-ggz-server], [Force GGZ server support]),
              [try_ggz_server=$withval])

  if test "x$try_ggz_server" != "xno"; then
    if test "$try_ggz" = "yes"; then
      # Must pass something as the action-if-failed, or the macro will exit
      AC_GGZ_GGZDMOD([ggz_server="yes"], [ggz_server="no"])
    fi
    if test "$ggz_server" = "yes"; then
      AC_GGZ_SERVER
      AC_DEFINE(GGZ_SERVER, 1, [Server support for GGZ])
    else
      if test "$try_ggz_server" = "yes"; then
        AC_MSG_ERROR([Could not configure GGZ server support. See above messages.])
      fi
    fi
  fi

  AM_CONDITIONAL(GGZ_SERVER, test "$ggz_server" = "yes")
])

dnl ------------------------------------------------------------------------
dnl AC_GGZ_CHECK
dnl   Check for presence of GGZ client and server libraries.
dnl
dnl   Simply call this function in programs that use GGZ.  GGZ_SERVER and
dnl   GGZ_CLIENT will be #defined in config.h, and created as conditionals
dnl   in Makefile.am files.
dnl
dnl   The only argument accepted gives the frontend for client embedding:
dnl      "gtk" => means the libggz-gtk library will be checked
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([AC_GGZ_CHECK],
[
  AC_GGZ_INIT([defaults])

  if test x$ac_ggz_prefix != xno ; then
    AC_GGZ_LIBGGZ([try_ggz="yes"], [try_ggz="no"])
  else
    try_ggz=no
  fi

  if test "$try_ggz" = "yes"; then
    # For now, version 0.0.14 is required.  This could be an additional
    # parameter.
    AC_GGZ_VERSION([0], [0], [14], [], [try_ggz=no])
  fi

  ggz_client="no"
  AC_ARG_WITH(ggz-client,
              AC_HELP_STRING([--with-ggz-client], [Force GGZ client support]),
              [try_ggz_client=$withval])

  if test "x$try_ggz_client" != "xno"; then
    if test "$try_ggz" = "yes"; then
      # Must pass something as the action-if-failed, or the macro will exit
      AC_GGZ_GGZMOD([AC_GGZ_CONFIG([ggz_client="yes"], [ggz_client="no"])],
                    [ggz_client="no"])
    fi
    if test "$ggz_client" = "yes"; then
      AC_DEFINE(GGZ_CLIENT, 1, [Client support for GGZ])
    else
      if test "$try_ggz_client" = "yes"; then
        AC_MSG_ERROR([Could not configure GGZ client support. See above messages.])
      fi
    fi
  fi

  ggz_gtk="no"
  if test "$ggz_client" = "yes"; then
    if test "x$1" = "xgtk"; then
      AC_GGZ_GTK([ggz_gtk="yes"])
      if test $ggz_gtk = "yes"; then
        AC_DEFINE(GGZ_GTK, 1, [Support for embedded GGZ through libggz-gtk])
      fi
    fi
  fi

  AM_CONDITIONAL(GGZ_CLIENT, test "$ggz_client" = "yes")
  AM_CONDITIONAL(GGZ_GTK, test "$ggz_gtk" = "yes")

  AC_GGZ_CHECK_SERVER
])
