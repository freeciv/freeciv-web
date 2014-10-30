#!/bin/sh

if test "x$(dirname $0)" != "x." ; then
  echo "Currently this script must be run on its own directory" >&2
  exit 1
fi

# Remove old version
rm -Rf freeciv

. ./version.txt

if ! svn --quiet export svn://svn.gna.org/svn/freeciv/$FCBRANCH -r $FCREV freeciv ; then
  echo "Svn export failed" >&2
  exit 1
fi

if ! ./apply_patches.sh ; then
  echo "Patching failed" >&2
  exit 1
fi

( cd freeciv

  ./autogen.sh --enable-fcweb --disable-nls --disable-debug --disable-fcmp --disable-freeciv-manual --disable-ruledit --enable-ai-static=classic,threaded && make
)
