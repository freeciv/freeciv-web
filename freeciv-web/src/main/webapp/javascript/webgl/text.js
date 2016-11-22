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
 Create a city name label
****************************************************************************/
function create_city_label(city_name)
{
  var canvas1 = document.createElement('canvas');
  canvas1.width = 128;
  canvas1.height = 16;
  var context1 = canvas1.getContext('2d');
  context1.font = "Bold 16px Arial";
  context1.fillStyle = "rgba(255,255,255, 1.0)";
  context1.strokeStyle= "black";
  context1.lineWidth = 0.5;
  context1.fillText(city_name, 20, 16);
  context1.strokeText(city_name, 20, 16);

  var texture1 = new THREE.Texture(canvas1);
  texture1.needsUpdate = true;

  var material1 = new THREE.MeshBasicMaterial( { map: texture1, side:THREE.DoubleSide } );
  material1.transparent = true;

  var mesh1 = new THREE.Mesh(
    new THREE.PlaneGeometry(canvas1.width / 2, canvas1.height / 2),
    material1
  );

  return mesh1;
}