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



function popup_diplomat_dialog(pdiplomat, action_probabilities,
                               punit, pcity)
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
}

