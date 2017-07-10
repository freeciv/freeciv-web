#!/bin/sh

# Downloads the wanted revision of Freeciv via Git.
# This will happen unless the script dl_freeciv.sh exists.
# If you wish to modify this file copy it to dl_freeciv.sh and edit it.

mkdir -p freeciv
cd freeciv
git init
git fetch https://github.com/freeciv/freeciv.git :$1
git reset --hard  $1

