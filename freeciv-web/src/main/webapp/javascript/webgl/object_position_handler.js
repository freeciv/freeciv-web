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
var city_label_positions = {};
var city_walls_positions = {};

// stores flag positions on the map. tile index is key, unit 3d model is value.
var unit_flag_positions = {};
var unit_label_positions = {};
var unit_activities_positions = {};

var unit_health_positions = {};
var unit_healthpercentage_positions = {};

var forest_positions = {}; // tile index is key, boolean is value.
var jungle_positions = {}; // tile index is key, boolean is value.

// stores tile extras (eg specials), key is extra + "." + tile_index.
var tile_extra_positions = {};

var road_positions = {};
var rail_positions = {};
var river_positions = {};

var selected_unit_indicator = null;
var selected_unit_material = null;
var selected_unit_material_counter = 0;

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

    if (scene != null) scene.remove(unit_health_positions[ptile['index']]);
    delete unit_health_positions[ptile['index']];
    unit_healthpercentage_positions[ptile['index']] = null;
  }

  if (unit_positions[ptile['index']] == null && visible_unit != null) {
    // add new unit to the unit_positions
    var unit_type_name = unit_type(visible_unit)['name'];
    if (unit_type_name == null) {
      console.error(unit_type_name + " model not loaded correcly.");
      return;
    }

    var new_unit = webgl_get_model(unit_type_name);
    if (new_unit == null) return;
    unit_positions[ptile['index']] = new_unit;
    var pos;
    if (visible_unit['anim_list'].length > 0) {
      var stile = tiles[visible_unit['anim_list'][0]['tile']];
      pos = map_to_scene_coords(stile['x'], stile['y']);
      height = 5 + stile['height'] * 100;
    } else {
      pos = map_to_scene_coords(ptile['x'], ptile['y']);
    }
    new_unit.matrixAutoUpdate = false
    new_unit.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_unit.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height - 2);
    new_unit.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    new_unit.rotateOnAxis(new THREE.Vector3(0,1,0).normalize(), (convert_unit_rotation(visible_unit['facing']) * Math.PI * 2 / 8));
    new_unit.updateMatrix();

    if (scene != null && new_unit != null) {
      scene.add(new_unit);
    }
    /* add flag. */
    var pflag = get_unit_nation_flag_sprite(visible_unit);
    var new_flag;
    if (unit_flag_positions[ptile['index']] == null) {
      new_flag = get_flag_shield_mesh(pflag['key']);
      new_flag.matrixAutoUpdate = false;
      unit_flag_positions[ptile['index']] = new_flag;
      new_flag.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 10);
      new_flag.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 20);
      new_flag.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
      new_flag.rotation.y = Math.PI / 4;
      new_flag.updateMatrix();
      if (scene != null && new_flag != null) {
        scene.add(new_flag);
      }
    }

    /* indicate focus unit*/
    var funit = get_focus_unit_on_tile(ptile);
    var selected_mesh;
    if (scene != null && funit != null && funit['id'] == visible_unit['id']) {
      if (selected_unit_indicator != null) {
        scene.remove(selected_unit_indicator);
        selected_unit_indicator = null;
      }
      if (visible_unit['anim_list'].length == 0) {
        selected_mesh = new THREE.Mesh( new THREE.RingGeometry( 16, 25, 24), selected_unit_material );
        selected_mesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
        selected_mesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 8);
        selected_mesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
        selected_mesh.rotation.x = -1 * Math.PI / 2;
        scene.add(selected_mesh);
        selected_unit_indicator = selected_mesh;
      }
    }

    anim_objs[visible_unit['id']] = {'unit' : visible_unit['id'], 'mesh' : new_unit, 'flag' : new_flag};


  } else if (unit_positions[ptile['index']] != null && visible_unit != null) {
    // Update of visible unit.
    // TODO: update_unit_position() contains _almost_ the same code twice. this is the duplicate part.
    var unit_type_name = unit_type(visible_unit)['name'];
    var pos;
    if (visible_unit['anim_list'].length > 0) {
      var stile = tiles[visible_unit['anim_list'][0]['tile']];
      pos = map_to_scene_coords(stile['x'], stile['y']);
      height = 5 + stile['height'] * 100;
    } else {
      pos = map_to_scene_coords(ptile['x'], ptile['y']);
    }

    if (scene != null) scene.remove(unit_positions[ptile['index']]);
    delete unit_positions[ptile['index']];

    if (scene != null && unit_flag_positions[ptile['index']] != null) scene.remove(unit_flag_positions[ptile['index']]);
    delete unit_flag_positions[ptile['index']];


    var activity;
    if (unit_activities_positions[ptile['index']] != get_unit_activity_text(visible_unit) && visible_unit['anim_list'].length == 0) {
      // add unit activity label
      if (scene != null && unit_label_positions[ptile['index']] != null) scene.remove(unit_label_positions[ptile['index']]);
      if (get_unit_activity_text(visible_unit) != null) {
        activity = create_unit_label(visible_unit);
        if (activity != null) {
          activity.matrixAutoUpdate = false;
          activity.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] + 8);
          activity.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 28);
          activity.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 5);
          activity.rotation.y = Math.PI / 4;
          activity.updateMatrix();
          if (scene != null) scene.add(activity);
          unit_label_positions[ptile['index']] = activity;
        }
      }
      unit_activities_positions[ptile['index']] = get_unit_activity_text(visible_unit);
    }

    var new_unit_health_bar;
    if (unit_healthpercentage_positions[ptile['index']] != visible_unit['hp'] && visible_unit['anim_list'].length == 0) {
      if (scene != null && unit_health_positions[ptile['index']] != null) scene.remove(unit_health_positions[ptile['index']]);
      new_unit_health_bar = get_unit_health_mesh(visible_unit);
      new_unit_health_bar.matrixAutoUpdate = false;
      unit_health_positions[ptile['index']] = new_unit_health_bar;
      new_unit_health_bar.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 10);
      new_unit_health_bar.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 26);
      new_unit_health_bar.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
      new_unit_health_bar.rotation.y = Math.PI / 4;
      new_unit_health_bar.updateMatrix();
      if (scene != null && new_unit_health_bar != null) {
        scene.add(new_unit_health_bar);
      }
      unit_healthpercentage_positions[ptile['index']] = visible_unit['hp'];
    }

    /* indicate focus unit*/
    var funit = get_focus_unit_on_tile(ptile);
    var selected_mesh;
    if (scene != null && funit != null && funit['id'] == visible_unit['id']) {
      if (selected_unit_indicator != null) {
        scene.remove(selected_unit_indicator);
        selected_unit_indicator = null;
      }
      if (visible_unit['anim_list'].length == 0) {
        selected_mesh = new THREE.Mesh( new THREE.RingGeometry( 16, 24, 20), selected_unit_material );
        selected_mesh.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
        selected_mesh.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 8);
        selected_mesh.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
        selected_mesh.rotation.x = -1 * Math.PI / 2;
        scene.add(selected_mesh);
        selected_unit_indicator = selected_mesh;
      }
    }

    if (unit_type_name == null) {
      console.error(unit_type_name + " model not loaded correcly.");
      return;
    }

    var new_unit = webgl_get_model(unit_type_name);
    if (new_unit == null) return;
    unit_positions[ptile['index']] = new_unit;
    unit_positions[ptile['index']]['unit_type'] = unit_type_name;

    new_unit.matrixAutoUpdate = false
    new_unit.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_unit.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height - 2);
    new_unit.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    new_unit.rotateOnAxis(new THREE.Vector3(0,1,0).normalize(), (convert_unit_rotation(visible_unit['facing']) * Math.PI * 2 / 8));
    new_unit.updateMatrix();

    if (scene != null && new_unit != null) {
      scene.add(new_unit);
    }

    /* add flag. */
    var pflag = get_unit_nation_flag_sprite(visible_unit);
    var new_flag;
    if (unit_flag_positions[ptile['index']] == null) {
      new_flag = get_flag_shield_mesh(pflag['key']);
      new_flag.matrixAutoUpdate = false;
      unit_flag_positions[ptile['index']] = new_flag;
      new_flag.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 10);
      new_flag.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 20);
      new_flag.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
      new_flag.rotation.y = Math.PI / 4;
      new_flag.updateMatrix();
      if (scene != null && new_flag != null) {
        scene.add(new_flag);
      }
    }

    anim_objs[visible_unit['id']] = {'unit' : visible_unit['id'], 'mesh' : new_unit, 'flag' : new_flag};
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
    // tile has no city, remove it from unit_positions.
    if (scene != null) scene.remove(city_positions[ptile['index']]);
    delete city_positions[ptile['index']];
    if (scene != null) scene.remove(city_label_positions[ptile['index']]);
    delete city_label_positions[ptile['index']];
    if (scene != null) scene.remove(city_walls_positions[ptile['index']]);
    delete city_walls_positions[ptile['index']];
  }

  if (city_positions[ptile['index']] == null && pcity != null) {
    // add new city
    var size = 0;
    if (pcity['size'] >=4 && pcity['size'] <=7) {
      size = 1;
    } else if (pcity['size'] >=8) {
      size = 2;
    }
    pcity['webgl_model_size'] = size;
    var new_city = webgl_get_model("city_european_" + size);
    if (new_city == null) return;
    city_positions[ptile['index']] = new_city;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    new_city.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    new_city.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
    new_city.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

    if (scene != null && new_city != null) {
      scene.add(new_city);
    }

    if (scene != null && pcity['walls'] && city_walls_positions[ptile['index']] == null) {
      var city_walls = webgl_get_model("citywalls");
      city_walls.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      city_walls.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
      city_walls.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
      scene.add(city_walls);
      city_walls_positions[ptile['index']] = city_walls;
    }

    var city_label = create_city_label(pcity);
    city_label_positions[ptile['index']] = city_label;
    city_label.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] + 5);
    city_label.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 35);
    city_label.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 5);
    city_label.rotation.y = Math.PI / 4;
    if (scene != null) scene.add(city_label);
  }

  if (city_positions[ptile['index']] != null && pcity != null) {
    // Update of visible city.
    var size = 0;
    if (pcity['size'] >=4 && pcity['size'] <=7) {
      size = 1;
    } else if (pcity['size'] >=8) {
      size = 2;
    }
    if (pcity['webgl_model_size'] != size) {
      // update city model to a different size.
      if (scene != null) scene.remove(city_positions[ptile['index']]);
      pcity['webgl_model_size'] = size;
      var new_city = webgl_get_model("city_european_" + size);
      if (new_city == null) return;
      city_positions[ptile['index']] = new_city;

      var pos = map_to_scene_coords(ptile['x'], ptile['y']);
      new_city.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      new_city.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
      new_city.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

      if (scene != null && new_city != null) {
        scene.add(new_city);
      }
    }
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);

    if (scene != null && pcity['walls'] && city_walls_positions[ptile['index']] == null) {
      var city_walls = webgl_get_model("citywalls");
      city_walls.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      city_walls.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
      city_walls.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
      scene.add(city_walls);
      city_walls_positions[ptile['index']] = city_walls;
    }

    if (scene != null) scene.remove(city_label_positions[ptile['index']]);
    delete city_label_positions[ptile['index']];
    var city_label = create_city_label(pcity);
    city_label_positions[ptile['index']] = city_label;
    city_label.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] + 5);
    city_label.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 35);
    city_label.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 5);
    city_label.rotation.y = Math.PI / 4;
    if (scene != null) scene.add(city_label);

  }

}


/****************************************************************************
  Handles tile extras, such as specials, irrigation, mine.
****************************************************************************/
function update_tile_extras(ptile) {

  var height = 5 + ptile['height'] * 100;

  if (tile_extra_positions[EXTRA_IRRIGATION + "." + ptile['index']] == null && tile_has_extra(ptile, EXTRA_IRRIGATION)) {
    var irrigation = webgl_get_model("Irrigation");
    if (irrigation == null) return;
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
    var mine = webgl_get_model("Mine");
    if (mine == null) return;
    tile_extra_positions[EXTRA_MINE + "." + ptile['index']] = mine;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    mine.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    mine.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 3);
    mine.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
    if (scene != null && mine != null) {
      scene.add(mine);
    }
  }

  if (scene != null && road_positions[ptile['index']] == null && tile_has_extra(ptile, EXTRA_ROAD)) {
    var road_added = false;
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    // 1. iterate over adjacent tiles, see if they have road.
    for (var dir = 0; dir < 8; dir++) {
      var checktile = mapstep(ptile, dir);
      if (checktile != null && tile_has_extra(checktile, EXTRA_ROAD)) {
        // if the road wraps the map edge, then skip it.
        if ((ptile['x'] == 0 && checktile['x'] == map['xsize'] -1)
            ||  (ptile['x'] == map['xsize'] -1 && checktile['x'] == 0)) continue;

        // 2. if road on this tile and adjacent tile, then add rectangle as road between this tile and adjacent tile.
        road_added = true;
        var road_width = 15;
        var dest = map_to_scene_coords(checktile['x'], checktile['y']);
        var roadGemetry = new THREE.PlaneGeometry(60, 15);
        roadGemetry.dynamic = true;
        var delta = 0;
        if (dir == 1 || dir == 6) delta = 10;
        roadGemetry.vertices[0].x = 0.0;
        roadGemetry.vertices[0].y = 0.0 - delta;
        roadGemetry.vertices[0].z = 0.0;
        roadGemetry.vertices[1].x = dest['x'] - pos['x'];
        roadGemetry.vertices[1].y = (checktile['height'] - ptile['height']) * 100 - delta;
        roadGemetry.vertices[1].z = dest['y'] - pos['y'];
        roadGemetry.vertices[2].x = 0.0;
        roadGemetry.vertices[2].y = delta;
        roadGemetry.vertices[2].z = road_width;
        roadGemetry.vertices[3].x = dest['x'] - pos['x'];
        roadGemetry.vertices[3].y = (checktile['height'] - ptile['height']) * 100 + delta;
        roadGemetry.vertices[3].z = dest['y'] - pos['y'] + road_width;

        roadGemetry.verticesNeedUpdate = true;

        var road = new THREE.Mesh(roadGemetry, webgl_materials['road_1']);

        road.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
        road.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 5);
        road.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
        scene.add(road);
        road_positions[ptile['index']] = road;
      }
    }

    // 3. if road on this tile only, then use Road model.
    if (road_added == false) {
      var road = webgl_get_model("Road");
      if (road == null) return;
      road_positions[ptile['index']] = road;
      road.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      road.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 4);
      road.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
      if (scene != null && road != null) {
        scene.add(road);
      }
    }
  }


  if (scene != null && road_positions[ptile['index']] == null && tile_has_extra(ptile, EXTRA_RAIL)) {
    var rail_added = false;
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);

    if (road_positions[ptile['index']] != null) {
      if (scene != null) scene.remove(road_positions[ptile['index']]);
    }

    // 1. iterate over adjacent tiles, see if they have rail.
    for (var dir = 0; dir < 8; dir++) {
      var checktile = mapstep(ptile, dir);
      if (checktile != null && tile_has_extra(checktile, EXTRA_RAIL)) {
        // if the rail wraps the map edge, then skip it.
        if ((ptile['x'] == 0 && checktile['x'] == map['xsize'] -1)
            ||  (ptile['x'] == map['xsize'] -1 && checktile['x'] == 0)) continue;

        // 2. if rail on this tile and adjacent tile, then add rectangle as rail between this tile and adjacent tile.
        rail_added = true;
        var rail_width = 15;
        var dest = map_to_scene_coords(checktile['x'], checktile['y']);
        var railGemetry = new THREE.PlaneGeometry(60, 15);
        railGemetry.dynamic = true;
        var delta = 0;
        if (dir == 1 || dir == 6) delta = 10;
        railGemetry.vertices[0].x = 0.0;
        railGemetry.vertices[0].y = 0.0 - delta;
        railGemetry.vertices[0].z = 0.0;
        railGemetry.vertices[1].x = dest['x'] - pos['x'];
        railGemetry.vertices[1].y = (checktile['height'] - ptile['height']) * 100 - delta;
        railGemetry.vertices[1].z = dest['y'] - pos['y'];
        railGemetry.vertices[2].x = 0.0;
        railGemetry.vertices[2].y = delta;
        railGemetry.vertices[2].z = rail_width;
        railGemetry.vertices[3].x = dest['x'] - pos['x'];
        railGemetry.vertices[3].y = (checktile['height'] - ptile['height']) * 100 + delta;
        railGemetry.vertices[3].z = dest['y'] - pos['y'] + rail_width;

        railGemetry.verticesNeedUpdate = true;

        var rail = new THREE.Mesh(railGemetry, webgl_materials['rail_1']);

        rail.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
        rail.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 5);
        rail.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 10);
        scene.add(rail);
        rail_positions[ptile['index']] = rail;
      }
    }

    // 3. if rail on this tile only, then use Rail model.
    if (rail_added == false) {
      var rail = webgl_get_model("Rail");
      if (rail == null) return;
      rail_positions[ptile['index']] = rail;
      rail.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      rail.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 4);
      rail.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);
      if (scene != null && rail != null) {
        scene.add(rail);
      }
    }
  }

  /* Rivers */
  if (scene != null && river_positions[ptile['index']] == null && tile_has_extra(ptile, EXTRA_RIVER)) {
    var river_added = false;
    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    // 1. iterate over adjacent tiles, see if they have river.
    for (var dir = 0; dir < 8; dir++) {
      var checktile = mapstep(ptile, dir);
      if (checktile != null && (tile_has_extra(checktile, EXTRA_RIVER) || is_ocean_tile(checktile))) {
        // if the river wraps the map edge, then skip it.
        if ((ptile['x'] == 0 && checktile['x'] == map['xsize'] -1)
            ||  (ptile['x'] == map['xsize'] -1 && checktile['x'] == 0)) continue;

        // 2. if river on this tile and adjacent tile, then add rectangle as river between this tile and adjacent tile.
        river_added = true;
        var river_width = 15;
        var dest = map_to_scene_coords(checktile['x'], checktile['y']);
        var riverGeometry = new THREE.PlaneGeometry(60, 15);
        riverGeometry.dynamic = true;
        var delta = 0;
        if (dir == 1 || dir == 6) delta = 10;
        riverGeometry.vertices[0].x = 0.0;
        riverGeometry.vertices[0].y = 0.0 - delta;
        riverGeometry.vertices[0].z = 0.0;
        riverGeometry.vertices[1].x = dest['x'] - pos['x'];
        riverGeometry.vertices[1].y = (checktile['height'] - ptile['height']) * 100 - delta - (is_ocean_tile(checktile) ? 26 : 0);
        riverGeometry.vertices[1].z = dest['y'] - pos['y'];
        riverGeometry.vertices[2].x = 0.0;
        riverGeometry.vertices[2].y = delta;
        riverGeometry.vertices[2].z = river_width;
        riverGeometry.vertices[3].x = dest['x'] - pos['x'];
        riverGeometry.vertices[3].y = (checktile['height'] - ptile['height']) * 100 + delta - (is_ocean_tile(checktile) ? 26 : 0);
        riverGeometry.vertices[3].z = dest['y'] - pos['y'] + river_width;

        riverGeometry.verticesNeedUpdate = true;

        var river = new THREE.Mesh(riverGeometry, webgl_materials['river']);

        river.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x'] - 5);
        river.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 4);
        river.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y'] - 13);
        scene.add(river);
        river_positions[ptile['index']] = river;
        river_positions[checktile['index']] = river;
      }
    }
  }


  if (tile_extra_positions[EXTRA_HUT + "." + ptile['index']] == null && tile_has_extra(ptile, EXTRA_HUT)) {
    // add hut
    var hut = webgl_get_model("Hut");
    if (hut == null) return;
    tile_extra_positions[EXTRA_HUT + "." + ptile['index']] = hut;

    var pos = map_to_scene_coords(ptile['x'], ptile['y']);
    hut.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
    hut.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
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

  var extra_resource = extras[ptile['resource']];
  if (extra_resource != null && scene != null && tile_extra_positions[extra_resource['id'] + "." + ptile['index']] == null) {
      var extra_model = webgl_get_model(extra_resource['name']);
      if (extra_model == null) return;

      tile_extra_positions[extra_resource['id'] + "." + ptile['index']] = extra_model;

      var pos = map_to_scene_coords(ptile['x'], ptile['y']);
      extra_model.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), pos['x']);
      extra_model.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height);
      extra_model.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), pos['y']);

      if (scene != null && extra_model != null) {
        scene.add(extra_model);
      }
  }

  // show forest
  var terrain_name = tile_terrain(ptile).name;
  if (scene != null && forest_geometry != null
      && tile_get_known(ptile) == TILE_KNOWN_SEEN
      && forest_positions[ptile['index']] == null
      && terrain_name == "Forest") {
    for (var s = 0; s < forest_geometry.vertices.length; s++) {
      if (forest_geometry.vertices[s]['tile'] == ptile['index']) {
        forest_geometry.vertices[s].y = forest_geometry.vertices[s]['height'];
        forest_positions[ptile['index']] = true;
      }
    }
    forest_geometry.verticesNeedUpdate = true;
  }

  // show jungle
  if (scene != null && jungle_geometry != null
      && tile_get_known(ptile) == TILE_KNOWN_SEEN
      && jungle_positions[ptile['index']] == null
      && terrain_name == "Jungle") {
    for (var s = 0; s < jungle_geometry.vertices.length; s++) {
      if (jungle_geometry.vertices[s]['tile'] == ptile['index']) {
        jungle_geometry.vertices[s].y = jungle_geometry.vertices[s]['height'];
        jungle_positions[ptile['index']] = true;
      }
    }
    jungle_geometry.verticesNeedUpdate = true;
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
  unit_positions = {};
  city_positions = {};
  city_label_positions = {};
  city_walls_positions = {};
  unit_flag_positions = {};
  unit_label_positions = {};
  unit_activities_positions = {};
  unit_health_positions = {};
  unit_healthpercentage_positions = {};
  forest_positions = {};
  jungle_positions = {};
  tile_extra_positions = {};
  road_positions = {};
  rail_positions = {};
  river_positions = {};

  for (var unit_id in units) {
    var punit = units[unit_id];
    var ptile = index_to_tile(punit['tile']);
    update_unit_position(ptile);
  }

  for (var city_id in cities) {
    var pcity = cities[city_id];
    update_city_position(city_tile(pcity));
  }

  for (var tile_id in tiles) {
    update_tile_extras(tiles[tile_id]);
  }


}
