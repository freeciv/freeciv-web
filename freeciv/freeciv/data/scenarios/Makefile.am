## Process this file with automake to produce Makefile.in

zipped_files =					\
	british-isles.sav.gz		\
	earth-large.sav.gz		\
	earth-small.sav.gz		\
	europe.sav.gz			\
	france.sav.gz			\
	hagworld.sav.gz			\
	iberian-peninsula.sav.gz	\
	italy.sav.gz			\
	japan.sav.gz			\
	north_america.sav.gz		\
	europe_1900_WWI.sav.gz		\
	tutorial.sav.gz

unzipped_files =			\
	british-isles.sav		\
	earth-large.sav			\
	earth-small.sav			\
	europe.sav			\
	france.sav			\
	hagworld.sav			\
	iberian-peninsula.sav		\
	italy.sav			\
	japan.sav			\
	north_america.sav		\
	europe_1900_WWI.sav		\
	tutorial.sav


## Override automake so that "make install" puts these in proper place:
pkgdatadir = $(datadir)/$(PACKAGE)/scenarios
pkgdata_DATA = $(zipped_files)

$(zipped_files): %.sav.gz: %.sav
	$(GZIP) --best -n -c $< > $@

MOSTLYCLEANFILES = $(zipped_files)

EXTRA_DIST = $(unzipped_files)
