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

var container;
var camera, scene, renderer;
var group;
var mouseX = 0, mouseY = 0;
var controls;
var clock = null;
var dirLight;

var windowHalfX = window.innerWidth / 2;
var windowHalfY = window.innerHeight / 2;
var active_globeview = false;
var fullmap_canvas = null;

/**************************************************************************
  Renders the world map on a globe using Three.js, with a loading message.
**************************************************************************/
function init_3d_globe_view()
{
  $.blockUI({ message: "<h2>Creating 3D globe view of map. Please wait.</h2>" });
  setTimeout(show_3d_globe_view, 100);
  keyboard_input = false;
}

/**************************************************************************
  Renders the world map on a 2d canvas.
**************************************************************************/
function init_2d_globe_view()
{
  $.blockUI({ message: "<h2>Creating 2D map view of the world map. Please wait.</h2>" });
  setTimeout(show_2d_globe_view, 100);
  keyboard_input = false;
}

/**************************************************************************
  Renders the world map on a globe using Three.js.
**************************************************************************/
function show_3d_globe_view()
{
  // reset dialog page.
  $("#globe_view_dialog").remove();
  $("<div id='globe_view_dialog'></div>").appendTo("div#game_page");

  // load Three.js dynamically. 
  $.ajax({
    async: false,
    url: "/javascript/libs/three.min.js",
    dataType: "script"
  });
  $.ajax({
    async: false,
    url: "/javascript/libs/Projector.js",
    dataType: "script"
  });
  $.ajax({
    async: false,
    url: "/javascript/libs/CanvasRenderer.js",
    dataType: "script"
  });
  $.ajax({
    async: false,
    url: "/javascript/libs/Detector.js",
    dataType: "script"
  });
  $.ajax({
    async: false,
    url: "/javascript/libs/FlyControls.js",
    dataType: "script"
  });

  $("#globe_view_dialog").html("<center><div id='globe_container' style='width: 100%; background-color: #0a0a0a;'></div></center>"
                               + "<small>Controls: <b>WASD</b> move, <b>R|F</b> up | down, "
                               + "<b>Q|E</b> roll, <b>up|down</b> pitch, <b>left|right</b> yaw</small>");

  $("#globe_view_dialog").attr("title", server_settings['metamessage']['val']);
  $("#globe_view_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "98%",
                        height: is_small_screen() ? $(window).height()-30 : $(window).height() - 10,
                        close : function(){
                          close_globe_view_dialog();
                        }
                     });

  $("#globe_view_dialog").dialog('open');

  // step 1: render whole map on a canvas using the Freeciv-web renderer.
  var tmpmap_canvas = document.createElement('canvas');
  var w = map['xsize'] * tileset_tile_width * 0.9;
  var h = map['ysize'] * tileset_tile_height * 1.4;

  tmpmap_canvas.width = w;
  tmpmap_canvas.height = h;

  mapview['width'] = w;
  mapview['height'] = h; 
  mapview['store_width'] = w;
  mapview['store_height'] = h;

  center_tile_mapcanvas(map_pos_to_tile(map['xsize']/2, map['ysize']/2));

  mapview_canvas = tmpmap_canvas;
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
  mapview_canvas_ctx.font = canvas_text_font;

    
  update_map_canvas_full();

  // step 2: change aspect ratio of map so that width = height, 
  // rendering the result on a new canvas.
  var scaledmap_canvas = document.createElement('canvas');
  scaledmap_canvas.width = w;
  scaledmap_canvas.height = w;
  var scaledmap_ctx = scaledmap_canvas.getContext("2d");
  scaledmap_ctx.drawImage(tmpmap_canvas, 0,0, w, h, 0, 0, w, w);

  tmp_canvas = null;

  // step 3: rotate the image -41 degrees and render the result on a new canvas.  
  // The canvas width and height must be a size in the power of 2, eg. 4096 and 2048.
  fullmap_canvas = document.createElement('canvas');
  fullmap_canvas.width = 4096;
  fullmap_canvas.height = 2048;
  fullmap_ctx = fullmap_canvas.getContext("2d");
  var x = fullmap_canvas.width * 0.5;
  var y = fullmap_canvas.height * 0.48;
  fullmap_ctx.translate(x, y);
  fullmap_ctx.rotate(-41 * Math.PI / 180);
  fullmap_ctx.drawImage(scaledmap_canvas, 0, 0, w, w, -4096 * 0.52, -2048 * 0.5 - 850, 4096, 4096 - 400);
  fullmap_ctx.rotate(41 * Math.PI / 180);
  fullmap_ctx.translate(-x, -y);
 
  // step 4: clip away left and right part of image, to make it look better. 
  fullmap_ctx.drawImage(fullmap_canvas, 4096 * 0.1, 0, 4096 - 4096 * 0.1, 2048, 0, 0, 4096 * 1.22, 2048);

  // result: the fullmap_canvas now contains the image which will be rendered on a sphere.

  scaledmap_canvas = null;

  active_globeview = true;

  globe_view_init();
  globe_view_animate();

  $.unblockUI();

  // prevent array keys from scrolling window. 
  window.addEventListener("keydown", function(e) {
    if([32, 37, 38, 39, 40].indexOf(e.keyCode) > -1) {
        e.preventDefault();
    }
  }, false);

}


/**************************************************************************
  Renders the world map to a canvas
**************************************************************************/
function show_2d_globe_view()
{
  // reset dialog page.
  $("#globe_view_dialog").remove();
  $("<div id='globe_view_dialog'></div>").appendTo("div#game_page");


  $("#globe_view_dialog").html("<center><div id='globe_container' style='width: 100%; background-color: #0a0a0a;'>"
        + "<canvas id='2d_globeview_canvas'></canvas></div></center>");

  $("#globe_view_dialog").attr("title", server_settings['metamessage']['val']);
  $("#globe_view_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "98%",
                        height: $(window).height()-30,
                        close : function(){
                          close_globe_view_dialog();
                        }
                     });

  $("#globe_view_dialog").dialog('open');

  // step 1: render whole map on a canvas using the Freeciv-web renderer.
  var tmpmap_canvas = document.createElement('canvas');
  var w = map['xsize'] * tileset_tile_width * 0.9;
  var h = map['ysize'] * tileset_tile_height * 1.4;

  tmpmap_canvas.width = w;
  tmpmap_canvas.height = h;

  mapview['width'] = w;
  mapview['height'] = h;
  mapview['store_width'] = w;
  mapview['store_height'] = h;

  center_tile_mapcanvas(map_pos_to_tile(map['xsize']/2, map['ysize']/2));

  mapview_canvas = tmpmap_canvas;
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
  mapview_canvas_ctx.font = canvas_text_font;

  update_map_canvas_full();

  // step 2: change aspect ratio of map so that width = height,
  // rendering the result on a new canvas.
  var scaledmap_canvas = document.createElement('canvas');
  scaledmap_canvas.width = w;
  scaledmap_canvas.height = w;
  var scaledmap_ctx = scaledmap_canvas.getContext("2d");
  scaledmap_ctx.drawImage(tmpmap_canvas, 0,0, w, h, 0, 0, w, w);

  tmp_canvas = null;

  // step 3: rotate the image -41 degrees and render the result on a new canvas.
  fullmap_canvas = document.createElement('canvas');
  fullmap_canvas.width = 4096;
  fullmap_canvas.height = 2048;
  fullmap_ctx = fullmap_canvas.getContext("2d");
  var x = fullmap_canvas.width * 0.5;
  var y = fullmap_canvas.height * 0.48;
  fullmap_ctx.translate(x, y);
  fullmap_ctx.rotate(-41 * Math.PI / 180);
  fullmap_ctx.drawImage(scaledmap_canvas, 0, 0, w, w, -4096 * 0.52, -2048 * 0.5 - 850, 4096, 4096 - 400);
  fullmap_ctx.rotate(41 * Math.PI / 180);
  fullmap_ctx.translate(-x, -y);

  var rheight = $("#2d_globeview_canvas").parent().parent().parent().height() - 5;
  var rwidth = $("#2d_globeview_canvas").parent().width() - 5;

  var finalmap_canvas = document.getElementById('2d_globeview_canvas');
  finalmap_canvas.width = rwidth;
  finalmap_canvas.height = rheight;
  var finalmap_ctx = finalmap_canvas.getContext("2d");
  finalmap_ctx.drawImage(fullmap_canvas, 4096 * 0.1, 0, 4096 - 4096 * 0.1, 2048, 0, 0, rwidth * 1.22, rheight);

  scaledmap_canvas = null;
  fullmap_canvas = null;

  active_globeview = true;

  $.unblockUI();

}

/**************************************************************************
  ...
**************************************************************************/
function close_globe_view_dialog()
{
  active_globeview = false;
  setup_window_size();
  set_default_mapview_active();
  center_tile_mapcanvas(map_pos_to_tile(map['xsize']/2, map['ysize']/2));
  setTimeout(set_default_mapview_active, 1000);
  keyboard_input = true;
  $(".setting_button").tooltip("destroy");
}

/**************************************************************************
  Init of Three.js rendering of the map on a 3d globe.
**************************************************************************/
function globe_view_init() {

  clock = new THREE.Clock();

  container = document.getElementById( 'globe_container' );
  var width = $("#globe_view_dialog").width() - 20;
  var height = $("#globe_view_dialog").height() - 26;
  if (width > height + 350) width = height + 350; 

  camera = new THREE.PerspectiveCamera( 60, width / height, 1, 2000 );
  camera.position.z = 580;

  controls = new THREE.FlyControls(camera);
  controls.movementSpeed = 80;
  controls.domElement = container;
  controls.rollSpeed = Math.PI / 24;
  controls.autoForward = false;
  controls.dragToLook = false;

  scene = new THREE.Scene();
  scene.fog = new THREE.FogExp2( 0x000000, 0.00000025 );

  group = new THREE.Group();
  scene.add( group );

  var geometry = new THREE.SphereGeometry( 300, 36, 36 );

  var texture = new THREE.Texture(fullmap_canvas);
  texture.needsUpdate = true;

  var material = new THREE.MeshBasicMaterial( { map: texture, overdraw: 0.5 } );
  var mesh = new THREE.Mesh( geometry, material );
  group.add( mesh );


  renderer = Detector.webgl? new THREE.WebGLRenderer(): new THREE.CanvasRenderer();
  renderer.setClearColor(0x0a0a0a);
  renderer.setPixelRatio(window.devicePixelRatio);
  renderer.setSize(width, height);

  container.appendChild(renderer.domElement);

  document.addEventListener('mousemove', onDocumentMouseMove, false);

  window.addEventListener('resize', onWindowResize, false);

}

/**************************************************************************
  ...
**************************************************************************/
function onWindowResize() {

  var width = $("#globe_view_dialog").width() - 20;
  var height = $("#globe_view_dialog").height() - 26;
  if (width > height + 350) width = height + 350;

  windowHalfX = $("#globe_view_dialog").width() / 2;
  windowHalfY = $("#globe_view_dialog").height() / 2;

  camera.aspect = width / height;
  camera.updateProjectionMatrix();


  renderer.setSize( width, height);

}

/**************************************************************************
  ...
**************************************************************************/
function onDocumentMouseMove( event ) {

  mouseX = ( event.clientX - windowHalfX );
  mouseY = ( event.clientY - windowHalfY );

}

/**************************************************************************
  ...
**************************************************************************/
function globe_view_animate() {
  if (!active_globeview) return;

  requestAnimationFrame( globe_view_animate );

  globe_view_render();

}

/**************************************************************************
  ...
**************************************************************************/
function globe_view_render() {

  var delta = clock.getDelta();

  group.rotation.y -= 0.0001;

  controls.update(delta);

  renderer.render(scene, camera);

}

