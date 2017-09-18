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
var auto_attack = false;

/**************************************************************************
  Returns true iff the given action probability belongs to an action that
  may be possible.
**************************************************************************/
function action_prob_possible(aprob)
{
  return 0 < aprob['max'] || action_prob_not_impl(aprob);
}

/**************************************************************************
  Returns TRUE iff the given action probability represents that support
  for finding this action probability currently is missing from Freeciv.
**************************************************************************/
function action_prob_not_impl(probability)
{
  return probability['min'] == 254
         && probability['max'] == 0;
}

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

  if (-1 == get_direction_for_step(tiles[actor_unit['tile']],
                                   target_tile)) {
    /* The target tile is too far away. */
    return FALSE;
  }

  for (var i = 0; i < tile_units(target_tile).length; i++) {
    tgt_owner_id = unit_owner(tile_units(target_tile)[i])['playerno'];

    if (tgt_owner_id != unit_owner(actor_unit)['playerno']
        && diplstates[tgt_owner_id] != DS_ALLIANCE
        && diplstates[tgt_owner_id] != DS_TEAM) {
      /* Can't move to a non allied foreign unit's tile. */
      return false;
    }
  }

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

/***************************************************************************
  Returns a part of an action probability in a user readable format.
***************************************************************************/
function format_act_prob_part(prob)
{
  return (prob / 2) + "%";
}

/****************************************************************************
  Format the probability that an action will be a success.
****************************************************************************/
function format_action_probability(probability)
{
  if (probability['min'] == probability['max']) {
    /* This is a regular and simple chance of success. */
    return " (" + format_act_prob_part(probability['max']) + ")";
  } else if (probability['min'] < probability['max']) {
    /* This is a regular chance of success range. */
    return " ([" + format_act_prob_part(probability['min']) + ", "
           + format_act_prob_part(probability['max']) + "])";
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
      format_action_probability(action_probabilities[action_id]));
}

/**************************************************************************
  Format the tooltip of an action selection button.
**************************************************************************/
function format_action_tooltip(act_id, act_probs)
{
  var out;

  if (act_probs[act_id]['min'] == act_probs[act_id]['max']) {
    out = "The probability of success is ";
    out += format_act_prob_part(act_probs[act_id]['max']) + ".";
  } else if (act_probs[act_id]['min'] < act_probs[act_id]['max']) {
    out = "The probability of success is ";
    out += format_act_prob_part(act_probs[act_id]['min']);
    out += ", ";
    out += format_act_prob_part(act_probs[act_id]['max']);
    out += " or somewhere in between.";

    if (act_probs[act_id]['max'] - act_probs[act_id]['min'] > 1) {
      /* The interval is wide enough to not be caused by rounding. It is
       * therefore imprecise because the player doesn't have enough
       * information. */
      out += " (This is the most precise interval I can calculate ";
      out += "given the information our nation has access to.)";
    }
  }

  return out;
}

/**************************************************************************
  Returns the function to run when an action is selected.
**************************************************************************/
function act_sel_click_function(parent_id,
                                actor_unit_id, tgt_id, action_id,
                                action_probabilities)
{
  switch (action_id) {
  case ACTION_SPY_TARGETED_STEAL_TECH:
  case ACTION_SPY_TARGETED_STEAL_TECH_ESC:
    return function() {
      popup_steal_tech_selection_dialog(units[actor_unit_id],
                                        cities[tgt_id],
                                        action_probabilities,
                                        action_id);

      $(parent_id).remove();
    };
  case ACTION_SPY_TARGETED_SABOTAGE_CITY:
  case ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC:
  case ACTION_SPY_INCITE_CITY:
  case ACTION_SPY_INCITE_CITY_ESC:
  case ACTION_SPY_BRIBE_UNIT:
  case ACTION_UPGRADE_UNIT:
    return function() {
      var packet = {
        "pid"         : packet_unit_action_query,
        "diplomat_id" : actor_unit_id,
        "target_id"   : tgt_id,
        "action_type" : action_id
      };
      send_request(JSON.stringify(packet));

      $(parent_id).remove();
    };
  case ACTION_FOUND_CITY:
    return function() {
      /* Ask the server to suggest a city name. */
      var packet = {
        "pid"     : packet_city_name_suggestion_req,
        "unit_id" : actor_unit_id
      };
      send_request(JSON.stringify(packet));

      $(parent_id).remove();
    };
  default:
    return function() {
      var packet = {
        "pid"         : packet_unit_do_action,
        "actor_id"    : actor_unit_id,
        "target_id"   : tgt_id,
        "value"       : 0,
        "name"        : "",
        "action_type" : action_id
      };
      send_request(JSON.stringify(packet));

      $(parent_id).remove();
    };
  }
}

/**************************************************************************
  Create a button that selects an action.

  Needed because of JavaScript's scoping rules.
**************************************************************************/
function create_act_sel_button(parent_id,
                               actor_unit_id, tgt_id, action_id,
                               action_probabilities)
{
  /* Create the initial button with this action */
  var button = {
    id      : "act_sel_" + action_id + "_" + actor_unit_id,
    "class" : 'act_sel_button',
    text    : format_action_label(action_id,
                                  action_probabilities),
    title   : format_action_tooltip(action_id,
                                    action_probabilities),
    click   : act_sel_click_function(parent_id,
                                     actor_unit_id, tgt_id, action_id,
                                     action_probabilities)
  };

  /* The button is ready. */
  return button;
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

  var buttons = [];

  var dhtml = "";

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
  } else {
    dhtml += "Your " + unit_types[actor_unit['type']]['name']
             + " is waiting for your command.";
  }

  $(id).html(dhtml);

  /* Show a button for each enabled action. The buttons are sorted by
   * target kind first and then by action id number. */
  for (var tgt_kind = ATK_CITY; tgt_kind < ATK_COUNT; tgt_kind++) {
    var tgt_id = -1;

    switch (tgt_kind) {
    case ATK_CITY:
      if (target_city != null) {
        tgt_id = target_city['id'];
      }
      break;
    case ATK_UNIT:
      if (target_unit != null) {
        tgt_id = target_unit['id'];
      }
      break;
    case ATK_UNITS:
      if (target_tile != null) {
        tgt_id = target_tile['index'];
      }
      break;
    case ATK_TILE:
      if (target_tile != null) {
        tgt_id = target_tile['index'];
      }
      break;
    case ATK_SELF:
      if (actor_unit != null) {
        tgt_id = actor_unit['id'];
      }
      break;
    default:
      console.log("Unsupported action target kind " + tgt_kind);
      break;
    }

    for (var action_id = 0; action_id < ACTION_COUNT; action_id++) {
      if (actions[action_id]['tgt_kind'] == tgt_kind
          && action_prob_possible(
              action_probabilities[action_id])) {
        buttons.push(create_act_sel_button(id, actor_unit['id'], tgt_id,
                                           action_id,
                                           action_probabilities));
      }
    }
  }

  if (can_actor_unit_move(actor_unit, target_tile)) {
    buttons.push({
        id      : "act_sel_move" + actor_unit['id'],
        "class" : 'act_sel_button',
        text    : 'Keep moving',
        click   : function() {
          var dir = get_direction_for_step(tiles[actor_unit['tile']],
                                           target_tile);
          var packet = {
            "pid"       : packet_unit_orders,
            "unit_id"   : actor_unit['id'],
            "src_tile"  : actor_unit['tile'],
            "length"    : 1,
            "repeat"    : false,
            "vigilant"  : false,
            "orders"    : [ORDER_MOVE],
            "dir"       : [dir],
            "activity"  : [ACTIVITY_LAST],
            "target"    : [EXTRA_NONE],
            "action"    : [ACTION_COUNT],
            "dest_tile" : target_tile['index']
          };

          if (dir == -1) {
            /* Non adjacent target tile? */
            console.log("Action slection move: bad target tile");
          } else {
            send_request(JSON.stringify(packet));
          }

          $(id).remove();
        } });
  }

  if (target_unit != null
      && tile_units(target_tile).length > 1) {
    buttons.push({
        id      : "act_sel_tgt_unit_switch" + actor_unit['id'],
        "class" : 'act_sel_button',
        text    : 'Change unit target',
        click   : function() {
          select_tgt_unit(actor_unit,
                          target_tile, tile_units(target_tile));

          $(id).remove();
        } });
  }

  /* Special-case handling for auto-attack. */
  if (action_prob_possible(action_probabilities[ACTION_ATTACK])) {
        if (!auto_attack) {
          var button = {
            id      : "act_sel_" + ACTION_ATTACK + "_" + actor_unit['id'],
            "class" : 'act_sel_button',
            text    : "Auto attack from now on!",
            title   : "Attack without showing this attack dialog in the future",
            click   : function() {
                          var packet = {
                              "pid"         : packet_unit_do_action,
                              "actor_id"    : actor_unit['id'],
                              "target_id"   : target_tile['index'],
                              "value"       : 0,
                              "name"        : "",
                              "action_type" : ACTION_ATTACK
                            };
                            send_request(JSON.stringify(packet));
                            auto_attack = true;
                            $(id).remove();
                          }
          };
          buttons.push(button);
        } else {
          var packet = {
              "pid"         : packet_unit_do_action,
              "actor_id"    : actor_unit['id'],
              "target_id"   : target_tile['index'],
              "value"       : 0,
              "name"        : "",
              "action_type" : ACTION_ATTACK
            };
            send_request(JSON.stringify(packet));
            return;
        }
  }

  buttons.push({
      id      : "act_sel_cancel" + actor_unit['id'],
      "class" : 'act_sel_button',
      text    : 'Cancel',
      click   : function() {
        $(id).remove();
      } });

  $(id).attr("title",
             "Choose Your " + unit_types[actor_unit['type']]['name']
             + "'s Strategy");
  $(id).dialog({
      bgiframe: true,
      modal: true,
      dialogClass: "act_sel_dialog",
      width: "390",
      buttons: buttons });

  $(id).dialog('open');
}

/**************************************************************************
  Show the player the price of bribing the unit and, if bribing is
  possible, allow him to order it done.
**************************************************************************/
function popup_bribe_dialog(actor_unit, target_unit, cost, act_id)
{
  var bribe_possible = false;
  var dhtml = "";
  var id = "#bribe_unit_dialog_" + actor_unit['id'];

  /* Reset dialog page. */
  $(id).remove();

  $("<div id='bribe_unit_dialog_" + actor_unit['id'] + "'></div>")
      .appendTo("div#game_page");

  dhtml += "Treasury contains " + unit_owner(actor_unit)['gold'] + " gold. ";
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
                    "action_type": act_id};
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
function popup_incite_dialog(actor_unit, target_city, cost, act_id)
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

  dhtml += "Treasury contains " + unit_owner(actor_unit)['gold'] + " gold.";
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
                                               "action_type": act_id};
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
  Show the player the price of upgrading the unit and, if upgrading is
  affordable, allow him to order it done.
**************************************************************************/
function popup_unit_upgrade_dlg(actor_unit, target_city, cost, act_id)
{
  var upgrade_possible;
  var id;
  var dhtml;

  id = "#upgrade_unit_dialog_" + actor_unit['id'];

  /* Reset dialog page. */
  $(id).remove();

  $("<div id='upgrade_unit_dialog_" + actor_unit['id'] + "'></div>")
      .appendTo("div#game_page");

  dhtml = "";

  dhtml += "Treasury contains " + unit_owner(actor_unit)['gold'] + " gold.";
  dhtml += " ";
  dhtml += "The price of upgrading our "
           + unit_types[actor_unit['type']]['name']
           + " is " + cost + ".";

  upgrade_possible = cost <= unit_owner(actor_unit)['gold'];

  $(id).html(dhtml);

  var close_button = {          Close:    function() {$(id).dialog('close');}};
  var upgrade_close_buttons = { 'Cancel': function() {$(id).dialog('close');},
                                'Do it!': function() {
                                  var packet = {
                                    "pid" : packet_unit_do_action,
                                    "actor_id" : actor_unit['id'],
                                    "target_id": target_city['id'],
                                    "value" : 0,
                                    "name" : "",
                                    "action_type": act_id
                                  };
                                  send_request(JSON.stringify(packet));

                                  $(id).dialog('close');
                                }
                             };

  $(id).attr("title", "Unit upgrade");

  $(id).dialog({bgiframe: true,
                modal: true,
                buttons: (upgrade_possible ? upgrade_close_buttons
                                           : close_button),
                height: "auto",
                width: "auto"});

  $(id).dialog('open');
}

/**************************************************************************
  Create a button that steals a tech.

  Needed because of JavaScript's scoping rules.
**************************************************************************/
function create_steal_tech_button(parent_id, tech,
                                  actor_unit_id, target_city_id,
                                  action_id)
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
        "action_type": action_id};

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
                                           act_probs, action_id)
{
  var id = "stealtech_dialog_" + actor_unit['id'];
  var buttons = [];
  var untargeted_action_id = ACTION_COUNT;

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
                && (act_kn == TECH_UNKNOWN)))) {
      /* Add a button for stealing this tech to the dialog. */
      buttons.push(create_steal_tech_button(id, tech,
                                            actor_unit['id'],
                                            target_city['id'],
                                            action_id));
    }
  }

  /* The player may change his mind after selecting targeted tech theft and
   * go for the untargeted version after concluding that no listed tech is
   * worth the extra risk. */
  if (action_id == ACTION_SPY_TARGETED_STEAL_TECH_ESC) {
    untargeted_action_id = ACTION_SPY_STEAL_TECH_ESC;
  } else if (action_id == ACTION_SPY_TARGETED_STEAL_TECH) {
    untargeted_action_id = ACTION_SPY_STEAL_TECH;
  }

  if (untargeted_action_id != ACTION_COUNT
      && action_prob_possible(
           act_probs[untargeted_action_id])) {
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
                       "action_type": untargeted_action_id};
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
                                     actor_unit_id, target_city_id, act_id)
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
        "action_type"  : act_id
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
function popup_sabotage_dialog(actor_unit, target_city, city_imprs, act_id)
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
                                               target_city['id'],
                                               act_id));
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

/**************************************************************************
  Create a button that selects a target unit.

  Needed because of JavaScript's scoping rules.
**************************************************************************/
function create_select_tgt_unit_button(parent_id, actor_unit_id,
                                       target_tile_id, target_unit_id)
{
  var text = "";
  var target_unit = units[target_unit_id];
  var button = {};

  text += unit_types[target_unit['type']]['name'];

  if (get_unit_homecity_name(target_unit) != null) {
    text += " from " + get_unit_homecity_name(target_unit);
  }

  text += " (";
  text += nations[unit_owner(target_unit)['nation']]['adjective'];
  text += ")";

  button = {
    text  : text,
    click : function() {
      var packet = {
        "pid"            : packet_unit_get_actions,
        "actor_unit_id"  : actor_unit_id,
        "target_unit_id" : target_unit_id,
        "target_tile_id" : target_tile_id,
        "disturb_player" : true
      };
      send_request(JSON.stringify(packet));

      $(parent_id).remove();
    }
  };

  /* The button is ready. */
  return button;
}

/**************************************************************************
  Create a dialog where a unit select what other unit to act on.
**************************************************************************/
function select_tgt_unit(actor_unit, target_tile, potential_tgt_units)
{
  var i;

  var rid     = "sel_tgt_unit_dialog_" + actor_unit['id'];
  var id      = "#" + rid;
  var dhtml   = "";
  var buttons = [];

  /* Reset dialog page. */
  $(id).remove();
  $("<div id='" + rid + "'></div>").appendTo("div#game_page");

  dhtml += "Select target unit for your ";
  dhtml += unit_types[actor_unit['type']]['name'];

  $(id).html(dhtml);

  for (i = 0; i < potential_tgt_units.length; i++) {
    var tgt_unit = potential_tgt_units[i];

    buttons.push(create_select_tgt_unit_button(id, actor_unit['id'],
                                               target_tile['index'],
                                               tgt_unit['id']));
  }

  $(id).dialog({
      title    : "Target unit selection",
      bgiframe : true,
      modal    : true,
      buttons  : buttons });

  $(id).dialog('open');
}
