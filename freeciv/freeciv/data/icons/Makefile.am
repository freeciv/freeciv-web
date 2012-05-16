## Process this file with automake to produce Makefile.in

if CLIENT
icons_16_c = \
	16x16/freeciv-client.png
icons_32_c = \
	32x32/freeciv-client.png
icons_48_c = \
	48x48/freeciv-client.png
icons_64_c = \
	64x64/freeciv-client.png
icons_128_c = \
	128x128/freeciv-client.png
else
icons_16_c =
icons_32_c =
icons_48_c =
icons_64_c =
icons_128_c =
endif

if SERVER
icons_16_s = \
	16x16/freeciv-server.png
icons_32_s = \
	32x32/freeciv-server.png
icons_48_s = \
	48x48/freeciv-server.png
icons_64_s = \
	64x64/freeciv-server.png
icons_128_s = \
	128x128/freeciv-server.png
else
icons_16_s =
icons_32_s =
icons_48_s =
icons_64_s =
icons_128_s =
endif

#icons_svg = \
#	scalable/freeciv-client.svg \
#	scalable/freeciv-server.svg

icon16dir = $(datadir)/icons/hicolor/16x16/apps
icon32dir = $(datadir)/icons/hicolor/32x32/apps
icon48dir = $(datadir)/icons/hicolor/48x48/apps
icon64dir = $(datadir)/icons/hicolor/64x64/apps
icon128dir = $(datadir)/icons/hicolor/128x128/apps
#iconscalabledir = $(datadir)/icons/hicolor/scalable/apps

icon16_DATA = $(icons_16_c) $(icons_16_s)
icon32_DATA = $(icons_32_c) $(icons_32_s)
icon48_DATA = $(icons_48_c) $(icons_48_s)
icon64_DATA = $(icons_64_c) $(icons_64_s)
icon128_DATA = $(icons_128_c) $(icons_128_s)
#iconscalable_DATA = $(icons_svg)

EXTRA_DIST = \
	$(icons_16_c)	\
	$(icons_16_s)	\
	$(icons_32_c)	\
	$(icons_32_s)	\
	$(icons_48_c)	\
	$(icons_48_s)	\
	$(icons_64_c)	\
	$(icons_64_s)	\
	$(icons_128_c)	\
	$(icons_128_s)

