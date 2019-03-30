# Check for sqlite3
#
# http://ac-archive.sourceforge.net/ac-archive/ax_lib_sqlite3.html
# Modified for freeciv use.
#
# FC_CHECK_SQLITE3([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND] [,VERSION]])

AC_DEFUN([FC_CHECK_SQLITE3],
[
  AC_ARG_WITH([sqlite3-prefix],
    AS_HELP_STRING([--with-sqlite3-prefix=PFX], [Prefix where SQLite3 is installed (optional)]),
[sqlite3_prefix="$withval"], [sqlite3_prefix=""])

  sqlite3_cflags=""
  sqlite3_ldflags=""
  SQLITE3_VERSION=""

  ac_sqlite3_header="sqlite3.h"

  sqlite3_version_req=ifelse([$3], [], [3.0.0], [$3])
  sqlite3_version_req_shorten=`expr $sqlite3_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
  sqlite3_version_req_major=`expr $sqlite3_version_req : '\([[0-9]]*\)'`
  sqlite3_version_req_minor=`expr $sqlite3_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
  sqlite3_version_req_micro=`expr $sqlite3_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
  if test "x$sqlite3_version_req_micro" = "x" ; then
    sqlite3_version_req_micro="0"
  fi

  sqlite3_version_req_number=`expr $sqlite3_version_req_major \* 1000000 \
                              \+ $sqlite3_version_req_minor \* 1000 \
                              \+ $sqlite3_version_req_micro`

  AC_MSG_CHECKING([for SQLite3 library >= $sqlite3_version_req])

  if test "$sqlite3_prefix" != ""; then
    ac_sqlite3_ldflags="-L$sqlite3_prefix/lib"
    ac_sqlite3_cppflags="-I$sqlite3_prefix/include"
  else
    for sqlite3_prefix_tmp in /usr /usr/local /opt ; do
      if test -f "$sqlite3_prefix_tmp/include/$ac_sqlite3_header" \
         && test -r "$sqlite3_prefix_tmp/include/$ac_sqlite3_header"; then
        sqlite3_prefix=$sqlite3_prefix_tmp
        ac_sqlite3_cppflags="-I$sqlite3_prefix_tmp/include"
        ac_sqlite3_ldflags="-L$sqlite3_prefix_tmp/lib"
        break;
      fi
    done
  fi

  ac_sqlite3_ldflags="$ac_sqlite3_ldflags -lsqlite3"

  saved_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $ac_sqlite3_cppflags"

  AC_COMPILE_IFELSE(
    [
      AC_LANG_PROGRAM([[@%:@include <sqlite3.h>]],[[
#if (SQLITE_VERSION_NUMBER >= $sqlite3_version_req_number)
// Everything is okay
#else
#  error SQLite version is too old
#endif
      ]])
    ],
    [
      AC_MSG_RESULT([yes])
      success="yes"
    ],
    [
      AC_MSG_RESULT([not found])
      success="no"
    ]
  )

  CPPFLAGS="$saved_CPPFLAGS"

  if test "$success" = "yes"; then
    sqlite3_cflags="$ac_sqlite3_cppflags"
    sqlite3_ldflags="$ac_sqlite3_ldflags"

    ac_sqlite3_header_path="$sqlite3_prefix/include/$ac_sqlite3_header"

    dnl Retrieve SQLite release version
    if test "x$ac_sqlite3_header_path" != "x"; then
      ac_sqlite3_version=`cat $ac_sqlite3_header_path \
                          | grep '#define.*SQLITE_VERSION.*\"' | $SED -e 's/.* "//' \
                          | $SED -e 's/"//'`
      if test $ac_sqlite3_version != ""; then
        SQLITE3_VERSION=$ac_sqlite3_version
      else
        AC_MSG_WARN([Can not find SQLITE_VERSION macro in sqlite3.h header to retrieve SQLite version!])
      fi
    fi

    AC_SUBST(sqlite3_cflags)
    AC_SUBST(sqlite3_ldflags)
    AC_SUBST(SQLITE3_VERSION)

    ifelse([$1], , :, [$1])
  else
    ifelse([$2], , :, [$2])
  fi
])
