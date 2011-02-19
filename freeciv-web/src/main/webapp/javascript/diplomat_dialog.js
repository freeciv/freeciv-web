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
 ...
**************************************************************************/
function create_diplomacy_dialog(counterpart) {

  var pplayer = client.conn.playing;

  // reset diplomacy_dialog div.
  $("#diplomacy_dialog").remove();
  $("<div id='diplomacy_dialog'></div>").appendTo("div#game_page");

  $("#diplomacy_dialog").html(
          "<div>Treaty clauses:<br><div id='diplomacy_messages'></div>"
	  + "<div id='diplomacy_player_box_self'></div>"
	  + "<div id='diplomacy_player_box_counterpart'></div>"
	  + "</div>");

  var sprite_self = get_player_fplag_sprite(pplayer);
  var sprite_counterpart = get_player_fplag_sprite(counterpart);

  var flag_self_html = "<div id='flag_self' style='float:left; background: transparent url("
           + sprite_self['image-src'] 
           + "); background-position:-" + sprite_self['tileset-x'] + "px -" 
	   + sprite_self['tileset-y'] 
           + "px;  width: " + sprite_self['width'] + "px;height: " 
	   + sprite_self['height'] + "px; margin: 5px; '>"
           + "</div>";
  var flag_counterpart_html = "<div id='flag_counterpart' style='float:left; background: transparent url("
           + sprite_counterpart['image-src'] 
           + "); background-position:-" + sprite_counterpart['tileset-x'] + "px -" 
	   + sprite_counterpart['tileset-y'] 
           + "px;  width: " + sprite_counterpart['width'] + "px;height: " 
	   + sprite_counterpart['height'] + "px; margin: 5px; '>"
           + "</div>";

  var player_info_html = "<div style='float:left; width: 70%;'>" 
		  			+ nations[pplayer['nation']]['adjective'] + "<br>"
		  			+ "<h3>" + pplayer['name'] + "</h3></div>"
  var counterpart_info_html = "<div style='float:left; width: 70%;'>"  
		  			      + nations[counterpart['nation']]['adjective'] + "<br>"
		                              + "<h3>" + counterpart['name'] + "</h3></div>"


  var agree_self_html = "<div id='agree_self' style='float':right;></div>";
  var agree_counterpart_html = "<div id='agree_counterpart' style='float:right;'></div>";


  var title = "Diplomacy: " + counterpart['name'] 
		 + " of the " + nations[counterpart['nation']]['adjective'];

  $("#diplomacy_dialog").attr("title", title);
  $("#diplomacy_dialog").dialog({
			bgiframe: true,
			modal: false,
			width: "50%",
			height: 430,
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

  // Diplomacy meny for current player.
  $("<div id='self_dipl_div' ></div>").appendTo("#diplomacy_player_box_self");
  $("<ul id='jmenu_self' class='jmenu ui-state-default ui-corner-all'></ul").appendTo("#self_dipl_div");
  $("<li id='self_dipl_top'></li>").appendTo("#jmenu_self");
  $("<a href='#'>Add Clause...</a>").appendTo("#self_dipl_top");
  $("<ul id='self_dipl_add'></ul>").appendTo("#self_dipl_top");
  $("<li>Maps...<ul id='self_maps'></ul></li>").appendTo("#self_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_MAP + ",1);'>World-map</a></li>").appendTo("#self_maps");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_SEAMAP + ",1);'>Sea-map</a></li>").appendTo("#self_maps");
  $("<li id='self_adv_menu'>Advances...<ul id='self_advances'></ul></li>").appendTo("#self_dipl_add");
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
  $("<li id='self_city_menu'>Cities...<ul id='self_cities'></ul></li>").appendTo("#self_dipl_add");
  for (city_id in cities) {
    var pcity = cities[city_id];
    if (!does_city_have_improvement(pcity, "Palace") && city_owner(pcity) == pplayer) {
      $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_CITY + "," + city_id + ");'>" + pcity['name'] + "</a></li>").appendTo("#self_cities");
      city_count_self += 1;
    }
  }
  if (city_count_self == 0) {
    $("#self_city_menu").hide();
  }

  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_VISION + ",1);'>Give shared vision</a></li>").appendTo("#self_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_EMBASSY + ",1);'>Give embassy</a></li>").appendTo("#self_dipl_add");
  $("<li>Pacts...<ul id='self_pacts'></ul></li>").appendTo("#self_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_CEASEFIRE + ",1);'>Cease-fire</a></li>").appendTo("#self_pacts");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_PEACE + ",1);'>Peace</a></li>").appendTo("#self_pacts");
  $("<li><a href='#' onclick='create_clause_req(" + pplayer['playerno']+ "," + CLAUSE_ALLIANCE + ",1);'>Alliance</a></li>").appendTo("#self_pacts");

  // Counterpart menu.
  $("<div id='counterpart_dipl_div'></div>").appendTo("#diplomacy_player_box_counterpart");
  $("<ul id='jmenu_counterpart' class='jmenu ui-state-default ui-corner-all'></ul").appendTo("#counterpart_dipl_div");
  $("<li id='counterpart_dipl_top'></li>").appendTo("#jmenu_counterpart");
  $("<a href='#'>Add Clause...</a>").appendTo("#counterpart_dipl_top");
  $("<ul id='counterpart_dipl_add'></ul>").appendTo("#counterpart_dipl_top");
  $("<li>Maps...<ul id='counterpart_maps'></ul></li>").appendTo("#counterpart_dipl_add");

  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_MAP + ",1);'>World-map</a></li>").appendTo("#counterpart_maps");
  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_SEAMAP + ",1);'>Sea-map</a></li>").appendTo("#counterpart_maps");
  $("<li id='counterpart_adv_menu'>Advances...<ul id='counterpart_advances'></ul></li>").appendTo("#counterpart_dipl_add");
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
  $("<li id='counterpart_city_menu'>Cities...<ul id='counterpart_cities'></ul></li>").appendTo("#counterpart_dipl_add");
  for (city_id in cities) {
    var pcity = cities[city_id];
    if (!does_city_have_improvement(pcity, "Palace") && city_owner(pcity) == counterpart) {
      $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_CITY + "," + city_id + ");'>" + pcity['name'] + "</a></li>").appendTo("#counterpart_cities");
      city_count_counterpart += 1;
    }
  }
  if (city_count_counterpart == 0) {
    $("#counterpart_city_menu").hide();
  }

  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_VISION + ",1);'>Give shared vision</a></li>").appendTo("#counterpart_dipl_add");
  $("<li><a href='#' onclick='create_clause_req(" + counterpart['playerno']+ "," + CLAUSE_EMBASSY + ",1);'>Give embassy</a></li>").appendTo("#counterpart_dipl_add");

  // Setup menus.
  $('#jmenu_self').jmenu({animation:'slide',duration:700});
  $('#jmenu_counterpart').jmenu({animation:'slide',duration:700});


}


