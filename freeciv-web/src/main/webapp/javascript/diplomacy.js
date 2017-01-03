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
  var packet = {"pid" : packet_diplomacy_init_meeting_req,
	        "counterpart" : counterpart};
  send_request(JSON.stringify(packet));
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
  var packet = {"pid" : packet_diplomacy_accept_treaty_req,
	         "counterpart" : active_diplomacy_meeting_id};
  send_request(JSON.stringify(packet));
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


  var agree_self_html = "<div id='agreement' style='float:left; background: transparent url("
           + agree_sprite['image-src']
           + "); background-position:-" + agree_sprite['tileset-x'] + "px -"
	   + agree_sprite['tileset-y']
           + "px;  width: " + agree_sprite['width'] + "px;height: "
	   + agree_sprite['height'] + "px; margin: 5px; '>"
           + "</div>";
  var disagree_self_html = "<div id='agreement' style='float:left; background: transparent url("
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

  setTimeout(refresh_diplomacy_request_queue, 1000);

}

/**************************************************************************
 ...
**************************************************************************/
function cancel_meeting_req()
{
  var packet = {"pid" : packet_diplomacy_cancel_meeting_req,
	        "counterpart" : active_diplomacy_meeting_id};
  send_request(JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function create_clause_req(giver, type, value)
{
  var packet = {"pid" : packet_diplomacy_create_clause_req,
	        "counterpart" : active_diplomacy_meeting_id,
                "giver" : giver,
                "type" : type,
                "value" : value};
  send_request(JSON.stringify(packet));
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

  setTimeout(refresh_diplomacy_request_queue, 1000);
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

  var packet = {"pid" : packet_diplomacy_remove_clause_req,
	        "counterpart" : clause['counterpart'],
                "giver": clause['giver'],
                "type" : clause['type'],
                "value": clause['value'] };
  send_request(JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function remove_clause(remove_clause_obj)
{
  var clause_list = diplomacy_clause_map[remove_clause_obj['counterpart']];
  for (var i = 0; i < clause_list.length; i++) {
    var check_clause = clause_list[i];
    if (remove_clause_obj['counterpart'] == check_clause['counterpart']
	&& remove_clause_obj['giver'] == check_clause['giver']
        && remove_clause_obj['type'] == check_clause['type']) {

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
      var nation = nations[pplayer['nation']]['adjective'];
      var ptech = techs[value];
      return "The " + nation + " give " + ptech['name'];
  case CLAUSE_CITY:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective'];
    var pcity = cities[value];

    if (pcity != null) {
      return "The " + nation + " give " + decodeURIComponent(pcity['name']);
    } else {
      return "The " + nation + " give unknown city.";
    }
    break;

  case CLAUSE_GOLD:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective'];
    if (giver == client.conn.playing['playerno']) {
      $("#self_gold").val(value);
    } else {
      $("#counterpart_gold").val(value);
    }
    return "The " + nation + " give " + value + " gold";
  case CLAUSE_MAP:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give their worldmap";
  case CLAUSE_SEAMAP:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give their seamap";
  case CLAUSE_CEASEFIRE:
    return "The parties agree on a cease-fire";
  case CLAUSE_PEACE:
    return "The parties agree on a peace";
  case CLAUSE_ALLIANCE:
    return "The parties create an alliance";
  case CLAUSE_VISION:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give shared vision";
  case CLAUSE_EMBASSY:
    var pplayer = players[giver];
    var nation = nations[pplayer['nation']]['adjective']
    return "The " + nation + " give an embassy";
  }

  return "";
}



/**************************************************************************
 ...
**************************************************************************/
function diplomacy_cancel_treaty(player_id)
{
  var packet = {"pid" : packet_diplomacy_cancel_pact,
	        "other_player_id" : player_id,
                "clause" : DS_CEASEFIRE};
  send_request(JSON.stringify(packet));

  update_nation_screen();

  setTimeout(update_nation_screen, 500);
  setTimeout(update_nation_screen, 1500);
}



/**************************************************************************
 ...
**************************************************************************/
function create_diplomacy_dialog(counterpart) {

  var pplayer = client.conn.playing;
  var city_id;

  // reset diplomacy_dialog div.
  $("#dialog").remove();
  $("#diplomacy_dialog").remove();
  $("#self-items").remove();
  $("#counterpart-items").remove();
  $(".positionHelper").remove();
  $("<div id='diplomacy_dialog'></div>").appendTo("div#game_page");

  $("#diplomacy_dialog").html(
          "<div>Treaty clauses:<br><div id='diplomacy_messages'></div>"
	  + "<div id='diplomacy_player_box_self'></div>"
	  + "<div id='diplomacy_player_box_counterpart'></div>"
	  + "</div>");

  var self_nation = nations[pplayer['nation']];
  var counterpart_nation = nations[counterpart['nation']];

  var flag_self_html = "<img src='/images/flags/" + self_nation['graphic_str'] + "-web.png' id='flag_self'>";
  var flag_counterpart_html = "<img src='/images/flags/" + counterpart_nation['graphic_str']
	  + "-web.png' id='flag_counterpart'>";

  var player_info_html = "<div style='float:left; width: 70%;'><h3>"
		  			+ nations[pplayer['nation']]['adjective']
		  			+ " " + pplayer['name'] + "</h3></div>"
  var counterpart_info_html = "<div style='float:left; width: 70%;'><h3>"
		  			      + nations[counterpart['nation']]['adjective']
		                              + " " + counterpart['name'] + "</h3></div>"


  var agree_self_html = "<div id='agree_self' style='float':right;></div>";
  var agree_counterpart_html = "<div id='agree_counterpart' style='float:right;'></div>";


  var title = "Diplomacy: " + counterpart['name']
		 + " of the " + nations[counterpart['nation']]['adjective'];

  $("#diplomacy_dialog").attr("title", title);
  $("#diplomacy_dialog").dialog({
			bgiframe: true,
			modal: false,
			width: is_small_screen() ? "90%" : "50%",
			height: 500,
			buttons: {
				"Accept treaty": function() {
				        accept_treaty_req();
				},
				"Cancel meeting" : function() {
				        cancel_meeting_req();
				}
			},
			close: function() {
			     cancel_meeting_req();
			}
		});

  $("#diplomacy_dialog").dialog('open');
  $(".ui-dialog").css("overflow", "visible");


  $("#diplomacy_player_box_self").html(flag_self_html + agree_self_html
		                       + player_info_html);
  $("#diplomacy_player_box_counterpart").html(flag_counterpart_html + agree_counterpart_html
		                               + counterpart_info_html);




  // Diplomacy menu for current player.
  $("<div id='self_dipl_div' ></div>").appendTo("#diplomacy_player_box_self");

  var fg_menu_self_html = "<a tabindex='0' class='fg-button fg-button-icon-right ui-widget ui-state-default ui-corner-all' id='hierarchy_self'>" +
	  "<span class='ui-icon ui-icon-triangle-1-s'></span>Add Clause...</a> " +
	  "<span class='diplomacy_gold'>Gold:<input id='self_gold' type='number' step='1' size='3' value='0'></span><div id='self-items' class='hidden'></div>";
  $(fg_menu_self_html).appendTo("#self_dipl_div");


  $("<ul id='self_dipl_add'></ul>").appendTo("#self-items");
  $("<li><a href='#'>Maps...</a><ul id='self_maps'></ul></li>").appendTo("#self_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_MAP + ",1);'>World-map</a></li>").appendTo("#self_maps");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_SEAMAP + ",1);'>Sea-map</a></li>").appendTo("#self_maps");
  $("<li id='self_adv_menu'><a href='#'>Advances...</a><ul id='self_advances'></ul></li>").appendTo("#self_dipl_add");
  var tech_count_self = 0;
  for (var tech_id in techs) {
    if (player_invention_state(pplayer, tech_id) == TECH_KNOWN
        // && player_invention_reachable(pother, i)
        && (player_invention_state(counterpart, tech_id) == TECH_UNKNOWN
            || player_invention_state(counterpart, tech_id) == TECH_PREREQS_KNOWN)) {
      var ptech = techs[tech_id];
      $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_ADVANCE + "," + tech_id
		      + ");'>" + ptech['name'] + "</a></li>").appendTo("#self_advances");
      tech_count_self += 1;
    }
  }
  if (tech_count_self == 0) {
    $("#self_adv_menu").hide();
  }
  var city_count_self = 0;
  $("<li id='self_city_menu'><a href='#'>Cities...</a><ul id='self_cities'></ul></li>").appendTo("#self_dipl_add");
  for (city_id in cities) {
    var pcity = cities[city_id];
    if (!does_city_have_improvement(pcity, "Palace") && city_owner(pcity) == pplayer) {
      $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_CITY + "," + city_id + ");'>" + decodeURIComponent(pcity['name']) + "</a></li>").appendTo("#self_cities");
      city_count_self += 1;
    }
  }
  if (city_count_self == 0) {
    $("#self_city_menu").hide();
  }

  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_VISION + ",1);'>Give shared vision</a></li>").appendTo("#self_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_EMBASSY + ",1);'>Give embassy</a></li>").appendTo("#self_dipl_add");
  $("<li><a href='#'>Pacts...</a><ul id='self_pacts'></ul></li>").appendTo("#self_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_CEASEFIRE + ",1);'>Cease-fire</a></li>").appendTo("#self_pacts");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_PEACE + ",1);'>Peace</a></li>").appendTo("#self_pacts");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_ALLIANCE + ",1);'>Alliance</a></li>").appendTo("#self_pacts");



  /* setup fg-menu */
  $('#hierarchy_self').fgmenu({
    content: $('#self-items').html(),
    flyOut: true
  });


  // Counterpart menu.
  $("<div id='counterpart_dipl_div'></div>").appendTo("#diplomacy_player_box_counterpart");

  var fg_menu_counterpart_html = "<a tabindex='0' class='fg-button fg-button-icon-right ui-widget ui-state-default ui-corner-all' id='hierarchy_counterpart'>" +
	  "<span class='ui-icon ui-icon-triangle-1-s'></span>Add Clause...</a> " +
	  "<span class='diplomacy_gold'>Gold:<input id='counterpart_gold' type='number' step='1' size='3' value='0'></span><div id='counterpart-items' class='hidden'> </div>";
  $(fg_menu_counterpart_html).appendTo("#counterpart_dipl_div");


  $("<ul id='counterpart_dipl_add'></ul>").appendTo("#counterpart-items");
  $("<li><a href='#'>Maps...</a><ul id='counterpart_maps'></ul></li>").appendTo("#counterpart_dipl_add");

  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_MAP + ",1);'>World-map</a></li>").appendTo("#counterpart_maps");
  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_SEAMAP + ",1);'>Sea-map</a></li>").appendTo("#counterpart_maps");
  $("<li id='counterpart_adv_menu'><a href='#'>Advances...</a><ul id='counterpart_advances'></ul></li>").appendTo("#counterpart_dipl_add");
  var tech_count_counterpart = 0;
  for (var tech_id in techs) {
    if (player_invention_state(counterpart, tech_id) == TECH_KNOWN
        // && player_invention_reachable(pother, i)
        && (player_invention_state(pplayer, tech_id) == TECH_UNKNOWN
            || player_invention_state(pplayer, tech_id) == TECH_PREREQS_KNOWN)) {
      var ptech = techs[tech_id];
      $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_ADVANCE + "," + tech_id
		      + ");'>" + ptech['name'] + "</a></li>").appendTo("#counterpart_advances");
      tech_count_counterpart += 1;
    }
  }
  if (tech_count_counterpart == 0) {
    $("#counterpart_adv_menu").hide();
  }
  var city_count_counterpart = 0;
  $("<li id='counterpart_city_menu'><a href='#'>Cities...</a><ul id='counterpart_cities'></ul></li>").appendTo("#counterpart_dipl_add");
  for (city_id in cities) {
    var pcity = cities[city_id];
    if (!does_city_have_improvement(pcity, "Palace") && city_owner(pcity) == counterpart) {
      $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_CITY + "," + city_id + ");'>" + decodeURIComponent(pcity['name']) + "</a></li>").appendTo("#counterpart_cities");
      city_count_counterpart += 1;
    }
  }
  if (city_count_counterpart == 0) {
    $("#counterpart_city_menu").hide();
  }

  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_VISION + ",1);'>Give shared vision</a></li>").appendTo("#counterpart_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_EMBASSY + ",1);'>Give embassy</a></li>").appendTo("#counterpart_dipl_add");


  /* setup fg-menu */
  $('#hierarchy_counterpart').fgmenu({
    content: $('#counterpart-items').html(),
    flyOut: true
  });

  $("#self_gold").attr({
       "max" : pplayer['gold'],
       "min" : 0
    });

  $("#counterpart_gold").attr({
       "max" : counterpart['gold'],
       "min" : 0
    });

  var wto;
  $("#counterpart_gold").change(function() {
    clearTimeout(wto);
    wto = setTimeout(function() {
    var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
    if (clauses != null) {
      for (var i = 0; i < clauses.length; i++) {
        var clause = clauses[i];
        if (clause['giver'] == counterpart['playerno'] && clause['type'] == CLAUSE_GOLD) {
  	  var packet = {"pid" : packet_diplomacy_remove_clause_req,
	                "counterpart" : active_diplomacy_meeting_id,
                        "giver": clause['giver'],
                        "type" : CLAUSE_GOLD,
                        "value": clause['value']};
           send_request(JSON.stringify(packet));
         }
       }
     }

     var packet = {"pid" : packet_diplomacy_create_clause_req,
	           "counterpart" : active_diplomacy_meeting_id,
                   "giver" : counterpart['playerno'],
                   "type" : CLAUSE_GOLD,
                   "value" : parseFloat($("#counterpart_gold").val())};
     send_request(JSON.stringify(packet));
    }, 500);
  });

   $("#self_gold").change(function() {
    clearTimeout(wto);
    wto = setTimeout(function() {
    var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
    if (clauses != null) {
      for (var i = 0; i < clauses.length; i++) {
        var clause = clauses[i];
        if (clause['giver'] == pplayer['playerno'] && clause['type'] == CLAUSE_GOLD) {
          var packet = {"pid" : packet_diplomacy_remove_clause_req,
	                "counterpart" : active_diplomacy_meeting_id,
                        "giver": clause['giver'],
                        "type" : CLAUSE_GOLD,
                        "value": clause['value']};
         send_request(JSON.stringify(packet));
        }
      }
     }

     var packet = {"pid" : packet_diplomacy_create_clause_req,
	           "counterpart" : active_diplomacy_meeting_id,
                   "giver" : pplayer['playerno'],
                   "type" : CLAUSE_GOLD,
                   "value" : parseFloat($("#self_gold").val())};
    send_request(JSON.stringify(packet));
    }, 500);
   });

  $("#diplomacy_dialog").parent().css("z-index", 1000);

}

