## Process this file with automake to produce Makefile.in

## Override automake so that "make install" puts these in proper place:
pkgdatadir = $(datadir)/$(PACKAGE)/themes/gtk3.22/Freeciv/gtk-3.0

SUBDIRS = Arrows Buttons Check-Radio Combo Frame-Gap Handles Lines \
          ListHeaders Menu-Menubar Others Panel ProgressBar Range \
          Scrollbars Shadows Spin Tabs Toolbar

pkgdata_DATA =          	\
	bg.png			\
	gtk.css			\
	menubar.css

EXTRA_DIST = $(pkgdata_DATA)
