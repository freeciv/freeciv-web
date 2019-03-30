## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = \
	libscriptcore.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/dependencies/tinycthread \
	$(LUA_CFLAGS) $(LUASQL_CFLAGS) $(TOLUA_CFLAGS)

# tolua_[common_a|common_z|game]_gen.[ch] are now distributed to aid in
# cross-compiling.
libscriptcore_la_SOURCES = \
	api_common_intl.c	\
	api_common_intl.h	\
	api_common_utilities.c	\
	api_common_utilities.h	\
	api_game_effects.c	\
	api_game_effects.h	\
	api_game_find.c		\
	api_game_find.h		\
	api_game_methods.c	\
	api_game_methods.h	\
	api_game_specenum.c	\
	api_game_specenum.h	\
	api_signal_base.c	\
	api_signal_base.h	\
	luascript.c		\
	luascript.h		\
	luascript_func.c	\
	luascript_func.h	\
	luascript_signal.c	\
	luascript_signal.h	\
	luascript_types.h	\
	tolua_common_a_gen.c	\
	tolua_common_a_gen.h	\
	tolua_common_z_gen.c	\
	tolua_common_z_gen.h	\
	tolua_game_gen.c	\
	tolua_game_gen.h	\
	tolua_signal_gen.c	\
	tolua_signal_gen.h

EXTRA_DIST = \
	tolua_common_a.pkg	\
	tolua_common_z.pkg	\
	tolua_game.pkg		\
	tolua_signal.pkg

BUILT_SOURCES = \
	tolua_common_a_gen.c	\
	tolua_common_a_gen.h	\
	tolua_common_z_gen.c	\
	tolua_common_z_gen.h	\
	tolua_game_gen.c	\
	tolua_game_gen.h	\
	tolua_signal_gen.c	\
	tolua_signal_gen.h

libscriptcore_la_LIBADD =	\
	$(LUA_LIBS) $(TOLUA_LIBS) $(LUASQL_LIBS)

tolua_common_a_gen.c tolua_common_a_gen.h: tolua_common_a.pkg
	$(TOLUA) -n common_a -o $(srcdir)/tolua_common_a_gen.c \
	-H $(srcdir)/tolua_common_a_gen.h $(srcdir)/tolua_common_a.pkg

tolua_common_z_gen.c tolua_common_z_gen.h: tolua_common_z.pkg
	$(TOLUA) -n common_z -o $(srcdir)/tolua_common_z_gen.c \
	-H $(srcdir)/tolua_common_z_gen.h $(srcdir)/tolua_common_z.pkg

tolua_game_gen.c tolua_game_gen.h: tolua_game.pkg
	$(TOLUA) -n game -o $(srcdir)/tolua_game_gen.c \
	-H $(srcdir)/tolua_game_gen.h $(srcdir)/tolua_game.pkg

tolua_signal_gen.c tolua_signal_gen.h: tolua_signal.pkg
	$(TOLUA) -n signal -o $(srcdir)/tolua_signal_gen.c \
	-H $(srcdir)/tolua_signal_gen.h $(srcdir)/tolua_signal.pkg

# These files are not generated to builddir, but to srcdir */
MAINTAINERCLEANFILES = \
	$(srcdir)/tolua_common_a_gen.c \
	$(srcdir)/tolua_common_a_gen.h \
	$(srcdir)/tolua_common_z_gen.c \
	$(srcdir)/tolua_common_z_gen.h \
	$(srcdir)/tolua_game_gen.c \
	$(srcdir)/tolua_game_gen.h \
	$(srcdir)/tolua_signal_gen.c \
	$(srcdir)/tolua_signal_gen.h
