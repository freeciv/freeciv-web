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


var units = {};

/* Depends on the ruleset. Comes in the packet ruleset_terrain_control.
 * Set in handle_ruleset_terrain_control(). */
var SINGLE_MOVE;

var ANIM_STEPS = 8;

var anim_units_max = 30;
var anim_units_count = 0;

/* The unit_orders enum from unit.h */
var ORDER_MOVE = 0;
var ORDER_ACTIVITY = 1;
var ORDER_FULL_MP = 2;
var ORDER_ACTION_MOVE = 3;
var ORDER_PERFORM_ACTION = 4;
var ORDER_LAST = 5;

/* The unit_ss_data_type enum from unit.h */
var USSDT_QUEUE = 0;
var USSDT_UNQUEUE = 1;
var USSDT_BATTLE_GROUP = 2;

/****************************************************************************
 ...
****************************************************************************/
function idex_lookup_unit(id)
{
  return units[id];
}

/****************************************************************************
 ...
****************************************************************************/
function unit_owner(punit)
{
  return player_by_number(punit['owner']);
}


/****************************************************************************
 ...
****************************************************************************/
function client_remove_unit(punit)
{
  if (unit_is_in_focus(punit)) {
    current_focus = [];
    if (renderer == RENDERER_WEBGL) webgl_clear_unit_focus();
  }

  delete units[punit['id']];
}

/**************************************************************************
 Returns a list of units on the given tile. See update_tile_unit().
**************************************************************************/
function tile_units(ptile)
{
  if (ptile == null) return null;
  return ptile['units'];
}

/**************************************************************************
 Returns a list of units supported by this city.
**************************************************************************/
function get_supported_units(pcity)
{
  if (pcity == null) return null;
  var result = [];
  for (var unit_id in units) {
    var punit = units[unit_id];
    if (punit['homecity'] == pcity['id']) {
      result.push(punit);
    }
  }
  return result;
}


/**************************************************************************
 Updates the index of which units can be found on a tile.
 Note: This must be called after a unit has moved to a new tile.
 See: clear_tile_unit()
**************************************************************************/
function update_tile_unit(punit)
{
  if (punit == null) return;

  var found = false;
  var ptile = index_to_tile(punit['tile']);

  if (ptile == null || ptile['units'] == null) return;

  for (var i = 0; i <  ptile['units'].length; i++) {
    if (ptile['units'][i]['id'] == punit['id']) {
      found = true;
    }
  }

  if (!found) {
    ptile['units'].push(punit);
  }

}

/**************************************************************************
 Updates the index of which units can be found on a tile.
 Note: This must be called before a unit has moved to a new tile.
**************************************************************************/
function clear_tile_unit(punit)
{
  if (punit == null) return;
  var ptile = index_to_tile(punit['tile']);
  if (ptile == null || ptile['units'] == null) return -1;

  if (ptile['units'].indexOf(punit) >= 0) {
    ptile['units'].splice(ptile['units'].indexOf(punit), 1);
  }

}

/**************************************************************************
  Returns the length of the unit list
**************************************************************************/
function unit_list_size(unit_list)
{
  if (unit_list == null) return 0;
  return unit_list.length;
}

/**************************************************************************
  Returns the type of the unit.
**************************************************************************/
function unit_type(unit)
{
  return unit_types[unit['type']];
}

/**************************************************************************
  Returns a string saying how many moves a unit has left.
**************************************************************************/
function get_unit_moves_left(punit)
{
  if (punit == null) {
    return 0;
  }

  return "Moves:" + move_points_text(punit['movesleft']);
}

/**************************************************************************
  Returns an amount of movement formated for player readability.
**************************************************************************/
function move_points_text(moves)
{
  var result = "";

  if ((moves % SINGLE_MOVE) != 0) {
    if (Math.floor(moves / SINGLE_MOVE) > 0) {
      result = Math.floor(moves / SINGLE_MOVE)
               + " " + Math.floor(moves % SINGLE_MOVE)
               + "/" + SINGLE_MOVE;
    } else {
      result = Math.floor(moves % SINGLE_MOVE)
               + "/" + SINGLE_MOVE;
    }
  } else {
    result = Math.floor(moves / SINGLE_MOVE);
  }

  return result;
}

/**************************************************************************
  ...
**************************************************************************/
function unit_has_goto(punit)
{
  /* don't show goto activity for enemy units. I'm not 100% sure this is correct.  */
  if (client.conn.playing == null || punit['owner'] != client.conn.playing.playerno) {
    return false;
  }

  return (punit['goto_tile'] != -1);
}


/**************************************************************************
  Determines the unit_anim_list for the specified unit (old and new unit).
**************************************************************************/
function update_unit_anim_list(old_unit, new_unit)
{
  var anim_tuple;
  if (old_unit == null || new_unit == null) return;
  /* unit is in same position. */
  if (new_unit['tile'] == old_unit['tile']) return;

  if (anim_units_count > anim_units_max) return;

  if (renderer == RENDERER_2DCANVAS && !is_unit_visible(new_unit)) return;

  if (old_unit['anim_list'] == null) old_unit['anim_list'] = [];

  if (new_unit['transported'] == true) {
    /* don't show transported units animated. */
    old_unit['anim_list'] = [];
    return;
  }

  anim_units_count += 1;
  var has_old_pos = false;
  var has_new_pos = false;
  for (var i = 0; i <  old_unit['anim_list'].length; i++) {
    anim_tuple = old_unit['anim_list'][i];
    if (anim_tuple['tile'] == old_unit['tile']) {
      has_old_pos = true;
    }
    if (anim_tuple['tile'] == new_unit['tile']) {
      has_new_pos = true;
    }
  }

  if (!has_old_pos) {
    anim_tuple = {};
    anim_tuple['tile'] = old_unit['tile'];
    anim_tuple['i'] = ANIM_STEPS;
    old_unit['anim_list'].push(anim_tuple);
  }

  if (!has_new_pos) {
    anim_tuple = {};
    anim_tuple['tile'] = new_unit['tile'];
    anim_tuple['i'] = ANIM_STEPS;
    old_unit['anim_list'].push(anim_tuple);
  }
}

/**************************************************************************
  Determines the pixel offset for the specified unit, based on the units
  anim list. This is how Freeciv-web does unit animations.
**************************************************************************/
function get_unit_anim_offset(punit)
{
  var offset = {};

  if (renderer == RENDERER_WEBGL) {
    offset['x'] = 0;
    offset['y'] = 0;
    return offset;
  }

  if (punit['anim_list'] != null && punit['anim_list'].length >= 2)  {
    var anim_tuple_src = punit['anim_list'][0];
    var anim_tuple_dst = punit['anim_list'][1];
    var src_tile = index_to_tile(anim_tuple_src['tile']);
    var dst_tile = index_to_tile(anim_tuple_dst['tile']);
    var u_tile = index_to_tile(punit['tile']);

    anim_tuple_dst['i'] = anim_tuple_dst['i'] - 1;

    var i = Math.floor((anim_tuple_dst['i'] + 2 ) / 3);

    var r = map_to_gui_pos( src_tile['x'], src_tile['y']);
    var src_gx = r['gui_dx'];
    var src_gy = r['gui_dy'];

    var s = map_to_gui_pos(dst_tile['x'], dst_tile['y']);
    var dst_gx = s['gui_dx'];
    var dst_gy = s['gui_dy'];

    var t = map_to_gui_pos(u_tile['x'], u_tile['y']);
    var punit_gx = t['gui_dx'];
    var punit_gy = t['gui_dy'];

    var gui_dx = Math.floor((dst_gx - src_gx) * (i / ANIM_STEPS)) + (punit_gx - dst_gx);
    var gui_dy = Math.floor((dst_gy - src_gy) * (i / ANIM_STEPS)) + (punit_gy - dst_gy);


    if (i == 0) {
      punit['anim_list'].splice(0, 1);
      if (punit['anim_list'].length == 1) {
        punit['anim_list'].splice(0, 1);
      }
    }


    offset['x'] = - gui_dx ;
    offset['y'] = - gui_dy;


  } else {
    offset['x'] = 0;
    offset['y'] = 0;
    anim_units_count -= 1;
  }
  return offset;
}

/**************************************************************************
 Resets the unit anim list, every turn.
**************************************************************************/
function reset_unit_anim_list()
{
 for (var unit_id in units) {
    var punit = units[unit_id];
    punit['anim_list'] = [];
  }
  anim_units_count = 0;
}

/**************************************************************************
  Returns the name of the unit's homecity.
**************************************************************************/
function get_unit_homecity_name(punit)
{
  if (punit['homecity'] != 0 && cities[punit['homecity']] != null) {
    return decodeURIComponent(cities[punit['homecity']]['name']);
  } else {
    return null;
  }

}

/**************************************************************************
  Determines if unit is visible
**************************************************************************/
function is_unit_visible(punit)
{
  if (punit == null || punit['tile'] == null) return false;
  if (renderer == RENDERER_WEBGL) return false;  // not supported by 3D version.

  var u_tile = index_to_tile(punit['tile']);
  var r = map_to_gui_pos(u_tile['x'], u_tile['y']);
  var unit_gui_x = r['gui_dx'];
  var unit_gui_y = r['gui_dy'];

  if (unit_gui_x < mapview['gui_x0'] || unit_gui_y < mapview['gui_y0']
      || unit_gui_x > mapview['gui_x0'] + mapview['width']
      || unit_gui_y > mapview['gui_y0'] + mapview['height']) {
    return false;
  } else {
    return true;
  }

}

/**************************************************************************
 Returns a list containing the unittype ids sorted by unittype name.
**************************************************************************/
function unittype_ids_alphabetic()
{
  var unittype_names = [];
  var unit_type;
  var unit_id;
  for (unit_id in unit_types) {
    var punit_type = unit_types[unit_id];
    unittype_names.push(punit_type['name']);
  }

  unittype_names.sort();

  var unittype_id_list = [];
  for (var n in unittype_names) {
    var unit_name = unittype_names[n];
    for (unit_id in unit_types) {
      punit_type = unit_types[unit_id];
      if (unit_name == punit_type['name']) {
        unittype_id_list.push(unit_id);
      }
    }
  }
  return unittype_id_list;
}

/**************************************************************************
 Returns a text about the unit to be shown in the city dialog, containing
 unit type name, home city, upkeep.
**************************************************************************/
function get_unit_city_info(punit)
{
  var result = "";

  var ptype = unit_type(punit);

  result += ptype['name'] + "\nFood/Shield/Gold: ";

  if (punit['upkeep'] != null) {
    result += punit['upkeep'][O_FOOD] + "/"
           + punit['upkeep'][O_SHIELD] + "/"
           + punit['upkeep'][O_GOLD];
  }

  result += "\n" + get_unit_moves_left(punit) + "\n";

  if (get_unit_homecity_name(punit) != null) {
    result += get_unit_homecity_name(punit);
  }

  return result;
}

/**************************************************************************
 Returns a list of extras a unit can pillage from a tile.
 It is empty if the unit can't pillage or there's nothing to.
 Contains just EXTRA_NONE if there's something but the unit can't choose.
**************************************************************************/
function get_what_can_unit_pillage_from(punit, ptile)
{
  var i, j;
  var extra;
  var targets = [];
  if (punit == null) return targets;

  /* If no tile is given, use the one the unit is on */
  if (ptile == null) {
    ptile = index_to_tile(punit.tile);
  }

  if (terrains[ptile.terrain].pillage_time == 0) return targets;
  var unit_class = unit_classes[unit_types[punit.type].unit_class_id];
  if (!unit_class.flags.isSet(UCF_CAN_PILLAGE)) return targets;

  var available = ptile.extras.toBitSet();
  var cannot_pillage = new BitVector([]);

  /* Get what other units are pillaging on the tile */
  for (unit in ptile.units) {
    if (unit.activity == ACTIVITY_PILLAGE) {
      cannot_pillage.set(unit.activity_tgt);
    }
  }

  /* Get what extras that are dependencies of other */
  for (i = 0; i < available.length; i++) {
    extra = extras[available[i]];
    for (j = 0; j < extra.reqs.length; j++) {
      var req = extra.reqs[j];
      if (req.kind == VUT_EXTRA && req.present == true) {
        cannot_pillage.set(req.value);
      }
    }
  }

  // TODO: more things to check?

  for (i = 0; i < available.length; i++) {
    extra = available[i];
    if ((extras[extra].rmcauses & (1 << ERM_PILLAGE))
        && !cannot_pillage.isSet(extra)) {
      if (game_info.pillage_select) {
        targets.push(extra);
      } else {
        targets.push(EXTRA_NONE);
        break;
      }
    }
  }

  return targets;
}

