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


var num_cardinal_tileset_dirs = 4;
var cardinal_tileset_dirs = [DIR8_NORTH, DIR8_EAST, DIR8_SOUTH, DIR8_WEST];

var NUM_CORNER_DIRS = 4;

var DIR4_TO_DIR8 = [ DIR8_NORTH, DIR8_SOUTH, DIR8_EAST, DIR8_WEST];

var current_select_sprite = 0;
var max_select_sprite = 4;

/* Items on the mapview are drawn in layers.  Each entry below represents
 * one layer.  The names are basically arbitrary and just correspond to
 * groups of elements in fill_sprite_array().  Callers of fill_sprite_array
 * must call it once for each layer. */
var LAYER_TERRAIN1 = 0;
var LAYER_TERRAIN2 = 1;
var LAYER_TERRAIN3 = 2;
var LAYER_ROADS = 3;
var LAYER_SPECIAL1 = 4;
var LAYER_CITY1 = 5;
var LAYER_UNIT = 6;
var LAYER_FOG = 7;
var LAYER_FOCUS_UNIT = 8;
var LAYER_CITYBAR = 9;
var LAYER_COUNT = 10;

// these layers are not used at the moment, for performance reasons.
//var LAYER_BACKGROUND = ; (not in use)
//var LAYER_GOTO = ; (not in use)
//var LAYER_EDITOR = ; (not in use)
//var LAYER_GRID* = ; (not in use)

/* An edge is the border between two tiles.  This structure represents one
 * edge.  The tiles are given in the same order as the enumeration name. */
var EDGE_NS = 0; /* North and south */
var EDGE_WE = 1; /* West and east */
var EDGE_UD = 2; /* Up and down (nw/se), for hex_width tilesets */
var EDGE_LR = 3; /* Left and right (ne/sw), for hex_height tilesets */
var EDGE_COUNT = 4;

var MATCH_NONE = 0;
var MATCH_SAME = 1;		/* "boolean" match */
var MATCH_PAIR = 2;
var MATCH_FULL = 3;

var CELL_WHOLE = 0;		/* entire tile */
var CELL_CORNER = 1;	/* corner of tile */

/* Darkness style.  Don't reorder this enum since tilesets depend on it. */
/* No darkness sprites are drawn. */
var DARKNESS_NONE = 0;

/* 1 sprite that is split into 4 parts and treated as a darkness4.  Only
 * works in iso-view. */
var DARKNESS_ISORECT = 1;

/* 4 sprites, one per direction.  More than one sprite per tile may be
 * drawn. */
var DARKNESS_CARD_SINGLE = 2;

/* 15=2^4-1 sprites.  A single sprite is drawn, chosen based on whether
 * there's darkness in _each_ of the cardinal directions. */
var DARKNESS_CARD_FULL = 3;

/* Corner darkness & fog.  3^4 = 81 sprites. */
var DARKNESS_CORNER = 4;

var dither_offset_x = [normal_tile_width/2, 0, normal_tile_width/2, 0];
var dither_offset_y = [0, normal_tile_height/2, normal_tile_height/2, 0];


var terrain_match = {"t.l0.hills1" : MATCH_NONE, 
"t.l0.mountains1" : MATCH_NONE, 
"t.l0.plains1" : MATCH_NONE,
"t.l0.desert1" : MATCH_NONE

};


/****************************************************************************
  Fill in the sprite array for the given tile, city, and unit.

  ptile, if specified, gives the tile.  If specified the terrain and specials
  will be drawn for this tile.  In this case (map_x,map_y) should give the
  location of the tile.

  punit, if specified, gives the unit.  For tile drawing this should
  generally be get_drawable_unit(); otherwise it can be any unit.

  pcity, if specified, gives the city.  For tile drawing this should
  generally be tile_city(ptile); otherwise it can be any city.

  citymode specifies whether this is part of a citydlg.  If so some drawing
  is done differently.
****************************************************************************/
function fill_sprite_array(layer, ptile, pedge, pcorner, punit, pcity, citymode)
{
  var sprite_array = [];
  
   var do_draw_unit = (punit != null && (draw_units || ptile == null || (draw_focus_unit
				     && unit_is_in_focus(punit))));
  
  
  switch (layer) {
    case LAYER_TERRAIN1:
    if (ptile != null) {
      var tterrain_near = tile_terrain_near(ptile);
      var pterrain = tile_terrain(ptile);
      sprite_array = sprite_array.concat(fill_terrain_sprite_layer(0, ptile, pterrain, tterrain_near));
           
    }
    break;
    
    case LAYER_TERRAIN2:
    if (ptile != null) {
      var tterrain_near = tile_terrain_near(ptile);
      var pterrain = tile_terrain(ptile);
      sprite_array = sprite_array.concat(fill_terrain_sprite_layer(1, ptile, pterrain, tterrain_near));
           
    }
    break;
    
    case LAYER_TERRAIN3:
      if (ptile != null) {
        var tterrain_near = tile_terrain_near(ptile);
        var pterrain = tile_terrain(ptile);
        sprite_array = sprite_array.concat(fill_terrain_sprite_layer(2, ptile, pterrain, tterrain_near));
           
        sprite_array = sprite_array.concat(fill_irrigation_sprite_array(ptile, pcity));   
      }
    break;
    
    case LAYER_ROADS:
      if (ptile != null) {
        sprite_array = sprite_array.concat(fill_road_rail_sprite_array(ptile, pcity));
      }
    break;

    case LAYER_SPECIAL1:
      if (ptile != null) {

        var river_sprite = get_tile_river_sprite(ptile);
        if (river_sprite != null) sprite_array.push(river_sprite);

        var spec_sprite = get_tile_specials_sprite(ptile);
        if (spec_sprite != null) sprite_array.push(spec_sprite);
        
        
        if (contains_special(ptile, S_MINE)) {
          sprite_array.push({"key" : "tx.mine"}); 
        }
        
        if (contains_special(ptile, S_HUT)) {
          sprite_array.push({"key" : "tx.village"}); 
        }
        
        if (contains_special(ptile, S_POLLUTION)) {
          sprite_array.push({"key" : "tx.pollution"}); 
        }

        sprite_array = sprite_array.concat(get_border_line_sprites(ptile));

      } 
    break;
    
    case LAYER_CITY1:
      if (pcity != null) {
        sprite_array.push(get_city_flag_sprite(pcity));
        sprite_array.push(get_city_sprite(pcity));
	if (pcity['unhappy']) {
          sprite_array.push({"key" : "city.disorder"});
	}
        sprite_array = sprite_array.concat(get_city_size_sprites(pcity));
      }


    break;
    
    
    case LAYER_UNIT:
    case LAYER_FOCUS_UNIT:
      if (do_draw_unit && XOR(layer == LAYER_UNIT, unit_is_in_focus(punit)) && active_city == null) {
        var stacked = true; /*ptile != null && (unit_list_size(ptile->units) > 1);*/
        var backdrop = false; /* !pcity;*/
      
        var stacked = ptile;


        if (unit_is_in_focus(punit)) {
          sprite_array.push(get_select_sprite());
        } 


	    /* TODO: Special case for drawing the selection rectangle.  The blinking
	    * unit is handled separately, inside get_drawable_unit(). */
        sprite_array = sprite_array.concat(fill_unit_sprite_array(punit, stacked, backdrop));
      
    }
    break;

    case LAYER_FOG:
      sprite_array = sprite_array.concat(fill_fog_sprite_array(ptile, pedge, pcorner));
      
      break;

    case LAYER_CITYBAR:
      if (pcity != null) {
        sprite_array.push(get_city_info_text(pcity));
      }   

      if (active_city != null && ptile != null && ptile['worked'] != null && active_city['id'] == ptile['worked']) {
        sprite_array.push(get_city_active_worked_sprite());
	var dx = city_tile(active_city)['x'] - ptile['x'];
	var dy = city_tile(active_city)['y'] - ptile['y'];
        var idx = get_city_dxy_to_index(dx, dy);
	var food_output = active_city['food_output'].substring(idx, idx + 1);
	var shield_output = active_city['shield_output'].substring(idx, idx + 1);
	var trade_output = active_city['trade_output'].substring(idx, idx + 1);
        
        sprite_array.push(get_city_food_output_sprite(food_output));
        sprite_array.push(get_city_shields_output_sprite(shield_output));
        sprite_array.push(get_city_trade_output_sprite(trade_output));
      } else if (active_city != null && ptile != null && ptile['worked'] != 0) {
        sprite_array.push(get_city_invalid_worked_sprite());
      }
 
    break;
    
  }
  
  
  return sprite_array;

}


/****************************************************************************
  Add sprites for the base tile to the sprite list.  This doesn't
  include specials or rivers.
****************************************************************************/
function fill_terrain_sprite_layer(layer_num, ptile, pterrain, tterrain_near)
{
  var result = [];
  
  result = result.concat(fill_terrain_sprite_array(layer_num, ptile, pterrain, tterrain_near));

  /* FIXME: handle blending and darkness. */
  
  return result;

}

/****************************************************************************
  Helper function for fill_terrain_sprite_layer.
****************************************************************************/
function fill_terrain_sprite_array(l, ptile, pterrain, tterrain_near)
{
    
  if (tile_types_setup["l" + l + "." + pterrain['graphic_str']] == null) {
    //console.log("missing " + "l" + l + "." + pterrain['graphic_str']);
    return [];
  }
  
  var dlp = tile_types_setup["l" + l + "." + pterrain['graphic_str']];
  
  switch (dlp['sprite_type']) {
    case CELL_WHOLE:
    {
      switch (dlp['match_style']) {
        case MATCH_NONE:
        {
          var result_sprites = [];
      	   if (dlp['dither'] == true) {
             for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
               var dir = cardinal_tileset_dirs[i];
               if (ts_tiles[tterrain_near[DIR4_TO_DIR8[i]]['graphic_str']] == null) continue; 
               var near_dlp = tile_types_setup["l" + l + "." + tterrain_near[DIR4_TO_DIR8[i]]['graphic_str']];
	       var terrain_near = (near_dlp['dither'] == true) ?  tterrain_near[DIR4_TO_DIR8[i]]['graphic_str'] : pterrain['graphic_str'];
	       var dither_tile = i + pterrain['graphic_str'] + "_" +  terrain_near;
               var x = dither_offset_x[i];
               var y = dither_offset_y[i];
	       result_sprites.push({"key": dither_tile, "offset_x" : x, "offset_y" : y});
             }
	     return result_sprites;
             
	   } else {
             return [ {"key" : "t.l" + l + "." + pterrain['graphic_str'] + 1} ];
	   }
          break;
        }
        
        case MATCH_SAME:
        {        
          var tileno = 0;
          var this_match_type = ts_tiles[pterrain['graphic_str']]['layer' + l + '_match_type'];
         
          for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
            var dir = cardinal_tileset_dirs[i];
            if (ts_tiles[tterrain_near[i]['graphic_str']] == null) continue; 
            var that = ts_tiles[tterrain_near[i]['graphic_str']]['layer' + l + '_match_type'];
            if (that == this_match_type) {
			  tileno |= 1 << i;
            }
          }
          
          return [ {"key" : "t.l" + l + "." + pterrain['graphic_str'] + "_" + cardinal_index_str(tileno)} ];
          break;
        } 
      }
    }
    
    case CELL_CORNER:
    {
      /* Divide the tile up into four rectangular cells.  Each of these
       * cells covers one corner, and each is adjacent to 3 different
       * tiles.  For each cell we pick a sprite based upon the adjacent
       * terrains at each of those tiles.  Thus, we have 8 different sprites
       * for each of the 4 cells (32 sprites total).
       *
       * These arrays correspond to the direction4 ordering. */
     
      var W = normal_tile_width;
      var H = normal_tile_height;
      var iso_offsets = [ [W / 4, 0], [W / 4, H / 2], [W / 2, H / 4], [0, H / 4]];
      var this_match_index = ('l' + l + '.' + pterrain['graphic_str'] in tile_types_setup) ? tile_types_setup['l' + l + '.' + pterrain['graphic_str']]['match_index'][0] : -1;  
      var that_match_index = ('l' + l + '.' + pterrain['graphic_str'] in tile_types_setup) ? tile_types_setup['l' + l + '.' + pterrain['graphic_str']]['match_index'][1] : -1;
      var result_sprites = [];

      /* put corner cells */
      for (var i = 0; i < NUM_CORNER_DIRS; i++) {
	    var count = dlp['match_indices'];
	    var array_index = 0;
	    var dir = dir_ccw(DIR4_TO_DIR8[i]);
	    var x = iso_offsets[i][0];
	    var y = iso_offsets[i][1];
	    
	    var m = [('l' + l + '.' + tterrain_near[dir_ccw(dir)]['graphic_str'] in tile_types_setup) ? tile_types_setup['l' + l + '.' + tterrain_near[dir_ccw(dir)]['graphic_str']]['match_index'][0] : -1 , 
	             ('l' + l + '.' + tterrain_near[dir]['graphic_str'] in tile_types_setup) ? tile_types_setup['l' + l + '.' + tterrain_near[dir]['graphic_str']]['match_index'][0] : -1,
	             ('l' + l + '.' + tterrain_near[dir_cw(dir)]['graphic_str'] in tile_types_setup) ? tile_types_setup['l' + l + '.' + tterrain_near[dir_cw(dir)]['graphic_str']]['match_index'][0] : -1];

        /* synthesize 4 dimensional array? */
	    switch (dlp['match_style']) {
	    case MATCH_NONE:
	      /* We have no need for matching, just plug the piece in place. */
	      break;
	    case MATCH_SAME:
	      	var b1 = (m[2] != this_match_index) ? 1 : 0;
	      	var b2 = (m[1] != this_match_index) ? 1 : 0;
	      	var b3 = (m[0] != this_match_index) ? 1 : 0;
	      	array_index = array_index * 2 + b1;
			array_index = array_index * 2 + b2;
	  		array_index = array_index * 2 + b3;
	      break;
	    case MATCH_PAIR:
            // FIXME: This doesn't work!
            /*var b1 = (m[2] == that_match_index) ? 1 : 0;
            var b2 = (m[1] == that_match_index) ? 1 : 0;
            var b3 = (m[0] == that_match_index) ? 1 : 0;
            array_index = array_index * 2 + b1;
            array_index = array_index * 2 + b2;
            array_index = array_index * 2 + b3;*/
            
            return [];

	      break;
	    case MATCH_FULL:
  	      {
	      var n = [];
	      var j = 0;
	      for (; j < 3; j++) {
	        var k = 0;
	        for (; k < count; k++) {
		      n[j] = k; /* default to last entry */
		      if (m[j] == dlp['match_index'][k]) {
		        break;
              }
	        }
	      }
	      array_index = array_index * count + n[2];
	      array_index = array_index * count + n[1];
	      array_index = array_index * count + n[0];
	    }
	    break;
	  };
	  array_index = array_index * NUM_CORNER_DIRS + i;
	  result_sprites.push({"key" : cellgroup_map[pterrain['graphic_str'] + "." + array_index]  + "." + i, "offset_x" : x, "offset_y" : y});

      }

      return result_sprites;
      break;
      
    }
  }
  
  return [];

}


/**********************************************************************
  Determine the sprite_type string.
***********************************************************************/
function check_sprite_type(sprite_type)
{
  if (sprite_type == "corner") {
    return CELL_CORNER;
  }
  if (sprite_type == "single") {
    return CELL_WHOLE;
  }
  if (sprite_type == "whole") {
    return CELL_WHOLE;
  }
  return CELL_WHOLE;
}


/**************************************************************************
 ...
**************************************************************************/
function fill_unit_sprite_array(punit, stacked, backdrop)
{
  var unit_offset = get_unit_anim_offset(punit);
  var result = [ get_unit_nation_flag_sprite(punit), 
           {"key" : unit_type(punit)['graphic_str'], 
            "offset_x": unit_offset['x'] + unit_offset_x, 
	    "offset_y" : unit_offset['y'] - unit_offset_y} ];
  var activities = get_unit_activity_sprite(punit);
  if (activities != null) {
    activities['offset_x'] = activities['offset_x'] + unit_offset['x']; 
    activities['offset_y'] = activities['offset_y'] + unit_offset['y']; 
    result.push(activities); 
  }
  
  result.push(get_unit_hp_sprite(punit));
             
  return result;           

  
}


/**************************************************************************
  Return the tileset name of the direction.  This is similar to
  dir_get_name but you shouldn't change this or all tilesets will break.
**************************************************************************/
function dir_get_tileset_name(dir)
{
  switch (dir) {
  case DIR8_NORTH:
    return "n";
  case DIR8_NORTHEAST:
    return "ne";
  case DIR8_EAST:
    return "e";
  case DIR8_SOUTHEAST:
    return "se";
  case DIR8_SOUTH:
    return "s";
  case DIR8_SOUTHWEST:
    return "sw";
  case DIR8_WEST:
    return "w";
  case DIR8_NORTHWEST:
    return "nw";
  };
  
  return "";
}


/****************************************************************************
  Return a directional string for the cardinal directions.  Normally the
  binary value 1000 will be converted into "n1e0s0w0".  This is in a
  clockwise ordering.
****************************************************************************/
function cardinal_index_str(idx)
{
  var c = "";

  for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
    var value = (idx >> i) & 1;

    c += dir_get_tileset_name(cardinal_tileset_dirs[i]) + value;
  }

  return c;
}


/**********************************************************************
  Return the flag graphic to be used by the city.
***********************************************************************/
function get_city_flag_sprite(pcity) {
  var owner_id = pcity['owner'];
  if (owner_id == null) return {};
  var owner = players[owner_id];
  if (owner == null) return {};
  var nation_id = owner['nation'];
  if (nation_id == null) return {};
  var nation = nations[nation_id];
  if (nation == null) return {};
  return {"key" : "f." + nation['graphic_str'], 
          "offset_x" : city_flag_offset_x, 
          "offset_y" : - city_flag_offset_y};
}

/**********************************************************************
  Return the sprite for a active city worked tile.
***********************************************************************/
function get_city_active_worked_sprite() {
  return {"key" : "city_active", 
          "offset_x" : 0, 
          "offset_y" : 0};
}

/**********************************************************************
...
***********************************************************************/
function get_city_food_output_sprite(num) {
  return {"key" : "city.t_food_" + num, 
          "offset_x" : normal_tile_width/4, 
          "offset_y" : -normal_tile_height/4};
}

/**********************************************************************
...
***********************************************************************/
function get_city_shields_output_sprite(num) {
  return {"key" : "city.t_shields_" + num, 
          "offset_x" : normal_tile_width/4, 
          "offset_y" : -normal_tile_height/4};
}

/**********************************************************************
...
***********************************************************************/
function get_city_trade_output_sprite(num) {
  return {"key" : "city.t_trade_" + num, 
          "offset_x" : normal_tile_width/4, 
          "offset_y" : -normal_tile_height/4};
}


/**********************************************************************
  Return the sprite for an invalid city worked tile.
***********************************************************************/
function get_city_invalid_worked_sprite() {
  return {"key" : "city_invalid", 
          "offset_x" : 0, 
          "offset_y" : 0};
}


/**********************************************************************
  Return the size graphic to be used by the city.
***********************************************************************/
function get_city_size_sprites(pcity) {
  var result = [];
  
  var size = pcity['size'];
  var size_str = size + '';

  var lsb = 0;
  var msb = 0;

  if (size_str.length == 1) {
    lsb = size_str;
    msb = 0;
  } else if (size_str.length == 2) {
    lsb = size_str.substring(1, 2);
    msb = size_str.substring(0, 1);
  }   
  
  result.push({"key" : "city.size_" + lsb, 
            "offset_x" : city_size_offset_x, 
            "offset_y" : - city_size_offset_y});

  if (size_str.length == 2) {
    result.push({"key" : "city.size_" + msb + "0", 
            "offset_x" : city_size_offset_x, 
            "offset_y" : - city_size_offset_y});
  }
  
  return result;
}

/**********************************************************************
...
***********************************************************************/
function get_border_line_sprites(ptile)
{
  var result = [];

  for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
    var dir = cardinal_tileset_dirs[i];
    var checktile = mapstep(ptile, dir);

    if (checktile != null && checktile['owner'] != null 
        && ptile['owner'] != null 
        && ptile['owner'] != 255 
	&& ptile['owner'] != checktile['owner']) {
      var pnation = nations[ptile['owner']];
      result.push({"key" : "border", "dir" : dir,
                   "color": pnation['color']});
    }
  }

  return result;
}


/**********************************************************************
  ...
***********************************************************************/
function get_unit_nation_flag_sprite(punit)
{
  var owner_id = punit['owner'];
  var owner = players[owner_id];
  var nation_id = owner['nation'];
  var nation = nations[nation_id];
  var unit_offset = get_unit_anim_offset(punit);

  return {"key" : "f.shield." + nation['graphic_str'], 
          "offset_x" : unit_flag_offset_x + unit_offset['x'], 
          "offset_y" : - unit_flag_offset_y + unit_offset['y']};
}

/**********************************************************************
  ...
***********************************************************************/
function get_unit_hp_sprite(punit)
{
  var hp = punit['hp'];
  var unit_type = unit_types[punit['type']];
  var max_hp = unit_type['hp'];
  var healthpercent = 10 * Math.floor((10 * hp) / max_hp);
  var unit_offset = get_unit_anim_offset(punit);


  return {"key" : "unit.hp_" + healthpercent, 
          "offset_x" : unit_flag_offset_x + -25 + unit_offset['x'], 
          "offset_y" : - unit_flag_offset_y - 15 + unit_offset['y']};
}

/**********************************************************************
  ...
***********************************************************************/
function get_unit_activity_sprite(punit)
{
  var activity = punit['activity'];
  
  /* don't draw activity for enemy units */
  if (client.conn.playing == null || punit['owner'] != client.conn.playing.playerno) {
    return null;
  }

  switch (activity) {
    case ACTIVITY_POLLUTION:
      return {"key" : "unit.fallout",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;

    case ACTIVITY_OLD_ROAD:
      return {"key" : "unit.road",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_MINE:
      return {"key" : "unit.mine",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_IRRIGATE:
      return {"key" : "unit.irrigate",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_FORTIFIED:
      return {"key" : "unit.fortified",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_FORTRESS:
      return {"key" : "unit.fortress",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_SENTRY:
      return {"key" : "unit.sentry",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_OLD_RAILROAD:
      return {"key" : "unit.road",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_PILLAGE:
      return {"key" : "unit.pillage",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_GOTO:
      return {"key" : "unit.goto",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_EXPLORE:
      return {"key" : "unit.auto_explore",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
    
    case ACTIVITY_TRANSFORM:
      return {"key" : "unit.transform",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
       
    case ACTIVITY_FORTIFYING:
      return {"key" : "unit.fortifying",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;

    case ACTIVITY_GEN_ROAD:
      return {"key" : "unit.road",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;

    case ACTIVITY_CONVERT:
      return {"key" : "unit.convert",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
    break;
  }

  if (unit_has_goto(punit)) {
      return {"key" : "unit.goto",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
  }  

  if (punit['ai'] == true) {
      return {"key" : "unit.auto_settler",
          "offset_x" : unit_activity_offset_x, 
          "offset_y" : - unit_activity_offset_y}
  }

  return null;
}

/****************************************************************************
  Return the sprite in the city_sprite listing that corresponds to this
  city - based on city style and size.

  See also load_city_sprite, free_city_sprite.
****************************************************************************/
function get_city_sprite(pcity) 
{
  var owner_id = pcity['owner'];
  var player = players[owner_id];
  var style_id = player['city_style'];
  var city_rule = city_rules[style_id];
  
  var size = 0;
  if (pcity['size'] >=4 && pcity['size'] <=7) {
    size = 1;
  } else if (pcity['size'] >=8 && pcity['size'] <=11) {
    size = 2;
  } else if (pcity['size'] >=12 && pcity['size'] <=15) {
    size = 3;
  } else if (pcity['size'] >=16) {
    size = 4;
  }  

  var city_walls = pcity['walls'] ? "wall" : "city"; 

  var tag = city_rule['graphic'] + "_" + city_walls + "_" + size;
  if (sprites[tag] == null) {
    tag = city_rule['graphic_alt'] + "_" + city_walls + "_" + size;
  }

  return {"key" :  tag, "offset_x": 0, "offset_y" : -unit_offset_y};
}


/****************************************************************************
  Add sprites for fog (and some forms of darkness).
****************************************************************************/
function fill_fog_sprite_array(ptile, pedge, pcorner)
{

  var i, tileno = 0;

  if (pcorner == null) return [];

  for (i = 3; i >= 0; i--) {
    var unknown = 0, fogged = 1, known = 2;
    var value = -1;

    if (pcorner['tile'][i] == null) {
	  value = fogged;
    } else {
	  switch (tile_get_known(pcorner['tile'][i])) {
	    case TILE_KNOWN_SEEN:
	      value = known;
	      break;
	    case TILE_KNOWN_UNSEEN:
	      value = fogged;
	      break;
	    case TILE_UNKNOWN:
	      value = unknown;
	      break;
      };
    }
    tileno = tileno * 3 + value;
  }
  
  if (tileno >= 80) return [];

  return [{"key" : fullfog[tileno]}];

}

/****************************************************************************
 ...
****************************************************************************/
function get_select_sprite() 
{
  // update selected unit sprite 10 times a second.
  current_select_sprite = (Math.floor(new Date().getTime() / 100) % max_select_sprite);
  return {"key" : "unit.select" + current_select_sprite };
}

/****************************************************************************
 ...
****************************************************************************/
function get_city_info_text(pcity)
{
  return {"key" : "city_text", "text" : unescape(pcity['name']),
  		  "offset_x": citybar_offset_x, "offset_y" : citybar_offset_y}; 
}

/****************************************************************************
 ...
****************************************************************************/
function get_tile_specials_sprite(ptile)
{
  if (ptile == null || ptile['resource'] == null) return null;

  var resource = resources[ptile['resource']];
  
  if (resource == null) return null; 
  
  return  {"key" : resource['graphic_str']} ;
}

/****************************************************************************
 ...
****************************************************************************/
function get_tile_river_sprite(ptile)
{
  if (ptile == null) {
    return null;
  }

  if (tile_has_road(ptile, ROAD_RIVER)) {
    var river_str = "";
    for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
      var dir = cardinal_tileset_dirs[i];
      var checktile = mapstep(ptile, dir);
      if (checktile 
          && (tile_has_road(checktile, ROAD_RIVER) || is_ocean_tile(checktile))) {
        river_str = river_str + dir_get_tileset_name(dir) + "1";
      } else {
        river_str = river_str + dir_get_tileset_name(dir) + "0";
      }

    }
    return {"key" : "river_s_" + river_str};
  }

  var pterrain = tile_terrain(ptile);
  if (pterrain['graphic_str'] == "coast") {
    for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
      var dir = cardinal_tileset_dirs[i];
      var checktile = mapstep(ptile, dir);
      if (checktile != null && tile_has_road(checktile, ROAD_RIVER)) {
        return {"key" : "river_outlet_" + dir_get_tileset_name(dir)};
      }
    }
  }

  return null;

}

/****************************************************************************
 ...
****************************************************************************/
function get_unit_image_sprite(punit)
{
  var tag = unit_type(punit)['graphic_str'];
  
  if (tileset[tag] == null) return null;
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}    


/****************************************************************************
 ...
****************************************************************************/
function get_unit_type_image_sprite(punittype)
{
  var tag = punittype['graphic_str'];
  
  if (tileset[tag] == null) return null;
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}    

/****************************************************************************
 ...
****************************************************************************/
function get_improvement_image_sprite(pimprovement)
{
  var tag = pimprovement['graphic_str'];
  
  if (tileset[tag] == null) {
    tag = pimprovement['graphic_alt'];
    if (tileset[tag] == null) return null;
  }
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
} 

/****************************************************************************
 ...
****************************************************************************/
function get_technology_image_sprite(ptech)
{
  var tag = ptech['graphic_str'];
  
  if (tileset[tag] == null) return null;
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
} 

/****************************************************************************
 ...
****************************************************************************/
function get_player_fplag_sprite(pplayer)
{
  var pnation = nations[pplayer['nation']]; 
  var tag = "f." + pnation['graphic_str'];
  
  if (tileset[tag] == null) return null;
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}


/****************************************************************************
 ...
****************************************************************************/
function get_nation_flag_sprite(pnation)
{
  var tag = "f." + pnation['graphic_str'];
  
  if (tileset[tag] == null) return null;
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}  

/****************************************************************************
 ...
****************************************************************************/
function get_treaty_agree_thumb_up()
{
  var tag = "treaty.agree_thumb_up";
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}  

/****************************************************************************
 ...
****************************************************************************/
function get_treaty_disagree_thumb_down()
{
  var tag = "treaty.disagree_thumb_down";
  
  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag, 
            "image-src" : "/tileset/freeciv-web-tileset-" + i + ".png",
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}  


/****************************************************************************
  Returns a list of tiles to draw to render roads and railroads.
  TODO: add support for road and railroad on same tile. 
****************************************************************************/
function fill_road_rail_sprite_array(ptile, pcity)
{
  var road = tile_has_road(ptile, ROAD_ROAD);
  var rail = tile_has_road(ptile, ROAD_RAIL);
  var road_near = [];
  var rail_near = [];
  var draw_rail = [];
  var draw_road = [];
  var result_sprites = [];
  
  var draw_single_road = road == true && pcity == null && rail == false;
  var draw_single_rail = rail && pcity == null;
  
  for (var dir = 0; dir < 8; dir++) {
    /* Check if there is adjacent road/rail. */
    var tile1 = mapstep(ptile, dir);
    if (tile1 != null && tile_get_known(tile1) != TILE_UNKNOWN) {
      road_near[dir] = tile_has_road(tile1, ROAD_ROAD);
      rail_near[dir] = tile_has_road(tile1, ROAD_RAIL);

      /* Draw rail/road if there is a connection from this tile to the
        * adjacent tile.  But don't draw road if there is also a rail
        * connection. */
      draw_rail[dir] = rail && rail_near[dir];
      draw_road[dir] = road && road_near[dir] && !draw_rail[dir];

      /* Don't draw an isolated road/rail if there's any connection. */
      draw_single_rail &= !draw_rail[dir];
      draw_single_road &= !draw_rail[dir] && !draw_road[dir];

    }
  }
  
  
    /* With roadstyle 0, we simply draw one road/rail for every connection.
     * This means we only need a few sprites, but a lot of drawing is
     * necessary and it generally doesn't look very good. */
    var i;

    /* First raw roads under rails. */
    if (road) {
      for (i = 0; i < 8; i++) {
        if (draw_road[i]) {
	      result_sprites.push({"key" : "r.road_" + dir_get_tileset_name(i)});
	    }
      }
    }

    /* Then draw rails over roads. */
    if (rail) {
      for (i = 0; i < 8; i++) {
        if (draw_rail[i]) {
	      result_sprites.push({"key" : "r.rail_" + dir_get_tileset_name(i)});
        }
      }
    }
  

 /* Draw isolated rail/road separately (styles 0 and 1 only). */
 
  if (draw_single_rail) {
      result_sprites.push({"key" : "r.rail_isolated"});
  } else if (draw_single_road) {
      result_sprites.push({"key" : "r.road_isolated"});
  }

  return result_sprites; 
}


/****************************************************************************
  ...
****************************************************************************/
function fill_irrigation_sprite_array(ptile, pcity)
{

  var result_sprites = [];

  /* We don't draw the irrigation if there's a city (it just gets overdrawn
   * anyway, and ends up looking bad). */
  if (contains_special(ptile, S_IRRIGATION) && pcity == null) {
    if (contains_special(ptile, S_FARMLAND)) {
      result_sprites.push({"key" : "tx.farmland"});
    } else {
      result_sprites.push({"key" : "tx.irrigation"});
    }
  }

  return result_sprites;
}

