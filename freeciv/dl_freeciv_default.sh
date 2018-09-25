#!/bin/bash

# Places the specified revision of Freeciv in freeciv/freeciv/
# This is the default. The script dl_freeciv.sh will run instead of
# dl_freeciv_default.sh if it exists.
# If you want to modify this file copy it to dl_freeciv.sh and edit it.
# Download the wanted Freeciv revision from GitHub unless it is here already.
if [ ! -d freeciv ]; then
    git clone https://github.com/freeciv/freeciv.git
fi