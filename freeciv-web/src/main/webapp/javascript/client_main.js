/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


var C_S_INITIAL = 0;    /* Client boot, only used once on program start. */
var C_S_PREPARING = 1;  /* Main menu (disconnected) and connected in pregame. */
var C_S_RUNNING = 2;    /* Connected with game in progress. */
var C_S_OVER = 3;       /* Connected with game over. */

var civclient_state = C_S_INITIAL;

var endgame_player_info = [];


/**************************************************************************
 Sets the client state (initial, pre, running, over etc).
**************************************************************************/
function set_client_state(newstate)
{
  var connect_error = (C_S_PREPARING == civclient_state)
      && (C_S_PREPARING == newstate);
  var oldstate = civclient_state;

  if (civclient_state != newstate) {
    civclient_state = newstate;

    switch (civclient_state) {
    case C_S_RUNNING:
      chatbox_text = " ";
      $.unblockUI(); 
      show_new_game_message();
      
      set_client_page(PAGE_GAME);
      setup_window_size();

      if (observing) center_tile_mapcanvas(map_pos_to_tile(15,15));
      update_metamessage_on_gamestart();
      deduplicate_player_colors();

      break;
    case C_S_OVER:
      setTimeout(show_endgame_dialog, 500);
      break;
    case C_S_PREPARING:
      break;
    default:
      break;
    }
  }


}

function client_state()
{
  return civclient_state;
}


/**************************************************************************
  Return TRUE if the client can change the view; i.e. if the mapview is
  active.  This function should be called each time before allowing the
  user to do mapview actions.
**************************************************************************/
function can_client_change_view()
{
  return ((client.conn.playing != null || client_is_observer())
      && (C_S_RUNNING == client_state()
	      || C_S_OVER == client_state())); 
}

/**************************************************************************
  Webclient does have observer support.
**************************************************************************/
function client_is_observer()
{
  return client.conn['observer'] || observing;
}

/**************************************************************************
  Intro message
**************************************************************************/
function show_new_game_message()
{

  if (observing) {
    /* do nothing. */

  } else if (is_small_screen()) {
    show_dialog_message("Welcome to Freeciv-web", 
      "You lead a great civilization. Your task is to conquer the world!\n" +
      "Click on units for giving them orders, and drag units on the map to move them.\n" +
      "Good luck, and have a lot of fun!");

  } else  {
    if (is_touch_device()) {
      show_dialog_message("Welcome to Freeciv-web", 
      "Welcome to Freeciv-web.  You lead a great civilization.  Your\n" +
      "task is to conquer the world!  You should start by\n" +
      "exploring the land around you with your explorer,\n" +
      "and using your settlers to find a good place to build\n" +
      "a city. Click on units for giving them orders. \n" +
      "To move your units around, carefully use your \n" +
      "finger to drag units to the place you want it to go.\n" +
      "Good luck, and have a lot of fun!");

    } else {
      show_dialog_message("Welcome to Freeciv-web", 
      "Welcome to Freeciv-web.  You lead a great civilization.  Your\n" +
      "task is to conquer the world!  You should start by\n" +
      "exploring the land around you with your explorer,\n" +
      "and using your settlers to find a good place to build\n" +
      "a city. Right-click with the mouse on your units for a list of available \n" +
      "orders such as move, explore, build cities and attack.\n" +
      "Good luck, and have a lot of fun!");

    }

  } 
}

/**************************************************************************
...
**************************************************************************/
function alert_war(player_no)
{
  var pplayer = players[player_no];
  show_dialog_message("War!", "You are now at war with the " 
	+ nations[pplayer['nation']]['adjective']
    + " leader " + pplayer['name'] + "!");
}

/**************************************************************************
 Shows the endgame dialog
**************************************************************************/
function show_endgame_dialog()
{
  var title = "Final Report: The Greatest Civilizations in the world!";
  var message = "";
  for (var i = 0; i < endgame_player_info.length; i++) {
    var pplayer = players[endgame_player_info[i]['player_id']];
    var nation_adj = nations[pplayer['nation']]['adjective'];
    message += (i+1) + ": The " + nation_adj + " ruler " + pplayer['name'] + " scored " + endgame_player_info[i]['score'] + " points" + "<br>";
  }

  show_dialog_message(title, message);
}


/**************************************************************************
 
**************************************************************************/
function update_metamessage_on_gamestart()
{
  if (!observing && !metamessage_changed && client.conn.playing != null
      && client.conn.playing['pid'] == players[0]['pid']) {
    var pplayer = client.conn.playing;
    var metasuggest = username + " ruler of the " + nations[pplayer['nation']]['adjective'] + " empire.";

    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/metamessage " + metasuggest};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
    setTimeout("chatbox_text = ' '; add_chatbox_text('');", 500);
  }
}
