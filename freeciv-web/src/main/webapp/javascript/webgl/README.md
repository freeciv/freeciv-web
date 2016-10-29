WebGL renderer for Freeciv-web
==============================

This is the WebGL + Three.js renderer for Freeciv-web.

[Three.js](https://threejs.org/) is the 3D engine used in Freeciv-web.

Custom GLSL Fragment and Vertex shaders can be found in the shaders subdirectory. 

![Freeciv-web](https://raw.githubusercontent.com/freeciv/freeciv-web/develop/freeciv-web/src/main/webapp/javascript/webgl/freeciv-webgl.png "Freeciv-web WebGL screenshot")

Building and testing
--------------------
Build Freeciv-web as normal with Vagrant as described in the main README file.

For test and development, use this url which will autostart a game as observer in WebGL mode:
http://localhost/webclient/?action=new&renderer=webgl&autostart=true

