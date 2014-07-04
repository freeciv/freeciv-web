/********************************************************************** 
 Freeciv - Copyright (C) 2010 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
var savename = "";


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
  } else if (load_game_id != -1) {
    $.blockUI();

    if (scenario == "true") {
      if (load_game_id == -1) {
        show_scenario_dialog();
      } else {
        scenario_game_id = scenarios[load_game_id]['savegame'];
        loadTimerId = setTimeout("load_game_real('" + scenario_game_id + "');", 
                                  1500);
        setTimeout(load_game_toggle,3500);
      }
    } else if (load_game_id != -1) {

      var savefile = simpleStorage.get("savegame-file-" + (load_game_id + 1));
      var savename = simpleStorage.get("savegame-savename-" + (load_game_id + 1));
      var saveusr = simpleStorage.get("savegame-username-" + (load_game_id + 1));
      if (savefile != null && savename != null && username != null) {
        console.log("Loading " + savename);
	$.ajax({
          url: "/loadservlet?username=" + saveusr + "&savename=" + savename,
          type: "POST",
          data: savefile,
          processData: false
    	}).done(function() {
          console.log("Upload of savegame complete");
          loadTimerId = setTimeout("load_game_real('" + username + "');", 
                             1500);
        }).fail(function() { alert("Loading game failed (ajax failed)"); });
      } else {
        alert("Loading game failed (simpleStorage)");
      }
    }
  } else if (scenario == "true" && $.getUrlVar('load') != "tutorial") {
    show_scenario_dialog();
  } else if ($.getUrlVar('action') == "load") {
    load_game_dialog();
  }
 
}


/**************************************************************************
 Send a load game command, if requested by user.
**************************************************************************/
function load_game_real(filename)
{
      console.log("Server command: /load " + filename );
      var test_packet = {"type" : packet_chat_msg_req, 
                         "message" : "/load " + filename};
      var myJSONText = JSON.stringify(test_packet);
      send_request (myJSONText);
      $.unblockUI();
}

/**************************************************************************
...
**************************************************************************/
function load_game_toggle()
{
  var test_packet = {"type" : packet_chat_msg_req, 
                         "message" : "/aitoggle AI*1"};

  send_request (JSON.stringify(test_packet));
  test_packet = {"type" : packet_chat_msg_req, 
                         "message" : "/take AI*1"};
  send_request (JSON.stringify(test_packet));
  $.unblockUI();

}

/****************************************************************************
  ...
****************************************************************************/
function save_game()
{
  if (!simpleStorage.canUse()) {
    show_dialog_message("Saving failed", "HTML5 Storage not available");
    return;
  }

  keyboard_input = false;
  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var dhtml = "<table>" +
  	  "<tr><td>Savegame name:</td>" +
	  "<td><input type='text' name='savegamename' id='savegamename' size='32' maxlength='64'></td></tr>" +
	  "</td></tr></table><br>" +
	  "<span id='settings_info'><i>Freeciv-web allows you to save games. Games are stored in your web " +
	  "browser using HTML5 localstorage. Saved games will be stored in your browser until you clear" +
	  " your browser cache. Savegames are tied to your username " + username + ".</i></span>" 


  $("#dialog").html(dhtml);
  $("#dialog").attr("title", "Save game");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "40%",
			buttons: {
				Save: function() {
					keyboard_input = true;
					savename = $("#savegamename").val()
					if (savename == null || savename.length < 4 || savename.length > 64) {
						alert("Invalid savegame name.");
					} else if (check_savegame_duplicate(savename)) {
						alert("Savegame name already in use. "
						 + "Please use a new savegame name.");
					} else {
						$("#dialog").dialog('close');
						save_game_send();
                                                $.blockUI();
					}
				}
			}
		});
  var pplayer = client.conn.playing;
  var save_cnt = simpleStorage.get("savegame-count");
  if (save_cnt == null) save_cnt = 0;
  var suggest_savename = username + " of the " + nations[pplayer['nation']]['adjective'] + " in the year " + get_year_string() + " [" 
                         + (save_cnt + 1) + "]";
  if (suggest_savename.length >= 64) suggest_savename = username + " [" 
                         + (save_cnt + 1) + "]";


  $("#savegamename").val(suggest_savename);	

  $("#dialog").dialog('open');		

}
/**************************************************************************
 Check for duplicate savegame name
**************************************************************************/
function check_savegame_duplicate(new_savename)
{
  var savegame_count = simpleStorage.get("savegame-count");
  if (savegame_count == null) savegame_count = 0;
  for (var i = 1; i <= savegame_count; i++) {
    var savename = simpleStorage.get("savegame-savename-" + i);
    if (savename == new_savename) return true;
  }
  return false;
}

/**************************************************************************
 Save the game
**************************************************************************/
function save_game_send()
{
  var test_packet = {"type" : packet_chat_msg_req, "message" : "/save"};
  var myJSONText = JSON.stringify(test_packet);
  send_request(myJSONText);
  sTimerId = setTimeout(save_game_fetch,  3000);

}


/**************************************************************************
 Get saved game from server.
 uses HTML5 local storage.
**************************************************************************/
function save_game_fetch()
{
  $.get("/saveservlet?username=" + username + "&savename=" + savename, 
    function(saved_file) {
      var savegame_count = simpleStorage.get("savegame-count");
      if (savegame_count == null) savegame_count = 0;
      savegame_count += 1;
      simpleStorage.set("savegame-file-" + savegame_count, saved_file);
      simpleStorage.set("savegame-savename-" + savegame_count, savename);
      simpleStorage.set("savegame-username-" + savegame_count, username);
      simpleStorage.set("savegame-count" , savegame_count);
      $.unblockUI();
      alert("Game saved successfully");
    }).fail(function() { 
	    alert("Failed saving game");
	    $.unblockUI();
    });
}


/**************************************************************************
 Load game
**************************************************************************/
function load_game_dialog()
{

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var saveHtml =  "<ol id='selectable'>";

  var savegame_count = simpleStorage.get("savegame-count");

  if (savegame_count == 0 || savegame_count == null) {
      saveHtml = "<b>No savegames found. Please start a new game.</b>";

  } else {
    for (var i = 1; i <= savegame_count; i++) {
      var savename = simpleStorage.get("savegame-savename-" + i);
      var username = simpleStorage.get("savegame-username-" + i);
      saveHtml += "<li class='ui-widget-content'>" + savename + " (" + username + ")</li>";
    }
  }


  saveHtml += "</ol><span id='savegame_note'>Note: Savegames are stored using HTML5 local storage in your browser. "+
	  "Clearing your browser cache will also clear your savegames. Savegames are stored with your username.</span>";

  $("#dialog").html(saveHtml);
  $("#dialog").attr("title", "Please select game to resume playing:");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "70%",
			buttons: {
				"Delete Savegames": function() {
					$("#dialog").dialog('close');
					$("#game_text_input").blur();
					simpleStorage.set("savegame-count" , 0);
				},
	  	  		"Load Scenario": function() {
					$("#dialog").dialog('close');
					$("#game_text_input").blur();
					show_scenario_dialog();
				},
	  			"Load Savegame": function() {
					load_game_check();
					$("#dialog").dialog('close');
					$("#game_text_input").blur();
				}
			}
		});
  $("#selectable").selectable();
  $("#dialog").dialog('open');		
  $("#game_text_input").blur();



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
	  			"Select scenario": function() {
					load_game_check();
					$("#dialog").dialog('close');
					$("#game_text_input").blur();
				}
			}
		});
  $("#selectable").selectable();
  $("#dialog").dialog('open');		
  $("#game_text_input").blur();

}
