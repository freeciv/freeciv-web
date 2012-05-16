# SUBDIRS = none

noinst_PROGRAMS = tolua

AM_CPPFLAGS = \
	-I$(top_srcdir)/dependencies/lua-5.1/src \
	-I$(top_srcdir)/dependencies/lua-5.1/etc \
	-I$(top_srcdir)/dependencies/tolua-5.1b/include

tolua_SOURCES = \
	$(top_srcdir)/dependencies/tolua-5.1b/include/tolua.h \
	tolua.c \
	toluabind.c

EXTRA_tolua_SOURCES = \
	lua/all.lua \
	lua/array.lua \
	lua/basic.lua \
	lua/class.lua \
	lua/clean.lua \
	lua/code.lua \
	lua/compat.lua \
	lua/container.lua \
	lua/declaration.lua \
	lua/define.lua \
	lua/doit.lua \
	lua/enumerate.lua \
	lua/feature.lua \
	lua/function.lua \
	lua/module.lua \
	lua/namespace.lua \
	lua/operator.lua \
	lua/package.lua \
	lua/typedef.lua \
	lua/variable.lua \
	lua/verbatim.lua

tolua_DEPENDENCIES = \
	$(top_builddir)/dependencies/lua-5.1/src/liblua.a \
	../lib/libtolua.a

tolua_LDADD = \
	$(top_builddir)/dependencies/lua-5.1/src/liblua.a \
	../lib/libtolua.a -lm
