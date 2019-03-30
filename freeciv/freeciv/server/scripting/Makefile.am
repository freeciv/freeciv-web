## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = \
	libscripting_fcdb.la \
	libscripting_server.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/common/scriptcore \
	-I$(top_srcdir)/server \
	-I$(top_srcdir)/server/generator \
	-I$(top_srcdir)/ai \
	-I$(top_srcdir)/dependencies/tinycthread \
	$(FCDB_MYSQL_CFLAGS) $(FCDB_POSTGRES_CFLAGS) $(FCDB_SQLITE_CFLAGS) \
	$(LUA_CFLAGS) $(LUASQL_CFLAGS) $(TOLUA_CFLAGS)

# tolua_[fcdb|server]_gen.[ch] are now distributed to aid in cross-compiling.
# See PR#13571.
dist_libscripting_server_la_SOURCES = \
	api_server_base.c	\
	api_server_base.h	\
	api_server_edit.c	\
	api_server_edit.h	\
	api_server_game_methods.c	\
	api_server_game_methods.h	\
	api_server_luadata.c	\
	api_server_luadata.h	\
	api_server_notify.c	\
	api_server_notify.h	\
	script_server.c		\
	script_server.h		\
	tolua_server_gen.c	\
	tolua_server_gen.h

dist_libscripting_fcdb_la_SOURCES = \
	api_fcdb_auth.c		\
	api_fcdb_auth.h		\
	api_fcdb_base.c		\
	api_fcdb_base.h		\
	script_fcdb.c		\
	script_fcdb.h		\
	tolua_fcdb_gen.c	\
	tolua_fcdb_gen.h

EXTRA_DIST = \
	tolua_fcdb.pkg		\
	tolua_server.pkg

BUILT_SOURCES = \
	tolua_fcdb_gen.c	\
	tolua_fcdb_gen.h	\
	tolua_server_gen.c	\
	tolua_server_gen.h

# LUA_LIBS & co are not LIBADDedded to the library since having them
# both from shared libfreeciv.so (common/scriptcore) and static library
# here would break build when libfreeciv.so is shared library. Here we
# can only rely on them getting included via libfreeciv.so.

tolua_fcdb_gen.c tolua_fcdb_gen.h: tolua_fcdb.pkg
	$(TOLUA) -n fcdb -o $(srcdir)/tolua_fcdb_gen.c \
	-H $(srcdir)/tolua_fcdb_gen.h $(srcdir)/tolua_fcdb.pkg

tolua_server_gen.c tolua_server_gen.h: tolua_server.pkg
	$(TOLUA) -n server -o $(srcdir)/tolua_server_gen.c \
	-H $(srcdir)/tolua_server_gen.h $(srcdir)/tolua_server.pkg

# These files are not generated to builddir, but to srcdir */
MAINTAINERCLEANFILES = \
	$(srcdir)/tolua_fcdb_gen.c \
	$(srcdir)/tolua_fcdb_gen.h \
	$(srcdir)/tolua_server_gen.c \
	$(srcdir)/tolua_server_gen.h
