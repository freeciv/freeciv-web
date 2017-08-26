Blender 3d models created for Freeciv-web.
==========================================

![Freeciv-web](https://raw.githubusercontent.com/freeciv/freeciv-web/develop/freeciv-web/src/main/webapp/javascript/webgl/freeciv-webgl.png "Freeciv-web WebGL screenshot")

These are the original Blender files for all 3D models in the WebGL version. 

3D artists are welcome to improve these models!

Export from Blender to glTF 2.0 .glb binary format
============================================================

Follow these steps to prepare the 3D models for usage in Freeciv-web:

1. Install and activate the Blender glTF 2.0 exporter 
https://github.com/KhronosGroup/glTF-Blender-Exporter

2. Export the blender file from Blender: File -> Export -> glTF 2.0 (.glb)
  - Don't export normals.
  - Export to the .glb file to freeciv-web/src/main/webapp/gltf/
  - Filename must match unit name from Freeciv ruleset.

3. Rebuild Freeciv-web using freeciv-web/build.sh script.

When creating new additional 3D-models they must also be added to preload.js in Freeciv-web so that they are preloaded correctly.

Be sure to limit the file size and number of verticies, since the result
will be rendered in a web browser. Check the file size of the generated .glb file! 

Don't export any lights or cameras.

3D Models with textures
=======================

See the citywalls.blend for an example of a 3D model with textures which works in Freeciv-web 3D. 


The unit models come from here: http://opengameart.org/content/blender-models-for-freeciv-units

