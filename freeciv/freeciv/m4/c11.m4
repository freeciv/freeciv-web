# Check for the presence of C11 features.

# Check for C11 _Static_assert
#
AC_DEFUN([FC_C11_STATIC_ASSERT],
[
  AC_CACHE_CHECK([for C11 static assert], [ac_cv_c11_static_assert],
    [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <assert.h>
]], [[ _Static_assert(1, "1 is not true"); ]])],
[ac_cv_c11_static_assert=yes], [ac_cv_c11_static_assert=no])])
  if test "x${ac_cv_c11_static_assert}" = "xyes" ; then
    AC_DEFINE([FREECIV_C11_STATIC_ASSERT], [1], [C11 static assert supported])
  fi
])

AC_DEFUN([FC_STATIC_STRLEN],
[
  AC_REQUIRE([FC_C11_STATIC_ASSERT])
  if test "x${ac_cv_c11_static_assert}" = "xyes" ; then
    AC_CACHE_CHECK([for strlen() usability in static assert], [ac_cv_static_strlen],
      [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <assert.h>
#include <string.h>
static const char str[] = "12345";
]], [[_Static_assert(5 == strlen(str), "Wrong length"); ]])],
[ac_cv_static_strlen=yes], [ac_cv_static_strlen=no])])
    if test "x${ac_cv_static_strlen}" = "xyes" ; then
      AC_DEFINE([FREECIV_STATIC_STRLEN], [1], [strlen() in static assert supported])
    fi
  fi
])

AC_DEFUN([FC_C11_AT_QUICK_EXIT],
[
  AC_CACHE_CHECK([for C11 at_quick_exit()], [ac_cv_c11_at_quick_exit], [
    dnl Add -Werror to detect cases where the header does not declare
    dnl at_quick_exit() but linking still work. This situation can happen
    dnl when the header is strict about the fact that at_quick_exit() is
    dnl a C11 feature and the compiler is not in C11 mode.
    dnl $EXTRA_DEBUG_CFLAGS contains -Wmissing-declarations if it's supported.
    fc_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -Werror $EXTRA_DEBUG_CFLAGS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>
static void func(void)
{}
]], [[ at_quick_exit(func); ]])],
[ac_cv_c11_at_quick_exit=yes], [ac_cv_c11_at_quick_exit=no])
    CPPFLAGS="$fc_save_CPPFLAGS"])
  if test "x${ac_cv_c11_at_quick_exit}" = "xyes" ; then
    AC_DEFINE([HAVE_AT_QUICK_EXIT], [1], [C11 at_quick_exit() available])
  fi
])
