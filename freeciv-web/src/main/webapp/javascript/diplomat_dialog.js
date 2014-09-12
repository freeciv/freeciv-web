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
function can_actor_unit_move(pdiplomat, target_tile)
{
  var tgt_owner_id;

  if (pdiplomat['tile'] == target_tile) {
    /* The unit is already on this tile. */
    return false;
  }

  for (var i = 0; i < tile_units(target_tile).length; i++) {
    tgt_owner_id = unit_owner(tile_units(target_tile)[i])['playerno'];

    if (tgt_owner_id != unit_owner(pdiplomat)['playerno']
        && diplstates[tgt_owner_id] != DS_ALLIANCE
        && diplstates[tgt_owner_id] != DS_TEAM) {
      /* In current Freeciv acting units can't attack foreign units. */
      return false;
    }
  };

  if (tile_city(target_tile) != null) {
    tgt_owner_id = city_owner(tile_city(target_tile))['playerno'];

    if (tgt_owner_id == unit_owner(pdiplomat)['playerno']) {
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

function popup_diplomat_dialog(pdiplomat, action_probabilities,
                               target_tile, punit, pcity)
{
 // reset dialog page.
  var id = "#diplo_dialog_" + pdiplomat['id'];
  $(id).remove();
  $("<div id='diplo_dialog_" + pdiplomat['id'] + "'></div>").appendTo("div#game_page");

  var dhtml = "<center>";

  if (pcity != null) {
    dhtml += "Your didplomat has arrived at " + pcity['name'] + ". What is your command?";
  } else {
    dhtml += "The diplomat is waiting for your command";
  }

  dhtml += "<br>";

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    dhtml += "<input id='diplo_emb' class='diplo_button' type='button' value='Establish Embassy'>";
  }
  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    dhtml += "<input id='diplo_inv' class='diplo_button' type='button' value='Investigate City'>"
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='diplo_sab' class='diplo_button' type='button' value='Sabotage City'>"
  }
  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    dhtml += "<input id='diplo_tech' class='diplo_button' type='button' value='Steal Technology'>"
  }
  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    dhtml += "<input id='diplo_revo' class='diplo_button' type='button' value='Incite a revolt'>"
  }
  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    dhtml += "<input id='diplo_poi' class='diplo_button' type='button' value='Poison city'>"
  }
  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    dhtml += "<input id='diplo_bribe' class='diplo_button' type='button' value='Bribe enemy unit'>"
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    dhtml += "<input id='diplo_spy_sabo' class='diplo_button' type='button' value='Sabotage enemy unit'>"
  }
  if (can_actor_unit_move(pdiplomat, target_tile)) {
    dhtml += "<input id='diplo_move' class='diplo_button' type='button' value='Keep moving'>"
  }

  dhtml += "<input id='diplo_cancel' class='diplo_button' type='button' value='Cancel'>"
         + "</center>";
  $(id).html(dhtml);

  $(id).attr("title", "Choose Your Diplomat's Strategy");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: "350"});
	
  $(id).dialog('open');		
  $(".diplo_button").button();
  $(".diplo_button").css("width", "250px");
  

  $("#diplo_cancel").click(function() {
    $(id).remove();		
  });

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    $("#diplo_emb").click(function() {
      var packet = {"type" : packet_unit_do_action,
                     "actor_id" : pdiplomat['id'],
                     "target_id": pcity['id'],
                     "value" : 0,
                     "action_type": ACTION_ESTABLISH_EMBASSY};
      send_request (JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    $("#diplo_inv").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": pcity['id'],
        "value" : 0,
        "action_type": ACTION_SPY_INVESTIGATE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    $("#diplo_sab").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": pcity['id'],
        "value" : 0,
        "action_type": ACTION_SPY_SABOTAGE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  var last_tech = 0;

  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    $("#diplo_tech").click(function() {
      var packet = {"type" : packet_unit_do_action, 
                     "actor_id" : pdiplomat['id'], 
                     "target_id": pcity['id'], 
                     "value" : last_tech, 
                     "action_type": ACTION_SPY_STEAL_TECH};
      send_request (JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    $("#diplo_revo").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": pcity['id'],
        "value" : 0,
        "action_type": ACTION_SPY_INCITE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    $("#diplo_poi").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": pcity['id'],
        "value" : 0,
        "action_type": ACTION_SPY_POISON};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    $("#diplo_bribe").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": punit['id'],
        "value" : 0,
        "action_type": ACTION_SPY_BRIBE_UNIT};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    $("#diplo_spy_sabo").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": punit['id'],
        "value" : 0,
        "action_type": ACTION_SPY_SABOTAGE_UNIT};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  if (can_actor_unit_move(pdiplomat, target_tile)) {
    $("#diplo_move").click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : pdiplomat['id'],
        "target_id": target_tile['index'],
        "value" : 0,
        "action_type": ACTION_MOVE};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }
}

