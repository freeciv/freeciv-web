#
# Makefile for Freeciv Windows installer creation
#
# See doc/README.msys2 for building instructions.
#

#
# Variable setup
#

WINARCH=$(shell ./win_arch.sh)

ifeq ($(WINARCH),win64)
ARCHDIR=mingw64
LIBGCCDLL=libgcc_s_seh-1.dll
GCCDLL=gcc_s-seh-1.dll
CRYPTOLIB_SUFFIX=-x64
else
ARCHDIR=mingw32
LIBGCCDLL=libgcc_s_dw2-1.dll
GCCDLL=gcc_s-1.dll
CRYPTOLIB_SUFFIX=
endif

ifeq ($(MIN_WIN_VER),)
MIN_WIN_VER=0x0601
endif

INSTSUB=${ARCHDIR}

# msys2 has dlls in...
DLLPATH_PREFIX=/$(ARCHDIR)/bin/

#
# Make rules
#

# by default build all installers that are buildable with current msys2 environment
all: gtk3.22-installer sdl2-installer ruledit-installer qt-installer

gtk3.22-installer:
	make GUI=gtk3.22 FCMP=gtk3 EXTRA_CONFIG="--disable-ruledit $(EXTRA_CONFIG)" wrap-gtk3.22

# These are what can be built in current official environment version
buildable-win32: gtk3.22-installer sdl2-installer ruledit-installer qt-installer

buildable-win64: gtk3.22-installer sdl2-installer ruledit-installer qt-installer

# See comments on 'snapshot' target why this wrapping is needed
buildable: buildable-$(WINARCH)

sdl2-installer:
	make GUI=sdl2 FCMP=gtk3 EXTRA_CONFIG="--disable-ruledit $(EXTRA_CONFIG)" wrap-sdl2

qt-installer:
	make GUI=qt FCMP=qt EXTRA_CONFIG="--with-qt5-includes=/$(ARCHDIR)/include --with-qt5-libs=/$(ARCHDIR)/lib $(EXTRA_CONFIG)" wrap-qt

ruledit-installer:
	make wrap-ruledit

# Autogen.sh here will make ./win_arch.sh to work.
# The $(WINACRH) value we got for this run is not good. The new run
# launched from this target will pick correct value, and can make 'buildable' target
# to depend on correct arch version.
snapshot:
	cd ../.. ; ./autogen.sh --no-configure-run
	make VERSION_REVTYPE="git" EXTRA_CONFIG="--enable-gitrev --with-project-definition=../../../bootstrap/snapshot.project" buildable


# configure Freeciv

configure-freeciv-common:
	# create build directory
	mkdir -p build-$(WINARCH)-$(GUI)
	# configure
	cd build-$(WINARCH)-$(GUI); ../../../configure FREECIV_LABEL_FORCE="<base>-msys2" CPPFLAGS="-D_WIN32_WINNT=$(MIN_WIN_VER)" --enable-client=$(GUI) --with-followtag="windows-S3_1" --with-tinycthread --without-readline --disable-sys-lua --enable-fcdb=sqlite3 --with-sqlite3-prefix=/$(ARCHDIR) --enable-fcmp=$(FCMP) $(EXTRA_CONFIG)

configure-freeciv-ruledit:
	# create build directory
	mkdir -p build-$(WINARCH)-ruledit
	# configure
	cd build-$(WINARCH)-ruledit; ../../../configure FREECIV_LABEL_FORCE="<base>-msys2" CPPFLAGS="-D_WIN32_WINNT=$(MIN_WIN_VER)" --disable-client --disable-server --disable-fcmp --disable-freeciv-manual --disable-sys-lua --enable-ruledit --enable-ai-static=stub --with-qt5-includes=/$(ARCHDIR)/include --with-qt5-libs=/$(ARCHDIR)/lib $(EXTRA_CONFIG)

# install Freeciv

install-freeciv-common: configure-freeciv-common
	# make langstat files
	make -C build-$(WINARCH)-$(GUI)/translations/core update-po
	make -C build-$(WINARCH)-$(GUI)/bootstrap langstat_core.txt
	# make install
	make -C build-$(WINARCH)-$(GUI) DESTDIR=`pwd`/install-$(WINARCH)-$(GUI) $(MAKE_PARAMS) install
	# reorder directory layout
	mv install-$(WINARCH)-$(GUI)/$(INSTSUB)/bin/* install-$(WINARCH)-$(GUI)/
	mv install-$(WINARCH)-$(GUI)/$(INSTSUB)/etc install-$(WINARCH)-$(GUI)/
	mv install-$(WINARCH)-$(GUI)/$(INSTSUB)/share/freeciv install-$(WINARCH)-$(GUI)/data
	mkdir -p install-$(WINARCH)-$(GUI)/share
	mv install-$(WINARCH)-$(GUI)/$(INSTSUB)/share/locale install-$(WINARCH)-$(GUI)/share/
	mv install-$(WINARCH)-$(GUI)/$(INSTSUB)/share/doc install-$(WINARCH)-$(GUI)/doc
	mkdir -p install-$(WINARCH)-$(GUI)/debuginfo
	cp build-$(WINARCH)-$(GUI)/gen_headers/fc_config.h install-$(WINARCH)-$(GUI)/debuginfo
	mkdir -p install-$(WINARCH)-$(GUI)/doc/freeciv/installer
	cp licenses/COPYING.installer install-$(WINARCH)-$(GUI)/doc/freeciv/installer/
	# delete unneeded files
	rm -r install-$(WINARCH)-$(GUI)/$(INSTSUB)
	# strip 'freeciv-manual' and 'freeciv-mp-<gui>' executables
	strip install-$(WINARCH)-$(GUI)/freeciv-m*
	# add start menu files
	cp freeciv-server.cmd freeciv-mp-$(FCMP).cmd freeciv-$(GUI).cmd Freeciv.url install-$(WINARCH)-$(GUI)/

install-freeciv-gtk3.22: install-freeciv-common

install-freeciv-sdl2: install-freeciv-common

install-freeciv-qt: install-freeciv-common
	# strip 'freeciv-ruledit' executable
	strip install-$(WINARCH)-$(GUI)/freeciv-ruledit.exe
	cp freeciv-ruledit.cmd install-$(WINARCH)-$(GUI)/

install-ruledit-dir:
	mkdir -p install-$(WINARCH)-ruledit/share

install-ruledit: configure-freeciv-ruledit install-ruledit-dir
	# make langstat files
	make -C build-$(WINARCH)-ruledit/translations/ruledit update-po
	make -C build-$(WINARCH)-ruledit/bootstrap langstat_ruledit.txt
	# make install
	make -C build-$(WINARCH)-ruledit DESTDIR=`pwd`/install-$(WINARCH)-ruledit/ $(MAKE_PARAMS) install
	# reorder directory layout
	mv install-$(WINARCH)-ruledit/$(INSTSUB)/bin/* install-$(WINARCH)-ruledit/
	mv install-$(WINARCH)-ruledit/$(INSTSUB)/share/freeciv install-$(WINARCH)-ruledit/data
	mv install-$(WINARCH)-ruledit/$(INSTSUB)/share/locale install-$(WINARCH)-ruledit/share/
	mv install-$(WINARCH)-ruledit/$(INSTSUB)/share/doc install-$(WINARCH)-ruledit/doc
	mkdir -p install-$(WINARCH)-ruledit/debuginfo
	cp build-$(WINARCH)-ruledit/gen_headers/fc_config.h install-$(WINARCH)-ruledit/debuginfo
	mkdir -p install-$(WINARCH)-ruledit/doc/freeciv/installer
	cp licenses/COPYING.installer install-$(WINARCH)-ruledit/doc/freeciv/installer/
	# delete unneeded files
	rm -r install-$(WINARCH)-ruledit/$(INSTSUB)
	# strip 'freeciv-ruledit' executable
	strip install-$(WINARCH)-ruledit/freeciv-ruledit.exe
	# add start menu files
	cp freeciv-ruledit.cmd Freeciv.url install-$(WINARCH)-ruledit/

# install Freeciv environment

SOUND_DLLS := \
	SDL2.dll \
	SDL2_mixer.dll \
	libFLAC-8.dll \
	libfluidsynth-2.dll \
	libmodplug-1.dll \
	libvorbis-0.dll \
	libvorbisenc-2.dll \
	libvorbisfile-3.dll \
	libogg-0.dll \
	libportaudio-2.dll \
	libsndfile-1.dll \
	libspeex-1.dll \
	libmpg123-0.dll

COMMON_DLLS := \
	libstdc++-6.dll \
	libcrypto-1_1${CRYPTOLIB_SUFFIX}.dll \
	libpsl-5.dll \
	libssl-1_1${CRYPTOLIB_SUFFIX}.dll \
	libidn2-0.dll \
	libnghttp2-14.dll \
	libssh2-1.dll \
	libcurl-4.dll \
	libgmp-10.dll \
	libp11-kit-0.dll \
	libtasn1-6.dll \
	libatk-1.0-0.dll \
	libbz2-1.dll \
	libcairo-2.dll \
	libcairo-gobject-2.dll \
	libffi-6.dll \
	libfontconfig-1.dll \
	libfreetype-6.dll \
	$(LIBGCCDLL) \
	libgdk_pixbuf-2.0-0.dll \
	libgio-2.0-0.dll \
	libglib-2.0-0.dll \
	libgmodule-2.0-0.dll \
	libgobject-2.0-0.dll \
	libgthread-2.0-0.dll \
	libiconv-2.dll \
	libintl-8.dll \
	liblzma-5.dll \
	libpango-1.0-0.dll \
	libpangocairo-1.0-0.dll \
	libpangowin32-1.0-0.dll \
	libfribidi-0.dll \
	libpixman-1-0.dll \
	libpng16-16.dll \
	libxml2-2.dll \
	zlib1.dll \
	libwinpthread-1.dll \
	libicuuc62.dll \
	libicudt62.dll \
	libsqlite3-0.dll \
	exchndl.dll \
	mgwhelp.dll \
	libbrotlidec.dll \
	libbrotlicommon.dll \
	$(SOUND_DLLS)

install-env-common:
	# add DLLs
	cp -R $(addprefix $(DLLPATH_PREFIX), $(COMMON_DLLS)) install-$(WINARCH)-$(GUI)/

DEFAULT_FCMP_DLLS := \
	libgdk-3-0.dll \
	libgtk-3-0.dll \
	libexpat-1.dll \
	libharfbuzz-0.dll \
	libepoxy-0.dll \
	libgraphite2.dll \
	libpangoft2-1.0-0.dll \
	libthai-0.dll \
	libdatrie-1.dll \
	libpcre-1.dll \
	libunistring-2.dll \
	librsvg-2-2.dll \
	libcroco-0.6-3.dll

install-env-default-fcmp: install-env-common
	cp -R $(addprefix $(DLLPATH_PREFIX), $(DEFAULT_FCMP_DLLS)) install-$(WINARCH)-$(GUI)/
	# add additional GTK+3 files
	mkdir -p install-$(WINARCH)-$(GUI)/etc
	cp -R /$(ARCHDIR)/etc/gtk-3.0 install-$(WINARCH)-$(GUI)/etc/
	mkdir -p install-$(WINARCH)-$(GUI)/share/icons
	cp -R /$(ARCHDIR)/share/icons/Adwaita install-$(WINARCH)-$(GUI)/share/icons/
	mkdir -p install-$(WINARCH)-$(GUI)/share/glib-2.0/schemas
	cp /$(ARCHDIR)/share/glib-2.0/schemas/gschemas.compiled install-$(WINARCH)-$(GUI)/share/glib-2.0/schemas/
	mkdir -p install-$(WINARCH)-$(GUI)/lib/gtk-3.0/
	cp -R /$(ARCHDIR)/lib/gtk-3.0/* install-$(WINARCH)-$(GUI)/lib/gtk-3.0/
	mkdir -p install-$(WINARCH)-$(GUI)/lib/gdk-pixbuf-2.0/
	cp -R /$(ARCHDIR)/lib/gdk-pixbuf-2.0/* install-$(WINARCH)-$(GUI)/lib/gdk-pixbuf-2.0/
	cp -R /$(ARCHDIR)/share/locale install-$(WINARCH)-$(GUI)/share/
	find install-$(WINARCH)-$(GUI)/share/locale -type f -not -name "freeciv*.mo" -and -not -name gtk30.mo -delete

install-env-gtk3.22: install-env-default-fcmp

SDL2_DLLS := \
	libSDL2_gfx-1-0-0.dll \
	SDL2_image.dll \
	SDL2_ttf.dll \
	libjpeg-8.dll \
	libtiff-5.dll \
	libwebp-7.dll

install-env-sdl2: install-env-default-fcmp
	# add DLLs
	cp -R $(addprefix $(DLLPATH_PREFIX), $(SDL2_DLLS)) install-$(WINARCH)-$(GUI)/

QT_DLLS := \
	libicuin62.dll \
	libicuuc62.dll \
	libicudt62.dll \
	libpcre16-0.dll \
	libharfbuzz-0.dll \
	libsqlite3-0.dll \
	Qt5Core.dll \
	Qt5Gui.dll \
	Qt5Widgets.dll \
	libgraphite2.dll \
	libpcre-1.dll \
	libpcre2-16-0.dll \
	libunistring-2.dll \
	libbrotlidec.dll \
	libbrotlicommon.dll

install-env-qt: install-env-common
	# add DLLs
	cp -R $(addprefix $(DLLPATH_PREFIX), $(QT_DLLS)) install-$(WINARCH)-$(GUI)/
	# add additional Qt files
	cp -R /$(ARCHDIR)/share/qt5/plugins install-$(WINARCH)-$(GUI)/
	cp -R /$(ARCHDIR)/share/locale install-$(WINARCH)-$(GUI)/share/
	find install-$(WINARCH)-$(GUI)/share/locale -type f -not -name "freeciv*.mo" -delete

RULEDIT_DLLS := \
        libstdc++-6.dll \
	libcrypto-1_1${CRYPTOLIB_SUFFIX}.dll \
	libpsl-5.dll \
	libssl-1_1${CRYPTOLIB_SUFFIX}.dll \
	libidn2-0.dll \
	libnghttp2-14.dll \
	libssh2-1.dll \
	libcurl-4.dll \
	libgmp-10.dll \
	libp11-kit-0.dll \
	libtasn1-6.dll \
	libatk-1.0-0.dll \
	libbz2-1.dll \
	libcairo-2.dll \
	libcairo-gobject-2.dll \
	libffi-6.dll \
	libfontconfig-1.dll \
	libfreetype-6.dll \
	$(LIBGCCDLL) \
	libglib-2.0-0.dll \
	libgdk_pixbuf-2.0-0.dll \
	libgio-2.0-0.dll \
	libiconv-2.dll \
	libintl-8.dll \
	liblzma-5.dll \
	libpango-1.0-0.dll \
	libpangocairo-1.0-0.dll \
	libpangowin32-1.0-0.dll \
	libpixman-1-0.dll \
	libpng16-16.dll \
	libxml2-2.dll \
	zlib1.dll \
	libwinpthread-1.dll \
	libgraphite2.dll \
	libpcre-1.dll

install-env-ruledit:
	# add DLLs
	cp -R $(addprefix $(DLLPATH_PREFIX), $(RULEDIT_DLLS)) install-$(WINARCH)-ruledit/
	cp -R $(addprefix $(DLLPATH_PREFIX), $(QT_DLLS)) install-$(WINARCH)-ruledit/
	# add additional Qt files
	cp -R /$(ARCHDIR)/share/qt5/plugins install-$(WINARCH)-ruledit/
	cp -R /usr/share/locale install-$(WINARCH)-ruledit/share/
	find install-$(WINARCH)-ruledit/share/locale -type f -not -name "freeciv-ruledit.mo" -delete

# build installer

installer-common: clean-install-common install-freeciv-$(GUI) install-env-$(GUI)
	# extract Freeciv version
	$(eval FREECIV_VERSION := $(shell ../../fc_version))
	# create NSIS script
	./create-freeciv-$(GUI)-nsi.sh install-$(WINARCH)-$(GUI) $(FREECIV_VERSION) $(WINARCH) > Freeciv-$(WINARCH)-$(FREECIV_VERSION)-$(GUI).nsi
	# create installer executable
	mkdir -p Output
	makensis Freeciv-$(WINARCH)-$(FREECIV_VERSION)-$(GUI).nsi

installer-ruledit: clean-install-ruledit install-ruledit install-env-ruledit
	# extract Freeciv version
	$(eval FREECIV_VERSION := $(shell ../../fc_version))
	# create NSIS script
	./create-freeciv-ruledit-nsi.sh install-$(WINARCH)-ruledit $(FREECIV_VERSION) $(WINARCH) > Freeciv-$(WINARCH)-$(FREECIV_VERSION)-ruledit.nsi
	# create installer executable
	mkdir -p Output
	makensis Freeciv-$(WINARCH)-$(FREECIV_VERSION)-ruledit.nsi

wrap-gtk3.22: installer-common

wrap-sdl2: installer-common

wrap-qt: installer-common

wrap-ruledit: installer-ruledit


# cleanup

clean-build-common:
	rm -rf build-*-$(GUI)

clean-build-ruledit:
	rm -rf build-*-ruledit

clean-install-common:
	rm -rf install-*-$(GUI)
	./cleanup_checkout.sh ../..

clean-install-ruledit:
	rm -rf install-*-ruledit
	./cleanup_checkout.sh ../..

clean-installer-common:
	rm -f Output/Freeciv-*-$(GUI)-setup.exe
	rm -f Freeciv-*-$(GUI).nsi

clean-installer-ruledit:
	rm -f Output/Freeciv-ruledit-*-setup.exe
	rm -f Freeciv-*-ruledit.nsi

clean:
	make GUI=gtk3.22 clean-build-common clean-install-common clean-installer-common
	make GUI=sdl2 clean-build-common clean-install-common clean-installer-common
	make GUI=qt clean-build-common clean-install-common clean-installer-common
	make clean-build-ruledit clean-install-ruledit clean-installer-ruledit
