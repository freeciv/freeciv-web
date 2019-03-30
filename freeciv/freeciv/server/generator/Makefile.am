## Process this file with automake to produce Makefile.in
noinst_LTLIBRARIES = libgenerator.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/server \
	-I$(top_srcdir)/server/generator \
	-I$(top_srcdir)/dependencies/tinycthread

libgenerator_la_SOURCES = \
	mapgen.c \
	mapgen.h \
	mapgen_topology.c \
	mapgen_topology.h \
	mapgen_utils.c \
	mapgen_utils.h \
	fracture_map.c \
	fracture_map.h \
	height_map.c \
	height_map.h \
	startpos.c \
	startpos.h \
	temperature_map.c \
	temperature_map.h

libgenerator_la_LIBADD = -lm
