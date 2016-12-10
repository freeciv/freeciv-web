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

var goto_line = null;

// TODO: lines don't support width. Use rectangles instead.

/****************************************************************************
...
****************************************************************************/
function webgl_render_goto_line(start_tile, goto_packet_dir)
{
  clear_goto_tiles();
  var ptile = start_tile;
  var startpos = map_to_scene_coords(ptile['x'], ptile['y']);

  var material = new THREE.LineBasicMaterial({
	color: 0x0505ff,
	linewidth: 1
  });

  var geometry = new THREE.Geometry();
  geometry.vertices.push(new THREE.Vector3(0, 0, 0));
  var height = 5 + ptile['height'] * 100;
  goto_line = new THREE.Line(geometry, material);

  var dx = 0;
  var dy = 0;
  var dh = 0;
  for (var i = 0; i < goto_packet_dir.length; i++) {
    if (ptile == null) break;
    var dir = goto_packet_dir[i];

    if (dir == -1) {
      /* Assume that this means refuel. */
      continue;
    }

    var nexttile = mapstep(ptile, dir);
    if (ptile != null && nexttile != null) {
      var currpos = map_to_scene_coords(ptile['x'], ptile['y']);
      var nextpos = map_to_scene_coords(nexttile['x'], nexttile['y']);
      dx = dx + nextpos['x'] - currpos['x'];
      dy = dy + nextpos['y'] - currpos['y'];
      dh = dh + ((nexttile['height'] - ptile['height']) * 100);
      geometry.vertices.push(
	    new THREE.Vector3(dx, dh, dy)
      );
    }

    ptile = mapstep(ptile, dir);
  }

  goto_line.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), startpos['x']);
  goto_line.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 5);
  goto_line.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), startpos['y']);


  scene.add(goto_line);


}