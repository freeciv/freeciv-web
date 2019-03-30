## Process this file with automake to produce Makefile.in

SUBDIRS = aicore networking scriptcore

lib_LTLIBRARIES = libfreeciv.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/utility \
	-I$(srcdir)/aicore \
	-I$(srcdir)/networking \
	-I$(srcdir)/scriptcore \
	-I$(top_srcdir)/dependencies/tinycthread \
	$(MAPIMG_WAND_CFLAGS) $(JANSSON_CFLAGS)

libfreeciv_la_SOURCES = \
		achievements.c	\
		achievements.h	\
		actions.c	\
		actions.h	\
		ai.c		\
		ai.h		\
		base.c		\
		base.h		\
		borders.c	\
		borders.h	\
		calendar.c	\
		calendar.h	\
		capstr.c	\
		capstr.h	\
		chat.h		\
		citizens.c	\
		citizens.h	\
		city.c		\
		city.h		\
		clientutils.c	\
		clientutils.h	\
		combat.c	\
		combat.h	\
		culture.c	\
		culture.h	\
		diptreaty.c	\
		diptreaty.h	\
		disaster.c	\
		disaster.h	\
		effects.c	\
		effects.h	\
		events.c	\
		events.h	\
		extras.c	\
		extras.h	\
		featured_text.c	\
		featured_text.h	\
		fc_cmdhelp.c	\
		fc_cmdhelp.h	\
		fc_interface.c	\
		fc_interface.h	\
		fc_types.h	\
		game.c		\
		game.h		\
		government.c	\
		government.h	\
		idex.c		\
		idex.h		\
		improvement.c	\
		improvement.h	\
		map.c		\
		map.h		\
		map_types.h	\
		mapimg.c	\
		mapimg.h	\
		metaknowledge.c \
		metaknowledge.h \
		movement.c	\
		movement.h	\
		multipliers.c   \
		multipliers.h   \
		name_translation.h \
		nation.c        \
		nation.h        \
		packets_gen.c	\
		packets_gen.h	\
		player.c	\
		player.h	\
		reqtext.c	\
		reqtext.h	\
		requirements.c	\
		requirements.h	\
		research.c	\
		research.h	\
		rgbcolor.c	\
		rgbcolor.h	\
		road.c		\
		road.h		\
		server_settings.h \
		server_settings.c \
		spaceship.c	\
		spaceship.h	\
		specialist.c	\
		specialist.h	\
		srvdefs.h	\
		style.c		\
		style.h		\
		team.c		\
		team.h		\
		tech.c		\
		tech.h		\
		terrain.c	\
		terrain.h	\
		tile.c		\
		tile.h		\
		traderoutes.c	\
		traderoutes.h	\
		traits.h	\
		unit.c		\
		unit.h		\
		unitlist.c	\
		unitlist.h	\
		unittype.c	\
		unittype.h	\
		version.c	\
		version.h	\
		victory.c	\
		victory.h	\
		vision.c	\
		vision.h	\
		workertask.c	\
		workertask.h	\
		worklist.c	\
		worklist.h	\
		world_object.h

EXTRA_DIST = \
		generate_packets.py

EXTRA_libfreeciv_la_DEPENDENCIES = \
	$(top_builddir)/utility/libcivutility.la \
	$(top_builddir)/common/aicore/libaicore.la \
	$(top_builddir)/common/networking/libfcivnetwork.la \
	$(top_builddir)/common/scriptcore/libscriptcore.la

libfreeciv_la_LIBADD = \
	$(top_builddir)/utility/libcivutility.la \
	$(top_builddir)/common/aicore/libaicore.la \
	$(top_builddir)/common/networking/libfcivnetwork.la \
	$(top_builddir)/common/scriptcore/libscriptcore.la \
	$(COMMON_LIBS)

BUILT_SOURCES = packets_gen.c packets_gen.h

# ./generate_packets.py makes both packets_gen.[ch]
#
# Feed the dependence through a fake intermediate file to avoid
# a race condition in a parallel build (like make -j2).
#
# Currently we run the generation directly into the source directory.
# Ideally we'd generate the new files into the build directory but first we'd
# have to resolve conflicts (since there could then be 2 copies).
packets_gen.h packets_gen.c: packets_generate
.INTERMEDIATE: packets_generate
packets_generate: networking/packets.def generate_packets.py
	cd $(srcdir) && ./generate_packets.py packets_gen.h packets_gen.c ../client/packhand_gen.h ../client/packhand_gen.c ../server/hand_gen.h ../server/hand_gen.c
	touch packets_generate

# These files are not generated to builddir, but to srcdir */
MAINTAINERCLEANFILES = \
	$(srcdir)/packets_gen.h \
	$(srcdir)/packets_gen.c

if GITREV
BUILT_SOURCES += fc_gitrev_gen.h
endif

.PHONY : fc_gitrev_gen.h

fc_gitrev_gen.h :
	"$(top_srcdir)/bootstrap/generate_gitrev.sh" "$(top_srcdir)" "$(top_builddir)"
