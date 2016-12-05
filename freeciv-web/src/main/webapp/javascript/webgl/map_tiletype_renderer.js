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

var map_tiletype_resolution = 2048;
var tiletype_palette = [];

/****************************************************************************

****************************************************************************/
function generate_map_tiletype_grid() {
  var row, col;

  // The grid of points that make up the image.
  var grid = Array(map_tiletype_resolution);
  for (row = 0; row < map_tiletype_resolution ; row++) {
    grid[row] = Array(map_tiletype_resolution);
  }

  for (var x = 0; x < map_tiletype_resolution ; x++) {
    for (var y = 0; y < map_tiletype_resolution; y++) {
      var gx = Math.floor((map['ysize'] / map['xsize']) * x * map['xsize'] / map_tiletype_resolution);
      var gy = Math.floor((map['xsize'] / map['ysize']) * y * map['ysize'] / map_tiletype_resolution);
      grid[x][y] = map_tiletype_tile_color(gy, gx);
    }
  }
  return grid;
}


/****************************************************************************
  Returns the color of the tile at the given map position.
****************************************************************************/
function map_tiletype_tile_color(map_x, map_y)
{
  var ptile = map_pos_to_tile(map_x, map_y);

  if (ptile != null && tile_terrain(ptile) != null) {
      return tile_terrain(ptile)['id'];
  }

  return COLOR_OVERVIEW_UNKNOWN;

}