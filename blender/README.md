Blender 3d models created for Freeciv-web.
==========================================

![Freeciv-web](https://raw.githubusercontent.com/freeciv/freeciv-web/develop/freeciv-web/src/main/webapp/javascript/webgl/freeciv-webgl.png "Freeciv-web WebGL screenshot")

These are the original Blender files for all 3D models in the WebGL version. 
The blender files are converted to Wavefront .obj, then to the Three.js binary format
for run-time display in the game.

3D artists are welcome to improve these models!

Export from Blender and conversion to Three.js binary format
============================================================

Follow these steps to prepare the 3d models for usage in Freeciv-web:

1. Export each blender model in Blender using File -> Export -> Wavefront (.obj). Select export to this directory.

2. Run update_process_models.sh which will convert the Wavefront .obj files to the Three.js binary format and copy the result to freeciv-web/src/main/webapp/3d-models.

Filename must match unit name from Freeciv ruleset.

When creating new additional 3D-models they must also be added to preload.js in Freeciv-web so that they are preloaded correctly.

Be sure to limit the file size and number of verticies, since the result
will be rendered in a web browser. Check the file size of the generated .obj file! 

Don't export any lights or cameras.

3D Models with textures
=======================

See the citywalls.blend for an example of a 3D model with textures which works in Freeciv-web 3D. 
The texture file is called brick1.png and is located in the \blender directory when working in Blender and in \freeciv-web\src\main\webapp\3d-models for loading in the webapp.


The unit models come from here: http://opengameart.org/content/blender-models-for-freeciv-units

