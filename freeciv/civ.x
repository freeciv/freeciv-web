#!/bin/sh
#/********************************************************************** 
# Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
# script by Rene Schalburg
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

BUILDDIR=`dirname $0`
PROGNAME=freeciv
EXENAME=${PROGNAME}

gui=`echo "$1 $2" | awk '$1 ~ /^--gui$/ { print $2 }'`
if test "x$gui" != "x" ; then
  EXENAME=${PROGNAME}-${gui}
  shift
  shift
fi

if test -x "$BUILDDIR/client/$EXENAME" ; then
  EXE=$BUILDDIR/client/$EXENAME
elif test -x "$BUILDDIR/$EXENAME" ; then
  EXE=$BUILDDIR/$EXENAME
else
  EXE=`ls -1 "$BUILDDIR/${PROGNAME}-"* "$BUILDDIR/client/${PROGNAME}-"* 2> /dev/null |\
  while read program ; do
    if test -x "$program" ; then
      echo $program
      break
    fi
  done`
fi

if test "x$EXE" = "x"; then
  echo $0: Unable to find client executable: $EXENAME
  exit 1
fi

if test "x$FREECIV_PATH" = "x" ; then
  FREECIV_PATH=".:data:~/.freeciv:~/.freeciv/2.3"
fi
export FREECIV_PATH="${FREECIV_PATH}:/home/matthias/data/git/freeciv.git"

if test "x$FREECIV_DATA_PATH" = "x" ; then
  FREECIV_DATA_PATH=".:data:~/.freeciv/2.3"
fi
export FREECIV_DATA_PATH="${FREECIV_DATA_PATH}:/home/matthias/data/git/freeciv.git/data:/home/matthias/data/git/freeciv.git/data"

if test "x$FREECIV_SAVE_PATH" = "x" ; then
  FREECIV_SAVE_PATH=".:~/.freeciv/saves"
fi
export FREECIV_SAVE_PATH="${FREECIV_SAVE_PATH}:/home/matthias/data/git/freeciv.git:/home/matthias/data/git/freeciv.git"

if test "x$FREECIV_SCENARIO_PATH" = "x" ; then
  FREECIV_SCENARIO_PATH=".:data/scenario:~/.freeciv/scenarios"
fi
export FREECIV_SCENARIO_PATH="${FREECIV_SCENARIO_PATH}:/home/matthias/data/git/freeciv.git/data/scenario:/home/matthias/data/git/freeciv.git/data/scenario"

echo "Running $EXE"
exec "$EXE" ${1+"$@"}
