/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


var mouse_x;
var mouse_y;
var keyboard_input = true;

var current_focus = [];
var goto_active = false;

/* Selecting unit from a stack without popup. */
var SELECT_POPUP = 0;
var SELECT_SEA = 1;
var SELECT_LAND = 2;
var SELECT_APPEND = 3;

var intro_click_description = true; 

function mouse_moved_cb(e)
{
  mouse_x = 0;
  mouse_y = 0;
  if (!e) {
    var e = window.event;
  }
  if (e.pageX || e.pageY) {
    mouse_x = e.pageX;
    mouse_y = e.pageY;
  } else {
    if (e.clientX || e.clientY) {
      mouse_x = e.clientX;
      mouse_y = e.clientY;
    }
  }
  
  if (mapview_canvas != null) {
    mouse_x = mouse_x - mapview_canvas.offsetLeft;
    mouse_y = mouse_y - mapview_canvas.offsetTop;
  }
  

}

function check_text_input(event,chatboxtextarea) {
	 
  if (event.keyCode == 13 && event.shiftKey == 0)  {
    message = $(chatboxtextarea).val();
    message = message.replace(/^\s+|\s+$/g,"");

    $(chatboxtextarea).val('');
    $(chatboxtextarea).focus();
 
    var test_packet = [{"packet_type" : "chat_msg_req", "message" : message}];
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
    return false;
  }
}



/****************************************************************************
  Return TRUE iff a unit on this tile is in focus.
****************************************************************************/
function get_focus_unit_on_tile(ptile)
{

  var funits = get_units_in_focus();
  if (funits == null) return null;
  
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    if (punit['x'] == ptile['x'] && punit['y'] == ptile['y']) {
      return punit;
    } 
  } 
  return null;
}


/****************************************************************************
  Return TRUE iff this unit is in focus.
  TODO: not implemented yet.
****************************************************************************/
function unit_is_in_focus(cunit)
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    if (punit['id'] == cunit['id']) {
      return true;
    } 
  } 
  return false;
}

/****************************************************************************
  Returns a list of units in focus.
****************************************************************************/
function get_units_in_focus()
{
  return current_focus;
}

/**************************************************************************
 If there is no unit currently in focus, or if the current unit in
 focus should not be in focus, then get a new focus unit.
 We let GOTO-ing units stay in focus, so that if they have moves left
 at the end of the goto, then they are still in focus.
**************************************************************************/
function update_unit_focus()
{

  /* iterate zero times for no units in focus,
   * otherwise quit for any of the conditions. */
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 

    if (unit_has_goto(punit)
	  && punit['movesleft'] > 0 
	  && punit['done_moving'] == false
	  && punit['ai'] == false) {
      return;
    }

  }
  
  advance_unit_focus();

}

/**************************************************************************
 This function may be called from packhand.c, via update_unit_focus(),
 as a result of packets indicating change in activity for a unit. Also
 called when user press the "Wait" command.
 
 FIXME: Add feature to focus only units of a certain category.
**************************************************************************/
function advance_unit_focus()
{
  if (client_is_observer()) return;
  
  var funits = get_units_in_focus();

  var candidate = find_best_focus_candidate(false);
  
  if (candidate == null) {
    candidate = find_best_focus_candidate(true);
  }
  
  if (candidate != null) {
    set_unit_focus_and_redraw(candidate);
  } else {
    /* Couldn't center on a unit, then try to center on a city... */
    current_focus = []; /* Reset focus units. */
    update_unit_info_label(current_focus);
    $("#game_unit_orders_default").hide();
    /* find a city to focus on. */
    for (city_id in cities) {
      var pcity = cities[city_id];
      if (city_owner_player_id(pcity) == client.conn.playing.playerno) {
        center_tile_mapcanvas(city_tile(pcity));
        break;
      }
    }  
  }

}

/**************************************************************************
 Find the nearest available unit for focus, excluding any current unit
 in focus unless "accept_current" is TRUE.  If the current focus unit
 is the only possible unit, or if there is no possible unit, returns NULL.
**************************************************************************/
function find_best_focus_candidate(accept_current)
{
  if (client_is_observer()) return null;
  
  for (unit_id in units) {
    var punit = units[unit_id];
    if ((!unit_is_in_focus(punit) || accept_current)
	  && punit['owner'] == client.conn.playing.playerno
	  && punit['activity'] == ACTIVITY_IDLE
	  && punit['movesleft'] > 0
	  && punit['done_moving'] == false
	  && punit['ai'] == false
	  && punit['transported'] == false) {
	  return punit;
    }
  }

  return null;
}


/**************************************************************************
  Sets the focus unit directly.  The unit given will be given the
  focus; if NULL the focus will be cleared.

  This function is called for several reasons.  Sometimes a fast-focus
  happens immediately as a result of a client action.  Other times it
  happens because of a server-sent packet that wakes up a unit.
**************************************************************************/
function set_unit_focus(punit)
{
  current_focus = [];
  current_focus[0] = punit;
}

/**************************************************************************
 See set_unit_focus()
**************************************************************************/
function set_unit_focus_and_redraw(punit)
{
  current_focus = [];
  
  if (punit != null) {
    current_focus[0] = punit;
  }
  
  update_map_canvas_full();
  update_unit_info_label(current_focus);
  $("#game_unit_orders_default").show();
}

/**************************************************************************
 See set_unit_focus_and_redraw()
**************************************************************************/
function city_dialog_activate_unit(punit)
{
  request_new_unit_activity(punit, ACTIVITY_IDLE)
  close_city_dialog();
  set_unit_focus_and_redraw(punit);
}

/**************************************************************************
Center on the focus unit, if off-screen and auto_center_on_unit is true.
**************************************************************************/
function auto_center_on_focus_unit()
{
  var ptile = find_a_focus_unit_tile_to_center_on();
  
  if (ptile != null && auto_center_on_unit) {
    center_tile_mapcanvas(ptile);
  }
}

/****************************************************************************
  Finds a single focus unit that we can center on.  May return NULL.
****************************************************************************/
function find_a_focus_unit_tile_to_center_on()
{
  var funit = current_focus[0];
  
  if (funit == null) return null;
  
  return map_pos_to_tile(funit['x'], funit['y']);
}

/**************************************************************************
Return a pointer to a visible unit, if there is one.
**************************************************************************/
function find_visible_unit(ptile)
{
  var panyowned = null;
  var panyother = null;
  var ptptother = null;
  var pfocus = null;

  /* If no units here, return nothing. */
  if (unit_list_size(tile_units(ptile))==0) {
    return null;
  }
  
  /* If the unit in focus is at this tile, show that on top */
  var pfocus = get_focus_unit_on_tile(ptile);
  if (pfocus != null) {
    return pfocus;
  }
  
  /* If a city is here, return nothing (unit hidden by city). */
  if (tile_city(ptile) != null) {
    return null;
  }
  
  /* TODO: add missing C logic here.*/
  var vunits = tile_units(ptile);
  for (var i = 0; i < vunits.length; i++) {
    var tunit = vunits[i];
    if (tunit['transported'] == false) {
      return tunit;
    } 
  }   

  return tile_units(ptile)[0];
}

/**********************************************************************
TODO: not complete yet
***********************************************************************/
function get_drawable_unit(ptile, citymode)
{
  var punit = find_visible_unit(ptile);

  if (punit == null) return null;

  /*if (citymode != null && unit_owner(punit) == city_owner(citymode))
    return null;*/
  
  if (!unit_is_in_focus(punit) || current_focus.length > 0 ) {
    return punit;
  } else {
    return null;
  }
}

/**************************************************************************
 Handles everything when the user clicked a tile
**************************************************************************/
function do_map_click(ptile, qtype)
{
  if (ptile == null) return;
  
  if (goto_active) {
    if (current_focus.length > 0) {
      var punit = current_focus[0];
      var packet = [{"packet_type" : "unit_move", "unit_id" : punit['id'], "x": ptile['x'], "y": ptile['y'] }];
      send_request (JSON.stringify(packet));
    }
  
    deactivate_goto();
    update_unit_focus();
  
  } else {
    var sunits = tile_units(ptile);
    
    var pcity = tile_city(ptile);
    
    if (pcity != null) {
      if (pcity['owner'] == client.conn.playing.playerno) {
        show_city_dialog(pcity);
      }
      return;
    }
  
    if (sunits != null && sunits.length > 0 ) {
      if (sunits[0]['owner'] == client.conn.playing.playerno) {
        if (sunits.length == 1) {
          /* A single unit has been clicked with the mouse. */
          var unit = sunits[0];  
          set_unit_focus_and_redraw(unit);
          request_new_unit_activity(unit, ACTIVITY_IDLE)
        } else {
          /* more than one unit is on the selected tile. */
          set_unit_focus_and_redraw(sunits[0]);
          update_select_unit_dialog(sunits);
        } 
      }
    }
    
    if (left_click_center()) {
      center_tile_mapcanvas(ptile)   
    }
    
  }
   
}

/**************************************************************************
 Callback to handle keyboard events
**************************************************************************/
function keyboard_listener(ev)
{
  // Check if focus is in chat field, where these keyboard events are ignored.
  if (!keyboard_input) return;

  if (!ev) ev = window.event;
  var keyboard_key = String.fromCharCode(ev.keyCode);

  civclient_handle_key(keyboard_key, ev.keyCode, ev['ctrlKey'],  ev['altKey'], ev['shiftKey']  );

  

}

/**************************************************************************
 Handles everything when the user typed on the keyboard.
**************************************************************************/
function
civclient_handle_key(keyboard_key, key_code, ctrl, alt, shift)
{

  switch (keyboard_key) {
    case 'B':
      request_unit_build_city();
 
      break;
      
    case 'G':
      if (current_focus.length > 0) {
        activate_goto();
      }
    break;
      
    case 'X':
      key_unit_auto_explore();
    break;  
    
    case 'A':
      key_unit_auto_settle();
    break;
    
    case 'R':
      key_unit_road();
    break;

    case 'F':
      key_unit_fortify();
    break;   
      
    case 'I':
      key_unit_irrigate();
    break;      
    
    case 'P':
      key_unit_pillage();
    break;
    
    case 'M':
      key_unit_mine();
    break;
    
    case 'Q':
      if (alt) civclient_benchmark(0); 
    break;

  };
  
  switch (key_code) {
    case 35: //1
    case 97:
      key_unit_move(DIR8_SOUTH);
    break;
  
    case 40: // 2
    case 98:
      key_unit_move(DIR8_SOUTHEAST);  
      break;

    case 34: // 3
    case 99:
      key_unit_move(DIR8_EAST);
      break;

    case 37: // 4
    case 100:
      key_unit_move(DIR8_SOUTHWEST);
      break;

    case 39: // 6
    case 102:
      key_unit_move(DIR8_NORTHEAST);
      break;

    case 36: // 7
    case 103:
      key_unit_move(DIR8_WEST);
      break;

    case 38: // 8
    case 104:
      key_unit_move(DIR8_NORTHWEST);
      break;

    case 33: // 9
    case 105:
      key_unit_move(DIR8_NORTH);
      break;
  
  
  };
  
}

/**************************************************************************
 ...
**************************************************************************/
function activate_goto()
{
  goto_active = true;
  mapview_canvas.style.cursor = "crosshair";
  
  if (intro_click_description) {
    add_chatbox_text("Click on the tile to send this unit to.");
    intro_click_description = false;
  }
}

/**************************************************************************
 ...
**************************************************************************/
function deactivate_goto()
{
  goto_active = false;
  mapview_canvas.style.cursor = "default";

}

/**************************************************************************
 Ends the current turn.
**************************************************************************/
function send_end_turn()
{
  var packet = [{"packet_type" : "player_phase_done", "turn" : game_info['turn']}];
  send_request (JSON.stringify(packet));
}


/**************************************************************************
 Tell the units in focus to auto explore.
**************************************************************************/
function key_unit_auto_explore()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_EXPLORE);
  }  
  update_unit_focus();
}

/**************************************************************************
 Tell the units in focus to fortify.
**************************************************************************/
function key_unit_fortify()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
  }  
  update_unit_focus();
}

/**************************************************************************
 Tell the units in focus to mine.
**************************************************************************/
function key_unit_irrigate()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_IRRIGATE);
  }  
  update_unit_focus();
}

/**************************************************************************
 Tell the units in focus to pillage.
**************************************************************************/
function key_unit_pillage()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_PILLAGE);
  }  
  update_unit_focus();
}

/**************************************************************************
 Tell the units in focus to mine.
**************************************************************************/
function key_unit_mine()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_MINE);
  }  
  update_unit_focus();
}

/**************************************************************************
 Tell the units in focus to build road.  (FIXME: railroads)
**************************************************************************/
function key_unit_road()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_ROAD);
  }  
  update_unit_focus();
}

/**************************************************************************
  Call to request (from the server) that the focus unit is put into
  autosettler mode.
**************************************************************************/
function key_unit_auto_settle()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_unit_autosettlers(punit);
  }  
  update_unit_focus();
}



/**************************************************************************
 ...
**************************************************************************/
function request_new_unit_activity(punit, activity)
{

  var packet = [{"packet_type" : "unit_change_activity", "unit_id" : punit['id'],
                "activity" : activity, "activity_target" : S_LAST, "activity_base" : 0}];
  send_request (JSON.stringify(packet));
}

/****************************************************************************
  Call to request (from the server) that the settler unit is put into
  autosettler mode.
****************************************************************************/
function request_unit_autosettlers(punit)
{
  if (punit != null ) {
    var packet = [{"packet_type" : "unit_autosettlers", "unit_id" : punit['id']}];
    send_request (JSON.stringify(packet));
  }
}


/****************************************************************************
  Request that a city is built.
****************************************************************************/
function request_unit_build_city()
{
  if (current_focus.length > 0) {
    var punit = current_focus[0];
    if (punit != null) { 
      var packet = [{"packet_type" : "unit_build_city", "name" : "default", "unit_id" : punit['id'] }];
      send_request (JSON.stringify(packet));
    }
  }
}

/**************************************************************************
 Tell the units in focus to disband.
**************************************************************************/
function key_unit_disband()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    var packet = [{"packet_type" : "unit_disband", "unit_id" : punit['id'] }];
    send_request (JSON.stringify(packet));
  }  
}

/**************************************************************************
 Moved the unit in focus in the specified direction.
**************************************************************************/
function key_unit_move(dir) 
{
  if (current_focus.length > 0) {
    var punit = current_focus[0];
    if (punit == null) return;
    
    var ptile = map_pos_to_tile(punit['x'], punit['y']);
    if (ptile == null) return;
    
    var newtile = mapstep(ptile, dir);
    if (newtile == null) return;
        
    var packet = [{"packet_type" : "unit_move", "unit_id" : punit['id'], "x": newtile['x'], "y": newtile['y'] }];
    send_request (JSON.stringify(packet));
        
  }
  
  deactivate_goto(); 
  //update_unit_focus();
  
}

/**************************************************************************
 ...
**************************************************************************/
function process_diplomat_arrival(pdiplomat, target_id)
{
  var pcity = game_find_city_by_number(target_id);
  var punit = game_find_unit_by_number(target_id);
  if (punit != null) {
 
    var packet = [{"packet_type" : "unit_diplomat_action", 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": target_id, 
                   "value" : 0, 
                   "action_type": DIPLOMAT_BRIBE}];
    send_request (JSON.stringify(packet));

  
  } else if (pcity != null) {
    var packet = [{"packet_type" : "unit_diplomat_action", 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": target_id, 
                   "value" : 0, 
                   "action_type": DIPLOMAT_INCITE}];
    send_request (JSON.stringify(packet));
 
  }

}


/**************************************************************************
 ...
**************************************************************************/
function left_click_center()
{
  return (is_iphone() || jQuery.browser.opera);
}
