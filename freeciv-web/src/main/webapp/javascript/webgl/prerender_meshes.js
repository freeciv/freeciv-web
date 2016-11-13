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


/****************************************************************************
  Prerender meshes, such as trees.
****************************************************************************/
function prerender(landGeometry, xquality) {
 /* Trees */
  var trees_geometry = new THREE.Geometry();
  for (var i = 0, l = landGeometry.vertices.length; i < l; i++) {
    var x = i % xquality, y = Math.floor(i / xquality);
    var gx = Math.round(x / 4 - 0.5);
    var gy = Math.round(y / 4 - 0.5);
    var ptile = map_pos_to_tile(gx, gy);
    if (ptile != null && tile_get_known(ptile) != TILE_UNKNOWN) {
      var terrain_name = tile_terrain(ptile).name;
      var add_tree = false;
      if (terrain_name == "Forest" || terrain_name == "Jungle") {
        /* Dense forests. */
        add_tree = true;
      } else if (terrain_name == "Grassland" ||
                 terrain_name == "Plains" ||
                 terrain_name == "Hills") {
        /* Sparse trees -> Monte-Carlo algorithm */
        add_tree = (Math.random() < 0.03);
      }

      /* No trees on beaches */
      add_tree &= (landGeometry.vertices[i].y > 57);

      if (add_tree) {
        var geometry = new THREE.SphereGeometry(6, 4, 4);
        geometry.rotateX(0.1 * Math.PI * Math.random());
        geometry.rotateY(0.1 * Math.PI * Math.random());
        geometry.rotateZ(0.1 * Math.PI * Math.random());
        geometry.translate(landGeometry.vertices[i].x + 5,
                           landGeometry.vertices[i].y + 2,
                           landGeometry.vertices[i].z + 5);
        geometry.translate(2.5 * (-1 + 2*Math.random()),
                           0.1 * (-1 + 2*Math.random()),
                           2.5 * (-1 + 2*Math.random()));

        var mesh = new THREE.Mesh(geometry);
        mesh.updateMatrix();
        trees_geometry.merge(mesh.geometry, mesh.matrix);
      }
    }
  }
  var trees_material = new THREE.MeshLambertMaterial({
      color: new THREE.Color(0, 0.15, 0),
      emissive: new THREE.Color(0, 0.01, 0) });
  meshes['trees'] = new THREE.Mesh(trees_geometry, trees_material);


}