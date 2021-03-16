#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd "${DIR}"

# Fix line endings on Windows
sed -i 's/\r$//' freeciv-web.project

. ./version.txt

# Allow the user to override how Freeciv is downloaded.
if test -f dl_freeciv.sh ; then
  DL_FREECIV=dl_freeciv.sh
else
  DL_FREECIV=dl_freeciv_default.sh
fi

if ! ./$DL_FREECIV $FCREV ; then
  echo "Git checkout failed" >&2
  exit 1
fi

if ! ./apply_patches.sh ; then
  echo "Patching failed" >&2
  exit 1
fi

echo "Copying Freeciv to /tmp, and compiling Freeciv in /tmp"   
# Copying Freeciv to /tmp, is a workaround for compiling Freeciv in a Vagrant box on VirtualBox, since in this environment
# autogen.sh can fail with this error:   "./conftest: Permission denied", which seems to be related to executable file permissions in a VirtualBox file system.
cp freeciv /tmp -rf
cp freeciv-web.project /tmp

( cd /tmp/freeciv
  ./autogen.sh CFLAGS="-O3" --enable-mapimg=magickwand --with-project-definition=../freeciv-web.project --enable-fcweb --enable-json --disable-delta-protocol --disable-nls --disable-fcmp --enable-freeciv-manual --disable-ruledit --disable-ruleup --disable-fcdb --enable-ai-static=classic,tex --prefix=${HOME}/freeciv/ && make -s -j$(nproc)
)

cp -rfu /tmp/freeciv .  || echo "done"
