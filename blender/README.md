Blender 3d models created for Freeciv-web.
==========================================

3d artists are welcome to improve these models!

These are the source Blender model files.

These should be exported in Collada format to:
freeciv-web/src/main/webapp/3d-models/

Filename must match unit name from Freeciv ruleset.

Be sure to limit the file size and number of verticies, since the result
will be rendered in a web browser.


3D Models with textures
=======================

See the citywalls.blend for an example of a 3D model with textures
which works in Freeciv-web 3D. 

The texture file is called brick1.png and is located in the \blender directory when working in Blender and in \freeciv-web\src\main\webapp\textures for loading in the webapp.

Export to collada format. Make sure the texture filename in the <init_from> tag in the collada .dae file  
does not contain any directories, it should only contain the filename. Don't export any lights or cameras.

