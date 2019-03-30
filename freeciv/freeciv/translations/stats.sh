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

dir_stats() {
declare -i TRANS
declare -i FUZZY
declare -i UNTRANS
declare -i SUM
declare -i PRCT

for POFILE in $@
do
  if test "x$POFILE" != "x" ; then
    CODE=$(echo "$POFILE" | sed -e 's/.po//' -e 's,.*/,,' -e 's,.*\\,,')
    FSTR="$(LANG=C msgfmt --stat "$POFILE" 2>&1)"
    if echo $FSTR | grep translated >/dev/null ; then
      TRANS=$(echo $FSTR | sed 's/ translated.*//')
      FSTR="$(echo $FSTR | sed 's/.* translated messages*[.,]//')"
    else
      TRANS=0
    fi
    if echo $FSTR | grep fuzzy >/dev/null ; then
      FUZZY=$(echo $FSTR | sed 's/ fuzzy.*//')
      FSTR="$(echo $FSTR | sed 's/.* fuzzy translations*[.,]//')"
    else
      FUZZY=0
    fi
    if echo $FSTR | grep untranslated >/dev/null ; then
      UNTRANS=$(echo $FSTR | sed 's/ untranslated.*//')
    else
      UNTRANS=0
    fi

    SUM=$TRANS+$FUZZY+$UNTRANS
    PRCT=$TRANS*100/$SUM

    echo "$CODE $PRCT%"
  fi
done
}

if test "x$1" = "x-h" || test "x$1" = "x--help" ; then
    echo "Usage: $(basename $0) [domain=all]"
    exit
fi

if test "x$1" != "x" && test "x$1" != "xall" ; then
    DOMAINLIST="$1"
else
    DOMAINLIST="core nations ruledit"
fi

for domain in $DOMAINLIST
do
  if test "x$1" = "x" ; then
    echo
    echo "$domain"
    echo "----------"
  fi

  dir_stats "$SRCDIR/$domain/*.po"
  rm messages.mo
done
