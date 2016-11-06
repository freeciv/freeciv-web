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

var saved_this_turn = false;

/****************************************************************************
  Shows the save game dialog.
****************************************************************************/
function save_game()
{

  if (saved_this_turn) {
    swal("You have already saved this turn, and you can only save once every turn each game-session.");
    return;
  }
  // reset dialog page.
  $("#save_dialog").remove();
  $("<div id='save_dialog'></div>").appendTo("div#game_page");

  var dhtml = "<span id='settings_info'><i>You can save your current game here. "
    + "Savegames are stored on the server, and deleted after 365 days. You can save once every turn in each game session.</i></span>";

  if (!logged_in_with_password) {
    dhtml += "<br><br>You have not logged in using a user account with password. Please "
    + "create a new user account with password next time you want save, so you are sure"
    + " you can load the savegame.<br>"
  }

  $("#save_dialog").html(dhtml);
  $("#save_dialog").attr("title", "Save game");
  $("#save_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "40%",
			close : function(){
			  keyboard_input = true;
                        },
			buttons: {
				"Save Game": function() {
					$("#save_dialog").dialog('close');
					send_message("/save");
					swal("Game saved.");
					}
				}
			});

  if (is_pbem()) {
    swal("Play-By-Email games can not be saved. Please use the end turn button.");
    return;
  }

  $("#save_dialog").dialog('open');
  saved_this_turn = true;
}

/**************************************************************************
 Press Ctrl-S to quickly save the game.
**************************************************************************/
function quicksave()
{
  if (is_pbem()) {
    swal("Play-By-Email games can not be saved. Please use the end turn button.");
    return;
  }

  if (saved_this_turn) {
    swal("You have already saved this turn, and you can only save once every turn each game-session.");
    return;
  }

  send_message("/save");
  add_chatbox_text("Game saved.");
  saved_this_turn = true;

}



/**************************************************************************
 Prepare Load game dialog
**************************************************************************/
function show_load_game_dialog()
{
 $.ajax({
   type: 'POST',
   url: "/listsavegames?username=" + username,
   success: function(data, textStatus, request){
                show_load_game_dialog_cb(data);
   },
   error: function (request, textStatus, errorThrown) {
     swal("Loading game failed (listsavegames failed)");
   }
  });
}

/**************************************************************************
 Show Load game dialog
**************************************************************************/
function show_load_game_dialog_cb(savegames_data)
{
  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var saveHtml =  "<ol id='selectable'>";


  if (savegames_data == null || savegames_data.length < 3) {
      saveHtml = "<b>No savegames found. Please start a new game.</b>";

  } else {
    var savegames = savegames_data.split(";");
    for (var i = 0; i < savegames.length; i++) {
        if (savegames[i].length > 2) {
          saveHtml += "<li class='ui-widget-content'>" + savegames[i] + "</li>";
        }

    }
  }

  saveHtml += "</ol><br>";

  var dialog_buttons = {};

  if (C_S_RUNNING != client_state()) {
    dialog_buttons = $.extend(dialog_buttons,
     {"Load Savegame": function() {
		  var load_game_id = $('#selectable .ui-selected').index();
		  if (load_game_id == -1) {
		    swal("Unable to load savegame: no game selected.");
		  } else if ($('#selectable .ui-selected').text() != null){
            send_message("/load " + $('#selectable .ui-selected').text());

		    $("#dialog").dialog('close');
		    $("#game_text_input").blur();
		  }
    },
     "Load Scenarios...": function() {
		  $("#dialog").dialog('close');
		  $("#game_text_input").blur();
		  show_scenario_dialog();
    },
          "Load Old Savegames...": function() {
     		  old_load_game_dialog();
         }
    });
  }


  $("#dialog").html(saveHtml);
  $("#dialog").attr("title", "Resume playing a saved game");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "50%",
			buttons: dialog_buttons
		});
  $("#selectable").selectable();
  $("#dialog").dialog('open');
  $("#game_text_input").blur();


  $('.ui-dialog-buttonpane button').eq(0).focus();

}