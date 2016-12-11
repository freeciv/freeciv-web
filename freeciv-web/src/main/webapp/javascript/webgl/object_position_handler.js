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
var unit_label_positions = {};
var unit_activities_positions = {};

// stores tile extras (eg specials), key is extra + "." + tile_index.
var tile_extra_positions = {};

var selected_unit_indicator = null;

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

    if (scene != null) scene.remove(unit_label_positions[ptile['index']]);
    delete unit_label_positions[ptile['index']];
    unit_activities_positions[ptile['index']] = null;
  }

  if (unit_positions[ptile['index']] == null && visible_unit != null) {
    // add new unit to the unit_positions
    var unit_type_name = unit_type(visible_unit)['name'];
    if (unit_type_name == null || webgl_models[unit_type_name] == null) {
      console.error(unit_type_name + " model not loaded correcly.");
      return;
    }

    var new_unit = webgl_models[unit_type_name].clone();
    unit_positions[ptile['index']] = new_unit;
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    new_unit.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_unit.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
    new_unit.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    // rotate unit in the direction it is facing.
    setTimeout("if (unit_positions["+ptile['index']+"] != null) unit_positions["+ptile['index']+"].rotateY(" + (convert_unit_rotation(visible_unit['facing']) * Math.PI * 2 / 8) + ")", 1);
    if (scene != null && new_unit != null) {
      scene.add(new_unit);
    }
    /* add flag. */
    var pflag = get_unit_nation_flag_normal_sprite(visible_unit);
    if (meshes[pflag['key']] != null && unit_flag_positions[ptile['index']] == null) {
      var new_flag = meshes[pflag['key']].clone();
      unit_flag_positions[ptile['index']] = new_flag;
      var fpos = map_to_scene_coords(ptile['x'], ptile['y']);
      new_flag.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 10);
      new_flag.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 14);
      new_flag.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
      new_flag.rotation.y = Math.PI / 4;
      if (scene != null && new_flag != null) {
        scene.add(new_flag);
      }
    }
    /* indicate focus unit*/
    var funit = get_focus_unit_on_tile(ptile);
    if (scene != null && funit != null && funit['id'] == visible_unit['id']) {
      if (selected_unit_indicator != null) {
        scene.remove(selected_unit_indicator);
        selected_unit_indicator = null;
      }
      var material = new THREE.MeshBasicMaterial( { color: 0xfeffc5, transparent: true, opacity: 0.5} );
      var selected_mesh = new THREE.Mesh( new THREE.RingGeometry( 18, 25, 24), material );
      selected_mesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      selected_mesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 11);
      selected_mesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
      selected_mesh.rotation.x = -1 * Math.PI / 2;
      scene.add(selected_mesh);
      selected_unit_indicator = selected_mesh;
    }
  }

  if (unit_positions[ptile['index']] != null && visible_unit != null) {
    // Update of visible unit.
    var unit_type_name = unit_type(visible_unit)['name'];
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);

    if (unit_activities_positions[ptile['index']] != get_unit_activity_text(visible_unit)) {
      // add unit activity label
      if (scene != null && unit_label_positions[ptile['index']] != null) scene.remove(unit_label_positions[ptile['index']]);
      if (get_unit_activity_text(visible_unit) != null) {
        var text = create_unit_label(visible_unit);
        if (text != null) {
          text.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] + 5);
          text.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 28);
          text.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 5);
          text.rotation.y = Math.PI / 4;
          if (scene != null) scene.add(text);
          unit_label_positions[ptile['index']] = text;
        }
      }
      unit_activities_positions[ptile['index']] = get_unit_activity_text(visible_unit);
    }

    /* indicate focus unit*/
    var funit = get_focus_unit_on_tile(ptile);
    if (scene != null && funit != null && funit['id'] == visible_unit['id']) {
      if (selected_unit_indicator != null) {
        scene.remove(selected_unit_indicator);
        selected_unit_indicator = null;
      }
      var material = new THREE.MeshBasicMaterial( { color: 0xfeffc5, transparent: true, opacity: 0.45} );
      var selected_mesh = new THREE.Mesh( new THREE.RingGeometry( 18, 24, 20), material );
      selected_mesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      selected_mesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 11);
      selected_mesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
      selected_mesh.rotation.x = -1 * Math.PI / 2;
      scene.add(selected_mesh);
      selected_unit_indicator = selected_mesh;
    }

    // no need up update unit model, since the model is unchanged.
    if (unit_positions[ptile['index']]['unit_type'] == unit_type_name) return;

    if (scene != null) scene.remove(unit_positions[ptile['index']]);
    delete unit_positions[ptile['index']];

    if (unit_type_name == null || webgl_models[unit_type_name] == null) {
      console.error(unit_type_name + " model not loaded correcly.");
      return;
    }

    var new_unit = webgl_models[unit_type_name].clone();
    unit_positions[ptile['index']] = new_unit;
    unit_positions[ptile['index']]['unit_type'] = unit_type_name;

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
      console.error("City model not loaded correcly.");
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

    var text = create_city_label(pcity['name']);
    text.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] + 5);
    text.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 47);
    text.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 5);
    text.rotation.y = Math.PI / 4;
    if (scene != null) scene.add(text);
  }

  if (city_positions[ptile['index']] != null && pcity != null) {
    // Update of visible city. TODO.

  }

}


/****************************************************************************
  Handles tile extras, such as specials, irrigation, mine.
****************************************************************************/
function update_tile_extras(ptile) {

  var height = 5 + ptile['height'] * 100;

  if (tile_extra_positions[EXTRA_IRRIGATION + "." + ptile['index']] == null && tile_has_extra(ptile, EXTRA_IRRIGATION)) {
    var irrigation = webgl_models["Irrigation"].clone()
    tile_extra_positions[EXTRA_IRRIGATION + "." + ptile['index']] = irrigation;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    irrigation.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    irrigation.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 3);
    irrigation.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    if (scene != null && irrigation != null) {
      scene.add(irrigation);
    }
  }

  if (tile_extra_positions[EXTRA_MINE + "." + ptile['index']] == null && tile_has_extra(ptile, EXTRA_MINE)) {
    var mine = webgl_models["Mine"].clone()
    tile_extra_positions[EXTRA_MINE + "." + ptile['index']] = mine;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    mine.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    mine.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 3);
    mine.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    if (scene != null && mine != null) {
      scene.add(mine);
    }
  }

  /* TODO: This is a temporary road solution. */
  if (tile_extra_positions[ROAD_ROAD + "." + ptile['index']] == null && tile_has_extra(ptile, ROAD_ROAD)) {
    var road = webgl_models["Road"].clone()
    tile_extra_positions[ROAD_ROAD + "." + ptile['index']] = road;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    road.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    road.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 3);
    road.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

    if (scene != null && road != null) {
      scene.add(road);
    }
  }

  if (tile_extra_positions[EXTRA_HUT + "." + ptile['index']] == null && tile_has_extra(ptile, EXTRA_HUT)) {
    // add hut
    var hut = webgl_models["Hut"].clone()
    tile_extra_positions[EXTRA_HUT + "." + ptile['index']] = hut;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    hut.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    hut.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 6);
    hut.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    if (scene != null && hut != null) {
      scene.add(hut);
    }
  } else if (tile_extra_positions[EXTRA_HUT + "." + ptile['index']] != null && !tile_has_extra(ptile, EXTRA_HUT)) {
    // remove hut
    if (scene != null) {
      scene.remove(tile_extra_positions[EXTRA_HUT + "." + ptile['index']]);
    }
  }

  var extra = extras[ptile['resource']];
  if (extra != null && scene != null && webgl_models[extra['name']] != null) {
      var extra = webgl_models[extra['name']].clone()
      tile_extra_positions[extra['id'] + "." + ptile['index']] = extra;

      var pos = map_to_scene_coords(ptile['x'], ptile['y']);
      extra.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      extra.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 3);
      extra.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

      if (scene != null && extra != null) {
        scene.add(extra);
      }
  }

}

/****************************************************************************
  Clears the selected unit indicator.
****************************************************************************/
function webgl_clear_unit_focus()
{
  if (selected_unit_indicator != null) {
    scene.remove(selected_unit_indicator);
    selected_unit_indicator = null;
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

