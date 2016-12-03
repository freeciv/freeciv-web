3D models for Freeciv-web
-------------------------

These files are 3d models of units from the Cimpletoons graphics set included in Freeciv. They have been exported to Collada format using Blender. The files are in public domain. 

The blender models can be found in the Freeciv-web source repository in the /blender directory.

The models must not contain lamps or light sources. I have not been able to export Blender models with armatures to the Collada 
format and import them into Three.js.

All the collada .dae files in this directory are compressed to a zip-file called models.zip by the build.sh script.
The file models.zip is downloaded by the browser and uncompressed, so that bandwidth usage is limited as much as possible.
So to update models, you have to run the build.sh script.


