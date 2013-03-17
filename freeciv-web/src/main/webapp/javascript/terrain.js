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



var terrains = {};
var resources = {};
var terrain_control = {};

var S_IRRIGATION = 0;
var S_MINE = 1;
var S_POLLUTION = 2;
var S_HUT = 3;
var S_RIVER = 4;
var S_FARMLAND = 5;
var S_FALLOUT = 6;

  /* internal values not saved */
var S_LAST = 7;
var S_RESOURCE_VALID = S_LAST;

var ROAD_ROAD  = 0;
var ROAD_RAIL  = 1;
var ROAD_RIVER = 2;

/**************************************************************************
 ...
**************************************************************************/
function tile_set_terrain(ptile, pterrain)
{
  ptile['terrain'] = pterrain;
}

/**************************************************************************
 ...
**************************************************************************/
function tile_terrain(ptile)
{ 
  return terrains[ptile['terrain']];
}

/**************************************************************************
 ...
**************************************************************************/
function tile_terrain_near(ptile)
{
  var tterrain_near = [];
  for (var dir = 0; dir < 8; dir++) {
    var tile1 = mapstep(ptile, dir);
    if (tile1 != null && tile_get_known(tile1) != TILE_UNKNOWN) {
      var terrain1 = tile_terrain(tile1);

      if (null != terrain1) {
        tterrain_near[dir] = terrain1;
        continue;
      }
      freelog(LOG_ERROR, "build_tile_data() tile (%d,%d) has no terrain!",
              TILE_XY(tile1));
    }
    /* At the edges of the (known) map, pretend the same terrain continued
     * past the edge of the map. */
    tterrain_near[dir] = tile_terrain(ptile);
    // FIXME: BV_CLR_ALL(tspecial_near[dir]);
  }
  
  return tterrain_near;
}

/**************************************************************************
 ...
**************************************************************************/
function is_ocean_tile(ptile) 
{
  var pterrain = tile_terrain(ptile); 
  return (pterrain['graphic_str'] == "floor" || pterrain['graphic_str'] == "coast");
}


/**************************************************************************
 ...
**************************************************************************/
function contains_special(ptile, special_id)
{
  if (ptile != null && ptile['special'] != null) {
    return ptile['special'][special_id]; 
  }
  return false;
}
