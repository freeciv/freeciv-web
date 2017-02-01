#!/bin/bash

echo "Converting Wavefront .obj files to Three.js .js files."

find . -depth -name "*.obj" -exec sh -c 'python3 convert_obj_three_for_python3.py -i "$1"  -o "${1%.obj}.js" -t binary ' _ {} \;
mv *.js ../freeciv-web/src/main/webapp/3d-models/
mv *.bin ../freeciv-web/src/main/webapp/3d-models/
cp ../freeciv-web/src/main/webapp/3d-models/*.* /var/lib/tomcat8/webapps/ROOT/3d-models/
