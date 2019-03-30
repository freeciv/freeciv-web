## Process this file with automake to produce Makefile.in

dist_noinst_DATA = \
		client.ico 	\
		mp.ico		\
		server.ico 	\
		clienticon.rc 	\
		mpicon.rc	\
		servericon.rc	\
		installer_msys2/licenses/COPYING.installer	\
		installer_msys2/create-freeciv-gtk-qt-nsi.sh	\
		installer_msys2/create-freeciv-gtk3.22-nsi.sh	\
		installer_msys2/create-freeciv-qt-nsi.sh	\
		installer_msys2/create-freeciv-sdl2-nsi.sh	\
		installer_msys2/create-freeciv-ruledit-nsi.sh	\
		installer_msys2/freeciv-gtk3.22.cmd		\
		installer_msys2/freeciv-mp-gtk3.cmd		\
		installer_msys2/freeciv-mp-gtk3x.cmd		\
		installer_msys2/freeciv-mp-qt.cmd		\
		installer_msys2/freeciv-qt.cmd			\
		installer_msys2/freeciv-sdl2.cmd		\
		installer_msys2/freeciv-server.cmd		\
		installer_msys2/freeciv-ruledit.cmd		\
		installer_msys2/Freeciv.url			\
		installer_msys2/Makefile			\
		installer_msys2/cleanup_checkout.sh		\
		installer_msys2/win_arch.sh			\
		installer_cross/create-freeciv-gtk-qt-nsi.sh	\
		installer_cross/create-freeciv-sdl2-nsi.sh	\
		installer_cross/create-freeciv-ruledit-nsi.sh	\
		installer_cross/installer_build.sh		\
		installer_cross/build_all_installers.sh		\
		installer_cross/winbuild.sh			\
		installer_cross/Freeciv.url			\
		installer_cross/freeciv-server.cmd		\
		installer_cross/freeciv-gtk3.22.cmd		\
		installer_cross/freeciv-mp-gtk3.cmd		\
		installer_cross/freeciv-qt.cmd			\
		installer_cross/freeciv-mp-qt.cmd		\
		installer_cross/freeciv-sdl2.cmd		\
		installer_cross/freeciv-ruledit.cmd		\
		installer_cross/licenses/COPYING.installer	\
		installer_cross/helpers/installer-helper-gtk3.cmd \
		installer_cross/helpers/installer-helper-qt.cmd

if MINGW
nodist_noinst_DATA = 		\
		clienticon.o 	\
		mpicon.o	\
		servericon.o

CLEANFILES = $(nodist_noinst_DATA)

clienticon.o: clienticon.rc
	$(WINDRES) -I$(srcdir) -i $(srcdir)/clienticon.rc -o clienticon.o

mpicon.o: mpicon.rc
	$(WINDRES) -I$(srcdir) -i $(srcdir)/mpicon.rc -o mpicon.o

servericon.o: servericon.rc
	$(WINDRES) -I$(srcdir) -i $(srcdir)/servericon.rc -o servericon.o
endif
