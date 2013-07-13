/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas RÃ¸sdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

var OVERVIEW_TILE_SIZE = 1;
var overviewTimerId = -1;
var overview_w = 230;
var overview_h = 140;

var OVERVIEW_REFRESH = 4500;

var palette = [];
var palette_size = 0;

var overview_active = false;

var COLOR_OVERVIEW_UNKNOWN = 0; /* Black */
var COLOR_OVERVIEW_MY_CITY = 1; /* white */
var COLOR_OVERVIEW_ALLIED_CITY = 2;
var COLOR_OVERVIEW_ENEMY_CITY = 3; /* cyan */
var COLOR_OVERVIEW_MY_UNIT = 4; /* yellow */
var COLOR_OVERVIEW_ALLIED_UNIT = 5;
var COLOR_OVERVIEW_ENEMY_UNIT = 6; /* red */
var COLOR_OVERVIEW_OCEAN = 7; /* ocean/blue */
var COLOR_OVERVIEW_LAND = 8; /* ground/green */
var COLOR_OVERVIEW_VIEWRECT = 9; /* white */


/****************************************************************************
  Initilaize the overview map.
****************************************************************************/
function init_overview()
{

  var xfact = 2;
  
  OVERVIEW_TILE_SIZE = Math.min(Math.floor((overview_w - 1) / (map['xsize'] * xfact) + 1) + 1,
			                    Math.floor((overview_h - 1) / map['ysize'] + 1) + 1);
  OVERVIEW_TILE_SIZE = Math.max(OVERVIEW_TILE_SIZE, 1);

  overview_active = true;

  $("#game_overview_panel").attr("title", "Overview map");		
  $("#game_overview_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "auto",
			height: "auto",
			resizable: false,
			dialogClass: 'overview_dialog',
			position: ["left", "bottom"],
			close: function(event, ui) { overview_active = false;}
		});
	
  $("#game_overview_panel").dialog('open');		
  $(".overview_dialog div.ui-dialog-titlebar").css("height", "5px");
  $(".overview_dialog div.ui-dialog-content").css("padding", "5px 0");
  $("#ui-dialog-title-game_overview_panel").css("margin-top", "-5px");
  $("#ui-dialog-title-game_overview_panel").css("font-size", "10px");
  $("#game_overview_panel").parent().css("overflow", "hidden");		
	  

  palette = generate_palette();
  
  redraw_overview();
  
  if (overviewTimerId == -1) {
    overviewTimerId = setInterval("redraw_overview()", OVERVIEW_REFRESH);
  }

  $("#game_overview_map").click(function(e) {
    var x = e.pageX - $(this).offset().left;
    var y = e.pageY - $(this).offset().top;
    overview_clicked (x, y)
  });

  palette_size = Object.keys(players).length;


}

/****************************************************************************
  Redraw the overview map.
****************************************************************************/
function redraw_overview()
{
  if (mapview_slide['active']) return;

  bmp_lib.render('overview_map', 
                  generate_overview_grid(map['xsize'], map['ysize']), 
                  palette);
}


/****************************************************************************
  Creates a grid representing the image for the overview map.
****************************************************************************/
function generate_overview_grid(cols, rows) {
  // Loop variables
  var row, col;

  // The grid of points that make up the image.
  var grid = Array(rows*OVERVIEW_TILE_SIZE);
  for (row = 0; row < rows * OVERVIEW_TILE_SIZE; row++) {
    grid[row] = Array(cols * OVERVIEW_TILE_SIZE);
  }

  for (var x = 0; x < rows ; x++) {
    for (var y = 0; y < cols; y++) {
      var ocolor = overview_tile_color(y, x);
      render_multipixel(grid, x, y, ocolor)
    }
  }

  render_viewrect(grid);

  return grid;
}

/****************************************************************************
  ...
****************************************************************************/
function render_viewrect(grid)
{
  var r;
  
  if (mapview['gui_x0'] != 0 && mapview['gui_y0'] != 0) { 
  
    r = base_canvas_to_map_pos(0, 0);
    s = base_canvas_to_map_pos(mapview['width'], 0);
    t = base_canvas_to_map_pos(0, mapview['height']);
    u = base_canvas_to_map_pos(mapview['width'], mapview['height']);

    drawLine(grid, r['map_y'], r['map_x'], s['map_y'], s['map_x']);
    drawLine(grid, s['map_y'], s['map_x'], u['map_y'], u['map_x']);
    drawLine(grid, t['map_y'], t['map_x'], u['map_y'], u['map_x']);
    drawLine(grid, r['map_y'], r['map_x'], t['map_y'], t['map_x']);                     

  }  
}

/****************************************************************************
  ...
****************************************************************************/
function drawLine(grid, Sx, Sy, Rx, Ry)
{
  var Ax = Sx * OVERVIEW_TILE_SIZE;
  var Ay = Sy * OVERVIEW_TILE_SIZE;
  var Bx = Rx * OVERVIEW_TILE_SIZE;
  var By = Ry * OVERVIEW_TILE_SIZE; 

  var lineLength = Math.sqrt( (Ax-Bx)*(Ax-Bx)+(Ay-By)*(Ay-By));
  for( var i=0; i<lineLength; i++ ) {
    var x = Math.round( Ax+(Bx-Ax)*i/lineLength);
    var y = Math.round( Ay+(By-Ay)*i/lineLength);
    render_singlepixel(grid, x, y, COLOR_OVERVIEW_VIEWRECT);
  }
}

/****************************************************************************
  ...
****************************************************************************/
function render_multipixel(grid, x, y, ocolor)
{
  if (x >= 0 && y >= 0 && x < map['ysize'] && y < map['xsize']) {
    for (var px = 0; px < OVERVIEW_TILE_SIZE; px++) {
      for (var py = 0; py < OVERVIEW_TILE_SIZE; py++) {
          grid[(OVERVIEW_TILE_SIZE*x)+px][(OVERVIEW_TILE_SIZE*y)+py] = ocolor;       
      }
    }
  }
}        

/****************************************************************************
  ...
****************************************************************************/
function render_singlepixel(grid, x, y, ocolor)
{
  if (x >= 0 && y >= 0 && x < (map['ysize'] * OVERVIEW_TILE_SIZE) 
      && y < (map['xsize'] * OVERVIEW_TILE_SIZE)) {
    grid[x][y] = ocolor;       
  }
}  

/****************************************************************************
  Creates the palette for the overview map.
****************************************************************************/
function generate_palette() {
  var palette = [];
  palette[COLOR_OVERVIEW_UNKNOWN] = [0,0,0];
  palette[COLOR_OVERVIEW_MY_CITY] = [255,255,255];
  palette[COLOR_OVERVIEW_ALLIED_CITY] = [0,255,255];
  palette[COLOR_OVERVIEW_ENEMY_CITY] = [0,255,255];  
  palette[COLOR_OVERVIEW_MY_UNIT] = [255,255,0];  
  palette[COLOR_OVERVIEW_ALLIED_UNIT] = [255,0,0];  
  palette[COLOR_OVERVIEW_ENEMY_UNIT] = [255,0,0];
  palette[COLOR_OVERVIEW_OCEAN] = [30,65,195];  
  palette[COLOR_OVERVIEW_LAND] = [40,114,22];
  palette[COLOR_OVERVIEW_VIEWRECT] = [200,200,255];  

  for (var player_id in players) {
    var pplayer = players[player_id];
    var pcolor = nations[pplayer['nation']]['color'];
    var color_rgb = pcolor.match(/\d+/g);
    color_rgb[0] = parseFloat(color_rgb[0]);
    color_rgb[1] = parseFloat(color_rgb[1]);
    color_rgb[2] = parseFloat(color_rgb[2]);
    palette[10+(player_id % palette_size)] = color_rgb;
  }
  return palette;
}

/****************************************************************************
  Returns the color of the tile at the given map position.
****************************************************************************/
function overview_tile_color(map_x, map_y)
{
  var ptile = map_pos_to_tile(map_x, map_y);
  
  var pcity = tile_city(ptile);
  
  if (pcity != null) {
    if (client.conn.playing == null) {
      return COLOR_OVERVIEW_ENEMY_CITY;
    } else if (city_owner_player_id(pcity) == client.conn.playing['id']) {
      return COLOR_OVERVIEW_MY_CITY;
    } else {
      return COLOR_OVERVIEW_ENEMY_CITY;
    } 
  } 
  
  var punit = find_visible_unit(ptile);
  if (punit != null) {
    if (client.conn.playing == null) {
      return COLOR_OVERVIEW_ENEMY_UNIT;
    } else if (punit['owner'] == client.conn.playing['id']) {
      return COLOR_OVERVIEW_MY_UNIT; 
    } else if (punit['owner'] != null && punit['owner'] != 255) {
      return 10 + (punit['owner'] % palette_size);
    } else {
      return COLOR_OVERVIEW_ENEMY_UNIT;
    }
  }
  
  if (tile_get_known(ptile) != TILE_UNKNOWN) {

    if (ptile['owner'] != null && ptile['owner'] != 255) {
      return 10 + (ptile['owner'] % palette_size);
    }

    if (is_ocean_tile(ptile)) {
      return COLOR_OVERVIEW_OCEAN;
    } else {
      return COLOR_OVERVIEW_LAND;
    }
  }
  
  return COLOR_OVERVIEW_UNKNOWN;
  
}


/****************************************************************************
  ...
****************************************************************************/
function overview_clicked (x, y)
{
  var width = $("#overview_map").width();
  var height = $("#overview_map").height();

  var x1 = Math.floor((x * map['xsize']) / width);
  var y1 = Math.floor((y * map['ysize']) / height);
  
  var ptile = map_pos_to_tile(x1, y1);  
  if (ptile != null) {
    center_tile_mapcanvas(ptile);
  }
  
  redraw_overview();
}
