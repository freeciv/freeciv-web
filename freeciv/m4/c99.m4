# Check for the presence of C99 features.  Generally the check will fail
# if the feature isn't present (a C99 compiler isn't that much to ask,
# right?).

# Check C99-style variadic macros (required):
#
#  #define PRINTF(msg, ...) (printf(msg, __VA_ARGS__)
#
AC_DEFUN([AC_C99_VARIADIC_MACROS],
[
  dnl Check for variadic macros
  AC_CACHE_CHECK([for C99 variadic macros],
    [ac_cv_c99_variadic_macros],
     [AC_TRY_COMPILE(
          [#include <stdio.h>
           #define MSG(...) fprintf(stderr, __VA_ARGS__)
          ],
          [MSG("foo");
           MSG("%s", "foo");
           MSG("%s%d", "foo", 1);],
          ac_cv_c99_variadic_macros=yes,
          ac_cv_c99_variadic_macros=no)])
  if test "x${ac_cv_c99_variadic_macros}" != "xyes"; then
    AC_MSG_ERROR([A compiler supporting C99 variadic macros is required])
  fi
])

# Check C99-style variable-sized arrays (required):
#
#   char concat_str[strlen(s1) + strlen(s2) + 1];
#
AC_DEFUN([AC_C99_VARIABLE_ARRAYS],
[
  dnl Check for variable arrays
  AC_CACHE_CHECK([for C99 variable arrays],
    [ac_cv_c99_variable_arrays],
    [AC_TRY_COMPILE(
        [
#include <string.h>
#include <stdio.h>
],
        [char *s1 = "foo", *s2 = "bar";
         char s3[strlen(s1) + strlen(s2) + 1];
         sprintf(s3, "%s%s", s1, s2);],
        ac_cv_c99_variable_arrays=yes,
        ac_cv_c99_variable_arrays=no)])
  if test "x${ac_cv_c99_variable_arrays}" != "xyes"; then
    AC_MSG_ERROR([A compiler supporting C99 variable arrays is required])
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
AC_DEFUN([AC_C99_INITIALIZERS],
[
  dnl Check for C99 initializers
  AC_CACHE_CHECK([for C99 initializers],
    [ac_cv_c99_initializers],
    [AC_TRY_COMPILE(
        [struct foo {
           int an_integer;
           char *a_string;
           int an_array[5];
           union {int x, y;} a_union;
         };
        ],
        [struct foo bar = {.an_array = {0, [3] = 2, [2] = 1, [4] = 3},
                           .an_integer = 999,
                           .a_string = "does it work?",
                           .a_union = {.y = 243}};],
        [ac_cv_c99_initializers=yes],
        [ac_cv_c99_initializers=no])])
  if test "${ac_cv_c99_initializers}" != "yes"; then
    AC_MSG_ERROR([A compiler supporting C99 initializers is required])
  fi
])

# Check C99-style stdint.h (required)
AC_DEFUN([AC_C99_STDINT_H],
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
