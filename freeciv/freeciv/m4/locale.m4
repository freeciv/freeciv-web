#serial 0

AC_DEFUN([AM_LIBCHARSET],
[
  AC_CACHE_CHECK([for libcharset], am_cv_libcharset,
    [lc_save_LIBS="$LIBS"
     LIBS="$LIBS $LIBICONV"
     AC_TRY_LINK([#include <libcharset.h>],
      [locale_charset()],
      am_cv_libcharset=yes,
      am_cv_libcharset=no) 
      LIBS="$lc_save_LIBS" 
    ])
  if test $am_cv_libcharset = yes; then
    AC_DEFINE(HAVE_LIBCHARSET, 1,
      [Define if you have <libcharset.h> and locale_charset().])
  fi
])
