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
var prev_mouse_x;
var prev_mouse_y;
var keyboard_input = true;
var unitpanel_active = false;
var allow_right_click = false;

var current_focus = [];
var goto_active = false;
var goto_preview_active = true;

/* Selecting unit from a stack without popup. */
var SELECT_POPUP = 0;
var SELECT_SEA = 1;
var SELECT_LAND = 2;
var SELECT_APPEND = 3;

var intro_click_description = true; 

var goto_request_map = {};
var goto_turns_request_map = {};
var current_goto_path = [];
var current_goto_turns = 0;

/****************************************************************************
...
****************************************************************************/
function control_init() 
{
  // Register keyboard and mouse listener using JQuery.
  $(document).keydown (keyboard_listener);
  $("#canvas").mouseup(mapview_mouse_click);
  $("#canvas").mousedown(mapview_mouse_down);
  $(window).mousemove(mouse_moved_cb);
  $(window).resize(mapview_window_resized);
  $(window).bind('orientationchange resize', orientation_changed);

  if (is_touch_device()) {
    $('#canvas').bind('touchstart', mapview_touch_start);
    $('#canvas').bind('touchend', mapview_touch_end);
    $('#canvas').bind('touchmove', mapview_touch_move);
  }

  $("#city_canvas").click(city_mapview_mouse_click);
  
  $("#turn_done_button").click(send_end_turn);
  $("#freeciv_logo").click(function(event) {
    window.open('http://play.freeciv.org/', '_new');
    });


  $("#game_text_input").keydown(function(event) {
	  return check_text_input(event, $("#game_text_input"));
    });
  $("#game_text_input").focus(function(event) {
	keyboard_input=false;
    });

  $("#game_text_input").blur(function(event) {
	  keyboard_input=true;
    });

  /* disable text-selection, as this gives wrong mouse cursor 
   * during drag to goto units. */
  document.onselectstart = function(){ return false; }

  /* disable right clicks. */
  document.oncontextmenu = function(){return allow_right_click;};
 
  $(window).bind('beforeunload', function(){
    return "Do you really want to leave your nation behind now?";
  });

  $(window).on('unload', function(){
    send_surrender_game();
    network_stop();
  });

  /* Click callbacks for main tabs. */
  $("#map_tab").click(function(event) {
    set_default_mapview_active();
  });

  $("#civ_tab").click(function(event) {
    city_dialog_remove();
    set_default_mapview_inactive();
    init_civ_dialog();
  });

  $("#tech_tab").click(function(event) {
    city_dialog_remove(); 
    set_default_mapview_inactive(); 
    update_tech_screen();
  });

  $("#players_tab").click(function(event) {
    city_dialog_remove(); 
    set_default_mapview_inactive(); 
    update_nation_screen();
  });

  $("#opt_tab").click(function(event) {
    city_dialog_remove();
    init_options_dialog(); 
    set_default_mapview_inactive();
  });

  $("#chat_tab").click(function(event) {
    city_dialog_remove();
    chatbox_resized();
    set_default_mapview_inactive();
  });


  $("#hel_tab").click(function(event) {
    city_dialog_remove();
    set_default_mapview_inactive();
    show_help();
  });





}

/****************************************************************************
 determined if this is a touch enabled device, such as iPhone, iPad.
****************************************************************************/
function is_touch_device() {
  if(('ontouchstart' in window) || 'onmsgesturechange' in window 
      || window.DocumentTouch && document instanceof DocumentTouch) {    
    return true;
  } else {
    return false;
  }
}

/****************************************************************************
...
****************************************************************************/
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
  if (active_city == null && mapview_canvas != null) {
    mouse_x = mouse_x - $("#canvas").offset().left;
    mouse_y = mouse_y - $("#canvas").offset().top;
  } else if (active_city != null && city_canvas != null) {
    mouse_x = mouse_x - $("#city_canvas").offset().left;
    mouse_y = mouse_y - $("#city_canvas").offset().top;

  }

  if (client.conn.playing == null) return;

  /* Request preview goto path */
  if (goto_active && current_focus.length > 0) {
    var ptile = canvas_pos_to_tile(mouse_x, mouse_y);
    if (ptile != null && goto_request_map[ptile['x'] + "," + ptile['y']] == null) {
      preview_goto_path(current_focus[0]['id'], ptile['x'], ptile['y']);
    }
  }

  if (C_S_RUNNING == client_state()) {
    update_mouse_cursor();
  }

}

/****************************************************************************
...
****************************************************************************/
function update_mouse_cursor()
{
  var ptile = canvas_pos_to_tile(mouse_x, mouse_y);

  if (ptile == null) return;

  var punit = find_visible_unit(ptile);
  var pcity = tile_city(ptile);

  if (goto_active) {
    mapview_canvas.style.cursor = "crosshair";
  } else if (pcity != null && city_owner_player_id(pcity) == client.conn.playing.playerno) { 
    mapview_canvas.style.cursor = "pointer";
  } else if (punit != null && punit['owner'] == client.conn.playing.playerno) {
    mapview_canvas.style.cursor = "move";
  } else {
    mapview_canvas.style.cursor = "default";
  }

}

/****************************************************************************
...
****************************************************************************/
function check_text_input(event,chatboxtextarea) {
	 
  if (event.keyCode == 13 && event.shiftKey == 0)  {
    var message = $(chatboxtextarea).val();
    message = escape(message.replace(/^\s+|\s+$/g,""));

    $(chatboxtextarea).val('');
    $(chatboxtextarea).focus();
    keyboard_input = true;
    var test_packet = {"type" : packet_chat_msg_req, "message" : message};
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
    if (punit['tile'] == ptile['index']) {
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
  if (active_city != null) return; /* don't change focus while city dialog is active.*/

  /* iterate zero times for no units in focus,
   * otherwise quit for any of the conditions. */
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 

    if (punit['movesleft'] > 0 
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
 Enables and disables the correct units commands for the unit in focus.
**************************************************************************/
function update_unit_order_commands()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    var ptype = unit_type(punit);
    if (ptype['name'] == "Settlers" || ptype['name'] == "Workers" 
        || ptype['name'] == "Engineers") {
      $("#order_road").show();
      $("#order_mine").show();
      $("#order_irrigate").show();
      $("#order_fortify").hide();
      $("#order_sentry").hide();
      $("#order_explore").hide();
      $("#order_auto_settlers").show();
      if (player_invention_state(client.conn.playing, 65) == TECH_KNOWN) {
        $("#order_railroad").show();
      }
    } else {
      $("#order_road").hide();
      $("#order_railroad").hide();
      $("#order_mine").hide();
      $("#order_irrigate").hide();
      $("#order_fortify").show();
      $("#order_auto_settlers").hide();
      $("#order_sentry").show();
      $("#order_explore").show();

    }

    if (ptype['name'] == "Settlers" || ptype['name'] == "Engineers") {
      $("#order_build_city").show();
    } else {
      $("#order_build_city").hide();
    }

  }

}


/**************************************************************************
...
**************************************************************************/
function init_game_unit_panel()
{
  unitpanel_active = true;

  $("#game_unit_panel").attr("title", "Units");		
  $("#game_unit_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "250px",
			height: "auto",
			resizable: false,
			dialogClass: 'unit_dialog',
			position: ["right", "bottom"],
			close: function(event, ui) { unitpanel_active = false;}

		});
	
  $("#game_unit_panel").dialog('open');		
  $(".unit_dialog div.ui-dialog-titlebar").css("height", "5px");
  $(".unit_dialog div.ui-dialog-content").css("padding", "5px 0");
  $("#ui-dialog-title-game_unit_panel").css("margin-top", "-5px");
  $("#ui-dialog-title-game_unit_panel").css("font-size", "10px");
  $("#game_unit_panel").parent().css("overflow", "hidden");		
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
  
  if (punit == null) {
    current_focus = [];
  } else {
    current_focus[0] = punit;
  }

  auto_center_on_focus_unit(); 
  update_unit_info_label(current_focus);
  update_unit_order_commands();
  $("#game_unit_orders_default").show();

}

/**************************************************************************
 ...
**************************************************************************/
function set_unit_focus_and_activate(punit)
{
  set_unit_focus_and_redraw(punit);
  request_new_unit_activity(punit, ACTIVITY_IDLE);

}

/**************************************************************************
 See set_unit_focus_and_redraw()
**************************************************************************/
function city_dialog_activate_unit(punit)
{
  request_new_unit_activity(punit, ACTIVITY_IDLE);
  close_city_dialog();
  set_unit_focus_and_redraw(punit);
}

/**************************************************************************
Center on the focus unit, if off-screen and auto_center_on_unit is true.
**************************************************************************/
function auto_center_on_focus_unit()
{
  if (active_city != null) return; /* don't change focus while city dialog is active.*/

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
  
  return index_to_tile(funit['tile']);
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
    var aunit = vunits[i];
    if (aunit['anim_list'] != null && aunit['anim_list'].length > 0) {
      return aunit;
    }
  }

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

      if (punit['tile'] == ptile['index']) {
	/* if unit moved by click and drag to the same tile, then deactivate goto. */
        deactivate_goto();
        return;
      }

      var packet = {"type" : packet_unit_move, "unit_id" : punit['id'], "tile": ptile['index'] };
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
    
    if (sunits != null && sunits.length == 0) {
      /* Clicked on a tile with no units. */
      set_unit_focus_and_redraw(null);
    } else if (sunits != null && sunits.length > 0 ) {
      if (sunits[0]['owner'] == client.conn.playing.playerno) {
         if (sunits.length == 1) {
          /* A single unit has been clicked with the mouse. */
          var unit = sunits[0];  
	  set_unit_focus_and_activate(unit);
        } else {
          /* more than one unit is on the selected tile. */
          set_unit_focus_and_redraw(sunits[0]);
          update_select_unit_dialog(sunits);
        } 
      }
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

    case 'W':
      key_unit_wait();
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

    case 'S':
      key_unit_sentry();
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

    case 'D':
      if (alt) show_debug_info(); 
    break;

    case 'L':
      key_unit_railroad();
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

  if (current_focus.length > 0) {
    var ptile = canvas_pos_to_tile(mouse_x, mouse_y);
    if (ptile != null) {
      preview_goto_path(current_focus[0]['id'], ptile['x'], ptile['y']);
    }

    if (intro_click_description) {
      add_chatbox_text("Click on the tile to send this unit to.");
      intro_click_description = false;
    }

  }

}

/**************************************************************************
 ...
**************************************************************************/
function check_goto()
{
  if (is_touch_device()) {
    goto_preview_active = false;
  }

  activate_goto();

}

/**************************************************************************
 ...
**************************************************************************/
function deactivate_goto()
{
  goto_active = false;
  mapview_canvas.style.cursor = "default";
  goto_request_map = {};
  goto_turns_request_map = {};
  current_goto_path = [];
  goto_preview_active = true;

  // update focus to next unit after 600ms.
  setTimeout("update_unit_focus();", 600);


}

/**************************************************************************
 Ends the current turn.
**************************************************************************/
function send_end_turn()
{
  var packet = {"type" : packet_player_phase_done, "turn" : game_info['turn']};
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
 Tell the unit to wait (focus to next unit with moves left)
**************************************************************************/
function key_unit_wait()
{
  advance_unit_focus();
}

/**************************************************************************
 Tell the units in focus to sentry.
**************************************************************************/
function key_unit_sentry()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_SENTRY);
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
 Tell the units in focus to irrigate.
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
 Tell the units in focus to build road.  
**************************************************************************/
function key_unit_road()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity_road(punit, ACTIVITY_GEN_ROAD, 0);
  }
  update_unit_focus();
}

/**************************************************************************
 Tell the units in focus to build railroads.  
**************************************************************************/
function key_unit_railroad()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i]; 
    request_new_unit_activity(punit, ACTIVITY_GEN_ROAD, 1);
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

  var packet = {"type" : packet_unit_change_activity, "unit_id" : punit['id'],
                "activity" : activity, "activity_target" : S_LAST };
  send_request (JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function request_new_unit_activity_base(punit, activity, base)
{

  var packet = {"type" : packet_unit_change_activity_base, "unit_id" : punit['id'],
                "activity" : activity, "activity_base" : base };
  send_request (JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function request_new_unit_activity_road(punit, activity, road)
{

  var packet = {"type" : packet_unit_change_activity_road, "unit_id" : punit['id'],
                "activity" : activity, "activity_road" : road };
  send_request (JSON.stringify(packet));
}

/****************************************************************************
  Call to request (from the server) that the settler unit is put into
  autosettler mode.
****************************************************************************/
function request_unit_autosettlers(punit)
{
  if (punit != null ) {
    var packet = {"type" : packet_unit_autosettlers, "unit_id" : punit['id']};
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

      if (punit['movesleft'] == 0) {
        add_chatbox_text("Unit has no moves left to build city");
        return;
      }

      var ptype = unit_type(punit);
      if (ptype['name'] == "Settlers" || ptype['name'] == "Engineers") {
        var packet = {"type" : packet_city_name_suggestion_req, "unit_id" : punit['id'] };
        send_request (JSON.stringify(packet));
      } 

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
    var packet = {"type" : packet_unit_disband, "unit_id" : punit['id'] };
    send_request (JSON.stringify(packet));

    /* Also remove unit immediately in client, to ensure it is removed. */
    clear_tile_unit(punit);
    client_remove_unit(punit);

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
    
    var ptile = index_to_tile(punit['tile']);
    if (ptile == null) return;
    
    var newtile = mapstep(ptile, dir);
    if (newtile == null) return;
        
    var packet = {"type" : packet_unit_move, "unit_id" : punit['id'], "tile": newtile['index'] };
    send_request (JSON.stringify(packet));
        
  }
  
  deactivate_goto(); 
  
}

/**************************************************************************
 ...
**************************************************************************/
function process_diplomat_arrival(pdiplomat, target_id)
{
  var pcity = game_find_city_by_number(target_id);
  var punit = game_find_unit_by_number(target_id);
  if (punit != null) {
    popup_diplomat_dialog(pdiplomat, punit, null);
  } else if (pcity != null) {
    popup_diplomat_dialog(pdiplomat, null, pcity);
  }
}



/****************************************************************************
  Calculates a preview of the goto path, based on info in the client only.
  FIXME: This doesn't support map wrapping.
****************************************************************************/
function preview_goto_path(unit_id, dst_x, dst_y)
{
  var start_tile = index_to_tile(units[unit_id]['tile']);
  current_goto_path = [];
  generate_preview_path(start_tile['x'], start_tile['y'], dst_x, dst_y);
}

/**************************************************************************
 ...
**************************************************************************/
function generate_preview_path(sx, sy, dx, dy)
{

  var ptile = map_pos_to_tile(sx, sy);
  current_goto_path.push(ptile);
	
  /* Check if full path has been found*/
  if (sx == dx && sy == dy) return;

  if (sx < dx && sy == dy) {
    generate_preview_path(sx+1, sy, dx, dy);
  } else if (sx > dx && sy == dy) {
    generate_preview_path(sx-1, sy, dx, dy);
  } else if (sx == dx && sy < dy) {
    generate_preview_path(sx, sy+1, dx, dy);
  } else if (sx == dx && sy > dy) {
    generate_preview_path(sx, sy-1, dx, dy);
  } else if (sx > dx && sy > dy) {
    generate_preview_path(sx-1, sy-1, dx, dy);
  } else if (sx < dx && sy < dy) {
    generate_preview_path(sx+1, sy+1, dx, dy);
  } else if (sx < dx && sy > dy) {
    generate_preview_path(sx+1, sy-1, dx, dy);
  } else if (sx > dx && sy < dy) {
    generate_preview_path(sx-1, sy+1, dx, dy);
  }

}

/****************************************************************************
  Request GOTO path for unit with unit_id, and dst_x, dst_y in map coords.
****************************************************************************/
function request_goto_path(unit_id, dst_x, dst_y)
{
  if (goto_request_map[dst_x + "," + dst_y] == null) {
    goto_request_map[dst_x + "," + dst_y] = true;

    var packet = {"type" : packet_goto_path_req, "unit_id" : unit_id,
                  "goal" : map_pos_to_tile(dst_x, dst_y)['index']};
    send_request (JSON.stringify(packet));
  
  } else {
    current_goto_path = goto_request_map[dst_x + "," + dst_y];
    current_goto_turns = goto_turns_request_map[dst_x + "," + dst_y];
  }
}

/****************************************************************************
...
****************************************************************************/
function check_request_goto_path()
{
 if (goto_active && current_focus.length > 0 
      && prev_mouse_x == mouse_x && prev_mouse_y == mouse_y) {
    var ptile = canvas_pos_to_tile(mouse_x, mouse_y);
    if (ptile != null) {
      /* Send request for goto_path to server. */
      request_goto_path(current_focus[0]['id'], ptile['x'], ptile['y']);
    }
  }
  prev_mouse_x = mouse_x;
  prev_mouse_y = mouse_y;

}

/****************************************************************************
  Show the GOTO path in the unit_goto_path packet.
****************************************************************************/
function show_goto_path(goto_packet)
{
  var punit = units[goto_packet['unit_id']];
  var t0 =  index_to_tile(punit['tile']);
  var ptile = t0;
  var goaltile = index_to_tile(goto_packet['dest']);
  current_goto_path = [];
  current_goto_path.push(ptile);

  for (var i = 0; i < goto_packet['dir'].length; i++) {
    var dir = goto_packet['dir'][i];
    ptile = mapstep(ptile, dir);
    current_goto_path.push(ptile);
  }

  current_goto_turns = goto_packet['turns'];

  goto_request_map[goaltile['x'] + "," + goaltile['y']] 
	  = current_goto_path;
  goto_turns_request_map[goaltile['x'] + "," + goaltile['y']] 
	  = current_goto_turns;
}

/****************************************************************************
 ...
****************************************************************************/
function popup_caravan_dialog(punit, traderoute, wonder)
{
  var id = "#caravan_dialog_" + punit['id'];
  $(id).remove();
  $("<div id='caravan_dialog_" + punit['id'] + "'></div>").appendTo("div#game_page");

  var homecity = cities[punit['homecity']];
  var ptile = index_to_tile(punit['index']);
  var pcity = tile_city(ptile);


  var dhtml = "<center>Your caravan from " + unescape(homecity['name']) + " reaches the city of "  
	    + unescape(pcity['name']) + ". What now? <br>"
	    + "<input id='car_trade' class='car_button' type='button' value='Establish Traderoute'>"
	    + "<input id='car_wonder' class='car_button' type='button' value='Help build Wonder'>"
	    + "<input id='car_cancel' class='car_button' type='button' value='Cancel'>"
	    + "</center>"
  $(id).html(dhtml);

  $(id).attr("title", "Your Caravan Has Arrived");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: "350"});
	
  $(id).dialog('open');		
  $(".car_button").button();
  $(".car_button").css("width", "250px");


  if (!traderoute) $("#car_trade").button( "option", "disabled", true);
  if (!wonder) $("#car_wonder").button( "option", "disabled", true);

  $("#car_trade").click(function() {
    var packet = {"type" : packet_unit_establish_trade, 
                   "unit_id": punit['id']};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });

  $("#car_wonder").click(function() {
    var packet = {"type" : packet_unit_help_build_wonder, 
                   "unit_id": punit['id']};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });


}

