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



/* Freeciv Web Client.
   This file contains the handling-code for packets from the civserver.
*/

function handle_processing_started(packet)
{
  client_frozen = true;
}

function handle_processing_finished(packet)
{
  client_frozen = false;
}

function handle_freeze_hint(packet)
{
  client_frozen = true;
}

function handle_thaw_hint(packet)
{
  client_frozen = false;
}

/* 100% */
function handle_ruleset_terrain(packet)
{
  /* FIXME: These two hacks are there since Freeciv-web doesn't support rendering Lake and Glacier correctly. */
  if (packet['name'] == "Lake") packet['graphic_str'] = packet['graphic_alt'];
  if (packet['name'] == "Glacier") packet['graphic_str'] = "tundra";
  terrains[packet['id']] = packet;
}

/****************************************************************************
  After we send a join packet to the server we receive a reply.  This
  function handles the reply.  100% Complete.
****************************************************************************/
function handle_server_join_reply(packet)
{
  if (packet['you_can_join']) {
    var client_info;

    client.conn.established = true;
    client.conn.id = packet['conn_id'];

    if (get_client_page() == PAGE_MAIN
	|| get_client_page() == PAGE_NETWORK
	|| get_client_page() == PAGE_GGZ) {
      set_client_page(PAGE_START);
    }

    client_info = {
      "pid"          : packet_client_info,
      "gui"          : GUI_WEB,
      "distribution" : ""
    };
    send_request(JSON.stringify(client_info));

    set_client_state(C_S_PREPARING);

    if ($.getUrlVar('action') == "new"
        && $.getUrlVar('ruleset') != null) {
      change_ruleset($.getUrlVar('ruleset'));
    }

    if (renderer == RENDERER_WEBGL && !observing) {
       if (graphics_quality == QUALITY_LOW) {
         // WebGL renderer on mobile devices needs to use very little RAM.
         send_message_delayed("/set size 1", 130);
       }
       // Reduce the amount of rivers, it's kind of ugly at the moment.
       send_message_delayed("/set wetness 25", 140);

       // Freeciv-web WebGL doesn't support rendering borders.
       send_message_delayed("/set borders disabled", 150);

       // Less hills will be more user-friendly in 3D mode.
       send_message_delayed("/set steepness 12", 155);

     }

    if (autostart) {
      if (renderer == RENDERER_WEBGL) {
        $.blockUI({ message: '<h2>Generating terrain map model...</h2>' });
      }
      if (loadTimerId == -1) {
        setTimeout(pregame_start_game, 600);
      } else {
        setTimeout(pregame_start_game, 3800);
      }
    } else if (observing) {
      setTimeout(request_observe_game, 800);
    }

  } else {

    swal("You were rejected from the game.");
    client.conn.id = -1;/* not in range of conn_info id */
    set_client_page(PAGE_MAIN);

  }

}

/**************************************************************************
  Remove, add, or update dummy connection struct representing some
  connection to the server, with info from packet_conn_info.
  Updates player and game connection lists.
  Calls update_players_dialog() in case info for that has changed.
  99% done.
**************************************************************************/
function handle_conn_info(packet)
{
  var pconn = find_conn_by_id(packet['id']);


  if (packet['used'] == false) {
    /* Forget the connection */
    if (pconn == null) {
      freelog(LOG_VERBOSE, "Server removed unknown connection " + packet['id']);
      return;
    }
    client_remove_cli_conn(pconn);
    pconn = null;
  } else {
    var pplayer = valid_player_by_number(packet['player_num']);

    packet['playing'] = pplayer;

    if (packet['id'] == client.conn.id) {
      client.conn = packet;
    }

    conn_list_append(packet);

  }

  if (packet['id'] == client.conn.id) {
    if (client.conn.playing != packet['playing']) {
      set_client_state(C_S_PREPARING);
    }
  }

  /* FIXME: not implemented yet.
  update_players_dialog();
  update_conn_list_dialog();*/

}

/* 100% done */
function handle_ruleset_resource(packet)
{
  resources[packet['id']] = packet;
}

/**************************************************************************
 100% complete.
**************************************************************************/
function handle_tile_info(packet)
{
  if (tiles != null) {
    packet['extras'] = new BitVector(packet['extras']);

    if (renderer == RENDERER_WEBGL) {
      var old_tile = $.extend({}, tiles[packet['tile']]);
      webgl_update_tile_known(tiles[packet['tile']], packet);
      update_tile_extras($.extend(old_tile, packet));
    }

    tiles[packet['tile']] = $.extend(tiles[packet['tile']], packet);
  }
}

/* 100% complete */
function handle_chat_msg(packet)
{
  var message = packet['message'];
  var conn_id = packet['conn_id'];
  var event = packet['event'];
  var ptile = packet['tile'];

  if (connections[conn_id] != null) {
    var username = connections[conn_id]['username'];
    add_chatbox_text("<b>" + username + ":</b>" + message);
  } else {
    if (packet['event'] == 45) {
      var regxp = /\n/gi;
      message = message.replace(regxp, "<br>\n");
      show_dialog_message("Message for you:", message);
    } else {
      if (ptile != null && ptile > 0) {
        add_chatbox_text("<span class='chatbox_text_tileinfo' "
             + "onclick='center_tile_id(" + ptile + ");'>" + message + "</span>");
      } else {
        add_chatbox_text(message);
      }

      if (is_speech_supported()) speak(message);

    }
  }
}

/**************************************************************************
  Handle an early message packet. Thease have format like other chat
  messages but server sends them only about events related to establishing
  the connection and other setup in the early phase. They are a separate
  packet just so that client knows thse to be already relevant when it's
  only setting itself up - other chat messages might be just something
  sent to all clients, and we might want to still consider ourselves
  "not connected" (not receivers of those messages) until we are fully
  in the game.
**************************************************************************/
function handle_early_chat_msg(packet)
{
  /* Handle as a regular chat message for now. */
  handle_chat_msg(packet);
}

/* 100% complete */
function handle_city_info(packet)
{
  /* Decode the city name. */
  packet['name'] = decodeURIComponent(packet['name']);

  /* Decode bit vectors. */
  packet['improvements'] = new BitVector(packet['improvements']);
  packet['city_options'] = new BitVector(packet['city_options']);

  if (cities[packet['id']] == null) {
    cities[packet['id']] = packet;
  } else {
    cities[packet['id']] = $.extend(cities[packet['id']], packet);
  }

  /* manually update tile relation.*/
  if (tiles[packet['tile']] != null) {
    tiles[packet['tile']]['worked'] = packet['id'];
  }

  if (active_city != null) {
    show_city_dialog(active_city);
  }

  if (packet['diplomat_investigate']) {
    show_city_dialog(cities[packet['id']]);
  }

  if (worklist_dialog_active && active_city != null) {
    city_worklist_dialog(active_city);
  }

  if (renderer == RENDERER_WEBGL) {
    update_city_position(index_to_tile(packet['tile']));
  }

}

/* 99% complete
   TODO: does this loose information? */
function handle_city_short_info(packet)
{
  /* Decode the city name. */
  packet['name'] = decodeURIComponent(packet['name']);

  /* Decode bit vectors. */
  packet['improvements'] = new BitVector(packet['improvements']);

  if (cities[packet['id']] == null) {
    cities[packet['id']] = packet;
  } else {
    cities[packet['id']] = $.extend(cities[packet['id']], packet);
  }

  if (renderer == RENDERER_WEBGL) {
    update_city_position(index_to_tile(packet['tile']));
  }
}

/**************************************************************************
  A traderoute-info packet contains information about one end of a
  traderoute
**************************************************************************/
function handle_traderoute_info(packet)
{
  if (city_trade_routes[packet['city']] == null) {
    /* This is the first trade route received for this city. */
    city_trade_routes[packet['city']] = {};
  }

  city_trade_routes[packet['city']][packet['index']] = packet;
}

/* 100% complete */
function handle_player_info(packet)
{
  /* Interpret player flags. */
  packet['flags'] = new BitVector(packet['flags']);

  players[packet['playerno']] = $.extend(players[packet['playerno']], packet);

  if (client.conn.playing != null) {
    if (packet['playerno'] == client.conn.playing['playerno']) {
      client.conn.playing = players[packet['playerno']];
      update_game_status_panel();
      update_net_income();
    }
  }
  update_player_info_pregame();

  if (is_tech_tree_init && tech_dialog_active) update_tech_screen();
}

/* 100% complete */
function handle_player_remove(packet)
{
  delete players[packet['playerno']];
  update_player_info_pregame();

}

/* 100% complete */
function handle_conn_ping(packet)
{
  ping_last = new Date().getTime();
  var pong_packet = {"pid" : packet_conn_pong};
  send_request(JSON.stringify(pong_packet));

}

/**************************************************************************
  Server requested topology change.

  0% complete.
**************************************************************************/
function handle_set_topology(packet)
{
  /* TODO */
}

/* 50% complete */
function handle_map_info(packet)
{
  map = packet;

  map_init_topology(false);

  map_allocate();

  /* TODO: init_client_goto();*/

  mapdeco_init();

  /* TODO: generate_citydlg_dimensions();*/

  /* TODO: calculate_overview_dimensions();*/

  /*packhand_init();*/

}

/* 100% complete */
function handle_game_info(packet)
{
  game_info = packet;
}

/**************************************************************************
  Handle the calendar info packet.
**************************************************************************/
function handle_calendar_info(packet)
{
  calendar_info = packet;
}

/* 30% complete */
function handle_start_phase(packet)
{
  update_client_state(C_S_RUNNING);

  set_phase_start();

  saved_this_turn = false;

  /* TODO: implement rest*/

}

/**************************************************************************
  Handle the ruleset control packet.

  This is the first ruleset packet the server sends.
**************************************************************************/
function handle_ruleset_control(packet)
{
  ruleset_control = packet;

  update_client_state(C_S_PREPARING);

  /* Clear out any effects belonging to the previous ruleset. */
  effects = {};

  /* Clear out the description of the previous ruleset. */
  ruleset_summary = null;
  ruleset_description = null;

  /* TODO: implement rest*/

}

/**************************************************************************
  Ruleset summary.
**************************************************************************/
function handle_ruleset_summary(packet)
{
  ruleset_summary = packet['text'];
}

/**************************************************************************
  Receive next part of the ruleset description.
**************************************************************************/
function handle_ruleset_description_part(packet)
{
  if (ruleset_description == null) {
    ruleset_description = packet['text'];
  } else {
    ruleset_description += packet['text'];
  }
}

function handle_endgame_report(packet)
{

  update_client_state(C_S_OVER);

  /* TODO: implement rest*/

}

function update_client_state(value)
{
  set_client_state(value);
}

function handle_authentication_req(packet)
{
  show_auth_dialog(packet);
}

function handle_server_shutdown(packet)
{
  /* TODO: implement*/
}

function handle_nuke_tile_info(packet)
{
  var ptile = index_to_tile(packet['tile']);
  ptile['nuke'] = 60;
}

/* done */
function handle_city_remove(packet)
{
  remove_city(packet['city_id']);
}


function handle_connect_msg(packet)
{
  var message = packet['message'];
  add_chatbox_text(message);
}


/* done */
function handle_city_name_suggestion_info(packet)
{
  /* Decode the city name. */
  packet['name'] = decodeURIComponent(packet['name']);

  city_name_dialog(packet['name'], packet['unit_id']);
}

/**************************************************************************
  Handle the response the a request asking what buildings a potential
  victim of targeted sabotage city victim.
**************************************************************************/
function handle_city_sabotage_list(packet)
{
  popup_sabotage_dialog(game_find_unit_by_number(packet['diplomat_id']),
                        game_find_city_by_number(packet['city_id']),
                        new BitVector(packet['improvements']));
}

function handle_player_attribute_chunk(packet)
{
  /* The attribute block of the player structure is an area of Freeciv
   * server memory that the client controls. The server will store it to
   * savegames, send it when the client requests a copy and change it on
   * the client's request. The server has no idea about its content. This
   * is a chunk of it.
   *
   * The C clients can use the attribute block to store key-value pair
   * attributes for the object types city, player, tile and unit. The
   * format the they use to encode this data can be found in Freeciv's
   * client/attribute.c.
   *
   * The C clients uses it to store parameters of cities for the (client
   * side) CMA agent. */

  /* TODO: Find out if putting something inside savegames is needed. If it
   * is: decide if compatibility with the format of the Freeciv C clients
   * is needed and implement the result of the decision. */
}

/**************************************************************************
  Handle a remove-unit packet, sent by the server to tell us any time a
  unit is no longer there.                             99% complete.
**************************************************************************/
function handle_unit_remove(packet)
{
  var punit = game_find_unit_by_number(packet['unit_id']);
  var powner;

  if (punit == null) {
    return;
  }

  var funits = get_units_in_focus();
  if (funits != null && funits.length == 1 && funits[0]['id'] == packet['unit_id']) {
    /* if the unit in focus is removed, then advance the unit focus. */
    advance_unit_focus();
  }

  /* TODO: Close diplomat dialog if the diplomat is lost */
  /* TODO: Notify agents. */

  clear_tile_unit(punit);
  client_remove_unit(punit);

  if (renderer == RENDERER_WEBGL) {
    update_unit_position(index_to_tile(punit['tile']));
  }

}

/* 100% complete */
function handle_unit_info(packet)
{
  handle_unit_packet_common(packet);

}

/* 99% complete FIXME: does this loose information? */
function handle_unit_short_info(packet)
{
  handle_unit_packet_common(packet);
}

/**************************************************************************
  Called to do basic handling for a unit_info or short_unit_info packet.

  Both owned and foreign units are handled; you may need to check unit
  owner, or if unit equals focus unit, depending on what you are doing.

  Note: Normally the server informs client about a new "activity" here.
  For owned units, the new activity can be a result of:
  - The player issued a command (a request) with the client.
  - The server side AI did something.
  - An enemy encounter caused a sentry to idle. (See "Wakeup Focus").

  Depending on what caused the change, different actions may be taken.
  Therefore, this function is a bit of a jungle, and it is advisable
  to read thoroughly before changing.

  Exception: When the client puts a unit in focus, it's status is set to
  idle immediately, before informing the server about the new status. This
  is because the server can never deny a request for idle, and should not
  be concerned about which unit the client is focusing on.
**************************************************************************/
function handle_unit_packet_common(packet_unit)
{
  var pcity;
  var punit;
  var need_update_menus = true;
  var repaint_unit = true;
  var repaint_city = true;
  var old_tile = true;
  var check_focus = true;
  var moved = true;
  var ret = true;

  punit = player_find_unit_by_id(unit_owner(packet_unit), packet_unit['id']);

  clear_tile_unit(punit);

  if (punit == null && game_find_unit_by_number(packet_unit['id'])) {
    /* This means unit has changed owner. We deal with this here
     * by simply deleting the old one and creating a new one. */
    handle_unit_remove(packet_unit['id']);
  }
  var old_tile = null;
  if (punit != null) old_tile = index_to_tile(punit['tile']);

  if (units[packet_unit['id']] == null) {
    /* This is a new unit. */
    unit_actor_wants_input(packet_unit);
    packet_unit['anim_list'] = [];
    units[packet_unit['id']] = packet_unit;
    units[packet_unit['id']]['facing'] = 6;
  } else {
    if (units[packet_unit['id']]['action_decision_want']
        != packet_unit['action_decision_want']) {
      /* The unit's action_decision_want has changed. */
      unit_actor_wants_input(packet_unit);
    }

    update_unit_anim_list(units[packet_unit['id']], packet_unit);
    units[packet_unit['id']] = $.extend(units[packet_unit['id']], packet_unit);
  }

  update_tile_unit(units[packet_unit['id']]);

  if (current_focus.length > 0 && current_focus[0]['id'] == packet_unit['id']) {
    update_active_units_dialog();
    update_unit_order_commands();

    if (current_focus[0]['done_moving'] != packet_unit['done_moving']) {
      update_unit_focus();
    }
  }

  if (renderer == RENDERER_WEBGL) {
    if (punit != null) update_unit_position(old_tile);
    update_unit_position(index_to_tile(units[packet_unit['id']]['tile']));
  }

  /* TODO: update various dialogs and mapview. */
}

function handle_unit_combat_info(packet)
{
  var attacker = units[packet['attacker_unit_id']];
  var defender = units[packet['defender_unit_id']];
  var attacker_hp = packet['attacker_hp'];
  var defender_hp = packet['defender_hp'];

  if (attacker_hp == 0 && is_unit_visible(attacker)) {
    explosion_anim_map[attacker['tile']] = 25;
  }
  if (defender_hp == 0 && is_unit_visible(defender)) {
    explosion_anim_map[defender['tile']] = 25;
  }
}

/**************************************************************************
  Handle the requested follow up question about an action
**************************************************************************/
function handle_unit_action_answer(packet)
{
  var diplomat_id = packet['diplomat_id'];
  var target_id = packet['target_id'];
  var cost = packet['cost'];
  var action_type = packet['action_type'];

  var target_city = game_find_city_by_number(target_id);
  var target_unit = game_find_unit_by_number(target_id);
  var actor_unit = game_find_unit_by_number(diplomat_id);

  if (actor_unit == null) {
    console.log("Bad actor unit (" + diplomat_id
                + ") in unit action answer.");
    return;
  }

  if (action_type == ACTION_SPY_BRIBE_UNIT) {
    if (target_unit == null) {
      console.log("Bad target unit (" + target_id
                  + ") in unit action answer.");
      return;
    } else {
      popup_bribe_dialog(actor_unit, target_unit, cost);
      return;
    }
  } else if (action_type == ACTION_SPY_INCITE_CITY) {
    if (target_city == null) {
      console.log("Bad target city (" + target_id
                  + ") in unit action answer.");
      return;
    } else {
      popup_incite_dialog(actor_unit, target_city, cost);
      return;
    }
  } else if (action_type == ACTION_COUNT) {
    console.log("unit_action_answer: Server refused to respond.");
  } else {
    console.log("unit_action_answer: Invalid answer.");
  }
}

/**************************************************************************
  Handle server request for user input about diplomat action to do.
**************************************************************************/
function unit_actor_wants_input(pdiplomat)
{
  if (observing || client.conn.playing == null) return;

  if (pdiplomat['action_decision_want'] == null
      || pdiplomat['owner'] != client.conn.playing['playerno']) {
    /* No authority to decide for this unit. */
    return;
  }

  if (pdiplomat['action_decision_want'] == ACT_DEC_NOTHING) {
    /* The unit doesn't want a decision. */
    return;
  }

  if (pdiplomat['action_decision_want'] == ACT_DEC_PASSIVE
      && !popup_actor_arrival) {
    /* The player isn't interested in getting a pop up for a mere
     * arrival. */
    return;
  }

  process_diplomat_arrival(pdiplomat, pdiplomat['action_decision_tile']);
}

/**************************************************************************
  Handle server reply about what actions an unit can do.
**************************************************************************/
function handle_unit_actions(packet)
{
  var actor_unit_id = packet['actor_unit_id'];
  var target_unit_id = packet['target_unit_id'];
  var target_city_id = packet['target_city_id'];
  var target_tile_id = packet['target_tile_id'];
  var action_probabilities = packet['action_probabilities'];
  var disturb_player = packet['disturb_player'];

  var pdiplomat = game_find_unit_by_number(actor_unit_id);
  var target_unit = game_find_unit_by_number(target_unit_id);
  var target_city = game_find_city_by_number(target_city_id);
  var ptile = index_to_tile(target_tile_id);

  var hasActions = false;

  /* The dead can't act. */
  if (pdiplomat != null && ptile != null) {
    action_probabilities.forEach(function(prob) {
      if (action_prob_possible(prob)) {
        hasActions = true;
      }
    });
  }

  if (disturb_player) {
    /* Clear the unit's action_decision_want. This was the reply to a
     * foreground request caused by it. Freeciv-web doesn't save open
     * action selection dialogs. It doesn't even wait for any other action
     * selection dialog to be answered before requesting data for the next
     * one. This lack of a queue allows it to be cleared here. */

    var unqueue = {
      "pid"     : packet_unit_sscs_set,
      "unit_id" : actor_unit_id,
      "type"    : USSDT_UNQUEUE,
      "value"   : IDENTITY_NUMBER_ZERO
    };
    send_request(JSON.stringify(unqueue));
  }

  if (hasActions && disturb_player) {
    popup_action_selection(pdiplomat, action_probabilities,
                           ptile, target_unit, target_city);
  } else if (hasActions) {
    /* This was a background request. */

    /* No background requests are currently made. */
    console.log("Received the reply to a background request I didn't do.");
  }
}

function handle_diplomacy_init_meeting(packet)
{
  // for hotseat games, only activate diplomacy if the player is playing.
  if (is_hotseat() && packet['initiated_from'] != client.conn.playing['playerno']) return;   

  if (diplomacy_request_queue.indexOf(packet['counterpart']) < 0) {
    diplomacy_request_queue.push(packet['counterpart']);
  }
  diplomacy_clause_map[packet['counterpart']] = [];
  refresh_diplomacy_request_queue();

}

function handle_diplomacy_cancel_meeting(packet)
{
  cancel_meeting(packet['counterpart']);
}

function handle_diplomacy_create_clause(packet)
{
  if(diplomacy_clause_map[packet['counterpart']] == null) {
    diplomacy_clause_map[packet['counterpart']] = [];
  }
  diplomacy_clause_map[packet['counterpart']].push(packet);
  show_diplomacy_clauses();
}

function handle_diplomacy_remove_clause(packet)
{
  remove_clause(packet);
}

function handle_diplomacy_accept_treaty(packet)
{
  accept_treaty(packet['counterpart'],
		packet['I_accepted'],
		packet['other_accepted']);
}

/* Assemble incoming page_msg here. */
var page_msg = {};

/**************************************************************************
  Page_msg header handler.
**************************************************************************/
function handle_page_msg(packet)
{
  /* Message information */
  page_msg['headline'] = packet['headline'];
  page_msg['caption'] = packet['caption'];
  page_msg['event'] = packet['event'];

  /* How many fragments to expect. */
  page_msg['missing_parts'] = packet['parts'];

  /* Will come in follow up packets. */
  page_msg['message'] = "";
}

/**************************************************************************
  Page_msg part handler.
**************************************************************************/
function handle_page_msg_part(packet)
{
  /* Add the new parts of the message content. */
  page_msg['message'] = page_msg['message'] + packet['lines'];

  /* Register that it was received. */
  page_msg['missing_parts']--;

  if (page_msg['missing_parts'] == 0) {
    /* This was the last part. */

    var regxp = /\n/gi;

    page_msg['message'] = page_msg['message'].replace(regxp, "<br>\n");
    show_dialog_message(page_msg['headline'], page_msg['message']);

    /* Clear the message. */
    page_msg = {};
  }
}

function handle_conn_ping_info(packet)
{
  if (debug_active) {
    conn_ping_info = packet;
    debug_ping_list.push(packet['ping_time'][0] * 1000);
  }
}

function handle_end_phase(packet)
{
  chatbox_clip_messages();
  if (is_pbem()) {
    pbem_end_phase();
  }
  if (is_hotseat())  {
    hotseat_next_player();
  }
}

/* Done. */
function handle_new_year(packet)
{
  game_info['year'] = packet['year'];
  /* TODO: Support calender fragments. */
  game_info['fragments'] = packet['fragments'];
  game_info['turn'] = packet['turn'];
}

function handle_begin_turn(packet)
{
  if (!observing) {
    $("#turn_done_button").button("option", "disabled", false);
    if (is_small_screen()) {
      $("#turn_done_button").button("option", "label", "Turn Done");
    } else {
      $("#turn_done_button").button("option", "label", "<i class='fa fa-check-circle-o' style='color: green;'aria-hidden='true'></i> Turn Done");
    }
  }
  waiting_units_list = [];
  update_unit_focus();
  update_active_units_dialog();
  update_game_status_panel();

  var funits = get_units_in_focus();
  if (funits != null && funits.length == 0) {
    /* auto-center if there is no unit in focus. */
    auto_center_on_focus_unit();
  }

  if (is_tech_tree_init && tech_dialog_active) update_tech_screen();
}

function handle_end_turn(packet)
{
  reset_unit_anim_list();
  if (!observing) {
    $("#turn_done_button").button( "option", "disabled", true);
  }
}

function handle_freeze_client(packet)
{
  client_frozen = true;
}

function handle_thaw_client(packet)
{
  client_frozen = false;
}

function handle_spaceship_info(packet)
{
  spaceship_info[packet['player_num']] = packet;
}

/* 100% complete */
function handle_ruleset_unit(packet)
{
  if (packet['name'] != null && packet['name'].indexOf('?unit:') == 0)
    packet['name'] = packet['name'].replace('?unit:', '');

  unit_types[packet['id']] = packet;
}

/* 100% complete */
function handle_ruleset_game(packet)
{
  game_rules = packet;
}

/* 100% complete */
function handle_ruleset_specialist(packet)
{
  specialists[packet['id']] = packet;
}

function handle_ruleset_government_ruler_title(packet)
{
  /* TODO: implement*/
}

/**************************************************************************
  Recreate the old req[] field of ruleset_tech packets.

  This makes it possible to delay research_reqs support.
**************************************************************************/
function recreate_old_tech_req(packet)
{
  var i;

  /* Recreate the field it self. */
  packet['req'] = [];

  /* Add all techs in research_reqs. */
  for (i = 0; i < packet['research_reqs'].length; i++) {
    var requirement = packet['research_reqs'][i];

    if (requirement.kind == VUT_ADVANCE
        && requirement.range == REQ_RANGE_PLAYER
        && requirement.present) {
      packet['req'].push(requirement.value);
    }
  }

  /* Fill in A_NONE just in case Freeciv-web assumes its size is 2. */
  while (packet['req'].length < 2) {
    packet['req'].push(A_NONE);
  }
}

/* 100% complete */
function handle_ruleset_tech(packet)
{
  packet['name'] = packet['name'].replace("?tech:", "");
  techs[packet['id']] = packet;

  recreate_old_tech_req(packet);
}

/**************************************************************************
  Packet ruleset_tech_class handler.
**************************************************************************/
function handle_ruleset_tech_class(packet)
{
  /* TODO: implement*/
}

function handle_ruleset_tech_flag(packet)
{
  /* TODO: implement*/
}

/* 100% complete */
function handle_ruleset_government(packet)
{
  governments[packet['id']] = packet;
}

/* 100% complete */
function handle_ruleset_terrain_control(packet)
{
  terrain_control = packet;

  /* Separate since it is easier understand what SINGLE_MOVE means than to
   * understand what terrain_control['move_fragments'] means. */
  SINGLE_MOVE = terrain_control['move_fragments'];
}

/* 100% complete */
function handle_ruleset_nation_groups(packet)
{
  nation_groups = packet['groups'];
}

/* 100% complete */
function handle_ruleset_nation(packet)
{
  nations[packet['id']] = packet;
  assign_nation_color(packet['id']);
}

function handle_ruleset_city(packet)
{
  city_rules[packet['style_id']] = packet;
}

/* 100% complete */
function handle_ruleset_building(packet)
{
  improvements[packet['id']] = packet;
}

function handle_ruleset_unit_class(packet)
{
  /* TODO: implement */
}

function handle_ruleset_disaster(packet)
{
  /* TODO: implement */
}

function handle_ruleset_trade(packet)
{
  /* TODO: implement */
}

function handle_rulesets_ready(packet)
{
  /* Nothing to do */
}

function handle_single_want_hack_reply(packet)
{
  /* TODO: implement */
}

function handle_ruleset_choices(packet)
{
  /* TODO: implement */
}

function handle_game_load(packet)
{
  /* TODO: implement */
}

 /* Done */
function handle_ruleset_effect(packet)
{
  if (effects[packet['effect_type']] == null) {
    /* This is the first effect of this type. */
    effects[packet['effect_type']] = [];
  }

  effects[packet['effect_type']].push(packet);
}

function handle_ruleset_unit_flag(packet)
{
  /* TODO: implement */
}

/***************************************************************************
  Packet ruleset_unit_class_flag handler.
***************************************************************************/
function handle_ruleset_unit_class_flag(packet)
{
  /* TODO: implement */
}

function handle_ruleset_unit_bonus(packet)
{
  /* TODO: implement */
}

function handle_ruleset_terrain_flag(packet)
{
  /* TODO: implement */
}

/**************************************************************************
  Receive scenario information about the current game.

  The current game is a scenario game if scenario_info's 'is_scenario'
  field is set to true.
**************************************************************************/
function handle_scenario_info(packet)
{
  scenario_info = packet;

  /* Don't call update_game_info_pregame() yet. Wait for the scenario
   * description. */
}

/**************************************************************************
  Receive scenario description of the current scenario.
**************************************************************************/
function handle_scenario_description(packet)
{
  scenario_info['description'] = packet['description'];

  /* Show the updated game information. */
  update_game_info_pregame();
}

function handle_vote_new(packet)
{
  /* TODO: implement*/
}

function handle_vote_update(packet)
{
  /* TODO: implement*/
}

function handle_vote_remove(packet)
{
  /* TODO: implement*/
}

function handle_vote_resolve(packet)
{
  /* TODO: implement*/
}

function handle_edit_object_created(packet)
{
  /* edit not supported. */
}

function handle_goto_path(packet)
{
  if (goto_active) {
    update_goto_path(packet);
  }
}

/**************************************************************************
  Receive general information about a server setting.

  Setting data type specific information comes in a follow up packet.
**************************************************************************/
function handle_server_setting_const(packet)
{
  /* The data type specific follow up packets needs to look up a setting by
   * its id. */
  server_settings[packet['id']] = packet;

  /* Make it possible to look up a setting by its name. */
  server_settings[packet['name']] = packet;
}

/**************************************************************************
  Receive general information about a server setting.

  This is a follow up packet with data type specific information.
**************************************************************************/
function handle_server_setting_int(packet)
{
  $.extend(server_settings[packet['id']], packet);
}

/**************************************************************************
  Receive general information about a server setting.

  This is a follow up packet with data type specific information.
**************************************************************************/
function handle_server_setting_enum(packet)
{
  $.extend(server_settings[packet['id']], packet);
}

/**************************************************************************
  Receive general information about a server setting.

  This is a follow up packet with data type specific information.
**************************************************************************/
function handle_server_setting_bitwise(packet)
{
  $.extend(server_settings[packet['id']], packet);
}

/**************************************************************************
  Receive general information about a server setting.

  This is a follow up packet with data type specific information.
**************************************************************************/
function handle_server_setting_bool(packet)
{
  $.extend(server_settings[packet['id']], packet);
}

/**************************************************************************
  Receive general information about a server setting.

  This is a follow up packet with data type specific information.
**************************************************************************/
function handle_server_setting_str(packet)
{
  $.extend(server_settings[packet['id']], packet);
}

function handle_server_setting_control(packet)
{
  /* TODO: implement */
}

function handle_player_diplstate(packet)
{
  if (client == null || client.conn.playing == null) return;

  if (packet['type'] == DS_WAR && packet['plr2'] == client.conn.playing['playerno']
      && diplstates[packet['plr1']] != DS_WAR && diplstates[packet['plr1']] != DS_NO_CONTACT) {
     alert_war(packet['plr1']);
  } else if (packet['type'] == DS_WAR && packet['plr1'] == client.conn.playing['playerno']
      && diplstates[packet['plr2']] != DS_WAR && diplstates[packet['plr2']] != DS_NO_CONTACT)  {
     alert_war(packet['plr2']);
  }

  if (packet['plr1'] == client.conn.playing['playerno']) {
    diplstates[packet['plr2']] = packet['type'];
  } else if (packet['plr2'] == client.conn.playing['playerno']) {
    diplstates[packet['plr1']] = packet['type'];
  }
}

function handle_ruleset_extra(packet)
{
  extras[packet['id']] = packet;
  extras[packet['name']] = packet;
}

/**************************************************************************
  Packet ruleset_extra_flag handler.
**************************************************************************/
function handle_ruleset_extra_flag(packet)
{
  /* TODO: implement */
}

function handle_ruleset_base(packet)
{
  /* TODO: Implement */
}

function handle_ruleset_road(packet)
{
  /* TODO: Implement */
}

function handle_ruleset_action_enabler(packet)
{
  /* TODO: Implement */
}

function handle_ruleset_nation_sets(packet)
{
  /* TODO: Implement */
}

function handle_ruleset_style(packet)
{
  /* TODO: Implement */
}

function handle_nation_availability(packet)
{
  /* TODO: Implement */
}

function handle_ruleset_music(packet)
{
  /* TODO: Implement */
}

function handle_endgame_player(packet)
{
  endgame_player_info.push(packet);
}

function handle_research_info(packet)
{
  var old_inventions = null;
  if (research_data[packet['id']] != null) old_inventions = research_data[packet['id']]['inventions'];

  research_data[packet['id']] = packet;

  if (game_info['team_pooled_research']) {
    for (var player_id in players) {
      var pplayer = players[player_id];
      if (pplayer['team'] == packet['id']) {
	pplayer = $.extend(pplayer, packet);
        delete pplayer['id'];
      }
    }
  } else {
    var pplayer = players[packet['id']];
    pplayer = $.extend(pplayer, packet);
    delete pplayer['id'];
  }

  if (!client_is_observer() && old_inventions != null && client.conn.playing != null && client.conn.playing['playerno'] == packet['id']) {
    for (var i = 0; i < packet['inventions'].length; i++) {
      if (packet['inventions'][i] != old_inventions[i] && packet['inventions'][i] == TECH_KNOWN) {
        queue_tech_gained_dialog(i);
	break;
      }
    }
  }

  if (is_tech_tree_init && tech_dialog_active) update_tech_screen();
}

function handle_worker_task(packet)
{
  /* TODO: Implement */
}

function handle_timeout_info(packet)
{
  last_turn_change_time = Math.ceil(packet['last_turn_change_time']);
}

function handle_play_music(packet)
{
  /* TODO: Implement */
}

/* Receive a generalized action. */
function handle_ruleset_action(packet)
{
  actions[packet['id']] = packet;
}

/**************************************************************************
  Handle an action auto performer rule.
**************************************************************************/
function handle_ruleset_action_auto(packet)
{
  /* Not stored. The web client doesn't use this rule knowledge (yet). */
}

function handle_ruleset_goods(packet)
{
  goods[packet['id']] = packet;
}

function handle_ruleset_achievement(packet)
{
  /* TODO: Implement. */
}

function handle_achievement_info(packet)
{
  /* TODO: implement */
}
