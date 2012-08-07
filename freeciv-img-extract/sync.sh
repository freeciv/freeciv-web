python img-extract.py && \
pngcrush pre-freeciv-web-tileset.png freeciv-web-tileset.png && \
mkdir -p ../freeciv-web/src/main/webapp/tileset &&
cp freeciv-web-tileset.png ../freeciv-web/src/main/webapp/tileset/ && \
cp freeciv-web-tileset.js ../freeciv-web/src/main/webapp/javascript/ 
cp tiles/*.png ../freeciv-web/src/main/webapp/tiles/
