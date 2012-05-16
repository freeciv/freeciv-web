## Process this file with automake to produce Makefile.in

SUBDIRS = generator scripting

bin_PROGRAMS = civserver
lib_LTLIBRARIES = libfreeciv-srv.la
AM_CPPFLAGS = \
	-I$(top_srcdir)/utility -I$(srcdir)/../common -I$(srcdir)/../ai \
	-I$(top_srcdir)/common/aicore \
	-I$(srcdir)/generator -I$(srcdir)/scripting \
	$(LIBGGZ_INCLUDES) $(GGZDMOD_INCLUDES) @AUTH_CFLAGS@

if MINGW32
SERVERICON=../win32/servericon.o
endif

civserver_SOURCES = \
		civserver.c	\
		civserver.h

libfreeciv_srv_la_SOURCES = \
		aiiface.c	\
		aiiface.h	\
		auth.c		\
		auth.h		\
		barbarian.c	\
		barbarian.h	\
		cityhand.c	\
		cityhand.h	\
		citytools.c	\
		citytools.h	\
		cityturn.c	\
		cityturn.h	\
		commands.c	\
		commands.h	\
		connecthand.c	\
		connecthand.h	\
		console.c	\
		console.h	\
		diplhand.c	\
		diplhand.h	\
		diplomats.c	\
		diplomats.h	\
		edithand.c	\
		edithand.h	\
		gamehand.c	\
		gamehand.h	\
		gotohand.c	\
		gotohand.h	\
		ggzserver.c	\
		ggzserver.h	\
		handchat.c	\
		handchat.h	\
		hand_gen.h	\
		hand_gen.c	\
		maphand.c	\
		maphand.h	\
		meta.c		\
		meta.h		\
		notify.c	\
		notify.h	\
		plrhand.c	\
		plrhand.h	\
		report.c	\
		report.h	\
		ruleset.c	\
		ruleset.h	\
		sanitycheck.c	\
		sanitycheck.h	\
		savegame.c	\
		savegame.h	\
		score.c		\
		score.h		\
		sernet.c	\
		sernet.h	\
		settings.c	\
		settings.h	\
		settlers.c	\
		settlers.h	\
		spacerace.c	\
		spacerace.h	\
		srv_main.c	\
		srv_main.h	\
		stdinhand.c	\
		stdinhand.h	\
		techtools.h	\
		techtools.c	\
		unithand.c	\
		unithand.h	\
		unittools.c	\
		unittools.h	\
		voting.c	\
		voting.h

# hand_gen.c & hand_gen.h are generated files, but as they are generated
# outside this directory (when building common) there's no point in
# setting them to BUILT_SOURCES.

# FIXME: Server library is calling AI functions.
#        We should get rid of this dependency.
libfreeciv_srv_la_DEPENDENCIES = \
 $(top_builddir)/ai/libcivai.la

libfreeciv_srv_la_LIBADD = \
 $(top_builddir)/ai/libcivai.la

civserver_DEPENDENCIES = \
 $(top_builddir)/common/libfreeciv.la \
 ./libfreeciv-srv.la \
 ./scripting/libscripting.la \
 $(LUA_AS_DEPENDENCY) $(TOLUA_AS_DEPENDENCY) \
 ./generator/libgenerator.la
civserver_LDFLAGS = $(GGZDMOD_LDFLAGS)
civserver_LDADD = \
 $(top_builddir)/common/libfreeciv.la \
 ./libfreeciv-srv.la \
 $(top_builddir)/common/libfreeciv.la \
 ./libfreeciv-srv.la \
 $(top_builddir)/common/libfreeciv.la \
 ./generator/libgenerator.la \
 ./scripting/libscripting.la $(INTLLIBS) \
 $(LUA_LIBS) $(TOLUA_LIBS) \
 $(AUTH_LIBS) $(SERVER_LIBS) $(LIB_GGZDMOD) $(SERVERICON)

desktopfiledir = $(prefix)/share/applications
desktopfile_DATA = \
	freeciv-server.desktop
