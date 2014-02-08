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

var EXTRA_IRRIGATION = 0;
var EXTRA_MINE = 1;
var EXTRA_POLLUTION = 2;
var EXTRA_HUT = 3;
var EXTRA_FARMLAND = 4;
var EXTRA_FALLOUT = 5;

var BASE_FORTRESS = 6;
var BASE_AIRBASE = 7;
var BASE_BUOY = 8;
var BASE_RUINS = 9;

var ROAD_ROAD  = 10;
var ROAD_RAIL  = 11;
var ROAD_RIVER = 12;

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
