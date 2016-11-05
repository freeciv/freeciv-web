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

/**
 * Save game support versjon 1 - DEPRECATED!
 */

var savename = "";
var loadTimerId = -1;
var scenario_activated = false;

var FC_SAVEGAME_VER = 2;

var loaded_game_type = null;

var scenarios = [
  {"img":"/images/world_small.png", "description":"The World - Small world map, 80x50 map of the Earth", "savegame":"earth-80x50-v3"},
  {"img":"/images/world_big.png", "description":"The World - Large world map, 160x90 map of the Earth", "savegame":"earth-160x90-v2"},
  {"img":"/images/iberian.png", "description":"Iberian Peninsula - 136x100 map of Spain and Portugal", "savegame":"iberian-peninsula-136x100-v1.0"},
  /*{"img":"/images/europe1901.png", "description":"Europe WWI scenario 1901", "savegame":"europe_1901"},*/
  {"img":"/images/france.png", "description":"France - Large (140x90)", "savegame":"france-140x90-v2"},
  {"img":"/images/japan.png", "description":"Japan - Medium (88x100)", "savegame":"japan-88x100-v1.3"},
  {"img":"/images/italy.png", "description":"Italy - Medium (100x100)", "savegame":"italy-100x100-v1.5"},
  {"img":"/images/america.png", "description":"North America - 116x100 map of North America", "savegame":"north_america_116x100-v1.2"},
  {"img":"/images/british.png", "description":"British Aisles - Medium (85x80)", "savegame":"british-isles-85x80-v2.80"},
  {"img":"/images/hagworld.png", "description":"The World - Classic-style 120x60 map of the Earth", "savegame":"hagworld-120x60-v1.2"},
  {"img":"/images/europe.png", "description":"Very large map of Europe, 200x100", "savegame":"europe-200x100-v2"}
];

var scenario_info = null;

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
    loadTimerId = setTimeout("load_game_real('" + $.getUrlVar('savegame') + "');",
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
 Check for duplicate savegame name
**************************************************************************/
function check_savegame_duplicate(new_savename)
{
  var savegames = simpleStorage.get("savegames");
  if (savegames == null) return false;
  for (var i = 0; i < savegames.length; i++) {
    var savename = savegames[i]['title'];
    if (savename == new_savename) return true;
  }
  return false;
}


/**************************************************************************
 Load game
 (deprecated)
**************************************************************************/
function old_load_game_dialog()
{

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var saveHtml =  "<h3>This is the old savegame system. It will be removed November 12th 2016.</h3>"
  + "From now on savegames will be stored on the server rather than in the HTML5 localstorage. New savegames will be in the new format.<br><ol id='selectable'>";

  var savegames = simpleStorage.get("savegames");

  if (savegames == null || savegames.length == 0) {
      saveHtml = "<b>No savegames found. Please start a new game.</b>";

  } else {
    for (var i = 0; i < savegames.length; i++) {
      var savename = savegames[i]["title"];
      var username = savegames[i]["username"];
      var datetime = savegames[i]["datetime"];
      if (savename != null && savegames[i]["file"] != null) {
        saveHtml += "<li class='ui-widget-content'>" + savename + " "
                 + datetime + " (" + username + ") </li>";
      }
    }
  }

  saveHtml += "</ol><br><span id='savegame_note'>Savegame usage: "
           + Math.floor(simpleStorage.storageSize() / 1000)
           + " of 5200 kB.<br>Note: Savegames are stored using HTML5 local storage in your browser."
	   + " Clearing your browser cache will also clear your savegames."
           + " Savegames are stored with your username.</span>";

  var dialog_buttons = {};

  if (C_S_RUNNING != client_state()) {
    dialog_buttons = $.extend(dialog_buttons,
     {"Load Savegame": function() {
		  var load_game_id = $('#selectable .ui-selected').index();
		  if (load_game_id == -1) {
		    swal("Unable to load savegame: no game selected.");
		  } else {
		    load_game_check();
		    $("#dialog").dialog('close');
		    $("#game_text_input").blur();
		  }
    },
     "Load Scenarios...": function() {
		  $("#dialog").dialog('close');
		  $("#game_text_input").blur();
		  show_scenario_dialog();
    },
    "Map from image..." : function() {
		  $("#dialog").dialog('close');
		  $("#game_text_input").blur();
                  show_map_from_image_dialog();
    }
    });
  }


  dialog_buttons = $.extend(dialog_buttons,
  {
  "Download": function() {
    var load_game_id = $('#selectable .ui-selected').index();
    if (load_game_id == -1) {
      swal("Unable to download savegame: no game selected.");
    } else {
      download_savegame_locally(load_game_id);
    }
  },
  "Upload": function() {
    upload_savegame_locally();
  },
  "Delete oldest": function() {
    delete_oldest_savegame();
    $("#dialog").dialog('close');
    old_load_game_dialog();
  },
  "Delete ALL": function() {
    var r = confirm("Do you really want to delete your savegames?");
    if (r) {
      $("#dialog").dialog('close');
      $("#game_text_input").blur();
      simpleStorage.flush();
    }
  }
  });

  $("#dialog").html(saveHtml);
  $("#dialog").attr("title", "Savegames");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "70%",
			buttons: dialog_buttons
		});
  $("#selectable").selectable();
  $("#dialog").dialog('open');
  $("#game_text_input").blur();

  var tooltips = [];
  if (C_S_RUNNING != client_state()) {
    tooltips.push('Load the selected savegame and starts the game.');
    tooltips.push('Shows the load scenadio dialog');
  }
  tooltips.push('Download the selected savegame to your local device');
  tooltips.push('Upload a savegame from your local device to play online');
  tooltips.push('Delete the oldest savegame');
  tooltips.push('Delete all savegames.');

  for (var i = 0; i < tooltips.length; i++) {
    $('.ui-dialog-buttonpane button').eq(i).attr('title', tooltips[i]);
  }
  $('.ui-dialog-buttonpane button').eq(0).focus();
  $('.ui-dialog-buttonpane button').tooltip();

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

/**************************************************************************
 Deletes the oldest savegame
**************************************************************************/
function delete_oldest_savegame()
{
  var savegames = simpleStorage.get("savegames");
  if (savegames == null || savegames.length == 0) {
    swal("No savegames.");
  } else {
    savegames.shift();
    simpleStorage.set("savegames", savegames);
  }

}

/**************************************************************************
  Allows the user to download a savegame to their own computer
**************************************************************************/
function download_savegame_locally(game_id)
{
  var savegames = simpleStorage.get("savegames");
  if (savegames == null || savegames.length == 0) {
    swal("No savegames.");
  } else {
    var blob = new Blob([JSON.stringify(savegames[game_id])],
                        {type: "text/plain;charset=utf-8"});
    saveAs(blob, "freecivweb-" + savegames[game_id]['title'] + ".js");
  }
}

/**************************************************************************
  Allows the user to upload a savegame from their own computer
**************************************************************************/
function upload_savegame_locally()
{

  // reset dialog page.
  $("#upload_dialog").remove();
  $("<div id='upload_dialog'></div>").appendTo("div#game_page");
  var html = "Select a savegame file: <input type='file' id='fileInput'>";
  $("#upload_dialog").html(html);
  $("#upload_dialog").attr("title", "Upload savegame:");
  $("#upload_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "55%" : "40%",
			buttons: {
	  			"Upload": function() {
				  handle_savegame_upload();
   				}
			}
		});
  $("#upload_dialog").dialog('open');



}

/**************************************************************************
 Handle uploded savegame (read savegame using FileReader API and create JSON)
**************************************************************************/
function handle_savegame_upload()
{
  var fileInput = document.getElementById('fileInput');
  var file = fileInput.files[0];

  if (!(window.FileReader)) {
    swal("Uploading files not supported");
    return;
  }

  var extension = file.name.substring(file.name.lastIndexOf('.'));
  console.log("Loading savegame of type: " + file.type + " with extention " + extension);

  if (extension == '.js' || extension == '.txt') {
    var reader = new FileReader();
    reader.onload = function(e) {
    var savegames = simpleStorage.get("savegames");
    if (savegames == null) savegames = [];
      var new_savegame = JSON.parse(reader.result);
      if (new_savegame != null && new_savegame['file'] != null
          && new_savegame['title'] != null && new_savegame['username'] != null
          && new_savegame['file'].length > 1000
          && new_savegame['file'].length < 10000000) {
        console.log("Loading savegame: " + new_savegame['title']
                    + " of length " + new_savegame['file'].length);
        savegames.push(new_savegame);
        simpleStorage.set("savegames", savegames);
      } else {
        swal("Savegame is invalid!");
        console.error("Savegame is invalid: " + new_savegame);
      }
    }

    reader.readAsText(file);
  } else {
    swal("Savegame file " + file.name + "  not supported: " + file.type);
    console.error("Savegame file not supported: " + file.type);
  }

  $("#upload_dialog").dialog('close');
  $("#dialog").dialog('close');
  setTimeout("old_load_game_dialog();", 1000);

}

