/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2016  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

var webgl_textures = {};
var webgl_models = {};

/****************************************************************************
  Preload textures and models
****************************************************************************/
function webgl_preload()
{
  var loadingManager = new THREE.LoadingManager();
  loadingManager.onLoad = function () {
    webgl_preload_models();

  };

  var textureLoader = new THREE.ImageLoader( loadingManager );

  /* Preload water overlay texture. */
  var water_texture = new THREE.Texture();
  webgl_textures["water_overlay"] = water_texture;
  textureLoader.load( '/textures/water_overlay_texture.png', function ( image ) {
      water_texture.image = image;
      water_texture.wrapS = THREE.RepeatWrapping;
      water_texture.wrapT = THREE.RepeatWrapping;
      water_texture.repeat.set( 5, 5);
      water_texture.needsUpdate = true;
  } );

  /* Preload a texture for each map tile type. */
  for (var i = 0; i < tiletype_terrains.length; i++) {
    var imgurl = "/textures/" + tiletype_terrains[i] + ".png";
    textureLoader.load(imgurl, (function (url, index) {
            return function (image, i) {
                webgl_textures[tiletype_terrains[index]] = new THREE.Texture();
                webgl_textures[tiletype_terrains[index]].image = image;
                webgl_textures[tiletype_terrains[index]].wrapS = THREE.RepeatWrapping;
                webgl_textures[tiletype_terrains[index]].wrapT = THREE.RepeatWrapping;
                webgl_textures[tiletype_terrains[index]].needsUpdate = true;
            }
      })(imgurl, i)
    );
  }

}
/****************************************************************************
  Preload models.
****************************************************************************/
function webgl_preload_models()
{
  /* Load a settler model. This is a Collada file which has been exported from Blender. */
  var colladaloader = new THREE.ColladaLoader();
  colladaloader.options.convertUpAxis = true;
  colladaloader.load( '/3d-models/settler3.dae', function ( collada ) {
      dae = collada.scene;
      dae.updateMatrix();
      dae.scale.x = dae.scale.y = dae.scale.z = 11;
      dae.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), 100);
      dae.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), 1000);
      webgl_models["settler"] = dae;
      setTimeout(render_testmap, 500);
  });

}