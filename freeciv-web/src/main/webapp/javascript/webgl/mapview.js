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
var anaglyph_effect;

var plane, cube;
var mouse, raycaster;
var directionalLight;
var clock;
var webgl_controls;

var tiletype_terrains = ["lake","coast","floor","arctic","desert","forest","grassland","hills","jungle","mountains","plains","swamp","tundra", "beach"];

var landGeometry;
var landMesh;
var water;

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
  var ambientLight = new THREE.AmbientLight( 0x606060, 1.1 );
  scene.add(ambientLight);

  directionalLight = new THREE.DirectionalLight( 0xffffff, 2.5 );
  directionalLight.position.set( 0.5, 0.75, 1.0 ).normalize();
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

  if (anaglyph_3d_enabled) {
    anaglyph_effect = new THREE.AnaglyphEffect( maprenderer );
    anaglyph_effect.setSize( new_mapview_width, new_mapview_height );
  }

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

  // high quality water using WebGL shader
  water = new THREE.Water( maprenderer, camera, scene, {
      textureWidth: (graphics_quality == QUALITY_LOW) ? 256 : 512,
      textureHeight: (graphics_quality == QUALITY_LOW) ? 256 : 512,
      waterNormals: webgl_textures["waternormals"],
      alpha: 0.7,
      sunDirection: directionalLight.position.clone().normalize(),
      sunColor: 0xfaf100,
      waterColor: 0x003e7b,
      distortionScale: 3.0,
      fog: false
  } );

  var mirrorMesh = new THREE.Mesh(
    new THREE.PlaneBufferGeometry( mapview_model_width - 10, mapview_model_height - 10),
    water.material
  );

  mirrorMesh.add( water );
  mirrorMesh.rotation.x = - Math.PI * 0.5;
  mirrorMesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), 50);
  mirrorMesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), Math.floor(mapview_model_width / 2) - 500);
  mirrorMesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), -mapview_model_height / 2);
  scene.add( mirrorMesh );

  if (graphics_quality > QUALITY_LOW) {
    var sunGeometry = new THREE.PlaneGeometry( 1000, 1000, 2, 2);
    sunGeometry.rotateX( Math.PI / 2 );
    sunGeometry.translate(Math.floor(mapview_model_width / 2) - 500, 800, Math.floor(mapview_model_height / 3));
    var sunMesh = new THREE.Mesh( sunGeometry, webgl_materials['sun'] );
    scene.add(sunMesh);
  }

  /* heightmap image */
  create_heightmap();
  init_borders_image();
  init_roads_image();

  /* uniforms are variables which are used in the fragment shader fragment.js */
  var freeciv_uniforms = {
    maptiles: { type: "t", value: init_map_tiletype_image() },
    borders: { type: "t", value: update_borders_image() },
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
    vertexShader: document.getElementById('terrain_vertex_shh').innerHTML,
    fragmentShader: document.getElementById('terrain_fragment_shh').innerHTML,
    vertexColors: THREE.VertexColors
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

  for ( var i = 0; i < landGeometry.faces.length; i ++ ) {
    var v = ['a', 'b', 'c'];
    var vertex_colors = [];
    for (var r = 0; r < v.length; r++) {
      var vertex = landGeometry.vertices[landGeometry.faces[i][v[r]]];
      var vPos = landMesh.localToWorld(vertex);
      var mapPos = scene_to_map_coords(vPos.x, vPos.z);
      if (mapPos['x'] >= 0 && mapPos['y'] >= 0) {
        var ptile = map_pos_to_tile(mapPos['x'], mapPos['y']);
        if (ptile == null) {
          vertex_colors.push(new THREE.Color(0,0,0));
        } else if (tile_get_known(ptile) == TILE_KNOWN_SEEN) {
          vertex_colors.push(new THREE.Color(1,0,0));
        } else if (tile_get_known(ptile) == TILE_KNOWN_UNSEEN) {
          vertex_colors.push(new THREE.Color(0.40,0,0));
        } else if (tile_get_known(ptile) == TILE_UNKNOWN) {
          vertex_colors.push(new THREE.Color(0,0,0));
        } else {
          vertex_colors.push(new THREE.Color(0,0,0));
        }
      } else {
        vertex_colors.push(new THREE.Color(0,0,0));
      }
    }
    landGeometry.faces[i].vertexColors = vertex_colors;
  }
  if (graphics_quality == QUALITY_HIGH) {
    setInterval(update_tiles_known_vertex_colors, 350);
  } else {
    setInterval(update_tiles_known_vertex_colors, 1200);
  }


  prerender(landGeometry, xquality);

  add_all_objects_to_scene();

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

  update_animated_objects();

  if (water != null && tree_points != null && jungle_points != null) {
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

  if (anaglyph_3d_enabled) {
    anaglyph_effect.render(scene,camera );
  } else if (cardboard_vr_enabled) {
    stereoEffect.render(scene, camera);
  } else {
    maprenderer.render(scene,camera );
  }

  if (goto_active) check_request_goto_path();
  if (stats != null) stats.end();
  if (initial_benchmark_enabled || benchmark_enabled) benchmark_frames_count++;

  if (webgl_controls != null && clock != null) webgl_controls.update(clock.getDelta());

  if (renderer == RENDERER_WEBGL) requestAnimationFrame(animate);

}