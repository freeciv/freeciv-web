# Check for Freeciv Authentication using mysql
#
# Called without any parameters.

AC_DEFUN([FC_FCDB_MYSQL],
[
  if test "x$fcdb_all" = "xyes" || test "x$fcdb_mysql" = "xyes" ; then

    FC_CHECK_MYSQL(
    [
      FCDB_MYSQL_CFLAGS="$mysql_cflags"
      FCDB_MYSQL_LIBS="$mysql_ldflags"

      AC_SUBST(FCDB_MYSQL_CFLAGS)

      AC_DEFINE([HAVE_FCDB_MYSQL], [1], [Have MySQL database backend])
      fcdb_mysql=yes
    ],
    [
      if test "x$fcdb_mysql" = "xyes" ; then
        AC_MSG_ERROR([fcdb database mysql not available])
      fi
      fcdb_mysql=no
    ])

  fi

  AM_CONDITIONAL(FCDB_MYSQL, test "x$fcdb_mysql" = "xyes")
])
