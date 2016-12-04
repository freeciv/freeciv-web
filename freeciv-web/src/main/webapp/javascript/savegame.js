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

var scenarios = [
  {"img":"/images/world_small.png", "description":"The World - Small world map, 80x50 map of the Earth", "savegame":"earth-80x50-v3"},
  {"img":"/images/world_big.png", "description":"The World - Large world map, 160x90 map of the Earth", "savegame":"earth-160x90-v2"},
  {"img":"/images/iberian.png", "description":"Iberian Peninsula - 136x100 map of Spain and Portugal", "savegame":"iberian-peninsula-136x100-v1.0"},
  {"img":"/images/france.png", "description":"France - Large (140x90)", "savegame":"france-140x90-v2"},
  {"img":"/images/japan.png", "description":"Japan - Medium (88x100)", "savegame":"japan-88x100-v1.3"},
  {"img":"/images/italy.png", "description":"Italy - Medium (100x100)", "savegame":"italy-100x100-v1.5"},
  {"img":"/images/america.png", "description":"North America - 116x100 map of North America", "savegame":"north_america_116x100-v1.2"},
  {"img":"/images/british.png", "description":"British Aisles - Medium (85x80)", "savegame":"british-isles-85x80-v2.80"},
  {"img":"/images/hagworld.png", "description":"The World - Classic-style 120x60 map of the Earth", "savegame":"hagworld-120x60-v1.2"},
  {"img":"/images/europe.png", "description":"Very large map of Europe, 200x100", "savegame":"europe-200x100-v2"}
];

var scenario_info = null;
var scenario_activated = false;
var loadTimerId = -1;


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
    dhtml += "<br><br>Warning: You have not logged in using a user account with password. Please "
    + "create a new user account with password next time you want save, so you are sure"
    + " you can load the savegame with a username and password. <a href='/webclient/?action=new'>Click here</a> "
    + "to start a new game, then click on the \"New User Account\" button to create a new account.<br>"
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
          if (i < savegames.length - 2) {
            saveHtml += "<li class='ui-widget-content'>" + savegames[i] + "</li>";
          } else {
            saveHtml += "<li class='ui-selected ui-widget-content'>" + savegames[i] + "</li>";
          }
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
    "Delete" : function() {
      var load_game_id = $('#selectable .ui-selected').index();
      if (load_game_id != -1) {
        delete_savegame($('#selectable .ui-selected').text());
      }
      $('#selectable .ui-selected').remove();
    }
     ,
     "Load Scenarios...": function() {
		  $("#dialog").dialog('close');
		  $("#game_text_input").blur();
		  show_scenario_dialog();
    }
    });
  }


  $("#dialog").html(saveHtml);
  $("#dialog").attr("title", "Resume playing a saved game");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "50%",
			height: is_small_screen() ? $(window).height() - 20 : $(window).height() - 80,
			buttons: dialog_buttons
		});
  $("#selectable").selectable();
  $("#dialog").dialog('open');
  $("#game_text_input").blur();


  $('.ui-dialog-buttonpane button').eq(0).focus();

}

/**************************************************************************
 Deletes a savegame
**************************************************************************/
function delete_savegame(filename)
{
  var stored_password = simpleStorage.get("password", "");
  if (stored_password != null && stored_password != false) {
    $.ajax({
     type: 'POST',
     url: "/deletesavegame?username=" + encodeURIComponent(username) + "&savegame=" + encodeURIComponent(filename) + "&password=" + encodeURIComponent(stored_password)
    });
  }
}


/**************************************************************************
 Send a load game command, if requested by user.
 uses HTML5 local storage.
**************************************************************************/
function load_game_check()
{
  var load_game_id = $('#selectable .ui-selected').index();
  var scenario = $.getUrlVar('scenario');

  if ($.getUrlVar('load') == "tutorial") {
    $.blockUI();
    loadTimerId = setTimeout("load_game_real('tutorial');",
                                  1500);
    setTimeout(load_game_toggle,3500);
  } else if ($.getUrlVar('action') == "earthload") {
    $.blockUI();
    var savegame_earth_file = $.getUrlVar('savegame').replace("#", "");
    loadTimerId = setTimeout("load_game_real('" + savegame_earth_file + "');",
                                  1500);
    setTimeout(load_game_toggle,3500);

  } else if (load_game_id != -1) {
    $.blockUI();

    if (scenario == "true" || scenario_activated) {
      if (load_game_id == -1) {
        show_scenario_dialog();
      } else {
        scenario_game_id = scenarios[load_game_id]['savegame'];
        loadTimerId = setTimeout("load_game_real('" + scenario_game_id + "');",
                                  1500);
        setTimeout(load_game_toggle,2500);
      }
    } else if (load_game_id != -1) {
      console.log("Attempting to load savegame-file-" + load_game_id);
      var savegames = simpleStorage.get("savegames");
      if (savegames == null || savegames.length == 0) {
        console.error("Loading game failed (savenames is empty).");
        swal("Loading game failed (savenames is empty).");
        return;
      } else {
        var savegame = savegames[load_game_id];
        console.log("Saved title: " +  savegame["title"]);
        console.log("Saved user: " + savegame["username"]);
        console.log("Game type: " + savegame["game_type"]);
        console.log("Logged in username: " + username);
        if (savegame["file"] == null) {
          swal("Loading game failed (savegame file is null)");
  	return;
        } else {
          console.log("Savegame file length: " + savegame["file"].length);
        }
        if (savegame["title"] == null) {
          swal("Loading game failed (savegame title is null)");
          return;
        }
        if (username == null || savegame["username"] == null) {
          swal("Loading game failed (username is null)");
	  return;
        }
        $.ajax({
          url: "/loadservlet?username=" + savegame["username"] + "&savename=" + savegame["title"],
          type: "POST",
          data: savegame["file"],
          processData: false
        }).done(function() {
          console.log("Upload of savegame complete");
          loadTimerId = setTimeout("load_game_real('" + username + "');", 1500);
          set_metamessage_on_loaded_game(savegame["game_type"]);
        }).fail(function() { swal("Loading game failed (ajax request failed)"); });
      }
    }
  } else if (scenario == "true" && $.getUrlVar('load') != "tutorial") {
    show_scenario_dialog();
  } else if ($.getUrlVar('action') == "load") {
    show_load_game_dialog();
  }

}


/**************************************************************************
 Send a load game command, if requested by user.
**************************************************************************/
function load_game_real(filename)
{
      console.log("Server command: /load " + filename );
      send_message("/load " + filename);
      $.unblockUI();
}


/**************************************************************************
...
**************************************************************************/
function set_metamessage_on_loaded_game(game_type)
{
  if (game_type == "multi") {
    metamessage_changed = true;
    send_message("/metamessage Multiplayer game loaded by " + username);
    loaded_game_type = game_type;
  } else if (game_type == "hotseat") {
    hotseat_enabled = true;
    loaded_game_type = game_type;
  }

}


/**************************************************************************
 Aitoggle and take first player.
**************************************************************************/
function load_game_toggle()
{
  if (players[0] == null) {
    setTimeout(load_game_toggle,1000);
  } else {

    var firstplayer = players[0]['name'].split(" ")[0];

    send_message("/aitoggle " + firstplayer);
    send_message("/take " + firstplayer);
    $.unblockUI();

  }

}




/**************************************************************************
 Show the select scenario dialog.
**************************************************************************/
function show_scenario_dialog()
{

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");
  $.unblockUI();

  var saveHtml =  "<ol id='selectable'>";
    for (var i = 0; i < scenarios.length; i++) {
      saveHtml += "<li class='ui-widget-content'><img border='0' src='" + scenarios[i]['img']
	       +  "' style='padding: 4px;' ><br>" + scenarios[i]['description'] + "</li>";
    }

  saveHtml += "</ol>";

  $("#dialog").html(saveHtml);
  $("#dialog").attr("title", "Select a scenario to play:");
  $("#selectable").css("height", $(window).height() - 180);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "40%",
			position: {my: 'center bottom', at: 'center bottom', of: window},
			buttons: {
                                "Cancel" : function() {
					$("#dialog").dialog('close');
                                },
                                "Create map from image upload.." : function() {
                                  $("#dialog").dialog('close');
                                  show_map_from_image_dialog();
                                },
	  			"Select scenario": function() {
	  			    if ($('#selectable .ui-selected').index() == -1) {
	  			        swal("Please select a scenario first.");
	  			    } else {
                        scenario_activated = true;
                        load_game_check();
                        $("#dialog").dialog('close');
                        $("#game_text_input").blur();
					}
				}
			}
		});
  $("#selectable").selectable();
  $("#dialog").dialog('open');
  $("#game_text_input").blur();

}