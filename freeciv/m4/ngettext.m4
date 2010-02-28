dnl FC_CHECK_NGETTEXT_RUNTIME(EXTRA-LIBS, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
dnl
dnl This tests whether ngettext works at runtime.  Here, "works"
dnl means "doesn't dump core", as some versions (for exmaple the 
dnl version which comes with glibc 2.2.5 is broken, gettext 
dnl version 0.10.38 however is ok).

AC_DEFUN([FC_CHECK_NGETTEXT_RUNTIME],
[
templibs="$LIBS"
LIBS="$1 $LIBS"
templang="$LANG"
LANG="de_DE"
AC_TRY_RUN([
/*
 * Check to make sure that ngettext works at runtime. Specifically,
 * some gettext versions dump core if the ngettext function is called.
 * (c) 2002 Raimar Falke <rfalke@freeciv.org>
 */
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  setlocale(LC_ALL, "");

  if (strcmp(ngettext("unit", "units", 1), "unit") == 0 &&
      strcmp(ngettext("unit", "units", 2), "units") == 0) {
    return 0;
  } else {
    return 1;
  }
}
],
[AC_MSG_RESULT(yes)
  [$2]],
[AC_MSG_RESULT(no)
  [$3]],
[AC_MSG_RESULT(unknown: cross-compiling)
  [$2]])
LIBS="$templibs"
LANG="$templang"
])
