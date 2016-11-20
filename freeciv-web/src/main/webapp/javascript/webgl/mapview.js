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

var dae;
var start_webgl;


/****************************************************************************
  Start the Freeciv-web WebGL renderer
****************************************************************************/
function webgl_start_renderer()
{
  container = document.getElementById('canvas_div');

  camera = new THREE.PerspectiveCamera( 45, window.innerWidth / window.innerHeight, 1, 10000 );

  scene = new THREE.Scene();

  raycaster = new THREE.Raycaster();
  mouse = new THREE.Vector2();

  var geometry = new THREE.PlaneBufferGeometry( 1000, 1000 );
  geometry.rotateX( - Math.PI / 2 );

  plane = new THREE.Mesh( geometry, new THREE.MeshBasicMaterial( { visible: false } ) );
  scene.add( plane );

  objects.push( plane );

  // Lights

  var ambientLight = new THREE.AmbientLight( 0x606060, 1.0 );
  scene.add( ambientLight );

  directionalLight = new THREE.DirectionalLight( 0xffffff, 2.5 );
  directionalLight.position.set( 1, 0.75, 0.5 ).normalize();
  scene.add( directionalLight );

  maprenderer = new THREE.WebGLRenderer( { antialias: true } ); /* TODO: make antialias configurable. */
  maprenderer.setClearColor( 0x000000 );
  maprenderer.setPixelRatio( window.devicePixelRatio );
  maprenderer.setSize( window.innerWidth, window.innerHeight );
  container.appendChild( maprenderer.domElement );

  if (location.hostname === "localhost") {
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
  var material = new THREE.MeshBasicMaterial( { map: webgl_textures["water_overlay"], overdraw: 0.5, transparent: true, opacity: 0.75, color: 0x5555ff } );

  var quality = 32, step = 1024 / quality;

  var geometry = new THREE.PlaneGeometry( 3000, 2000, quality - 1, quality - 1 );
  geometry.rotateX( - Math.PI / 2 );
  geometry.translate(1000, 0, 1000);

  for ( var i = 0, l = geometry.vertices.length; i < l; i ++ ) {
      var x = i % quality, y = Math.floor( i / quality );
      geometry.vertices[ i ].y = 50;
  }

  mesh = new THREE.Mesh( geometry, material );
  scene.add( mesh );

  /* create a texture which contains one pixel for each map tile, where the color of each pixel
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

  /* create a WebGL shader for terrain. */
  var terrain_material = new THREE.ShaderMaterial({
    uniforms: uniforms,
    vertexShader: vertShader,
    fragmentShader: fragShader
  });

  var xquality = map.xsize * 4 + 1;
  var yquality = map.ysize * 4 + 1;
  var step = 1024 / quality;

  /* LandGeometry is a plane representing the landscape of the map. */
  var landGeometry = new THREE.PlaneGeometry(3000, 2000, xquality - 1, yquality - 1);
  landGeometry.rotateX( - Math.PI / 2 );
  landGeometry.translate(1000, 0, 1000);


  for ( var i = 0, l = landGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    if (heightmap[x] != null && heightmap[x][y] != null) {
      landGeometry.vertices[ i ].y = heightmap[x][y] * 100;
    } else {
      //console.log("x: " + x + ", y: " + y + " not found in heightmap.");
    }

  }
  landGeometry.computeVertexNormals();
  landGeometry.computeMorphNormals();


  landMesh = new THREE.Mesh( landGeometry, terrain_material );
  scene.add( landMesh );

  prerender(landGeometry, xquality);
  add_all_objects_to_scene();

  scene.add(meshes['trees']);
  $.unblockUI();
  console.log("WebGL render_testmap took: " + (new Date().getTime() - start_webgl) + " ms.");
}

/****************************************************************************
  Main animation method
****************************************************************************/
function animate() {
  if (stats != null) stats.begin();
  maprenderer.render( scene, camera );
  if (stats != null) stats.end();
  requestAnimationFrame( animate );

}