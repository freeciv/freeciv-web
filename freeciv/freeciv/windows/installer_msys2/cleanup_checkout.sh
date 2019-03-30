#!/bin/sh

if test "x$VERSION_REVTYPE" = "xgit" ; then
    git checkout $1/translations
fi
