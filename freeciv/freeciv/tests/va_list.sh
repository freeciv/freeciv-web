#!/bin/sh

echo "# C or C++ files which use va_list but don't include stdarg.h:"
find $1 -name "*.c" -o -name "*.cpp" \
 | sort \
 | while read line; do 
    (grep "va_list" $line >/dev/null \
     && ! grep "^#include.*<stdarg.h>" $line >/dev/null \
     && echo "$line")
done
echo
