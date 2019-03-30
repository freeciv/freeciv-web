# Macros to help with winsock2 setup
#
# serial 1

# Prepend winsock2.h to default includes if possible.
# Set HAVE_WINSOCK2 if winsock2.h was found.
#
# FC_WINSOCK2_INCLUDE([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
#
AC_DEFUN([FC_WINSOCK2_INCLUDE],
[
  dnl We have to poke autoconf internals so we get correct include order for the tests
  _backup_includes_default="${ac_includes_default}"
  ac_includes_default="\
#include <winsock2.h>
${ac_includes_default}"
  AC_CHECK_HEADER([winsock2.h], [$1], [ac_includes_default="$_backup_includes_default"
$2])
])
