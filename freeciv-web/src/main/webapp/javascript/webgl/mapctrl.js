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

var timeOfLastPinchZoom = new Date().getTime();

/****************************************************************************
 Init WebGL mapctrl.
****************************************************************************/
function init_webgl_mapctrl()
{

  $("#canvas_div").mousedown(webglOnDocumentMouseDown);
  $("#canvas_div").mouseup(webglOnDocumentMouseUp);
  $('#canvas_div').bind('mousewheel', webglOnMouseWheel);
  $(window).mousemove(mouse_moved_cb);

  if (is_touch_device()) {
    $('#canvas_div').bind('touchstart', webgl_mapview_touch_start);
    $('#canvas_div').bind('touchend', webgl_mapview_touch_end);
    $('#canvas_div').bind('touchmove', webgl_mapview_touch_move);
  }

  window.addEventListener('resize', webglOnWindowResize, false );

  /* setup zoom in/zoom out*/
  if (is_touch_device()) {
    var mc = new Hammer.Manager(document.getElementById('canvas_div'));
    mc.add(new Hammer.Pinch({ threshold: 0.1 }));
    mc.on("pinch", webgl_mapview_pinch_zoom);
  }

}


/****************************************************************************
...
****************************************************************************/
function webglOnWindowResize() {

  if (camera != null) {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
  }
  if (maprenderer != null) {
    maprenderer.setSize(window.innerWidth, window.innerHeight);
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

  var ptile = webgl_canvas_pos_to_tile(e.clientX, e.clientY - $("#canvas_div").offset().top);
  if (ptile == null) return;

  if (rightclick) {
    /* right click to recenter. */
    if (!map_select_active || !map_select_setting_enabled) {
      context_menu_active = true;
      webgl_recenter_button_pressed(ptile);
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
    /* The context menu blocks the right click mouse up event on some
     * browsers. */
    context_menu_active = false;
  }

}


/****************************************************************************
  Mouse wheel scrolling for map zoom in and zoom out.
****************************************************************************/
function webglOnMouseWheel(e) {
  var new_camera_dx;
  var new_camera_dy;
  var new_camera_dz;

  if(e.originalEvent.wheelDelta / 120 > 0) {
    // zoom in
    new_camera_dy = camera_dy - 60;
    new_camera_dx = camera_dx - 45;
    new_camera_dz = camera_dz - 45;
  } else {
    // zoom out
    new_camera_dy = camera_dy + 60;
    new_camera_dx = camera_dx + 45;
    new_camera_dz = camera_dz + 45;
  }

  if (new_camera_dy < 350 || new_camera_dy > 1200) {
    return;
  } else {
    camera_dx = new_camera_dx;
    camera_dy = new_camera_dy;
    camera_dz = new_camera_dz;
  }

  camera_look_at(camera_current_x, camera_current_y, camera_current_z);

}


/****************************************************************************
 Handle zoom in / out on touch device
****************************************************************************/
function webgl_mapview_pinch_zoom(ev)
{
  timeOfLastPinchZoom = new Date().getTime();
  var new_camera_dx;
  var new_camera_dy;
  var new_camera_dz;

  if(ev.scale > 1) {
    // zoom in
    new_camera_dy = camera_dy - 8;
    new_camera_dx = camera_dx - 4;
    new_camera_dz = camera_dz - 4;
  } else{
    // zoom out
    new_camera_dy = camera_dy + 8;
    new_camera_dx = camera_dx + 4;
    new_camera_dz = camera_dz + 4;
  }
  if (new_camera_dy < 250 || new_camera_dy > 1100) {
    return;
  } else {
    camera_dx = new_camera_dx;
    camera_dy = new_camera_dy;
    camera_dz = new_camera_dz;
  }

  camera_look_at(camera_current_x, camera_current_y, camera_current_z);
}

/****************************************************************************
  This function is triggered when beginning a touch event on a touch device,
  eg. finger down on screen.
****************************************************************************/
function webgl_mapview_touch_start(e)
{
  e.preventDefault();

  touch_start_x = e.originalEvent.touches[0].pageX - $('#canvas_div').position().left;
  touch_start_y = e.originalEvent.touches[0].pageY - $('#canvas_div').position().top;

  var ptile = webgl_canvas_pos_to_tile(touch_start_x, touch_start_y);
  if (ptile == null) return;
  var sunit = find_visible_unit(ptile);
  if (sunit != null && client.conn.playing != null && sunit['owner'] == client.conn.playing.playerno) {
    mouse_touch_started_on_unit = true;
  } else {
    mouse_touch_started_on_unit = false;
  }

}

/****************************************************************************
  This function is triggered when ending a touch event on a touch device,
  eg finger up from screen.
****************************************************************************/
function webgl_mapview_touch_end(e)
{
  if (new Date().getTime() - timeOfLastPinchZoom < 400) {
    return;
  }
  webgl_action_button_pressed(touch_start_x, touch_start_y, SELECT_POPUP);
}

/****************************************************************************
  This function is triggered on a touch move event on a touch device.
****************************************************************************/
function webgl_mapview_touch_move(e)
{
  mouse_x = e.originalEvent.touches[0].pageX - $('#canvas_div').position().left;
  mouse_y = e.originalEvent.touches[0].pageY - $('#canvas_div').position().top;

  var diff_x = (touch_start_x - mouse_x) * 2;
  var diff_y = (touch_start_y - mouse_y) * 2;

  var spos = webgl_canvas_pos_to_map_pos(touch_start_x, touch_start_y);
  var epos = webgl_canvas_pos_to_map_pos(mouse_x, mouse_y);

  touch_start_x = mouse_x;
  touch_start_y = mouse_y;

  if (!goto_active) {
    webgl_check_mouse_drag_unit(mouse_x, mouse_y);
  }

  if (client.conn.playing == null) return;

  /* Request preview goto path */
  goto_preview_active = true;
  if (goto_active && current_focus.length > 0) {
    var ptile = webgl_canvas_pos_to_tile(mouse_x, mouse_y);
    if (ptile != null) {
      for (var i = 0; i < current_focus.length; i++) {
        if (i >= 20) return;  // max 20 units goto a time.
        if (goto_request_map[current_focus[i]['id'] + "," + ptile['x'] + "," + ptile['y']] == null) {
          request_goto_path(current_focus[i]['id'], ptile['x'], ptile['y']);
        }
      }
    }
    return;
  }


  camera_look_at(camera_current_x + spos['x'] - epos['x'], camera_current_y, camera_current_z + spos['y'] - epos['y']);

}


/**************************************************************************
  Recenter the map on the canvas location, on user request.  Usually this
  is done with a right-click.
**************************************************************************/
function webgl_recenter_button_pressed(ptile)
{

  if (can_client_change_view() && ptile != null) {
    var sunit = find_visible_unit(ptile);
    if (!client_is_observer() && sunit != null
        && sunit['owner'] == client.conn.playing.playerno) {
      /* the user right-clicked on own unit, show context menu instead of recenter. */
      if (current_focus.length <= 1) set_unit_focus(sunit);
      $("#canvas_div").contextMenu(true);
      $("#canvas_div").contextmenu();
    } else {
      $("#canvas_div").contextMenu(false);
      enable_mapview_slide_3d(ptile);
    }
  }
}

/**************************************************************************
  Do some appropriate action when the "main" mouse button (usually
  left-click) is pressed.  For more sophisticated user control use (or
  write) a different xxx_button_pressed function.
**************************************************************************/
function webgl_action_button_pressed(canvas_x, canvas_y, qtype)
{
  var ptile = webgl_canvas_pos_to_tile(canvas_x, canvas_y)

  if (can_client_change_view() && ptile != null) {
    do_map_click(ptile, qtype, true);
  }
}


/****************************************************************************
 This function checks if there is a visible unit on the given canvas position,
 and selects that visible unit, and activates goto for touch devices.
****************************************************************************/
function webgl_check_mouse_drag_unit(canvas_x, canvas_y)
{
  var ptile = webgl_canvas_pos_to_tile(canvas_x, canvas_y);
  if (ptile == null || !mouse_touch_started_on_unit) return;

  var sunit = find_visible_unit(ptile);

  if (sunit != null) {
    if (client.conn.playing != null && sunit['owner'] == client.conn.playing.playerno) {
      set_unit_focus(sunit);
      if (is_touch_device()) activate_goto();
    }
  }

  var ptile_units = tile_units(ptile);
  if (ptile_units.length > 1) {
     update_active_units_dialog();
  }

}