Blender 3d models created for Freeciv-web.
==========================================

3d artists are welcome to improve these models!

These are the source Blender model files. Follow these steps to prepare
the 3d models for usage in Freeciv-web:

1. Export each blender model in Blender using File -> Export -> Wavefront (.obj) to this directory.

2. Run update_process_models.sh which will convert the Wavefront .obj files to the Three.js binary format to freeciv-web/src/main/webapp/3d-models.


Filename must match unit name from Freeciv ruleset.

Be sure to limit the file size and number of verticies, since the result
will be rendered in a web browser. 

Check the file size of the generated .obj file!


3D Models with textures
=======================

See the citywalls.blend for an example of a 3D model with textures which works in Freeciv-web 3D. 
The texture file is called brick1.png and is located in the \blender directory when working in Blender and in \freeciv-web\src\main\webapp\3d-models for loading in the webapp.

Don't export any lights or cameras.
