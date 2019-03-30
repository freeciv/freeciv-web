#!/bin/sh

files=`find $1 -name "*.c" -o -name "*.cpp" -o -name "*.h" | sort`

echo "# check for __LINE__ (should be replaced by __FC_LINE__):"
for file in $files; do
  COUNT=`cat $file | grep _LINE__ | grep -v __FC_LINE__ | wc -l`
  if [ "x$COUNT" != "x0" ]; then
    echo $file
  fi
done
