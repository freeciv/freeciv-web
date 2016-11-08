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

var map_heightmap_resolution = 2048;
var heightmap_palette = [];

/****************************************************************************

****************************************************************************/
function generate_map_heightmap_grid() {
  var row, col;
  var heightmap_resolution_x = map['xsize'] * 4;
  var heightmap_resolution_y = map['ysize'] * 4;

  var grid = Array(heightmap_resolution_x);
  for (row = 0; row < heightmap_resolution_x ; row++) {
    grid[row] = Array(heightmap_resolution_y);
  }

  for (var x = 0; x < heightmap_resolution_x ; x++) {
    for (var y = 0; y < heightmap_resolution_y; y++) {
      var gx = Math.floor((map['ysize'] / map['xsize']) * x * map['xsize'] / heightmap_resolution_x);
      var gy = Math.floor((map['xsize'] / map['ysize']) * y * map['ysize'] / heightmap_resolution_y);
      grid[x][y] = Math.floor(heightmap[gy][gx] * 100);
    }
  }
  return grid;
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