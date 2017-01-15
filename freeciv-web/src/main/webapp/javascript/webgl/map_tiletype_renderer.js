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

var map_tiletype_resolution;
var tiletype_palette = [];

/****************************************************************************

****************************************************************************/
function generate_map_tiletype_grid() {

  if (graphics_quality == QUALITY_LOW) map_tiletype_resolution = 1024;
  if (graphics_quality == QUALITY_MEDIUM) map_tiletype_resolution = 2048;
  if (graphics_quality == QUALITY_HIGH) map_tiletype_resolution = 4096;

  var row, col;
  var start_tiletype = new Date().getTime();
  // The grid of points that make up the image.
  var grid = Array(map_tiletype_resolution);
  for (row = 0; row < map_tiletype_resolution ; row++) {
    grid[row] = Array(map_tiletype_resolution);
  }

  for (var x = 0; x < map_tiletype_resolution ; x++) {
    for (var y = 0; y < map_tiletype_resolution; y++) {
      var gx = Math.floor(map.ysize * x / map_tiletype_resolution);
      var gy = Math.floor(map.xsize * y / map_tiletype_resolution);
      grid[x][y] = map_tiletype_tile_color(gy, gx);
    }
  }

  // randomize tile edges
  var num_iterations;
  if (graphics_quality == QUALITY_LOW) num_iterations = 1;
  if (graphics_quality == QUALITY_MEDIUM) num_iterations = 2;
  if (graphics_quality == QUALITY_HIGH) num_iterations = 3;
  for (var i = 0; i < num_iterations; i++) {
    for (var x = 0; x < map_tiletype_resolution - 2; x++) {
      for (var y = 0; y < map_tiletype_resolution - 2; y++) {
        if (Math.random() >= 0.6) {
           grid[x][y] = grid[x + 2][y + 2];
        }
      }
    }
  }

  console.log("generate_map_tiletype_grid took: " + (new Date().getTime() - start_tiletype) + " ms.");
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