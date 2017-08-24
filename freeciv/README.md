Freeciv C source checkout and patching
--------------------------------------

This directory is where the Freeciv C server will be checked out from Git, and patched to work with Freeciv-web in the browser.

Freeciv will be checkout out from here:  https://github.com/freeciv/freeciv

prepare_freeciv.sh  - a script which will checkout Freeciv from Git, then patch apply the patches and finally configure and compile the Freeciv C server.

apply_patches.sh - applies patches against the Freeciv C source code.

version.txt - contains the Git revision of Freeciv to check out from Git.

The Freeciv C server is installed to $HOME/freeciv.
