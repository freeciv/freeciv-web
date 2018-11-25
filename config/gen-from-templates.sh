#!/bin/bash

#   Copyright (C) 2018  The Freeciv-web project
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Substitutes variables from the configuration file in the templates.
# gen-from-templates [configfile [source_dir]]
# When source_dir is not given, configfile's parent is used.
# When configfile is not given, script_src_dir/config is used.

set -e
unset CDPATH

CONFIG="$1"
SRCDIR="$2"

if [ -z "${CONFIG}" ]; then
  CONFIG=${BASH_SOURCE%/*}
  if [ -z "${CONFIG}" ]; then
    echo >&2 "Configuration file not found."
    exit 2
  fi
  CONFIG="${CONFIG}"/config
fi

if [ "${CONFIG:0:1}" = - ]; then
  CONFIG=./"${CONFIG}"
fi

if [ ! -f "${CONFIG}" ]; then
  echo >&2 "Configuration file not found."
  exit 2
fi

if [ -z "${SRCDIR}" ]; then
  SRCDIR=${CONFIG%/*}/..
fi

. "${CONFIG}"

TMPFILE=$(mktemp)
while IFS= read -r line; do
  left=${line%%=*}
  if [ "${#left}" -gt 0 ] && [ "${left:0:1}" != '#' ]; then
    right=${!left}
    right=${right//\\/\\\\}
    right=${right////\\/}
    right=${right//&/\\&}
    echo -E "s/#${left}#/${right}/g"
  fi
done < "${CONFIG}" > "${TMPFILE}"

for f in "${SRCDIR}"/config/*.tmpl; do
  DEST="${SRCDIR}"/$(sed -n 's/.* LOCATION:\(.*\)/\1/p' < "$f" | head -n 1)
  sed -f "${TMPFILE}" < "$f" > "$f".new
  if [ -f "${DEST}" ]; then
    cp "${DEST}" "$f".bck
    if [ -f "$f".gen ]; then
      if ! diff -u "$f".{gen,new} | patch --posix -s --merge -o - "$f".bck > "${DEST}"; then
        cp "$f".new "${DEST}"
        echo >&2 "Couldn't merge user changes for ${DEST}, the newly generated file will be used. There's a backup of the previous file in $f.bck"
      fi
    else
      echo "${DEST} overwriten, a backup with user changes can be found in $f.bck"
      cp "$f".new "${DEST}"
    fi
  else
    cp "$f".new "${DEST}"
  fi
  mv "$f".{new,gen}
done
rm "${TMPFILE}"

