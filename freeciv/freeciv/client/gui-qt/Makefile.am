## Process this file with automake to produce Makefile.in

MOCCMD=@MOCCMD@

include $(top_srcdir)/bootstrap/Makerules.mk

noinst_LTLIBRARIES = libgui-qt.la
AM_CPPFLAGS = \
	-I. \
	-I$(srcdir)/.. \
	-I$(srcdir)/../include \
	-I$(top_srcdir)/utility \
        -I$(top_srcdir)/common \
        -I$(top_srcdir)/common/scriptcore \
        -I$(srcdir)/../luascript \
	-I$(top_srcdir)/common/aicore \
	-I$(top_srcdir)/common/networking \
	-I$(srcdir)/../agents \
	-I$(top_srcdir)/dependencies/tinycthread \
        $(gui_qt_cppflags) $(SOUND_CFLAGS)

AM_CFLAGS = $(gui_qt_cflags)

AM_CXXFLAGS = $(gui_qt_cxxflags)

MOC_FILES = \
	meta_fc_client.cpp	\
	meta_ratesdlg.cpp	\
	meta_mapview.cpp	\
	meta_menu.cpp		\
	meta_repodlgs.cpp	\
	meta_dialogs.cpp	\
	meta_optiondlg.cpp	\
	meta_citydlg.cpp	\
	meta_cityrep.cpp	\
	meta_helpdlg.cpp	\
	meta_plrdlg.cpp		\
	meta_diplodlg.cpp	\
	meta_spaceshipdlg.cpp	\
	meta_messagewin.cpp	\
	meta_chatline.cpp	\
	meta_messagedlg.cpp	\
	meta_sidebar.cpp	\
	meta_shortcuts.cpp	\
	meta_voteinfo_bar.cpp	\
	meta_gotodlg.cpp	\
	meta_hudwidget.cpp

libgui_qt_la_SOURCES = \
	canvas.cpp	\
	canvas.h	\
	chatline.cpp	\
	chatline.h	\
	citydlg.cpp	\
	citydlg.h	\
	cityrep.cpp	\
	cityrep.h	\
	colors.cpp	\
	colors.h	\
	connectdlg.cpp	\
	connectdlg.h	\
	dialogs.cpp	\
	dialogs.h	\
	diplodlg.cpp	\
	diplodlg.h	\
	fc_client.cpp	\
	fc_client.h	\
	fonts.cpp	\
	fonts.h		\
	finddlg.cpp	\
	finddlg.h	\
	gotodlg.cpp	\
	gotodlg.h	\
	graphics.cpp	\
	graphics.h	\
	gui_main.cpp	\
	gui_main.h	\
	helpdlg.cpp	\
	helpdlg.h	\
	hudwidget.cpp	\
	hudwidget.h	\
	inteldlg.cpp	\
	inteldlg.h	\
	listener.h	\
	luaconsole.cpp	\
	luaconsole.h	\
	mapctrl.cpp	\
	mapctrl.h	\
	mapview.cpp	\
	mapview.h	\
	menu.cpp	\
	menu.h		\
	messagedlg.cpp	\
	messagedlg.h	\
	messagewin.cpp	\
	messagewin.h	\
	optiondlg.cpp	\
	optiondlg.h	\
	pages.cpp	\
	pages.h		\
	plrdlg.cpp	\
	plrdlg.h	\
	qtg_cxxside.cpp \
	qtg_cxxside.h	\
	ratesdlg.cpp	\
	ratesdlg.h	\
	repodlgs.cpp	\
	repodlgs.h	\
	shortcuts.h	\
	shortcuts.cpp	\
	sidebar.cpp	\
	sidebar.h	\
	spaceshipdlg.cpp \
	spaceshipdlg.h	\
	sprite.cpp	\
	sprite.h	\
	themes.cpp	\
	voteinfo_bar.cpp \
	voteinfo_bar.h	\
	wldlg.cpp	\
	wldlg.h

nodist_libgui_qt_la_SOURCES = $(MOC_FILES)

CLEANFILES = $(MOC_FILES)
