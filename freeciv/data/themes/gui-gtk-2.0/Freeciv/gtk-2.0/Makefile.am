## Process this file with automake to produce Makefile.in

## Override automake so that "make install" puts these in proper place:
pkgdatadir = $(datadir)/$(PACKAGE)/themes/gui-gtk-2.0/Freeciv/gtk-2.0

SUBDIRS = Arrows Buttons Check-Radio Combo Frame-Gap Handles Lines \
          ListHeaders Menu-Menubar Others Panel ProgressBar Range \
          Scrollbars Shadows Spin Tabs Toolbar

pkgdata_DATA =          	\
	bg.png			\
	gtkrc			\
	menubar.rc		\
	panel.rc

EXTRA_DIST = $(pkgdata_DATA)
