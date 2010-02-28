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
var tileset_images = {}; 

var height_offset = 240;
var height_offset_iphone = 110;
var width_offset = 30;

var font = "sans";  // without canvas text support
var canvas_text_font = "12pt Arial"; // with canvas text support

var fontsize = 12;

var has_canvas_text_support = false;

var fullfog = [];


/**************************************************************************
  ...
**************************************************************************/
function initCanvas(canvas) {
        if (window.G_vmlCanvasManager && window.attachEvent && !window.opera) {
          canvas = window.G_vmlCanvasManager.initElement(canvas);
        }
        return canvas;
}


/**************************************************************************
  ...
**************************************************************************/
function init_mapview()
{
 
  if (is_canvas_supported()) {
    mapview_canvas = initCanvas(document.getElementById('canvas'));
    mapview_canvas_ctx = mapview_canvas.getContext("2d");
    
  
    has_canvas_text_support = (mapview_canvas_ctx.fillText && mapview_canvas_ctx.measureText && !is_iphone());
  
    if (!has_canvas_text_support) {
      CanvasTextFunctions.enable(mapview_canvas_ctx);
    }
    
  } else {
    mapview_canvas = document.getElementById('ie_canvas');
  }
     
  
  if (!is_canvas_supported()) {
    $("#canvas").remove();
  } else {
    $("#ie_canvas").remove();
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
  
  init_small_tiles();
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
  
  if (is_canvas_supported()) {
    mapview_canvas.width = winWidth - width_offset;
    mapview_canvas.height = winHeight - height_offset;
  } else {
    $("#ie_canvas").width(winWidth - width_offset);
    $("#ie_canvas").height(winHeight - height_offset);
  }
  
  mapview['width'] = winWidth - width_offset;
  mapview['height'] = winHeight - height_offset; 
  mapview['store_width'] = winWidth - width_offset;
  mapview['store_height'] = winHeight - height_offset;
}

/**************************************************************************
  ...
**************************************************************************/
function init_small_tiles()
{
  if (is_canvas_clipping_supported()) {
    /* Load tileset images. FIXME: loading images this way doesn't work in Opera. */
    var img1 = new Image();
    img1.src = '/tileset/freeciv-web-tileset-1.png';
    tileset_images[1] = img1; 
  
    var img2 = new Image();
    img2.src = '/tileset/freeciv-web-tileset-2.png';
    tileset_images[2] = img2;
  } else {
    for (var tile_tag in tileset) {
      var imgx = new Image();
      imgx.src = '/tiles/' + tile_tag + '.png';
      tileset_images[tile_tag] = imgx; 
    }
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
  if (tileset[tag] == null) {
    console.log("Missing in tileset " + tag);
    return;
  } 

  var tileset_file_no = tileset[tag][0];
  var tileset_x = tileset[tag][1];
  var tileset_y = tileset[tag][2];
  var width = tileset[tag][3];
  var height = tileset[tag][4];
 
  if (is_canvas_clipping_supported()) {
    pcanvas.drawImage(tileset_images[tileset_file_no], tileset_x, tileset_y, width, height,
                      canvas_x, canvas_y, width, height);
  } else {
    pcanvas.drawImage(tileset_images[tag], canvas_x, canvas_y);
  }
             
}

/**************************************************************************
  ...
**************************************************************************/
function mapview_put_tile_ie(pcanvas, tag, canvas_x, canvas_y) {
  if (tileset[tag] == null) {
    console.log("Missing in tileset " + tag);
    return;
  } 

  var tileset_file_no = tileset[tag][0];
  var tileset_x = tileset[tag][1];
  var tileset_y = tileset[tag][2];
  var width = tileset[tag][3];
  var height = tileset[tag][4];

  var xtile = document.createElement("div");
  xtile.style.position = "absolute";
  xtile.style.left = canvas_x+"px";
  xtile.style.top = canvas_y+"px";
  xtile.style.backgroundImage = "url(/tileset/freeciv-web-tileset-" + tileset_file_no + ".png)";
  xtile.style.width = width+"px";
  xtile.style.height = height+"px";
  xtile.style.backgroundPosition = "-" + tileset_x + "px -" + tileset_y + "px"; 
  
  mapview_frag.appendChild(xtile);   
              
}

/**************************************************************************
  Draw city text onto the canvas.
**************************************************************************/
function mapview_put_city_text(pcanvas, text, canvas_x, canvas_y) {

  if (!is_canvas_supported()) {
    var xtile = document.createElement("div");
    xtile.style.position = "absolute";
    xtile.style.left = canvas_x-20+"px";
    xtile.style.top = canvas_y-20+"px";
    xtile.appendChild(document.createTextNode(text));
    mapview_frag.appendChild(xtile);  
  
  } else if (has_canvas_text_support) {
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
    
    unit_info_html += "<div style='float:left;'><div style='float: left; padding-right: 10px; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + "'></div>";
    unit_info_html += "<div style='float:left;'><b>" + ptype['name'] + "</b><br>"
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
    
    unit_info_html += "<div id='game_unit_list_item' style='cursor: hand; background: transparent url(" 
    + sprite['image-src'] +
    ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
       + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
       + " onclick='set_unit_focus_and_redraw(units[" + punit['id'] + "])'"
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
  if (is_canvas_supported()) {
    mapview_canvas = initCanvas(document.getElementById('city_canvas'));
    mapview_canvas_ctx = mapview_canvas.getContext("2d");
     
    if (!has_canvas_text_support) {
      CanvasTextFunctions.enable(mapview_canvas_ctx);
    }
    
  } else {
    mapview_canvas = document.getElementById('city_ie_canvas');
  }
     
  
  if (!is_canvas_supported()) {
    $("#city_canvas").remove();
  } else {
    $("#city_ie_canvas").remove();
  }

  mapview['width'] = 300;
  mapview['height'] = 150; 
  mapview['store_width'] = 300;
  mapview['store_height'] = 150;

}


/**************************************************************************
  ...
**************************************************************************/
function set_default_mapview_active()
{
  if (is_canvas_supported()) {
    mapview_canvas = initCanvas(document.getElementById('canvas'));
    mapview_canvas_ctx = mapview_canvas.getContext("2d");
     
    if (!has_canvas_text_support) {
      CanvasTextFunctions.enable(mapview_canvas_ctx);
    }
    
  } else {
    mapview_canvas = document.getElementById('ie_canvas');
  }
     
  
  if (!is_canvas_supported()) {
    $("#canvas").remove();
  } else {
    $("#ie_canvas").remove();
  }
 
  if (active_city != null) {
    setup_window_size (); 
    center_tile_mapcanvas(city_tile(active_city)); 
    active_city = null;
  }
  update_map_canvas_full();
}