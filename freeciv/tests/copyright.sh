#!/bin/sh

files=`find $1 -name "*.c" -o -name "*.h" \
       | sort \
       | grep -v intl \
       | grep -v "Freeciv.h" \
       | fgrep -v "_gen." \
       | grep -v "config.h" \
       | grep -v "config.mac.h" \
       | grep -v gtkpixcomm \
       | grep -v mmx.h \
       | grep -v SDL_ttf \
       | grep -v xaw/canvas \
       | grep -v pixcomm \
       | grep -v dependencies \
       | grep -v utility/md5\.. \
       | grep -v client/gui-sdl/SDL_rotozoom\.. \
       | grep -v client/gui-sdl/alphablit.c `

echo "# No Freeciv Copyright:"
echo "# Excludes: generated files, various 3rd party sources"
for file in $files; do
#    echo "testing $file..."
    grep "Freeciv - Copyright" $file >/dev/null || echo $file
done
echo

echo "# No or other GPL:"
for file in $files; do
    grep "GNU General Public License" $file >/dev/null || echo $file
done
echo
