#!/bin/sh

# Downloads the wanted revision of Freeciv via Git.
# This will happen unless the script dl_freeciv.sh exists.
# If you wish to modify this file copy it to dl_freeciv.sh and edit it.

git clone https://github.com/freeciv/freeciv.git --depth=10000
cd freeciv
git reset --hard $1
