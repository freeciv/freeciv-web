/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

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

var camera;

var camera_dx = 300;
var camera_dy = 480;
var camera_dz = 300;
var camera_current_x = 0;
var camera_current_y = 0;
var camera_current_z = 0;


/****************************************************************************
  Point the camera to look at point x, y, z in Three.js coordinates.
****************************************************************************/
function camera_look_at(x, y, z)
{
  camera_current_x = x;
  camera_current_y = y;
  camera_current_z = z;

  camera.position.set( x + camera_dx, y + camera_dy, z + camera_dz);
  camera.lookAt( new THREE.Vector3(x, 0, z));

  if (directionalLight != null) directionalLight.position.set( x + camera_dx, y + camera_dy, z + camera_dz ).normalize();

}

/**************************************************************************
  Centers the mapview around the given tile..
**************************************************************************/
function center_tile_mapcanvas_3d(ptile)
{
  var pos = map_to_scene_coords(ptile['x'], ptile['y']);
  camera_look_at(pos['x'], 0, pos['y']);

}

/**************************************************************************
  Enabled silding of the mapview to the given tile.
**************************************************************************/
function enable_mapview_slide_3d(ptile)
{
  var pos_dest = map_to_scene_coords(ptile['x'], ptile['y']);

  mapview_slide['dx'] = camera_current_x - pos_dest['x'];
  mapview_slide['dy'] = camera_current_z - pos_dest['y'];
  mapview_slide['i'] = mapview_slide['max'];
  mapview_slide['prev'] = mapview_slide['i'];
  mapview_slide['start'] = new Date().getTime();
  mapview_slide['active'] = true;

}

/**************************************************************************
  Updates mapview slide, called once per frame.
**************************************************************************/
function update_map_slide_3d()
{
  var elapsed = 1 + new Date().getTime() - mapview_slide['start'];
  mapview_slide['i'] = Math.floor(mapview_slide['max']
                        * (mapview_slide['slide_time']
                        - elapsed) / mapview_slide['slide_time']);

  if (mapview_slide['i'] <= 0) {
    mapview_slide['active'] = false;
    return;
  }

  var dx = Math.floor(mapview_slide['dx'] * (mapview_slide['i'] - mapview_slide['prev']) / mapview_slide['max']);
  var dy = Math.floor(mapview_slide['dy'] * (mapview_slide['i'] - mapview_slide['prev']) / mapview_slide['max']);

  camera_look_at(camera_current_x + dx, 0, camera_current_z + dy);
  mapview_slide['prev'] = mapview_slide['i'];

}