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



var terrains = {};
var resources = {};
var terrain_control = {};

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
