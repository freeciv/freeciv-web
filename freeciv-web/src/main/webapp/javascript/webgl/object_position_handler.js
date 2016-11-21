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

// stores unit positions on the map. tile index is key, unit 3d model is value.
var unit_positions = {};
// stores city positions on the map. tile index is key, unit 3d model is value.
var city_positions = {};
// stores flag positions on the map. tile index is key, unit 3d model is value.
var city_flag_positions = {};
var unit_flag_positions = {};

/****************************************************************************
  Handles unit positions
****************************************************************************/
function update_unit_position(ptile) {
  if (renderer != RENDERER_WEBGL) return;

  var visible_unit = find_visible_unit(ptile);
  var height = 5 + ptile['height'] * 100;

  if (unit_positions[ptile['index']] != null && visible_unit == null) {
    // tile has no visible units, remove it from unit_positions.
    if (scene != null) scene.remove(unit_positions[ptile['index']]);
    delete unit_positions[ptile['index']];

    if (scene != null) scene.remove(unit_flag_positions[ptile['index']]);
    delete unit_flag_positions[ptile['index']];
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
    var pflag = get_unit_nation_flag_normal_sprite(visible_unit);
    if (meshes[pflag['key']] != null && unit_flag_positions[ptile['index']] == null) {
      var new_flag = meshes[pflag['key']].clone()
      unit_flag_positions[ptile['index']] = new_flag;
      var fpos = map_to_scene_coords(ptile['x'], ptile['y']);
      new_flag.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 10);
      new_flag.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 10);
      new_flag.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
      new_flag.rotation.y = Math.PI / 4;
      if (scene != null && new_flag != null) {
        scene.add(new_flag);
      }
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
  Handles city positions
****************************************************************************/
function update_city_position(ptile) {
  if (renderer != RENDERER_WEBGL) return;

  var pcity = tile_city(ptile);

  var height = 5 + ptile['height'] * 100;

  if (city_positions[ptile['index']] != null && pcity == null) {
    // tile has no visible units, remove it from unit_positions.
    if (scene != null) scene.remove(city_positions[ptile['index']]);
    delete city_positions[ptile['index']];
  }

  if (city_positions[ptile['index']] == null && pcity != null) {
    // add new city
    if (webgl_models['city_1'] == null) {
      console.log("City model not loaded correcly.");
      return;
    }

    var new_city = webgl_models["city_1"].clone()
    city_positions[ptile['index']] = new_city;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    new_city.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_city.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
    new_city.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

    if (scene != null && new_city != null) {
      scene.add(new_city);
    }
    var cflag = get_city_flag_sprite(pcity);
    if (cflag != null && meshes[cflag['key']] != null) {
      var new_flag = meshes[cflag['key']].clone()
      city_flag_positions[ptile['index']] = new_flag;
      var fpos = map_to_scene_coords(ptile['x'], ptile['y']);
      new_flag.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 5);
      new_flag.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 30);
      new_flag.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']- 5);
      new_flag.rotation.y = Math.PI / 4;
      if (scene != null && new_flag != null) {
        scene.add(new_flag);
      }
    }
  }

  if (city_positions[ptile['index']] != null && pcity != null) {
    // Update of visible city. TODO.

  }

}

/****************************************************************************
  Adds all units and cities to the map.
****************************************************************************/
function add_all_objects_to_scene()
{
  for (var tile_index in unit_positions) {
    var punit = unit_positions[tile_index];
    scene.add(punit);
  }

  for (var tile_index in city_positions) {
    var pcity = city_positions[tile_index];
    scene.add(pcity);
  }

  for (var tile_index in unit_flag_positions) {
    var pflag = unit_flag_positions[tile_index];
    scene.add(pflag);
  }

  for (var tile_index in city_flag_positions) {
    var pflag = city_flag_positions[tile_index];
    scene.add(pflag);
  }

}