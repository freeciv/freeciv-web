#!/bin/bash
# shell script run from Cron every 2 minute, to convert and move images.

cd /mnt/savegames
find . -maxdepth 1 -name "*.ppm" -exec convert {} {}.png \; 

count=`ls -1 *.ppm 2>/dev/null | wc -l`

if [ $count != 0 ] 
then
  rm *.ppm
  #update this to the directory for Resin in production.
  mv *.png /home/andreas/freeciv-build/resin-4.0.15/webapps/ROOT/freecivmetaserve/
fi

