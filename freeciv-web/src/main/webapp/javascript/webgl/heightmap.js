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

var heightmap_resolution = 2048;
var heightmap = null;

/****************************************************************************
  Create heightmap, a 2d array with the height.
****************************************************************************/
function create_heightmap()
{
  var row, col;
  var heightmap_init = new Array(heightmap_resolution);
  for (var i = 0; i < heightmap_resolution; i++) {
    heightmap_init[i] = new Array(heightmap_resolution);
  }

  for (var x = 0; x < heightmap_resolution ; x++) {
    for (var y = 0; y < heightmap_resolution; y++) {
      var gx = Math.floor((map['ysize'] / map['xsize']) * x * map['xsize'] / heightmap_resolution);
      var gy = Math.floor((map['xsize'] / map['ysize']) * y * map['ysize'] / heightmap_resolution);
      var ptile = map_pos_to_tile(gx, gy);
      if (ptile != null) {
        heightmap_init[gx][gy] = map_tile_height(ptile);
      }
    }
  }

  heightmap = new Array(heightmap_resolution);
  for (var i = 0; i < heightmap_resolution; i++) {
    heightmap[i] = new Array(heightmap_resolution);
  }
  /* smooth */
  for (var x = 1; x < heightmap_resolution -1 ; x++) {
    for (var y = 1; y < heightmap_resolution -1; y++) {
      heightmap[x][y] = (heightmap_init[x][y] * 0.5 + heightmap_init[x+1][y] * 0.125 + heightmap_init[x-1][y] * 0.125 + heightmap_init[x][y+1] * 0.125 + heightmap_init[x][y-1] * 0.125);
    }
  }

}