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


var units = {};

var SINGLE_MOVE = 3;

/* Changing this enum will break network compatability. */
var DIPLOMAT_MOVE = 0;	/* move onto city square - only for allied cities */
var DIPLOMAT_EMBASSY = 1;
var DIPLOMAT_BRIBE = 2;
var DIPLOMAT_INCITE = 3;
var DIPLOMAT_INVESTIGATE = 4;
var DIPLOMAT_SABOTAGE = 5;
var DIPLOMAT_STEAL = 6;
var SPY_POISON = 7; 
var SPY_SABOTAGE_UNIT = 8;
var DIPLOMAT_ANY_ACTION = 9;   /* leave this one last */

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
 Updates the index of which units can be found on a tile.
 Note: This must be called after a unit has moved to a new tile.
 See: clear_tile_unit()
**************************************************************************/
function update_tile_unit(punit)
{
  if (punit == null) return;
  
  var found = false;
  var ptile = map_pos_to_tile(punit['x'], punit['y']);
  
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
  var ptile = map_pos_to_tile(punit['x'], punit['y']);
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
  Returns the type of the unit.
**************************************************************************/
function get_unit_moves_left(punit)
{
  if (punit == null) return 0;
  var result = "";
  if ((punit['movesleft'] % SINGLE_MOVE) != 0) {
    if (Math.floor(punit['movesleft'] / SINGLE_MOVE) > 0) {
      result = "Moves: " + Math.floor(punit['movesleft'] / SINGLE_MOVE)
               + " " + Math.floor(punit['movesleft'] % SINGLE_MOVE) 
               + "/" + SINGLE_MOVE;          
    } else {
      result = "Moves: "  
               + Math.floor(punit['movesleft'] % SINGLE_MOVE) 
               + "/" + SINGLE_MOVE;          
    }
  } else {
    result = "Moves: "  + Math.floor(punit['movesleft'] / SINGLE_MOVE);
  }
  return result;  
}

/**************************************************************************
  ...
**************************************************************************/
function unit_has_goto(punit)
{
  return (punit['goto_dest_x'] != 255 && punit['goto_dest_y'] != 255);
}