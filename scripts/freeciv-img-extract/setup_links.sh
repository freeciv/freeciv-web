#!/bin/sh
# Script used by Freeciv-img-extract to get access to Freeciv image files.
# Will first try to setup symlinks to the Freeciv tileset, then simply
# copy the Freeciv images over if symlinks are not working on the OS (Windows).

ln -fs ../../freeciv/freeciv/data/amplio2 . || cp -rf ../../freeciv/freeciv/data/amplio2 .
ln -fs ../../freeciv/freeciv/data/isotrident . || cp -rf ../../freeciv/freeciv/data/isotrident .
ln -fs ../../freeciv/freeciv/data/trident . || cp -rf ../../freeciv/freeciv/data/trident .
ln -fs ../../freeciv/freeciv/data/buildings . || cp -rf ../../freeciv/freeciv/data/buildings .
ln -fs ../../freeciv/freeciv/data/flags . || cp -rf ../../freeciv/freeciv/data/flags . 
ln -fs ../../freeciv/freeciv/data/misc . || cp -rf ../../freeciv/freeciv/data/misc .
ln -fs ../../freeciv/freeciv/data/wonders . || cp -rf ../../freeciv/freeciv/data/wonders .
