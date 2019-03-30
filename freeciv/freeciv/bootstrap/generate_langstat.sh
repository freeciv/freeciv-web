#!/bin/bash
#/**********************************************************************
# Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#***********************************************************************/

if test "x$1" = "x-h" || test "x$1" = "x--help" || test "x$1" = "x" ; then
   echo "Usage: $(basename $0) <translation domain> <freeciv source root> <freeciv build root>"
   exit
fi

"$2/translations/stats.sh" $1 | (
    while read CODE PRCT ; do
        NLANG=$(grep "^$CODE " "$2/bootstrap/langnames.txt" 2>/dev/null | sed "s/$CODE //")
        echo "$CODE $PRCT $NLANG"
    done ) > "$3/bootstrap/langstat_${1}.txt.tmp"

if ! test -f "$2/bootstrap/langstat_${1}.txt" ||
   ! cmp "$2/bootstrap/langstat_${1}.txt" "$3/bootstrap/langstat_${1}.txt.tmp" ; then
    mv "$3/bootstrap/langstat_${1}.txt.tmp" "$2/bootstrap/langstat_${1}.txt"
else
    rm -f "$3/bootstrap/langstat_${1}.txt.tmp"
fi
