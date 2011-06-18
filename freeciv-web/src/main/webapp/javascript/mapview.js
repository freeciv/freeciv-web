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

var tileset_image = null; 
var sprites = {};

var sprites_init = false;
var mapview_slide = {};
mapview_slide['active'] = false;
mapview_slide['dx'] = 0;
mapview_slide['dy'] = 0;
mapview_slide['i'] = 0;
mapview_slide['max'] = 100; 
mapview_slide['slide_time'] = 800;

var height_offset = 67;
var height_offset_iphone = 110;
var width_offset = 10;

var font = "sans";  // without canvas text support
var canvas_text_font = "12pt Arial"; // with canvas text support

var fontsize = 12;

var has_canvas_text_support = false;

var fullfog = [];


/**************************************************************************
  ...
**************************************************************************/
function init_mapview()
{
 
  mapview_canvas = document.getElementById('canvas');
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
  buffer_canvas = document.createElement('canvas');
  buffer_canvas_ctx = buffer_canvas.getContext('2d');

  if ("mozImageSmoothingEnabled" in mapview_canvas_ctx) {
    // if this Boolean value is false, images won't be smoothed when scaled. This property is true by default.
    mapview_canvas_ctx.mozImageSmoothingEnabled = false;
  } 
  
  has_canvas_text_support = (mapview_canvas_ctx.fillText && mapview_canvas_ctx.measureText && !is_iphone());
  
  if (!has_canvas_text_support) {
    CanvasTextFunctions.enable(mapview_canvas_ctx);
  }
    
  
  if (is_iphone()) {
    height_offset = height_offset_iphone;
    width_offset = 0;
  }
     
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
  var winWidth, winHeight, d=document;
  if (typeof window.innerWidth!='undefined') {
    winWidth = window.innerWidth;
    winHeight = window.innerHeight;
  } else {
    if (d.documentElement &&
      typeof d.documentElement.clientWidth!='undefined' &&
      d.documentElement.clientWidth!=0) {
      winWidth = d.documentElement.clientWidth
      winHeight = d.documentElement.clientHeight
    } else {
      if (d.body &&
        typeof d.body.clientWidth!='undefined') {
        winWidth = d.body.clientWidth
        winHeight = d.body.clientHeight
      }
    }
  }
  
  mapview_canvas.width = winWidth - width_offset;
  mapview_canvas.height = winHeight - height_offset;

  mapview['width'] = winWidth - width_offset;
  mapview['height'] = winHeight - height_offset; 
  mapview['store_width'] = winWidth - width_offset;
  mapview['store_height'] = winHeight - height_offset;

  var twidth = $("#game_unit_orders_default").width();
  $("#game_unit_orders_default").css("left", Math.floor((mapview_canvas.width - twidth) / 2));
  $("#game_status_panel").css("width", mapview_canvas.width);


  $("#pregame_message_area").height( mapview['height'] - 50);
  $("#pregame_player_list").height( mapview['height'] - 80);
  $("#technologies").height( mapview['height'] - 100);
  $("#technologies").width( mapview['width'] - 20);
  
  $("#nations").height( mapview['height'] - 100);
  $("#nations").width( mapview['width']);
  
  $(".manual-tab").height( mapview['height'] - 200);
  
  $("#city_viewport").height( mapview['height'] - 20);


  if (overview_active) init_overview();
  if (unitpanel_active) init_game_unit_panel(); 
	
}

/**************************************************************************
  ...
**************************************************************************/
function init_sprites()
{
  /* Load tileset images. FIXME: loading images this way doesn't work in Opera. */
  tileset_image = new Image();
  tileset_image.src = '/tileset/freeciv-web-tileset.png';
  
}

/**************************************************************************
  ...
**************************************************************************/
function init_cache_sprites() 
{
 try {
  for (var tile_tag in tileset) {
    var x = tileset[tile_tag][0];
    var y = tileset[tile_tag][1];
    var w = tileset[tile_tag][2];
    var h = tileset[tile_tag][3];

    var newCanvas = document.createElement('canvas');
    newCanvas.height = "" + h;
    newCanvas.width = "" + w;
    var newCtx = newCanvas.getContext('2d');

    newCtx.drawImage(tileset_image, x, y, 
                       w, h, 0, 0, w, h);
    sprites[tile_tag] = newCanvas;

  }

  sprites_init = true;

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
function drawGotoLine(ctx, x1, y1, x2, y2)
{
    ctx.strokeStyle = '#f00';
    ctx.lineWidth = 2;
    ctx.beginPath(); 
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
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
function mapview_put_city_text(pcanvas, text, canvas_x, canvas_y) {

  if (has_canvas_text_support) {
    pcanvas.font = canvas_text_font;

    var width = pcanvas.measureText(text).width;
    pcanvas.fillStyle = "rgba(0, 0, 0, 0.5)";
    pcanvas.fillRect (canvas_x - Math.floor(width / 2) - 5, canvas_y - 14, width + 10, 20);
    pcanvas.fillStyle = "rgba(255, 255, 255, 1)";
    pcanvas.fillText(text, canvas_x - Math.floor(width / 2), canvas_y);  
  } else {
    var width = pcanvas.measureText(font, fontsize, text)
    pcanvas.fillStyle = "rgba(0, 0, 0, 0.5)";
    pcanvas.fillRect (canvas_x - Math.floor(width / 2) - 5, canvas_y - 14, width + 10, 20);
    
    pcanvas.strokeStyle = "rgba(255,255,255,1)";
    pcanvas.drawTextCenter ( font, fontsize, canvas_x, canvas_y, text );
    

    
    
  }
  
    
}

/**************************************************************************
  Draw unit activity text onto the canvas. (not in use?)
**************************************************************************/
function mapview_put_unit_text(pcanvas, text, canvas_x, canvas_y) {

  
  if (has_canvas_text_support) {
    pcanvas.font = canvas_text_font;
    pcanvas.fillStyle = "rgba(255, 200, 70, 1)";
    pcanvas.fillText(text, canvas_x, canvas_y);  
  } else {  
    pcanvas.strokeStyle = "rgba(255, 200, 70, 1)"
    pcanvas.drawTextCenter ( font, fontsize, canvas_x, canvas_y, text );
  }
  
    
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
    unit_info_html += get_unit_moves_left(punit) ;
    unit_info_html += "<br>";
    unit_info_html += "<div style='font-size: 80%;'><span title='Attack strength'>A:" + ptype['attack_strength'] 
    + "</span> <span title='Defense strength'>D:" + ptype['defense_strength'] 
    + "</span> <span title='Firepower'>F:" + ptype['firepower']  
    + "</span> <span title='Health points'>H:" 
    + ptype['hp'] + "</span>";
    unit_info_html += "</div><div id='turns_to_target'></div></div></div>";
    
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
  mapview_canvas = document.getElementById('city_canvas');
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
     
  if (!has_canvas_text_support) {
    CanvasTextFunctions.enable(mapview_canvas_ctx);
  }
    

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
  mapview_canvas = document.getElementById('canvas');
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
    
  if (!has_canvas_text_support) {
    CanvasTextFunctions.enable(mapview_canvas_ctx);
  }
    
 
  if (active_city != null) {
    setup_window_size (); 
    center_tile_mapcanvas(city_tile(active_city)); 
    active_city = null;
  }
  update_map_canvas_full();
  chatbox_scroll_down();
  if (overview_active) $("#game_overview_panel").parent().show();
  if (unitpanel_active) $("#game_unit_panel").parent().show();
  if (chatbox_active) $("#game_chatbox_panel").parent().show();

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
