#!/bin/sh

if test "x$(dirname $0)" != "x." ; then
  echo "Currently this script must be run on its own directory" >&2
  exit 1
fi

# Remove old version
rm -Rf freeciv

. ./version.txt

# Allow the user to override how Freeciv is downloaded.
if test -f dl_freeciv.sh ; then
  FC_DL=dl_freeciv.sh
else
  FC_DL=dl_freeciv_default.sh
fi

if ! sh $FC_DL $FCREV $FCBRANCH ; then
  echo "Svn export failed" >&2
  exit 1
fi

if ! ./apply_patches.sh ; then
  echo "Patching failed" >&2
  exit 1
fi

( cd freeciv

  ./autogen.sh CFLAGS="-O3" --with-project-definition=../freeciv-web.project --enable-fcweb --enable-json --disable-delta-protocol --disable-nls --disable-fcmp --enable-freeciv-manual=html --disable-ruledit --enable-ai-static=classic,threaded --prefix=${HOME}/freeciv/ && make -s -j$(nproc)
)
