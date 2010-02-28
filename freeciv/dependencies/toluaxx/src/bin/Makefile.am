# SUBDIRS = none

noinst_PROGRAMS = tolua

AM_CPPFLAGS = \
	-I$(top_srcdir)/dependencies/lua-5.1/src \
	-I$(top_srcdir)/dependencies/lua-5.1/etc \
	-I$(top_srcdir)/dependencies/toluaxx/include

tolua_SOURCES = \
	$(top_srcdir)/dependencies/toluaxx/include/toluaxx.h \
	toluaxx.bind.cxx	\
	main.cxx \
	main.hxx \
	help.cxx \
	help.hxx \
	parsecmd.cxx \
	parsecmd.hxx \
	platform.hxx

EXTRA_tolua_SOURCES = toluaxx.pxx \
	lua/tlx_all.lua \
	lua/tlx_array.lua \
	lua/tlx_basic.lua \
	lua/tlx_class.lua \
	lua/tlx_clean.lua \
	lua/tlx_code.lua \
	lua/tlx_compat.lua \
	lua/tlx_compat51.lua \
	lua/tlx_container.lua \
	lua/tlx_custom.lua \
	lua/tlx_declaration.lua \
	lua/tlx_define.lua \
	lua/tlx_doit.lua \
	lua/tlx_enumerate.lua \
	lua/tlx_feature.lua \
	lua/tlx_function.lua \
	lua/tlx_module.lua \
	lua/tlx_namespace.lua \
	lua/tlx_operator.lua \
	lua/tlx_package.lua \
	lua/tlx_typedef.lua \
	lua/tlx_variable.lua \
	lua/tlx_verbatim.lua

tolua_DEPENDENCIES = \
	$(top_builddir)/dependencies/lua-5.1/src/liblua.a \
	../lib/libtolua.a

tolua_LDADD = \
	$(top_builddir)/dependencies/lua-5.1/src/liblua.a \
	../lib/libtolua.a -lm
