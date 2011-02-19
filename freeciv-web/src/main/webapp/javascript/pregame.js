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
var chosen_nation = 1;

/****************************************************************************
  ...
****************************************************************************/
function pregame_start_game()
{
  var test_packet = [{"packet_type" : "chat_msg_req", "message" : "/start"}];
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
    $("#observe_button").text("Observe Game");
    var test_packet = [{"packet_type" : "chat_msg_req", "message" : "/detach"}];
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
    
  } else {
    $("#observe_button").text("Don't observe");
    var test_packet = [{"packet_type" : "chat_msg_req", "message" : "/observe"}];
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

  var nations_html = "";
  
  nations_html += "<div id='nation_list'> ";
  for (var nation_id in nations) {
    var pnation = nations[nation_id];
    var sprite = get_nation_flag_sprite(pnation);
    nations_html += "<div onclick='select_nation(" + nation_id + ");' style='background: transparent url("
           + sprite['image-src'] 
           + "); background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '>"
           + "<div id='nation_choice'>" + pnation['adjective'] + "</div></div>";
  }

  
  nations_html += "</div><div id='nation_legend'></div>";
  
  $("#pick_nation_dialog").html(nations_html);
  $("#pick_nation_dialog").attr("title", "Which nation do you want to play?");
  $("#pick_nation_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "50%",
			buttons: {
				Ok: function() {
					$(this).dialog('close');
					submit_nation_choise(chosen_nation);
				}
			}
		});
	
  $("#pick_nation_dialog").dialog('open');		

}



/****************************************************************************
  ...
****************************************************************************/
function select_nation(nation_id)
{
  var pnation = nations[nation_id];
  $("#nation_legend").html(pnation['legend']);
  chosen_nation = nation_id
}

/****************************************************************************
  ...
****************************************************************************/
function submit_nation_choise(chosen_nation)
{
  var test_packet = [{"packet_type" : "nation_select_req", 
                      "player_no" : client.conn['player_num'],
                      "nation_no" : chosen_nation,
                      "is_male" : true, /* FIXME */
                      "name" : client.conn['username'],
                      "city_style" : nations[chosen_nation]['city_style']}];
  var myJSONText = JSON.stringify(test_packet);
  send_request (myJSONText);
  
  
}
