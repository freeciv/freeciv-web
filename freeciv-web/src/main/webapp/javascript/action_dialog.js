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

  if (index_to_tile(pdiplomat['tile']) == target_tile) {
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

/****************************************************************************
 ...
****************************************************************************/
function popup_caravan_dialog(punit, traderoute, wonder)
{
  var id = "#caravan_dialog_" + punit['id'];
  $(id).remove();
  $("<div id='caravan_dialog_" + punit['id'] + "'></div>").appendTo("div#game_page");

  var homecity = cities[punit['homecity']];
  var ptile = index_to_tile(punit['tile']);
  var pcity = tile_city(ptile);


  var dhtml = "<center>Your caravan from " + decodeURIComponent(homecity['name']) + " reaches the city of "
        + decodeURIComponent(pcity['name']) + ". What now? <br>"
        + "<input id='car_trade" + punit['id']
        + "' class='car_button' type='button' value='Establish Traderoute'>"
        + "<input id='car_wonder" + punit['id']
        + "' class='car_button' type='button' value='Help build Wonder'>"
        + "<input id='car_cancel" + punit['id']
        + "' class='car_button' type='button' value='Cancel'>"
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


  if (!traderoute) $("#car_trade" + punit['id']).button( "option", "disabled", true);
  if (!wonder) $("#car_wonder" + punit['id']).button( "option", "disabled", true);

  $("#car_trade" + punit['id']).click(function() {
    var packet = {"type" : packet_unit_establish_trade,
                   "unit_id": punit['id'],
                   "city_id" : pcity['id']};
    send_request (JSON.stringify(packet));

    $(id).remove();
  });

  $("#car_wonder" + punit['id']).click(function() {
    var packet = {"type" : packet_unit_help_build_wonder,
                   "unit_id": punit['id'],
                   "city_id" : pcity['id']};
    send_request (JSON.stringify(packet));

    $(id).remove();
  });

  $("#car_cancel" + punit['id']).click(function() {
    $(id).remove();
  });

}

function popup_diplomat_dialog(pdiplomat, action_probabilities,
                               target_tile, punit, pcity)
{
 // reset dialog page.
  var id = "#diplo_dialog_" + pdiplomat['id'];
  $(id).remove();
  $("<div id='diplo_dialog_" + pdiplomat['id'] + "'></div>").appendTo("div#game_page");

  var actor_homecity = cities[pdiplomat['homecity']];

  var dhtml = "<center>";

  if (pcity != null) {
    dhtml += "Your " + unit_types[pdiplomat['type']]['name'];

    /* Some units don't have a home city. */
    if (actor_homecity != null) {
      dhtml += " from " + decodeURIComponent(actor_homecity['name']);
    }

    dhtml += " has arrived at " + decodeURIComponent(pcity['name'])
             + ". What is your command?";
  } else {
    dhtml += "The " + unit_types[pdiplomat['type']]['name']
             + " is waiting for your command";
  }

  dhtml += "<br>";

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    dhtml += "<input id='diplo_emb" + pdiplomat['id']
             + "' class='diplo_button' type='button' value='Establish Embassy'>";
  }
  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    dhtml += "<input id='diplo_inv" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Investigate City'>"
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='diplo_sab" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Sabotage City'>"
  }
  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    dhtml += "<input id='diplo_tech" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Steal Technology'>"
  }
  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    dhtml += "<input id='diplo_revo" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Incite a revolt'>"
  }
  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    dhtml += "<input id='diplo_poi" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Poison city'>"
  }
  if (action_probabilities[ACTION_SPY_BRIBE_UNIT] != 0) {
    dhtml += "<input id='diplo_bribe" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Bribe enemy unit'>"
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    dhtml += "<input id='diplo_spy_sabo" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Sabotage enemy unit'>"
  }
  if (can_actor_unit_move(pdiplomat, target_tile)) {
    dhtml += "<input id='diplo_move" + pdiplomat['id']
    + "' class='diplo_button' type='button' value='Keep moving'>"
  }

  dhtml += "<input id='diplo_cancel" + pdiplomat['id']
          + "' class='diplo_button' type='button' value='Cancel'>"
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
  

  $("#diplo_cancel" + pdiplomat['id']).click(function() {
    $(id).remove();		
  });

  if (action_probabilities[ACTION_ESTABLISH_EMBASSY] != 0) {
    $("#diplo_emb" + pdiplomat['id']).click(function() {
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
    $("#diplo_inv" + pdiplomat['id']).click(function() {
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
    $("#diplo_sab" + pdiplomat['id']).click(function() {
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
    $("#diplo_tech" + pdiplomat['id']).click(function() {
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
    $("#diplo_revo" + pdiplomat['id']).click(function() {
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
    $("#diplo_poi" + pdiplomat['id']).click(function() {
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
    $("#diplo_bribe" + pdiplomat['id']).click(function() {
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
    $("#diplo_spy_sabo" + pdiplomat['id']).click(function() {
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
    $("#diplo_move" + pdiplomat['id']).click(function() {
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

