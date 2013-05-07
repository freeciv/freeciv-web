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

var observing = false;
var chosen_nation = -1;
var ai_skill_level = 1;
var nation_select_id = -1;

/****************************************************************************
  ...
****************************************************************************/
function pregame_start_game()
{
  var test_packet = {"type" : packet_chat_msg_req, "message" : "/start"};
  var myJSONText = JSON.stringify(test_packet);
  send_request (myJSONText);
}

/****************************************************************************
  ...
****************************************************************************/
function leave_pregame()
{
  window.location = "/wireframe.jsp?do=login";
}

/****************************************************************************
  ...
****************************************************************************/
function observe()
{
  if (observing) {
    $("#observe_button").button("option", "label", "Observe Game");
    var test_packet = {"type" : packet_chat_msg_req, "message" : "/detach"};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
    
  } else {
    $("#observe_button").button("option", "label", "Don't observe");
    var test_packet = {"type" : packet_chat_msg_req, "message" : "/observe"};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  }
  
  observing = !observing;
}

/****************************************************************************
  ...
****************************************************************************/
function update_player_info()
{
  player_html = "";
  for (id in players) {
    player = players[id];
    if (player != null) {
      if (player['name'].indexOf("AI") != -1) {
        player_html += "<div id='pregame_player_name'><div id='pregame_ai_icon'></div><b>" 
                      + player['name'] + "</b></div>";
      } else {
        player_html += "<div id='pregame_player_name'><div id='pregame_player_icon'></div><b>" 
                      + player['name'] + "</b></div>";
      } 
    }
  }
  $("#pregame_player_list").html(player_html);


}


/****************************************************************************
  ...
****************************************************************************/
function pick_nation()
{

  var nations_html = "<div id='nation_heading'><span>Select your nation:</span> <br>"
                  + "<input id='nation_autocomplete_box' type='text' size='20'>"
		  + "<div id='nation_choice'></div></div> <div id='nation_list'> ";
  var nation_name_list = [];
  for (var nation_id in nations) {
    var pnation = nations[nation_id];
    var sprite = get_nation_flag_sprite(pnation);
    nations_html += "<div onclick='select_nation(" + nation_id + ");' style='background: transparent url("
           + sprite['image-src'] 
           + "); background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '>"
           + "<div id='nation_" + nation_id + "' class='nation_choice'>" + pnation['adjective'] + "</div></div>";
    nation_name_list.push(pnation['adjective']);
  }

  
  nations_html += "</div><div id='nation_legend'></div>";
  
  $("#pick_nation_dialog").html(nations_html);
  $("#pick_nation_dialog").attr("title", "Which nation do you want to rule?");
  $("#pick_nation_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "600px",
			buttons: {
				Ok: function() {
					$(this).dialog('close');
					submit_nation_choise();
				}
			}
		});
	
  $("#nation_legend").html("Please choose your nation carefully.");


  $("#nation_autocomplete_box").autocomplete({
      source: nation_name_list
  });
  

  nation_select_id = setInterval ("update_nation_selection();", 100);
  $("#pick_nation_dialog").dialog('open');		

}

/****************************************************************************
  ...
****************************************************************************/
function update_nation_selection()
{
  var nation_name = $("#nation_autocomplete_box").val();
  if (nation_name.length == 0) return;

  for (var nation_id in nations) {
    var pnation = nations[nation_id];
    if (pnation['adjective'].toLowerCase() == nation_name.toLowerCase()) {
      select_nation(nation_id);
    }
  }
}

/****************************************************************************
  ...
****************************************************************************/
function select_nation(nation_id)
{
  var pnation = nations[nation_id];
  $("#nation_legend").html(pnation['legend']);

  $("#nation_autocomplete_box").val(pnation['adjective']);

  $("#nation_" + chosen_nation).css("background-color", "transparent");

  $("#nation_choice").html("Your Nation: " + pnation['adjective']);

  if (chosen_nation != nation_id) {
    document.getElementById("nation_" + nation_id).scrollIntoView()
  }

  chosen_nation = parseFloat(nation_id);
  
  $("#nation_" + chosen_nation).css("background-color", "#555555");
}

/****************************************************************************
  ...
****************************************************************************/
function submit_nation_choise()
{
  if (chosen_nation == -1) return;

  var test_packet = {"type" : packet_nation_select_req, 
                      "player_no" : client.conn['player_num'],
                      "nation_no" : chosen_nation,
                      "is_male" : true, /* FIXME */
                      "name" : client.conn['username'],
                      "city_style" : nations[chosen_nation]['city_style']};
  var myJSONText = JSON.stringify(test_packet);
  send_request (myJSONText);
  clearInterval(nation_select_id);
  $("#pick_nation_button").button( "option", "disabled", true); 
}



/****************************************************************************
  ...
****************************************************************************/
function pregame_settings()
{
  var id = "#pregame_settings";
  $(id).remove();
  $("<div id='pregame_settings'></div>").appendTo("div#pregame_page");


  var dhtml = "<table>" +
  	  "<tr><td>Game title:</td>" +
	  "<td><input type='text' name='metamessage' id='metamessage' size='20' maxlength='32'></td></tr>" +
	  "<tr><td>Number of Players (including AI):</td>" +
	  "<td><input type='number' name='aifill' id='aifill' size='4' length='3' min='0' max='30' step='1'></td></tr>" +
	  "<tr><td>Timeout (number of seconds per round):</td>" +
	  "<td><input type='number' name='timeout' id='timeout' size='4' length='3' min='30' max='3600' step='1'></td></tr>" +
	  "<tr><td>Map size:</td>" +
	  "<td><input type='number' name='mapsize' id='mapsize' size='4' length='3' min='1' max='15' step='1'></td></tr>" +
	  "<tr><td>AI skill level:</td>" +
	  "<td><select name='skill_level' id='skill_level'>" +
	  "<option value='0'>Novice</option>" +
	  "<option value='1'>Easy</option>" +
          "<option value='2'>Normal</option>" +
          "<option value='3'>Hard</option>" +
	  "</select></td></tr></table><br>" +
	  "<span id='settings_info'><i>Freeciv-web can be customized using the command line in many other ways also. Type /help in the command line for more information.</i></span>" 
	  ;
  $(id).html(dhtml);

  $(id).attr("title", "Game Settings");
  $(id).dialog({
			bgiframe: true,
			modal: false,
			width: "450",
			  buttons: {
				Ok: function() {
					$(this).dialog('close');
				}
			  }

  });

  $("#aifill").val(game_info['aifill']);
  $("#mapsize").val(game_info['mapsize']);
  $("#timeout").val(game_info['timeout']);
  $("#skill_level").val(ai_skill_level);

  $(id).dialog('open');		

  $('#aifill').change(function() {
    var test_packet = {"type" : packet_chat_msg_req, "message" : "/set aifill " + $('#aifill').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  });

  $('#metamessage').change(function() {
    var test_packet = {"type" : packet_chat_msg_req, "message" : "/metamessage " + $('#metamessage').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  });

  $('#metamessage').bind('keyup blur',function(){ 
    $(this).val( $(this).val().replace(/[^a-z\s]/g,'') ); }
  );

  $('#mapsize').change(function() {
    var test_packet = {"type" : packet_chat_msg_req, "message" : "/set size " + $('#mapsize').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  });

  $('#timeout').change(function() {
    var test_packet = {"type" : packet_chat_msg_req, "message" : "/set timeout " + $('#timeout').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  });

  $('#skill_level').change(function() {
    ai_skill_level = parseFloat($('#skill_level').val());
    var test_packet = "";
    if (ai_skill_level == 0) {
      test_packet = {"type" : packet_chat_msg_req, "message" : "/novice"};
    } else if (ai_skill_level == 1) {
      test_packet = {"type" : packet_chat_msg_req, "message" : "/easy"};
    } else if (ai_skill_level == 2) {
      test_packet = {"type" : packet_chat_msg_req, "message" : "/normal"};
    } else if (ai_skill_level == 3) {
      test_packet = {"type" : packet_chat_msg_req, "message" : "/hard"};
    }
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  });

}

