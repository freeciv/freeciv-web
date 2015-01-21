#!/bin/bash
echo "running Freeciv-img-extract..."
python3.4 img-extract.py &&
pngcrush pre-freeciv-web-tileset-amplio2-0.png freeciv-web-tileset-amplio2-0.png &&
pngcrush pre-freeciv-web-tileset-amplio2-1.png freeciv-web-tileset-amplio2-1.png &&
pngcrush pre-freeciv-web-tileset-amplio2-2.png freeciv-web-tileset-amplio2-2.png &&
pngcrush pre-freeciv-web-tileset-amplio2-3.png freeciv-web-tileset-amplio2-3.png &&
pngcrush pre-freeciv-web-tileset-trident-0.png freeciv-web-tileset-trident-0.png &&
mkdir -p ../../freeciv-web/src/main/webapp/tileset &&
cp freeciv-web-tileset-*.png ../../freeciv-web/src/main/webapp/tileset/ &&
cp web_tileset_*.js ../../freeciv-web/src/main/webapp/javascript/ && 
echo "converting flag svg files to png..." && 
mkdir -p ../../freeciv-web/src/main/webapp/images/flags/ &&
(for X in `find ../../freeciv/freeciv/data/flags/*.svg` 
do
 convert -density 80 -resize 180 $X ${X/.svg/-web.png}
done) &&
mv ../../freeciv/freeciv/data/flags/*-web.png ../../freeciv-web/src/main/webapp/images/flags/ &&	
echo "Freeciv-img-extract done." || echo "Freeciv-img-extract failed!"
