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
var webgl_models_xml_strings = {};
var webgl_models = {};
var start_preload = 0;
var total_model_count = 0;
var load_count = 0;

/****************************************************************************
  Preload textures and models
****************************************************************************/
function webgl_preload()
{
  $.blockUI({ message: "<h2>Downloading textures and models...</h2>" });
  start_preload = new Date().getTime();
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

  /* Preload tree sprite. */
  var tree_sprite = new THREE.Texture();
  webgl_textures["tree_1"] = tree_sprite;
  textureLoader.load( '/textures/tree_1.png', function ( image ) {
      tree_sprite.image = image;
      tree_sprite.needsUpdate = true;
  } );

  var jungle_sprite = new THREE.Texture();
  webgl_textures["jungle_1"] = jungle_sprite;
  textureLoader.load( '/textures/jungle_1.png', function ( image ) {
      jungle_sprite.image = image;
      jungle_sprite.needsUpdate = true;
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
  var url = '/3d-models/models.zip';
  zip.createReader(new zip.HttpReader(url), function(zipReader){
     $.blockUI({ message: "<h2>Uncompressing files and parsing 3D models...</h2>" });
     zipReader.getEntries(function(entries){
       total_model_count = entries.length;
       for (var i = 0; i < entries.length; i++) {
         var file = entries[i];
         handle_zip_file(file['filename'].replace(".dae", ""), file);
       }
     });
  }, function(message){
    swal("Unable to download models. Please reload the page and try again.");
    console.error("Problem downloading models.zip. Error message: " + message);
  });

}

/****************************************************************************
  Handle each unzipped file, call load_model on each file.
****************************************************************************/
function handle_zip_file(filename, file)
{
  file.getData(new zip.TextWriter(), function(text) {
    load_count++;

    webgl_models_xml_strings[filename] = text;
    if (load_count == total_model_count) {
      webgl_preload_complete();
      console.log("WebGL preloading took: " + (new Date().getTime() - start_preload) + " ms.");
    }
  });

}

/****************************************************************************
 Loads a single model file.
****************************************************************************/
function webgl_get_model(filename)
{
  if (webgl_models[filename] != null) return webgl_models[filename].clone();;
  if (webgl_models_xml_strings[filename] == null) {
    return null;
  }

  var colladaloader = new THREE.ColladaLoader();
  colladaloader.options.convertUpAxis = true;
  colladaloader.parse( webgl_models_xml_strings[filename], function ( collada ) {
    dae = collada.scene;
    dae.updateMatrix();
    dae.scale.x = dae.scale.y = dae.scale.z = 11;
    webgl_models[filename] = dae;

    webgl_models_xml_strings[filename] = ""; // clear xml string for this model.

    return dae.clone();
  });

}

/****************************************************************************
 Create flag meshes
****************************************************************************/
function create_flags()
{
  for (var sprite_key in sprites) {
    if (sprite_key.substring(0,2) == "f.") {
      var flagGeometry = new THREE.PlaneGeometry( 14, 10 );
      /* resize flag to 32x16, since that is required by Three.js*/
      var fcanvas = document.createElement("canvas");
      fcanvas.width = 32;
      fcanvas.height = 16;
      var fcontext = fcanvas.getContext("2d");
      fcontext.drawImage(sprites[sprite_key], 0, 0,
                sprites[sprite_key].width, sprites[sprite_key].height,
                0,0,32,16);

      var texture = new THREE.CanvasTexture(fcanvas);
      var material = new THREE.MeshBasicMaterial( { map: texture, overdraw: 0.5 } );
      meshes[sprite_key] = new THREE.Mesh(flagGeometry, material);
    }
  }

}