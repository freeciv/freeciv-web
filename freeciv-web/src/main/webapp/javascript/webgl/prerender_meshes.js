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

var meshes = {};
var forest_geometry = null;
var jungle_geometry = null;

/****************************************************************************
  Prerender trees and jungle on known tiles.
****************************************************************************/
function prerender(landGeometry, xquality) {
 /* Trees */
  forest_geometry = new THREE.Geometry();
  var treecolors = []
  for (var i = 0, l = landGeometry.vertices.length; i < l; i++) {
    var x = i % xquality, y = Math.floor(i / xquality);
    var gx = Math.round(x / 4 - 0.5);
    var gy = Math.round(y / 4 - 0.5);
    var ptile = map_pos_to_tile(gx, gy);
    if (ptile != null) {
      var terrain_name = tile_terrain(ptile).name;
      var add_tree = false;
      if (terrain_name == "Forest") {
        /* Dense forests. */
        add_tree = true;
      } else if (terrain_name == "Grassland" ||
                 terrain_name == "Plains" ||
                 terrain_name == "Hills") {
        /* Sparse trees -> Monte-Carlo algorithm */
        add_tree = (Math.random() < 0.01);
      }

      /* No trees on beaches */
      add_tree &= (landGeometry.vertices[i].y > 57);

      if (add_tree) {
        var vertex = new THREE.Vector3();
        vertex['tile'] = ptile['index'];
        var theight = Math.floor(landGeometry.vertices[i].y + 2 + 0.1 * (-1 + 2 * Math.random()));
        vertex['height'] = theight;
        if (tile_get_known(ptile) != TILE_UNKNOWN) forest_positions[ptile['index']] = true;
        vertex.x = Math.floor(landGeometry.vertices[i].x + 5 + 2.5 * (-1 + 2 * Math.random()));
        vertex.y = tile_get_known(ptile) == TILE_UNKNOWN ? 35 : theight;
        vertex.z = Math.floor(landGeometry.vertices[i].z + 5 + 2.5 * (-1 + 2 * Math.random()));
        forest_geometry.vertices.push( vertex );
        if (graphics_quality == QUALITY_HIGH) {
          var mycolor = new THREE.Color( 0xffffff );
          mycolor.setHSL(0.5 + (Math.random() - 0.5) / 2.5, 0.2 + Math.random(), 0.5 );
          treecolors.push(mycolor);
        }
      }
    }
  }
  if (graphics_quality == QUALITY_HIGH) forest_geometry.colors = treecolors;
  var forest_material = new THREE.PointsMaterial( { size: 42, sizeAttenuation: true, map: webgl_textures["tree_1"], vertexColors: (graphics_quality == QUALITY_HIGH) ? THREE.VertexColors : THREE.NoColors, alphaTest: 0.5, transparent: true } );
  var tree_particles = new THREE.Points( forest_geometry, forest_material );
  scene.add(tree_particles);


 /* Jungle */
  jungle_geometry = new THREE.Geometry();
  for (var i = 0, l = landGeometry.vertices.length; i < l; i++) {
    var x = i % xquality, y = Math.floor(i / xquality);
    var gx = Math.round(x / 4 - 0.5);
    var gy = Math.round(y / 4 - 0.5);
    var ptile = map_pos_to_tile(gx, gy);
    if (ptile != null) {
      var terrain_name = tile_terrain(ptile).name;
      var add_tree = false;
      if (terrain_name == "Jungle") {
        add_tree = true;
      }
      add_tree &= (landGeometry.vertices[i].y > 57);
      if (add_tree) {
        var vertex = new THREE.Vector3();
        vertex['tile'] = ptile['index'];
        var theight = Math.floor(landGeometry.vertices[i].y + 2 + 0.1 * (-1 + 2 * Math.random()));
        vertex['height'] = theight;
        if (tile_get_known(ptile) != TILE_UNKNOWN) jungle_positions[ptile['index']] = true;
        vertex.x = Math.floor(landGeometry.vertices[i].x + 5 + 2.5 * (-1 + 2 * Math.random()));
        vertex.y = tile_get_known(ptile) == TILE_UNKNOWN ? 35 : theight;
        vertex.z = Math.floor(landGeometry.vertices[i].z + 5 + 2.5 * (-1 + 2 * Math.random()));
        jungle_geometry.vertices.push( vertex );
      }
    }
  }
  var jungle_material = new THREE.PointsMaterial( { size: 42, sizeAttenuation: true, map: webgl_textures["jungle_1"], alphaTest: 0.5, transparent: true } );
  var jungle_particles = new THREE.Points( jungle_geometry, jungle_material );
  scene.add(jungle_particles);

}