#!/bin/sh

files=`find $1 -name "*.c" -o -name "*.h" -o -name "*.cpp" \
       | sort \
       | grep -v "Freeciv.h" \
       | fgrep -v "_gen." \
       | grep -v "fc_config.h" \
       | grep -v gtkpixcomm \
       | grep -v pixcomm \
       | grep -v dependencies \
       | grep -v utility/md5\.. `

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
