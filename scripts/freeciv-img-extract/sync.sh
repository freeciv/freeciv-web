#!/bin/bash
TILESET_DEST=../../freeciv-web/src/main/webapp/tileset
echo "running Freeciv-img-extract..."
python3 img-extract.py &&
mkdir -p "${TILESET_DEST}" &&
pngcrush -d "${TILESET_DEST}" freeciv-web-tileset*.png &&
echo "converting tileset .png files to .webp ..." && 
(for X in "${TILESET_DEST}"/*.png
do
 cwebp -quiet -lossless $X -o ${X/.png/.webp}
done) &&
cp tileset_spec_*.js ../../freeciv-web/src/main/webapp/javascript/2dcanvas/ && 
echo "converting flag svg files to png and webp..." && 
mkdir -p ../../freeciv-web/src/main/webapp/images/flags/ &&
(for X in `find ../../freeciv/freeciv/data/flags/*.svg` 
do
 convert -density 80 -resize 180 $X ${X/.svg/-web.png}
done) &&
(for X in `find ../../freeciv/freeciv/data/flags/*.png` 
do
 cwebp -quiet -lossless $X -o ${X/.png/.webp}
done) &&	
mv -f ../../freeciv/freeciv/data/flags/*-web.png ../../freeciv-web/src/main/webapp/images/flags/ &&	
mv -f ../../freeciv/freeciv/data/flags/*-web.webp ../../freeciv-web/src/main/webapp/images/flags/ &&	
echo "Freeciv-img-extract done." || (>&2 echo "Freeciv-img-extract failed!" && exit 1)
