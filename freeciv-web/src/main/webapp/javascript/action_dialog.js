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
             + "Establish Embassy"
             + format_action_probability(action_probabilities[ACTION_ESTABLISH_EMBASSY])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_INVESTIGATE_CITY] != 0) {
    dhtml += "<input id='act_sel_inv" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Investigate City"
             + format_action_probability(action_probabilities[ACTION_SPY_INVESTIGATE_CITY])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='act_sel_sab" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Sabotage City"
             + format_action_probability(action_probabilities[ACTION_SPY_SABOTAGE_CITY])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_TARGETED_SABOTAGE_CITY] != 0) {
    dhtml += "<input id='act_sel_sab_tgt" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Industrial Sabotage"
             + format_action_probability(action_probabilities[ACTION_SPY_TARGETED_SABOTAGE_CITY])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_STEAL_TECH] != 0) {
    dhtml += "<input id='act_sel_tech" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Steal Technology"
             + format_action_probability(action_probabilities[ACTION_SPY_STEAL_TECH])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_TARGETED_STEAL_TECH] != 0) {
    dhtml += "<input id='act_sel_tech_tgt" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Industrial Espionage"
             + format_action_probability(action_probabilities[ACTION_SPY_TARGETED_STEAL_TECH])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_STEAL_GOLD] != 0) {
    dhtml += "<input id='act_sel_steal_gold" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Steal Gold"
             + format_action_probability(action_probabilities[ACTION_SPY_STEAL_GOLD])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    dhtml += "<input id='act_sel_revo" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Incite a revolt"
             + format_action_probability(action_probabilities[ACTION_SPY_INCITE_CITY])
             + "'>";
  }
  if (action_probabilities[ACTION_SPY_POISON] != 0) {
    dhtml += "<input id='act_sel_poi" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Poison city"
             + format_action_probability(action_probabilities[ACTION_SPY_POISON])
             + "'>";
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
    + "' class='act_sel_button' type='button' value='"
        + "Bribe enemy unit"
        + format_action_probability(action_probabilities[ACTION_SPY_BRIBE_UNIT])
        + "'>";
  }
  if (action_probabilities[ACTION_SPY_SABOTAGE_UNIT] != 0) {
    dhtml += "<input id='act_sel_spy_sabo" + actor_unit['id']
             + "' class='act_sel_button' type='button' value='"
             + "Sabotage enemy unit"
             + format_action_probability(action_probabilities[ACTION_SPY_SABOTAGE_UNIT])
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

  if (action_probabilities[ACTION_SPY_TARGETED_SABOTAGE_CITY] != 0) {
    $("#act_sel_sab_tgt" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_city['id'],
        /* FIXME: Let the player choose the target. */
        "value" : B_LAST,
        "action_type": ACTION_SPY_TARGETED_SABOTAGE_CITY};
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

  if (action_probabilities[ACTION_SPY_TARGETED_STEAL_TECH] != 0) {
    $("#act_sel_tech_tgt" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
                     "actor_id" : actor_unit['id'],
                     "target_id": target_city['id'],
                     /* FIXME: Let the player choose the target. */
                     "value" : A_UNSET,
                     "action_type": ACTION_SPY_TARGETED_STEAL_TECH};
      send_request (JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_STEAL_GOLD] != 0) {
    $("#act_sel_steal_gold" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
                     "actor_id" : actor_unit['id'],
                     "target_id": target_city['id'],
                     "value" : A_UNSET,
                     "action_type": ACTION_SPY_STEAL_GOLD};
      send_request (JSON.stringify(packet));

      $(id).remove();
    });
  }

  if (action_probabilities[ACTION_SPY_INCITE_CITY] != 0) {
    $("#act_sel_revo" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_action_query,
        "diplomat_id" : actor_unit['id'],
        "target_id": target_city['id'],
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
      var packet = {"type" : packet_unit_action_query,
        "diplomat_id" : actor_unit['id'],
        "target_id": target_unit['id'],
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
  var bribe_close_button = {	Close: function() {$(id).dialog('close');},
  				Bribe: function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_unit['id'],
        "value" : 0,
        "action_type": ACTION_SPY_BRIBE_UNIT};
        send_request (JSON.stringify(packet));
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

  if (incite_possible) {
    dhtml += "<br>";
    dhtml += "<input id='incite_city_dialog_incite" + actor_unit['id']
             + "' class='incite_city_button' type='button' value='Do it!'>";
  } else {
    dhtml += " ";
    dhtml += "Traitors Demand Too Much!";
    dhtml += "<br>";
  }

  dhtml += "<input id='incite_city_dialog_cancel" + actor_unit['id']
           + "' class='incite_city_button' type='button' value='Cancel'>";

  $(id).html(dhtml);

  $(id).attr("title", "About that incite you requested...");

  $(id).dialog({bgiframe: true,
                modal: true,
                height: "auto",
                width: "auto"});

  $(id).dialog('open');
  $(".incite_city_button").button();

  if (incite_possible) {
    $("#incite_city_dialog_incite" + actor_unit['id']).click(function() {
      var packet = {"type" : packet_unit_do_action,
        "actor_id" : actor_unit['id'],
        "target_id": target_city['id'],
        "value" : 0,
        "action_type": ACTION_SPY_INCITE_CITY};
        send_request (JSON.stringify(packet));

        $(id).remove();
    });
  }

  $("#incite_city_dialog_cancel" + actor_unit['id']).click(function() {
    $(id).remove();
  })
}
