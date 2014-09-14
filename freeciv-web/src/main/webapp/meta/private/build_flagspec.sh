#!/bin/sh

if test "x$1" = "x" || test "x$1" = "x-h" || test "x$1" = "x--help" ; then
  echo "Usage: $0 <flags spec file>"
  exit
fi

if ! test -f "$1" ; then
  echo "Flag spec \"$1\" not found" >&2
  exit 1
fi

(
echo "<?php";
echo "\$flags = array(";

cat $1 | grep "\".*\".*,.*\".*\"" | grep -v ";.*\"" | grep -v "\"tag\"" | sed 's/;.*//' | sed -e 's/,/ => /' -e "s/\"[:space:\t ]*\$/-web.png\",/" -e "s/\"/'/g" -e "s,flags/,,"

echo "'unknown' => 'unknown-web.png');";
echo "?>";
) > ../php_code/flagspecs.php
