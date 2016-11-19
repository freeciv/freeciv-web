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
  if (renderer != RENDERER_WEBGL) return;

  var visible_unit = find_visible_unit(ptile);
  var height = 77 + map_tile_height(ptile) * 44;

  if (unit_positions[ptile['index']] != null && visible_unit == null) {
    // tile has no visible units, remove it from unit_positions.
    if (scene != null) scene.remove(unit_positions[ptile['index']]);
    delete unit_positions[ptile['index']];
  }

  if (unit_positions[ptile['index']] == null && visible_unit != null) {
    // add new unit to the unit_positions
    var unit_type_name = unit_type(visible_unit)['name'];
    if (unit_type_name == null || webgl_models[unit_type_name] == null) {
      console.log(unit_type_name + " model not loaded correcly.");
      return;
    }

    var new_unit = webgl_models[unit_type_name].clone()
    unit_positions[ptile['index']] = new_unit;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    new_unit.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_unit.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
    new_unit.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

    if (scene != null && new_unit != null) {
      scene.add(new_unit);
    }
  }

  if (unit_positions[ptile['index']] != null && visible_unit != null) {
    // Update of visible unit.
    if (scene != null) scene.remove(unit_positions[ptile['index']]);
    delete unit_positions[ptile['index']];

    var unit_type_name = unit_type(visible_unit)['name'];
    if (unit_type_name == null || webgl_models[unit_type_name] == null) {
      console.log(unit_type_name + " model not loaded correcly.");
      return;
    }

    var new_unit = webgl_models[unit_type_name].clone()
    unit_positions[ptile['index']] = new_unit;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    new_unit.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_unit.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
    new_unit.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

    if (scene != null && new_unit != null) {
      scene.add(new_unit);
    }
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