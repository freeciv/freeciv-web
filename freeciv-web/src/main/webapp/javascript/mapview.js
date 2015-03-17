/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
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
var mapview_slide = {};
mapview_slide['active'] = false;
mapview_slide['dx'] = 0;
mapview_slide['dy'] = 0;
mapview_slide['i'] = 0;
mapview_slide['max'] = 100; 
mapview_slide['slide_time'] = 700;

var height_offset = 67;
var width_offset = 10;

var canvas_text_font = "14pt Arial"; // with canvas text support
var fontsize = 12;

var fullfog = [];

var GOTO_DIR_DX = [0, 1, 2, -1, 1, -2, -1, 0];
var GOTO_DIR_DY = [-2, -1, 0, -1, 1, 0, 1, 2];

var dashedSupport = false;


/**************************************************************************
  ...
**************************************************************************/
function init_mapview()
{

  /* Loads the two tileset definition files */
  $.ajax({
    url: "/javascript/tileset_config_amplio2.js",
    dataType: "script",
    async: false
  }).fail(function() {
    console.error("Unable to load tileset config.");
  });

  $.ajax({
    url: "/javascript/tileset_spec_amplio2.js",
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

}


/**************************************************************************
  ...
**************************************************************************/
function setup_window_size () 
{
  var winWidth = $(window).width(); 
  var winHeight = $(window).height(); 

  mapview_canvas.width = winWidth - width_offset;
  mapview_canvas.height = winHeight - height_offset;
  buffer_canvas.width = Math.floor(mapview_canvas.width * 1.5);
  buffer_canvas.height = Math.floor(mapview_canvas.height * 1.5);

  mapview['width'] = winWidth - width_offset;
  mapview['height'] = winHeight - height_offset; 
  mapview['store_width'] = winWidth - width_offset;
  mapview['store_height'] = winHeight - height_offset;

  mapview_canvas_ctx.font = canvas_text_font;
  buffer_canvas_ctx.font = canvas_text_font;

  $("#game_status_panel").css("width", mapview_canvas.width);

  $('#tabs').css("height", $(window).height());
  $("#tabs-map").height("auto");

  $("#pregame_message_area").height( mapview['height'] - 80);
  $("#pregame_player_list").height( mapview['height'] - 80);
  $("#technologies").height( mapview['height'] - 50);
  $("#technologies").width( mapview['width'] - 20);
  
  $("#nations").height( mapview['height'] - 100);
  $("#nations").width( mapview['width']);
  
  $("#city_viewport").height( mapview['height'] - 20);

  var i = 0;

  $("#opt_tab").children().html("Options")
  $("#players_tab").children().html("Nations")
  $("#tech_tab").children().html("Research")
  $("#civ_tab").children().html("Government")

  $("#opt_tab").show();
  $("#players_tab").show(); 
  $("#freeciv_logo").show();
  $("#tabs-hel").hide();


  /* dynamically reduce content in top meny according to content*/
  while ($(window).width() - sum_width() < 30) {
    if (i == 0) $("#freeciv_logo").width(40);
    if (i == 1) $("#hel_tab").hide();

    if (i == 2) $("#opt_tab").children().html("Opts")
    if (i == 3) $("#players_tab").children().html("Nat")
    if (i == 4) $("#tech_tab").children().html("Res")
    if (i == 5) $("#civ_tab").children().html("Govt")

    if (i == 6) $("#freeciv_logo").hide();
    if (i == 7) $("#opt_tab").children().html("O")
    if (i == 8) $("#players_tab").children().html("N")
    if (i == 9) $("#tech_tab").children().html("R")
    if (i == 10) $("#civ_tab").children().html("G")
    if (i == 11) $("#map_tab").children().html("M")

    if (i == 12) $("#opt_tab").hide();
    if (i == 13) $("#tabs-hel").hide();
    if (i == 14) $("#players_tab").hide(); 

    if (i == 15) break;

    i++;
  }

  if (is_small_screen()) {
    $(".ui-tabs-anchor").css("padding", "3px");
    $(".ui-button-text").css("padding", "5px");
    $(".overview_dialog").hide();
    $(".unit_dialog").hide();
    $(".ui-dialog-titlebar").hide();

    overview_active = false;
    unitpanel_active = false;
    $("#game_unit_orders_default").css("bottom", "3px");
    $("#game_status_panel").css("font-size", "0.8em");
    $(".order_button").css("padding-right", "5px");
    $("#freeciv_logo").height(22);
  }

  if (overview_active) init_overview();
  if (unitpanel_active) init_game_unit_panel(); 

	
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
  var winWidth = $(window).width(); 
  var winHeight = $(window).height(); 

  if (winWidth <= 640 || winHeight <= 640) {
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
    $.unblockUI();
    init_cache_sprites();
    init_common_intro_dialog();
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
  delete tileset_images;

 }  catch(e) {
  console.log("Problem caching sprite: " + tile_tag);
 }

}

/**************************************************************************
  ...
**************************************************************************/
function mapview_window_resized ()
{
  if (active_city != null) return;
  setup_window_size();
  update_map_canvas_full();
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

/**************************************************************************
  Draw city text onto the canvas.
**************************************************************************/
function mapview_put_city_bar(pcanvas, city, canvas_x, canvas_y) {

  var text = decodeURIComponent(city['name']);
  var size = city['size'];
  var color = nations[city_owner(city)['nation']]['color'];

  var txt_measure = pcanvas.measureText(text);
  var size_measure = pcanvas.measureText(size);

  pcanvas.fillStyle = "rgba(0, 0, 0, 0.5)";
  pcanvas.fillRect (canvas_x - Math.floor(txt_measure.width / 2) - 14, canvas_y - 17, 
                    txt_measure.width + 20, 20);

  pcanvas.fillStyle = color;
  pcanvas.fillRect (canvas_x + Math.floor(txt_measure.width / 2) + 5, canvas_y - 17, 
                    size_measure.width + 8, 20);

  pcanvas.fillStyle = "rgba(0, 0, 0, 1)";
  pcanvas.fillText(size, canvas_x + Math.floor(txt_measure.width / 2) + 10, canvas_y + 1);  

  pcanvas.fillStyle = "rgba(255, 255, 255, 1)";
  pcanvas.fillText(text, canvas_x - Math.floor(txt_measure.width / 2), canvas_y - 1);  

  pcanvas.fillText(size, canvas_x + Math.floor(txt_measure.width / 2) + 8, canvas_y - 1);  

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
  pcanvas.lineWidth = 2;

  pcanvas.lineCap = 'butt';
  if (dashedSupport) {
    pcanvas.setLineDash([4,4]);
  }

  pcanvas.beginPath();
  if (dir == DIR8_NORTH) {
    if (dashedSupport) {
      pcanvas.moveTo(x, y - 2, x + (tileset_tile_width / 2));
      pcanvas.lineTo(x + (tileset_tile_width / 2),  y + (tileset_tile_height / 2) - 2);
    }
  } else if (dir == DIR8_EAST) {
    if (dashedSupport) {
      pcanvas.moveTo(x - 3, y + tileset_tile_height - 3);
      pcanvas.lineTo(x + (tileset_tile_width / 2) - 3,  y + (tileset_tile_height / 2) - 3);
    }
  } else if (dir == DIR8_SOUTH) {
    if (dashedSupport) {
      pcanvas.moveTo(x - (tileset_tile_width / 2) + 3, y + (tileset_tile_height / 2) - 3);
      pcanvas.lineTo(x + 3,  y + tileset_tile_height - 3);
    }
  } else if (dir == DIR8_WEST) {
    if (dashedSupport) {
      pcanvas.moveTo(x - (tileset_tile_width / 2) + 3, y + (tileset_tile_height / 2) - 3);
      pcanvas.lineTo(x + 3,  y - 3);
    }
  }
  pcanvas.closePath();
  pcanvas.stroke();
  if (dashedSupport) {
    pcanvas.setLineDash([]);
  }
    
}

/**************************************************************************
...
**************************************************************************/
function mapview_put_goto_line(pcanvas, dir, canvas_x, canvas_y) {

  var x0 = canvas_x + (tileset_tile_width / 2);
  var y0 = canvas_y + (tileset_tile_height / 2);
  var x1 = x0 + GOTO_DIR_DX[dir] * (tileset_tile_width / 2);
  var y1 = y0 + GOTO_DIR_DY[dir] * (tileset_tile_height / 2);

  pcanvas.strokeStyle = '#f00';
  pcanvas.lineWidth = 2;
  pcanvas.beginPath(); 
  pcanvas.moveTo(x0, y0);
  pcanvas.lineTo(x1, y1);
  pcanvas.stroke();

}

/**************************************************************************
  Update the information label which gives info on the current unit and the
  square under the current unit, for specified unit.  Note that in practice
  punit is always the focus unit.
  Clears label if punit is NULL.
  Also updates the cursor for the map_canvas (this is related because the
  info label includes a "select destination" prompt etc).
  Also calls update_unit_pix_label() to update the icons for units on this
  square.
**************************************************************************/
function update_unit_info_label(punits)
{
  
  var unit_info_html = "";
  
  for (var i = 0; i < punits.length; i++) {
    var punit = punits[i];
    var ptype = unit_type(punit);
    var sprite = get_unit_image_sprite(punit);
    
    unit_info_html += "<div id='unit_info_div'><div id='unit_info_image' onclick='set_unit_focus_and_redraw(units[" + punit['id'] + "])' "
	   + " style='background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           + "'></div>";
    unit_info_html += "<div id='unit_text_details'><b>" + ptype['name'] + "</b><br>"
    if (get_unit_homecity_name(punit) != null) {
      unit_info_html += "<b>" + get_unit_homecity_name(punit) + "</b>";
      unit_info_html += "<br>";
    }
    if (punit['veteran'] > 0) {
      unit_info_html += "Veteran level: " + punit['veteran'] + "";
      unit_info_html += "<br>";
    }

    unit_info_html += get_unit_moves_left(punit) ;
    unit_info_html += "<br>";
    unit_info_html += "<div style='font-size: 80%;'><span title='Attack strength'>A:" + ptype['attack_strength'] 
    + "</span> <span title='Defense strength'>D:" + ptype['defense_strength'] 
    + "</span> <span title='Firepower'>F:" + ptype['firepower']  
    + "</span> <span title='Health points'>H:" 
    + ptype['hp'] + "</span>";
    unit_info_html += "</div></div></div>";
    
  }
  
  $("#game_unit_info").html(unit_info_html);
  
}


/**************************************************************************
  Update the information label which gives info on the current unit and the
  square under the current unit, for specified unit.  Note that in practice
  punit is always the focus unit.
  Clears label if punit is NULL.
  Also updates the cursor for the map_canvas (this is related because the
  info label includes a "select destination" prompt etc).
  Also calls update_unit_pix_label() to update the icons for units on this
  square.
**************************************************************************/
function update_select_unit_dialog(punits)
{
  var unit_info_html = "<div><b>Select a unit:</b></div>";
  
  for (var i = 0; i < punits.length; i++) {
    var punit = punits[i];
    
    var sprite = get_unit_image_sprite(punit);
    
    unit_info_html += "<div id='game_unit_list_item' style='cursor: pointer; background: transparent url(" 
    + sprite['image-src'] +
    ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
       + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
       + " onclick='set_unit_focus_and_activate(units[" + punit['id'] + "])'"
       +"></div>";
  }
  
  $("#game_unit_info").html(unit_info_html);
  
  $("#game_unit_orders_default").hide();
}

/**************************************************************************
  ...
**************************************************************************/
function set_city_mapview_active()
{
  city_canvas = document.getElementById('city_canvas');
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
function set_default_mapview_active()
{
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
  mapview_canvas_ctx.font = canvas_text_font;

  /* shows mapview elements */
  $("#tabs-map").show();
  $("#map_tab").addClass("ui-state-active");
  $("#map_tab").addClass("ui-tabs-selected");
  $("#tabs-hel").hide();

  update_map_canvas_full();
  chatbox_scroll_down();

  if (!is_small_screen()) {
    if (overview_active) {
      $("#game_overview_panel").parent().show();
      $(".overview_dialog").position({my: 'left bottom', at: 'left bottom', of: window});
    }
    if (unitpanel_active) {
      $("#game_unit_panel").parent().show();
      $(".unit_dialog").position({my: 'right bottom', at: 'right bottom', of: window});
    }
  }
  if (chatbox_active) $("#game_chatbox_panel").parent().show();

  $("#tabs").tabs("option", "active", 0);
  $("#tabs-map").height("auto");

  tech_dialog_active = false;
  allow_right_click = false;
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
