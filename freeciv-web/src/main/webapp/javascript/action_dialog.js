/********************************************************************** 
 Freeciv - Copyright (C) 2009-2010 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/



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
  } else {
    dhtml += "The " + unit_types[actor_unit['type']]['name']
             + " is waiting for your command";
  }

  dhtml += "<br>";

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    dhtml += "<input id='act_sel_emb" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='Establish Embassy'>";
  }
  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    dhtml += "<input id='act_sel_inv" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Investigate City'>"
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='act_sel_sab" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Sabotage City'>"
  }
  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    dhtml += "<input id='act_sel_tech" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Steal Technology'>"
  }
  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    dhtml += "<input id='act_sel_revo" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Incite a revolt'>"
  }
  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    dhtml += "<input id='act_sel_poi" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Poison city'>"
  }
  if (actor_unit['caravan_trade']) {
    dhtml += "<input id='act_sel_trade" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Establish Traderoute'>"
  }
  if (actor_unit['caravan_wonder']) {
    dhtml += "<input id='act_sel_wonder" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Help build Wonder'>"
  }
  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    dhtml += "<input id='act_sel_bribe" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Bribe enemy unit'>"
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    dhtml += "<input id='act_sel_spy_sabo" + actor_unit['id']
    + "' class='act_sel_button' type='button' value='Sabotage enemy unit'>"
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
  $(".act_sel_button").css("width", "250px");
  

  $("#act_sel_cancel" + actor_unit['id']).click(function() {
    $(id).remove();		
  });

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    $("#act_sel_emb" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
                     "actor_id" : actor_unit['id'],
                     "target_id": target_city['id'],
                     "value" : 0,
                     "action_type": ACTION_ESTABLISH_EMBASSY};
      send_request (JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    $("#act_sel_inv" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_city['id'],
        "value" : 0,
        "action_type": ACTION_SPY_INVESTIGATE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    $("#act_sel_sab" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_city['id'],
        "value" : 0,
        "action_type": ACTION_SPY_SABOTAGE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  var last_tech = 0;

  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    $("#act_sel_tech" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action, 
                     "actor_id" : actor_unit['id'],
                     "target_id": target_city['id'],
                     "value" : last_tech, 
                     "action_type": ACTION_SPY_STEAL_TECH};
      send_request (JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    $("#act_sel_revo" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_city['id'],
        "value" : 0,
        "action_type": ACTION_SPY_INCITE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    $("#act_sel_poi" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_city['id'],
        "value" : 0,
        "action_type": ACTION_SPY_POISON};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (actor_unit['caravan_trade']) {
    $("#act_sel_trade" + actor_unit['id']).click(function() {
        var packet = {"type" : packet_unit_establish_trade,
                       "unit_id": actor_unit['id'],
                       "city_id" : target_city['id'],
                       "est_if_able" : true};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (actor_unit['caravan_wonder']) {
    $("#act_sel_wonder" + actor_unit['id']).click(function() {
        var packet = {"type" : packet_unit_help_build_wonder,
                       "unit_id": actor_unit['id'],
                       "city_id" : target_city['id']};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    $("#act_sel_bribe" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_unit['id'],
        "value" : 0,
        "action_type": ACTION_SPY_BRIBE_UNIT};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    $("#act_sel_spy_sabo" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_unit['id'],
        "value" : 0,
        "action_type": ACTION_SPY_SABOTAGE_UNIT};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (can_actor_unit_move(actor_unit, target_tile)) {
    $("#act_sel_move" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_tile['index'],
        "value" : 0,
        "action_type": ACTION_MOVE};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }
}

