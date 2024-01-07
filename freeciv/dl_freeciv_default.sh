#!/usr/bin/env -S bash -e

# Places the specified revision of Freeciv in freeciv/freeciv/
# This is the default. The script dl_freeciv.sh will run instead of
# dl_freeciv_default.sh if it exists.
# If you want to modify this file copy it to dl_freeciv.sh and edit it.

if test "$2" != "" ; then
  GIT_PATCHING="$2"
else
  GIT_PATCHING="yes"
fi

echo "Updating freeciv to commit $1, git patching: $GIT_PATCHING"

# Remove old version
echo "  removing existing source"
rm -Rf freeciv

if test "$GIT_PATCHING" = "yes" ; then

  # Fetch one commit from freeciv
  git clone --no-tags --branch=main --single-branch https://github.com/freeciv/freeciv.git freeciv
  ( cd freeciv && git checkout $1 )

else

  # Download the wanted Freeciv revision from GitHub unless it is here already.
  # The download step saves having to merge in Freeciv's history each time the
  # Freeciv server revision is updated.
  echo "  fetching missing revisions"
  git cat-file -e $1 || git fetch --no-tags --depth=1 https://github.com/freeciv/freeciv.git $1:freeciv || /bin/true

  # Place the requested Freeciv revision in the freeciv/freeciv folder.
  # The checkout isn't owned by git. This means that the patches automatically
  # applied during the build won't accidentally end up in commits. It also
  # means that committing unrelated changes won't accidentally revert the
  # Freeciv server revision because a command didn't run.
  echo "  checking out commit $1"
  git read-tree --prefix=freeciv/freeciv/ --index-output=.freeciv_index $1
  mkdir freeciv && cd freeciv && GIT_INDEX_FILE=.freeciv_index git checkout-index -af && cd ..
  rm -f ../.freeciv_index

fi
