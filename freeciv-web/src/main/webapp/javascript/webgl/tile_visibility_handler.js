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

/**************************************************************************
...
**************************************************************************/
function webgl_update_tile_known(old_tile, new_tile)
{
  if (unknownTerritoryGeometry == null || new_tile == null || old_tile == null) return;

  /* TODO: don't update anything if nothing has changed. */

  var tx = old_tile['x'];
  var ty = old_tile['y'];

  /* Update unknown territory */
  for ( var i = 0, l = unknownTerritoryGeometry.vertices.length; i < l; i ++ ) {
    var x = i % xquality, y = Math.floor( i / xquality );
    var gx = Math.floor(x / 4);
    var gy = Math.floor(y / 4);

    if (gx != tx || gy != ty) continue;
    if (heightmap[x] != null && heightmap[x][y] != null && new_tile != null) {
      if (tile_get_known(new_tile) != TILE_UNKNOWN) {
        unknownTerritoryGeometry.vertices[ i ].y = 0;
      } else {
        unknownTerritoryGeometry.vertices[ i ].y = heightmap[x][y] * 100 + 26;
      }
      unknownTerritoryGeometry.verticesNeedUpdate = true;
    }
  }


}