# Check for mysql
#
# http://ac-archive.sourceforge.net/ac-archive/ax_lib_mysql.html
#
# FC_CHECK_MYSQL([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND[, VERSION]]])

AC_DEFUN([FC_CHECK_MYSQL],
[
  AC_ARG_WITH([mysql-prefix],
    AS_HELP_STRING([--with-mysql-prefix=PFX], [Prefix where MySql is installed (optional)]),
[mysql_prefix="$withval"], [mysql_prefix=""])

  mysql_cflags=""
  mysql_ldflags=""
  MYSQL_VERSION=""

  dnl
  dnl Check MySQL libraries (libmysql)
  dnl

  if test -z "$MYSQL_CONFIG" -o test; then
    AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], [no])
  fi

  if test "$MYSQL_CONFIG" != "no"; then
    AC_MSG_CHECKING([for MySQL libraries])

    mysql_cflags="`$MYSQL_CONFIG --cflags`"
    mysql_ldflags="`$MYSQL_CONFIG --libs`"
    MYSQL_VERSION=`$MYSQL_CONFIG --version`

    # remove NDEBUG from MYSQL_CFLAGS
    mysql_cflags=`echo $mysql_cflags | $SED -e 's/-DNDEBUG//g'`

    found_mysql="yes"
    AC_MSG_RESULT([yes])

  fi

  dnl
  dnl Check if required version of MySQL is available
  dnl


  mysql_version_req=ifelse([$3], [], [], [$3])

  if test "$found_mysql" = "yes" -a -n "$mysql_version_req"; then

    AC_MSG_CHECKING([if MySQL version is &gt;= $mysql_version_req])

    dnl Decompose required version string of MySQL
    dnl and calculate its number representation
    mysql_version_req_major=`expr $mysql_version_req : '\([[0-9]]*\)'`
    mysql_version_req_minor=`expr $mysql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
    mysql_version_req_micro=`expr $mysql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$mysql_version_req_micro" = "x"; then
      mysql_version_req_micro="0"
    fi

    mysql_version_req_number=`expr $mysql_version_req_major \* 1000000 \
                              \+ $mysql_version_req_minor \* 1000 \
                              \+ $mysql_version_req_micro`

    dnl Decompose version string of installed MySQL
    dnl and calculate its number representation
    mysql_version_major=`expr $MYSQL_VERSION : '\([[0-9]]*\)'`
    mysql_version_minor=`expr $MYSQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
    mysql_version_micro=`expr $MYSQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$mysql_version_micro" = "x"; then
      mysql_version_micro="0"
    fi

    mysql_version_number=`expr $mysql_version_major \* 1000000 \
                          \+ $mysql_version_minor \* 1000 \
                          \+ $mysql_version_micro`

    mysql_version_check=`expr $mysql_version_number \&gt;\= $mysql_version_req_number`
    if test "$mysql_version_check" = "1"; then
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
    fi
  fi

  AC_SUBST([MYSQL_VERSION])
  AC_SUBST([mysql_cflags])
  AC_SUBST([mysql_ldflags])

  if test "x$found_mysql" = "xyes" ; then
    ifelse([$1], , :, [$1])
  else
    ifelse([$2], , :, [$2])
  fi
])
