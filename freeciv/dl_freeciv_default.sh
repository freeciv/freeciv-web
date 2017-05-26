#!/bin/sh

# Downloads the wanted revision of Freeciv via Subversion.
# This will happen unless the script dl_freeciv.sh exists.
# If you wish to modify this file copy it to dl_freeciv.sh and edit it.

#svn --quiet export svn://svn.gna.org/svn/freeciv/$2 -r $1 freeciv

#Temporary git repo for Freeciv. 
#Note to self: say thanks to JTN for setting up new Git repo for Freeciv.
git clone git://repo.or.cz/freeciv.git  --depth=10

