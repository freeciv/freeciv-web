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
  result['x'] = Math.floor(-470 + x * 3000 / map['xsize']);
  result['y'] = Math.floor(30 + y * 2000 / map['ysize']);
  return result;
}

/****************************************************************************
  Converts from scene to map coordinates.
****************************************************************************/
function scene_to_map_coords(x, y)
{
  var result = {};
  result['x'] = Math.floor((x + 470) * map['xsize'] / 3000);
  result['y'] = Math.floor((y - 30) * map['ysize'] / 2000);
  return result;
}


/****************************************************************************
  Converts from map canvas coordinates to a tile.
****************************************************************************/
function webgl_canvas_pos_to_tile(x, y) {

  mouse.set( ( x / window.innerWidth ) * 2 - 1, - ( y / window.innerHeight ) * 2 + 1 );

  raycaster.setFromCamera( mouse, camera );

  var intersects = raycaster.intersectObjects( scene.children );

  if ( intersects.length > 0 ) {
    var intersect = intersects[ 0 ];
    var pos = scene_to_map_coords(intersect.point.x, intersect.point.z)
    console.log(pos);
    var ptile = map_pos_to_tile(pos['x'], pos['y']);
    return ptile;
  } else {
    return null;
  }
}