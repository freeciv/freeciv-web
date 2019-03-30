## Process this file with automake to produce Makefile.in

module_dirs = 

if AI_MOD_DEFAULT_NEEDED
module_dirs += default
endif

if AI_MOD_STATIC_CLASSIC
module_dirs += classic
endif

if AI_MOD_STATIC_THREADED
module_dirs += threaded
endif

if AI_MOD_STATIC_TEX
module_dirs += tex
endif

if AI_MOD_STATIC_STUB
module_dirs += stub
endif

AM_CPPFLAGS = -I$(top_srcdir)/utility -I$(top_srcdir)/common \
 -I$(top_srcdir)/common/networking \
 -I$(top_srcdir)/dependencies/tinycthread

if AI_MODULES
if !AI_MOD_STATIC_CLASSIC
module_dirs += classic
endif
if EXP_AI_MODULES
if !AI_MOD_STATIC_THREADED
module_dirs += threaded
endif
if !AI_MOD_STATIC_TEX
module_dirs += tex
endif
if !AI_MOD_STATIC_STUB
module_dirs += stub
endif
else
# AI module loading supported, but experimental modules not built
endif
endif

SUBDIRS = $(module_dirs)

noinst_LTLIBRARIES = libaicommon.la

libaicommon_la_SOURCES = \
	aitraits.c	\
	aitraits.h	\
	difficulty.c	\
	difficulty.h	\
	handicaps.c	\
	handicaps.h
