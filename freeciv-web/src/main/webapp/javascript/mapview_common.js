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


var mapview = {};
var mapdeco_highlight_table = {};
var mapdeco_crosshair_table = {};
var mapdeco_gotoline_table = {};
var last_redraw_time = 0;
var MAPVIEW_REFRESH_INTERVAL = 10;


function mapdeco_init()
{
  mapdeco_highlight_table = {};
  mapdeco_crosshair_table = {};
  mapdeco_gotoline_table = {};

  init_game_unit_panel();
  init_chatbox();
  keyboard_input=true;


}

/**************************************************************************
  Centers the mapview around (map_x, map_y).
**************************************************************************/
function center_tile_mapcanvas(ptile)
{
  var r = map_to_gui_pos(ptile['x'], ptile['y']);
  var gui_x = r['gui_dx'];
  var gui_y = r['gui_dy'];
  
  gui_x -= (mapview['width'] - tileset_tile_width) >> 1 ;
  gui_y -= (mapview['height'] - tileset_tile_height) >> 1;
  
  set_mapview_origin(gui_x, gui_y);
  
}

/****************************************************************************
  Translate from a cartesian system to the GUI system.  This function works
  on vectors, meaning it can be passed a (dx,dy) pair and will return the
  change in GUI coordinates corresponding to this vector.  It is thus more
  general than map_to_gui_pos.

  Note that a gui_to_map_vector function is not possible, since the
  resulting map vector may differ based on the origin of the gui vector.
  Note that is function is for isometric tilesets only.
****************************************************************************/
function map_to_gui_vector(map_dx, map_dy)
{
    /*
     * Convert the map coordinates to isometric GUI
     * coordinates.  We'll make tile map(0,0) be the origin, and
     * transform like this:
     * 
     *                     3
     * 123                2 6
     * 456 -> becomes -> 1 5 9
     * 789                4 8
     *                     7
     */

    var gui_dx = ((map_dx - map_dy) * tileset_tile_width) >> 1;
    var gui_dy = ((map_dx + map_dy) * tileset_tile_height) >> 1;
    return {'gui_dx' : gui_dx, 'gui_dy' : gui_dy};

}

/****************************************************************************
  Change the mapview origin, clip it, and update everything.
****************************************************************************/
function set_mapview_origin(gui_x0, gui_y0)
{
  var xmin, xmax, ymin, ymax, xsize, ysize;
  
  /* Normalize (wrap) the mapview origin. */
  var r = normalize_gui_pos(gui_x0, gui_y0);
  gui_x0 = r['gui_x'];
  gui_y0 = r['gui_y'];
  
  /* TODO: add support for mapview scrolling here. */
  
  base_set_mapview_origin(gui_x0, gui_y0);
  
}

/****************************************************************************
  Move the GUI origin to the given normalized, clipped origin.  This may
  be called many times when sliding the mapview.
****************************************************************************/
function base_set_mapview_origin(gui_x0, gui_y0)
{
  /* We need to calculate the vector of movement of the mapview.  So
   * we find the GUI distance vector and then use this to calculate
   * the original mapview origin relative to the current position.  Thus
   * if we move one tile to the left, even if this causes GUI positions
   * to wrap the distance vector is only one tile. */
  var g = normalize_gui_pos(gui_x0, gui_y0);
  gui_x0 = g['gui_x'];
  gui_y0 = g['gui_y'];
  
  mapview['gui_x0'] = gui_x0;
  mapview['gui_y0'] = gui_y0;
        
}

/****************************************************************************
  Normalize (wrap) the GUI position.  This is equivalent to a map wrapping,
  but in GUI coordinates so that pixel accuracy is preserved.
****************************************************************************/
function normalize_gui_pos(gui_x, gui_y)
{
  var map_x, map_y, nat_x, nat_y, gui_x0, gui_y0, diff_x, diff_y;
  
  /* Convert the (gui_x, gui_y) into a (map_x, map_y) plus a GUI offset
   * from this tile. */
  var r = gui_to_map_pos(gui_x, gui_y);
  map_x = r['map_x'];
  map_y = r['map_y'];

  
  var s = map_to_gui_pos(map_x, map_y);
  gui_x0 = s['gui_dx'];
  gui_y0 = s['gui_dy'];
  
  diff_x = gui_x - gui_x0;
  diff_y = gui_y - gui_y0;
  
  
  /* Perform wrapping without any realness check.  It's important that
   * we wrap even if the map position is unreal, which normalize_map_pos
   * doesn't necessarily do. */
  var t = MAP_TO_NATIVE_POS(map_x, map_y);
  nat_x = t['nat_x'];
  nat_y = t['nat_y'];
  
  if (topo_has_flag(TF_WRAPX)) {
    nat_x = FC_WRAP(nat_x, map['xsize']);
  }
  if (topo_has_flag(TF_WRAPY)) {
    nat_y = FC_WRAP(nat_y, map['ysize']);
  }
  
  var u = NATIVE_TO_MAP_POS(nat_x, nat_y);
  map_x = u['map_x'];
  map_y = u['map_y']; 

  /* Now convert the wrapped map position back to a GUI position and add the
   * offset back on. */
  var v = map_to_gui_pos(map_x, map_y);
  gui_x = v['gui_dx'];
  gui_y = v['gui_dy'];

  gui_x += diff_x;
  gui_y += diff_y;

  return {'gui_x' : gui_x, 'gui_y' : gui_y}; 
}

/****************************************************************************
  Translate from gui to map coordinate systems.  See map_to_gui_pos().

  Note that you lose some information in this conversion.  If you convert
  from a gui position to a map position and back, you will probably not get
  the same value you started with.
****************************************************************************/
function gui_to_map_pos(gui_x, gui_y)
{

    var W = tileset_tile_width;
    var H = tileset_tile_height;

    /* The basic operation here is a simple pi/4 rotation; however, we
     * have to first scale because the tiles have different width and
     * height.  Mathematically, this looks like
     *   | 1/W  1/H | |x|    |x`|
     *   |          | | | -> |  |
     *   |-1/W  1/H | |y|    |y`|
     *
     * Where W is the tile width and H the height.
     *
     * In simple terms, this is
     *   map_x = [   x / W + y / H ]
     *   map_y = [ - x / W + y / H ]
     * where [q] stands for integer part of q.
     *
     * Here the division is proper mathematical floating point division.
     *
     * A picture demonstrating this can be seen at
     * http://bugs.freeciv.org/Ticket/Attachment/16782/9982/grid1.png.
     *
     * We have to subtract off a half-tile in the X direction before doing
     * the transformation.  This is because, although the origin of the tile
     * is the top-left corner of the bounding box, after the transformation
     * the top corner of the diamond-shaped tile moves into this position.
     *
     * The calculation is complicated somewhat because of two things: we
     * only use integer math, and C integer division rounds toward zero
     * instead of rounding down.
     *
     * For another example of this math, see canvas_to_city_pos().
     */
    
    gui_x -= W >> 1;
    var map_x = DIVIDE(gui_x * H + gui_y * W, W * H);
    var map_y = DIVIDE(gui_y * W - gui_x * H, W * H);

    return {'map_x' : map_x, 'map_y' : map_y};


}


/****************************************************************************
  Translate from map to gui coordinate systems.

  GUI coordinates are comparable to canvas coordinates but extend in all
  directions.  gui(0,0) == map(0,0).
****************************************************************************/
function map_to_gui_pos(map_x, map_y)
{
  /* Since the GUI origin is the same as the map origin we can just do a
   * vector conversion. */
  return map_to_gui_vector(map_x, map_y);
}


/**************************************************************************
  Update (refresh) the map canvas starting at the given tile (in map
  coordinates) and with the given dimensions (also in map coordinates).

  In non-iso view, this is easy.  In iso view, we have to use the
  Painter's Algorithm to draw the tiles in back first.  When we draw
  a tile, we tell the GUI which part of the tile to draw - which is
  necessary unless we have an extra buffering step.

  After refreshing the backing store tile-by-tile, we write the store
  out to the display if write_to_screen is specified.

  x, y, width, and height are in map coordinates; they need not be
  normalized or even real.
**************************************************************************/
function update_map_canvas(canvas_x, canvas_y, width, height)
{
  var gui_x0, gui_y0;
  var full = true;

  gui_x0 = mapview['gui_x0'] + canvas_x;
  gui_y0 = mapview['gui_y0'] + canvas_y;

  /* Clear the area, if the mapview extends beyond map borders.  
   *
   * This is necessary since some parts of the rectangle
   * may not actually have any tiles drawn on them.  This will happen when
   * the mapview is large enough so that the tile is visible in multiple
   * locations.  In this case it will only be drawn in one place.
   *
   * Of course it's necessary to draw to the whole area to cover up any old
   * drawing that was done there. */
    r = base_canvas_to_map_pos(0, 0);
    s = base_canvas_to_map_pos(mapview['width'], 0);
    t = base_canvas_to_map_pos(0, mapview['height']);
    u = base_canvas_to_map_pos(mapview['width'], mapview['height']);
    if (    r['map_x'] < 0 || r['map_x'] > map['xsize'] || r['map_y'] < 0 || r['map_y'] > map['ysize']
	 || s['map_x'] < 0 || s['map_x'] > map['xsize'] || s['map_y'] < 0 || s['map_y'] > map['ysize']
	 || t['map_x'] < 0 || t['map_x'] > map['xsize'] || t['map_y'] < 0 || t['map_y'] > map['ysize']
	 || u['map_x'] < 0 || u['map_x'] > map['xsize'] || u['map_y'] < 0 || u['map_y'] > map['ysize']) {
      canvas_put_rectangle(mapview_canvas_ctx, "rgb(0,0,0)", canvas_x, canvas_y, width, height);
    }
  
  // mapview_layer_iterate
  for (var layer = 0; layer < LAYER_COUNT; layer++) {
  
    //gui_rect_iterate begin   
    var gui_x_0 = (gui_x0);
    var gui_y_0 = (gui_y0); 
    var gui_x_w = width + (tileset_tile_width >> 1);
    var gui_y_h = height + (tileset_tile_height >> 1);
    if (gui_x_w < 0) { 
      gui_x_0 += gui_x_w; 
      gui_x_w = -gui_x_w; 
    } 
    
    if (gui_y_h < 0) { 
      gui_y_0 += gui_y_h; 
      gui_y_h = -gui_y_h; 
    } 
    
    if (gui_x_w > 0 && gui_y_h > 0) { 
      var ptilepedge = {}; 
      var ptilepcorner = {}; 
      var ptile_xi, ptile_yi, ptile_si, ptile_di; 
      var gui_x, gui_y; 
      var ptile_r1 = 2;
      var ptile_r2 = ptile_r1 * 2; 
      var ptile_w = tileset_tile_width; 
      var ptile_h = tileset_tile_height; 
      var ptile_x0 = Math.floor(( (gui_x_0 * ptile_r2) / (ptile_w) - (( (gui_x_0 * ptile_r2) < 0 && (gui_x_0 * ptile_r2) % (ptile_w) < 0 ) ? 1 : 0) ) - ptile_r1 / 2);
      var ptile_y0 = Math.floor(( (gui_y_0 * ptile_r2) / (ptile_h) - (( (gui_y_0 * ptile_r2) < 0 && (gui_y_0 * ptile_r2) % (ptile_h) < 0 ) ? 1 : 0) ) - ptile_r1 / 2); 
      var ptile_x1 = Math.floor(( ((gui_x_0 + gui_x_w) * ptile_r2 + ptile_w - 1) / (ptile_w) - (( ((gui_x_0 + gui_x_w) * ptile_r2 + ptile_w - 1) < 0 
                       && ((gui_x_0 + gui_x_w) * ptile_r2 + ptile_w - 1) % (ptile_w) < 0 ) ? 1 : 0) ) + ptile_r1);
      var ptile_y1 = Math.floor(( ((gui_y_0 + gui_y_h) * ptile_r2 + ptile_h - 1) / (ptile_h) - (( ((gui_y_0 + gui_y_h) * ptile_r2 + ptile_h - 1) < 0 
                       && ((gui_y_0 + gui_y_h) * ptile_r2 + ptile_h - 1) % (ptile_h) < 0 ) ? 1 : 0) ) + ptile_r1); 
      var ptile_count = (ptile_x1 - ptile_x0) * (ptile_y1 - ptile_y0);
      
      for (var ptile_index = 0; ptile_index < ptile_count; ptile_index++) { 
        var ptile = null; 
        var pedge = null; 
        var pcorner = null; 
        ptile_xi = ptile_x0 + (ptile_index % (ptile_x1 - ptile_x0)); 
        ptile_yi = Math.floor(ptile_y0 + (ptile_index / (ptile_x1 - ptile_x0)));
        ptile_si = ptile_xi + ptile_yi; 
        ptile_di = ptile_yi - ptile_xi;
        if (2 == ptile_r1 ) { 
          if ((ptile_xi + ptile_yi) % 2 != 0) { 
            continue; 
          } 
          if (ptile_xi % 2 == 0 && ptile_yi % 2 == 0) { 
            if ((ptile_xi + ptile_yi) % 4 == 0) { 
              /* Tile */
              ptile = map_pos_to_tile((ptile_si >> 2) - 1, (ptile_di >> 2));
            } else { 
              /* Corner */
              pcorner = ptilepcorner;
              pcorner['tile'] = []; 
              pcorner['tile'][0] = map_pos_to_tile(((ptile_si - 6) >> 2), ((ptile_di - 2) >> 2)); 
              pcorner['tile'][1] = map_pos_to_tile(((ptile_si - 2) >> 2), ((ptile_di - 2) >> 2)); 
              pcorner['tile'][2] = map_pos_to_tile(((ptile_si - 2) >> 2), ((ptile_di + 2) >> 2)); 
              pcorner['tile'][3] = map_pos_to_tile(((ptile_si - 6) >> 2), ((ptile_di + 2) >> 2));              
            } 
          } else { 
            /* Edge. */
            pedge = ptilepedge; 
            if (ptile_si % 4 == 0) { 
              pedge['type'] = EDGE_NS;
              pedge['tile'] = [];
              pedge['tile'][0] = map_pos_to_tile(((ptile_si - 4) >> 2), ((ptile_di - 2) >> 2)); 
              pedge['tile'][1] = map_pos_to_tile(((ptile_si - 4) >> 2), ((ptile_di + 2) >> 2));
            } else { 
              pedge['type'] = EDGE_WE;
              pedge['tile'] = [];
              pedge['tile'][0] = map_pos_to_tile(((ptile_si - 6) >> 2), (ptile_di >> 2)); 
              pedge['tile'][1] = map_pos_to_tile(((ptile_si - 2) >> 2), (ptile_di >> 2));
            } 
          } 
        } else { 
          if (ptile_si % 2 == 0) { 
            if (ptile_xi % 2 == 0) { 
              /* Corner */
              pcorner = ptilepcorner;
              pcorner['tile'] = []; 
              pcorner['tile'][0] = map_pos_to_tile((ptile_xi >> 1) - 1, (ptile_yi >> 1) - 1); 
              pcorner['tile'][1] = map_pos_to_tile((ptile_xi >> 1), (ptile_yi >> 1) - 1); 
              pcorner['tile'][2] = map_pos_to_tile((ptile_xi >> 1), (ptile_yi >> 1));
              pcorner['tile'][3] = map_pos_to_tile((ptile_xi >> 1) - 1, (ptile_yi >> 1));
            } else { 
              /* Tile */
              ptile = map_pos_to_tile(((ptile_xi - 1) >> 1), ((ptile_yi - 1) >> 1)); 
            } 
          } else {
            /* Edge */ 
            pedge = ptilepedge; 
            if (ptile_yi % 2 == 0) { 
              pedge['type'] = EDGE_NS;
              pedge['tile'] = [];
              pedge['tile'][0] = map_pos_to_tile(((ptile_xi - 1) >> 1), ((ptile_yi >> 1) - 1));
              pedge['tile'][1] = map_pos_to_tile(((ptile_xi - 1) >> 1), (ptile_yi >> 1));
            } else { 
              pedge['type'] = EDGE_WE; 
              pedge['tile'] = [];
              pedge['tile'][0] = map_pos_to_tile(((ptile_xi >> 1) - 1), ((ptile_yi - 1) >> 1)); 
              pedge['tile'][1] = map_pos_to_tile((ptile_xi >> 1), ((ptile_yi - 1) >> 1));
            } 
          } 
        } 

        gui_x = Math.floor(ptile_xi * ptile_w / ptile_r2 - ptile_w / 2); 
        gui_y = Math.floor(ptile_yi * ptile_h / ptile_r2 - ptile_h / 2);
         
        { // gui_rect_iterate begin_end

          var cx = gui_x - mapview['gui_x0'];
          var cy = gui_y - mapview['gui_y0'];


          if (ptile != null) {
            put_one_tile(mapview_canvas_ctx, layer, ptile, cx, cy, null);
            
          } else if (pedge != null) {
            put_one_element(mapview_canvas_ctx, layer, null, pedge, null,
                            null, null, cx, cy, null);
          } else if (pcorner != null) {
            put_one_element(mapview_canvas_ctx, layer, null, null, pcorner,
                            null, null, cx, cy, null);
          } else {
            /* This can happen, for instance for unreal tiles. */
          }
        }
      } 
    } 
  }
 
}


/**************************************************************************
  Draw some or all of a tile onto the canvas.
**************************************************************************/
function put_one_tile(pcanvas, layer, ptile, canvas_x, canvas_y, citymode)
{
  if (tile_get_known(ptile) != TILE_UNKNOWN) {
    put_one_element(pcanvas, layer, ptile, null, null,
		    get_drawable_unit(ptile, citymode),
		    tile_city(ptile), canvas_x, canvas_y, citymode);
  }
}


/**************************************************************************
  Draw one layer of a tile, edge, corner, unit, and/or city onto the
  canvas at the given position.
**************************************************************************/
function put_one_element(pcanvas, layer, ptile, pedge, pcorner, punit, 
                         pcity, canvas_x, canvas_y, citymode)
{

  var tile_sprs = fill_sprite_array(layer, ptile, pedge, pcorner, punit, pcity, citymode);
				                    
  var fog = (ptile != null && draw_fog_of_war
	      && TILE_KNOWN_UNSEEN == tile_get_known(ptile));				                    

  put_drawn_sprites(pcanvas, canvas_x, canvas_y, tile_sprs, fog);
   
  
}



/**************************************************************************
  Draw an array of drawn sprites onto the canvas.
**************************************************************************/
function put_drawn_sprites(pcanvas, canvas_x, canvas_y, pdrawn, fog)
{ 
  for (var i = 0; i < pdrawn.length; i++) {
    var offset_x = 0, offset_y = 0;
    if ('offset_x' in pdrawn[i]) {
      offset_x += pdrawn[i]['offset_x'];
    }
    if ('offset_y' in pdrawn[i]) {
      offset_y += pdrawn[i]['offset_y'];
    }
    if (pdrawn[i]['key'] == "city_text" ) {
      mapview_put_city_text(pcanvas, pdrawn[i]['text'], canvas_x + offset_x, canvas_y + offset_y);
      
    } else if (pdrawn[i]['key'] == "unit_activity_text" ) {
      mapview_put_unit_text(pcanvas, pdrawn[i]['text'], canvas_x + offset_x, canvas_y + offset_y);        
    } else if (pdrawn[i]['key'] == "border" ) {
      mapview_put_border_line(pcanvas, pdrawn[i]['dir'], pdrawn[i]['color'], canvas_x, canvas_y);
    } else {
      mapview_put_tile(pcanvas, pdrawn[i]['key'], canvas_x + offset_x, canvas_y + offset_y);

    }    
  }
  
}


/****************************************************************************
  Finds the map coordinates corresponding to pixel coordinates.  The
  resulting position is unwrapped and may be unreal.
****************************************************************************/
function base_canvas_to_map_pos(canvas_x, canvas_y)
{
  return gui_to_map_pos(canvas_x + mapview.gui_x0,
                        canvas_y + mapview.gui_y0);		 
}

/**************************************************************************
  Finds the tile corresponding to pixel coordinates.  Returns that tile,
  or NULL if the position is off the map.
**************************************************************************/
function canvas_pos_to_tile(canvas_x, canvas_y)
{
  var map_x, map_y;

  var r = base_canvas_to_map_pos(canvas_x, canvas_y);
  map_x = r['map_x'];
  map_y = r['map_y'];
  /*FIXME: if (normalize_map_pos(&map_x, &map_y)) {
    return map_pos_to_tile(map_x, map_y);
  } else {
    return null;
  }*/
  return map_pos_to_tile(map_x, map_y);
}

/**************************************************************************
  Updates the entire mapview.
**************************************************************************/
function update_map_canvas_full()
{
  if (tiles != null && civclient_state >= C_S_RUNNING) {
    //var start = new Date().getTime();
    if (!sprites_init) init_cache_sprites();
  
    // If city dialog is open, don't redraw default mapview.
    if (active_city != null) return;
    
    if (!mapview_slide['active']) { 
      update_map_canvas(0, 0, mapview['store_width'], mapview['store_height']);
      update_goto_path_lines(); 
      check_request_goto_path();
    } else {
      update_map_slide();
    }

    last_redraw_time = new Date().getTime();
    
  }
} 

/**************************************************************************
  Possibly update the entire mapview, if some conditions apply..
**************************************************************************/
function update_map_canvas_check()
{
  var time = new Date().getTime() - last_redraw_time;
  if (time > MAPVIEW_REFRESH_INTERVAL) {
    update_map_canvas_full();
  }
  requestAnimFrame(update_map_canvas_check);

}

/**************************************************************************
  ...
**************************************************************************/
function update_goto_path_lines()
{
  if (goto_active && goto_preview_active) {
    for (var i = 0; i < current_goto_path.length - 1; i++) {
      var ptile = current_goto_path[i];
      var ntile = current_goto_path[i+1];
      if (ptile == null || ntile == null) continue;

      if (ptile['x'] == (map['xsize'] - 1) && ntile['x'] == 0) continue;
      if (ntile['x'] == (map['xsize'] - 1) && ptile['x'] == 0) continue;

      var r = map_to_gui_pos(ptile['x'], ptile['y']);
      var gui_x1 = r['gui_dx'] - mapview['gui_x0'] + (tileset_tile_width / 2);
      var gui_y1 = r['gui_dy'] - mapview['gui_y0'] + (tileset_tile_height / 2); 

      var s = map_to_gui_pos(ntile['x'], ntile['y']);
      var gui_x2 = s['gui_dx'] - mapview['gui_x0'] + (tileset_tile_width / 2);
      var gui_y2 = s['gui_dy'] - mapview['gui_y0'] + (tileset_tile_height / 2);

      drawGotoLine(mapview_canvas_ctx, gui_x1, gui_y1, gui_x2, gui_y2);

    }
    if (current_goto_turns != undefined) {
      $("#turns_to_target").html("Turns to target: " + current_goto_turns);
    } else {
      $("#turns_to_target").html("-" );
    }

  }

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

  if (Math.abs(dx) > mapview['width'] || Math.abs(dy) > mapview['height']) {
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

  if (dx == 0 && dy == 0) {
    mapview_slide['active'] = false;
    return;
  } else if (dx >= 0 && dy <= 0) {
    mapview['gui_y0'] -= Math.abs(dy);
  } else if (dx >= 0 && dy >= 0) {
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
  update_map_canvas(0, 0, mapview['store_width'], mapview['store_height']);
  update_goto_path_lines(); 

  /* restore default mapview. */
  mapview_canvas = document.getElementById('canvas');
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
    
  mapview['store_width'] = old_width;
  mapview['store_height'] = old_height;
  mapview['width'] = old_width;
  mapview['height'] = old_height;

}

/**************************************************************************
  Renders a single frame in the mapview sliding animation, by clipping a
  region from the buffer_canvas onto the mapview canvas.
**************************************************************************/
function update_map_slide()
{
  var elapsed = 1 + new Date().getTime() - mapview_slide['start'];  
  mapview_slide['i'] = Math.floor(mapview_slide['max'] 
                        * (mapview_slide['slide_time'] 
                        - elapsed) / mapview_slide['slide_time']);

  if (mapview_slide['i'] <= 0) {
    mapview_slide['active'] = false;
    return;
  } 

  var dx = mapview_slide['dx'];
  var dy = mapview_slide['dy'];
  var sx = 0;
  var sy = 0;

  if (dx >= 0 && dy <= 0) {
    sx = Math.floor((dx * ((mapview_slide['max'] - mapview_slide['i']) / mapview_slide['max'])));
    sy = Math.floor((dy * ( -1 * mapview_slide['i'] / mapview_slide['max'])));
  } else if (dx >= 0 && dy >= 0) {
    sx = Math.floor((dx * ((mapview_slide['max'] - mapview_slide['i']) / mapview_slide['max'])));
    sy = Math.floor((dy * ((mapview_slide['max'] - mapview_slide['i']) / mapview_slide['max'])));
  } else if (dx <= 0 && dy >= 0) {
    sx = Math.floor((dx * ( -1 * mapview_slide['i'] / mapview_slide['max'])));
    sy = Math.floor((dy * ((mapview_slide['max'] - mapview_slide['i']) / mapview_slide['max'])));
  } else if (dx <= 0 && dy <= 0) {
    sx = Math.floor((dx * ( -1 * mapview_slide['i'] / mapview_slide['max'])));
    sy = Math.floor((dy * ( -1 * mapview_slide['i'] / mapview_slide['max'])));
  }

  mapview_canvas_ctx.drawImage(buffer_canvas, sx, sy, 
      mapview['width'], mapview['height'],
      0,0, mapview['width'], mapview['height']);

}
