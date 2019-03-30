# Check for postgres
#
# http://ac-archive.sourceforge.net/ac-archive/ax_lib_postgresql.html
#
# FC_CHECK_POSTGRES([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND[, VERSION]]])

AC_DEFUN([FC_CHECK_POSTGRES],
[
  AC_ARG_WITH([postgres-prefix],
    AS_HELP_STRING([--with-postgres-prefix=PFX], [Prefix where PostgreSQL is installed (optional)]),
[postgres_prefix="$withval"], [postgres_prefix=""])

  postgresql_cflags=""
  postgresql_ldflags=""
  POSTGRESQL_POSTGRESQL=""

  dnl
  dnl Check PostgreSQL libraries (libpq)
  dnl

  if test -z "$PG_CONFIG" -o test; then
    AC_PATH_PROG([PG_CONFIG], [pg_config], [no])
  fi

  if test "$PG_CONFIG" != "no"; then
    AC_MSG_CHECKING([for PostgreSQL libraries])

    postgresql_cflags="-I`$PG_CONFIG --includedir`"
    postgresql_ldflags="-L`$PG_CONFIG --libdir` -lpq"
    POSTGRESQL_VERSION=`$PG_CONFIG --version | sed -e 's#PostgreSQL ##'`

    found_postgresql="yes"
    AC_MSG_RESULT([yes])
  fi

  dnl
  dnl Check if required version of PostgreSQL is available
  dnl


  postgresql_version_req=ifelse([$3], [], [], [$3])

  if test "$found_postgresql" = "yes" -a -n "$postgresql_version_req"; then

    AC_MSG_CHECKING([if PostgreSQL version is &gt;= $postgresql_version_req])

    dnl Decompose required version string of PostgreSQL
    dnl and calculate its number representation
    postgresql_version_req_major=`expr $postgresql_version_req : '\([[0-9]]*\)'`
    postgresql_version_req_minor=`expr $postgresql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
    postgresql_version_req_micro=`expr $postgresql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$postgresql_version_req_micro" = "x"; then
        postgresql_version_req_micro="0"
    fi

    postgresql_version_req_number=`expr $postgresql_version_req_major \* 1000000 \
                                   \+ $postgresql_version_req_minor \* 1000 \
                                   \+ $postgresql_version_req_micro`

    dnl Decompose version string of installed PostgreSQL
    dnl and calculate its number representation
    postgresql_version_major=`expr $POSTGRESQL_VERSION : '\([[0-9]]*\)'`
    postgresql_version_minor=`expr $POSTGRESQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
    postgresql_version_micro=`expr $POSTGRESQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$postgresql_version_micro" = "x"; then
        postgresql_version_micro="0"
    fi

    postgresql_version_number=`expr $postgresql_version_major \* 1000000 \
                               \+ $postgresql_version_minor \* 1000 \
                               \+ $postgresql_version_micro`

    postgresql_version_check=`expr $postgresql_version_number \&gt;\= $postgresql_version_req_number`
    if test "$postgresql_version_check" = "1"; then
        AC_MSG_RESULT([yes])
    else
        AC_MSG_RESULT([no])
    fi
  fi

  AC_SUBST([POSTGRESQL_VERSION])
  AC_SUBST([postgresql_cflags])
  AC_SUBST([postgresql_ldflags])

  if test "x$found_postgresql" = "xyes" ; then
    ifelse([$1], , :, [$1])
  else
    ifelse([$2], , :, [$2])
  fi
])
