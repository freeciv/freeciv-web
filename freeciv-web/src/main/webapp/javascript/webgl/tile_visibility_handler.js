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
  if (new_tile == null || old_tile == null || tile_get_known(new_tile) == tile_get_known(old_tile)) return;

  var tx = old_tile['x'];
  var ty = old_tile['y'];

  /* Update unknown territory */
  if (fogOfWarGeometry != null && unknown_terrain_mesh != null) {
    for ( var i = 0, l = fogOfWarGeometry.vertices.length; i < l; i ++ ) {

      if (Math.floor((i % xquality) / 4) != tx || Math.floor(Math.floor( i / xquality ) / 4) != ty) continue;

      if (unknownTerritoryGeometry != null && tile_get_known(new_tile) == TILE_KNOWN_SEEN) {
        unknownTerritoryGeometry.vertices[i].y = 0;
        unknownTerritoryGeometry.verticesNeedUpdate = true;
        normalsNeedsUpdating = true;
      }

      if (tile_get_known(new_tile) == TILE_KNOWN_SEEN) {
        fogOfWarGeometry.vertices[i].y = landGeometry.vertices[ i ].y - 16;
      } else if (tile_get_known(new_tile) == TILE_KNOWN_UNSEEN) {
        fogOfWarGeometry.vertices[i].y = landGeometry.vertices[ i ].y + 13;
      } else if (tile_get_known(new_tile) == TILE_UNKNOWN) {
        fogOfWarGeometry.vertices[i].y = landGeometry.vertices[ i ].y + 5;
      }
      fogOfWarGeometry.verticesNeedUpdate = true;
      normalsNeedsUpdating = true;
    }
  }

}

/**************************************************************************
 Check if we can remove  unknown_terrain_mesh because whole map is known.
**************************************************************************/
function check_remove_unknown_territory()
{
  for (var x = 0; x < map['xsize']; x++) {
    for (var y = 0; y < map['ysize']; y++) {
      var ptile = tiles[x + y * map['xsize']];
      if (tile_get_known(ptile) != TILE_KNOWN_SEEN) {
        setTimeout(check_remove_unknown_territory, 120000);
        return;
      }
    }
  }

  if (unknown_terrain_mesh != null && unknownTerritoryGeometry != null) {
    scene.remove(unknown_terrain_mesh);
    unknownTerritoryGeometry.dispose();
    unknown_terrain_mesh = null;
    unknownTerritoryGeometry = null;
  }
}
