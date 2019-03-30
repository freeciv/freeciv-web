## Process this file with automake to produce Makefile.in

if AI_MOD_STATIC_STUB
noinst_LTLIBRARIES = libstubai.la
else
aimodule_LTLIBRARIES = fc_ai_stub.la
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/dependencies/tinycthread

if AI_MOD_STATIC_STUB
libstubai_la_SOURCES = stubai.c
else
fc_ai_stub_la_SOURCES = stubai.c
fc_ai_stub_la_LDFLAGS = -module
endif
