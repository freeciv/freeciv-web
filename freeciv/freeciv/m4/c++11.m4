# Check for the presence of C++11 features.

# Check for C++11 static_assert
#
AC_DEFUN([FC_CXX11_STATIC_ASSERT],
[
  if test "x$cxx_works" = "xyes" ; then
    AC_CACHE_CHECK([for C++11 static assert], [ac_cv_cxx11_static_assert],
      [AC_LANG_PUSH([C++])
       AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <assert.h>
]], [[ static_assert(1, "1 is not true"); ]])],
[ac_cv_cxx11_static_assert=yes], [ac_cv_cxx11_static_assert=no])
       AC_LANG_POP([C++])])
    if test "x${ac_cv_cxx11_static_assert}" = "xyes" ; then
      AC_DEFINE([FREECIV_CXX11_STATIC_ASSERT], [1], [C++11 static assert supported])
    fi
  fi
])

# Check for C++11 nullptr, and define nullptr as '0' if it's missing
#
AC_DEFUN([FC_CXX11_NULLPTR],
[
  if test "x$cxx_works" = "xyes" ; then
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([cstddef])
    AC_CACHE_CHECK([for C++11 nullptr], [ac_cv_cxx11_nullptr],
      [AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#ifdef HAVE_CSTDDEF
#include <cstddef>
#endif]],
 [[ int *var = nullptr; ]])],
[ac_cv_cxx11_nullptr=yes], [ac_cv_cxx11_nullptr=no])])
    if test "x${ac_cv_cxx11_nullptr}" != "xyes" ; then
      AC_DEFINE([nullptr], [0], [Fallback since C++11 nullptr not available])
    fi
    AC_LANG_POP([C++])
  fi
])
