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
function create_city_label(pcity)
{
  var canvas1 = document.createElement('canvas');
  canvas1.width = 128;
  canvas1.height = 16;
  var ctx = canvas1.getContext('2d');

  var owner_id = pcity['owner'];
  if (owner_id == null) return null;
  var owner = players[owner_id];

  ctx.fillStyle=nations[owner['nation']]['color'];
  ctx.fillRect(0,0,128,16);
  ctx.font = "Bold 16px Arial";
  ctx.fillStyle = "rgba(0,0,0, 1.0)";
  //TODO: show city size
  ctx.fillText(pcity['name'], 30, 15);

  var city_gfx = get_city_flag_sprite(pcity);
  ctx.drawImage(sprites[city_gfx['key']], 0, 0,
                                  sprites[city_gfx['key']].width, sprites[city_gfx['key']].height,
                                  0,0,28,16);

  var texture1 = new THREE.Texture(canvas1);
  texture1.needsUpdate = true;

  var material1 = new THREE.MeshBasicMaterial( { map: texture1, side:THREE.DoubleSide } );
  material1.transparent = false;

  var mesh1 = new THREE.Mesh(
    new THREE.PlaneGeometry(64, 11),
    material1
  );

  return mesh1;
}

/****************************************************************************
 Create a unit label
****************************************************************************/
function create_unit_label(punit)
{
  var canvas1 = document.createElement('canvas');
  canvas1.width = 32;
  canvas1.height = 16;
  var context1 = canvas1.getContext('2d');
  context1.font = "Bold 16px Arial";
  context1.fillStyle = "rgba(222,255,0, 1.0)";
  context1.strokeStyle= "black";
  context1.lineWidth = 0.5;
  context1.fillText(get_unit_activity_text(punit), 2, 16);
  context1.strokeText(get_unit_activity_text(punit), 2, 16);

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


/**********************************************************************
  ...
***********************************************************************/
function get_unit_activity_text(punit)
{
  var activity = punit['activity'];
  var act_tgt  = punit['activity_tgt'];

  /* don't draw activity for enemy units */
  if (client.conn.playing == null || punit['owner'] != client.conn.playing.playerno) {
    return null;
  }

  switch (activity) {
    case ACTIVITY_POLLUTION:
      return "F";

    case ACTIVITY_MINE:
      return "M";

    case ACTIVITY_IRRIGATE:
      return "I";

    case ACTIVITY_FORTIFIED:
      return "F";

    case ACTIVITY_BASE:
      return "B";

    case ACTIVITY_SENTRY:
      return "S";

    case ACTIVITY_PILLAGE:
      return "P";

    case ACTIVITY_GOTO:
      return "G";

    case ACTIVITY_EXPLORE:
      return "X";

    case ACTIVITY_TRANSFORM:
      return "T";

    case ACTIVITY_FORTIFYING:
      return "f";

    case ACTIVITY_GEN_ROAD:
      return "R";

    case ACTIVITY_CONVERT:
      return "C";
  }

  if (unit_has_goto(punit)) {
      return "G";
  }

  if (punit['ai'] == true) {
      return "A";
  }

  return null;
}