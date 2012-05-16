## Process this file with automake to produce Makefile.in

## Must be set before adding anything
GUI_SUBDIRS =

desktopfiledir = $(prefix)/share/applications

if CLIENT_GUI_SDL
GUI_SUBDIRS += gui-sdl
endif
if CLIENT_GUI_GTK_2_0
GUI_SUBDIRS += gui-gtk-2.0
desktopfile_DATA = \
	freeciv.desktop
endif
if CLIENT_GUI_XAW
GUI_SUBDIRS += gui-xaw
endif
if CLIENT_GUI_STUB
GUI_SUBDIRS += gui-stub
endif
if CLIENT_GUI_WIN32
GUI_SUBDIRS += gui-win32
endif
if CLIENT_GUI_FTWL
GUI_SUBDIRS += gui-ftwl
endif

ALL_AUDIO_SDL_FILES=audio_sdl.c audio_sdl.h

if AUDIO_SDL
AUDIO_SDL_FILES=$(ALL_AUDIO_SDL_FILES)
endif

if MINGW32
CLIENTICON=../win32/clienticon.o
endif

EXTRA_DIST= $(ALL_AUDIO_SDL_FILES)

## This is usually false, so "include" is not recursed into 
## by 'make', but it can be enabled in configure, and it is
## necessary to have something like this so that the include 
## directory is included for 'make dist'.
if MAKE_CLIENT_INCLUDE
CLI_INCLUDE = include
else
CLI_INCLUDE = 
endif

SUBDIRS = $(GUI_SUBDIRS) $(CLI_INCLUDE) agents

AM_CPPFLAGS = -I$(top_srcdir)/utility -I$(srcdir)/include -I$(top_srcdir)/common -I$(top_srcdir)/common/aicore -I$(srcdir)/agents $(CLIENT_CFLAGS) $(SOUND_CFLAGS) $(LIBGGZ_INCLUDES) $(GGZMOD_INCLUDES) $(GGZ_GTK_INCLUDES)

# Build client common functionality in to library
# instead linking them directly to civclient. This is necessary
# for static linking.
freeciv_client_src = $(AUDIO_SDL_FILES) \
	attribute.h	\
	attribute.c	\
	citydlg_common.c \
	citydlg_common.h \
	cityrepdata.c	\
	cityrepdata.h	\
	client_main.c	\
	client_main.h	\
	chatline_common.c \
	chatline_common.h \
	connectdlg_common.c \
	connectdlg_common.h \
	climisc.c	\
	climisc.h	\
	climap.c	\
	climap.h	\
	clinet.c	\
	clinet.h	\
	colors_common.c		\
	colors_common.h		\
	control.c	\
	control.h	\
	editor.c 		\
	editor.h 		\
	ggzclient.c	\
	ggzclient.h	\
	goto.c		\
	goto.h		\
	helpdata.c	\
	helpdata.h	\
	mapctrl_common.c \
	mapctrl_common.h \
	mapview_common.c \
	mapview_common.h \
	messagewin_common.c \
	messagewin_common.h \
	overview_common.c	\
	overview_common.h	\
	packhand.c	\
	packhand.h	\
	packhand_gen.h	\
	packhand_gen.c	\
	plrdlg_common.c \
	plrdlg_common.h \
	options.c	\
	options.h	\
	repodlgs_common.c \
	repodlgs_common.h \
	reqtree.c \
	reqtree.h \
	servers.c		\
	servers.h		\
	text.c	\
	text.h	\
	themes_common.c	\
	themes_common.h	\
	tilespec.c	\
	tilespec.h	\
	voteinfo.c	\
	voteinfo.h	\
	audio.c         \
	audio.h         \
	audio_none.c    \
	audio_none.h

# packhand_gen.c & packhand_gen.h are generated files, but as they are
# generated outside this directory (when building common) there's no point in
# setting them to BUILT_SOURCES.

bin_PROGRAMS =
noinst_LTLIBRARIES =

if CLIENT_GUI_GTK_2_0
bin_PROGRAMS += freeciv-gtk2
noinst_LTLIBRARIES += libfcgui-gtk2.la
libfcgui_gtk2_la_SOURCES = $(freeciv_client_src)
libfcgui_gtk2_la_LIBADD = gui-gtk-2.0/libgui-gtk2.la agents/libagents.la
freeciv_gtk2_SOURCES = dummy.c
freeciv_gtk2_LDFLAGS = $(GUI_gtk2_LDFLAGS) $(GGZMOD_LDFLAGS)
freeciv_gtk2_LDADD = \
 $(SOUND_LIBS) \
 libfcgui-gtk2.la \
 $(top_builddir)/common/libfreeciv.la \
 $(INTLLIBS) $(CLIENT_LIBS) $(SOUND_LIBS) $(LIB_GGZMOD) $(CLIENTICON) \
 $(GUI_gtk2_LIBS)
endif

if CLIENT_GUI_SDL
bin_PROGRAMS += freeciv-sdl
noinst_LTLIBRARIES += libfcgui-sdl.la
libfcgui_sdl_la_SOURCES = $(freeciv_client_src)
libfcgui_sdl_la_LIBADD = gui-sdl/libgui-sdl.la agents/libagents.la
freeciv_sdl_SOURCES = dummy.c
freeciv_sdl_LDFLAGS = $(GUI_sdl_LDFLAGS) $(GGZMOD_LDFLAGS)
freeciv_sdl_LDADD = \
 $(GUI_sdl_LIBS) \
 libfcgui-sdl.la \
 $(top_builddir)/common/libfreeciv.la \
 $(INTLLIBS) $(CLIENT_LIBS) $(SOUND_LIBS) $(LIB_GGZMOD) $(CLIENTICON) \
 $(GUI_sdl_LIBS)
endif

if CLIENT_GUI_XAW
bin_PROGRAMS += freeciv-xaw
noinst_LTLIBRARIES += libfcgui-xaw.la
libfcgui_xaw_la_SOURCES = $(freeciv_client_src)
libfcgui_xaw_la_LIBADD = gui-xaw/libgui-xaw.la agents/libagents.la
freeciv_xaw_SOURCES = dummy.c
freeciv_xaw_LDFLAGS = $(GUI_xaw_LDFLAGS) $(GGZMOD_LDFLAGS)
freeciv_xaw_LDADD = \
 libfcgui-xaw.la \
 $(top_builddir)/common/libfreeciv.la \
 $(INTLLIBS) $(CLIENT_LIBS) $(SOUND_LIBS) $(LIB_GGZMOD) $(CLIENTICON) \
 $(GUI_xaw_LIBS)
endif

if CLIENT_GUI_FTWL
bin_PROGRAMS += freeciv-ftwl
noinst_LTLIBRARIES += libfcgui-ftwl.la
libfcgui_ftwl_la_SOURCES = $(freeciv_client_src)
libfcgui_ftwl_la_LIBADD = gui-ftwl/libgui-ftwl.la agents/libagents.la
freeciv_ftwl_SOURCES = dummy.c
freeciv_ftwl_LDFLAGS = $(GUI_ftwl_LDFLAGS) $(GGZMOD_LDFLAGS)
freeciv_ftwl_LDADD = \
 libfcgui-ftwl.la \
 $(top_builddir)/common/libfreeciv.la \
 $(top_builddir)/utility/ftwl/libftwl.la \
 $(INTLLIBS) $(CLIENT_LIBS) $(SOUND_LIBS) $(LIB_GGZMOD) $(CLIENTICON) \
 $(GUI_ftwl_LIBS)
endif

if CLIENT_GUI_WIN32
bin_PROGRAMS += freeciv-win32
noinst_LTLIBRARIES += libfcgui-win32.la
libfcgui_win32_la_SOURCES = $(freeciv_client_src)
libfcgui_win32_la_LIBADD = gui-win32/libgui-win32.la agents/libagents.la
freeciv_win32_SOURCES = dummy.c
freeciv_win32_LDFLAGS = $(GUI_win32_LDFLAGS) $(GGZMOD_LDFLAGS)
freeciv_win32_LDADD = \
 $(SOUND_LIBS) \
 libfcgui-win32.la \
 $(top_builddir)/common/libfreeciv.la \
 $(INTLLIBS) $(CLIENT_LIBS) $(SOUND_LIBS) $(LIB_GGZMOD) $(CLIENTICON) \
 $(GUI_win32_LIBS)
endif

if CLIENT_GUI_STUB
bin_PROGRAMS += freeciv-stub
noinst_LTLIBRARIES += libfcgui-stub.la
libfcgui_stub_la_SOURCES = $(freeciv_client_src)
libfcgui_stub_la_LIBADD = gui-stub/libgui-stub.la agents/libagents.la
freeciv_stub_SOURCES = dummy.c
freeciv_stub_LDFLAGS = $(GUI_stub_LDFLAGS) $(GGZMOD_LDFLAGS)
freeciv_stub_LDADD = \
 libfcgui-stub.la \
 $(top_builddir)/common/libfreeciv.la \
 $(INTLLIBS) $(CLIENT_LIBS) $(SOUND_LIBS) $(LIB_GGZMOD) $(CLIENTICON) \
 $(GUI_stub_LIBS)
endif
