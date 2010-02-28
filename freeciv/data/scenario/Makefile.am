## Process this file with automake to produce Makefile.in

zipped_files =					\
	british-isles-85x80-v2.80.sav.gz	\
	earth-160x90-v2.sav.gz			\
	earth-80x50-v2.sav.gz			\
	europe-200x100-v2.sav.gz		\
	hagworld-120x60-v1.2.sav.gz		\
	iberian-peninsula-136x100-v1.0.sav.gz	\
	tutorial.sav.gz

unzipped_files =				\
	british-isles-85x80-v2.80.sav		\
	earth-160x90-v2.sav			\
	earth-80x50-v2.sav			\
	europe-200x100-v2.sav			\
	hagworld-120x60-v1.2.sav		\
	iberian-peninsula-136x100-v1.0.sav	\
	tutorial.sav


## Override automake so that "make install" puts these in proper place:
pkgdatadir = $(datadir)/$(PACKAGE)/scenario
pkgdata_DATA = $(zipped_files)

$(zipped_files): %.sav.gz: %.sav
	$(GZIP) --best -c $< > $@

MOSTLYCLEANFILES = $(zipped_files)

EXTRA_DIST = $(unzipped_files)
