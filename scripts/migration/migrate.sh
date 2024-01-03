#!/usr/bin/env bash

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

# Runs new migration scripts.
# migrate.sh [migration_dir]
#
# Every file in migration_dir starting with a number is run in lexicographical
# order, except the ones lower than or equal to the saved checkpoint.

set -e

if [ -z "$1" ]; then
  BASEDIR=${BASH_SOURCE%/*}
else
  BASEDIR=$1
fi

if [ -z "${BASEDIR}" ]; then
  echo >&2 "Please pass the directory with the migration scripts"
  exit 1
fi

if [ "${BASEDIR:0:1}" = - ]; then
  BASEDIR=./"${BASEDIR}"
fi

cd "${BASEDIR}"

if [ -f checkpoint ]; then
  read LAST < checkpoint
fi

for f in [0-9]*; do
  if [ "$f" \> "${LAST}" ]; then
    echo "Running $f"
    ./"$f"
    echo "$f" > checkpoint
  fi
done

echo "Migration finished"
