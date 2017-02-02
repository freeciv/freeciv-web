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

var goto_lines = [];

/****************************************************************************
 Renders a goto line by creating many planes along the goto path.
****************************************************************************/
function webgl_render_goto_line(start_tile, goto_packet_dir)
{
  clear_goto_tiles();
  var ptile = start_tile;
  var startpos = map_to_scene_coords(ptile['x'], ptile['y']);

  var material = new THREE.MeshBasicMaterial( { color: 0x055dff, side:THREE.DoubleSide} );
  var goto_width = 3;

  var height = 5 + ptile['height'] * 100;

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
      var height = 5 + ptile['height'] * 100;
      if (ptile['x'] == 0 || ptile['x'] >= map['xsize'] - 1 || nexttile['x'] == 0 || nexttile['x'] >= map['xsize'] - 1) continue;

      var gotoLineGemetry = new THREE.PlaneGeometry(60, 5);
      gotoLineGemetry.dynamic = true;
      var delta = 0;
      if (dir == 1 || dir == 6) delta = 3;
      gotoLineGemetry.vertices[0].x = 0.0;
      gotoLineGemetry.vertices[0].y = 0.0;
      gotoLineGemetry.vertices[0].z = 0.0;
      gotoLineGemetry.vertices[1].x = nextpos['x'] - currpos['x'];
      gotoLineGemetry.vertices[1].y = (nexttile['height'] - ptile['height']) * 100 - delta;
      gotoLineGemetry.vertices[1].z = nextpos['y'] - currpos['y'];
      gotoLineGemetry.vertices[2].x = 0.0;
      gotoLineGemetry.vertices[2].y = delta;
      gotoLineGemetry.vertices[2].z = goto_width;
      gotoLineGemetry.vertices[3].x = nextpos['x'] - currpos['x'];
      gotoLineGemetry.vertices[3].y = (nexttile['height'] - ptile['height']) * 100 + delta;
      gotoLineGemetry.vertices[3].z = nextpos['y'] - currpos['y'] + goto_width;
      gotoLineGemetry.verticesNeedUpdate = true;
      var gotoline = new THREE.Mesh(gotoLineGemetry, material);

      gotoline.translateOnAxis(new THREE.Vector3(1,0,0).normalize(), currpos['x'] + 10);
      gotoline.translateOnAxis(new THREE.Vector3(0,1,0).normalize(), height + 25);
      gotoline.translateOnAxis(new THREE.Vector3(0,0,1).normalize(), currpos['y'] + 10);
      scene.add(gotoline);
      goto_lines.push(gotoline);

    }

    ptile = mapstep(ptile, dir);
  }


}