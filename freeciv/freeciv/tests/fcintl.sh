#!/bin/sh

files=`find $1 -name "*.c" -o -name "*.h" | sort`

# helper variables
RES_ERR1="" # header files including fcintl.h
RES_ERR2="" # fc_config.h for fcintl.h
RES_ERR3="" # header files with fc_config.h

# check all files
for file in $files; do
  #echo "check $file"
  HAS_FCINTL=0
  HAS_CONFIG=0
  IS_HEADER=0
  grep "#include \"fcintl.h\"" $file > /dev/null && HAS_FCINTL=1
  grep "fc_config.h" $file > /dev/null && HAS_CONFIG=1
  echo $file | grep "\.h" > /dev/null && IS_HEADER=1
  if [ $HAS_FCINTL = 1 -a $IS_HEADER = 1 ]; then
    RES_ERR1="$RES_ERR1 $file"
  fi
  if [ $HAS_FCINTL = 1 -a $HAS_CONFIG = 0 ]; then
    RES_ERR2="$RES_ERR2 $file"
  fi
  if [ $IS_HEADER = 1 -a $HAS_CONFIG = 1 ]; then
    RES_ERR3="$RES_ERR3 $file"
  fi
done

# print results
echo "# Check for header files including fc_config.h"
if [ "x$RES_ERR3" = "x" ]; then
  echo "(no match)"
else
  for II in $RES_ERR3; do
    echo "  $II"
  done
fi
echo

echo "# Check for header files including fcintl.h"
if [ "x$RES_ERR1" = "x" ]; then
  echo "(no match)"
else
  for II in $RES_ERR1; do
    echo "  $II"
  done
fi
echo

echo "# Check if fc_config.h is include when fcintl.h is used:"
if [ "x$RES_ERR2" = "x" ]; then
  echo "(no match)"
else
  for II in $RES_ERR2; do
    echo "  $II"
  done
fi
echo
