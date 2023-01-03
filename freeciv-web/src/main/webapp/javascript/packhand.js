/***********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
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

const auto_attack_actions = [
  ACTION_ATTACK, ACTION_SUICIDE_ATTACK,
  ACTION_NUKE_UNITS, ACTION_NUKE_CITY, ACTION_NUKE
];

/* Indicates that the player initiated a request.
 * Special request number used by the server too. */
const REQEST_PLAYER_INITIATED = 0;
/* Refresh the action selection dialog */
const REQEST_BACKGROUND_REFRESH = 1;
/* Get possible actions for fast auto attack. */
const REQEST_BACKGROUND_FAST_AUTO_ATTACK = 2;

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
	|| get_client_page() == PAGE_NETWORK) {
      set_client_page(PAGE_START);
    }

    client_info = {
      "pid"           : packet_client_info,
      "gui"           : GUI_WEB,
      "emerg_version" : 0,
      "distribution"  : ""
    };
    send_request(JSON.stringify(client_info));

    set_client_state(C_S_PREPARING);

    if (($.getUrlVar('action') == "new" || $.getUrlVar('action') == "hack")
        && $.getUrlVar('ruleset') != null) {
      change_ruleset($.getUrlVar('ruleset'));
    }

    if (renderer == RENDERER_WEBGL && !observing) {
       // Reduce the amount of rivers, it's kind of ugly at the moment.
       send_message("/set wetness 25");

       // Freeciv WebGL doesn't support hex or iso yet.
       send_message("/set topology=");

       // Freeciv WebGL doesn't support map wrapping yet.
       send_message("/set wrap=");

       // Less hills will be more user-friendly in 3D mode.
       send_message("/set steepness 12");

     }

    if (autostart) {
      if (renderer == RENDERER_WEBGL) {
        $.blockUI({ message: '<h2>Generating terrain map model...</h2>' });
      }
      if (loadTimerId == -1) {
        wait_for_text("You are logged in as", pregame_start_game);
      } else {
        wait_for_text("Load complete", pregame_start_game);
      }
    } else if (observing) {
      wait_for_text("You are logged in as", request_observe_game);
    }

  } else {

    swal("You were rejected from the game.", (packet['message'] || ""), "error");
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
      if (client.conn.player_num == null
          || client.conn.player_num !== packet['player_num']) {
        discard_diplomacy_dialogs();
      }
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

  if (message == null) return;
  if (event == null || event < 0 || event >= E_UNDEFINED) {
    console.log('Undefined message event type');
    console.log(packet);
    packet['event'] = E_UNDEFINED;
  }

  if (connections[conn_id] != null) {
    if (!is_longturn()) {
      message = "<b>" + connections[conn_id]['username'] + ":</b>" + message;
    }
  } else if (packet['event'] == E_SCRIPT) {
    var regxp = /\n/gi;
    message = message.replace(regxp, "<br>\n");
    show_dialog_message("Message for you:", message);
    return;
  } else {
    if (message.indexOf("/metamessage") != -1) return;  //don't spam message dialog on game start.
    if (message.indexOf("Metaserver message string") != -1) return;  //don't spam message dialog on game start.

    if (ptile != null && ptile > 0) {
       message = "<span class='chatbox_text_tileinfo' "
           + "onclick='center_tile_id(" + ptile + ");'>" + message + "</span>";
    }
    if (is_speech_supported()) speak(message);
  }

  packet['message'] = message;
  add_chatbox_text(packet);
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

/***************************************************************************
  The city_info packet is used when the player has full information about a
  city, including it's internals.

  This is followed by other packets.
  First one is the city_nationalities, followed by city_rally_point

  Finally, web_city_info_addition gives additional
  information only needed by Freeciv-web. Its processing will therefore
  stop while it waits for the corresponding web_city_info_addition packet.
***************************************************************************/
function handle_city_info(packet)
{
  let shield_stock_changed;
  let production_changed;
  let pcity;

  /* Decode the city name. */
  packet['name'] = decodeURIComponent(packet['name']);

  /* Decode bit vectors. */
  packet['improvements'] = new BitVector(packet['improvements']);
  packet['city_options'] = new BitVector(packet['city_options']);

  /* Add an unhappy field like the city_short_info packet has. */
  packet['unhappy'] = city_unhappy(packet);

  if (cities[packet['id']] == null) {
    pcity = packet;
    cities[packet['id']] = packet;
    if (C_S_RUNNING == client_state() && !observing && benchmark_start == 0
        && !client_is_observer() && packet['owner'] == client.conn.playing.playerno) {
      show_city_dialog_by_id(packet['id']);
    }
  } else {
    pcity = cities[packet['id']];
    cities[packet['id']] = $.extend(cities[packet['id']], packet);
  }

  if (pcity['shield_stock'] != packet['shield_stock']) {
    shield_stock_changed = true;
  }

  if (pcity['production_kind'] != packet['production_kind']
      || pcity['production_value'] != packet['production_value']) {
    production_changed = true;
  }

  /* manually update tile relation.*/
  if (tiles[packet['tile']] != null) {
    tiles[packet['tile']]['worked'] = packet['id'];
  }

  /* update caravan dialog */
  if ((production_changed || shield_stock_changed)
      && action_selection_target_city() == pcity['id']) {
    let refresh_packet = {
      "pid"             : packet_unit_get_actions,
      "actor_unit_id"   : action_selection_actor_unit(),
      "target_unit_id"  : action_selection_target_unit(),
      "target_tile_id"  : city_tile(pcity)['index'],
      "target_extra_id" : action_selection_target_extra(),
      "request_kind"    : REQEST_BACKGROUND_REFRESH,
    };

    send_request(JSON.stringify(refresh_packet));
  }

  /* Stop the processing here. Wait for the web_city_info_addition packet.
   * The processing of this packet will continue once it arrives. */
}

/***************************************************************************
  Generic handling of follow up packets of city_info.
***************************************************************************/
function city_info_follow_up(packet, pname)
{
  if (cities[packet['id']] == null) {
    /* The city should have been sent before the additional info. */
    console.log(pname + " for unknown city "
                + packet['id']);
    return;
  }

  $.extend(cities[packet['id']], packet);
}

/***************************************************************************
  This is a follow up packet to city_info packet.
***************************************************************************/
function handle_city_nationalities(packet)
{
  city_info_follow_up(packet, "packet_city_nationalities");
}

/***************************************************************************
  This is a follow up packet to city_info packet.
***************************************************************************/
function handle_city_rally_point(packet)
{
  city_info_follow_up(packet, "packet_city_rally_point");
}

/***************************************************************************
  The web_city_info_addition packet is a follow up packet to
  city_info packet. It gives some information the C clients calculates on
  their own. It is used when the player has full information about a city,
  including it's internals.
***************************************************************************/
function handle_web_city_info_addition(packet)
{
  if (cities[packet['id']] == null) {
    /* The city should have been sent before the additional info. */
    console.log("packet_web_city_info_addition for unknown city "
                + packet['id']);
    return;
  } else {
    packet['can_build_improvement'] = new BitVector(packet['can_build_improvement']);
    packet['can_build_unit'] = new BitVector(packet['can_build_unit']);

    /* Merge the information from web_city_info_addition into the recently
     * received city_info. */
    $.extend(cities[packet['id']], packet);
  }

  /* Get the now merged city_info. */
  packet = cities[packet['id']];

  /* Continue with the city_info processing. */

  if (active_city != null && !client_is_observer()) {
    show_city_dialog(active_city);
  }

  if (packet['diplomat_investigate'] && !client_is_observer()) {
    show_city_dialog(cities[packet['id']]);
  }

  if (worklist_dialog_active && active_city != null) {
    city_worklist_dialog(active_city);
  }

  if (renderer == RENDERER_WEBGL) {
    update_city_position(index_to_tile(packet['tile']));
  }

  /* Update the cities info tab */
  city_screen_updater.update();
  bulbs_output_updater.update();
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

  /* Update the cities info tab */
  city_screen_updater.update();
  bulbs_output_updater.update();
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

/************************************************************************//**
  Handle information about a player.

  It is followed by web_player_info_addition that gives additional
  information only needed by Freeciv-web. Its processing will therefore
  stop while it waits for the corresponding web_player_info_addition packet.
****************************************************************************/
function handle_player_info(packet)
{
  /* Interpret player flags. */
  packet['flags'] = new BitVector(packet['flags']);
  packet['gives_shared_vision'] = new BitVector(packet['gives_shared_vision']);

  players[packet['playerno']] = $.extend(players[packet['playerno']], packet);
}

/************************************************************************//**
  The web_player_info_addition packet is a follow up packet to the
  player_info packet. It gives some information the C clients calculates on
  their own.
****************************************************************************/
function handle_web_player_info_addition(packet)
{
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

  assign_nation_color(players[packet['playerno']]['nation']);
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

  if (renderer == RENDERER_WEBGL) {
    mapview_model_width = Math.floor(MAPVIEW_ASPECT_FACTOR * map['xsize']);
    mapview_model_height = Math.floor(MAPVIEW_ASPECT_FACTOR * map['ysize']);
  }

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

  add_replay_frame();


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

  game_rules = null;
  nation_groups = [];
  nations = {};
  specialists = {};
  techs = {};
  governments = {};
  terrain_control = {};
  SINGLE_MOVE = undefined;
  unit_types = {};
  unit_classes = {};
  city_rules = {};
  terrains = {};
  resources = {};
  goods = {};
  actions = {};

  improvements_init();

  /* handle_ruleset_extra defines some variables dynamically */
  for (var extra in extras) {
    var ename = extras[extra]['rule_name'];
    if (ename == "Railroad") delete window["EXTRA_RAIL"];
    else if (ename == "Oil Well") delete window["EXTRA_OIL_WELL"];
    else delete window["EXTRA_" + ename.toUpperCase()];
  }
  extras = {};

  /* Reset legal diplomatic clauses. */
  clause_infos = {};

  /* TODO: implement rest.
   * Some ruleset packets don't have handlers *yet*. Remember to clean up
   * when they are implemented:
   *   handle_ruleset_government_ruler_title
   *   handle_ruleset_base
   *   handle_ruleset_choices
   *   handle_ruleset_counter
   *   handle_ruleset_road
   *   handle_ruleset_disaster
   *   handle_ruleset_extra_flag
   *   handle_ruleset_trade
   *   handle_ruleset_unit_bonus
   *   handle_ruleset_unit_flag
   *   handle_ruleset_unit_class_flag
   *   handle_ruleset_terrain_flag
   *   handle_ruleset_achievement
   *   handle_ruleset_tech_flag
   *   handle_ruleset_nation_sets
   *   handle_ruleset_style
   *   handle_ruleset_music
   *   handle_ruleset_multiplier
   *   handle_ruleset_action_auto
   *
   * Maybe also:
   *   handle_rulesets_ready
   *   handle_nation_availability
   */

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
  /* TODO: implement */
}

function handle_nuke_tile_info(packet)
{
  var ptile = index_to_tile(packet['tile']);
  if (renderer == RENDERER_WEBGL) {
    render_nuclear_explosion(ptile);
  } else {
    ptile['nuke'] = 60;
  }
  play_sound('LrgExpl.ogg');

}

/* done */
function handle_city_remove(packet)
{
  remove_city(packet['city_id']);
}

function handle_city_update_counter(packet)
{
  /* TODO */
}

function handle_connect_msg(packet)
{
  add_chatbox_text(packet);
}

/* done */
function handle_server_info(packet)
{

  if (packet['emerg_version'] > 0) {
    console.log('Server has version %d.%d.%d.%d%s',
      packet.major_version, packet.minor_version, packet.patch_version,
      packet.emerg_version, packet.version_label);
  } else {
    console.log("Server has version %d.%d.%d%s",
      packet.major_version, packet.minor_version, packet.patch_version,
      packet.version_label);
  }
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
  if (packet["request_kind"] != REQEST_PLAYER_INITIATED) {
    console.log("handle_city_sabotage_list(): was asked to not disturb "
                + "the player. That is unimplemented.");
  }

  popup_sabotage_dialog(game_find_unit_by_number(packet['actor_id']),
                        game_find_city_by_number(packet['city_id']),
                        new BitVector(packet['improvements']),
                        packet['act_id']);
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

  if (punit == null) {
    return;
  }

  /* Close the action selection dialog if the actor unit is lost */
  if (action_selection_in_progress_for === punit.id) {
    action_selection_close();
    /* Open another action selection dialog if there are other actors in the
     * current selection that want a decision. */
    action_selection_next_in_focus(punit.id);
  }

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

/**********************************************************************//**
  Request that the player makes a decision for the specified unit unless
  it may be an automatic decision. In that case check if one of the auto
  actions are enabled.
**************************************************************************/
function action_decision_handle(punit)
{
  var a;

  for (a = 0; a < auto_attack_actions.length; a++) {
    let action = auto_attack_actions[a];
    if (utype_can_do_action(unit_type(punit), action) && auto_attack) {
      /* An auto action like auto attack could be legal. Check for those at
      * once so they won't have to wait for player focus. */
      var packet = {
        "pid"             : packet_unit_get_actions,
        "actor_unit_id"   : punit['id'],
        "target_unit_id"  : IDENTITY_NUMBER_ZERO,
        "target_tile_id"  : punit['action_decision_tile'],
        "target_extra_id" : EXTRA_NONE,
        "request_kind"    : REQEST_BACKGROUND_FAST_AUTO_ATTACK,
      };

      send_request(JSON.stringify(packet));
      return; // Exit, don't request other possible actions in the loop.
    }
  }
  /* Other auto_action types can be checked here. Also see function below */

  action_decision_request(punit);
}

/**********************************************************************//**
  Do an auto action or request that the player makes a decision for the
  specified unit.
**************************************************************************/
function action_decision_maybe_auto(actor_unit, action_probabilities,
                                    target_tile, target_extra,
                                    target_unit, target_city)
{
  var a;

  for (a = 0; a < auto_attack_actions.length; a++) {
    let action = auto_attack_actions[a];

    if (action_prob_possible(action_probabilities[action])
        && auto_attack) {

      var target = target_tile['index'];
      if (action == ACTION_NUKE_CITY) {
        target = tile_city(target_tile);
        if (!target) continue;
        target = target['id'];
      }

      request_unit_do_action(action,
          actor_unit['id'], target);

      return; // Exit, don't request other possible actions in the loop.
    }
  }
  /* Other auto_action types can be checked here. Also see function above */

  action_decision_request(actor_unit);
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
  var punit = player_find_unit_by_id(unit_owner(packet_unit), packet_unit['id']);

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
    if (should_ask_server_for_actions(packet_unit)) {
      action_decision_handle(packet_unit);
    }
    packet_unit['anim_list'] = [];
    units[packet_unit['id']] = packet_unit;
    units[packet_unit['id']]['facing'] = 6;
  } else {
    if ((punit['action_decision_want'] != packet_unit['action_decision_want']
         || punit['action_decision_tile'] != packet_unit['action_decision_tile'])
        && should_ask_server_for_actions(packet_unit)) {
      /* The unit wants the player to decide. */
      action_decision_handle(packet_unit);
    }

    update_unit_anim_list(units[packet_unit['id']], packet_unit);
    units[packet_unit['id']] = $.extend(units[packet_unit['id']], packet_unit);

    for (var i = 0; i < current_focus.length; i++) {
      if (current_focus[i]['id'] == packet_unit['id']) {
        $.extend(current_focus[i], packet_unit);
      }
    }
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

  if (renderer == RENDERER_WEBGL) {
    if (attacker_hp == 0) animate_explosion_on_tile(attacker['tile'], 0);
    if (defender_hp == 0) animate_explosion_on_tile(defender['tile'], 0);
  } else {
    if (attacker_hp == 0 && is_unit_visible(attacker)) {
      explosion_anim_map[attacker['tile']] = 25;
    }
    if (defender_hp == 0 && is_unit_visible(defender)) {
      explosion_anim_map[defender['tile']] = 25;
    }
  }


}

/**************************************************************************
  Handle the requested follow up question about an action
**************************************************************************/
function handle_unit_action_answer(packet)
{
  var diplomat_id = packet['actor_id'];
  var target_id = packet['target_id'];
  var cost = packet['cost'];
  var action_type = packet['action_type'];

  var target_city = game_find_city_by_number(target_id);
  var target_unit = game_find_unit_by_number(target_id);
  var actor_unit = game_find_unit_by_number(diplomat_id);

  if (actor_unit == null) {
    console.log("Bad actor unit (" + diplomat_id
                + ") in unit action answer.");
    act_sel_queue_done(diplomat_id);
    return;
  }

  if (packet["request_kind"] != REQEST_PLAYER_INITIATED) {
    console.log("handle_unit_action_answer(): was asked to not disturb "
                + "the player. That is unimplemented.");
  }

  if (action_type == ACTION_SPY_BRIBE_UNIT) {
    if (target_unit == null) {
      console.log("Bad target unit (" + target_id
                  + ") in unit action answer.");
      act_sel_queue_done(diplomat_id);
      return;
    } else {
      popup_bribe_dialog(actor_unit, target_unit, cost, action_type);
      return;
    }
  } else if (action_type == ACTION_SPY_INCITE_CITY
             || action_type == ACTION_SPY_INCITE_CITY_ESC) {
    if (target_city == null) {
      console.log("Bad target city (" + target_id
                  + ") in unit action answer.");
      act_sel_queue_done(diplomat_id);
      return;
    } else {
      popup_incite_dialog(actor_unit, target_city, cost, action_type);
      return;
    }
  } else if (action_type == ACTION_UPGRADE_UNIT) {
    if (target_city == null) {
      console.log("Bad target city (" + target_id
                  + ") in unit action answer.");
      act_sel_queue_done(diplomat_id);
      return;
    } else {
      popup_unit_upgrade_dlg(actor_unit, target_city, cost, action_type);
      return;
    }
  } else if (action_type == ACTION_COUNT) {
    console.log("unit_action_answer: Server refused to respond.");
  } else {
    console.log("unit_action_answer: Invalid answer.");
  }
  act_sel_queue_done(diplomat_id);
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
  var target_extra_id = packet['target_extra_id'];
  var action_probabilities = packet['action_probabilities'];

  var pdiplomat = game_find_unit_by_number(actor_unit_id);
  var target_unit = game_find_unit_by_number(target_unit_id);
  var target_city = game_find_city_by_number(target_city_id);
  var ptile = index_to_tile(target_tile_id);
  var target_extra = extra_by_number(target_extra_id);

  var hasActions = false;

  /* The dead can't act. */
  if (pdiplomat != null && ptile != null) {
    action_probabilities.forEach(function(prob) {
      if (action_prob_possible(prob)) {
        hasActions = true;
      }
    });
  }

  switch (packet['request_kind']) {
  case REQEST_PLAYER_INITIATED:
    if (hasActions) {
      popup_action_selection(pdiplomat, action_probabilities,
                             ptile, target_extra, target_unit, target_city);
    } else if (packet['request_kind'] == REQEST_PLAYER_INITIATED) {
      /* Nothing to do. */
      action_selection_no_longer_in_progress(actor_unit_id);
      action_decision_clear_want(actor_unit_id);
      action_selection_next_in_focus(actor_unit_id);
    }
    break;
  case REQEST_BACKGROUND_REFRESH:
    action_selection_refresh(pdiplomat,
                             target_city, target_unit, ptile,
                             target_extra,
                             action_probabilities);
    break;
  case REQEST_BACKGROUND_FAST_AUTO_ATTACK:
    action_decision_maybe_auto(pdiplomat, action_probabilities,
                               ptile, target_extra,
                               target_unit, target_city);
    break;
  default:
    console.log("handle_unit_actions(): unrecognized request_kind %d",
                packet['request_kind']);
    break;
  }
}

function handle_diplomacy_init_meeting(packet)
{
  // for hotseat games, only activate diplomacy if the player is playing.
  if (is_hotseat() && packet['initiated_from'] != client.conn.playing['playerno']) return;   

  diplomacy_clause_map[packet['counterpart']] = [];
  show_diplomacy_dialog(packet['counterpart']);
  show_diplomacy_clauses(packet['counterpart']);
}

function handle_diplomacy_cancel_meeting(packet)
{
  cancel_meeting(packet['counterpart']);
}

function handle_diplomacy_create_clause(packet)
{
  var counterpart_id = packet['counterpart'];
  if(diplomacy_clause_map[counterpart_id] == null) {
    diplomacy_clause_map[counterpart_id] = [];
  }
  diplomacy_clause_map[counterpart_id].push(packet);
  show_diplomacy_clauses(counterpart_id);
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
      $("#turn_done_button").button("option", "label", "Turn Done");
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

/************************************************************************//**
  The web_ruleset_unit_addition packet is a follow up packet to the
  ruleset_unit packet. It gives some information the C clients calculates on
  their own.
****************************************************************************/
function handle_web_ruleset_unit_addition(packet)
{
  /* Decode bit vector. */
  packet['utype_actions'] = new BitVector(packet['utype_actions']);

  unit_types[packet['id']] = $.extend(unit_types[packet['id']], packet);
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
}

function handle_ruleset_city(packet)
{
  city_rules[packet['style_id']] = packet;
}

/* 100% complete */
function handle_ruleset_building(packet)
{
  improvements_add_building(packet);
}

function handle_ruleset_unit_class(packet)
{
  packet['flags'] = new BitVector(packet['flags']);
  unit_classes[packet['id']] = packet;
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

function handle_edit_startpos(packet)
{
  /* edit not supported. */
}

function handle_edit_startpos_full(packet)
{
  /* edit not supported. */
}

function handle_edit_object_created(packet)
{
  /* edit not supported. */
}

/**************************************************************************
  Received goto path, likely because we requested one.
**************************************************************************/
function handle_web_goto_path(packet)
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
  let need_players_dialog_update = false;

  if (client == null || client.conn.playing == null) return;

  if (packet['plr2'] == client.conn.playing['playerno']) {
    let ds = players[packet['plr1']].diplstates;

    if (ds != undefined && ds[packet['plr2']] != undefined
        && ds[packet['plr2']]['state'] != packet['type']) {
      need_players_dialog_update = true;
    }
  }

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

  // TODO: remove current diplstates (after moving all users to the new one),
  //       or just make it a reference to players[me].diplstates
  //
  // There's no need to set players[packet.plr2].diplstates, as there'll be
  // a packet for that.  In fact, there's a packet for each of (p1,x) and (p2,x)
  // when the state changes between p1 and p2, and for all pairs of players
  // when a turn begins
  if (players[packet['plr1']].diplstates === undefined) {
    players[packet['plr1']].diplstates = [];
  }
  players[packet['plr1']].diplstates[packet['plr2']] = {
    state: packet['type'],
    turns_left: packet['turns_left'],
    contact_turns_left: packet['contact_turns_left']
  };

  if (need_players_dialog_update
      && action_selection_actor_unit() != IDENTITY_NUMBER_ZERO) {
    /* An action selection dialog is open and our diplomatic state just
     * changed. Find out if the relationship that changed was to a
     * potential target. */
    let tgt_tile;
    let tgt_unit;
    let tgt_city;

    if ((action_selection_target_unit() != IDENTITY_NUMBER_ZERO
         && ((tgt_unit
              = game_find_unit_by_number(action_selection_target_unit())))
         && tgt_unit['owner'] == packet['plr1'])
        || (action_selection_target_city() != IDENTITY_NUMBER_ZERO
            && ((tgt_city
                 = game_find_city_by_number(action_selection_target_city())))
            && tgt_city['owner'] == packet['plr1'])
        || ((tgt_tile = index_to_tile(action_selection_target_tile()))
            && tile_owner(tgt_tile) == packet['plr1'])) {
      /* The diplomatic relationship to the target in an open action
       * selection dialog have changed. This probably changes
       * the set of available actions. */
      let refresh_packet = {
        "pid"             : packet_unit_get_actions,
        "actor_unit_id"   : action_selection_actor_unit(),
        "target_unit_id"  : action_selection_target_unit(),
        "target_tile_id"  : action_selection_target_tile(),
        "target_extra_id" : action_selection_target_extra(),
        "request_kind"    : REQEST_BACKGROUND_REFRESH,
      };

      send_request(JSON.stringify(refresh_packet));
    }
  }
}

/**************************************************************************
  Packet handle_ruleset_extra handler. Also defines EXTRA_* variables
  dynamically.
**************************************************************************/
function handle_ruleset_extra(packet)
{
  /* Decode bit vectors. */
  packet['causes'] = new BitVector(packet['causes']);
  packet['rmcauses'] = new BitVector(packet['rmcauses']);

  packet['name'] = string_unqualify(packet['name']);
  extras[packet['id']] = packet;
  extras[packet['rule_name']] = packet;

  if (packet['rule_name'] == "Railroad") window["EXTRA_RAIL"] = packet['id'];
  else if (packet['rule_name'] == "Oil Well") window["EXTRA_OIL_WELL"] = packet['id'];
  else window["EXTRA_" + packet['rule_name'].toUpperCase()] = packet['id'];
}

/************************************************************************//**
  Packet handle_ruleset_ruleset handler.
****************************************************************************/
function handle_ruleset_counter(packet)
{
  /* TODO: implement */
}

/************************************************************************//**
  Packet ruleset_extra_flag handler.
****************************************************************************/
function handle_ruleset_extra_flag(packet)
{
  /* TODO: implement */
}

/************************************************************************//**
  Handle a packet about a particular base type.
****************************************************************************/
function handle_ruleset_base(packet)
{
  var i;

  for (i = 0; i < MAX_EXTRA_TYPES; i++) {
    if (is_extra_caused_by(extras[i], EC_BASE)
        && extras[i]['base'] == null) {
      /* This is the first base without base data */
      extras[i]['base'] = packet;
      extras[extras[i]['rule_name']]['base'] = packet;
      return;
    }
  }

  console.log("Didn't find Extra to put Base on");
  console.log(packet);
}

/************************************************************************//**
  Handle a packet about a particular road type.
****************************************************************************/
function handle_ruleset_road(packet)
{
  var i;

  for (i = 0; i < MAX_EXTRA_TYPES; i++) {
    if (is_extra_caused_by(extras[i], EC_ROAD)
        && extras[i]['road'] == null) {
      /* This is the first road without road data */
      extras[i]['road'] = packet;
      extras[extras[i]['rule_name']]['road'] = packet;
      return;
    }
  }

  console.log("Didn't find Extra to put Road on");
  console.log(packet);
}

/************************************************************************//**
  Handle a packet about a particular action enabler.
****************************************************************************/
function handle_ruleset_action_enabler(packet)
{
  var paction = actions[packet.enabled_action];

  if (paction === undefined) {
    console.log("Unknown action " + packet.action + " for enabler ");
    console.log(packet);
    return;
  }

  /* Store the enabler in its action. */
  paction.enablers.push(packet);
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

/************************************************************************//**
  Packet ruleset_multiplier handler.
****************************************************************************/
function handle_ruleset_multiplier(packet)
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
  bulbs_output_updater.update();
}

function handle_unknown_research(packet)
{
  delete research_data[packet['id']];
}

function handle_worker_task(packet)
{
  /* TODO: Implement */
}

function handle_timeout_info(packet)
{
  last_turn_change_time = Math.ceil(packet['last_turn_change_time']);
  seconds_to_phasedone = Math.floor(packet['seconds_to_phasedone']);
  seconds_to_phasedone_sync = new Date().getTime();
}

function handle_play_music(packet)
{
  /* TODO: Implement */
}

/* Receive a generalized action. */
function handle_ruleset_action(packet)
{
  actions[packet['id']] = packet;
  packet["enablers"] = [];
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

/************************************************************************//**
  Handle a packet about a particular clause.
****************************************************************************/
function handle_ruleset_clause(packet)
{
  clause_infos[packet['type']] = packet;
}

function handle_achievement_info(packet)
{
  /* TODO: implement */
}

function handle_team_name_info(packet)
{
  /* TODO: implement */
}
