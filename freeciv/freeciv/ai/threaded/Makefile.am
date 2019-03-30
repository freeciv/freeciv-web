## Process this file with automake to produce Makefile.in

if AI_MOD_STATIC_THREADED
noinst_LTLIBRARIES = libthreadedai.la
else
aimodule_LTLIBRARIES = fc_ai_threaded.la
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/aicore \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/server/ \
	-I$(top_srcdir)/server/advisors \
	-I$(top_srcdir)/ai/default \
	-I$(top_srcdir)/dependencies/tinycthread

da_sources = \
	taicity.c		\
	taicity.h		\
	taimsg.c		\
	taimsg.h		\
	taiplayer.c		\
	taiplayer.h		\
	threadedai.c

if AI_MOD_STATIC_THREADED
libthreadedai_la_SOURCES = $(da_sources)
else
fc_ai_threaded_la_SOURCES = $(da_sources)
fc_ai_threaded_la_LDFLAGS = -module
endif
