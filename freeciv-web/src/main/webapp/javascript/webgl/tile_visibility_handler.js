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

var normalsNeedsUpdating = false;

/**************************************************************************
...
**************************************************************************/
function webgl_update_tile_known(old_tile, new_tile)
{
  if (unknownTerritoryGeometry == null || new_tile == null || old_tile == null) return;

  if (tile_get_known(new_tile) == tile_get_known(old_tile)) return;

  var tx = old_tile['x'];
  var ty = old_tile['y'];

  /* Update unknown territory */
  for ( var i = 0, l = unknownTerritoryGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    var gx = Math.floor(x / 4);
    var gy = Math.floor(y / 4);

    if (gx != tx || gy != ty) continue;
    if ( new_tile != null) {
      if (tile_get_known(new_tile) == TILE_KNOWN_SEEN) {
        unknownTerritoryGeometry.vertices[ i ].y = 0;
      }
      unknownTerritoryGeometry.verticesNeedUpdate = true;
      normalsNeedsUpdating = true;
    }
  }

  /* Update fog of war */
  for ( var i = 0, l = fogOfWarGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    var gx = Math.floor(x / 4);
    var gy = Math.floor(y / 4);
   if (gx != tx || gy != ty) continue;
    if ( new_tile != null) {
      if (tile_get_known(new_tile) == TILE_KNOWN_SEEN) {
        fogOfWarGeometry.vertices[ i ].y = landGeometry.vertices[ i ].y - 16;
      } else if (tile_get_known(new_tile) == TILE_KNOWN_UNSEEN) {
        fogOfWarGeometry.vertices[ i ].y = landGeometry.vertices[ i ].y + 13;
      } else if (tile_get_known(new_tile) == TILE_UNKNOWN) {
        fogOfWarGeometry.vertices[ i ].y = landGeometry.vertices[ i ].y + 5;
      }
      fogOfWarGeometry.verticesNeedUpdate = true;
      normalsNeedsUpdating = true;
    }
  }
}