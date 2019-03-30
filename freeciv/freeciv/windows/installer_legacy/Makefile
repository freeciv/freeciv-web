#
# Makefile for Freeciv Windows installer creation
#
# Usage:
#
# 1. download and extract the MinGW build environment from
#    https://sourceforge.net/projects/freeciv/files/dependencies/gnuwin32/
#    Current version is gnuwin32-2018-05-11.
# 2. double click on "gnuwin32/msys.bat"
# 3. install the development packages:
#    $ hg qpush -a
# 4. close current session and restart msys.bat. This is required
#    for PATH to be set correctly after development packages
#    installation.
# 5. change into the Freeciv directory where this Makefile resides
#    $ cd <path_to_freeciv_source>/win32/installer
# 6. run make
#    $ make
# 7. find the installer executables in the 'Output' directory
#

#
# Variable setup
#

ifeq ($(MIN_WIN_VER),)
MIN_WIN_VER=0x0600
endif

#
# Make rules
#


# by default build all installers
all: gtk3-installer sdl2-installer qt-installer ruledit-installer

gtk3-installer:
	make GUI=gtk3 FCMP=gtk3 EXTRA_CONFIG="--disable-ruledit $(EXTRA_CONFIG)" wrap-gtk3

sdl2-installer:
	make GUI=sdl2 FCMP=gtk3 EXTRA_CONFIG="--disable-ruledit $(EXTRA_CONFIG)" wrap-sdl2

qt-installer:
	make GUI=qt FCMP=qt wrap-qt

ruledit-installer:
	make wrap-ruledit

snapshot:
	cd ../.. ; ./autogen.sh --no-configure-run
	make VERSION_REVTYPE="git" EXTRA_CONFIG="--enable-gitrev" all


# build Freeciv

build-freeciv-common:
	# create build directory
	mkdir -p build-$(GUI)
	# configure
	cd build-$(GUI); ../../../configure CPPFLAGS="-D_WIN32_WINNT=$(MIN_WIN_VER)" --enable-client=$(GUI) --with-followtag="windows-S3_1" --enable-fcdb=sqlite3 --enable-fcmp=$(FCMP) $(EXTRA_CONFIG)
	# make
	make -C build-$(GUI)
	make -C build-$(GUI)/translations/core update-po
	make -C build-$(GUI)/bootstrap langstat_core.txt

build-freeciv-ruledit:
	# create build directory
	mkdir -p build-ruledit
	# configure
	cd build-ruledit; ../../../configure CPPFLAGS="-D_WIN32_WINNT=$(MIN_WIN_VER)" --disable-client --disable-server --disable-fcmp --disable-freeciv-manual --enable-ruledit --enable-ai-static=stub $(EXTRA_CONFIG)
	# make
	make -C build-ruledit
	make -C build-ruledit/translations/ruledit update-po
	make -C build-ruledit/bootstrap langstat_ruledit.txt

build-freeciv-gtk3: build-freeciv-common

build-freeciv-sdl2: build-freeciv-common

build-freeciv-qt: build-freeciv-common

# install Freeciv

install-freeciv-common:
	# make install
	make -C build-$(GUI) DESTDIR=`pwd`/install-$(GUI) install
	# reorder directory layout
	mv install-$(GUI)/usr/local/bin/* install-$(GUI)/
	mv install-$(GUI)/usr/local/etc install-$(GUI)/
	mv install-$(GUI)/usr/local/share/freeciv install-$(GUI)/data
	mkdir -p install-$(GUI)/share
	mv install-$(GUI)/usr/local/share/locale install-$(GUI)/share/
	mv install-$(GUI)/usr/local/share/doc install-$(GUI)/doc
	mkdir -p install-$(GUI)/debuginfo
	cp build-$(GUI)/gen_headers/fc_config.h install-$(GUI)/debuginfo
	mkdir -p install-$(GUI)/doc/freeciv/installer
	cp licenses/COPYING.installer install-$(GUI)/doc/freeciv/installer/
	# delete unneeded files
	rm -r install-$(GUI)/usr
	# strip 'freeciv-manual' and 'freeciv-mp-<gui>' executables
	strip install-$(GUI)/freeciv-m*
	# add start menu files
	cp freeciv-server.cmd freeciv-mp-$(FCMP).cmd freeciv-$(GUI).cmd Freeciv.url install-$(GUI)/

install-freeciv-gtk3: install-freeciv-common

install-freeciv-sdl2: install-freeciv-common
	# add CJK fonts
	cp /opt/fireflysung-1.3.0/fireflysung.ttf install-sdl2/data/themes/gui-sdl2/human/
	cp /opt/fireflysung-1.3.0/COPYRIGHT install-sdl2/data/themes/gui-sdl2/human/COPYING.fireflysung
	cp /opt/sazanami-20040629/sazanami-gothic.ttf install-sdl2/data/themes/gui-sdl2/human/
	cp /opt/sazanami-20040629/README install-sdl2/data/themes/gui-sdl2/human/COPYING.sazanami
	cp /opt/un-fonts/UnDotum.ttf install-sdl2/data/themes/gui-sdl2/human/
	cp /opt/un-fonts/COPYING install-sdl2/data/themes/gui-sdl2/human/COPYING.UnDotum

install-freeciv-qt: install-freeciv-common
	# strip 'freeciv-ruledit' executable
	strip install-$(GUI)/freeciv-ruledit.exe
	cp freeciv-ruledit.cmd install-$(GUI)/

install-ruledit-dir:
	mkdir -p install-ruledit/share

install-ruledit: install-ruledit-dir
	# make install
	make -C build-ruledit DESTDIR=`pwd`/install-ruledit install
	# reorder directory layout
	mv install-ruledit/usr/local/bin/* install-ruledit/
	mv install-ruledit/usr/local/share/freeciv install-ruledit/data
	mv install-ruledit/usr/local/share/locale install-ruledit/share/
	mv install-ruledit/usr/local/share/doc install-ruledit/doc
	mkdir -p install-ruledit/debuginfo
	cp build-ruledit/gen_headers/fc_config.h install-ruledit/debuginfo
	mkdir -p install-ruledit/doc/freeciv/installer
	cp licenses/COPYING.installer install-ruledit/doc/freeciv/installer/
	# delete unneeded files
	rm -r install-ruledit/usr
	# strip 'freeciv-ruledit' executable
	strip install-ruledit/freeciv-ruledit.exe
	# add start menu files
	cp freeciv-ruledit.cmd Freeciv.url install-ruledit/

# install Freeciv environment

COMMON_DLLS := \
	exchndl.dll \
	freetype6.dll \
	intl.dll \
	libMagickCore-6.Q16-1.dll \
	libMagickWand-6.Q16-1.dll \
	libatk-1.0-0.dll \
	libbz2-2.dll \
	libcurl.dll \
	libcairo-2.dll \
	libcairo-gobject-2.dll \
	libeay32.dll \
	libffi-6.dll \
	libfontconfig-1.dll \
	libfreetype-6.dll \
	libgcc_s_dw2-1.dll \
	libgdk_pixbuf-2.0-0.dll \
	libgio-2.0-0.dll \
	libglib-2.0-0.dll \
	libgmodule-2.0-0.dll \
	libgobject-2.0-0.dll \
	libgthread-2.0-0.dll \
	libiconv-2.dll \
	libidn-11.dll \
	libintl-8.dll \
	liblzma-5.dll \
	libogg-0.dll \
	libpango-1.0-0.dll \
	libpangocairo-1.0-0.dll \
	libpangowin32-1.0-0.dll \
	libpixman-1-0.dll \
	libpng14-14.dll \
	libpng15-15.dll \
	libpthread-2.dll \
	libsqlite3-0.dll \
	libvorbis-0.dll \
	libvorbisfile-3.dll \
	libxml2-2.dll \
	mgwhelp.dll \
	pthreadGC2.dll \
	ssleay32.dll \
	zlib1.dll \
	SDL2.dll \
	SDL2_mixer.dll

install-env-common:
	# add DLLs
	cp -R $(addprefix /bin/, $(COMMON_DLLS)) install-$(GUI)

DEFAULT_FCMP_DLLS := \
	libgdk-3-0.dll \
	libgtk-3-0.dll

install-env-default-fcmp: install-env-common
	cp -R $(addprefix /bin/, $(DEFAULT_FCMP_DLLS)) install-$(GUI)
	# add additional GTK+3 files
	mkdir -p install-$(GUI)/etc
	cp -R /etc/gtk-3.0 install-$(GUI)/etc/
	mkdir -p install-$(GUI)/lib/gtk-3.0/
	cp -R /lib/gtk-3.0/3.0.0 install-$(GUI)/lib/gtk-3.0/
	mkdir -p install-$(GUI)/share/glib-2.0/schemas
	cp /share/glib-2.0/schemas/gschemas.compiled install-$(GUI)/share/glib-2.0/schemas/
	cp -R /share/locale install-$(GUI)/share/
	find install-$(GUI)/share/locale -type f -not -name "freeciv*.mo" -and -not -name gtk30.mo -delete

install-env-gtk3: install-env-default-fcmp

SDL2_DLLS := \
	libpng16-16.dll \
	libSDL2_gfx-1-0-0.dll \
	SDL2_image.dll \
	SDL2_ttf.dll

install-env-sdl2: install-env-default-fcmp
	# add DLLs
	cp -R $(addprefix /bin/, $(SDL2_DLLS)) install-$(GUI)

QT_DLLS := \
	libstdc++-6.dll \
	Qt5Core.dll \
	Qt5Gui.dll \
	Qt5Widgets.dll

install-env-qt: install-env-common
	# add DLLs
	cp -R $(addprefix /bin/, $(QT_DLLS)) install-$(GUI)
	# add additional Qt files
	cp -R /plugins install-$(GUI)/
	cp -R /share/locale install-$(GUI)/share/
	find install-$(GUI)/share/locale -type f -not -name "freeciv*.mo" -delete

install-env-ruledit:
	# add DLLs
	cp -R $(addprefix /bin/, $(COMMON_DLLS)) install-ruledit
	cp -R $(addprefix /bin/, $(QT_DLLS)) install-ruledit
	# add additional Qt files
	cp -R /plugins install-ruledit/
	cp -R /share/locale install-ruledit/share/
	find install-ruledit/share/locale -type f -not -name "freeciv-ruledit.mo" -delete

# build installer

installer-common: clean-install-common install-freeciv-$(GUI) install-env-$(GUI)
	# extract Freeciv version
	$(eval FREECIV_VERSION := $(shell ../../fc_version))
	# create NSIS script
	./create-freeciv-$(GUI)-nsi.sh install-$(GUI) $(FREECIV_VERSION) > Freeciv-$(FREECIV_VERSION)-$(GUI).nsi
	# create installer executable
	mkdir -p Output
	makensis Freeciv-$(FREECIV_VERSION)-$(GUI).nsi

installer-ruledit: clean-install-ruledit install-ruledit install-env-ruledit
	# extract Freeciv version
	$(eval FREECIV_VERSION := $(shell ../../fc_version))
	# create NSIS script
	./create-freeciv-ruledit-nsi.sh install-ruledit $(FREECIV_VERSION) > Freeciv-$(FREECIV_VERSION)-ruledit.nsi
	# create installer executable
	mkdir -p Output
	makensis Freeciv-$(FREECIV_VERSION)-ruledit.nsi

wrap-gtk3: build-freeciv-gtk3 installer-common

wrap-sdl2: build-freeciv-sdl2 installer-common

wrap-qt: build-freeciv-qt installer-common

wrap-ruledit: build-freeciv-ruledit installer-ruledit


# cleanup

clean-build-common:
	rm -rf build-$(GUI)

clean-build-ruledit:
	rm -rf build-ruledit

clean-install-common:
	rm -rf install-$(GUI)
	./cleanup_checkout.sh ../..

clean-install-ruledit:
	rm -rf install-ruledit
	./cleanup_checkout.sh ../..

clean-installer-common:
	rm -f Output/Freeciv-*-$(GUI)-setup.exe
	rm -f Freeciv-*-$(GUI).nsi

clean-installer-ruledit:
	rm -f Output/Freeciv-ruledit-*-setup.exe
	rm -f Freeciv-*-ruledit.nsi

clean:
	make GUI=gtk3 clean-build-common clean-install-common clean-installer-common
	make GUI=sdl2 clean-build-common clean-install-common clean-installer-common
	make GUI=qt clean-build-common clean-install-common clean-installer-common
	make clean-build-ruledit clean-install-ruledit clean-installer-ruledit
