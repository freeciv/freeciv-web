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
var tiletype_hash = -1;
var map_texture;

/****************************************************************************
  Returns a texture containing each map tile, where the color of each pixel
  indicates which Freeciv tile type the pixel is.
****************************************************************************/
function init_map_tiletype_image()
{
  for (var terrain_id in terrains) {
    tiletype_palette.push([terrain_id * 10, 0, 0]);    //no river
    tiletype_palette.push([terrain_id * 10, 10, 0]);   //river
  }
  bmp_lib.render('map_tiletype_grid',
                    generate_map_tiletype_grid(),
                    tiletype_palette);
  map_texture = new THREE.Texture();
  map_texture.magFilter = THREE.NearestFilter;
  map_texture.minFilter = THREE.NearestFilter;
  map_texture.image = document.getElementById("map_tiletype_grid");
  map_texture.image.onload = function () {
     map_texture.needsUpdate = true;
  };

  if (graphics_quality == QUALITY_LOW) setInterval(update_tiletypes_image, 120000);
  if (graphics_quality == QUALITY_MEDIUM) setInterval(update_tiletypes_image, 80000);
  if (graphics_quality == QUALITY_HIGH) setInterval(update_tiletypes_image, 60000);

  tiletype_hash = generate_tiletype_hash();

  return map_texture;
 }

/****************************************************************************

****************************************************************************/
function generate_map_tiletype_grid() {

  if (graphics_quality == QUALITY_LOW) map_tiletype_resolution = 1024;
  if (graphics_quality == QUALITY_MEDIUM) map_tiletype_resolution = 2048;
  if (graphics_quality == QUALITY_HIGH) map_tiletype_resolution = 4096;

  var row, col;
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
  var change_probability = 0.6;
  if (graphics_quality == QUALITY_LOW) {
    num_iterations = 0;
  }
  if (graphics_quality == QUALITY_MEDIUM) {
    num_iterations = 2;
  }
  if (graphics_quality == QUALITY_HIGH) {
    num_iterations = 3;
  }
  for (var i = 0; i < num_iterations; i++) {
    for (var x = 0; x < map_tiletype_resolution - 2; x++) {
      for (var y = 0; y < map_tiletype_resolution - 2; y++) {
        if (Math.random() >= change_probability) {
           grid[x][y] = grid[x + 2][y + 2];
        }
      }
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

  if (ptile != null && tile_terrain(ptile) != null && !tile_has_extra(ptile, EXTRA_RIVER)) {
      return tile_terrain(ptile)['id'] * 2;
  } else if (ptile != null && tile_terrain(ptile) != null && tile_has_extra(ptile, EXTRA_RIVER)) {
    return tile_terrain(ptile)['id'] * 2 + 1;
  }

  return COLOR_OVERVIEW_UNKNOWN;

}


/****************************************************************************
  ...
****************************************************************************/
function update_tiletypes_image()
{
   var hash = generate_tiletype_hash();
   if (hash != tiletype_hash) {
     bmp_lib.render('map_tiletype_grid',
                    generate_map_tiletype_grid(),
                    tiletype_palette);
     map_texture.image = document.getElementById("map_tiletype_grid");
     map_texture.image.onload = function () {
       map_texture.needsUpdate = true;
     };
     tiletype_hash = hash;
  }

   return roads_texture;
}

/****************************************************************************
 Creates a hash of the map tiletypes.
****************************************************************************/
function generate_tiletype_hash() {
  var hash = 0;

  for (var x = 0; x < map.xsize ; x++) {
    for (var y = 0; y < map.ysize; y++) {
      hash += map_tiletype_tile_color(x, y);
    }
  }
  return hash;
}