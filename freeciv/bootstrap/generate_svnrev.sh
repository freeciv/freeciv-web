#!/bin/sh

# Freeciv - Copyright (C) 2007 - The Freeciv Project

# This script generates fc_svnrev_gen.h from fc_svnrev_gen.h.in.
# See fc_svnrev_gen.h.in for details.

# Parameters - $1 - top srcdir
#              $2 - top builddir
#

# Absolete paths
SRCROOT=$(cd "$1" ; pwd)
INPUTDIR=$(cd "$1/bootstrap" ; pwd)
OUTPUTDIR=$(cd "$2/common" ; pwd)

REVSTATE="OFF"
REV="dist"

(cd "$INPUTDIR"
 # Check that all commands required by this script are available
 # If not, we will not claim to know which svn revision this is
 # (REVSTATE will be OFF)
 if which svn && which tail && which wc ; then
   REVTMP="r$(LANG=C svn info 2>/dev/null | grep "^Revision: " | sed 's/^Revision: //')"
   if test "$REVTMP" != "r" ; then
     # This is svn checkout. Check for local modifications
     if test $(cd "$SRCROOT" ; svn diff | wc -l) -eq 0 ; then
       REVSTATE=ON
       REV="$REVTMP"
     else
       REVSTATE=MOD
       REV="modified $REVTMP"
     fi
   fi
 fi

 sed -e "s,<SVNREV>,$REV," -e "s,<SVNREVSTATE>,$REVSTATE," fc_svnrev_gen.h.in > "$OUTPUTDIR/fc_svnrev_gen.h.tmp"
 if ! test -f "$OUTPUTDIR/fc_svnrev_gen.h" ||
    ! cmp "$OUTPUTDIR/fc_svnrev_gen.h" "$OUTPUTDIR/fc_svnrev_gen.h.tmp"
 then
   mv "$OUTPUTDIR/fc_svnrev_gen.h.tmp" "$OUTPUTDIR/fc_svnrev_gen.h"
 fi
 rm -f "$OUTPUTDIR/fc_svnrev_gen.h.tmp"
) > /dev/null
