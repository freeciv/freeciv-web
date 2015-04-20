#!/bin/sh

# Downloads the wanted revision of Freeciv via Subversion.
# This will happen unless the script dl_freeciv.sh exists.
# If you wish to modify this file copy it to dl_freeciv.sh and edit it.

svn --quiet export svn://svn.gna.org/svn/freeciv/$2 -r $1 freeciv
