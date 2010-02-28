
dnl FC_CHECK_X_LIB(LIBRARY, FUNCTION [, ACTION-IF-FOUND [,
dnl   ACTION-IF-NOT-FOUND]])
dnl
dnl This macro is intended to search for X11-related libraries.  It takes the
dnl following variables for input:
dnl   X_LIBS		-- prefixed to all linker lines
dnl   X_EXTRA_LIBS	-- suffixed to all linker lines
dnl   LIBS		-- suffixed to all linker lines (after X_EXTRA_LIBS)
dnl Thus, the trial linker line will be "$X_LIBS -l$1 $X_EXTRA_LIBS $LIBS".
dnl
dnl The following variables are output:
dnl   X_EXTRA_LIBS	-- contains "-l$1 $X_EXTRA_LIBS" if the link succeeds
dnl
dnl Thus, the intended usage of this macro is something like this:
dnl   AC_PATH_XTRA
dnl   X_LIBS="$X_LIBS $X_PRE_LIBS"
dnl	dnl Is it just me or is AC_PATH_XTRA broken?
dnl   FC_CHECK_X_LIB(X11, XOpenDisplay, , AC_MSG_ERROR("Need X11"))
dnl   FC_CHECK_X_LIB(Xext, XShapeCombineMask)
dnl     [etc.]
dnl   LIBS="$X_LIBS $X_EXTRA_LIBS $LIBS"
dnl
AC_DEFUN([FC_CHECK_X_LIB], [
 AC_MSG_CHECKING([for $2 in X library -l$1])

 dnl Use a cache variable name containing both the library and function name,
 dnl because the test really is for library $1 defining function $2, not
 dnl just for library $1.  Separate tests with the same $1 and different $2s
 dnl may have different results.

 ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
 AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
  [ac_save_LIBS="$LIBS"
   LIBS="$X_LIBS -l$1 $X_EXTRA_LIBS $LIBS"
   AC_TRY_LINK(dnl
    ifelse([$2], [main], ,
     [#ifdef __cplusplus
       extern "C"
      #endif]
     [/* We use char because int might match the return type of a gcc2
      builtin and then its argument prototype would still apply.  */
      char $2();]
    )
   , [$2()],
   eval "ac_cv_lib_$ac_lib_var=yes",
   eval "ac_cv_lib_$ac_lib_var=no")
   LIBS="$ac_save_LIBS"
  ])dnl
 if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$3], ,
   [changequote(, )dnl
    ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
    changequote([, ])dnl

    # The HAVE_LIBX** values are defined in FC_CHECK_X_LIB, but we need an
    # AH_TEMPLATE for them so that autoheader will know about them.
    AH_TEMPLATE([HAVE_LIBX11], [Define if the X11 lib is available])
    AH_TEMPLATE([HAVE_LIBXEXT], [Define if the Xext lib is available])
    AH_TEMPLATE([HAVE_LIBXT], [Define if the Xt lib is available])
    AH_TEMPLATE([HAVE_LIBXMU], [Define if the Xmu lib is available])
    AH_TEMPLATE([HAVE_LIBXPM], [Define if the Xpm lib is available])
    AH_TEMPLATE([HAVE_LIBXAW], [Define if the Xaw lib is to be used])
    AH_TEMPLATE([HAVE_LIBXAW3D], [Define if the Xaw3d lib is to be used])
    if (test $ac_tr_lib == HAVE_LIBX11     \
        || test $ac_tr_lib == HAVE_LIBXEXT \
        || test $ac_tr_lib == HAVE_LIBXT   \
        || test $ac_tr_lib == HAVE_LIBXMU  \
        || test $ac_tr_lib == HAVE_LIBXPM  \
        || test $ac_tr_lib == HAVE_LIBXAW  \
        || test $ac_tr_lib == HAVE_LIBXAW3D); then
      AC_DEFINE_UNQUOTED($ac_tr_lib)
      X_EXTRA_LIBS="-l$1 $X_EXTRA_LIBS"
    else
      AC_MSG_ERROR([Invalid define of $ac_tr_lib in $1])
    fi
   ], [$3])
 else
  AC_MSG_RESULT(no)
 ifelse([$4], , , [$4
 ])dnl
 fi
])

dnl FC_EXPAND_DIR(VARNAME, DIR)
dnl expands occurrences of ${prefix} and ${exec_prefix} in the given DIR,
dnl and assigns the resulting string to VARNAME
dnl example: FC_EXPAND_DIR(LOCALEDIR, "$datadir/locale")
dnl eg, then: AC_DEFINE_UNQUOTED(LOCALEDIR, "$LOCALEDIR")
dnl by Alexandre Oliva 
dnl from http://www.cygnus.com/ml/automake/1998-Aug/0040.html
AC_DEFUN([FC_EXPAND_DIR], [
        $1=$2
        $1=`(
            test "x$prefix" = xNONE && prefix="$ac_default_prefix"
            test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
            eval echo \""[$]$1"\"
        )`
])


dnl FC_XPM_PATHS
dnl Allow user to specify extra include/lib paths for Xpm, with
dnl --with-xpm=prefix  --with-xpm-lib=dir  --with-xpm-include=dir
dnl The latter two override the prefix form.
dnl Sets variables xpm_libdir and xpm_incdir
dnl If user supplies a path, use that.
dnl If user specifies "no", set that, meaning "no extra path"
dnl If user specifies "yes" (default), then use /usr/local if it looks
dnl likely, else set to "no".
dnl Doesn't do any cache stuff.
dnl
AC_DEFUN([FC_XPM_PATHS],
[AC_MSG_CHECKING(extra paths for Xpm)
dnl General Xpm prefix:
dnl "no" means no prefix is required, "yes" means try /usr/local
AC_ARG_WITH(xpm-prefix,
    [  --with-xpm-prefix=DIR   Xpm files are in DIR/lib and DIR/include,
                          or use the following to set them separately:],
    xpm_prefix="$withval", 
    xpm_prefix="yes"
)
if test "$xpm_prefix" = "yes" || test "$xpm_prefix" = "no"; then
    xpm_libdir="$xpm_prefix"
    xpm_incdir="$xpm_prefix"
else
    xpm_libdir="$xpm_prefix/lib"
    xpm_incdir="$xpm_prefix/include"
fi
dnl May override general Xpm prefix with explicit individual paths:
AC_ARG_WITH(xpm-lib,
    [  --with-xpm-lib=DIR      Xpm library is in DIR],
    xpm_libdir="$withval" 
)
AC_ARG_WITH(xpm-include,
    [  --with-xpm-include=DIR  Xpm header file is in DIR (that is, DIR/X11/xpm.h)],
    xpm_incdir="$withval" 
)
dnl If xpm-lib path was not specified, try /usr/local/lib if that 
dnl looks likely; we don't actually try to link.
fc_xpm_default=/usr/local
if test "$xpm_libdir" = "yes"; then
    xpm_libdir="no"
    fc_xpm_default_lib="$fc_xpm_default/lib"
    for fc_extension in a so sl; do
        if test -r $fc_xpm_default_lib/libXpm.$fc_extension; then
            xpm_libdir=$fc_xpm_default_lib
            break
        fi
    done
fi
dnl Likewise for xpm-include with /usr/local/include;
dnl we don't actually try to include.
if test "$xpm_incdir" = "yes"; then
    xpm_incdir="no"
    fc_xpm_default_inc="$fc_xpm_default/include"
    if test -r $fc_xpm_default_inc/X11/xpm.h; then
        xpm_incdir=$fc_xpm_default_inc
    elif test -r $fc_xpm_default_inc/xpm.h; then
    	xpm_incdir=$fc_xpm_default_inc
	xpm_h_no_x11=yes
    fi
fi
AC_MSG_RESULT([library $xpm_libdir, include $xpm_incdir])
])


dnl FC_CHECK_X_PROTO_DEFINE(DEFINED-VARIABLE)
dnl
dnl This macro determines the value of the given defined
dnl variable needed by Xfuncproto.h in order to compile correctly.
dnl
dnl Typical DEFINED-VARIABLEs are:
dnl   FUNCPROTO
dnl   NARROWPROTO
dnl
dnl The following variables are output:
dnl   fc_x_proto_value		-- contains the value to which
dnl				the DEFINED-VARIABLE is set,
dnl				or "" if it has no known value.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_DEFINE(FUNCPROTO)
dnl   if test -n "$fc_x_proto_value"; then
dnl     AC_DEFINE_UNQUOTED(FUNCPROTO, $fc_x_proto_value)
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_DEFINE],
[AC_REQUIRE([FC_CHECK_X_PROTO_FETCH])dnl
AC_MSG_CHECKING(for Xfuncproto control definition $1)
# Search for the requested defined variable; return it's value:
fc_x_proto_value=
for fc_x_define in $fc_x_proto_defines; do
  fc_x_val=1
  eval `echo $fc_x_define | sed -e 's/=/ ; fc_x_val=/' | sed -e 's/^/fc_x_var=/'`
  if test "x$fc_x_var" = "x$1"; then
    fc_x_proto_value=$fc_x_val
    break
  fi
done
if test -n "$fc_x_proto_value"; then
  AC_MSG_RESULT([yes: $fc_x_proto_value])
else
  AC_MSG_RESULT([no])
fi
])

dnl FC_CHECK_X_PROTO_FETCH
dnl
dnl This macro fetches the Xfuncproto control definitions.
dnl (Intended to be called once from FC_CHECK_X_PROTO_DEFINE.)
dnl
dnl The following variables are output:
dnl   fc_x_proto_defines	-- contains the list of defines of
dnl				Xfuncproto control definitions
dnl				(defines may or may not include
dnl				the -D prefix, or an =VAL part).
dnl
dnl Example use:
dnl   AC_REQUIRE([FC_CHECK_X_PROTO_FETCH])
dnl
AC_DEFUN([FC_CHECK_X_PROTO_FETCH],
[AC_REQUIRE([AC_PATH_X])dnl
AC_MSG_CHECKING(whether Xfuncproto was supplied)
dnl May override determined defines with explicit argument:
AC_ARG_WITH(x-funcproto,
    [  --with-x-funcproto=DEFS Xfuncproto control definitions are DEFS
                          (e.g.: --with-x-funcproto='FUNCPROTO=15 NARROWPROTO']dnl
)
if test "x$with_x_funcproto" = "x"; then
  fc_x_proto_defines=
  rm -fr conftestdir
  if mkdir conftestdir; then
    cd conftestdir
    # Make sure to not put "make" in the Imakefile rules, since we grep it out.
    cat > Imakefile <<'EOF'
fcfindpd:
	@echo 'fc_x_proto_defines=" ${PROTO_DEFINES}"'
EOF
    if (xmkmf) >/dev/null 2>/dev/null && test -f Makefile; then
      # GNU make sometimes prints "make[1]: Entering...", which would confuse us.
      eval `${MAKE-make} fcfindpd 2>/dev/null | grep -v make | sed -e 's/ -D/ /g'`
      AC_MSG_RESULT([no, found: $fc_x_proto_defines])
      cd ..
      rm -fr conftestdir
    else
      dnl Oops -- no/bad xmkmf... Time to go a-guessing...
      AC_MSG_RESULT([no])
      cd ..
      rm -fr conftestdir
      dnl First, guess something for FUNCPROTO:
      AC_MSG_CHECKING([for compilable FUNCPROTO definition])
      dnl Try in order of preference...
      for fc_x_value in 15 11 3 1 ""; do
	FC_CHECK_X_PROTO_FUNCPROTO_COMPILE($fc_x_value)
	if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
	  break
	fi
      done
      if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
	fc_x_proto_defines="$fc_x_proto_defines FUNCPROTO=$fc_x_proto_FUNCPROTO"
	AC_MSG_RESULT([yes, determined: $fc_x_proto_FUNCPROTO])
      else
	AC_MSG_RESULT([no, cannot determine])
      fi
      dnl Second, guess something for NARROWPROTO:
      AC_MSG_CHECKING([for workable NARROWPROTO definition])
      dnl Try in order of preference...
      for fc_x_value in 1 ""; do
	FC_CHECK_X_PROTO_NARROWPROTO_WORKS($fc_x_value)
	if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
	  break
	fi
      done
      if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
	fc_x_proto_defines="$fc_x_proto_defines NARROWPROTO=$fc_x_proto_NARROWPROTO"
	AC_MSG_RESULT([yes, determined: $fc_x_proto_NARROWPROTO])
      else
	AC_MSG_RESULT([no, cannot determine])
      fi
      AC_MSG_CHECKING(whether Xfuncproto was determined)
      if test -n "$fc_x_proto_defines"; then
	AC_MSG_RESULT([yes: $fc_x_proto_defines])
      else
	AC_MSG_RESULT([no])
      fi
    fi
  else
    AC_MSG_RESULT([no, examination failed])
  fi
else
  fc_x_proto_defines=$with_x_funcproto
  AC_MSG_RESULT([yes, given: $fc_x_proto_defines])
fi
])

dnl FC_CHECK_X_PROTO_FUNCPROTO_COMPILE(FUNCPROTO-VALUE)
dnl
dnl This macro determines whether or not Xfuncproto.h will
dnl compile given a value to use for the FUNCPROTO definition.
dnl
dnl Typical FUNCPROTO-VALUEs are:
dnl   15, 11, 3, 1, ""
dnl
dnl The following variables are output:
dnl   fc_x_proto_FUNCPROTO	-- contains the passed-in
dnl				FUNCPROTO-VALUE if Xfuncproto.h
dnl				compiled, or "no" if it did not.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_FUNCPROTO_COMPILE($fc_x_value)
dnl   if test "x$fc_x_proto_FUNCPROTO" != "xno"; then
dnl     echo Compile using FUNCPROTO=$fc_x_proto_FUNCPROTO
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_FUNCPROTO_COMPILE],
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_LANG_SAVE
AC_LANG_C
fc_x_proto_FUNCPROTO=no
if test "x$1" = "x"; then
  fc_x_compile="#undef FUNCPROTO"
else
  fc_x_compile="#define FUNCPROTO $1"
fi
fc_x_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS"
AC_TRY_COMPILE([
$fc_x_compile
#include <X11/Xfuncproto.h>
  ],[
exit (0)
  ],
  [fc_x_proto_FUNCPROTO=$1])
CFLAGS="$fc_x_save_CFLAGS"
AC_LANG_RESTORE
])

dnl FC_CHECK_X_PROTO_NARROWPROTO_WORKS(NARROWPROTO-VALUE)
dnl
dnl This macro determines whether or not NARROWPROTO is required
dnl to get a typical X function (XawScrollbarSetThumb) to work.
dnl
dnl Typical NARROWPROTO-VALUEs are:
dnl   1, ""
dnl
dnl The following variables are required for input:
dnl   fc_x_proto_FUNCPROTO	-- the value to use for FUNCPROTO.
dnl
dnl The following variables are output:
dnl   fc_x_proto_NARROWPROTO	-- contains the passed-in
dnl				NARROWPROTO-VALUE if the test
dnl				worked, or "no" if it did not.
dnl
dnl Example use:
dnl   FC_CHECK_X_PROTO_NARROWPROTO_WORKS($fc_x_value)
dnl   if test "x$fc_x_proto_NARROWPROTO" != "xno"; then
dnl     echo Compile using NARROWPROTO=$fc_x_proto_NARROWPROTO
dnl   fi
dnl
AC_DEFUN([FC_CHECK_X_PROTO_NARROWPROTO_WORKS],
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_LANG_SAVE
AC_LANG_C
fc_x_proto_NARROWPROTO=no
if test "x$1" = "x"; then
  fc_x_works="#undef NARROWPROTO"
else
  fc_x_works="#define NARROWPROTO $1"
fi
if test "x$fc_x_proto_FUNCPROTO" = "x"; then
  fc_x_compile="#define FUNCPROTO 1"
else
  fc_x_compile="#define FUNCPROTO $fc_x_proto_FUNCPROTO"
fi
fc_x_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS $X_LIBS $X_PRE_LIBS -lXaw -lXt -lX11 $X_EXTRA_LIBS"
AC_TRY_RUN([
$fc_x_works
$fc_x_compile
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Scrollbar.h>
#define TOP_VAL 0.125
#define SHOWN_VAL 0.25
int main (int argc, char ** argv)
{
  Widget toplevel;
  XtAppContext appcon;
  Widget scrollbar;
  double topbuf;
  double shownbuf;
  float * top = (float *)(&topbuf);
  float * shown = (float *)(&shownbuf);
  toplevel =
    XtAppInitialize
    (
     &appcon,
     "FcXTest",
     NULL, 0,
     &argc, argv,
     NULL,
     NULL, 0
    );
  scrollbar =
    XtVaCreateManagedWidget
    (
     "my_scrollbar",
     scrollbarWidgetClass,
     toplevel,
     NULL
    );
  XawScrollbarSetThumb (scrollbar, TOP_VAL, SHOWN_VAL);
  XtVaGetValues
  (
   scrollbar,
   XtNtopOfThumb, top,
   XtNshown, shown,
   NULL
  );
  if ((*top == TOP_VAL) && (*shown == SHOWN_VAL))
    {
      exit (0);
    }
  else
    {
      exit (1);
    }
  return (0);
}
  ],
  [fc_x_proto_NARROWPROTO=$1], [], [:])
CFLAGS="$fc_x_save_CFLAGS"
AC_LANG_RESTORE
])

