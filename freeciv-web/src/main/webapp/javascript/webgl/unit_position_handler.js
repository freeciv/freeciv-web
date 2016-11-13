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

var unit_positions = {};  // tile index is key, unit 3d model is value.

/****************************************************************************
...
****************************************************************************/
function update_unit_position(ptile) {
  var visible_unit = find_visible_unit(ptile);

  if (unit_positions[ptile['index']] != null && visible_unit == null) {
    // tile has no visible units, remove it from unit_positions.
    delete unit_positions[ptile['index']];
  }

  if (unit_positions[ptile['index']] == null && visible_unit != null) {
    // add new unit to the unit_positions
    var new_unit = webgl_models["settler"].clone()
    unit_positions[ptile['index']] = new_unit;
    //ptile['x']
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    new_unit.position = new THREE.Vector3(pos['x'], 100, pos['y']);
    //new_unit.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), 100);
    //new_unit.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), 1000);
  }

}

/****************************************************************************
...
****************************************************************************/
function add_all_units_to_scene()
{
  for (var tile_index in unit_positions) {
    var punit = unit_positions[tile_index];
    scene.add(punit);
  }

}