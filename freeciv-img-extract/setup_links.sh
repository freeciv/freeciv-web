#!/bin/sh
# Script used by Freeciv-img-extract to get access to Freeciv image files.
# Will first try to setup symlinks to the Freeciv tileset, then simply
# copy the Freeciv images over if symlinks are not working on the OS (Windows).

ln -s ../freeciv/freeciv/data/amplio2 . || cp -rf ../freeciv/freeciv/data/amplio2 .
ln -s ../freeciv/freeciv/data/buildings . || cp -rf ../freeciv/freeciv/data/buildings .
ln -s ../freeciv/freeciv/data/flags . || cp -rf ../freeciv/freeciv/data/flags . 
ln -s ../freeciv/freeciv/data/misc . || cp -rf ../freeciv/freeciv/data/misc .
ln -s ../freeciv/freeciv/data/wonders . || cp -rf ../freeciv/freeciv/data/wonders .
