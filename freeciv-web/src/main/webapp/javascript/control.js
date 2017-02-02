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


var mouse_x;
var mouse_y;
var prev_mouse_x;
var prev_mouse_y;
var keyboard_input = true;
var unitpanel_active = false;
var allow_right_click = false;

var current_focus = [];
var goto_active = false;
var paradrop_active = false;
var airlift_active = false;
var action_tgt_sel_active = false;

/* Will be set when the goto is activated. */
var goto_last_order = -1;
var goto_last_action = -1;

/* Selecting unit from a stack without popup. */
var SELECT_POPUP = 0;
var SELECT_SEA = 1;
var SELECT_LAND = 2;
var SELECT_APPEND = 3;

var intro_click_description = true;
var resize_enabled = true;
var goto_request_map = {};
var goto_turns_request_map = {};
var current_goto_turns = 0;
var waiting_units_list = [];
var show_citybar = true;
var context_menu_active = true;
var has_movesleft_warning_been_shown = false;

/****************************************************************************
...
****************************************************************************/
function control_init()
{

  if (renderer == RENDERER_2DCANVAS) {
    mapctrl_init_2d();
  } else {
    init_webgl_mapctrl();
  }

  $(document).keydown(keyboard_listener);
  $(window).resize(mapview_window_resized);
  $(window).bind('orientationchange resize', orientation_changed);

  $("#turn_done_button").click(send_end_turn);
  if (!is_touch_device()) $("#turn_done_button").tooltip();

  $("#freeciv_logo").click(function(event) {
    window.open('/', '_new');
    });


  $("#game_text_input").keydown(function(event) {
	  return check_text_input(event, $("#game_text_input"));
  });
  $("#game_text_input").focus(function(event) {
    keyboard_input=false;
    resize_enabled = false;
  });

  $("#game_text_input").blur(function(event) {
    keyboard_input=true;
    resize_enabled = true;
  });

  $("#pregame_text_input").keydown(function(event) {
   return check_text_input(event, $("#pregame_text_input"));
  });

  $("#pregame_text_input").blur(function(event) {
      keyboard_input=true;
      if (this.value=='') {
        $("#pregame_text_input").value='>'
      }
  });

  $("#pregame_text_input").focus(function(event) {
    keyboard_input=false; if (this.value=='>') this.value='';
  });

  $("#start_game_button").click(function(event) {
    pregame_start_game();
  });

  $("#load_game_button").click(function(event) {
      show_load_game_dialog();
  });

  $("#pick_nation_button").click(function(event) {
    pick_nation(null);
  });

  $("#pregame_settings_button").click(function(event) {
    pregame_settings();
  });

  /* disable text-selection, as this gives wrong mouse cursor
   * during drag to goto units. */
  document.onselectstart = function(){ return false; };

  /* disable right clicks. */
  document.oncontextmenu = function(){return allow_right_click;};

  $.contextMenu({
        selector: (renderer == RENDERER_2DCANVAS) ? '#canvas' : '#canvas_div' ,
	    zIndex: 5000,
        autoHide: true,
        callback: function(key, options) {
          handle_context_menu_callback(key);
        },
        build: function($trigger, e) {
            if (!context_menu_active) {
              context_menu_active = true;
              return false;
            }
            var unit_actions = update_unit_order_commands();
            return {
                 callback: function(key, options) {
                   handle_context_menu_callback(key);
                  } ,
                 items: unit_actions
            };
        }
  });

  $(window).bind('beforeunload', function(){
    return "Do you really want to leave your nation behind now?";
  });

  $(window).on('unload', function(){
    send_surrender_game();
    network_stop();
  });

  /* Click callbacks for main tabs. */
  $("#map_tab").click(function(event) {
    setTimeout(set_default_mapview_active, 5);
  });


  $("#civ_tab").click(function(event) {
    set_default_mapview_inactive();
    init_civ_dialog();
  });

  $("#tech_tab").click(function(event) {
    set_default_mapview_inactive();
    update_tech_screen();
  });

  $("#players_tab").click(function(event) {
    set_default_mapview_inactive();
    update_nation_screen();
  });

  $("#cities_tab").click(function(event) {
    set_default_mapview_inactive();
    update_city_screen();
  });

  $("#opt_tab").click(function(event) {
    $("#tabs-hel").hide();
    init_options_dialog();
    set_default_mapview_inactive();
  });

  $("#chat_tab").click(function(event) {
    chatbox_resized();
    set_default_mapview_inactive();
    $("#tabs-chat").show();

  });


  $("#hel_tab").click(function(event) {
    set_default_mapview_inactive();
    show_help();
  });

  if (!is_touch_device()) {
    $("#game_unit_orders_default").tooltip();
  }

  $("#game_overview_map").click(function(e) {
    var x = e.pageX - $(this).offset().left;
    var y = e.pageY - $(this).offset().top;
    overview_clicked (x, y);
  });

  $('#meet_player_button').click(nation_meet_clicked);
  $('#view_player_button').click(center_on_player);
  $('#cancel_treaty_button').click(cancel_treaty_clicked);
  $('#take_player_button').click(take_player_clicked);
  $('#toggle_ai_button').click(toggle_ai_clicked);
  $('#game_scores_button').click(view_game_scores);

  /* prevents keyboard input from changing tabs. */
  $('#tabs').tabs({
    activate: function (event, ui) {
        ui.newTab.blur();
    }
  });
  $('#tabs a').click(function () {
    $(this).blur();
  });

}

/****************************************************************************
 determined if this is a touch enabled device, such as iPhone, iPad.
****************************************************************************/
function is_touch_device() 
{
  if(!cardboard_vr_enabled && ('ontouchstart' in window) || 'onmsgesturechange' in window
      || window.DocumentTouch && document instanceof DocumentTouch) {
    return true;
  } else {
    return false;
  }
}

/****************************************************************************
 Remove focus from all input elements on touch devices, since the mobile
 keyboard can be annoying to constantly popup and resize screen etc.  
****************************************************************************/
function blur_input_on_touchdevice() 
{
  if (is_touch_device() || is_small_screen()) {
    $('input[type=text], textarea').blur();
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
    e = window.event;
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
  if (renderer == RENDERER_2DCANVAS && active_city == null && mapview_canvas != null
      && $("#canvas").length) {
    mouse_x = mouse_x - $("#canvas").offset().left;
    mouse_y = mouse_y - $("#canvas").offset().top;
  } else if (renderer == RENDERER_WEBGL && active_city == null && $("#canvas_div").length) {
    mouse_x = mouse_x - $("#canvas_div").offset().left;
    mouse_y = mouse_y - $("#canvas_div").offset().top;
  } else if (active_city != null && city_canvas != null
             && $("#city_canvas").length) {
    mouse_x = mouse_x - $("#city_canvas").offset().left;
    mouse_y = mouse_y - $("#city_canvas").offset().top;

  }

  if (client.conn.playing == null) return;

  if (C_S_RUNNING == client_state()) {
    update_mouse_cursor();
  }

  /* determine if Right-click-and-drag to select multiple units should be activated,
     only if more than an area of 45 pixels has been selected and more than 200ms has past.
     See mapview_mouse_click and mapview_mouse_down. */
  if (map_select_check && Math.abs(mouse_x - map_select_x) > 45
      && Math.abs(mouse_y - map_select_y) > 45
      && (new Date().getTime() - map_select_check_started) > 200)  {
    map_select_active = true;
  }


}

/****************************************************************************
...
****************************************************************************/
function update_mouse_cursor()
{

  if (tech_dialog_active) {
    update_tech_dialog_cursor();
    return;
  }

  var ptile;
  if (renderer == RENDERER_2DCANVAS) {
    ptile = canvas_pos_to_tile(mouse_x, mouse_y);
  } else {
    ptile = webgl_canvas_pos_to_tile(mouse_x, mouse_y);
  }

  if (ptile == null) return;

  var punit = find_visible_unit(ptile);
  var pcity = tile_city(ptile);

  if (goto_active && current_goto_turns != null) {
    /* show goto cursor */
    $("#canvas_div").css("cursor", "crosshair");
  } else if (goto_active && current_goto_turns == null) {
    /* show invalid goto cursor*/
    $("#canvas_div").css("cursor", "not-allowed");
  } else if (pcity != null && city_owner_player_id(pcity) == client.conn.playing.playerno) {
    /* selecti city cursor*/
    $("#canvas_div").css("cursor", "pointer");
  } else if (punit != null && punit['owner'] == client.conn.playing.playerno) {
    /* move unit cursor */
    $("#canvas_div").css("cursor", "move");
  } else {
    $("#canvas_div").css("cursor", "default");
  }

}

/****************************************************************************
...
****************************************************************************/
function check_text_input(event,chatboxtextarea) {

  if (event.keyCode == 13 && event.shiftKey == 0)  {
    var message = $(chatboxtextarea).val();
    message = message.replace(/^\s+|\s+$/g,"");
    message = message.replace("'", "");
    message = encodeURIComponent(message);

    $(chatboxtextarea).val('');
    if (!is_touch_device()) $(chatboxtextarea).focus();
    keyboard_input = true;
    send_message(message);
    $("#game_text_input").blur();
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

  if (C_S_RUNNING != client_state()) return;


  /* iterate zero times for no units in focus,
   * otherwise quit for any of the conditions. */
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];

    if (punit['movesleft'] > 0
	  && punit['done_moving'] == false
	  && punit['ai'] == false
	  && punit['activity'] == ACTIVITY_IDLE) {
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
    if (renderer == RENDERER_WEBGL) webgl_clear_unit_focus();
    update_active_units_dialog();
    $("#game_unit_orders_default").hide();

    /* find a city to focus on if new game. consider removing this.  */
    if (game_info['turn'] <= 1) {
      for (var city_id in cities) {
        var pcity = cities[city_id];
        if (city_owner_player_id(pcity) == client.conn.playing.playerno) {
          center_tile_mapcanvas(city_tile(pcity));
          break;
        }
      }
    }
  }

}

/**************************************************************************
 Enables and disables the correct units commands for the unit in focus.
**************************************************************************/
function update_unit_order_commands()
{
  var i;
  var punit;
  var ptype;
  var pcity;
  var unit_actions = {};
  var funits = get_units_in_focus();
  for (i = 0; i < funits.length; i++) {
    punit = funits[i];
    ptype = unit_type(punit);
    ptile = index_to_tile(punit['tile']);
    if (ptile == null) continue;
    pcity = tile_city(ptile);

    if (pcity != null) {
      unit_actions["show_city"] = {name: "Show city"};
    }

  }

  unit_actions = $.extend(unit_actions, {
            "goto": {name: "Unit goto (G)"},
	    "tile_info": {name: "Tile info"}
            });

  for (i = 0; i < funits.length; i++) {
    punit = funits[i];
    ptype = unit_type(punit);
    ptile = index_to_tile(punit['tile']);
    if (ptile == null) continue;
    pcity = tile_city(ptile);

    if (ptype['name'] == "Settlers") {
      $("#order_build_city").show();
      unit_actions["build"] = {name: "Build city (B)"};
    } else {
      $("#order_build_city").hide();
    }

    if (ptype['name'] == "Settlers" || ptype['name'] == "Workers"
        || ptype['name'] == "Engineers") {
      if (!tile_has_extra(ptile, ROAD_ROAD)) {
        $("#order_road").show();
        $("#order_railroad").hide();
	unit_actions["road"] = {name: "Build road (R)"};
      } else if (player_invention_state(client.conn.playing, 65) == TECH_KNOWN
                 && tile_has_extra(ptile, ROAD_ROAD)
               && !tile_has_extra(ptile, ROAD_RAIL)) {
        $("#order_road").hide();
        $("#order_railroad").show();
	unit_actions['railroad'] = {name: "Build railroad (R)"};
      } else {
        $("#order_road").hide();
        $("#order_railroad").hide();
      }
      $("#order_mine").show();
      $("#order_fortify").hide();
      $("#order_sentry").hide();
      $("#order_explore").hide();
      $("#order_auto_settlers").show();
      $("#order_pollution").show();
      unit_actions["mine"] = {name: "Build mine (M)"};
      unit_actions["autosettlers"] = {name: "Auto settler (A)"};

      if (tile_has_extra(ptile, EXTRA_FALLOUT)) {
        unit_actions["fallout"] = {name: "Remove fallout (N)"};
      }

      if (tile_has_extra(ptile, EXTRA_POLLUTION)) {
        $("#order_pollution").show();
	unit_actions["pollution"] = {name: "Remove pollution (P)"};
      } else {
        $("#order_pollution").hide();
      }

      if (tile_terrain(ptile)['name'] == "Forest") {
        $("#order_forest_remove").show();
        $("#order_irrigate").hide();
	unit_actions["forest"] = {name: "Cut down forest (I)"};
      } else if (!tile_has_extra(ptile, EXTRA_IRRIGATION)) {
        $("#order_irrigate").show();
        $("#order_forest_remove").hide();
        unit_actions["irrigation"] = {name: "Irrigation (I)"};
      } else {
        $("#order_forest_remove").hide();
        $("#order_irrigate").hide();
      }
      unit_actions["fortress"] = {name: string_unqualify(terrain_control['gui_type_base0']) + " (Shift-F)"};
      unit_actions["airbase"] = {name: string_unqualify(terrain_control['gui_type_base1']) + " (E)"};

    } else {
      $("#order_road").hide();
      $("#order_railroad").hide();
      $("#order_mine").hide();
      $("#order_irrigate").hide();
      $("#order_fortify").show();
      $("#order_auto_settlers").hide();
      $("#order_sentry").show();
      $("#order_explore").show();
      $("#order_pollution").hide();

    }

    /* Practically all unit types can currently perform some action. */
    unit_actions["action_selection"] = {name: "Do... (D)"};

    if (ptype['name'] == "Engineers") {
      $("#order_transform").show();
      unit_actions["transform"] = {name: "Transform terrain (O)"};
    } else {
      $("#order_transform").hide();
    }

    if (ptype['name'] == "Nuclear") {
      $("#order_nuke").show();
      unit_actions["nuke"] = {name: "Detonate Nuke At (Shift-N)"};
    } else {
      $("#order_nuke").hide();
    }

    if (ptype['name'] == "Paratroopers") {
      $("#order_paradrop").show();
      unit_actions["paradrop"] = {name: "Paradrop"};
    } else {
      $("#order_paradrop").hide();
    }

    if (ptype['attack_strength'] > 0) {
      $("#order_pillage").show();
      unit_actions["pillage"] = {name: "Pillage (Shift-P)"};
    } else {
      $("#order_pillage").hide();
    }

    if (pcity == null || punit['homecity'] == 0 || (pcity != null && punit['homecity'] == pcity['id'])) {
      $("#order_change_homecity").hide();
    } else if (pcity != null && punit['homecity'] != pcity['id']) {
      $("#order_change_homecity").show();
      unit_actions["homecity"] = {name: "Change homecity of unit (H)"};
    }

    if (pcity != null) {
      unit_actions["airlift"] = {name: "Airlift (Shift-L)"};
    }

  }
  unit_actions = $.extend(unit_actions, {
            "explore": {name: "Auto explore (X)"},
            "fortify": {name: "Fortify (F)"},
            "sentry": {name: "Sentry (S)"},
            "wait": {name: "Wait (W)"},
            "noorders": {name: "No orders (J)"},
            "upgrade": {name: "Upgrade unit (U)"},
            "disband": {name: "Disband (Shift-D)"}
            });

  if (is_touch_device()) {
    $(".context-menu-list").css("width", "600px");
    $(".context-menu-item").css("font-size", "220%");
  }
  $(".context-menu-list").css("z-index", 5000);

  return unit_actions;
}


/**************************************************************************
...
**************************************************************************/
function init_game_unit_panel()
{
  if (observing) return;
  unitpanel_active = true;

  $("#game_unit_panel").attr("title", "Units");
  $("#game_unit_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "370px",
			height: "auto",
			resizable: false,
			closeOnEscape: false,
			dialogClass: 'unit_dialog  no-close',
			position: {my: 'right bottom', at: 'right bottom', of: window, within: $("#game_page")},
			close: function(event, ui) { unitpanel_active = false;}

		});

  $("#game_unit_panel").dialog('open');
  $("#game_unit_panel").parent().css("overflow", "hidden");
}

/**************************************************************************
 Find the nearest available unit for focus, excluding any current unit
 in focus unless "accept_current" is TRUE.  If the current focus unit
 is the only possible unit, or if there is no possible unit, returns NULL.
**************************************************************************/
function find_best_focus_candidate(accept_current)
{
  var punit;
  var i;
  if (client_is_observer()) return null;

  var sorted_units = [];
  for (var unit_id in units) {
    punit = units[unit_id];
    if (client.conn.playing != null && punit['owner'] == client.conn.playing.playerno) {
      sorted_units.push(punit);
    }
  }
  sorted_units.sort(unit_distance_compare);

  for (i = 0; i < sorted_units.length; i++) {
    punit = sorted_units[i];
    if ((!unit_is_in_focus(punit) || accept_current)
       && client.conn.playing != null
       && punit['owner'] == client.conn.playing.playerno
       && punit['activity'] == ACTIVITY_IDLE
       && punit['movesleft'] > 0
       && punit['done_moving'] == false
       && punit['ai'] == false
       && waiting_units_list.indexOf(punit['id']) < 0
       && punit['transported'] == false) {
         return punit;
    }
  }

  /* check waiting units for focus candidates */
  for (i = 0; i < waiting_units_list.length; i++) {
      punit = game_find_unit_by_number(waiting_units_list[i]);
      if (punit != null && punit['movesleft'] > 0) {
        waiting_units_list.splice(i, 1);
        return punit;
      }
  }

  return null;
}

/**************************************************************************
...
**************************************************************************/
function unit_distance_compare(unit_a, unit_b)
{
  if (unit_a == null || unit_b == null) return 0;
  var ptile_a = index_to_tile(unit_a['tile']);
  var ptile_b = index_to_tile(unit_b['tile']);

  if (ptile_a['x'] == ptile_b['x'] && ptile_a['y'] == ptile_b['y']) {
    return 0;
  } else if (ptile_a['x'] > ptile_b['x'] || ptile_a['y'] > ptile_b['y']) {
    return 1;
  } else {
    return -1;
  }
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
  if (punit == null) {
    current_focus = [];
  } else {
    current_focus[0] = punit;
    if (renderer == RENDERER_WEBGL) update_unit_position(index_to_tile(punit['tile']));
  }
  update_active_units_dialog();
  update_unit_order_commands();
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
    if (renderer == RENDERER_WEBGL) update_unit_position(index_to_tile(punit['tile']));
  }

  auto_center_on_focus_unit();
  update_active_units_dialog();
  update_unit_order_commands();
  $("#game_unit_orders_default").show();

}

/**************************************************************************
 ...
**************************************************************************/
function set_unit_focus_and_activate(punit)
{
  set_unit_focus_and_redraw(punit);
  request_new_unit_activity(punit, ACTIVITY_IDLE, EXTRA_NONE);

}

/**************************************************************************
 See set_unit_focus_and_redraw()
**************************************************************************/
function city_dialog_activate_unit(punit)
{
  request_new_unit_activity(punit, ACTIVITY_IDLE, EXTRA_NONE);
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
    update_unit_position(ptile);
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
  var i;

  /* If no units here, return nothing. */
  if (unit_list_size(tile_units(ptile))==0) {
    return null;
  }

  /* If the unit in focus is at this tile, show that on top */
  pfocus = get_focus_unit_on_tile(ptile);
  if (pfocus != null) {
    return pfocus;
  }

  /* If a city is here, return nothing (unit hidden by city). */
  if (tile_city(ptile) != null) {
    return null;
  }

  /* TODO: add missing C logic here.*/
  var vunits = tile_units(ptile);
  for (i = 0; i < vunits.length; i++) {
    var aunit = vunits[i];
    if (aunit['anim_list'] != null && aunit['anim_list'].length > 0) {
      return aunit;
    }
  }

  for (i = 0; i < vunits.length; i++) {
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
  Returns true if the order preferably should be performed from an
  adjacent tile.
**************************************************************************/
function order_wants_direction(order, act_id, ptile) {
  var action = actions[act_id];

  if (order == ORDER_PERFORM_ACTION && action == null) {
    /* Bad action id or action rule data not received and stored
     * properly. */
    console.log("Asked to put invalid action " + act_id + " in an order.");
    return false;
  }

  switch (order) {
  case ORDER_MOVE:
  case ORDER_ACTION_MOVE:
    /* Not only is it legal. It is mandatory. A move is always done in a
     * direction. */
    return true;
  case ORDER_PERFORM_ACTION:
    if (action['min_distance'] > 0) {
      /* Always illegal to do to a target on the actor's own tile. */
      return true;
    }

    if (action['max_distance'] < 1) {
      /* Always illegal to perform to a target on a neighbor tile. */
      return false;
    }

    /* FIXME: allied units and cities shouldn't always make actions be
     * performed from the neighbor tile. */
    if (tile_city(ptile) != null
        || tile_units(ptile).length != 0) {
      /* Won't be able to move to the target tile to perform the action on
       * top of it. */
      return true;
    }

    return false;
  default:
    return false;
  }
}

/**************************************************************************
 Handles everything when the user clicked a tile
**************************************************************************/
function do_map_click(ptile, qtype, first_time_called)
{
  var punit;
  var packet;
  var pcity;
  if (ptile == null || client_is_observer()) return;

  if (current_focus.length > 0 && current_focus[0]['tile'] == ptile['index']) {
    /* clicked on unit at the same tile, then deactivate goto and show context menu. */
    if (goto_active && !is_touch_device()) {
      deactivate_goto();
    }
    if (renderer == RENDERER_2DCANVAS) {
      $("#canvas").contextMenu();
    } else {
      $("#canvas_div").contextMenu();
    }
    return;
  }
  var sunits = tile_units(ptile);
  pcity = tile_city(ptile);

  if (goto_active) {
 
    if (current_focus.length > 0) {
      // send goto order for all units in focus. 
      for (var s = 0; s < current_focus.length; s++) {
        punit = current_focus[s];
        /* Get the path the server sent using PACKET_GOTO_PATH. */
        var goto_path = goto_request_map[punit['id'] + "," + ptile['x'] + "," + ptile['y']];
        if (goto_path == null) {
          continue;
        }

        /* The tile the unit currently is standing on. */
        var old_tile = index_to_tile(punit['tile']);

        /* Create an order to move along the path. */
        packet = {
          "pid"      : packet_unit_orders,
          "unit_id"  : punit['id'],
          "src_tile" : old_tile['index'],
          "length"   : goto_path['length'],
          "repeat"   : false,
          "vigilant" : false,

          /* Each individual order is added later. */

          "dest_tile": ptile['index']
        };

        /* Add each individual order. */
        packet['orders'] = [];
        packet['dir'] = [];
        packet['activity'] = [];
        packet['target'] = [];
        packet['action'] = [];
        for (var i = 0; i < goto_path['length']; i++) {
          /* TODO: Have the server send the full orders in stead of just the
           * dir part. Use that data in stead. */

          if (goto_path['dir'][i] == -1) {
            /* Assume that this means refuel. */
            packet['orders'][i] = ORDER_FULL_MP;
          } else if (i + 1 != goto_path['length']) {
            /* Don't try to do an action in the middle of the path. */
            packet['orders'][i] = ORDER_MOVE;
          } else {
            /* It is OK to end the path in an action. */
            packet['orders'][i] = ORDER_ACTION_MOVE;
          }

          packet['dir'][i] = goto_path['dir'][i];
          packet['activity'][i] = ACTIVITY_LAST;
          packet['target'][i] = EXTRA_NONE;
          packet['action'][i] = ACTION_COUNT;
        }

        if (goto_last_order != ORDER_LAST) {
          /* The final order is specified. */
          var pos;

          /* Should the final order be performed from the final tile or
           * from the tile before it? In some cases both are legal. */
          if (!order_wants_direction(goto_last_order, goto_last_action,
                                     ptile)) {
            /* Append the final order. */
            pos = packet['length'];

            /* Increase orders length */
            packet['length'] = packet['length'] + 1;

            /* Initialize the order to "empthy" values. */
            packet['orders'][pos] = ORDER_LAST;
            packet['dir'][pos] = -1;
            packet['activity'][pos] = ACTIVITY_LAST;
            packet['target'][pos] = EXTRA_NONE;
            packet['action'][pos] = ACTION_COUNT;
          } else {
            /* Replace the existing last order with the final order */
            pos = packet['length'] - 1;
          }

          /* Set the final order. */
          packet['orders'][pos] = goto_last_order;

          /* Perform the final action. */
          packet['action'][pos] = goto_last_action;
        }

        /* The last order has now been used. Clear it. */
        goto_last_order = ORDER_LAST;
        goto_last_action = ACTION_COUNT;

        if (punit['id'] != goto_path['unit_id']) {
          /* Shouldn't happen. Maybe an old path wasn't cleared out. */
          console.log("Error: Tried to order unit " + punit['id']
                      + " to move along a path made for unit "
                      + goto_path['unit_id']);
          return;
        }
        /* Send the order to move using the orders system. */
        send_request(JSON.stringify(packet));
        if (punit['movesleft'] > 0) {
          unit_move_sound_play(punit);
        } else if (!has_movesleft_warning_been_shown) {
          has_movesleft_warning_been_shown = true;
          var ptype = unit_type(punit);
          add_chatbox_text(ptype['name'] + " has no moves left. Press turn done for the next turn.");
        }

      }
      clear_goto_tiles();

    } else if (is_touch_device()) {
      /* this is to handle the case where a mobile touch device user chooses
      GOTO from the context menu or clicks the goto icon. Then the goto path
      has to be requested first, and then do_map_click will be called again
      to issue the unit order based on the goto path. */
      if (ptile != null && current_focus.length > 0) {
        request_goto_path(current_focus[0]['id'], ptile['x'], ptile['y']);
        if (first_time_called) {
          setTimeout(function(){
            do_map_click(ptile, qtype, false)}
          , 250);
        }
        return;

      }
    }

    deactivate_goto();
    update_unit_focus();

  } else if (paradrop_active && current_focus.length > 0) {
    punit = current_focus[0];
    packet = {
      "pid"         : packet_unit_do_action,
      "actor_id"    : punit['id'],
      "target_id"   : ptile['index'],
      "value"       : 0,
      "name"        : "",
      "action_type" : ACTION_PARADROP
    };
    send_request(JSON.stringify(packet));
    paradrop_active = false;

  } else if (airlift_active && current_focus.length > 0) {
    punit = current_focus[0];
    pcity = tile_city(ptile);
    if (pcity != null) {
      packet = {
        "pid"         : packet_unit_do_action,
        "actor_id"    : punit['id'],
        "target_id"   : pcity['id'],
        "value"       : 0,
        "name"        : "",
        "action_type" : ACTION_AIRLIFT
      };
      send_request(JSON.stringify(packet));
    }
    airlift_active = false;

  } else if (action_tgt_sel_active && current_focus.length > 0) {
    request_unit_act_sel_vs(ptile);
    action_tgt_sel_active = false;
  } else {
    if (pcity != null) {
      if (pcity['owner'] == client.conn.playing.playerno) {
	if (sunits != null && sunits.length > 0
            && sunits[0]['activity'] == ACTIVITY_IDLE) {
          set_unit_focus_and_redraw(sunits[0]);
          if (renderer == RENDERER_2DCANVAS) {
            $("#canvas").contextMenu();
          } else {
            $("#canvas_div").contextMenu();
          }
	} else if (!goto_active) {
          show_city_dialog(pcity);
	}
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
          update_active_units_dialog();
        }

        if (is_touch_device()) {
          if (renderer == RENDERER_2DCANVAS) {
            $("#canvas").contextMenu();
          } else {
            $("#canvas_div").contextMenu();
          }
	    }
      }
    }

  }

  paradrop_active = false;
  airlift_active = false;
  action_tgt_sel_active = false;
}

/**************************************************************************
 Callback to handle keyboard events
**************************************************************************/
function keyboard_listener(ev)
{
  // Check if focus is in chat field, where these keyboard events are ignored.
  if (!keyboard_input) return;

  if (C_S_RUNNING != client_state()) return;

  if (!ev) ev = window.event;
  var keyboard_key = String.fromCharCode(ev.keyCode);

  civclient_handle_key(keyboard_key, ev.keyCode, ev['ctrlKey'],  ev['altKey'], ev['shiftKey'], ev);

  if (renderer == RENDERER_2DCANVAS) $("#canvas").contextMenu('hide');

}

/**************************************************************************
 Handles everything when the user typed on the keyboard.
**************************************************************************/
function
civclient_handle_key(keyboard_key, key_code, ctrl, alt, shift, the_event)
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

    case 'H':
      key_unit_homecity();
    break;

    case 'X':
      key_unit_auto_explore();
    break;

    case 'A':
      key_unit_auto_settle();
    break;

    case 'L':
      if (shift) {
        key_unit_airlift();
      }
    break;

    case 'W':
      key_unit_wait();
    break;

    case 'J':
      key_unit_noorders();
    break;

    case 'R':
      key_unit_road();
    break;

    case 'E':
      if (shift) {
        key_unit_airbase();
      }
    break;

    case 'F':
      if (shift) {
        key_unit_fortress();
      } else {
        key_unit_fortify();
      }
    break;

    case 'I':
      key_unit_irrigate();
    break;

   case 'U':
      key_unit_upgrade();
    break;

    case 'S':
      if (ctrl) {
        the_event.preventDefault();
        quicksave();
      } else {
        key_unit_sentry();
      }
    break;
    case 'P':
      if (shift) {
        key_unit_pillage();
      } else {
        key_unit_pollution();
      }
    break;

    case 'M':
      key_unit_mine();
    break;

    case 'O':
      key_unit_transform();
    break;

    case 'C':
      if (ctrl) {
        show_citybar = !show_citybar;
      } else if (current_focus.length > 0) {
        auto_center_on_focus_unit();
      }

    break;


    case 'N':
      if (shift) {
        key_unit_nuke();
      } else {
        key_unit_fallout();
      }
    break;

    case 'Q':
      if (alt) civclient_benchmark(0);
    break;

    case 'D':
      if (shift) {
        key_unit_disband();
      } else if (alt || ctrl) {
        show_debug_info();
      } else {
        key_unit_action_select();
      }
    break;

  }

  switch (key_code) {
    case 13:
      if (shift) send_end_turn();
      break;

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

    case 27:
      deactivate_goto();

      /* Abort started multiple unit selection. */
      map_select_active = false;
      map_select_check = false;

      /* Abort any context menu blocking. */
      context_menu_active = true;
      $("#canvas").contextMenu(true);

      /* Abort target tile selection. */
      paradrop_active = false;
      airlift_active = false;
      action_tgt_sel_active = false;
      break;

    case 32: // space, will clear selection and goto.
      current_focus = [];
      if (renderer == RENDERER_WEBGL) webgl_clear_unit_focus();
      goto_active = false;
      $("#canvas_div").css("cursor", "default");
      goto_request_map = {};
      goto_turns_request_map = {};
      clear_goto_tiles();
      update_active_units_dialog();

      break;

    case 107:
      //zoom in
      if (renderer == RENDERER_WEBGL) {
        new_camera_dy = camera_dy - 60;
        new_camera_dx = camera_dx - 45;
        new_camera_dz = camera_dz - 45;
        if (new_camera_dy < 350 || new_camera_dy > 1200) {
          return;
        } else {
          camera_dx = new_camera_dx;
          camera_dy = new_camera_dy;
          camera_dz = new_camera_dz;
        }
        camera_look_at(camera_current_x, camera_current_y, camera_current_z);
      }
      break;

    case 109:
      //zoom out
      if (renderer == RENDERER_WEBGL) {
        new_camera_dy = camera_dy + 60;
        new_camera_dx = camera_dx + 45;
        new_camera_dz = camera_dz + 45;
        if (new_camera_dy < 350 || new_camera_dy > 1200) {
          return;
        } else {
          camera_dx = new_camera_dx;
          camera_dy = new_camera_dy;
          camera_dz = new_camera_dz;
        }
        camera_look_at(camera_current_x, camera_current_y, camera_current_z);
      }
      break;

  }

}

/**************************************************************************
 Handles everything when the user clicked on the context menu
**************************************************************************/
function handle_context_menu_callback(key)
{
  switch (key) {
    case "build":
      request_unit_build_city();
      break;

    case "tile_info":
      var ptile = find_a_focus_unit_tile_to_center_on();
      if (ptile != null) popit_req(ptile);
      break;

    case "goto":
      activate_goto();
      break;

    case "explore":
      key_unit_auto_explore();
      break;

    case "fortify":
      key_unit_fortify();
      break;

    case "road":
      key_unit_road();
      break;

    case "railroad":
      key_unit_road();
      break;

    case "mine":
      key_unit_mine();
      break;

    case "autosettlers":
      key_unit_auto_settle();
      break;

    case "fallout":
      key_unit_fallout();
      break;

    case "pollution":
      key_unit_pollution();
      break;

    case "forest":
      key_unit_irrigate();
      break;

    case "irrigation":
      key_unit_irrigate();
      break;

    case "fortress":
      key_unit_fortress();
      break;

    case "airbase":
      key_unit_airbase();
      break;

    case "transform":
      key_unit_transform();
      break;

    case "nuke":
      key_unit_nuke();
      break;

    case "paradrop":
      key_unit_paradrop();
      break;

    case "pillage":
      key_unit_pillage();
      break;

    case "homecity":
      key_unit_homecity();
      break;

    case "airlift":
      key_unit_airlift();
      break;

    case "sentry":
      key_unit_sentry();
      break;

    case "wait":
      key_unit_wait();
      break;

    case "noorders":
      key_unit_noorders();
      break;

    case "upgrade":
      key_unit_upgrade();
      break;

    case "disband":
      key_unit_disband();
      break;

    case "action_selection":
      key_unit_action_select();
      break;

    case "show_city":
      var stile = find_a_focus_unit_tile_to_center_on();
      if (stile != null) {
        show_city_dialog(tile_city(stile));
      }
      break;
  }
  if (key != "goto" && is_touch_device()) {
    deactivate_goto();
  }
}

/**************************************************************************
  Activate a regular goto.
**************************************************************************/
function activate_goto()
{
  clear_goto_tiles();
  activate_goto_last(ORDER_LAST, ACTION_COUNT);
}

/**************************************************************************
  Activate a goto and specify what to do once there.
**************************************************************************/
function activate_goto_last(last_order, last_action)
{
  goto_active = true;
  $("#canvas_div").css("cursor", "crosshair");

  /* Set what the unit should do on arrival. */
  goto_last_order = last_order;
  goto_last_action = last_action;

  if (current_focus.length > 0) {
    if (intro_click_description) {
      if (is_touch_device()) {
        add_chatbox_text("Carefully drag unit to the tile you want it to go to.");
      } else {
        add_chatbox_text("Click on the tile to send this unit to.");
      }
      intro_click_description = false;
    }

  } else {
    add_chatbox_text("First select a unit to move by clicking on it, then click on the"
		     + " goto button or the 'G' key, then click on the position to move to.");
    deactivate_goto();
  }

}

/**************************************************************************
 ...
**************************************************************************/
function deactivate_goto()
{
  goto_active = false;
  $("#canvas_div").css("cursor", "default");
  goto_request_map = {};
  goto_turns_request_map = {};
  clear_goto_tiles();

  /* Clear the order this action would have performed. */
  goto_last_order = ORDER_LAST;
  goto_last_action = ACTION_COUNT;

  // update focus to next unit after 600ms.
  setTimeout(update_unit_focus, 600);


}

/**************************************************************************
 Ends the current turn.
**************************************************************************/
function send_end_turn()
{
  $("#turn_done_button").button( "option", "disabled", true);
  if (!is_touch_device()) $("#turn_done_button").tooltip({ disabled: true });
  var packet = {"pid" : packet_player_phase_done, "turn" : game_info['turn']};
  send_request(JSON.stringify(packet));
  update_turn_change_timer();
}


/**************************************************************************
 Tell the units in focus to auto explore.
**************************************************************************/
function key_unit_auto_explore()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_EXPLORE, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the unit to wait (focus to next unit with moves left)
**************************************************************************/
function key_unit_wait()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    waiting_units_list.push(punit['id']);
  }
  advance_unit_focus();
}

/**************************************************************************
 Tell the unit to have no orders this turn, set unit to done moving.
**************************************************************************/
function key_unit_noorders()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    punit['done_moving'] = true;
  }

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
    request_new_unit_activity(punit, ACTIVITY_SENTRY, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to fortify.
**************************************************************************/
function key_unit_fortify()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_FORTIFYING, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to build base.
**************************************************************************/
function key_unit_fortress()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    /* EXTRA_NONE -> server decides */
    request_new_unit_activity(punit, ACTIVITY_BASE, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to build airbase.
**************************************************************************/
function key_unit_airbase()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_BASE, BASE_AIRBASE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to irrigate.
**************************************************************************/
function key_unit_irrigate()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    /* EXTRA_NONE -> server decides */
    request_new_unit_activity(punit, ACTIVITY_IRRIGATE, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units to remove pollution.
**************************************************************************/
function key_unit_pollution()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_POLLUTION, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}


/**************************************************************************
  Start a goto that will end in the unit(s) detonating in a nuclear
  exlosion.
**************************************************************************/
function key_unit_nuke()
{
  /* The last order of the goto is the nuclear detonation. */
  activate_goto_last(ORDER_PERFORM_ACTION, ACTION_NUKE);
}

/**************************************************************************
 Tell the units to upgrade.
**************************************************************************/
function key_unit_upgrade()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    var pcity = tile_city(index_to_tile(punit['tile']));
    var target_id = (pcity != null) ? pcity['id'] : 0;
    var packet = {
      "pid"         : packet_unit_do_action,
      "actor_id"    : punit['id'],
      "target_id"   : target_id,
      "value"       : 0,
      "name"        : "",
      "action_type" : ACTION_UPGRADE_UNIT
    };
    send_request(JSON.stringify(packet));
  }
  update_unit_focus();
}

/**************************************************************************
 Tell the units to paradrop.
**************************************************************************/
function key_unit_paradrop()
{
  paradrop_active = true;
  add_chatbox_text("Click on the tile to send this paratrooper to.");
}

/**************************************************************************
 Tell the units to airlift.
**************************************************************************/
function key_unit_airlift()
{
  airlift_active = true;
  add_chatbox_text("Click on the city to airlift this unit to.");
}

/**************************************************************************
 Tell the units to remove nuclear fallout.
**************************************************************************/
function key_unit_fallout()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_FALLOUT, EXTRA_NONE);
  }
  update_unit_focus();
}

/**************************************************************************
 Tell the units to transform the terrain.
**************************************************************************/
function key_unit_transform()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_TRANSFORM, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to pillage.
**************************************************************************/
function key_unit_pillage()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    request_new_unit_activity(punit, ACTIVITY_PILLAGE, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to mine.
**************************************************************************/
function key_unit_mine()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    /* EXTRA_NONE -> server decides */
    request_new_unit_activity(punit, ACTIVITY_MINE, EXTRA_NONE);
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Tell the units in focus to build road or railroad.
**************************************************************************/
function key_unit_road()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    var ptile = index_to_tile(punit['tile']);
    if (!tile_has_extra(ptile, ROAD_ROAD)) {
      request_new_unit_activity(punit, ACTIVITY_GEN_ROAD, extras['Road']['id']);
    } else if (!tile_has_extra(ptile, ROAD_RAIL)) {
      request_new_unit_activity(punit, ACTIVITY_GEN_ROAD, extras['Railroad']['id']);
    }
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Changes unit homecity to the city on same tile.
**************************************************************************/
function key_unit_homecity()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    var ptile = index_to_tile(punit['tile']);
    var pcity = tile_city(ptile);

    if (pcity != null) {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : punit['id'],
                    "target_id": pcity['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_HOME_CITY};
      send_request(JSON.stringify(packet));
      $("#order_change_homecity").hide();
    }

  }
}

/**************************************************************************
  Show action selection dialog for unit(s).
**************************************************************************/
function key_unit_action_select()
{
  if (action_tgt_sel_active == true) {
    /* The 2nd key press means that the actor should target its own
     * tile. */
    action_tgt_sel_active = false;

    /* Target tile selected. Clean up hover state. */
    request_unit_act_sel_vs_own_tile();
  } else {
    action_tgt_sel_active = true;
    add_chatbox_text("Click on a tile to act against it. "
                     + "Press 'd' again to act against own tile.");
  }
}

/**************************************************************************
  An action selection dialog for the selected units against the specified
  tile is wanted.
**************************************************************************/
function request_unit_act_sel_vs(ptile)
{
  var funits = get_units_in_focus();

  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    var packet = {
      "pid"     : packet_unit_sscs_set,
      "unit_id" : punit['id'],
      "type"    : USSDT_QUEUE,
      "value"   : ptile['index']
    };

    /* Have the server record that an action decision is wanted for this
     * unit. */
    send_request(JSON.stringify(packet));
  }
}

/**************************************************************************
  An action selection dialog for the selected units against its own tile.
**************************************************************************/
function request_unit_act_sel_vs_own_tile()
{
  var funits = get_units_in_focus();
  for (var i = 0; i < funits.length; i++) {
    var punit = funits[i];
    var packet = {
      "pid"     : packet_unit_sscs_set,
      "unit_id" : punit['id'],
      "type"    : USSDT_QUEUE,
      "value"   : punit['tile']
    };

    /* Have the server record that an action decision is wanted for this
     * unit. */
    send_request(JSON.stringify(packet));
  }
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
  setTimeout(update_unit_focus, 700);
}



/**************************************************************************
 ...
**************************************************************************/
function request_new_unit_activity(punit, activity, target)
{

  var packet = {"pid" : packet_unit_change_activity, "unit_id" : punit['id'],
                "activity" : activity, "target" : target };
  send_request(JSON.stringify(packet));
}


/****************************************************************************
  Call to request (from the server) that the settler unit is put into
  autosettler mode.
****************************************************************************/
function request_unit_autosettlers(punit)
{
  if (punit != null ) {
    var packet = {"pid" : packet_unit_autosettlers, "unit_id" : punit['id']};
    send_request(JSON.stringify(packet));
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
        var packet = null;
        var target_city = tile_city(index_to_tile(punit['tile']));

        /* Do Join City if located inside a city. */
        if (target_city == null) {
          packet = {"pid" : packet_city_name_suggestion_req,
            "unit_id"     : punit['id'] };
        } else {
          packet = {"pid" : packet_unit_do_action,
            "actor_id"    : punit['id'],
            "target_id"   : target_city['id'],
            "value"       : 0,
            "name"        : "",
            "action_type" : ACTION_JOIN_CITY };
        }

        send_request(JSON.stringify(packet));
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
    var packet = null;
    var target_city = tile_city(index_to_tile(punit['tile']));

    /* Do Recycle Unit if located inside a city. */
    /* FIXME: Only rulesets where the player can do Recycle Unit to all
     * domestic and allied cities are supported here. */
    packet = {
      "pid"         : packet_unit_do_action,
      "actor_id"    : punit['id'],
      "target_id"   : (target_city == null ? punit['id']
                                           : target_city['id']),
      "value"       : 0,
      "name"        : "",
      "action_type" : (target_city == null ? ACTION_DISBAND_UNIT
                                           : ACTION_RECYCLE_UNIT)
    };

    send_request(JSON.stringify(packet));
  }
  setTimeout(update_unit_focus, 700);
}

/**************************************************************************
 Moved the unit in focus in the specified direction.
**************************************************************************/
function key_unit_move(dir)
{
  if (current_focus.length > 0) {
    var punit = current_focus[0];
    if (punit == null) {
      return;
    }

    var ptile = index_to_tile(punit['tile']);
    if (ptile == null) {
      return;
    }

    var newtile = mapstep(ptile, dir);
    if (newtile == null) {
      return;
    }

    /* Send the order to move using the orders system. */
    var packet = {
      "pid"      : packet_unit_orders,
      "unit_id"  : punit['id'],
      "src_tile" : ptile['index'],
      "length"   : 1,
      "repeat"   : false,
      "vigilant" : false,
      "orders"   : [ORDER_ACTION_MOVE],
      "dir"      : [dir],
      "activity" : [ACTIVITY_LAST],
      "target"   : [EXTRA_NONE],
      "action"   : [ACTION_COUNT],
      "dest_tile": newtile['index']
    };

    send_request(JSON.stringify(packet));
    unit_move_sound_play(punit);
  }

  deactivate_goto();
}

/**************************************************************************
 ...
**************************************************************************/
function process_diplomat_arrival(pdiplomat, target_tile_id)
{
  var ptile = index_to_tile(target_tile_id);

  /* No queue. An action selection dialog is opened at once. If multiple
   * action selection dialogs are open at once one will hide all others.
   * The hidden dialogs are based on information from the time they
   * were opened. It is therefore more outdated than it would have been if
   * the server was asked the moment before the action selection dialog
   * was shown.
   *
   * The C client's bundled with Freeciv asks right before showing the
   * action selection dialog. They used to have a custom queue for it.
   * Freeciv patch #6601 (SVN r30682) made the desire for an action
   * decision a part of a unit's data. They used it to drop their custom
   * queue and move unit action decisions to the unit focus queue. */

  if (pdiplomat != null && ptile != null) {
    /* Ask the server about what actions pdiplomat can do. The server's
     * reply will pop up an action selection dialog for it. */
    var packet = {
      "pid" : packet_unit_get_actions,
      "actor_unit_id" : pdiplomat['id'],
      "target_unit_id" : IDENTITY_NUMBER_ZERO,
      "target_city_id" : IDENTITY_NUMBER_ZERO,
      "target_tile_id": target_tile_id,
      "disturb_player": true
    };
    send_request(JSON.stringify(packet));
  }
}

/****************************************************************************
  Request GOTO path for unit with unit_id, and dst_x, dst_y in map coords.
****************************************************************************/
function request_goto_path(unit_id, dst_x, dst_y)
{
  if (goto_request_map[unit_id + "," + dst_x + "," + dst_y] == null) {
    goto_request_map[unit_id + "," + dst_x + "," + dst_y] = true;

    var packet = {"pid" : packet_goto_path_req, "unit_id" : unit_id,
                  "goal" : map_pos_to_tile(dst_x, dst_y)['index']};
    send_request(JSON.stringify(packet));
    current_goto_turns = null;
    $("#unit_text_details").html("Choose unit goto");
    setTimeout(update_mouse_cursor, 700);
  } else {
    update_goto_path(goto_request_map[unit_id + "," + dst_x + "," + dst_y]);
  }
}

/****************************************************************************
...
****************************************************************************/
function check_request_goto_path()
{
  if (goto_active && current_focus.length > 0
      && prev_mouse_x == mouse_x && prev_mouse_y == mouse_y) {
    var ptile;
    clear_goto_tiles();
    if (renderer == RENDERER_2DCANVAS) {
      ptile = canvas_pos_to_tile(mouse_x, mouse_y);
    } else {
      ptile = webgl_canvas_pos_to_tile(mouse_x, mouse_y);
    }
    if (ptile != null) {
      /* Send request for goto_path to server. */
      for (var i = 0; i < current_focus.length; i++) {
        request_goto_path(current_focus[i]['id'], ptile['x'], ptile['y']);
      }
    }
  }
  prev_mouse_x = mouse_x;
  prev_mouse_y = mouse_y;

}

/****************************************************************************
  Show the GOTO path in the unit_goto_path packet.
****************************************************************************/
function update_goto_path(goto_packet)
{
  var punit = units[goto_packet['unit_id']];
  if (punit == null) return;
  var t0 = index_to_tile(punit['tile']);
  var ptile = t0;
  var goaltile = index_to_tile(goto_packet['dest']);

  if (renderer == RENDERER_2DCANVAS) {
    for (var i = 0; i < goto_packet['dir'].length; i++) {
      if (ptile == null) break;
      var dir = goto_packet['dir'][i];

      if (dir == -1) {
        /* Assume that this means refuel. */
        continue;
      }

      ptile['goto_dir'] = dir;
      ptile = mapstep(ptile, dir);
    }
  } else {
    webgl_render_goto_line(ptile, goto_packet['dir']);
  }

  current_goto_turns = goto_packet['turns'];

  goto_request_map[goto_packet['unit_id'] + "," + goaltile['x'] + "," + goaltile['y']] = goto_packet;
  goto_turns_request_map[goto_packet['unit_id'] + "," + goaltile['x'] + "," + goaltile['y']]
	  = current_goto_turns;

  if (current_goto_turns != undefined) {
    $("#active_unit_info").html("Turns for goto: " + current_goto_turns);
  }
  update_mouse_cursor();
}



/**************************************************************************
  Centers the mapview around the given tile..
**************************************************************************/
function center_tile_mapcanvas(ptile)
{
  if (renderer == RENDERER_2DCANVAS) {
    center_tile_mapcanvas_2d(ptile);
  } else {
    center_tile_mapcanvas_3d(ptile);
  }

}

/**************************************************************************
  Show tile information in a popup
**************************************************************************/
function popit()
{
  var ptile;

  if (renderer == RENDERER_2DCANVAS) {
    ptile = canvas_pos_to_tile(mouse_x, mouse_y);
  } else {
    ptile = webgl_canvas_pos_to_tile(mouse_x, mouse_y);
  }

  if (ptile == null) return;

  popit_req(ptile);

}

/**************************************************************************
  request tile popup
**************************************************************************/
function popit_req(ptile)
{
  if (ptile == null) return;
  var punit_id = 0;
  var punit = find_visible_unit(ptile);
  if (punit != null) punit_id = punit['id'];

  var focus_unit_id = 0;
  if (current_focus.length > 0) {
    focus_unit_id = current_focus[0]['id'];
  }

  var packet = {"pid" : packet_info_text_req, "visible_unit" : punit_id,
                "loc" : ptile['index'], "focus_unit": focus_unit_id};
  send_request(JSON.stringify(packet));
}