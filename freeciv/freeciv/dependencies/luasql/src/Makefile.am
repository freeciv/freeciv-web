## Process this file with automake to produce Makefile.in

## Must be set before adding anything
noinst_LTLIBRARIES =

AM_CPPFLAGS = $(LUA_CFLAGS) $(FCDB_SQLITE3_CFLAGS) $(FCDB_MYSQL_CFLAGS) $(FCDB_POSTGRES_CFLAGS)

if FCDB_DB
noinst_LTLIBRARIES += libluasql_base.la
libluasql_base_la_SOURCES = \
		luasql.h	\
		luasql.c
endif

if FCDB_MYSQL
noinst_LTLIBRARIES += libluasql_mysql.la
libluasql_mysql_la_SOURCES = \
		ls_mysql.h	\
		ls_mysql.c
endif

if FCDB_POSTGRES
noinst_LTLIBRARIES += libluasql_postgres.la
libluasql_postgres_la_SOURCES = \
		ls_postgres.h	\
		ls_postgres.c
endif

if FCDB_SQLITE3
noinst_LTLIBRARIES += libluasql_sqlite3.la
libluasql_sqlite3_la_SOURCES = \
		ls_sqlite3.h	\
		ls_sqlite3.c
endif
