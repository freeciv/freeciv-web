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

var map_index_to_face = {};  // a map where the key is x.y and the value is a Tree.js Face

/**************************************************************************
 Updates the terrain vertex colors to set tile to known, unknown or fogged.
**************************************************************************/
function webgl_update_tile_known(old_tile, new_tile)
{
  if (new_tile == null || old_tile == null || tile_get_known(new_tile) == tile_get_known(old_tile) || landGeometry == null) return;

  var tx = old_tile['x'];
  var ty = old_tile['y'];

  for (var i = 0; i < map_index_to_face[tx + "." + ty].length; i++) {
    var face = map_index_to_face[tx + "." + ty][i];
    face.color.copy(get_vertex_color_from_tile(new_tile));
  }

  landGeometry.colorsNeedUpdate = true;

}



/**************************************************************************
Updates the terrain vertex colors to set irrigation, farmland, or none.
**************************************************************************/
function webgl_update_farmland_irrigation_vertex_colors(ptile)
{
  if (ptile == null || landGeometry == null) return;

  var tx = ptile['x'];
  var ty = ptile['y'];

  for (var i = 0; i < map_index_to_face[tx + "." + ty].length; i++) {
    var face = map_index_to_face[tx + "." + ty][i];
    var new_color = get_vertex_color_from_tile(ptile);
    new_color.r = face.color.r; // don't update tile known status here.
    face.color.copy(new_color);
  }

  landGeometry.colorsNeedUpdate = true;

}



/**************************************************************************
 Returns the vertex colors (THREE.Color) of a tile. The color is used to
 set terrain type in the terrain fragment shader.
**************************************************************************/
function get_vertex_color_from_tile(ptile)
{
    var known_status_color = 0;
    if (tile_get_known(ptile) == TILE_KNOWN_SEEN) {
      known_status_color = 1;
    } else if (tile_get_known(ptile) == TILE_KNOWN_UNSEEN) {
      known_status_color = 0.5;
    } else if (tile_get_known(ptile) == TILE_UNKNOWN) {
      known_status_color = 0;
    }

    var farmland_irrigation_color = 0;
    if (tile_has_extra(ptile, EXTRA_FARMLAND)) {
      farmland_irrigation_color = 1.0;
    } else if (tile_has_extra(ptile, EXTRA_IRRIGATION)) {
      farmland_irrigation_color = 0.5;
    } else {
      farmland_irrigation_color = 0;
    }

    return new THREE.Color(known_status_color, farmland_irrigation_color,0);

}
