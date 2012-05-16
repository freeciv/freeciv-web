## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libscripting.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility -I$(top_srcdir)/common \
	-I$(top_srcdir)/server \
	$(LUA_CFLAGS) $(TOLUA_CFLAGS)

# api_gen.[ch] are now distributed to aid in cross-compiling.  See PR#13571.
dist_libscripting_la_SOURCES = \
		api_actions.c	\
		api_actions.h	\
		api_find.c	\
		api_find.h	\
		api_gen.c	\
		api_gen.h	\
		api_intl.c	\
		api_intl.h	\
		api_methods.c	\
		api_methods.h	\
		api_notify.c	\
		api_notify.h	\
		api_utilities.c	\
		api_utilities.h	\
		api_types.h	\
		script.c	\
		script.h	\
		script_signal.c	\
		script_signal.h

EXTRA_DIST = \
		api.pkg

BUILT_SOURCES = api_gen.c api_gen.h

api_gen.c api_gen.h: api.pkg
	$(TOLUA) -n api -o $(srcdir)/api_gen.c -H $(srcdir)/api_gen.h $(srcdir)/api.pkg
