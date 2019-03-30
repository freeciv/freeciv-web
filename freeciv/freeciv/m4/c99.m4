# Check for the presence of C99 features.  Generally the check will fail
# if the feature isn't present (a C99 compiler isn't that much to ask,
# right?).

# Check C99-style variadic macros (required):
#
#  #define PRINTF(msg, ...) (printf(msg, __VA_ARGS__)
#
AC_DEFUN([FC_C99_VARIADIC_MACROS],
[
  dnl Check for variadic macros
  AC_CACHE_CHECK([for C99 variadic macros],
    [ac_cv_c99_variadic_macros],
     [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
           #define MSG(...) fprintf(stderr, __VA_ARGS__)
          ]], [[MSG("foo");
           MSG("%s", "foo");
           MSG("%s%d", "foo", 1);]])],[ac_cv_c99_variadic_macros=yes],[ac_cv_c99_variadic_macros=no])])
  if test "x${ac_cv_c99_variadic_macros}" != "xyes"; then
    AC_MSG_ERROR([A compiler supporting C99 variadic macros is required])
  fi
])

# Check C99-style initializers (required):
#
# Examples:
#   struct timeval tv = {.tv_sec = 0, .tv_usec = 500000};
#   int fibonacci[6] = {[0] = 0, [1] = 1, [2] = 1, [3] = 2, [4] = 3, [5] = 5};
# Note we do not check for multi-field initializers like
#   struct { struct { int b; } a; } = {.a.b = 5}
# which are not supported by many compilers.  It is best to avoid this
# problem by writing these using nesting.  The above case becomes
#   struct { struct { int b; } a; } = {.a = {.b = 5}}
AC_DEFUN([FC_C99_INITIALIZERS],
[
  dnl Check for C99 initializers
  AC_CACHE_CHECK([for C99 initializers],
    [ac_cv_c99_initializers],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[struct foo {
           int an_integer;
           char *a_string;
           int an_array[5];
           union {int x, y;} a_union;
         };
        ]], [[struct foo bar = {.an_array = {0, [3] = 2, [2] = 1, [4] = 3},
                           .an_integer = 999,
                           .a_string = "does it work?",
                           .a_union = {.y = 243}};]])],[ac_cv_c99_initializers=yes],[ac_cv_c99_initializers=no])])
  if test "${ac_cv_c99_initializers}" != "yes"; then
    AC_MSG_ERROR([A compiler supporting C99 initializers is required])
  fi
])

# Check C99-style stdint.h (required)
AC_DEFUN([FC_C99_STDINT_H],
[
  AC_CHECK_HEADERS([stdint.h])
  dnl Check for C99 stdint.h
  AC_CACHE_CHECK([for C99 stdint.h],
    [ac_cv_c99_stdint_h],
    [ac_cv_c99_stdint_h=$ac_cv_header_stdint_h])
  if test "${ac_cv_c99_stdint_h}" != "yes"; then
    AC_MSG_ERROR([A compiler supporting C99's stdint.h is required])
  fi
])

# Check that token concenation works as we expect
#
AC_DEFUN([FC_C99_TOKEN_CONCENATION],
[
AC_CACHE_CHECK([whether preprocessor token concenation works],
  [ac_cv_c99_token_concenation],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#define COMBINE(a, b) a ## b
  #define CALLER(...) COMBINE(, __VA_ARGS__)]],
    [[CALLER();
    char *text = CALLER("string");]])],
  [ac_cv_c99_token_concenation=yes],[ac_cv_c99_token_concenation=no])])
  if test "x${ac_cv_c99_token_concenation}" != "xyes" ; then
    AC_MSG_ERROR([A preprocessor supporting token concenation is required])
  fi
])

# Whether C99-style initializers of a struct can, or even must, be
# within braces.
# Sets macros INIT_BRACE_BEGIN and INIT_BRACE_END accordingly.
#
AC_DEFUN([FC_C99_INITIALIZER_BRACES],
[
AC_CACHE_CHECK([can struct initializers be within braces],
  [ac_cv_c99_initializer_braces],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
    [[
struct outer
{
  int v1;
  int v2;
  union
  {
    int v3;
    struct
    {
      int v4;
      int v5;
    } inner;
  };
};

  struct outer init_me = { 1, 2, { .inner = { 3, 4 }}}
]])],
  [ac_cv_c99_initializer_braces=yes], [ac_cv_c99_initializer_braces=no])])
  if test "x${ac_cv_c99_initializer_braces}" = "xyes" ; then
    AC_DEFINE([INIT_BRACE_BEGIN], [{], [Beginning of C99 structure initializer])
    AC_DEFINE([INIT_BRACE_END], [}], [End of C99 structure initializer])
  else
    AC_DEFINE([INIT_BRACE_BEGIN], [], [Beginning of C99 structure initializer])
    AC_DEFINE([INIT_BRACE_END], [], [End of C99 structure initializer])
  fi
])
