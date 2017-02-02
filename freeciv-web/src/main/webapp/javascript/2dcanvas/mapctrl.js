/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

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


var touch_start_x;
var touch_start_y;

var map_select_setting_enabled = true;
var map_select_check = false;
var map_select_check_started = 0;
var map_select_active = false;
var map_select_x;
var map_select_y;
var mouse_touch_started_on_unit = false;

/****************************************************************************
  Init 2D mapctrl
****************************************************************************/
function mapctrl_init_2d()
{
  // Register keyboard and mouse listener using JQuery.
  $("#canvas").mouseup(mapview_mouse_click);
  $("#canvas").mousedown(mapview_mouse_down);
  $(window).mousemove(mouse_moved_cb);

  if (is_touch_device()) {
    $('#canvas').bind('touchstart', mapview_touch_start);
    $('#canvas').bind('touchend', mapview_touch_end);
    $('#canvas').bind('touchmove', mapview_touch_move);
  }

}

/****************************************************************************
  Triggered when the mouse button is clicked UP on the mapview canvas.
****************************************************************************/
function mapview_mouse_click(e)
{
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
  if (rightclick) {
    /* right click to recenter. */
    if (!map_select_active || !map_select_setting_enabled) {
      context_menu_active = true;
      recenter_button_pressed(mouse_x, mouse_y);
    } else {
      context_menu_active = false;
      map_select_units(mouse_x, mouse_y);
    }
    map_select_active = false;
    map_select_check = false;

  } else if (!rightclick && !middleclick) {
    /* Left mouse button*/
    action_button_pressed(mouse_x, mouse_y, SELECT_POPUP);
  }

}

/****************************************************************************
  Triggered when the mouse button is clicked DOWN on the mapview canvas.
****************************************************************************/
function mapview_mouse_down(e)
{
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

    setTimeout("check_mouse_drag_unit(" + mouse_x + "," + mouse_y + ");", 200);
  } else if (middleclick || e['altKey']) {
    popit();
    return false;
  } else if (rightclick && !map_select_active && is_right_mouse_selection_supported()) {
    map_select_check = true;
    map_select_x = mouse_x;
    map_select_y = mouse_y;
    map_select_check_started = new Date().getTime();

    /* The context menu blocks the right click mouse up event on some
     * browsers. */
    context_menu_active = false;
  }
}

/****************************************************************************
  This function is triggered when beginning a touch event on a touch device,
  eg. finger down on screen.
****************************************************************************/
function mapview_touch_start(e)
{
  e.preventDefault();

  touch_start_x = e.originalEvent.touches[0].pageX - $('#canvas').position().left;
  touch_start_y = e.originalEvent.touches[0].pageY - $('#canvas').position().top;

  var ptile = canvas_pos_to_tile(touch_start_x, touch_start_y);
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
function mapview_touch_end(e)
{
  action_button_pressed(touch_start_x, touch_start_y, SELECT_POPUP);
}

/****************************************************************************
  This function is triggered on a touch move event on a touch device.
****************************************************************************/
function mapview_touch_move(e)
{
  mouse_x = e.originalEvent.touches[0].pageX - $('#canvas').position().left;
  mouse_y = e.originalEvent.touches[0].pageY - $('#canvas').position().top;

  var diff_x = (touch_start_x - mouse_x) * 2;
  var diff_y = (touch_start_y - mouse_y) * 2;

  touch_start_x = mouse_x;
  touch_start_y = mouse_y;

  if (!goto_active) {
    check_mouse_drag_unit(mouse_x, mouse_y);

    mapview['gui_x0'] += diff_x;
    mapview['gui_y0'] += diff_y;
  }

  if (client.conn.playing == null) return;

  /* Request preview goto path */
  goto_preview_active = true;
  if (goto_active && current_focus.length > 0) {
    var ptile = canvas_pos_to_tile(mouse_x, mouse_y);
    if (ptile != null) {
      for (var i = 0; i < current_focus.length; i++) {
        if (i >= 20) return;  // max 20 units goto a time.
        if (goto_request_map[current_focus[i]['id'] + "," + ptile['x'] + "," + ptile['y']] == null) {
          request_goto_path(current_focus[i]['id'], ptile['x'], ptile['y']);
        }
      }
    }
  }


}


/****************************************************************************
  This function is triggered when the mouse is clicked on the city canvas.
****************************************************************************/
function city_mapview_mouse_click(e)
{
  var rightclick;
  if (!e) var e = window.event;
  if (e.which) {
    rightclick = (e.which == 3);
  } else if (e.button) {
    rightclick = (e.button == 2);
  }

  if (!rightclick) {
    city_action_button_pressed(mouse_x, mouse_y);
  }


}

/****************************************************************************
 This function checks if there is a visible unit on the given canvas position,
 and selects that visible unit, and activates goto for touch devices.
****************************************************************************/
function check_mouse_drag_unit(canvas_x, canvas_y)
{
  var ptile = canvas_pos_to_tile(canvas_x, canvas_y);
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

/**************************************************************************
  Do some appropriate action when the "main" mouse button (usually
  left-click) is pressed.  For more sophisticated user control use (or
  write) a different xxx_button_pressed function.
**************************************************************************/
function action_button_pressed(canvas_x, canvas_y, qtype)
{
  var ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile != null) {
    /* FIXME: Some actions here will need to check can_client_issue_orders.
     * But all we can check is the lowest common requirement. */
    do_map_click(ptile, qtype, true);
  }
}


/**************************************************************************
  Do some appropriate action when the "main" mouse button (usually
  left-click) is pressed.  For more sophisticated user control use (or
  write) a different xxx_button_pressed function.
**************************************************************************/
function city_action_button_pressed(canvas_x, canvas_y)
{
  var ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile != null) {
    /* FIXME: Some actions here will need to check can_client_issue_orders.
     * But all we can check is the lowest common requirement. */
    do_city_map_click(ptile);
  }
}


/**************************************************************************
  This will select and set focus to all the units which are in the 
  selected rectangle on the map when the mouse is selected using the right
  mouse button. 
  [canvas_x, canvas_y, map_select_x, map_select_y].
**************************************************************************/
function map_select_units(canvas_x, canvas_y)
{
  var selected_tiles = {};
  var selected_units = [];
  if (client_is_observer()) return;

  var start_x = (map_select_x < canvas_x) ? map_select_x : canvas_x; 
  var start_y = (map_select_y < canvas_y) ? map_select_y : canvas_y; 
  var end_x = (map_select_x < canvas_x) ? canvas_x : map_select_x; 
  var end_y = (map_select_y < canvas_y) ? canvas_y : map_select_y; 


  for (var x = start_x; x < end_x; x += 15) {
    for (var y = start_y; y < end_y; y += 15) {
      var ptile = canvas_pos_to_tile(x, y);
      if (ptile != null) {
        selected_tiles[ptile['tile']] = ptile;
      }
    }
  }

  for (var tile_id in selected_tiles) {
    var ptile = selected_tiles[tile_id];
    var cunits = tile_units(ptile);
    if (cunits == null) continue;
    for (var i = 0; i < cunits.length; i++) {
      var aunit = cunits[i];
      if (aunit['owner'] == client.conn.playing.playerno) {
        selected_units.push(aunit);
      }
    }
  }

  current_focus = selected_units;
  update_active_units_dialog();
}

/**************************************************************************
  Recenter the map on the canvas location, on user request.  Usually this
  is done with a right-click.
**************************************************************************/
function recenter_button_pressed(canvas_x, canvas_y)
{
  var map_scroll_border = 8;
  var big_map_size = 24;
  var ptile = canvas_pos_to_tile(canvas_x, canvas_y);
  var orig_tile = ptile;

  /* Prevent the user from scrolling outside the map. */
  if (ptile != null && ptile['y'] > (map['ysize'] - map_scroll_border)
      && map['xsize'] > big_map_size && map['ysize'] > big_map_size) {
    ptile = map_pos_to_tile(ptile['x'], map['ysize'] - map_scroll_border);
  }
  if (ptile != null && ptile['y'] < map_scroll_border
      && map['xsize'] > big_map_size && map['ysize'] > big_map_size) {
    ptile = map_pos_to_tile(ptile['x'], map_scroll_border);
  }

  if (can_client_change_view() && ptile != null && orig_tile != null) {
    var sunit = find_visible_unit(orig_tile);
    if (!client_is_observer() && sunit != null
        && sunit['owner'] == client.conn.playing.playerno) {
      /* the user right-clicked on own unit, show context menu instead of recenter. */
      if (current_focus.length <= 1) set_unit_focus(sunit);
      $("#canvas").contextMenu(true);
      $("#canvas").contextmenu();
    } else {
      $("#canvas").contextMenu(false);
      /* FIXME: Some actions here will need to check can_client_issue_orders.
       * But all we can check is the lowest common requirement. */
      enable_mapview_slide(ptile);
      center_tile_mapcanvas(ptile);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
function handle_info_text_message(packet)
{
  var message = decodeURIComponent(packet['message']);
  var regxp = /\n/gi;
  message = message.replace(regxp, "<br>\n");

  show_dialog_message("Tile Information", message);

}

/**************************************************************************
 This function shows the dialog containing active units on the current tile.
**************************************************************************/
function update_active_units_dialog()
{
  var unit_info_html = "";
  var ptile = null;
  var punits = [];
  var width = 0;

  if (client_is_observer()) return;

  if (current_focus.length == 1) {
    ptile = index_to_tile(current_focus[0]['tile']);
    punits.push(current_focus[0]);
    var tmpunits = tile_units(ptile); 
    for (var i = 0; i < tmpunits.length; i++) {
      var kunit = tmpunits[i];
      if (kunit['id'] == current_focus[0]['id']) continue;
      punits.push(kunit);
    }
  } else if (current_focus.length > 1) {
    punits = current_focus;
  }

  for (var i = 0; i < punits.length; i++) {
    var punit = punits[i];
    var ptype = unit_type(punit);
    var sprite = get_unit_image_sprite(punit);
    var active = (current_focus.length > 1 || current_focus[0]['id'] == punit['id']);

    unit_info_html += "<div id='unit_info_div' class='" + (active ? "current_focus_unit" : "")
           + "'><div id='unit_info_image' onclick='set_unit_focus_and_redraw(units[" + punit['id'] + "])' "
	   + " style='background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           + "'></div></div>";
    width = sprite['width'];
  }

  if (current_focus.length == 1) {
    /* show info about the active focus unit. */
    var aunit = current_focus[0];
    var ptype = unit_type(aunit);
    unit_info_html += "<div id='active_unit_info' title='" + ptype['helptext'] + "'>";
    unit_info_html += "<b>" + ptype['name'] + "</b>: "
    if (get_unit_homecity_name(punit) != null) {
      unit_info_html += " " + get_unit_homecity_name(punit) + " ";
    }
    unit_info_html += "<span>" + get_unit_moves_left(punit) + "</span> ";
    unit_info_html += "<br><span title='Attack strength'>A:" + ptype['attack_strength']
    + "</span> <span title='Defense strength'>D:" + ptype['defense_strength']
    + "</span> <span title='Firepower'>F:" + ptype['firepower']
    + "</span> <span title='Health points'>H:"
    + ptype['hp'] + "</span>";
    if (punit['veteran'] > 0) {
      unit_info_html += " <span>Veteran: " + punit['veteran'] + "</span>";
    }
    if (ptype['transport_capacity'] > 0) {
      unit_info_html += " <span>Transport: " + ptype['transport_capacity'] + "</span>";
    }

    unit_info_html += "</div>";
  } else if (current_focus.length > 1) {
    unit_info_html += "<div id='active_unit_info'>" + current_focus.length + " units selected.</div> ";
  }

  $("#game_unit_info").html(unit_info_html);

  if (current_focus.length > 0) {
    /* reposition and resize unit dialog. */
    var newwidth = 32 + punits.length * (width + 10);
    if (newwidth < 140) newwidth = 140;
    var newheight = 75 + normal_tile_height;
    $("#game_unit_panel").parent().show();
    $("#game_unit_panel").parent().width(newwidth);
    $("#game_unit_panel").parent().height(newheight);
    $("#game_unit_panel").parent().css("left", ($( window ).width() - newwidth) + "px");
    $("#game_unit_panel").parent().css("top", ($( window ).height() - newheight - 30) + "px");
    $("#game_unit_panel").parent().css("background", "rgba(50,50,40,0.5)");
  } else {
    $("#game_unit_panel").parent().hide();
  }
  $("#active_unit_info").tooltip();
}

