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
...
**************************************************************************/
function webgl_update_tile_known(old_tile, new_tile)
{
  if (new_tile == null || old_tile == null || tile_get_known(new_tile) == tile_get_known(old_tile) || landGeometry == null) return;

  var tx = old_tile['x'];
  var ty = old_tile['y'];

  for (var i = 0; i < map_index_to_face[tx + "." + ty].length; i++) {
    var face = map_index_to_face[tx + "." + ty][i];

    if (tile_get_known(new_tile) == TILE_KNOWN_SEEN) {
      face.color.copy(new THREE.Color(1,0,0));
    } else if (tile_get_known(new_tile) == TILE_KNOWN_UNSEEN) {
      face.color.copy(new THREE.Color(0.5,0,0));
    } else if (tile_get_known(new_tile) == TILE_UNKNOWN) {
      face.color.copy(new THREE.Color(0,0,0));
    }
  }

  landGeometry.colorsNeedUpdate = true;

}


