#!/bin/bash

# Places the specified revision of Freeciv in freeciv/freeciv/
# This is the default. The script dl_freeciv.sh will run instead of
# dl_freeciv_default.sh if it exists.
# If you want to modify this file copy it to dl_freeciv.sh and edit it.

echo "Updating freeciv to commit $1"

# Remove old version
echo "  removing existing source"
rm -Rf freeciv

# Download the wanted Freeciv revision from GitHub unless it is here already.
# The download step saves having to merge in Freeciv's history each time the
# Freeciv server revision is updated.
echo "  fetching missing revisions"
git cat-file -e $1 || git fetch --no-tags --depth=1 https://github.com/freeciv/freeciv.git $1:freeciv

# Place the requested Freeciv revision in the freeciv/freeciv folder.
# The checkout isn't owned by git. This means that the patches automatically
# applied during the build won't accidentally end up in commits. It also
# means that committing unrelated changes won't accidentally revert the
# Freeciv server revision because a command didn't run.
echo "  checking out commit $1"
git read-tree --prefix=freeciv/freeciv/ --index-output=.freeciv_index $1
mkdir freeciv && cd freeciv && GIT_INDEX_FILE=.freeciv_index git checkout-index -af && cd ..
rm -f ../.freeciv_index
