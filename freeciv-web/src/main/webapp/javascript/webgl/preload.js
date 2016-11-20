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

var load_count = 0;
var model_files = ["Settlers", "Workers", "Explorer", "city_1"];
var total_model_count = model_files.length;

/****************************************************************************
  Preload textures and models
****************************************************************************/
function webgl_preload()
{
  $.blockUI({ message: "<h2>Downloading textures and models...</h2>" });

  create_flags();

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
  Preload all models.
****************************************************************************/
function webgl_preload_models()
{

  for (var i = 0; i < total_model_count; i++) {
    load_model(model_files[i]);
  }

}

/****************************************************************************
 Loads a single model file.
****************************************************************************/
function load_model(filename)
{
   var colladaloader = new THREE.ColladaLoader();
    colladaloader.options.convertUpAxis = true;
    colladaloader.load( '/3d-models/' + filename + '.dae', function ( collada ) {
        dae = collada.scene;
        dae.updateMatrix();
        dae.scale.x = dae.scale.y = dae.scale.z = 11;
        webgl_models[filename] = dae;
        load_count++;
        if (load_count == total_model_count) webgl_preload_complete();
    });

}

/****************************************************************************
 Create flag meshes
****************************************************************************/
function create_flags()
{
  var flagGeometry = new THREE.PlaneGeometry( 14, 8 );
  // FIXME: Don't hardcode to one nation flag.
  var texture = new THREE.CanvasTexture(sprites['f.norway']);
  var material = new THREE.MeshBasicMaterial( { map: texture, overdraw: 0.5 } );
  meshes['flag'] = new THREE.Mesh(flagGeometry, material);

}