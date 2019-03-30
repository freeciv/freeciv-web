## Process this file with automake to produce Makefile.in

if AI_MOD_STATIC_CLASSIC
noinst_LTLIBRARIES = libclassicai.la
else
aimodule_LTLIBRARIES = fc_ai_classic.la
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/aicore \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/server \
	-I$(top_srcdir)/server/advisors \
	-I$(top_srcdir)/ai/default \
	-I$(top_srcdir)/dependencies/tinycthread

da_sources = \
	classicai.c	\
	classicai.h

if !AI_MOD_STATIC_CLASSIC
fc_ai_classic_la_SOURCES = $(da_sources)

fc_ai_classic_la_LDFLAGS = -module
else
libclassicai_la_SOURCES = $(da_sources)
endif
