## Process this file with automake to produce Makefile.in

bin_PROGRAMS = civmanual
AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/utility -I$(top_srcdir)/common -I$(top_srcdir)/ai -I$(top_srcdir)/common/aicore -I$(top_srcdir)/server/generator -I$(top_srcdir)/client -I$(top_srcdir)/client/include

civmanual_SOURCES = \
		civmanual.c

# This is a bit of a hack.  The program links in with the server lib
# but also uses some files from the client.
civmanual_DEPENDENCIES = \
 $(top_builddir)/common/libfreeciv.la \
 $(top_builddir)/server/libfreeciv-srv.la \
 $(top_builddir)/client/helpdata.lo \
 $(top_builddir)/server/scripting/libscripting.la \
 $(LUA_AS_DEPENDENCY) $(TOLUA_AS_DEPENDENCY) \
 $(top_builddir)/server/generator/libgenerator.la
civmanual_LDFLAGS = $(GGZDMOD_LDFLAGS)
civmanual_LDADD = \
 $(top_builddir)/server/libfreeciv-srv.la \
 ../client/helpdata.lo \
 $(top_builddir)/server/scripting/libscripting.la \
 $(LUA_LIBS) $(TOLUA_LIBS) \
 $(top_builddir)/server/generator/libgenerator.la \
 $(top_builddir)/common/libfreeciv.la $(INTLLIBS) \
 $(SERVER_LIBS) $(AUTH_LIBS) $(LIB_GGZDMOD)
