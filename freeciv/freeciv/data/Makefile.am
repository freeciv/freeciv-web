## Process this file with automake to produce Makefile.in

if CLIENT
CLIENT_FILES =				\
	hex2t.tilespec			\
	amplio.tilespec			\
	isophex.tilespec		\
	isotrident.tilespec		\
	trident.tilespec		\
	stdsounds.soundspec		\
	helpdata.txt
if CLIENT_GUI_XAW
CLIENT_FILES += Freeciv
endif
if CLIENT_GUI_GTK_2_0
CLIENT_FILES += \
	freeciv.rc-2.0	\
	gtk_menus.xml
endif
else
CLIENT_FILES =
endif

if SERVER
SERVER_FILES = \
	civ1.serv \
	civ2.serv \
	default.serv
else
SERVER_FILES =
endif

pkgdata_DATA = $(SERVER_FILES) $(CLIENT_FILES)

# Freeciv.rc was originally for gui-gtk but is also used by gui-sdl
EXTRA_DIST = \
	Freeciv	\
	freeciv-client.png \
	freeciv.rc	\
	freeciv.rc-2.0	\
	gtk_menus.xml	\
	civserver.dsc.in	\
	civclient.dsc.in	\
	civserver.room.in	\
	hex2t.tilespec	\
	amplio.tilespec	\
	isophex.tilespec	\
	isotrident.tilespec \
	trident.tilespec	\
	stdsounds.soundspec	\
	helpdata.txt	\
	civ1.serv	\
	civ2.serv	\
	default.serv

if CLIENT
CLIENTDATADIRS = amplio misc flags buildings isotrident trident isophex \
 hex2t stdsounds themes wonders
else
CLIENTDATADIRS = 
endif

if SERVER
SERVERDATADIRS = default civ1 civ2 scenario nation
else
SERVERDATADIRS = 
endif

if CLIENT
ICONS = freeciv-client.png
else
ICONS = 
endif

icondir = $(prefix)/share/pixmaps
icon_DATA=$(ICONS)

SUBDIRS = icons $(CLIENTDATADIRS) $(SERVERDATADIRS)

if GGZ_CLIENT
install-data-local:
	$(GGZ_CONFIG) -D --install --modfile=civclient.dsc --force

uninstall-local:
	$(GGZ_CONFIG) -D --remove --modfile=civclient.dsc
endif

if GGZ_SERVER
ggzroom_DATA = civserver.room
ggzroomdir = $(ggzdconfdir)/rooms/

ggzgame_DATA = civserver.dsc
ggzgamedir = $(ggzdconfdir)/games/
endif
