## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libcivutility.la

AM_CPPFLAGS = -I$(top_srcdir)/dependencies/tinycthread

AM_CFLAGS = $(UTILITY_CFLAGS)

libcivutility_la_SOURCES = \
		astring.c       \
		astring.h       \
		bitvector.c     \
		bitvector.h     \
		bugs.c		\
		bugs.h		\
		capability.c	\
		capability.h	\
		deprecations.c	\
		deprecations.h	\
		distribute.c	\
		distribute.h	\
		fc_cmdline.c	\
		fc_cmdline.h	\
		fc_dirent.c	\
		fc_dirent.h	\
		fc_utf8.c	\
		fc_utf8.h	\
		fc_prehdrs.h	\
		fcbacktrace.c	\
		fcbacktrace.h	\
		fciconv.c	\
		fciconv.h	\
		fcintl.c	\
		fcintl.h	\
		fcthread.c	\
		fcthread.h	\
		genhash.c	\
		genhash.h	\
		genlist.c	\
		genlist.h	\
		inputfile.c	\
		inputfile.h	\
		ioz.c		\
		ioz.h		\
		iterator.c	\
		iterator.h	\
		log.c		\
		log.h		\
		mem.c           \
		mem.h		\
		net_types.h	\
		netfile.c	\
		netfile.h	\
		netintf.c	\
		netintf.h	\
		rand.c		\
		rand.h		\
		registry.c	\
		registry.h	\
		registry_ini.c	\
		registry_ini.h	\
		registry_xml.c	\
		registry_xml.h	\
		section_file.c	\
		section_file.h	\
		shared.c	\
		shared.h	\
		specenum_gen.h	\
		spechash.h	\
		speclist.h	\
		specpq.h	\
		specvec.h	\
		string_vector.c	\
		string_vector.h	\
		support.c	\
		support.h	\
		timing.c	\
		timing.h	\
		md5.c		\
		md5.h

EXTRA_DIST = \
		generate_specenum.py

libcivutility_la_LIBADD = $(UTILITY_LIBS)

BUILT_SOURCES = specenum_gen.h

specenum_gen.h: specenum_generate
.INTERMEDIATE: specenum_generate
specenum_generate: generate_specenum.py
	cd $(srcdir) && ./generate_specenum.py specenum_gen.h
	touch specenum_generate

# These files are not generated to builddir, but to srcdir */
MAINTAINERCLEANFILES = $(srcdir)/specenum_gen.h
