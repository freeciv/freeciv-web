#!/bin/bash

# Either version number to install, or "yes" to use system meson.
# Empty to use autotools
# Minimum requirement is 0.57.0, and Debian Bullseye has only 0.56.2
MESON_VER="0.57.2"

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

if test "$MESON_VER" != "" ; then
  if test "$MESON_VER" != "yes" ; then
    rm -Rf meson-install && mkdir -p meson-install

    ( cd meson-install

      wget "https://github.com/mesonbuild/meson/releases/download/${MESON_VER}/meson-${MESON_VER}.tar.gz"
      tar xzf meson-${MESON_VER}.tar.gz
      ln -s "meson-${MESON_VER}/meson.py" meson
    )

    export PATH="$(pwd)/meson-install:$PATH"
  fi

  mkdir -p build

  ( cd build
    meson ../freeciv -Dack_experimental=true -Dfreeciv-web=true \
          -Dclients=[] -Dfcmp=[] -Djson-protocol=true -Dnls=false \
          -Daudio=false -Druledit=false \
          -Dproject-definition=../freeciv-web.fcproj \
          -Ddefault_library=static -Dprefix=${HOME}/freeciv
    ninja
  )

else

echo "Building Freeciv in /tmp"
# Building Freeciv in /tmp, is a workaround for compiling Freeciv in a Vagrant box on VirtualBox, since in this environment
# configure can fail with this error:   "./conftest: Permission denied", which seems to be related to executable file permissions in a VirtualBox file system.
mkdir -p /tmp/freeciv

# We have to have .project next to build dir, as refence to it does not work with
# absolute path (one that begins with '/')
cp freeciv-web.project /tmp/

( cd freeciv
  ./autogen.sh --no-configure-run --disable-nls
)

( cd /tmp/freeciv
  ${DIR}/freeciv/configure CFLAGS="-O3" \
              --enable-mapimg=magickwand --with-project-definition=../freeciv-web.project --enable-fcweb --enable-json --disable-delta-protocol --disable-nls --disable-fcmp --enable-freeciv-manual --disable-ruledit --disable-ruleup --disable-fcdb --enable-ai-static=classic,tex --prefix=${HOME}/freeciv/ && make -s -j$(nproc)
)

fi

echo "done"
