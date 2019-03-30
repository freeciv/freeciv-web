# SUBDIRS = none

noinst_PROGRAMS = tolua

AM_CPPFLAGS = \
	$(LUA_CFLAGS) \
	-I$(top_srcdir)/dependencies/tolua-5.2/include

tolua_SOURCES = \
	$(top_srcdir)/dependencies/tolua-5.2/include/tolua.h \
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

tolua_LDADD = \
	$(LUA_LIBS) \
	../lib/libtolua.la

EXTRA_DIST = \
	tolua.pkg
