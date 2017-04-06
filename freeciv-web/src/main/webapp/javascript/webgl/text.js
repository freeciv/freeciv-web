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

var texture_cache = {};

/****************************************************************************
 Convert a canvas to a mesh that will always face the user. The height of
 `canvas' should be 16px.
 `width_final' gives the width of the content that will actually be displayed, not of the canvas itself.
 'width_input' I think is the width of the input.
 'height' is the height of the result.
 `transparent' should be true if canvas' content is transparent.
****************************************************************************/
function canvas_to_user_facing_mesh(canvas, width_input, width_final, height, transparent, texture_cache_key)
{
  var texture;
  if (texture_cache_key != null && texture_cache[texture_cache_key] != null) {
    texture = texture_cache[texture_cache_key];
  } else {
    // Create a new texture out of the canvas
    texture = new THREE.Texture(canvas);
    texture.needsUpdate = true;
    texture_cache[texture_cache_key] = texture;
  }

  // Make a material
  var material = new THREE.ShaderMaterial({
    vertexShader: document.getElementById('labels_vertex_shh').textContent,
    fragmentShader: document.getElementById('tex_fragment_shh').textContent,
    uniforms: {
      texture: { value: texture },
      u_scale_factor: { value: width_input / canvas.width }
    }
  });
  material.transparent = transparent;

  // Put it all together
  return new THREE.Mesh(new THREE.PlaneBufferGeometry(width_final, height), material);
}

/****************************************************************************
 Create a city name label
****************************************************************************/
function create_city_label(pcity)
{
  var canvas1 = document.createElement('canvas');
  canvas1.width = 256;
  canvas1.height = 32;
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
                0, 0, 28, 32);
  width += 28;

  // Occupied
  var ptile = city_tile(pcity);
  var punits = tile_units(ptile);
  if (punits.length > 0) {
    // Background
    ctx.fillStyle = 'black';
    ctx.fillRect(width, 0, 16, 32);
    // Stars
    ctx.drawImage(sprites[get_city_occupied_sprite(pcity)], width, 0, 13, 32);
    width += 13;
  }

  // Name and size
  var city_text = pcity.name + "  " + pcity.size;
  ctx.font = 'Bold 25px Arial';
  var txt_measure = ctx.measureText(city_text);
  // Background
  ctx.fillStyle = nations[owner.nation].color;
  ctx.fillRect(width, 0, txt_measure.width + 11 /* padding */, 32);
  // Text
  var dark_bg = false;
  var nation_colors = nations[owner['nation']].color.replace("rgb(", "").replace(")", "").split(",");
  if (parseInt(nation_colors[0]) + parseInt(nation_colors[1]) + parseInt(nation_colors[2]) < 300) dark_bg = true;
  if (dark_bg) {
    ctx.fillStyle = '#EEEEEE';
  } else {
    ctx.fillStyle = '#111111';
  }
  ctx.fillText(city_text, width + 4 /* padding */, 13*2);

  width += txt_measure.width + 11 /* padding */;

  // Production
  var prod_type = get_city_production_type(pcity);
  if (prod_type != null) {
    var tag = prod_type.graphic_str;
    if (tileset[tag] != null) {
      ctx.fillStyle = nations[owner.nation].color;
      ctx.fillRect(width, 0, 36, 32);
      ctx.drawImage(sprites[tag], width, 0, 31, 18*2);
      width += 32;
    }
  }

  if (width > 256) width = 256;
  return canvas_to_user_facing_mesh(canvas1, width, Math.floor(width * 0.7), 13, false);
}

/****************************************************************************
 Create a unit label
****************************************************************************/
function create_unit_label(punit)
{
  var canvas1 = document.createElement('canvas');
  canvas1.width = 16;
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

  return canvas_to_user_facing_mesh(canvas1, width, Math.floor(width * 0.8), 13, true, get_unit_activity_text(punit));
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