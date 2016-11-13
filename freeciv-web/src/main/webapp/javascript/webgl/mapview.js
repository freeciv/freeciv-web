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
  camera_look_at(1000, 0, 1000);

  scene = new THREE.Scene();

  // roll-over helpers

  rollOverGeo = new THREE.BoxGeometry( 50, 50, 50 );
  rollOverMaterial = new THREE.MeshBasicMaterial( { color: 0xff0000, opacity: 0.5, transparent: true } );
  rollOverMesh = new THREE.Mesh( rollOverGeo, rollOverMaterial );
  scene.add( rollOverMesh );

    // cubes

  cubeGeo = new THREE.BoxGeometry( 50, 50, 50 );
  cubeMaterial = new THREE.MeshLambertMaterial( { color: 0xfeb74c, map: new THREE.TextureLoader().load( "/textures/square-outline-textured.png" ) } );

  // grid

  var step = 50;
  var size = step * (map['xsize'] / 2);

  var geometry = new THREE.Geometry();

  for ( var i = 0; i <= size; i += step ) {

    geometry.vertices.push( new THREE.Vector3( 0, 0, i ) );
    geometry.vertices.push( new THREE.Vector3(   size, 0, i ) );

    geometry.vertices.push( new THREE.Vector3( i, 0, 0 ) );
    geometry.vertices.push( new THREE.Vector3( i, 0,   size ) );

  }

  var material = new THREE.LineBasicMaterial( { color: 0x000000, opacity: 0.2, transparent: true } );

  var line = new THREE.LineSegments( geometry, material );
  scene.add( line );

  //

  raycaster = new THREE.Raycaster();
  mouse = new THREE.Vector2();

  var geometry = new THREE.PlaneBufferGeometry( 1000, 1000 );
  geometry.rotateX( - Math.PI / 2 );

  plane = new THREE.Mesh( geometry, new THREE.MeshBasicMaterial( { visible: false } ) );
  scene.add( plane );

  objects.push( plane );

  // Lights

  var ambientLight = new THREE.AmbientLight( 0x606060 );
  scene.add( ambientLight );

  var directionalLight = new THREE.DirectionalLight( 0xffffff );
  directionalLight.position.set( 1, 0.75, 0.5 ).normalize();
  scene.add( directionalLight );

  maprenderer = new THREE.WebGLRenderer( { antialias: true } ); /* TODO: make antialias configurable. */
  maprenderer.setClearColor( 0x000000 );
  maprenderer.setPixelRatio( window.devicePixelRatio );
  maprenderer.setSize( window.innerWidth, window.innerHeight );
  container.appendChild( maprenderer.domElement );

  document.addEventListener( 'mousemove', webglOnDocumentMouseMove, false );
  document.addEventListener( 'mousedown', webglOnDocumentMouseDown, false );
  document.addEventListener( 'keydown', webglOnDocumentKeyDown, false );
  document.addEventListener( 'keyup', webglOnDocumentKeyUp, false );
  window.addEventListener( 'resize', webglOnWindowResize, false );

  if (location.hostname === "localhost") {
    stats = new Stats();
    container.appendChild( stats.dom );
  }

  animate();
  webgl_preload();

}




/****************************************************************************
...
****************************************************************************/
function render_testmap() {
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

  if (webgl_models["settler"] != null) scene.add(webgl_models["settler"]);

  /* Trees */
  var trees_geometry = new THREE.Geometry();
  for (var i = 0, l = landGeometry.vertices.length; i < l; i++) {
    var x = i % xquality, y = Math.floor(i / xquality);
    var gx = Math.round(x / 4 - 0.5);
    var gy = Math.round(y / 4 - 0.5);
    var ptile = map_pos_to_tile(gx, gy);
    if (ptile != null && tile_get_known(ptile) != TILE_UNKNOWN) {
      var terrain_name = tile_terrain(ptile).name;
      var add_tree = false;
      if (terrain_name == "Forest" || terrain_name == "Jungle") {
        /* Dense forests. */
        add_tree = true;
      } else if (terrain_name == "Grassland" ||
                 terrain_name == "Plains" ||
                 terrain_name == "Hills") {
        /* Sparse trees -> Monte-Carlo algorithm */
        add_tree = (Math.random() < 0.03);
      }

      /* No trees on beaches */
      add_tree &= (landGeometry.vertices[i].y > 57);

      if (add_tree) {
        var geometry = new THREE.SphereGeometry(6, 4, 4);
        geometry.rotateX(0.1 * Math.PI * Math.random());
        geometry.rotateY(0.1 * Math.PI * Math.random());
        geometry.rotateZ(0.1 * Math.PI * Math.random());
        geometry.translate(landGeometry.vertices[i].x + 5,
                           landGeometry.vertices[i].y + 2,
                           landGeometry.vertices[i].z + 5);
        geometry.translate(2.5 * (-1 + 2*Math.random()),
                           0.1 * (-1 + 2*Math.random()),
                           2.5 * (-1 + 2*Math.random()));

        var mesh = new THREE.Mesh(geometry);
        mesh.updateMatrix();
        trees_geometry.merge(mesh.geometry, mesh.matrix);
      }
    }
  }
  var trees_material = new THREE.MeshLambertMaterial({
      color: new THREE.Color(0, 0.15, 0),
      emissive: new THREE.Color(0, 0.01, 0) });
  var trees_mesh = new THREE.Mesh(trees_geometry, trees_material);
  scene.add(trees_mesh);

  console.log("WebGL render_testmap took: " + (new Date().getTime() - start_webgl) + " ms.");
}

/****************************************************************************
...
****************************************************************************/
function animate() {
  if (stats != null) stats.begin();
  maprenderer.render( scene, camera );
  if (stats != null) stats.end();
  requestAnimationFrame( animate );

}


