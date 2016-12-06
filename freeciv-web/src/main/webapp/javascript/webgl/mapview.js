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

var container, stats;
var scene, maprenderer;

var plane, cube;
var mouse, raycaster, isShiftDown = false;

var rollOverMesh, rollOverMaterial;
var cubeGeo, cubeMaterial;
var directionalLight;

var objects = [];
var vertShader;
var fragShader;

var tiletype_terrains = ["lake","coast","floor","arctic","desert","forest","grassland","hills","jungle","mountains","plains","swamp","tundra", "beach"];

var unknownTerritoryGeometry;

var start_webgl;

var mapview_model_width = 3000;
var mapview_model_height = 2000;
var xquality;
var yquality;


/****************************************************************************
  Start the Freeciv-web WebGL renderer
****************************************************************************/
function webgl_start_renderer()
{
  var new_mapview_width = $(window).width() - width_offset;
  var new_mapview_height = $(window).height() - height_offset;

  container = document.getElementById('canvas_div');
  camera = new THREE.PerspectiveCamera( 45, new_mapview_width / new_mapview_height, 1, 10000 );
  scene = new THREE.Scene();

  raycaster = new THREE.Raycaster();
  mouse = new THREE.Vector2();

  // Lights
  var ambientLight = new THREE.AmbientLight( 0x606060, 1.0 );
  scene.add(ambientLight);

  directionalLight = new THREE.DirectionalLight( 0xffffff, 2.5 );
  directionalLight.position.set( 1, 0.75, 0.5 ).normalize();
  scene.add( directionalLight );

  if (Detector.webgl) {
    var enable_antialiasing = true;
    var stored_antialiasing_setting = simpleStorage.get("antialiasing_setting", "");
    if (stored_antialiasing_setting != null && stored_antialiasing_setting == "false") {
      enable_antialiasing = false;
    }

    maprenderer = new THREE.WebGLRenderer( { antialias: enable_antialiasing } );
  } else {
    maprenderer = new THREE.CanvasRenderer();
  }
  maprenderer.setClearColor(0x000000);
  maprenderer.setPixelRatio(window.devicePixelRatio );
  maprenderer.setSize(new_mapview_width, new_mapview_height);
  container.appendChild(maprenderer.domElement);

  if (location.hostname === "localhost" && Detector.webgl) {
    stats = new Stats();
    container.appendChild( stats.dom );
    console.log("MAX_FRAGMENT_UNIFORM_VECTORS:" + maprenderer.context.getParameter(maprenderer.context.MAX_FRAGMENT_UNIFORM_VECTORS));
  }

  init_webgl_mapctrl();
  animate();

}


/****************************************************************************
 This will render the map terrain mesh.
****************************************************************************/
function render_map_terrain() {
  start_webgl = new Date().getTime();
  var quality = 32, step = 1024 / quality;

  mapview_model_width = Math.floor(mapview_model_width * map['xsize'] / 84);
  mapview_model_height = Math.floor(mapview_model_height * map['ysize'] / 56);

  /* Create water mesh with a texture. */
  var waterGeometry = new THREE.PlaneGeometry( mapview_model_width, mapview_model_height, 2, 2);
  waterGeometry.rotateX( - Math.PI / 2 );
  waterGeometry.translate(Math.floor(mapview_model_height / 2), 0, Math.floor(mapview_model_height / 2));

  for ( var i = 0, l = waterGeometry.vertices.length; i < l; i ++ ) {
      var x = i % quality, y = Math.floor( i / quality );
      waterGeometry.vertices[ i ].y = 50;
  }

  var waterMaterial = new THREE.MeshBasicMaterial( { map: webgl_textures["water_overlay"], overdraw: 0.5, transparent: true, opacity: 0.65, color: 0x5555ff } );
  var waterMesh = new THREE.Mesh( waterGeometry, waterMaterial );
  scene.add(waterMesh);

  /* map_texture: create a texture which contains one pixel for each map tile, where the color of each pixel
    indicates which Freeciv tile type the pixel is. */
  for (var terrain_id in terrains) {
    tiletype_palette.push([terrain_id * 10, 0, 0]);
  }
  bmp_lib.render('map_tiletype_grid',
                    generate_map_tiletype_grid(),
                    tiletype_palette);
  var map_texture = new THREE.Texture();
  map_texture.image = document.getElementById("map_tiletype_grid");
  map_texture.needsUpdate = true;

  /* heightmap image */
  create_heightmap();

  /* uniforms are variables which are used in the fragment shader fragment.js */
  var uniforms = {
    maptiles: { type: "t", value: map_texture },
    mapWidth: { type: "i", value: map['xsize'] },
    mapHeight: { type: "i", value: map['ysize'] }
  };

  /* create a texture for each map tile type. */
  for (var i = 0; i < tiletype_terrains.length; i++) {
    uniforms[tiletype_terrains[i]] = {type: "t", value: webgl_textures[tiletype_terrains[i]]};
  }

  var terrain_material;
  if (Detector.webgl) {
    /* create a WebGL shader for terrain. */
    terrain_material = new THREE.ShaderMaterial({
      uniforms: uniforms,
      vertexShader: vertShader,
      fragmentShader: fragShader
    });
  } else {
    terrain_material = new THREE.MeshLambertMaterial( { color: 0x00ff00} );
  }

  xquality = map.xsize * 4 + 1;
  yquality = map.ysize * 4 + 1;
  var step = 1024 / quality;

  /* LandGeometry is a plane representing the landscape of the map. */
  var landGeometry = new THREE.PlaneGeometry(mapview_model_width, mapview_model_height, xquality - 1, yquality - 1);
  landGeometry.rotateX( - Math.PI / 2 );
  landGeometry.translate(Math.floor(mapview_model_height / 2), 0, Math.floor(mapview_model_height / 2));
  for ( var i = 0, l = landGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    if (heightmap[x] != null && heightmap[x][y] != null) {
      landGeometry.vertices[ i ].y = heightmap[x][y] * 100;
    }
    if (x == xquality - 1 && landGeometry.vertices[ i ].y >= 54) landGeometry.vertices[ i ].y = 54;
    if (y == yquality - 1 && landGeometry.vertices[ i ].y >= 54) landGeometry.vertices[ i ].y = 54;
  }
  landGeometry.computeVertexNormals();
  landGeometry.computeMorphNormals();
  landMesh = new THREE.Mesh( landGeometry, terrain_material );
  scene.add( landMesh );

  prerender(landGeometry, xquality);
  add_all_objects_to_scene();

  /* Unknown territory */
  unknownTerritoryGeometry = new THREE.PlaneGeometry(mapview_model_width, mapview_model_height, xquality - 1, yquality - 1);
  unknownTerritoryGeometry.rotateX( - Math.PI / 2 );
  unknownTerritoryGeometry.translate(Math.floor(mapview_model_height / 2), 0, Math.floor(mapview_model_height / 2));
  for ( var i = 0, l = unknownTerritoryGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    var gx = Math.floor(x / 4);
    var gy = Math.floor(y / 4);
    if (heightmap[x] != null && heightmap[x][y] != null) {
      var ptile = map_pos_to_tile(gx, gy);
      if (ptile != null && tile_get_known(ptile) != TILE_UNKNOWN) {
        unknownTerritoryGeometry.vertices[ i ].y = 0;
      } else {
        unknownTerritoryGeometry.vertices[ i ].y = heightmap[x][y] * 100 + 20;
      }
    }

    if (x == xquality - 1) unknownTerritoryGeometry.vertices[ i ].x += 50;
    if (y == yquality - 1) unknownTerritoryGeometry.vertices[ i ].z += 50;
  }
  unknownTerritoryGeometry.computeVertexNormals();
  unknownTerritoryGeometry.computeMorphNormals();
  var unknownMaterial = new THREE.MeshBasicMaterial( {color: 0x000000 } );
  var unknownMesh = new THREE.Mesh( unknownTerritoryGeometry, unknownMaterial );
  unknownMesh.geometry.dynamic = true;
  scene.add( unknownMesh );

  scene.add(meshes['trees']);
  $.unblockUI();
  console.log("WebGL render_testmap took: " + (new Date().getTime() - start_webgl) + " ms.");
}

/****************************************************************************
  Main animation method
****************************************************************************/
function animate() {
  if (stats != null) stats.begin();
  if (mapview_slide['active']) update_map_slide_3d();
  maprenderer.render( scene, camera );
  check_request_goto_path();
  if (stats != null) stats.end();
  requestAnimationFrame( animate );

}