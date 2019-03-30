#!/bin/sh

if ! test -f ../../bootstrap/config.guess ; then
    echo "win"
else
  case $(../../bootstrap/config.guess 2>/dev/null) in
      *mingw32*) echo "win32" ;;
      *mingw64*) echo "win64" ;;
      *) echo "win" ;;
  esac
fi
