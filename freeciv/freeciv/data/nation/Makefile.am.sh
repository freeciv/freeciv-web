#!/bin/sh
#
# Regenerate Makefile.am based on actual contents of directory.
# This is all so 'make distcheck' will work.
#
cat <<EOF
## Process this file with automake to produce Makefile.in
# Note: After adding a new nation file, 'make Makefile.am'

## Override automake so that "make install" puts these in proper place:
pkgdatadir = \$(datadir)/@PACKAGE@/nation

pkgdata_DATA = \\
`find * -name "*.ruleset" -print | sed -e 's/.*ruleset$/		& \\\/' -e '$s/.$//'`

EXTRA_DIST = \$(pkgdata_DATA) Makefile.am.sh

EOF
