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
 Init WebGL mapctrl.
****************************************************************************/
function init_webgl_mapctrl()
{
  document.addEventListener('mousemove', webglOnDocumentMouseMove, false );
  document.addEventListener('keydown', webglOnDocumentKeyDown, false );
  document.addEventListener('keyup', webglOnDocumentKeyUp, false );
  window.addEventListener('resize', webglOnWindowResize, false );

  $("#canvas_div").mousedown(webglOnDocumentMouseDown);
  $("#canvas_div").mouseup(webglOnDocumentMouseUp);
  $('#canvas_div').bind('mousewheel', webglOnMouseWheel);

}


/****************************************************************************
...
****************************************************************************/
function webglOnWindowResize() {

  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();

  maprenderer.setSize( window.innerWidth, window.innerHeight );

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentMouseMove( event ) {

  event.preventDefault();
  mouse.set( ( event.clientX / window.innerWidth ) * 2 - 1, - ( event.clientY / window.innerHeight ) * 2 + 1 );

  raycaster.setFromCamera( mouse, camera );

  var intersects = raycaster.intersectObjects( objects );

  if ( intersects.length > 0 ) {

    var intersect = intersects[ 0 ];

  }

}

/****************************************************************************
Triggered when the mouse button is clicked UP on the mapview canvas.
****************************************************************************/
function webglOnDocumentMouseUp( e ) {

  var rightclick = false;
  var middleclick = false;

  if (!e) var e = window.event;
  if (e.which) {
    rightclick = (e.which == 3);
    middleclick = (e.which == 2);
  } else if (e.button) {
    rightclick = (e.button == 2);
    middleclick = (e.button == 1 || e.button == 4);
  }

  var ptile = webgl_canvas_pos_to_tile(e.clientX, e.clientY);
  if (ptile == null) return;

  if (rightclick) {
    /* right click to recenter. */
    if (!map_select_active || !map_select_setting_enabled) {
      enable_mapview_slide_3d(ptile);
      context_menu_active = false;

    } else {
      context_menu_active = false;
      //map_select_units(mouse_x, mouse_y);
    }
    map_select_active = false;
    map_select_check = false;

  } else if (!rightclick && !middleclick) {
    /* Left mouse button*/
    do_map_click(ptile, SELECT_POPUP, true);
  }
  e.preventDefault();

}

/****************************************************************************
  Triggered when the mouse button is clicked DOWN on the mapview canvas.
****************************************************************************/
function webglOnDocumentMouseDown(e) {
  var rightclick = false;
  var middleclick = false;

  if (!e) var e = window.event;
  if (e.which) {
    rightclick = (e.which == 3);
    middleclick = (e.which == 2);
  } else if (e.button) {
    rightclick = (e.button == 2);
    middleclick = (e.button == 1 || e.button == 4);
  }

  if (!rightclick && !middleclick) {
    /* Left mouse button is down */
    if (goto_active) return;

    //setTimeout("check_mouse_drag_unit(" + mouse_x + "," + mouse_y + ");", 200);
  } else if (middleclick || e['altKey']) {
    popit();
    return false;
  } else if (rightclick && !map_select_active && is_right_mouse_selection_supported()) {
    /*map_select_check = true;
    map_select_x = mouse_x;
    map_select_y = mouse_y;
    map_select_check_started = new Date().getTime();*/

    /* The context menu blocks the right click mouse up event on some
     * browsers. */
    context_menu_active = false;
  }

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentKeyDown( event ) {

  switch( event.keyCode ) {
     case 16: isShiftDown = true; break;

  }

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentKeyUp( event ) {
  switch ( event.keyCode ) {
    case 16: isShiftDown = false; break;

  }

}

/****************************************************************************
  Mouse wheel scrolling for map zoom in and zoom out.
****************************************************************************/
function webglOnMouseWheel(e) {
  var new_camera_dx;
  var new_camera_dy;
  var new_camera_dz;

  if(e.originalEvent.wheelDelta /120 > 0) {
    // zoom in
    new_camera_dy = camera_dy - 60;
    new_camera_dx = camera_dx - 45;
    new_camera_dz = camera_dz - 45;
  } else{
    // zoom out
    new_camera_dy = camera_dy + 60;
    new_camera_dx = camera_dx + 45;
    new_camera_dz = camera_dz + 45;
  }

  if (new_camera_dy < 220 || new_camera_dy > 2000) {
    return;
  } else {
    camera_dx = new_camera_dx;
    camera_dy = new_camera_dy;
    camera_dz = new_camera_dz;
  }

  camera_look_at(camera_current_x, camera_current_y, camera_current_z);

}

