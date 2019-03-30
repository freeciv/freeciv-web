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

if MODINST
icons_16_m = \
	16x16/freeciv-modpack.png
icons_32_m = \
	32x32/freeciv-modpack.png
icons_48_m = \
	48x48/freeciv-modpack.png
icons_64_m = \
	64x64/freeciv-modpack.png
icons_128_m = \
	128x128/freeciv-modpack.png
else
icons_16_m =
icons_32_m =
icons_48_m =
icons_64_m =
icons_128_m =
endif

#icons_svg = \
#	scalable/freeciv-client.svg \
#	scalable/freeciv-server.svg \
#	scalable/freeciv-modpack.svg

icon16dir = $(datadir)/icons/hicolor/16x16/apps
icon32dir = $(datadir)/icons/hicolor/32x32/apps
icon48dir = $(datadir)/icons/hicolor/48x48/apps
icon64dir = $(datadir)/icons/hicolor/64x64/apps
icon128dir = $(datadir)/icons/hicolor/128x128/apps
#iconscalabledir = $(datadir)/icons/hicolor/scalable/apps

icon16_DATA = $(icons_16_c) $(icons_16_s) $(icons_16_m)
icon32_DATA = $(icons_32_c) $(icons_32_s) $(icons_32_m)
icon48_DATA = $(icons_48_c) $(icons_48_s) $(icons_48_m)
icon64_DATA = $(icons_64_c) $(icons_64_s) $(icons_64_m)
icon128_DATA = $(icons_128_c) $(icons_128_s) $(icons_128_m)
#iconscalable_DATA = $(icons_svg)

EXTRA_DIST = \
	$(icons_16_c)	\
	$(icons_16_s)	\
	$(icons_16_m)	\
	$(icons_32_c)	\
	$(icons_32_s)	\
	$(icons_32_m)	\
	$(icons_48_c)	\
	$(icons_48_s)	\
	$(icons_48_m)	\
	$(icons_64_c)	\
	$(icons_64_s)	\
	$(icons_64_m)	\
	$(icons_128_c)	\
	$(icons_128_s)	\
	$(icons_128_m)
