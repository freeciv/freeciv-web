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
# When configfile is not given, script_src_dir/../configuration.sh is used.

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
  CONFIG="${CONFIG}"/../configuration.sh
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

for f in "${SRCDIR}"/{freeciv-proxy,pbem}/settings.ini "${SRCDIR}"/freeciv-web/flyway.properties "${SRCDIR}"/freeciv-web/src/main/webapp/META-INF/context.xml "${SRCDIR}"/freeciv-web/src/main/webapp/WEB-INF/config.properties; do
  sed -f "${TMPFILE}" < "$f".dist > "$f"
done
rm "${TMPFILE}"

