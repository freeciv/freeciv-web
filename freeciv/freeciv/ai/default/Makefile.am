## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libdefaultai.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/aicore \
	-I$(top_srcdir)/common/networking \
	-I$(top_srcdir)/server \
	-I$(top_srcdir)/server/advisors \
	-I$(top_srcdir)/ai \
	-I$(top_srcdir)/server/generator \
	-I$(top_srcdir)/dependencies/tinycthread

libdefaultai_la_SOURCES = \
		aiair.c		\
		aiair.h		\
		aicity.c	\
		aicity.h	\
		aidata.c	\
		aidata.h	\
		aiferry.c	\
		aiferry.h	\
		aiguard.c	\
		aiguard.h	\
		aihand.c	\
		aihand.h	\
		aihunt.c	\
		aihunt.h	\
		ailog.c		\
		ailog.h		\
		aiparatrooper.c \
		aiparatrooper.h \
		aiplayer.c	\
		aiplayer.h	\
		aisettler.c	\
		aisettler.h	\
		aitech.c	\
		aitech.h	\
		aitools.c	\
		aitools.h	\
		aiunit.c	\
		aiunit.h	\
		aidiplomat.c	\
		aidiplomat.h	\
		daidiplomacy.c	\
		daidiplomacy.h	\
		daidomestic.c	\
		daidomestic.h	\
		daieffects.c	\
		daieffects.h	\
		daimilitary.c	\
		daimilitary.h


libdefaultai_la_LIBADD = -lm
