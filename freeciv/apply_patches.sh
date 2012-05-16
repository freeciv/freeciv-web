#!/bin/sh

PATCHLIST="arctic-bugfix caravan_fixes1"

apply_patch() {
  patch -u -p1 -d freeciv.upstream < patches/$1.diff
}

for patch in $PATCHLIST
do
  if ! apply_patch $patch ; then
    echo "Patching failed" >&2
    exit 1
  fi
done
