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

SRCDIR="$(dirname "$0")"

if test "x$1" = "x-h" || test "x$1" = "x--help" ; then
    echo "Usage: $(basename $0) [domain=all]"
    exit
fi

DOMAINLIST_FULL="core nations ruledit"

cd $SRCDIR

if test "x$1" != "x" && test "x$1" != "xall" ; then
  DOMAINLIST="$1"
else
  DOMAINLIST="$DOMAINLIST_FULL"
fi

for domain in $DOMAINLIST
do
  cp global.skip $domain/POTFILES.skip
  for other_domain in $DOMAINLIST_FULL
  do
    if test $domain != $other_domain ; then
      cat $other_domain/POTFILES.in >> $domain/POTFILES.skip
    fi
  done
done
