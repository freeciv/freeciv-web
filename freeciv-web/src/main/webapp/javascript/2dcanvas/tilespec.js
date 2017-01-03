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


var num_cardinal_tileset_dirs = 4;
var cardinal_tileset_dirs = [DIR8_NORTH, DIR8_EAST, DIR8_SOUTH, DIR8_WEST];

var NUM_CORNER_DIRS = 4;

var DIR4_TO_DIR8 = [ DIR8_NORTH, DIR8_SOUTH, DIR8_EAST, DIR8_WEST];

var current_select_sprite = 0;
var max_select_sprite = 4;

var explosion_anim_map = {};

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
var LAYER_SPECIAL2 = 6;
var LAYER_UNIT = 7;
var LAYER_FOG = 8;
var LAYER_SPECIAL3 = 9;
var LAYER_TILELABEL = 10;
var LAYER_CITYBAR = 11;
var LAYER_GOTO = 12;
var LAYER_COUNT = 13;

// these layers are not used at the moment, for performance reasons.
//var LAYER_BACKGROUND = ; (not in use)
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

var terrain_match = {"t.l0.hills1" : MATCH_NONE,
"t.l0.mountains1" : MATCH_NONE,
"t.l0.plains1" : MATCH_NONE,
"t.l0.desert1" : MATCH_NONE

};

/**************************************************************************
  Returns true iff the tileset has graphics for the specified tag.
**************************************************************************/
function tileset_has_tag(tagname)
{
  return (sprites[tagname] != null);
}

/**************************************************************************
  Returns the tag name of the graphic showing the specified Extra on the
  map.
**************************************************************************/
function tileset_extra_graphic_tag(extra)
{
  if (extra == null) {
    console.log("No extra to return tag for.");
    return null;
  }

  if (tileset_has_tag(extra['graphic_str'])) {
    return extra['graphic_str'];
  }

  if (tileset_has_tag(extra['graphic_alt'])) {
    return extra['graphic_alt'];
  }

  console.log("No graphic for extra " + extra['name']);
  return null;
}

/**************************************************************************
  Returns the tag name of the graphic showing the Extra specified by ID on
  the map.
**************************************************************************/
function tileset_extra_id_graphic_tag(extra_id)
{
  return tileset_extra_graphic_tag(extras[extra_id]);
}

/**************************************************************************
  Returns the tag name of the graphic showing that a unit is building the
  specified Extra.
**************************************************************************/
function tileset_extra_activity_graphic_tag(extra)
{
  if (extra == null) {
    console.log("No extra to return tag for.");
    return null;
  }

  if (tileset_has_tag(extra['activity_gfx'])) {
    return extra['activity_gfx'];
  }

  if (tileset_has_tag(extra['act_gfx_alt'])) {
    return extra['act_gfx_alt'];
  }

  if (tileset_has_tag(extra['act_gfx_alt2'])) {
    return extra['act_gfx_alt2'];
  }

  console.log("No activity graphic for extra " + extra['name']);
  return null;
}

/**************************************************************************
  Returns the tag name of the graphic showing that a unit is building the
  Extra specified by the id.
**************************************************************************/
function tileset_extra_id_activity_graphic_tag(extra_id)
{
  return tileset_extra_activity_graphic_tag(extras[extra_id]);
}

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


        if (tile_has_extra(ptile, EXTRA_MINE)) {
          sprite_array.push({"key" :
                              tileset_extra_id_graphic_tag(EXTRA_MINE)});
        }
        if (tile_has_extra(ptile, EXTRA_OIL_WELL)) {
          sprite_array.push({"key" :
                              tileset_extra_id_graphic_tag(EXTRA_OIL_WELL)});
        }

        sprite_array = sprite_array.concat(fill_layer1_sprite_array(ptile, pcity));

        if (tile_has_extra(ptile, EXTRA_HUT)) {
          sprite_array.push({"key" :
                              tileset_extra_id_graphic_tag(EXTRA_HUT)});
        }

        if (tile_has_extra(ptile, EXTRA_POLLUTION)) {
          sprite_array.push({"key" :
                              tileset_extra_id_graphic_tag(EXTRA_POLLUTION)});
        }

        if (tile_has_extra(ptile, EXTRA_FALLOUT)) {
          sprite_array.push({"key" :
                              tileset_extra_id_graphic_tag(EXTRA_FALLOUT)});
        }

        sprite_array = sprite_array.concat(get_border_line_sprites(ptile));

      }
    break;

    case LAYER_CITY1:
      if (pcity != null) {
        sprite_array.push(get_city_sprite(pcity));
	if (pcity['unhappy']) {
          sprite_array.push({"key" : "city.disorder"});
	}
      }


    break;

    case LAYER_SPECIAL2:
      if (ptile != null) {
        sprite_array = sprite_array.concat(fill_layer2_sprite_array(ptile, pcity));
      }
      break;

    case LAYER_UNIT:
      var do_draw_unit = (punit != null && (draw_units || ptile == null || (draw_focus_unit
				     && unit_is_in_focus(punit))));

      if (do_draw_unit && active_city == null) {
        var stacked = (ptile['units'] != null && ptile['units'].length > 1);
        var backdrop = false; /* !pcity;*/

        if (unit_is_in_focus(punit)) {
          sprite_array.push(get_select_sprite());
        }

        /* TODO: Special case for drawing the selection rectangle.  The blinking
        * unit is handled separately, inside get_drawable_unit(). */
        sprite_array = sprite_array.concat(fill_unit_sprite_array(punit, stacked, backdrop));

    }

    /* show explosion animation on current tile.*/
     if (ptile != null && explosion_anim_map[ptile['index']] != null) {
       var explode_step = explosion_anim_map[ptile['index']];
       explosion_anim_map[ptile['index']] =  explode_step - 1;
       if (explode_step > 20) {
         sprite_array.push({"key" : "explode.unit_0",
           "offset_x" : unit_offset_x,
           "offset_y" : unit_offset_y});
       } else if (explode_step > 15) {
         sprite_array.push({"key" : "explode.unit_1",
           "offset_x" : unit_offset_x,
           "offset_y" : unit_offset_y});
       } else if (explode_step > 20) {
         sprite_array.push({"key" : "explode.unit_2",
           "offset_x" : unit_offset_x,
           "offset_y" : unit_offset_y});
       } else if (explode_step > 10) {
         sprite_array.push({"key" : "explode.unit_3",
           "offset_x" : unit_offset_x,
           "offset_y" : unit_offset_y});
       } else if (explode_step > 0) {
         sprite_array.push({"key" : "explode.unit_4",
           "offset_x" : unit_offset_x,
           "offset_y" : unit_offset_y});
       } else {
         delete explosion_anim_map[ptile['index']];
       }

     }


    break;

    case LAYER_FOG:
      sprite_array = sprite_array.concat(fill_fog_sprite_array(ptile, pedge, pcorner));

      break;

    case LAYER_SPECIAL3:
      if (ptile != null) {
        sprite_array = sprite_array.concat(fill_layer3_sprite_array(ptile, pcity));
      }
      break;

    case LAYER_TILELABEL:
      if (ptile != null) {
        sprite_array.push(get_tile_label_text(ptile));
      }
      break;

    case LAYER_CITYBAR:
      if (pcity != null && show_citybar) {
        sprite_array.push(get_city_info_text(pcity));
      }

      if (active_city != null && ptile != null && ptile['worked'] != null
          && active_city['id'] == ptile['worked'] && active_city['food_output'] != null) {
	var dx = city_tile(active_city)['x'] - ptile['x'];
	var dy = city_tile(active_city)['y'] - ptile['y'];

        //this is a quick-fix for showing city workers correctly near map edges.
        if (dx > 4) dx -= map['xsize']; 
        if (dx < -4) dx += map['xsize'];

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

    case LAYER_GOTO:
      if (ptile != null && ptile['goto_dir'] != null) {
        sprite_array = sprite_array.concat(fill_goto_line_sprite_array(ptile));
      }

      if (ptile != null && ptile['nuke'] > 0) {
        ptile['nuke'] = ptile['nuke'] - 1;
        sprite_array.push({"key" : "explode.nuke",
               "offset_x" : -45,
               "offset_y" : -45});
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
  /* FIXME: handle blending and darkness. */

  return fill_terrain_sprite_array(layer_num, ptile, pterrain, tterrain_near);

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
          var gfx_key = "t.l" + l + "." + pterrain['graphic_str'] + "_" + cardinal_index_str(tileno);
	  var y = tileset_tile_height - tileset[gfx_key][3];

          return [ {"key" : gfx_key, "offset_x" : 0, "offset_y" : y} ];
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
  if (stacked) result.push(get_unit_stack_sprite());
  if (punit['veteran'] > 0) result.push(get_unit_veteran_sprite(punit));

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
  Return the flag graphic to be used by the base on tile
***********************************************************************/
function get_base_flag_sprite(ptile) {
  var owner_id = ptile['extras_owner'];
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
 Returns the sprite key for the number of defending units in a city.
***********************************************************************/
function get_city_occupied_sprite(pcity) {
  var owner_id = pcity['owner'];
  var ptile = city_tile(pcity);
  var punits = tile_units(ptile);

  if (!observing && client.conn.playing != null
      && owner_id != client.conn.playing.playerno && pcity['occupied']) {
    return "citybar.occupied";
  } else if (punits.length == 1) {
    return "citybar.occupancy_1";
  } else if (punits.length == 2) {
    return "citybar.occupancy_2";
  } else if (punits.length >= 3) {
    return "citybar.occupancy_3";
  } else {
    return "citybar.occupancy_0";
  }

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
  return {"key" : "grid.unavailable",
          "offset_x" : 0,
          "offset_y" : 0};
}


/**********************************************************************
...
***********************************************************************/
function fill_goto_line_sprite_array(ptile)
{
  return {"key" : "goto_line", "goto_dir" : ptile['goto_dir']};
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
      var pnation = nations[players[ptile['owner']]['nation']];
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
function get_unit_nation_flag_normal_sprite(punit)
{
  var owner_id = punit['owner'];
  var owner = players[owner_id];
  var nation_id = owner['nation'];
  var nation = nations[nation_id];
  var unit_offset = get_unit_anim_offset(punit);

  return {"key" : "f." + nation['graphic_str'],
          "offset_x" : unit_flag_offset_x + unit_offset['x'],
          "offset_y" : - unit_flag_offset_y + unit_offset['y']};
}

/**********************************************************************
  ...
***********************************************************************/
function get_unit_stack_sprite(punit)
{
  return {"key" : "unit.stack",
          "offset_x" : unit_flag_offset_x + -25,
          "offset_y" : - unit_flag_offset_y - 15};
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
function get_unit_veteran_sprite(punit)
{
  return {"key" : "unit.vet_" + punit['veteran'],
          "offset_x" : unit_activity_offset_x - 20,
          "offset_y" : - unit_activity_offset_y - 10};
}

/**********************************************************************
  ...
***********************************************************************/
function get_unit_activity_sprite(punit)
{
  var activity = punit['activity'];
  var act_tgt  = punit['activity_tgt'];

  /* don't draw activity for enemy units */
  if (client.conn.playing == null || punit['owner'] != client.conn.playing.playerno) {
    return null;
  }

  switch (activity) {
    case ACTIVITY_POLLUTION:
      return {"key" : "unit.fallout",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_MINE:
      return {"key"      : -1 == act_tgt ?
                             "unit.plant" :
                             tileset_extra_id_activity_graphic_tag(act_tgt),
              "offset_x" : unit_activity_offset_x,
              "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_IRRIGATE:
      return {"key" : -1 == act_tgt ?
                        "unit.irrigate" :
                        tileset_extra_id_activity_graphic_tag(act_tgt),
              "offset_x" : unit_activity_offset_x,
              "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_FORTIFIED:
      return {"key" : "unit.fortified",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_BASE:
      return {"key" : tileset_extra_id_activity_graphic_tag(act_tgt),
              "offset_x" : unit_activity_offset_x,
              "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_SENTRY:
      return {"key" : "unit.sentry",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_PILLAGE:
      return {"key" : "unit.pillage",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_GOTO:
      return {"key" : "unit.goto",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_EXPLORE:
      return {"key" : "unit.auto_explore",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_TRANSFORM:
      return {"key" : "unit.transform",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_FORTIFYING:
      return {"key" : "unit.fortifying",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_GEN_ROAD:
      return {"key" : tileset_extra_id_activity_graphic_tag(act_tgt),
              "offset_x" : unit_activity_offset_x,
              "offset_y" : - unit_activity_offset_y};

    case ACTIVITY_CONVERT:
      return {"key" : "unit.convert",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};
  }

  if (unit_has_goto(punit)) {
      return {"key" : "unit.goto",
          "offset_x" : unit_activity_offset_x,
          "offset_y" : - unit_activity_offset_y};
  }

  if (punit['ai'] == true) {
      return {"key" : "unit.auto_settler",
          "offset_x" : 20, //FIXME.
          "offset_y" : - unit_activity_offset_y};
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
  var style_id = pcity['style'];
  if (style_id == -1) style_id = 0;   /* sometimes a player has no city_style. */
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
	  value = unknown;
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
  // update selected unit sprite 6 times a second.
  current_select_sprite = (Math.floor(new Date().getTime() * 6 / 1000) % max_select_sprite);
  return {"key" : "unit.select" + current_select_sprite };
}

/****************************************************************************
 ...
****************************************************************************/
function get_city_info_text(pcity)
{
  return {"key" : "city_text", "city" : pcity,
  		  "offset_x": citybar_offset_x, "offset_y" : citybar_offset_y};
}

/****************************************************************************
 ...
****************************************************************************/
function get_tile_label_text(ptile)
{
  return {"key" : "tile_label", "tile" : ptile,
  		  "offset_x": tilelabel_offset_x, "offset_y" : tilelabel_offset_y};
}

/****************************************************************************
 ...
****************************************************************************/
function get_tile_specials_sprite(ptile)
{
  if (ptile == null || ptile['resource'] == null) return null;

  var extra = extras[ptile['resource']];

  if (extra == null) {
    return null;
  }

  return  {"key" : extra['graphic_str']} ;
}

/****************************************************************************
 ...
****************************************************************************/
function get_tile_river_sprite(ptile)
{
  if (ptile == null) {
    return null;
  }

  if (tile_has_extra(ptile, ROAD_RIVER)) {
    var river_str = "";
    for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
      var dir = cardinal_tileset_dirs[i];
      var checktile = mapstep(ptile, dir);
      if (checktile
          && (tile_has_extra(checktile, ROAD_RIVER) || is_ocean_tile(checktile))) {
        river_str = river_str + dir_get_tileset_name(dir) + "1";
      } else {
        river_str = river_str + dir_get_tileset_name(dir) + "0";
      }

    }
    return {"key" : "road.river_s_" + river_str};
  }

  var pterrain = tile_terrain(ptile);
  if (pterrain['graphic_str'] == "coast") {
    for (var i = 0; i < num_cardinal_tileset_dirs; i++) {
      var dir = cardinal_tileset_dirs[i];
      var checktile = mapstep(ptile, dir);
      if (checktile != null && tile_has_extra(checktile, ROAD_RIVER)) {
        return {"key" : "road.river_outlet_" + dir_get_tileset_name(dir)};
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
  var height = (tileset[tag][3] - 2);
  var i = tileset[tag][4];
  return {"tag": tag,
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
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
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
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
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}

/****************************************************************************
 ...
****************************************************************************/
function get_specialist_image_sprite(tag)
{
  if (tileset[tag] == null) return null;

  var tileset_x = tileset[tag][0];
  var tileset_y = tileset[tag][1];
  var width = tileset[tag][2];
  var height = tileset[tag][3];
  var i = tileset[tag][4];
  return {"tag": tag,
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
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
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
            "tileset-x" : tileset_x,
            "tileset-y" : tileset_y,
            "width" : width,
            "height" : height
            };
}

/****************************************************************************
 ...
****************************************************************************/
function get_player_flag_url(pplayer)
{
  var pnation = nations[pplayer['nation']];

  return "/images/flags/" + pnation['graphic_str'] + "-web.png"
}

/****************************************************************************
 ...
****************************************************************************/
function get_nation_flag_url(pnation)
{
  return "/images/flags/" + pnation['graphic_str'] + "-web.png"
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
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
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
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
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
            "image-src" : "/tileset/freeciv-web-tileset-" + tileset_name + "-" + i + ".png?ts=" + ts,
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
  var road = tile_has_extra(ptile, ROAD_ROAD);
  var rail = tile_has_extra(ptile, ROAD_RAIL);
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
      road_near[dir] = tile_has_extra(tile1, ROAD_ROAD);
      rail_near[dir] = tile_has_extra(tile1, ROAD_RAIL);

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
	      result_sprites.push({"key" : "road.road_" + dir_get_tileset_name(i)});
	    }
      }
    }

    /* Then draw rails over roads. */
    if (rail) {
      for (i = 0; i < 8; i++) {
        if (draw_rail[i]) {
	      result_sprites.push({"key" : "road.rail_" + dir_get_tileset_name(i)});
        }
      }
    }


 /* Draw isolated rail/road separately (styles 0 and 1 only). */

  if (draw_single_rail) {
      result_sprites.push({"key" : "road.rail_isolated"});
  } else if (draw_single_road) {
      result_sprites.push({"key" : "road.road_isolated"});
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
  if (tile_has_extra(ptile, EXTRA_IRRIGATION) && pcity == null) {
    if (tile_has_extra(ptile, EXTRA_FARMLAND)) {
      result_sprites.push({"key" :
                            tileset_extra_id_graphic_tag(EXTRA_FARMLAND)});
    } else {
      result_sprites.push({"key" :
                            tileset_extra_id_graphic_tag(EXTRA_IRRIGATION)});
    }
  }

  return result_sprites;
}

/****************************************************************************
  ...
****************************************************************************/
function fill_layer1_sprite_array(ptile, pcity)
{
  var result_sprites = [];

  /* We don't draw the bases if there's a city */
  if (pcity == null) {
    if (tile_has_extra(ptile, BASE_FORTRESS)) {
      result_sprites.push({"key" : "base.fortress_bg",
                           "offset_y" : -normal_tile_height / 2});
    }
  }

  return result_sprites;
}

/****************************************************************************
  ...
****************************************************************************/
function fill_layer2_sprite_array(ptile, pcity)
{
  var result_sprites = [];

  /* We don't draw the bases if there's a city */
  if (pcity == null) {
    if (tile_has_extra(ptile, BASE_AIRBASE)) {
      result_sprites.push({"key" : "base.airbase_mg",
                           "offset_y" : -normal_tile_height / 2});
    }
    if (tile_has_extra(ptile, BASE_BUOY)) {
      result_sprites.push(get_base_flag_sprite(ptile));
      result_sprites.push({"key" : "base.buoy_mg",
                           "offset_y" : -normal_tile_height / 2});
    }
    if (tile_has_extra(ptile, EXTRA_RUINS)) {
      result_sprites.push({"key" : "extra.ruins_mg",
                           "offset_y" : -normal_tile_height / 2});
    }
  }

  return result_sprites;
}

/****************************************************************************
  ...
****************************************************************************/
function fill_layer3_sprite_array(ptile, pcity)
{
  var result_sprites = [];

  /* We don't draw the bases if there's a city */
  if (pcity == null) {
    if (tile_has_extra(ptile, BASE_FORTRESS)) {
      result_sprites.push({"key" : "base.fortress_fg",
                           "offset_y" : -normal_tile_height / 2});
    }
  }

  return result_sprites;
}

/****************************************************************************
  Assigns the nation's color based on the color of the flag, currently
  the most common color in the flag is chosen.
****************************************************************************/
function assign_nation_color(nation_id)
{

  var nation = nations[nation_id];
  if (nation == null || nation['color'] != null) return;

  var flag_key = "f." + nation['graphic_str'];
  var flag_sprite = sprites[flag_key];
  if (flag_sprite == null) return;
  var c = flag_sprite.getContext('2d');
  var width = tileset[flag_key][2];
  var height = tileset[flag_key][3];
  var color_counts = {};
  /* gets the flag image data, except for the black border. */
  if (c == null) return;
  var img_data = c.getImageData(1, 1, width-2, height-2).data;

  /* count the number of each pixel's color */
  for (var i = 0; i < img_data.length; i += 4) {
    var current_color = "rgb(" + img_data[i] + "," + img_data[i+1] + ","
                        + img_data[i+2] + ")";
    if (current_color in color_counts) {
      color_counts[current_color] = color_counts[current_color] + 1;
    } else {
      color_counts[current_color] = 1;
    }
  }

  var max = -1;
  var max_color = null;

  for (var current_color in color_counts) {
    if (color_counts[current_color] > max) {
      max = color_counts[current_color];
      max_color = current_color;
    }
  }



  nation['color'] = max_color;
  color_counts = null;
  img_data = null;

}

/****************************************************************************
...
****************************************************************************/
function deduplicate_player_colors()
{
  for (var player_id in players) {
    var cplayer = players[player_id];
    var cnation = nations[cplayer['nation']]
    var pcolor = cnation['color'];
    for (var splayer_id in players) {
      var splayer = players[splayer_id];
      var snation = nations[splayer['nation']]
      var scolor = snation['color'];
      if (splayer_id != player_id && is_color_collision(pcolor, scolor)) {
        cnation['color'] = "rgb(" + get_random_int(80,255)  + "," + get_random_int(80,200) + ","
                          + get_random_int(80,200) + ")";
	break;
      }
    }
  }
  palette = generate_palette();
}

/****************************************************************************
...
****************************************************************************/
function is_color_collision(color_a, color_b)
{
  var distance_threshold = 20;

  if (color_a == null || color_b == null) return false;

  var pcolor_a = color_rbg_to_list(color_a);
  var pcolor_b = color_rbg_to_list(color_b);

  var color_distance = Math.sqrt( Math.pow(pcolor_a[0] - pcolor_b[0], 2)
		  + Math.pow(pcolor_a[1] - pcolor_b[1], 2)
		  + Math.pow(pcolor_a[2] - pcolor_b[2], 2));

  return (color_distance <= distance_threshold);
}

/****************************************************************************
...
****************************************************************************/
function color_rbg_to_list(pcolor)
{
  if (pcolor == null) return null;
  var color_rgb = pcolor.match(/\d+/g);
  color_rgb[0] = parseFloat(color_rgb[0]);
  color_rgb[1] = parseFloat(color_rgb[1]);
  color_rgb[2] = parseFloat(color_rgb[2]);
  return color_rgb;
}

