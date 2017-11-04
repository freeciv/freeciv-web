/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

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

var border_image_resolution = 512;
var borders_palette = [];
var borders_texture;
var borders_hash = -1;

/****************************************************************************
 Initialize borders image.
****************************************************************************/
function init_borders_image()
{
  borders_palette = [];
  borders_palette.push([142, 0, 0]);
  for (var player_id in players) {
    var pplayer = players[player_id];
    var nation_colors;
    if (nations[pplayer['nation']].color != null) {
      nation_colors = nations[pplayer['nation']].color.replace("rgb(", "").replace(")", "").split(",");
      if (nation_colors[0] < nation_colors[1] - 10 && nation_colors[2] < nation_colors[1] - 10) nation_colors = [0, 20, 0]; //darken green
    } else {
      nation_colors = [0, 0, 0];
    }
    borders_palette.push([parseInt(nation_colors[0]) * 0.65, parseInt(nation_colors[2]) * 0.65, parseInt(nation_colors[1]) * 0.65]);
  }

  bmp_lib.render('borders_image',
                    generate_borders_image(),
                    borders_palette);
  borders_texture = new THREE.Texture();
  borders_texture.magFilter = THREE.NearestFilter;
  borders_texture.minFilter = THREE.NearestFilter;
  borders_texture.image = document.getElementById("borders_image");
  borders_texture.image.onload = function () {
    borders_texture.needsUpdate = true;
  };

  if (graphics_quality == QUALITY_MEDIUM) setInterval(update_borders_image, 20000);
  if (graphics_quality == QUALITY_HIGH) setInterval(update_borders_image, 10000);

}

/****************************************************************************
  Returns a texture containing one pixel for each map tile, where the color of each pixel
  contains the border color.
****************************************************************************/
function update_borders_image()
{
   var hash = generate_borders_image_hash();

   if (hash != borders_hash) {
     borders_palette = [];
     borders_palette.push([142, 0, 0]);
     for (var player_id in players) {
       var pplayer = players[player_id];
       var nation_colors;
       if (nations[pplayer['nation']].color != null) {
         nation_colors = nations[pplayer['nation']].color.replace("rgb(", "").replace(")", "").split(",");
         if (nation_colors[0] < nation_colors[1] - 10 && nation_colors[2] < nation_colors[1] - 10) nation_colors = [0, 20, 0]; //darken green
       } else {
         nation_colors = [0,0,0];
       }

       borders_palette.push([parseInt(nation_colors[0]) * 0.65, parseInt(nation_colors[2]) * 0.65, parseInt(nation_colors[1]) * 0.65]);
     }

     bmp_lib.render('borders_image',
                       generate_borders_image(),
                       borders_palette);
     borders_texture.image = document.getElementById("borders_image");
     borders_texture.image.onload = function () {
       borders_texture.needsUpdate = true;
     };
     borders_hash = hash;

     return borders_texture;
  }

}

/****************************************************************************

****************************************************************************/
function generate_borders_image() {

  var row, col;
  // The grid of points that make up the image.
  var grid = Array(border_image_resolution);
  for (row = 0; row < border_image_resolution ; row++) {
    grid[row] = Array(border_image_resolution);
  }

  for (var x = 0; x < border_image_resolution ; x++) {
    for (var y = 0; y < border_image_resolution; y++) {
      var gx = Math.floor(map.ysize * x / border_image_resolution);
      var gy = Math.floor(map.xsize * y / border_image_resolution);
      grid[x][y] = border_image_color(gy, gx);
    }
  }

  var result = Array(border_image_resolution);
  for (row = 0; row < border_image_resolution ; row++) {
    result[row] = Array(border_image_resolution);
  }

  for (var x = 0; x < border_image_resolution ; x++) {
    for (var y = 0; y < border_image_resolution; y++) {
      if (x == 0 || y == 0 || x >= border_image_resolution - 1  || y >= border_image_resolution - 1) {
        result[x][y] = grid[x][y];
      } else {
        var is_border = (grid[x][y] > 0
           && (grid[x-1][y-1] != grid[x][y] || grid[x-1][y] != grid[x][y] || grid[x][y-1] != grid[x][y] || grid[x+1][y] != grid[x][y]
            || grid[x][y+1] != grid[x][y] || grid[x+1][y+1] != grid[x][y] || grid[x-1][y+1] != grid[x][y] || grid[x+1][y-1] != grid[x][y]));
        if (is_border) {
          result[x][y] = grid[x][y];
        } else {
          result[x][y] = 0;
        }
      }
    }
  }

  return result;
}


/****************************************************************************
 Creates a hash of the map borders.
****************************************************************************/
function generate_borders_image_hash() {
  var hash = 0;
  var row, col;

  for (var x = 0; x < border_image_resolution ; x++) {
    for (var y = 0; y < border_image_resolution; y++) {
      var gx = Math.floor(map.ysize * x / border_image_resolution);
      var gy = Math.floor(map.xsize * y / border_image_resolution);
      hash += border_image_color(gy, gx);
    }
  }

  return hash;
}


/****************************************************************************
  Returns the color of the tile at the given map position.
****************************************************************************/
function border_image_color(map_x, map_y)
{
  var ptile = map_pos_to_tile(map_x, map_y);

  if (ptile != null && ptile['owner'] != null && ptile['owner'] < 255) {
      return 1 + ptile['owner'];
  }

  return 0;

}