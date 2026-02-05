#!/usr/bin/env bash


declare -a PATCHLIST=(
  "create_virtual_sockets_interface"
  "disable_curl"
  "switch_netintf_to_virtual"
  "switch_sernet_to_virtual"
  "fix_missing_type_declaration"
  "add_breaks_into_loops_for_js"
  "update-cross-file"
  "make_emsbuild_for_server"
  "update-meson-build"
)

apply_patch() {
  echo "*** Applying $1.patch ***"
  if ! patch -u -p1 -d freeciv < patches-wasm-server/$1.patch ; then
    echo "APPLYING PATCH $1.patch FAILED!"
    return 1
  fi
  echo "=== $1.patch applied ==="
}

for patch in "${PATCHLIST[@]}"
do
  if test "${patch}.patch" = "$APPLY_UNTIL" ; then
    echo "$patch not applied as requested to stop"
    break
  fi
  if ! apply_patch $patch ; then
    echo "Patching failed ($patch.patch)" >&2
    exit 1
  fi
done
