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

var vertex_colors_dirty = false;

/**************************************************************************
 Updates the terrain vertex colors to set tile to known, unknown or fogged.
**************************************************************************/
function webgl_update_tile_known(old_tile, new_tile)
{
  if (new_tile == null || old_tile == null || tile_get_known(new_tile) == tile_get_known(old_tile) || landGeometry == null) return;
  vertex_colors_dirty = true;
}



/**************************************************************************
Updates the terrain vertex colors to set irrigation, farmland, or none.
**************************************************************************/
function webgl_update_farmland_irrigation_vertex_colors(ptile)
{
  if (ptile == null || landGeometry == null) return;
  vertex_colors_dirty = true;
}

/**************************************************************************
 This will update the fog of war and unknown tiles, and farmland/irrigation
 by storing these as vertex colors in the landscape mesh.
**************************************************************************/
function update_tiles_known_vertex_colors()
{
  if (!vertex_colors_dirty) return;
  for ( var i = 0; i < landGeometry.faces.length; i ++ ) {
    var v = ['a', 'b', 'c'];
    for (var r = 0; r < v.length; r++) {
      var vertex = landGeometry.vertices[landGeometry.faces[i][v[r]]];
      var vPos = landMesh.localToWorld(vertex);
      var mapPos = scene_to_map_coords(vPos.x, vPos.z);
      if (mapPos['x'] >= 0 && mapPos['y'] >= 0) {
        var ptile = map_pos_to_tile(mapPos['x'], mapPos['y']);
        if (ptile != null && tile_get_known(ptile) != TILE_UNKNOWN) {
          if (landGeometry.faces[i].vertexColors[r] != null) landGeometry.faces[i].vertexColors[r].copy(get_vertex_color_from_tile(ptile));
        }
      }
    }
  }
  landGeometry.colorsNeedUpdate = true;
  vertex_colors_dirty = false;

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
      known_status_color = 0.40;
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
