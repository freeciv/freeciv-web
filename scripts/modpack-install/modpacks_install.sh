#!/usr/bin/env bash

# Install modpacks listed in freeciv/modpackURLs.lst

resolve() { echo "$(cd "$1" >/dev/null && pwd)"; }
while [[ $# -gt 0 ]]; do
  case $1 in
    -b) FCW_BASE_DIR=$(resolve "$2"); shift; shift;;
    -i) INSTALL_DIR=$(resolve "$2"); shift; shift;;
    *) echo "Unrecognized argument: $1"; shift;;
  esac
done
: ${FCW_BASE_DIR:?Must specify freeciv-web source root dir with -b}
: ${INSTALL_DIR:?Must specify freeciv install dir with -i}

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

RULEDIR="${HOME}/.freeciv-web"

freeciv_mp="${INSTALL_DIR}/bin/freeciv-mp-cli"

cat "${FCW_BASE_DIR}/freeciv/modpackURLs.lst" |
  ( while read URL REST ;
    do if test "${URL}" != "#" ; then
      if ! "${freeciv_mp}" -p "${RULEDIR}" -i "${URL}" ; then
        exit 1
      fi
    fi ; done )

echo "Installed modpacks in ${RULEDIR}"
