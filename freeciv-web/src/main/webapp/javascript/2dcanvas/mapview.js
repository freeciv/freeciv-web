/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

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


var mapview_canvas_ctx = null;
var mapview_canvas = null;
var buffer_canvas_ctx = null;
var buffer_canvas = null;
var city_canvas_ctx = null;
var city_canvas = null;

var tileset_images = [];
var sprites = {};
var loaded_images = 0;

var sprites_init = false;

var canvas_text_font = "17px Arial"; // with canvas text support

var fullfog = [];

var GOTO_DIR_DX = [0, 1, 2, -1, 1, -2, -1, 0];
var GOTO_DIR_DY = [-2, -1, 0, -1, 1, 0, 1, 2];
var dashedSupport = false;

/**************************************************************************
  ...
**************************************************************************/
function init_mapview()
{

  $("#canvas_div").append($('<canvas/>', { id: 'canvas'}));

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

  mapview_canvas = document.getElementById('canvas');
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
  buffer_canvas = document.createElement('canvas');
  buffer_canvas_ctx = buffer_canvas.getContext('2d');

  if ("mozImageSmoothingEnabled" in mapview_canvas_ctx) {
    // if this Boolean value is false, images won't be smoothed when scaled. This property is true by default.
    mapview_canvas_ctx.mozImageSmoothingEnabled = false;
  }
  dashedSupport = ("setLineDash" in mapview_canvas_ctx);

  setup_window_size();

  mapview['gui_x0'] = 0;
  mapview['gui_y0'] = 0;



  /* Initialize fog array. */
  var i;
  for (i = 0; i < 81; i++) {
    /* Unknown, fog, known. */
    var ids = ['u', 'f', 'k'];
    var buf = "t.fog";
    var values = [];
    var j, k = i;

    for (j = 0; j < 4; j++) {
	  values[j] = k % 3;
	  k = Math.floor(k / 3);

      buf += "_" + ids[values[j]];

    }

    fullfog[i] = buf;
  }


  orientation_changed();
  init_sprites();
  requestAnimationFrame(update_map_canvas_check, mapview_canvas);

}


function sum_width()
{
  var sum=0;
  $("#tabs_menu").children().each( function(){ if ($(this).is(":visible")) sum += $(this).width(); });
  return sum;
}


/**************************************************************************
  ...
**************************************************************************/
function is_small_screen()
{
  if ($(window).width() <= 640 || $(window).height() <= 590) {
    return true;
  } else {
    return false;
  }

}

/**************************************************************************
  This will load the tileset, blocking the UI while loading.
**************************************************************************/
function init_sprites()
{
  $.blockUI({ message: "<h1>Freeciv-web is loading. Please wait..."
	  + "<br><center><img src='/images/loading.gif'></center></h1>" });

  for (var i = 0; i < tileset_image_count; i++) {
    var tileset_image = new Image();
    tileset_image.onload = preload_check;
    tileset_image.src = '/tileset/freeciv-web-tileset-'
                        + tileset_name + '-' + i + '.png?ts=' + ts;
    tileset_images[i] = tileset_image;
  }

}

/**************************************************************************
  Determines when the whole tileset has been preloaded.
**************************************************************************/
function preload_check()
{
  loaded_images += 1;

  if (loaded_images == tileset_image_count) {
    init_cache_sprites();
    if (renderer == RENDERER_WEBGL) {
      webgl_preload();
    } else {
      $.unblockUI();
    }
  }
}

/**************************************************************************
  ...
**************************************************************************/
function init_cache_sprites()
{
 try {

  if (typeof tileset === 'undefined') {
    swal("Tileset not generated correctly. Run sync.sh in "
          + "freeciv-img-extract and recompile.");
    return;
  }

  for (var tile_tag in tileset) {
    var x = tileset[tile_tag][0];
    var y = tileset[tile_tag][1];
    var w = tileset[tile_tag][2];
    var h = tileset[tile_tag][3];
    var i = tileset[tile_tag][4];

    var newCanvas = document.createElement('canvas');
    newCanvas.height = h;
    newCanvas.width = w;
    var newCtx = newCanvas.getContext('2d');

    newCtx.drawImage(tileset_images[i], x, y,
                       w, h, 0, 0, w, h);
    sprites[tile_tag] = newCanvas;

  }

  sprites_init = true;
  tileset_images[0] = null;
  tileset_images[1] = null;
  tileset_images = null;

 }  catch(e) {
  console.log("Problem caching sprite: " + tile_tag);
 }

}

/**************************************************************************
  ...
**************************************************************************/
function mapview_window_resized ()
{
  if (active_city != null || !resize_enabled) return;
  setup_window_size();
  if (renderer == RENDERER_2DCANVAS) update_map_canvas_full();
}

/**************************************************************************
  ...
**************************************************************************/
function drawPath(ctx, x1, y1, x2, y2, x3, y3, x4, y4)
{
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.lineTo(x3, y3);
    ctx.lineTo(x4, y4);
    ctx.lineTo(x1, y1);
}

/**************************************************************************
  ...
**************************************************************************/
function mapview_put_tile(pcanvas, tag, canvas_x, canvas_y) {
  if (sprites[tag] == null) {
    //console.log("Missing sprite " + tag);
    return;
  }

  pcanvas.drawImage(sprites[tag], canvas_x, canvas_y);

}

/****************************************************************************
  Draw a filled-in colored rectangle onto the mapview or citydialog canvas.
****************************************************************************/
function canvas_put_rectangle(canvas_context, pcolor, canvas_x, canvas_y, width, height)
{
  canvas_context.fillStyle = pcolor;
  canvas_context.fillRect (canvas_x, canvas_y, canvas_x + width, canvas_y + height);

}

/****************************************************************************
  Draw a colored rectangle onto the mapview.
****************************************************************************/
function canvas_put_select_rectangle(canvas_context, canvas_x, canvas_y, width, height)
{
  canvas_context.beginPath();
  canvas_context.strokeStyle = "rgb(255,0,0)";
  canvas_context.rect(canvas_x, canvas_y, width, height);
  canvas_context.stroke();

}


/**************************************************************************
  Draw city text onto the canvas.
**************************************************************************/
function mapview_put_city_bar(pcanvas, city, canvas_x, canvas_y) {

  var text = decodeURIComponent(city['name']);
  var size = city['size'];
  var color = nations[city_owner(city)['nation']]['color'];
  var prod_type = get_city_production_type(city);

  var txt_measure = pcanvas.measureText(text);
  var size_measure = pcanvas.measureText(size);
  pcanvas.globalAlpha = 0.65;
  pcanvas.fillStyle = "rgba(0, 0, 0, 0.5)";
  pcanvas.fillRect (canvas_x - Math.floor(txt_measure.width / 2) - 14, canvas_y - 17,
                    txt_measure.width + 20, 20);

  pcanvas.fillStyle = color;
  pcanvas.fillRect(canvas_x + Math.floor(txt_measure.width / 2) + 5, canvas_y - 19,
               (prod_type != null) ? size_measure.width + 35 : size_measure.width + 8, 24);

  var city_flag = get_city_flag_sprite(city);
  pcanvas.drawImage(sprites[city_flag['key']],
              canvas_x - Math.floor(txt_measure.width / 2) - 45, canvas_y - 17);

  pcanvas.drawImage(sprites[get_city_occupied_sprite(city)],
              canvas_x - Math.floor(txt_measure.width / 2) - 12, canvas_y - 16);

  pcanvas.strokeStyle = color;
  pcanvas.lineWidth = 1.5;
  pcanvas.beginPath();
  pcanvas.moveTo(canvas_x - Math.floor(txt_measure.width / 2) - 46, canvas_y - 18);
  pcanvas.lineTo(canvas_x + Math.floor(txt_measure.width / 2) + size_measure.width + 13,
                 canvas_y - 18);
  pcanvas.moveTo(canvas_x + Math.floor(txt_measure.width / 2) + size_measure.width + 13,
                 canvas_y + 4);
  pcanvas.lineTo(canvas_x - Math.floor(txt_measure.width / 2) - 46, canvas_y + 4);
  pcanvas.lineTo(canvas_x - Math.floor(txt_measure.width / 2) - 46, canvas_y - 18);
  pcanvas.moveTo(canvas_x - Math.floor(txt_measure.width / 2) - 15, canvas_y - 17);
  pcanvas.lineTo(canvas_x - Math.floor(txt_measure.width / 2) - 15, canvas_y + 3);
  pcanvas.stroke();

  pcanvas.globalAlpha = 1.0;

  if (prod_type != null) {
    var tag = prod_type['graphic_str'];
    if (tileset[tag] == null) return;
    pcanvas.drawImage(sprites[tag],
              canvas_x + Math.floor(txt_measure.width / 2) + size_measure.width + 13,
              canvas_y - 19, 28, 24);
  }

  pcanvas.fillStyle = "rgba(0, 0, 0, 1)";
  pcanvas.fillText(size, canvas_x + Math.floor(txt_measure.width / 2) + 10, canvas_y + 1);
  pcanvas.fillStyle = "rgba(255, 255, 255, 1)";
  pcanvas.fillText(text, canvas_x - Math.floor(txt_measure.width / 2) - 2, canvas_y - 1);
  pcanvas.fillText(size, canvas_x + Math.floor(txt_measure.width / 2) + 8, canvas_y - 1);

}

/**************************************************************************
  Draw tile label onto the canvas.
**************************************************************************/
function mapview_put_tile_label(pcanvas, tile, canvas_x, canvas_y) {
  var text = tile['label'];
  var txt_measure = pcanvas.measureText(text);

  pcanvas.fillStyle = "rgba(255, 255, 255, 1)";
  pcanvas.fillText(text, canvas_x + normal_tile_width / 2 - Math.floor(txt_measure.width / 2), canvas_y - 1);
}

/**************************************************************************
  Renders the national border lines onto the canvas.
**************************************************************************/
function mapview_put_border_line(pcanvas, dir, color, canvas_x, canvas_y) {
  var x = canvas_x + 47;
  var y = canvas_y + 3;
  pcanvas.strokeStyle = color;
  pcanvas.beginPath();

  if (dir == DIR8_NORTH) {
    pcanvas.moveTo(x, y - 2, x + (tileset_tile_width / 2));
    pcanvas.lineTo(x + (tileset_tile_width / 2),  y + (tileset_tile_height / 2) - 2);
  } else if (dir == DIR8_EAST) {
    pcanvas.moveTo(x - 3, y + tileset_tile_height - 3);
    pcanvas.lineTo(x + (tileset_tile_width / 2) - 3,  y + (tileset_tile_height / 2) - 3);
  } else if (dir == DIR8_SOUTH) {
    pcanvas.moveTo(x - (tileset_tile_width / 2) + 3, y + (tileset_tile_height / 2) - 3);
    pcanvas.lineTo(x + 3,  y + tileset_tile_height - 3);
  } else if (dir == DIR8_WEST) {
    pcanvas.moveTo(x - (tileset_tile_width / 2) + 3, y + (tileset_tile_height / 2) - 3);
    pcanvas.lineTo(x + 3,  y - 3);
  }
  pcanvas.closePath();
  pcanvas.stroke();

}

/**************************************************************************
...
**************************************************************************/
function mapview_put_goto_line(pcanvas, dir, canvas_x, canvas_y) {

  var x0 = canvas_x + (tileset_tile_width / 2);
  var y0 = canvas_y + (tileset_tile_height / 2);
  var x1 = x0 + GOTO_DIR_DX[dir] * (tileset_tile_width / 2);
  var y1 = y0 + GOTO_DIR_DY[dir] * (tileset_tile_height / 2);

  pcanvas.strokeStyle = 'rgba(0,168,255,0.9)';
  pcanvas.lineWidth = 10;
  pcanvas.lineCap = "round";
  pcanvas.beginPath();
  pcanvas.moveTo(x0, y0);
  pcanvas.lineTo(x1, y1);
  pcanvas.stroke();

}

/**************************************************************************
  ...
**************************************************************************/
function set_city_mapview_active()
{
  city_canvas = document.getElementById('city_canvas');
  if (city_canvas == null) return;
  city_canvas_ctx = city_canvas.getContext('2d');
  city_canvas_ctx.font = canvas_text_font;

  mapview_canvas_ctx = city_canvas.getContext("2d");

  mapview['width'] = 350;
  mapview['height'] = 175;
  mapview['store_width'] = 350;
  mapview['store_height'] = 175;

  set_default_mapview_inactive();

}

/**************************************************************************
  ...
**************************************************************************/
function set_default_mapview_inactive()
{
  if (overview_active) $("#game_overview_panel").parent().hide();
  if (unitpanel_active) $("#game_unit_panel").parent().hide();
  if (chatbox_active) $("#game_chatbox_panel").parent().hide();

}

/**************************************************************************
 Initializes mapview sliding. This is done by rendering the area to scroll
 across to a new canvas (buffer_canvas), and clip a region of this
 buffer_canvas to the mapview canvas so it looks like scrolling.
**************************************************************************/
function enable_mapview_slide(ptile)
{
  var r = map_to_gui_pos(ptile['x'], ptile['y']);
  var gui_x = r['gui_dx'];
  var gui_y = r['gui_dy'];

  gui_x -= (mapview['width'] - tileset_tile_width) >> 1;
  gui_y -= (mapview['height'] - tileset_tile_height) >> 1;

  var dx = gui_x - mapview['gui_x0'];
  var dy = gui_y - mapview['gui_y0'];
  mapview_slide['dx'] = dx;
  mapview_slide['dy'] = dy;
  mapview_slide['i'] = mapview_slide['max'];
  mapview_slide['start'] = new Date().getTime();

  if ((dx == 0 && dy == 0) || mapview_slide['active']
      || Math.abs(dx) > mapview['width'] || Math.abs(dy) > mapview['height']) {
    // sliding across map edge: don't slide, just go there directly.
    mapview_slide['active'] = false;
    update_map_canvas_full();
    return;
  }

  mapview_slide['active'] = true;

  var new_width = mapview['width'] + Math.abs(dx);
  var new_height = mapview['height'] + Math.abs(dy);
  var old_width = mapview['store_width'];
  var old_height = mapview['store_height'];

  mapview_canvas = buffer_canvas;
  mapview_canvas_ctx = buffer_canvas_ctx;

  if (dx >= 0 && dy <= 0) {
    mapview['gui_y0'] -= Math.abs(dy);
  } else if (dx <= 0 && dy >= 0) {
    mapview['gui_x0'] -= Math.abs(dx);
  }  else if (dx <= 0 && dy <= 0) {
    mapview['gui_x0'] -= Math.abs(dx);
    mapview['gui_y0'] -= Math.abs(dy);
  }

  mapview['store_width'] = new_width;
  mapview['store_height'] = new_height;
  mapview['width'] = new_width;
  mapview['height'] = new_height;

  /* redraw mapview on large back buffer. */
  if (dx >= 0 && dy >= 0) {
    update_map_canvas(old_width, 0, dx, new_height);
    update_map_canvas(0, old_height, old_width, dy);
  } else if (dx <= 0 && dy <= 0) {
    update_map_canvas(0, 0, Math.abs(dx), new_height);
    update_map_canvas(Math.abs(dx), 0, old_width, Math.abs(dy));
  } else if (dx <= 0 && dy >= 0) {
    update_map_canvas(0, 0, Math.abs(dx), new_height);
    update_map_canvas(Math.abs(dx), old_height, old_width, Math.abs(dy));
  } else if (dx >= 0 && dy <= 0) {
    update_map_canvas(0, 0, new_width, Math.abs(dy));
    update_map_canvas(old_width, Math.abs(dy), Math.abs(dx), old_height);
  }

  /* restore default mapview. */
  mapview_canvas = document.getElementById('canvas');
  mapview_canvas_ctx = mapview_canvas.getContext("2d");

  if (dx >= 0 && dy >= 0) {
    buffer_canvas_ctx.drawImage(mapview_canvas, 0, 0, old_width, old_height, 0, 0, old_width, old_height);
  } else if (dx <= 0 && dy <= 0) {
    buffer_canvas_ctx.drawImage(mapview_canvas, 0, 0, old_width, old_height, Math.abs(dx), Math.abs(dy), old_width, old_height);
  } else if (dx <= 0 && dy >= 0) {
    buffer_canvas_ctx.drawImage(mapview_canvas, 0, 0, old_width, old_height, Math.abs(dx), 0, old_width, old_height);
  } else if (dx >= 0 && dy <= 0) {
    buffer_canvas_ctx.drawImage(mapview_canvas, 0, 0, old_width, old_height, 0, Math.abs(dy), old_width, old_height);
  }
  mapview['store_width'] = old_width;
  mapview['store_height'] = old_height;
  mapview['width'] = old_width;
  mapview['height'] = old_height;

}