/********************************************************************** 
 Freeciv - Copyright (C) 2009-2020 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

var CLAUSE_ADVANCE = 0;
var CLAUSE_GOLD = 1;
var CLAUSE_MAP = 2;
var CLAUSE_SEAMAP = 3;
var CLAUSE_CITY = 4;
var CLAUSE_CEASEFIRE = 5;
var CLAUSE_PEACE = 6;
var CLAUSE_ALLIANCE = 7;
var CLAUSE_VISION = 8;
var CLAUSE_EMBASSY = 9;

var diplomacy_request_queue = [];
var diplomacy_clause_map = {};
var active_diplomacy_meeting_id = null;

/**************************************************************************
 ...
**************************************************************************/
function diplomacy_init_meeting_req(counterpart)
{
  var packet = [{"packet_type" : "diplomacy_init_meeting_req", 
	         "counterpart" : counterpart}];
  send_request (JSON.stringify(packet));

}


/**************************************************************************
 ...
**************************************************************************/
function refresh_diplomacy_request_queue()
{
  if (diplomacy_request_queue.length > 0) {
    var next = diplomacy_request_queue[0];
    if (next != null && next != active_diplomacy_meeting_id) {
        active_diplomacy_meeting_id = next;
      	show_diplomacy_dialog(active_diplomacy_meeting_id);
	show_diplomacy_clauses();
    }
  }

}

/**************************************************************************
 ...
**************************************************************************/
function show_diplomacy_dialog(counterpart)
{
 var pplayer = players[counterpart];
 create_diplomacy_dialog(pplayer);
 
}

/**************************************************************************
 ...
**************************************************************************/
function accept_treaty_req()
{

  var packet = [{"packet_type" : "diplomacy_accept_treaty_req", 
	         "counterpart" : active_diplomacy_meeting_id}];
  send_request (JSON.stringify(packet));

}

/**************************************************************************
 ...
**************************************************************************/
function accept_treaty(counterpart, I_accepted, other_accepted)
{
  if (active_diplomacy_meeting_id == counterpart 
      && I_accepted == true && other_accepted == true) {
    $("#diplomacy_dialog").remove(); //close the dialog.
  
    if (diplomacy_request_queue.indexOf(counterpart) >= 0) {
      diplomacy_request_queue.splice(diplomacy_request_queue.indexOf(counterpart), 1);
    }
  } else if (active_diplomacy_meeting_id == counterpart) {

  var agree_sprite = get_treaty_agree_thumb_up();
  var disagree_sprite = get_treaty_disagree_thumb_down();


  var agree_self_html = "<div id='flag_self' style='float:right; background: transparent url("
           + agree_sprite['image-src'] 
           + "); background-position:-" + agree_sprite['tileset-x'] + "px -" 
	   + agree_sprite['tileset-y'] 
           + "px;  width: " + agree_sprite['width'] + "px;height: " 
	   + agree_sprite['height'] + "px; margin: 5px; '>"
           + "</div>";
  var disagree_self_html = "<div id='flag_self' style='float:right; background: transparent url("
           + disagree_sprite['image-src'] 
           + "); background-position:-" + disagree_sprite['tileset-x'] + "px -" 
	   + disagree_sprite['tileset-y'] 
           + "px;  width: " + disagree_sprite['width'] + "px;height: " 
	   + disagree_sprite['height'] + "px; margin: 5px; '>"
           + "</div>";
    if (I_accepted == true) {
      $("#agree_self").html(agree_self_html);
    } else {
      $("#agree_self").html(disagree_self_html);
    }

    if (other_accepted) {
      $("#agree_counterpart").html(agree_self_html);
    } else {
      $("#agree_counterpart").html(disagree_self_html);
    }
  }

  setTimeout("refresh_diplomacy_request_queue();", 1000);

}

/**************************************************************************
 ...
**************************************************************************/
function cancel_meeting_req()
{
  var packet = [{"packet_type" : "diplomacy_cancel_meeting_req", 
	         "counterpart" : active_diplomacy_meeting_id}];
  send_request (JSON.stringify(packet));

}

/**************************************************************************
 ...
**************************************************************************/
function create_clause_req(giver, type, value)
{
  var packet = [{"packet_type" : "diplomacy_create_clause_req", 
	         "counterpart" : active_diplomacy_meeting_id,
                 "giver" : giver,
                 "type" : type,
                 "value" : value}];
  send_request (JSON.stringify(packet));

}


/**************************************************************************
 ...
**************************************************************************/
function cancel_meeting(counterpart)
{
  if (active_diplomacy_meeting_id == counterpart) {
    $("#diplomacy_dialog").remove(); //close the dialog.
    active_diplomacy_meeting_id = null;
  }

  if (diplomacy_request_queue.indexOf(counterpart) >= 0) {
    diplomacy_request_queue.splice(diplomacy_request_queue.indexOf(counterpart), 1);
  }

  setTimeout("refresh_diplomacy_request_queue();", 1000);
}

/**************************************************************************
 ...
**************************************************************************/
function show_diplomacy_clauses()
{
  if (active_diplomacy_meeting_id != null) {
    var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
    var diplo_html = "";
    for (var i = 0; i < clauses.length; i++) {
      var clause = clauses[i];
      var diplo_str = client_diplomacy_clause_string(clause['counterpart'], 
 		          clause['giver'],
		  	  clause['type'],
			  clause['value']);
      diplo_html += "<a href='#' onclick='remove_clause_req(" + i + ");'>" + diplo_str + "</a><br>";
	
    }
  
    $("#diplomacy_messages").html(diplo_html);
  }

}

/**************************************************************************
 ...
**************************************************************************/
function remove_clause_req(clause_no) 
{
  var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
  var clause = clauses[clause_no];
  
  var packet = [{"packet_type" : "diplomacy_remove_clause_req", 
	         "counterpart" : clause['counterpart'],
                 "giver": clause['giver'],
                 "type" : clause['type'],
                 "value": clause['value'] }];
  send_request (JSON.stringify(packet));

}

/**************************************************************************
 ...
**************************************************************************/
function remove_clause(remove_clause) 
{	
  var clause_list = diplomacy_clause_map[remove_clause['counterpart']];

  for (var i = 0; i < clause_list.length; i++) {
    var check_clause = clause_list[i];
    if (remove_clause['counterpart'] == check_clause['counterpart']
	&& remove_clause['giver'] == check_clause['giver']
	&& remove_clause['type'] == check_clause['type']) {

      clause_list.splice(i, 1);
      break;
    }
  }

  show_diplomacy_clauses();
}

/**************************************************************************
 ...
**************************************************************************/
function client_diplomacy_clause_string(counterpart, giver, type, value)
{
  
  switch (type) {
    case CLAUSE_ADVANCE:
      var pplayer = players[giver];
      var nation = nations[pplayer['nation']]['adjective']
      var ptech = techs[value];
      return "The " + nation + " give " + ptech['name'];
    break;
  case CLAUSE_CITY:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    var pcity = cities[value];

    if (pcity != null) {
      return "The " + nation + " give " + unescape(pcity['name']);
    } else {
      return "The " + nation + " give unknown city.";
    }

    break;
  case CLAUSE_GOLD:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give " + value + " gold";
    break;
  case CLAUSE_MAP:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give their worldmap";
    break;
  case CLAUSE_SEAMAP:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give their seamap";
    break;
  case CLAUSE_CEASEFIRE:
    return "The parties agree on a cease-fire";
    break;
  case CLAUSE_PEACE:
    return "The parties agree on a peace";
    break;
  case CLAUSE_ALLIANCE:
    return "The parties create an alliance";
    break;
  case CLAUSE_VISION:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give shared vision";
    break;
  case CLAUSE_EMBASSY:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give an embassy";
    break;

  }

  return "";
}



/**************************************************************************
 ...
**************************************************************************/
function diplomacy_cancel_treaty(player_id)
{
  var packet = [{"packet_type" : "diplomacy_cancel_pact", 
	         "other_player_id" : player_id,
                 "clause" : DS_CEASEFIRE}];
  send_request (JSON.stringify(packet));

  update_nation_screen();

  setTimeout("update_nation_screen();", 500);
  setTimeout("update_nation_screen();", 1500);
}
