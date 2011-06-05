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



function popup_diplomat_dialog(pdiplomat, punit, pcity)
{
 // reset dialog page.
  var id = "#diplo_dialog_" + pdiplomat['id'];
  $(id).remove();
  $("<div id='diplo_dialog_" + pdiplomat['id'] + "'></div>").appendTo("div#game_page");

  if (pcity != null) {
    var dhtml = "<center>Your didplomat has arrived at " + pcity['name'] + ". What is your command?<br>"
	    + "<input id='diplo_emb' class='diplo_button' type='button' value='Establish Embassy'>"
	    + "<input id='diplo_inv' class='diplo_button' type='button' value='Investigate City'>"
	    + "<input id='diplo_sab' class='diplo_button' type='button' value='Sabotage City'>"
	    + "<input id='diplo_tech' class='diplo_button' type='button' value='Steal Technology'>"
	    + "<input id='diplo_revo' class='diplo_button' type='button' value='Incite a revolt'>"
	    + "<input id='diplo_cancel' class='diplo_button' type='button' value='Cancel'>"
	    + "</center>"
    $(id).html(dhtml);
  } else {
    var dhtml = "<center>The diplomat is waiting for your command"
	    + "<input id='diplo_bribe' class='diplo_button' type='button' value='Bribe enemy unit'>"
	    + "<input id='diplo_spy_sabo' class='diplo_button' type='button' value='Sabotage enemy unit'>"
	    + "<input id='diplo_cancel' class='diplo_button' type='button' value='Cancel'>"
	    + "</center>";	  
    $(id).html(dhtml);
  }

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

  $("#diplo_emb").click(function() {
    var packet = {"type" : packet_unit_diplomat_action, 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": pcity['id'], 
                   "value" : 0, 
                   "action_type": DIPLOMAT_EMBASSY};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });

  $("#diplo_inv").click(function() {
    var packet = {"type" : packet_unit_diplomat_action, 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": pcity['id'], 
                   "value" : 0, 
                   "action_type": DIPLOMAT_INVESTIGATE};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });


  $("#diplo_sab").click(function() {
    var packet = {"type" : packet_unit_diplomat_action, 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": pcity['id'], 
                   "value" : 0, 
                   "action_type": DIPLOMAT_SABOTAGE};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });

  var tech_count_counterpart = 0;
  var pplayer = client.conn.playing;
  var counterpart = city_owner(pcity);
  var last_tech = 0;
  for (var tech_id in techs) {
    if (player_invention_state(counterpart, tech_id) == TECH_KNOWN
        // && player_invention_reachable(pother, i)
        && (player_invention_state(pplayer, tech_id) == TECH_UNKNOWN
            || player_invention_state(pplayer, tech_id) == TECH_PREREQS_KNOWN)) {
      last_tech = tech_id;
      tech_count_counterpart += 1;
    }
  }

  if (tech_count_counterpart > 0) {
    $("#diplo_tech").click(function() {
      var packet = {"type" : packet_unit_diplomat_action, 
                     "diplomat_id" : pdiplomat['id'], 
                     "target_id": pcity['id'], 
                     "value" : last_tech, 
                     "action_type": DIPLOMAT_STEAL};
      send_request (JSON.stringify(packet));

      $(id).remove();	
    });
  } else {
    $("#diplo_tech").button( "option", "disabled", true);
  }

  $("#diplo_revo").click(function() {
    var packet = {"type" : packet_unit_diplomat_action, 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": pcity['id'], 
                   "value" : 0, 
                   "action_type": DIPLOMAT_INCITE};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });


  $("#diplo_bribe").click(function() {
    var packet = {"type" : packet_unit_diplomat_action, 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": punit['id'], 
                   "value" : 0, 
                   "action_type": DIPLOMAT_BRIBE};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });
  
  $("#diplo_spy_sabo").click(function() {
    var packet = {"type" : packet_unit_diplomat_action, 
                   "diplomat_id" : pdiplomat['id'], 
                   "target_id": punit['id'], 
                   "value" : 0, 
                   "action_type": SPY_SABOTAGE_UNIT};
    send_request (JSON.stringify(packet));		  

    $(id).remove();	
  });

  if (punit != null) {
    var ptype = unit_type(punit);
    if (ptype['name'] != "Spy") {
      $("#diplo_spy_sabo").button("option", "disabled", true);
    }

  }
}

