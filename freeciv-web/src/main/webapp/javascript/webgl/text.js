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
 Convert a canvas to a mesh that will always face the user. The height of
 `canvas' should be 16px. `width' gives the width of the content that will
 actually be displayed, not of the canvas itself. `transparent' should be
 true if canvas' content is transparent.
****************************************************************************/
function canvas_to_user_facing_mesh(canvas, width, transparent)
{
  // Create a texture out of the canvas
  var texture = new THREE.Texture(canvas);
  texture.needsUpdate = true;

  // Make a material
  var material = new THREE.ShaderMaterial({
    vertexShader: document.getElementById('labels_vertex_shh').textContent,
    fragmentShader: document.getElementById('tex_fragment_shh').textContent,
    uniforms: {
      texture: { value: texture },
      u_scale_factor: { value: width / canvas.width }
    }
  });
  material.transparent = transparent;

  // Put it all together
  return new THREE.Mesh(new THREE.PlaneBufferGeometry(Math.floor(width * 0.80), 13), material);
}

/****************************************************************************
 Create a city name label
****************************************************************************/
function create_city_label(pcity)
{
  var canvas1 = document.createElement('canvas');
  canvas1.width = 256;
  canvas1.height = 16;
  var ctx = canvas1.getContext('2d');

  var owner_id = pcity.owner;
  if (owner_id == null) return null;
  var owner = players[owner_id];

  // We draw from left to right, updating `width' after each call.
  var width = 0; // Total width of the bar

  // Flag
  var city_gfx = get_city_flag_sprite(pcity);
  ctx.drawImage(sprites[city_gfx.key],
                1, 1, // Remove the 1px black border, it's ugly
                sprites[city_gfx.key].width - 2, sprites[city_gfx.key].height - 2,
                0, 0, 28, 16);
  width += 28;

  // Occupied
  var ptile = city_tile(pcity);
  var punits = tile_units(ptile);
  if (punits.length > 0) {
    // Background
    ctx.fillStyle = 'black';
    ctx.fillRect(width, 0, 16, 16);
    // Stars
    ctx.drawImage(sprites[get_city_occupied_sprite(pcity)], width, 0, 13, 16);
    width += 13;
  }

  // Name and size
  var city_text = pcity.name + "  " + pcity.size;
  ctx.font = 'Bold 14px Arial';
  var txt_measure = ctx.measureText(city_text);
  // Background
  ctx.fillStyle = nations[owner.nation].color;
  ctx.fillRect(width, 0, txt_measure.width + 8 /* padding */, 16);
  // Text
  var dark_bg = false;
  var nation_colors = nations[owner['nation']].color.replace("rgb(", "").replace(")", "").split(",");
  if (parseInt(nation_colors[0]) + parseInt(nation_colors[1]) + parseInt(nation_colors[2]) < 300) dark_bg = true;
  if (dark_bg) {
    ctx.fillStyle = 'white';
  } else {
    ctx.fillStyle = 'black';
  }
  ctx.fillText(city_text, width + 4 /* padding */, 13);

  width += txt_measure.width + 8 /* padding */;

  // Production
  var prod_type = get_city_production_type(pcity);
  if (prod_type != null) {
    var tag = prod_type.graphic_str;
    if (tileset[tag] != null) {
      ctx.fillStyle = nations[owner.nation].color;
      ctx.fillRect(width, 0, 36, 16);
      ctx.drawImage(sprites[tag], width, 0, 36, 18);
      width += 32;
    }
  }

  return canvas_to_user_facing_mesh(canvas1, width, false);
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

  var text = get_unit_activity_text(punit);
  var width = context1.measureText(text).width;
  context1.strokeText(text, 0, 16);
  context1.fillText(text, 0, 16);

  return canvas_to_user_facing_mesh(canvas1, width, true);
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