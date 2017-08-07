#!/bin/sh

# Places the specified revision of Freeciv in freeciv/freeciv/
# This is the default. The script dl_freeciv.sh will run instead of
# dl_freeciv_default.sh if it exists.
# If you want to modify this file copy it to dl_freeciv.sh and edit it.

# Download the wanted Freeciv revision from GitHub unless it is here already.
# The download step saves having to merge in Freeciv's history each time the
# Freeciv server revision is updated.
git cat-file -e $1 || git fetch --no-tags https://github.com/freeciv/freeciv.git master:freeciv_master

# Place the requested Freeciv revision in the freeciv/freeciv folder.
# The checkout isn't owned by git. This means that the patches automatically
# applied during the build won't accidentally end up in commits. It also
# means that committing unrelated changes won't accidentally revert the
# Freeciv server revision because a command didn't run.
git read-tree --prefix=freeciv/freeciv/ --index-output=.freeciv_index $1
GIT_INDEX_FILE=.freeciv_index git checkout-index -af
rm -f .freeciv_index
