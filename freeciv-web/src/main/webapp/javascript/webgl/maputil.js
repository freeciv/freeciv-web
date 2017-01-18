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


/****************************************************************************
  Converts from map to scene coordinates.
****************************************************************************/
function map_to_scene_coords(x, y)
{
  var result = {};
  result['x'] = Math.floor(-470 + x * mapview_model_width / map['xsize']);
  result['y'] = Math.floor(30 + y * mapview_model_height / map['ysize']);
  return result;
}

/****************************************************************************
  Converts from scene to map coordinates.
****************************************************************************/
function scene_to_map_coords(x, y)
{
  var result = {};
  result['x'] = Math.floor((x + 500) * map['xsize'] / mapview_model_width);
  result['y'] = Math.floor((y) * map['ysize'] / mapview_model_height);
  return result;
}


/****************************************************************************
  Converts from canvas coordinates to a tile.
****************************************************************************/
function webgl_canvas_pos_to_tile(x, y) {
  if (mouse == null || landMesh == null) return null;

  mouse.set( ( x / $('#canvas_div').width() ) * 2 - 1, - ( y / $('#canvas_div').height() ) * 2 + 1);

  raycaster.setFromCamera( mouse, camera );

  var intersects = raycaster.intersectObject(landMesh);

  for (var i = 0; i < intersects.length; i++) {
    var intersect = intersects[i];
    var pos = scene_to_map_coords(intersect.point.x, intersect.point.z);
    var ptile = map_pos_to_tile(pos['x'], pos['y']);
    if (ptile != null) return ptile;
  }

  return null;
}

/****************************************************************************
  Converts from canvas coordinates to Three.js coordinates.
****************************************************************************/
function webgl_canvas_pos_to_map_pos(x, y) {
  if (mouse == null || landMesh == null) return null;

  mouse.set( ( x / $('#canvas_div').width() ) * 2 - 1, - ( y / $('#canvas_div').height() ) * 2 + 1);

  raycaster.setFromCamera( mouse, camera );

  var intersects = raycaster.intersectObject(landMesh);

  for (var i = 0; i < intersects.length; i++) {
    var intersect = intersects[i];
    return {'x' : intersect.point.x, 'y' : intersect.point.z};
  }

  return null;
}

/****************************************************************************
  Converts from unit['facing'] to number of rotations of 1/8 parts of full circle rotations (2PI).
****************************************************************************/
function convert_unit_rotation(facing_dir)
{
  if (facing_dir == 0) return -3;
  if (facing_dir == 1) return -4;
  if (facing_dir == 2) return -5;
  if (facing_dir == 4) return -6;
  if (facing_dir == 7) return -7;
  if (facing_dir == 6) return 0;
  if (facing_dir == 5) return -1;
  if (facing_dir == 3) return -2;
}