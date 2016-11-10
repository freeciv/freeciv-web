/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2016  The Freeciv-web project

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


var heightmap = null;

/****************************************************************************
  Create heightmap, a 2d array with the height.
****************************************************************************/
function create_heightmap()
{
  var heightmap_resolution_x = map['xsize'] * 4 + 1;
  var heightmap_resolution_y = map['ysize'] * 4 + 1;
  var row, col;
  var heightmap_init = new Array(heightmap_resolution_x);
  for (var i = 0; i <= heightmap_resolution_x; i++) {
    heightmap_init[i] = new Array(heightmap_resolution_y);
  }

  for (var x = 0; x < heightmap_resolution_x ; x++) {
    for (var y = 0; y < heightmap_resolution_y; y++) {
      var gx = Math.floor(x * map['xsize'] / heightmap_resolution_x);
      var gy = Math.floor((map['xsize'] / map['ysize']) * y * map['ysize'] / heightmap_resolution_y) - 1;
      var ptile = map_pos_to_tile(gx, gy);
      if (ptile != null) {
        heightmap_init[gx][gy] = map_tile_height(ptile);
      }
    }
  }

  heightmap = new Array(heightmap_resolution_x);
  for (var i = 0; i < heightmap_resolution_x; i++) {
    heightmap[i] = new Array(heightmap_resolution_y);
  }
  /* smooth */
  for (var x = 0; x < heightmap_resolution_x; x++) {
    for (var y = 0; y < heightmap_resolution_y; y++) {
      if (x == 0 || y == 0 || x >= heightmap_resolution_x || y >= heightmap_resolution_y) {
        heightmap[x][y] = heightmap_init[x][y];
      } else {
        heightmap[x][y] = (heightmap_init[x][y] * 0.5 + heightmap_init[x+1][y] * 0.125 + heightmap_init[x-1][y] * 0.125 + heightmap_init[x][y+1] * 0.125 + heightmap_init[x][y-1] * 0.125);
      }
    }
  }

}

/****************************************************************************
  Returns the tile height from 0 to 1.0
****************************************************************************/
function map_tile_height(ptile)
{
  if (ptile != null && tile_get_known(ptile) != TILE_UNKNOWN) {
      if (is_ocean_tile(ptile)) return 0.2;
      if (tile_terrain(ptile)['name'] == "Hills") return 0.8;
      if (tile_terrain(ptile)['name'] == "Mountains") return 1.0;
  }

  return 0.7;

}