#!/bin/sh

files=`find $1 -name "*.h" \
       | sort \
       | grep -v utility/spec \
       | grep -v "Freeciv\.h" \
       | grep -v \./common/packets_gen\.h \
       | grep -v tolua_.*_gen\.h \
       | grep -v fc_config\.h \
       | grep -v dependencies \
       | grep -v utility/md5\.h `

echo "# Header files without any include guard:"
for file in $files; do
    grep "^#ifndef FC_.*_H$" $file >/dev/null \
     || grep ifndef $file >/dev/null \
     || echo $file
done
echo

echo "# Header files with a misnamed guard (doesn't match 'FC_.*_H'):"
for file in $files; do
    grep "^#ifndef FC_.*_H$" $file >/dev/null \
     || (grep ifndef $file >/dev/null \
         && grep ifndef $file /dev/null | head -n1)
done
echo
