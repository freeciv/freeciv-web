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


/* All generalized actions. */
var actions = {};


/*
 * ACTPROB_IMPOSSIBLE is another way of saying that the probability is 0%.
 */
var ACTPROB_IMPOSSIBLE = 0;

/*
 * The special value ACTPROB_NA indicates that no probability should exist.
 */
var ACTPROB_NA = 253;

/*
 * The special value ACTPROB_NOT_IMPLEMENTED indicates that support
 * for finding this probability currently is missing.
 */
var ACTPROB_NOT_IMPLEMENTED = 254;

/*
 * The special value ACTPROB_NOT_KNOWN indicates that the player don't know
 * enough to find out. It is caused by the probability depending on a rule
 * that depends on game state the player don't have access to. It may be
 * possible for the player to later gain access to this game state.
 */
var ACTPROB_NOT_KNOWN = 255;


/**************************************************************************
  Returns true unless a situation were a regular move always would be
  impossible is recognized.
**************************************************************************/
function can_actor_unit_move(actor_unit, target_tile)
{
  var tgt_owner_id;

  if (index_to_tile(actor_unit['tile']) == target_tile) {
    /* The unit is already on this tile. */
    return false;
  }

  for (var i = 0; i < tile_units(target_tile).length; i++) {
    tgt_owner_id = unit_owner(tile_units(target_tile)[i])['playerno'];

    if (tgt_owner_id != unit_owner(actor_unit)['playerno']
        && diplstates[tgt_owner_id] != DS_ALLIANCE
        && diplstates[tgt_owner_id] != DS_TEAM) {
      /* In current Freeciv acting units can't attack foreign units. */
      return false;
    }
  };

  if (tile_city(target_tile) != null) {
    tgt_owner_id = city_owner(tile_city(target_tile))['playerno'];

    if (tgt_owner_id == unit_owner(actor_unit)['playerno']) {
      /* This city isn't foreign. */
      return true;
    }

    if (diplstates[tgt_owner_id] == DS_ALLIANCE
        || diplstates[tgt_owner_id] == DS_TEAM) {
      /* This city belongs to an ally. */
      return true;
    }

    /* TODO: Support rulesets were actors can invade cities. */
    return false;
  }

  /* It is better to show the "Keep moving" option one time to much than
   * one time to little. */
  return true;
}

/**************************************************************************
  Encode a building ID for transfer in the value field of
  packet_unit_do_action for targeted sabotage city.
**************************************************************************/
function encode_building_id(building_id)
{
  /* Building ID is encoded in the value field by adding one so the
   * building ID -1 (current production) can be transferred. */
  return building_id + 1;
}

/****************************************************************************
  Format the probability that an action will be a success.
****************************************************************************/
function format_action_probability(probability)
{
  if (probability <= 200) {
    /* This is a regular chance of success. */
    return " (" + (probability / 2) + "%)";
  } else if (probability == ACTPROB_NOT_KNOWN) {
    /* This is know to be unknown. */
    return " (?%)";
  } else {
    /* The remaining action probabilities shouldn't be displayed. */
    return "";
  }
}

/**************************************************************************
  Format the label of an action selection button.
**************************************************************************/
function format_action_label(action_id, action_probabilities)
{
  return actions[action_id]['ui_name'].replace("%s", "").replace("%s",
      format_action_probability(action_probabilities[action_id]))
}

/****************************************************************************
  Ask the player to select an action.
****************************************************************************/
function popup_action_selection(actor_unit, action_probabilities,
                                target_tile, target_unit, target_city)
{
  // reset dialog page.
  var id = "#act_sel_dialog_" + actor_unit['id'];
  $(id).remove();
  $("<div id='act_sel_dialog_" + actor_unit['id'] + "'></div>").appendTo("div#game_page");

  var actor_homecity = cities[actor_unit['homecity']];

  var dhtml = "<center>";

  if (target_city != null) {
    dhtml += "Your " + unit_types[actor_unit['type']]['name'];

    /* Some units don't have a home city. */
    if (actor_homecity != null) {
      dhtml += " from " + decodeURIComponent(actor_homecity['name']);
    }

    dhtml += " has arrived at " + decodeURIComponent(target_city['name'])
             + ". What is your command?";
  } else if (target_unit != null) {
    dhtml += "Your " + unit_types[actor_unit['type']]['name']
             + " is ready to act against "
             + nations[unit_owner(target_unit)['nation']]['adjective']
             + " " + unit_types[target_unit['type']]['name'] + ".";
  }

  dhtml += "<br>";

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    dhtml += "<input id='act_sel_emb" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_ESTABLISH_EMBASSY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    dhtml += "<input id='act_sel_inv" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_INVESTIGATE_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='act_sel_sab" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_SABOTAGE_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_TARGETED_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='act_sel_tgt_sab" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_TARGETED_SABOTAGE_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    dhtml += "<input id='act_sel_tech" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_STEAL_TECH,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_TARGETED_STEAL_TECH] != 0) {
    dhtml += "<input id='act_sel_tgt_tech" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_TARGETED_STEAL_TECH,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_STEAL_GOLD] != 0) {
    dhtml += "<input id='act_sel_steal_gold" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_STEAL_GOLD,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_STEAL_MAPS] != 0) {
    dhtml += "<input id='act_sel_steal_maps" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_STEAL_MAPS,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    dhtml += "<input id='act_sel_revo" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_INCITE_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    dhtml += "<input id='act_sel_poi" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_POISON,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_TRADE_ROUTE] != 0) {
    dhtml += "<input id='act_sel_trade" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_TRADE_ROUTE,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_MARKETPLACE] != 0) {
    dhtml += "<input id='act_sel_market" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_MARKETPLACE,
                              action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_HELP_WONDER] != 0) {
    dhtml += "<input id='act_sel_wonder" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_HELP_WONDER,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_JOIN_CITY] != 0) {
    dhtml += "<input id='act_sel_join_city" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_JOIN_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_NUKE] != 0) {
    dhtml += "<input id='act_sel_spy_nuke" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_NUKE,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_DESTROY_CITY] != 0) {
    dhtml += "<input id='act_sel_destroy_city" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_DESTROY_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_RECYCLE_UNIT] != 0) {
    dhtml += "<input id='act_sel_recycle_unit" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_RECYCLE_UNIT,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    dhtml += "<input id='act_sel_bribe" + actor_unit['id']
        + "' class='act_sel_button' type='button' value='"
        + format_action_label(ACTION_SPY_BRIBE_UNIT,
                              action_probabilities)
        + "'>";
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    dhtml += "<input id='act_sel_spy_sabo" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_SPY_SABOTAGE_UNIT,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_EXPEL_UNIT] != 0) {
    dhtml += "<input id='act_sel_expel_unit" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_EXPEL_UNIT,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_CAPTURE_UNITS] != 0) {
    dhtml += "<input id='act_sel_capture_units" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_CAPTURE_UNITS,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_FOUND_CITY] != 0) {
    dhtml += "<input id='act_sel_found_city" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_FOUND_CITY,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_NUKE] != 0) {
    dhtml += "<input id='act_sel_nuke" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_NUKE,
                                   action_probabilities)
             + "'>";
  }
  if (action_probabilities[ACTION_BOMBARD] != 0) {
    dhtml += "<input id='act_sel_bombard" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + format_action_label(ACTION_BOMBARD,
                                   action_probabilities)
             + "'>";
  }
  if (can_actor_unit_move(actor_unit, target_tile)) {
    dhtml += "<input id='act_sel_move" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Keep moving'>"
  }

  dhtml += "<input id='act_sel_cancel" + actor_unit['id']
          + "' class='act_sel_button' type='button' value='Cancel'>"
         + "</center>";
  $(id).html(dhtml);

  $(id).attr("title",
             "Choose Your " + unit_types[actor_unit['type']]['name']
             + "'s Strategy");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: "350"});

  $(id).dialog('open');
  $(".act_sel_button").button();


  $("#act_sel_cancel" + actor_unit['id']).click(function() {
    $(id).remove();
  });

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    $("#act_sel_emb" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_ESTABLISH_EMBASSY};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    $("#act_sel_inv" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_INVESTIGATE_CITY};
        send_request(JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    $("#act_sel_sab" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_SABOTAGE_CITY};
        send_request(JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_TARGETED_SABOTAGE_CITY] != 0) {
    $("#act_sel_tgt_sab" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_action_query,
                    "diplomat_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "action_type": ACTION_SPY_TARGETED_SABOTAGE_CITY};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    $("#act_sel_tech" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_STEAL_TECH};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_TARGETED_STEAL_TECH] != 0) {
    $("#act_sel_tgt_tech" + actor_unit['id']).click(function() {
      popup_steal_tech_selection_dialog(actor_unit, target_city,
                                        action_probabilities);

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_STEAL_GOLD] != 0) {
    $("#act_sel_steal_gold" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_STEAL_GOLD};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_STEAL_MAPS] != 0) {
    $("#act_sel_steal_maps" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_STEAL_MAPS};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    $("#act_sel_revo" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_action_query,
                    "diplomat_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "action_type": ACTION_SPY_INCITE_CITY};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    $("#act_sel_poi" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_POISON};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_TRADE_ROUTE] != 0) {
    $("#act_sel_trade" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_TRADE_ROUTE};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_MARKETPLACE] != 0) {
    $("#act_sel_market" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_MARKETPLACE};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_HELP_WONDER] != 0) {
    $("#act_sel_wonder" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_city['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_HELP_WONDER};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_JOIN_CITY] != 0) {
    $("#act_sel_join_city" + actor_unit['id']).click(function() {
      var packet = {"pid"         : packet_unit_do_action,
                    "actor_id"    : actor_unit['id'],
                    "target_id"   : target_city['id'],
                    "value"       : 0,
                    "name"        : "",
                    "action_type" : ACTION_JOIN_CITY};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_NUKE] != 0) {
    $("#act_sel_spy_nuke" + actor_unit['id']).click(function() {
      var packet = {"pid"         : packet_unit_do_action,
                    "actor_id"    : actor_unit['id'],
                    "target_id"   : target_city['id'],
                    "value"       : 0,
                    "name"        : "",
                    "action_type" : ACTION_SPY_NUKE};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_DESTROY_CITY] != 0) {
    $("#act_sel_destroy_city" + actor_unit['id']).click(function() {
      var packet = {"pid"         : packet_unit_do_action,
                    "actor_id"    : actor_unit['id'],
                    "target_id"   : target_city['id'],
                    "value"       : 0,
                    "name"        : "",
                    "action_type" : ACTION_DESTROY_CITY};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_RECYCLE_UNIT] != 0) {
    $("#act_sel_recycle_unit" + actor_unit['id']).click(function() {
      var packet = {"pid"         : packet_unit_do_action,
                    "actor_id"    : actor_unit['id'],
                    "target_id"   : target_city['id'],
                    "value"       : 0,
                    "name"        : "",
                    "action_type" : ACTION_RECYCLE_UNIT};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    $("#act_sel_bribe" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_action_query,
                    "diplomat_id" : actor_unit['id'],
                    "target_id": target_unit['id'],
                    "action_type": ACTION_SPY_BRIBE_UNIT};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    $("#act_sel_spy_sabo" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_unit['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_SABOTAGE_UNIT};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_EXPEL_UNIT] != 0) {
    $("#act_sel_expel_unit" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_unit['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_EXPEL_UNIT};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_CAPTURE_UNITS] != 0) {
    $("#act_sel_capture_units" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_tile['index'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_CAPTURE_UNITS};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_FOUND_CITY] != 0) {
    $("#act_sel_found_city" + actor_unit['id']).click(function() {
      /* Ask the server to suggest a city name. */
      var packet = {"pid"     : packet_city_name_suggestion_req,
                    "unit_id" : actor_unit['id'] };
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_NUKE] != 0) {
    $("#act_sel_nuke" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_tile['index'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_NUKE};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_BOMBARD] != 0) {
    $("#act_sel_bombard" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_tile['index'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_BOMBARD};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (can_actor_unit_move(actor_unit, target_tile)) {
    $("#act_sel_move" + actor_unit['id']).click(function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_tile['index'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_MOVE};
      send_request(JSON.stringify(packet));

      $(id).remove();
    });
  }
}

/**************************************************************************
  Show the player the price of bribing the unit and, if bribing is
  possible, allow him to order it done.
**************************************************************************/
function popup_bribe_dialog(actor_unit, target_unit, cost)
{
  var bribe_possible = false;
  var dhtml = "";
  var id = "#bribe_unit_dialog_" + actor_unit['id'];

  /* Reset dialog page. */
  $(id).remove();

  $("<div id='bribe_unit_dialog_" + actor_unit['id'] + "'></div>")
      .appendTo("div#game_page");

  dhtml += "Treasury contains " + unit_owner(actor_unit)['gold'] + " gold. "
  dhtml += "The price of bribing "
              + nations[unit_owner(target_unit)['nation']]['adjective']
              + " " + unit_types[target_unit['type']]['name']
           + " is " + cost + ". ";

  bribe_possible = cost <= unit_owner(actor_unit)['gold'];

  if (!bribe_possible) {
    dhtml += "Traitors Demand Too Much!";
    dhtml += "<br>";
  }

  $(id).html(dhtml);

  var close_button = {	Close: function() {$(id).dialog('close');}};
  var bribe_close_button = {	"Cancel": function() {$(id).dialog('close');},
  				"Do it!": function() {
      var packet = {"pid" : packet_unit_do_action,
                    "actor_id" : actor_unit['id'],
                    "target_id": target_unit['id'],
                    "value" : 0,
                    "name" : "",
                    "action_type": ACTION_SPY_BRIBE_UNIT};
      send_request(JSON.stringify(packet));
      $(id).dialog('close');
    }
  };

  $(id).attr("title", "About that bribery you requested...");

  $(id).dialog({bgiframe: true,
                modal: true,
                buttons: (bribe_possible ? bribe_close_button : close_button),
                height: "auto",
                width: "auto"});

  $(id).dialog('open');

}

/**************************************************************************
  Show the player the price of inviting the city and, if inciting is
  possible, allow him to order it done.
**************************************************************************/
function popup_incite_dialog(actor_unit, target_city, cost)
{
  var incite_possible;
  var id;
  var dhtml;

  id = "#incite_city_dialog_" + actor_unit['id'];

  /* Reset dialog page. */
  $(id).remove();

  $("<div id='incite_city_dialog_" + actor_unit['id'] + "'></div>")
      .appendTo("div#game_page");

  dhtml = "";

  dhtml += "Treasury contains " + unit_owner(actor_unit)['gold'] + " gold."
  dhtml += " ";
  dhtml += "The price of inciting "
           + decodeURIComponent(target_city['name'])
           + " is " + cost + ".";

  incite_possible = cost != INCITE_IMPOSSIBLE_COST
                    && cost <= unit_owner(actor_unit)['gold'];

  if (!incite_possible) {
    dhtml += " ";
    dhtml += "Traitors Demand Too Much!";
    dhtml += "<br>";
  }

  $(id).html(dhtml);

  var close_button = {         Close:    function() {$(id).dialog('close');}};
  var incite_close_buttons = { 'Cancel': function() {$(id).dialog('close');},
                               'Do it!': function() {
                                 var packet = {"pid" : packet_unit_do_action,
                                               "actor_id" : actor_unit['id'],
                                               "target_id": target_city['id'],
                                               "value" : 0,
                                               "name" : "",
                                               "action_type": ACTION_SPY_INCITE_CITY};
                                 send_request(JSON.stringify(packet));

                                 $(id).dialog('close');
                               }
                             };

  $(id).attr("title", "About that incite you requested...");

  $(id).dialog({bgiframe: true,
                modal: true,
                buttons: (incite_possible ? incite_close_buttons : close_button),
                height: "auto",
                width: "auto"});

  $(id).dialog('open');
}

/**************************************************************************
  Create a button that steals a tech.

  Needed because of JavaScript's scoping rules.
**************************************************************************/
function create_steal_tech_button(parent_id, tech,
                                  actor_unit_id, target_city_id)
{
  /* Create the initial button with this tech */
  var button = {
    text : tech['name'],
    click : function() {
      var packet = {"pid" : packet_unit_do_action,
        "actor_id" : actor_unit_id,
        "target_id": target_city_id,
        "value" : tech['id'],
        "name" : "",
        "action_type": ACTION_SPY_TARGETED_STEAL_TECH};

      send_request(JSON.stringify(packet));

      $("#" + parent_id).remove();
    }
  };

  /* The button is ready. */
  return button;
}

/**************************************************************************
  Select what tech to steal when doing targeted tech theft.
**************************************************************************/
function popup_steal_tech_selection_dialog(actor_unit, target_city,
                                           act_probs)
{
  var id = "stealtech_dialog_" + actor_unit['id'];
  var buttons = [];

  /* Reset dialog page. */
  $("#" + id).remove();
  $("<div id='" + id + "'></div>").appendTo("div#game_page");

  /* Set dialog title */
  $("#" + id).attr("title", "Select Advance to Steal");

  /* List the alternatives */
  for (var tech_id in techs) {
    /* JavaScript for each iterates over keys. */
    var tech = techs[tech_id];

    /* Actor and target player tech known state. */
    var act_kn = player_invention_state(client.conn.playing, tech_id);
    var tgt_kn = player_invention_state(city_owner(target_city), tech_id);

    /* Can steal a tech if the target player knows it and the actor player
     * has the pre requirements. Some rulesets allows the player to steal
     * techs the player don't know the prereqs of. */
    if ((tgt_kn == TECH_KNOWN)
        && ((act_kn == TECH_PREREQS_KNOWN)
            || (game_info['tech_steal_allow_holes']
                && (act_kn == TECH_TECH_UNKNOWN)))) {
      /* Add a button for stealing this tech to the dialog. */
      buttons.push(create_steal_tech_button(id, tech,
                                            actor_unit['id'],
                                            target_city['id']));
    }
  }

  /* The player may change his mind after selecting targeted tech theft. */
  if (act_probs[ACTION_SPY_STEAL_TECH] != 0) {
    /* Untargeted tech theft may be legal. Add it as an alternative. */
    buttons.push({
                   text  : "At " + unit_types[actor_unit['type']]['name']
                           + "'s Discretion",
                   click : function() {
                     var packet = {
                       "pid" : packet_unit_do_action,
                       "actor_id" : actor_unit['id'],
                       "target_id": target_city['id'],
                       "value" : 0,
                       "name" : "",
                       "action_type": ACTION_SPY_STEAL_TECH};
                     send_request(JSON.stringify(packet));

                     $("#" + id).remove();
                   }
                 });
  }

  /* Allow the user to cancel. */
  buttons.push({
                 text : 'Cancel',
                 click : function() {
                   $("#" + id).remove();
                 }
               });

  /* Create the dialog. */
  $("#" + id).dialog({
                       modal: true,
                       buttons: buttons,
                       width: "90%"});

  /* Show the dialog. */
  $("#" + id).dialog('open');
}

/**************************************************************************
  Create a button that orders a spy to try to sabotage an improvement.

  Needed because of JavaScript's scoping rules.
**************************************************************************/
function create_sabotage_impr_button(improvement, parent_id,
                                     actor_unit_id, target_city_id)
{
  /* Create the initial button with this tech */
  var button = {
    text : improvement['name'],
    click : function() {
      var packet = {
        "pid"          : packet_unit_do_action,
        "actor_id"     : actor_unit_id,
        "target_id"    : target_city_id,
        "value"        : encode_building_id(improvement['id']),
        "name"         : "",
        "action_type"  : ACTION_SPY_TARGETED_SABOTAGE_CITY
      };
      send_request(JSON.stringify(packet));

      $("#" + parent_id).remove();
    }
  };

  /* The button is ready. */
  return button;
}

/**************************************************************************
  Select what improvement to sabotage when doing targeted sabotage city.
**************************************************************************/
function popup_sabotage_dialog(actor_unit, target_city, city_imprs)
{
  var id = "sabotage_impr_dialog_" + actor_unit['id'];
  var buttons = [];

  /* Reset dialog page. */
  $("#" + id).remove();
  $("<div id='" + id + "'></div>").appendTo("div#game_page");

  /* Set dialog title */
  $("#" + id).attr("title", "Select Improvement to Sabotage");

  /* List the alternatives */
  for (var i = 0; i < ruleset_control['num_impr_types']; i++) {
    var improvement = improvements[i];

    if (city_imprs.isSet(i)
        && improvement['sabotage'] > 0) {
      /* The building is in the city. The probability of successfully
       * sabotaging it as above zero. */
      buttons.push(create_sabotage_impr_button(improvement, id,
                                               actor_unit['id'],
                                               target_city['id']));
    }
  }

  /* Allow the user to cancel. */
  buttons.push({
                 text : 'Cancel',
                 click : function() {
                   $("#" + id).remove();
                 }
               });

  /* Create the dialog. */
  $("#" + id).dialog({
                       modal: true,
                       buttons: buttons,
                       width: "90%"});

  /* Show the dialog. */
  $("#" + id).dialog('open');
}
