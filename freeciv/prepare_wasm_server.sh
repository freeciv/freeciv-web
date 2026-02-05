#!/usr/bin/env bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd "${DIR}"

export GIT_PATCHING="no"

. ./version.txt

# Allow the user to override how Freeciv is downloaded.
if test -f dl_freeciv.sh ; then
  DL_FREECIV=dl_freeciv.sh
else
  DL_FREECIV=dl_freeciv_default.sh
fi

if ! "./${DL_FREECIV}" "$FCREV" "$GIT_PATCHING" ; then
  echo "Git checkout failed" >&2
  exit 1
fi

if ! ./apply_patches.sh ; then
  echo "Patching failed" >&2
  exit 1
fi

if ! ./apply_wasm_server_patches.sh ; then
  echo "Patching WASM server failed" >&2
  exit 1
fi

if ! ./dl_jansson_lib.sh ; then
  echo "Can't download Jansson JSON library" >&2
  exit 1
fi

if ! ./build_jansson_lib.sh ; then
  echo "Can't compile Jansson JSON library for WASM" >&2
  exit 1
fi

rm -rf build
mkdir build
cd build
bash ../freeciv/platforms/emscripten/emsbuild.sh ~/github/emsdk/
cd ..
