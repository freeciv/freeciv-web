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

var QUALITY_LOW = 1;    // low quality, no antialiasing. Default for mobile.
var QUALITY_MEDIUM = 2; // medium quality. Default for desktop and laptop computers.
var QUALITY_HIGH = 3;   // best quality, add features which require high-end graphics hardware here.

var graphics_quality = QUALITY_HIGH;

/****************************************************************************
  Init the Freeciv-web WebGL renderer
****************************************************************************/
function init_webgl_renderer()
{
  // load Three.js dynamically.
  $.ajax({
      async: false,
      url: "/javascript/webgl/libs/three.min.js",
      dataType: "script"
  });


  $.ajax({
    async: false,
    url: "/javascript/webgl/libs/webgl-client.min.js",
    dataType: "script"
  });

  if (!Detector.webgl) {
    swal("3D WebGL not supported by your browser or you don't have a 3D graphics card. Please go back and try the 2D version instead. ");
    return;
  }

  /* Loads the two tileset definition files */
  $.ajax({
    url: "/javascript/2dcanvas/tileset_config_amplio2.js",
    dataType: "script",
    async: false
  }).fail(function() {
    console.error("Unable to load tileset config.");
  });

  $.ajax({
    url: "/javascript/2dcanvas/tileset_spec_amplio2.js",
    dataType: "script",
    async: false
  }).fail(function() {
    console.error("Unable to load tileset spec. Run Freeciv-img-extract.");
  });

  if (is_touch_device()) {
    $.ajax({
      url: "/javascript/libs/hammer.min.js",
      dataType: "script",
      async: false
    });
  }


  terrainVertShader = document.getElementById('terrain_vertex_shh').innerHTML;
  terrainFragShader = document.getElementById('terrain_fragment_shh').innerHTML;
  darknessVertShader = document.getElementById('darkness_vertex_shh').innerHTML;
  darknessFragShader = document.getElementById('darkness_fragment_shh').innerHTML;

  init_sprites();

  var stored_graphics_quality_setting = simpleStorage.get("graphics_quality", "");
  if (stored_graphics_quality_setting != null && stored_graphics_quality_setting > 0) {
    graphics_quality = stored_graphics_quality_setting;
  } else if (is_small_screen()) {
    graphics_quality = QUALITY_LOW;
  } else {
    graphics_quality = QUALITY_HIGH; //default value
  }

}


/****************************************************************************
  Preload is complete.
****************************************************************************/
function webgl_preload_complete()
{
  $.unblockUI();
  if ($.getUrlVar('autostart') == "true") {
    username = "autostart";
    network_init();
  }
}