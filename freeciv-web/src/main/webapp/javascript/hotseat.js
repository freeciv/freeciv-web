/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2016  The Freeciv-web project

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

var num_hotseat_players = 2;
var hotseat_players = [];
var hotseat_active_player = 0;
var hotseat_enabled = false;

/**************************************************************************
 Shows the new hotseat game dialog.
**************************************************************************/
function show_hotseat_dialog() 
{
  var message = "Now you can start a new hotseat game, where two or more "
   + "players plays on the same device by taking turns playing the game.<br>"
   + "Please give the names of each human player here:"
   + "<br><br><div id='new_hotseat_players'>"
   + "<div class='hotseat_player'>Player name 1: <input id='username_req_1' type='text' size='25' maxlength='31'></div>"
   + "<div class='hotseat_player'>Player name 2: <input id='username_req_2' type='text' size='25' maxlength='31'></div>"
   + "</div>"
   + "<br><br><span id='username_validation_result'></span>";

  // reset dialog page.
  $("#hotseat_dialog").remove();
  $("<div id='hotseat_dialog'></div>").appendTo("div#game_page");
  $("#hotseat_dialog").html(message);
  $("#hotseat_dialog").attr("title", "New hotseat game");
  $("#hotseat_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "55%",
                        height: 500,
			buttons:
			{
                                 "Add player" : function() {
                                    add_hotseat_player(); 
                                 },
				 "New hotseat game": function() {
                                    new_hotseat_game();
				}


			}

		});

  $("#hotseat_dialog").dialog('open');
  var stored_username = simpleStorage.get("username", "");
  if (stored_username != null && stored_username != false) {
    $("#username_req_1").val(stored_username);
  }


}

/**************************************************************************
 ...
**************************************************************************/
function new_hotseat_game() 
{
  for (var i = 1; i <= num_hotseat_players; i++) {
    if (!validate_hotseat_username("#username_req_" + i)) return;
  }

  username = $("#username_req_1").val();
  simpleStorage.set("username", username);
  hotseat_enabled = true;
  network_init();
  setTimeout(setup_hotseat_game, 800);
}

/**************************************************************************
 ...
**************************************************************************/
function setup_hotseat_game() 
{
  send_message("/set phasemode player");
  send_message("/set minp 2");
  send_message("/set ec_chat=enabled");
  send_message("/set ec_info=enabled"); 
  send_message("/set ec_max_size=20000");
  send_message("/set ec_turns=32768");
  send_message("/set autotoggle disabled");
  hotseat_players.push($("#username_req_1").val());
  for (var i = 2; i <= num_hotseat_players; i++) {
    send_message("/create " + $("#username_req_" + i).val());
    send_message("/ai " + $("#username_req_" + i).val());
    hotseat_players.push($("#username_req_" + i).val());
  }
  $("#hotseat_dialog").dialog('close');
  send_message("Press Start Game to begin.");
}

/**************************************************************************
 ...
**************************************************************************/
function validate_hotseat_username(field) {
  var check_username = $(field).val();

  var cleaned_username = check_username.replace(/[^a-zA-Z]/g,'');

  if (check_username == null || check_username.length == 0) {
    $("#username_validation_result").html("The name can't be empty.");
    return false;
  } else if (check_username.length <= 2 ) {
    $("#username_validation_result").html("The name is too short.");
    return false;
  } else if (check_username.length >= 32) {
    $("#username_validation_result").html("The name is too long.");
    return false;
  } else if (check_username != cleaned_username) {
    $("#username_validation_result").html("The name contains invalid characters, only the English alphabet is allowed.");
    return false;
  }

  return true;
}


/**************************************************************************
 ...
**************************************************************************/
function add_hotseat_player() 
{
  if (num_hotseat_players >= 125) {
    swal("Support for more players is not available now.");
    return;
  }
  num_hotseat_players += 1;

  $("#new_hotseat_players").append("<div class='hotseat_player'>Player name " + num_hotseat_players 
    + ": <input id='username_req_" + num_hotseat_players + "' type='text' size='25' maxlength='31'></div>");

}

/**************************************************************************
...
**************************************************************************/
function is_hotseat() 
{
  return ($.getUrlVar('action') == "hotseat") || hotseat_enabled;
}

/**************************************************************************
...
**************************************************************************/
function hotseat_next_player() 
{
  hotseat_active_player = ((hotseat_active_player + 1) % num_hotseat_players);
  send_message("/take " + hotseat_players[hotseat_active_player]);
  chatbox_text = ' ';
  setTimeout(advance_unit_focus, 400);
  $("#turn_done_button").button("option", "disabled", false);
  $("#turn_done_button").button("option", "label", "Turn Done");
  show_hotseat_new_phase();

}

/**************************************************************************
...
**************************************************************************/
function show_hotseat_new_phase() 
{

  if (hotseat_players.length == 0) hotseat_load_refresh();

  var message = "It is now " + hotseat_players[hotseat_active_player] 
                 + "'s turn to play in this hotseat game.";

  // reset dialog page.
  $("#hotseat_dialog").remove();
  $("<div id='hotseat_dialog'></div>").appendTo("div#game_page");
  $("#hotseat_dialog").html(message);
  $("#hotseat_dialog").attr("title", "Hotseat game");
  $("#hotseat_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "55%",
			buttons:
			{
                                 "Ok" : function() {
                                   $("#hotseat_dialog").dialog('close');
                                   $("#game_text_input").blur();
                                 }
			}

		});

  $("#hotseat_dialog").dialog('open');


}

/**************************************************************************
 Initialize hotseat data after loading a hotseat savegame.
**************************************************************************/
function hotseat_load_refresh() 
{
  num_hotseat_players = 0;
  for (var player_id in players) {
    var pplayer = players[player_id];
    if (pplayer['flags'].isSet(PLRF_AI) == false) {
      hotseat_players.push(pplayer['name']);
      num_hotseat_players++;
    }
  }

  for (var player_id in players) {
    var pplayer = players[player_id];
    if (pplayer['flags'].isSet(PLRF_AI) == false && pplayer['phase_done'] == false) {
      return;
    }
    if (hotseat_players.indexOf(pplayer['name']) >= 0) hotseat_active_player++;
  }

}
