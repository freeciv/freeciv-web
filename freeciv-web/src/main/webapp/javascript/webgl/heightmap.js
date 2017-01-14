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
  var start_heightmap = new Date().getTime();
  var heightmap_resolution_x = map.xsize * 4 + 1;
  var heightmap_resolution_y = map.ysize * 4 + 1;

  var heightmap_tiles = new Array(map.xsize);
  var distance_from_coast_map = new Array(map.xsize);
  for (var x = 0; x < map.xsize; x++) {
    heightmap_tiles[x] = new Array(map.ysize);
    distance_from_coast_map[x] = new Array(map.ysize);
  }

  /* Here we look at every tile and determine its distance from the sea. */
  /* First of all we set non-sea tiles' distance to be very big. */
  for (var x = 0; x < map.xsize ; x++) {
    for (var y = 0; y < map.ysize; y++) {
      var ptile = map_pos_to_tile(x, y);
      if (ptile != null && tile_terrain(ptile) != null && !is_ocean_tile(ptile)) {
        distance_from_coast_map[x][y] = 100000;
      } else {
        distance_from_coast_map[x][y] = 0;
      }
    }
  }
  /* Then, for each sea tile, we propagate the distance information to the inner
   * land. */
  for (var x = 0; x < map.xsize; x++) {
    for (var y = 0; y < map.ysize; y++) {
      var ptile = map_pos_to_tile(x, y);
      if (ptile != null && tile_terrain(ptile) != null && is_ocean_tile(ptile)) {
        propagate_distance_from_coast(distance_from_coast_map, x, y);
      }
    }
  }

  for (var x = 0; x < map.xsize; x++) {
    for (var y = 0; y < map.ysize; y++) {
      var ptile = map_pos_to_tile(x, y);
      if (ptile != null) {
        heightmap_tiles[x][y] = 0.5;
        heightmap_tiles[x][y] += 0.4 * map_tile_height(ptile);
        heightmap_tiles[x][y] += 0.1 * distance_from_coast_map[x][y];
        if (ptile != null && heightmap_tiles[x][y] > ptile['height']) ptile['height'] = heightmap_tiles[x][y];
      }
    }
  }

  heightmap = new Array(heightmap_resolution_x);
  for (var hx = 0; hx < heightmap_resolution_x; hx++) {
    heightmap[hx] = new Array(heightmap_resolution_y);
  }

  /* smooth */
  for (var x = 0; x < heightmap_resolution_x; x++) {
    for (var y = 0; y < heightmap_resolution_y; y++) {
      var gx = x / 4 - 0.5;
      var gy = y / 4 - 0.5;

      if (Math.round(gx) == gx && Math.round(gy) == gy) {
        /* On tile center */
        heightmap[x][y] = heightmap_tiles[gx][gy];
      } else {
        /* Interpolate between neighbouring tiles, with each tile having weight:
         * 1 / (distance to the tile)^2.
         */
        var neighbours = [
          { "x": Math.floor(gx), "y": Math.floor(gy) },
          { "x": Math.floor(gx), "y": Math.ceil(gy) },
          { "x": Math.ceil(gx),  "y": Math.floor(gy) },
          { "x": Math.ceil(gx),  "y": Math.ceil(gy) }];

        var norm = 0;
        var sum = 0;
        for (var i = 0; i < 4; i++) {
          var coords = neighbours[i];
          if (coords.x < 0 || coords.x >= map.xsize ||
              coords.y < 0 || coords.y >= map.ysize) {
            /* Outside of map, don't use in the sum */
            continue;
          }
          var dx = gx - coords.x;
          var dy = gy - coords.y;
          var distance = Math.sqrt(dx*dx + dy*dy);
          sum += heightmap_tiles[coords.x][coords.y] / distance / distance;
          norm += 1. / distance / distance;
        }
        // set final heightmap value, and add some random noise.
        heightmap[x][y] = (sum / norm) + ((Math.random() - 0.5) / 55);
      }
    }
  }
  console.log("create_heightmap took: " + (new Date().getTime() - start_heightmap) + " ms.");

}

/****************************************************************************
  ...
****************************************************************************/
function propagate_distance_from_coast(distance_from_coast_map, x, y)
{
  var current_distance = distance_from_coast_map[x][y];

  // limit distance from coast to 2. this is a quick-fix for inland tiles
  // incorrectly assigned as mountains by the fragment shader.
  if (current_distance >= 2) current_distance = 2;

  for (var dir = 0; dir < DIR8_LAST; dir++) {
    var dir_x = x + DIR_DX[dir];
    var dir_y = y + DIR_DY[dir];
    if (dir_x < 0 || dir_x >= map.xsize || dir_y < 0 || dir_y >= map.ysize) {
      /* Outside of the map */
      continue;
    }
    if (distance_from_coast_map[dir_x][dir_y] > current_distance + 1) {
      distance_from_coast_map[dir_x][dir_y] = current_distance + 1;
      propagate_distance_from_coast(distance_from_coast_map, dir_x, dir_y);
    }
  }
}

/****************************************************************************
  Returns the tile height from -1.0 to 1.0
****************************************************************************/
function map_tile_height(ptile)
{
  if (ptile != null && tile_terrain(ptile) != null) {
      if (tile_terrain(ptile)['name'] == "Deep Ocean") return -0.15;
      if (is_ocean_tile(ptile)) return -0.1;
      if (tile_terrain(ptile)['name'] == "Hills") return 0.45;
      if (tile_terrain(ptile)['name'] == "Mountains") return 1.0;
  }

  return 0.0;
}
