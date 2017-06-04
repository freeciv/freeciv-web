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


var stereoEffect;
var cardboard_vr_enabled = false;

var eye_separation = 0.064 * 2.3;  // default is 0.064


/****************************************************************************
  Init 3D Stereo VR cardboard
****************************************************************************/
function init_stereo_vr_cardboard(width, height)
{
  $.ajax({
    async: false,
    url: "/javascript/webgl/libs/StereoEffect.js",
    dataType: "script"
  });

  stereoEffect = new THREE.StereoEffect(maprenderer);
  stereoEffect.setSize(width, height);
  stereoEffect.setEyeSeparation(eye_separation);
  camera_dy = 300;

  $(".overview_dialog").hide();
  $("#tabs_menu").hide();
  $(".chatbox_dialog").hide();
  $(".unit_dialog").hide();
  unitpanel_active = false;
  $("#game_unit_orders_default").hide();
  $("#game_unit_orders_settlers").hide();
  $("#game_status_panel_bottom").hide();





}