#!/bin/sh

echo "# C files which use va_list but don't include stdarg.h:"
find $1 -name "*.c" \
 | sort \
 | grep -v intl \
 | while read line; do 
    (grep "va_list" $line >/dev/null \
     && ! grep "^#include.*<stdarg.h>" $line >/dev/null \
     && echo "$line")
done
echo
