/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

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
var directionalLight;
var activeUnitLight;
var clock;
var webgl_controls;

var terrainVertShader;
var terrainFragShader;
var darknessVertShader;
var darknessFragShader;

var tiletype_terrains = ["lake","coast","floor","arctic","desert","forest","grassland","hills","jungle","mountains","plains","swamp","tundra", "beach"];

var landGeometry;
var landMesh;
var unknown_terrain_mesh;
var unknownTerritoryGeometry;
var fogOfWarGeometry;
var water;
var high_quality_water_force_enabled = false;

var start_webgl;

var mapview_model_width;
var mapview_model_height;
var xquality;
var yquality;
var MAPVIEW_ASPECT_FACTOR = 35.71;


/****************************************************************************
  Start the Freeciv-web WebGL renderer
****************************************************************************/
function webgl_start_renderer()
{
  var new_mapview_width = $(window).width() - width_offset;
  var new_mapview_height;
  if (!is_small_screen()) {
    new_mapview_height = $(window).height() - height_offset;
  } else {
    new_mapview_height = $(window).height() - height_offset - 40;
  }

  container = document.getElementById('canvas_div');
  camera = new THREE.PerspectiveCamera( 45, new_mapview_width / new_mapview_height, 1, 10000 );
  scene = new THREE.Scene();

  raycaster = new THREE.Raycaster();
  mouse = new THREE.Vector2();

  clock = new THREE.Clock();

  // Lights
  var ambientLight = new THREE.AmbientLight( 0x606060, 1.0 );
  scene.add(ambientLight);

  directionalLight = new THREE.DirectionalLight( 0xffffff, 2.5 );
  directionalLight.position.set( 1, 0.75, 0.5 ).normalize();
  scene.add( directionalLight );

  if (Detector.webgl) {
    var enable_antialiasing = graphics_quality >= QUALITY_MEDIUM;
    var stored_antialiasing_setting = simpleStorage.get("antialiasing_setting", "");
    if (stored_antialiasing_setting != null && stored_antialiasing_setting == "false") {
      enable_antialiasing = false;
    }

    maprenderer = new THREE.WebGLRenderer( { antialias: enable_antialiasing } );
  } else {
    swal("3D WebGL not supported by your browser or you don't have a 3D graphics card. Please go back and try the 2D version instead. ");
    return;
  }

  if (is_small_screen() || $(window).width() <= 1366) {
    camera_dy = 390;
    camera_dx = 180;
    camera_dz = 180;
  }

  if (cardboard_vr_enabled) {
    init_stereo_vr_cardboard(new_mapview_width, new_mapview_height);
  }

  maprenderer.setClearColor(0x000000);
  maprenderer.setPixelRatio(window.devicePixelRatio);
  maprenderer.setSize(new_mapview_width, new_mapview_height);
  container.appendChild(maprenderer.domElement);

  if (location.hostname != "play.freeciv.org" && Detector.webgl) {
    stats = new Stats();
    container.appendChild( stats.dom );
    console.log("MAX_FRAGMENT_UNIFORM_VECTORS:" + maprenderer.context.getParameter(maprenderer.context.MAX_FRAGMENT_UNIFORM_VECTORS));
  }

  animate();

}


/****************************************************************************
 This will render the map terrain mesh.
****************************************************************************/
function init_webgl_mapview() {
  start_webgl = new Date().getTime();
  var quality = 32, step = 1024 / quality;

  selected_unit_material = new THREE.MeshBasicMaterial( { color: 0xf6f7bf, transparent: true, opacity: 0.5} );


  if (graphics_quality > QUALITY_LOW || high_quality_water_force_enabled) {
    // high quality water using WebGL shader
    water = new THREE.Water( maprenderer, camera, scene, {
        textureWidth: 512,
        textureHeight: 512,
        waterNormals: webgl_textures["waternormals"],
        alpha: 	0.7,
        sunDirection: directionalLight.position.clone().normalize(),
        sunColor: 0xfaf100,
        waterColor: 0x003e7b,
        distortionScale: 3.0,
        fog: false
    } );

    mirrorMesh = new THREE.Mesh(
	  new THREE.PlaneBufferGeometry( mapview_model_width, mapview_model_height),
	  water.material
    );

    mirrorMesh.add( water );
	mirrorMesh.rotation.x = - Math.PI * 0.5;
	mirrorMesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), 50);
	mirrorMesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), Math.floor(mapview_model_width / 2) - 500);
	mirrorMesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), -mapview_model_height / 2);
	scene.add( mirrorMesh );

    var sunGeometry = new THREE.PlaneGeometry( 1000, 1000, 2, 2);
    sunGeometry.rotateX( Math.PI / 2 );
    sunGeometry.translate(Math.floor(mapview_model_width / 2) - 500, 800, Math.floor(mapview_model_height / 3));
    var sunMesh = new THREE.Mesh( sunGeometry, webgl_materials['sun'] );
    scene.add(sunMesh);

  } else {
    // lower quality water, create water mesh with a texture.
    var waterMaterial = new THREE.MeshBasicMaterial( { transparent: true, opacity: 0.5, color: 0x173355 } );
    var waterMesh = new THREE.Mesh( new THREE.PlaneBufferGeometry( mapview_model_width, mapview_model_height), waterMaterial );
	waterMesh.rotation.x = - Math.PI * 0.5;
	waterMesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), 50);
	waterMesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), Math.floor(mapview_model_width / 2) - 500);
	waterMesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), -mapview_model_height / 2);
    scene.add(waterMesh);
  }

  /* heightmap image */
  create_heightmap();
  if (graphics_quality > QUALITY_LOW) init_borders_image();
  init_roads_image();

  /* uniforms are variables which are used in the fragment shader fragment.js */
  var freeciv_uniforms = {
    maptiles: { type: "t", value: init_map_tiletype_image() },
    borders: { type: "t", value: (graphics_quality > QUALITY_LOW) ? update_borders_image() : null },
    borders_enabled: { type: "b", value: graphics_quality > QUALITY_LOW },
    map_x_size: { type: "f", value: map['xsize'] },
    map_y_size: { type: "f", value: map['ysize'] },
    terrains: {type: "t", value: webgl_textures["terrains"]},
    roadsmap: { type: "t", value: update_roads_image()},
    roadsprites: {type: "t", value: webgl_textures["roads"]},
    railroadsprites: {type: "t", value: webgl_textures["railroads"]}

  };



  /* create a WebGL shader for terrain. */
  var terrain_material = new THREE.ShaderMaterial({
    uniforms: freeciv_uniforms,
    vertexShader: terrainVertShader,
    fragmentShader: terrainFragShader
  });

  xquality = map.xsize * 4 + 1;
  yquality = map.ysize * 4 + 1;
  var step = 1024 / quality;

  /* LandGeometry is a plane representing the landscape of the map. */
  landGeometry = new THREE.PlaneGeometry(mapview_model_width, mapview_model_height, xquality - 1, yquality - 1);
  landGeometry.rotateX( - Math.PI / 2 );
  landGeometry.translate(Math.floor(mapview_model_width / 2) - 500, 0, Math.floor(mapview_model_height / 2));
  for ( var i = 0, l = landGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    if (heightmap[x] != null && heightmap[x][y] != null) {
      landGeometry.vertices[ i ].y = heightmap[x][y] * 100;
    }
    if (x == xquality - 1 && landGeometry.vertices[ i ].y >= 54) landGeometry.vertices[ i ].y = 54;
    if (y == yquality - 1 && landGeometry.vertices[ i ].y >= 54) landGeometry.vertices[ i ].y = 54;
  }
  landGeometry.computeVertexNormals();
  landMesh = new THREE.Mesh( landGeometry, terrain_material );
  scene.add( landMesh );

  prerender(landGeometry, xquality);

  add_all_objects_to_scene();

  /* Unknown territory */
  var darkness_material;
  if (graphics_quality >= QUALITY_MEDIUM) {
    darkness_material = new THREE.ShaderMaterial({
      vertexShader: darknessVertShader,
      fragmentShader: darknessFragShader,
      uniforms: []
    });
    darkness_material.transparent = true;
  } else {
    darkness_material = new THREE.MeshBasicMaterial( { color: 0x000000} );
  }

  unknownTerritoryGeometry = new THREE.PlaneGeometry(mapview_model_width, mapview_model_height, xquality - 1, yquality - 1);
  unknownTerritoryGeometry.rotateX( - Math.PI / 2 );
  unknownTerritoryGeometry.translate(Math.floor(mapview_model_width / 2) - 496, 0, Math.floor(mapview_model_height / 2) + 4);
  for ( var i = 0, l = unknownTerritoryGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    var gx = Math.floor(x / 4);
    var gy = Math.floor(y / 4);
    if (heightmap[x] != null && heightmap[x][y] != null) {
      var ptile = map_pos_to_tile(gx, gy);
      if (ptile != null && tile_get_known(ptile) != TILE_UNKNOWN) {
        unknownTerritoryGeometry.vertices[ i ].y = 0;
      } else {
        unknownTerritoryGeometry.vertices[ i ].y = heightmap[x][y] * 100 + 13;
      }
    }

    if (x == xquality - 1) unknownTerritoryGeometry.vertices[ i ].x += 50;
    if (y == yquality - 1) unknownTerritoryGeometry.vertices[ i ].z += 50;
  }
  unknownTerritoryGeometry.computeVertexNormals();
  unknown_terrain_mesh = new THREE.Mesh( unknownTerritoryGeometry, darkness_material );
  unknown_terrain_mesh.geometry.dynamic = true;
  if (!observing) scene.add(unknown_terrain_mesh);
  setTimeout(check_remove_unknown_territory, 120000);

  // Fog of war
  fogOfWarGeometry = new THREE.PlaneGeometry(mapview_model_width, mapview_model_height, xquality - 1, yquality - 1);
  fogOfWarGeometry.rotateX( - Math.PI / 2 );
  fogOfWarGeometry.translate(Math.floor(mapview_model_width / 2) - 496, 0, Math.floor(mapview_model_height / 2) + 4);
  for ( var i = 0, l = fogOfWarGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    var gx = Math.floor(x / 4);
    var gy = Math.floor(y / 4);
    if (heightmap[x] != null && heightmap[x][y] != null) {
      var ptile = map_pos_to_tile(gx, gy);
      if (ptile != null && tile_get_known(ptile) == TILE_KNOWN_SEEN) {
        fogOfWarGeometry.vertices[ i ].y = heightmap[x][y] * 100 - 12;
      } else {
        fogOfWarGeometry.vertices[ i ].y = heightmap[x][y] * 100 + 13;
      }
    }
    if (x == xquality - 1) fogOfWarGeometry.vertices[ i ].x += 50;
    if (y == yquality - 1) fogOfWarGeometry.vertices[ i ].z += 50;
  }
  fogOfWarGeometry.computeVertexNormals();
  var fogOfWar_material = new THREE.MeshLambertMaterial({color: 0x000000, transparent: true, opacity: 0.55});
  var fog_of_war_mesh = new THREE.Mesh( fogOfWarGeometry, fogOfWar_material );
  fog_of_war_mesh.geometry.dynamic = true;
  if (!observing) scene.add(fog_of_war_mesh);

  // cleanup
  heightmap = {};
  for (var i = 0; i < tiletype_terrains.length; i++) {
    webgl_textures[tiletype_terrains[i]] = null;
  }
  webgl_textures = {};
  $.unblockUI();
  console.log("init_webgl_mapview took: " + (new Date().getTime() - start_webgl) + " ms.");

  benchmark_start = new Date().getTime();
  setTimeout(initial_benchmark_check, 10000);

}

/****************************************************************************
  Main animation method
****************************************************************************/
function animate() {
  if (scene == null) return;
  if (stats != null) stats.begin();
  if (mapview_slide['active']) update_map_slide_3d();

  if (normalsNeedsUpdating  && unknownTerritoryGeometry != null) {
    unknownTerritoryGeometry.computeFaceNormals();
    unknownTerritoryGeometry.computeVertexNormals();
    normalsNeedsUpdating = false;
  }

  update_animated_objects();

  if ((graphics_quality > QUALITY_LOW || high_quality_water_force_enabled) && water != null && tree_points != null && jungle_points != null) {
    water.material.uniforms.time.value += 1.0 / 60.0;
    tree_points.visible = false;
    jungle_points.visible = false;
    water.render();
    tree_points.visible = true;
    jungle_points.visible = true;
  }

  if (selected_unit_indicator != null && selected_unit_material != null) {
    selected_unit_material.color.multiplyScalar (0.994);
    if (selected_unit_material_counter > 80) {
      selected_unit_material_counter = 0;
      selected_unit_material.color.setHex(0xf6f7bf);
    }
    selected_unit_material_counter++;
  }

  if (!cardboard_vr_enabled) {
    maprenderer.render(scene,camera );
  } else {
    stereoEffect.render(scene, camera);
  }

  if (goto_active) check_request_goto_path();
  if (stats != null) stats.end();
  if (initial_benchmark_enabled || benchmark_enabled) benchmark_frames_count++;

  if (webgl_controls != null && clock != null) webgl_controls.update(clock.getDelta());

  if (renderer == RENDERER_WEBGL) requestAnimationFrame(animate);

}