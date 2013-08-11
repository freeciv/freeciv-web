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

/**************************************************************************
 Send a load game command, if requested by user.
 uses HTML5 local storage.
**************************************************************************/
function load_game_check()
{
  var load_game_id = $.getUrlVar('load');
  var scenario = $.getUrlVar('scenario');
 
  if (load_game_id != null) {
    $.blockUI();

    if (scenario == "true") {
      loadTimerId = setTimeout("load_game_real('" + load_game_id + "');", 
                             1500);
      setTimeout(load_game_toggle,3500);
    } else {

      var savefile = $.jStorage.get("savegame-file-" + load_game_id);
      var savename = $.jStorage.get("savegame-savename-" + load_game_id);
      var saveusr = $.jStorage.get("savegame-username-" + load_game_id);

      if (savefile != null && savename != null && username != null) {
	$.ajax({
          url: "/loadservlet?username=" + saveusr + "&savename=" + savename,
          type: "POST",
          data: savefile,
          processData: false
    	}).done(function() {
          loadTimerId = setTimeout("load_game_real('" + username + "');", 
                             1000);
        }).fail(function() { alert("Loading game failed"); });
      } else {
        alert("Loading game failed");
      }
    }
  }
 
}


/**************************************************************************
 Send a load game command, if requested by user.
**************************************************************************/
function load_game_real(filename)
{
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
  if (!$.jStorage.storageAvailable()) {
    alert("HTML5 Storage not available");
    return;
  }

  keyboard_input = false;
  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var dhtml = "<table>" +
  	  "<tr><td>Savegame name:</td>" +
	  "<td><input type='text' name='savegamename' id='savegamename' size='20' maxlength='32'></td></tr>" +
	  "</td></tr></table><br>" +
	  "<span id='settings_info'><i>Freeciv-web allows you to save games. Games are stored in your web browser using HTML5 localstorage. Saved games will be stored in your browser until you clear your browser cache. Savegames are tied to your username " + username + ".</i></span>" 


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
					if (savename == null || savename.length < 4 || savename.length > 32) {
						alert("Invalid savegame name.");
					} else if (check_savegame_duplicate(savename)) {
						alert("Savegame name already in use. "
						 + "Please use a new savegame name.");
					} else {
						$(this).dialog('close');
						save_game_send();
                                                $.blockUI();
					}
				}
			}
		});
	
  $("#dialog").dialog('open');		

}
/**************************************************************************
 Check for duplicate savegame name
**************************************************************************/
function check_savegame_duplicate(new_savename)
{
  var savegame_count = $.jStorage.get("savegame-count", 0);
  for (var i = 1; i <= savegame_count; i++) {
    var savename = $.jStorage.get("savegame-savename-" + i);
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
      var savegame_count = $.jStorage.get("savegame-count", 0) + 1;
      $.jStorage.set("savegame-file-" + savegame_count, saved_file);
      $.jStorage.set("savegame-savename-" + savegame_count, savename);
      $.jStorage.set("savegame-username-" + savegame_count, username);
      $.jStorage.set("savegame-count" , savegame_count);
      $.unblockUI();
      alert("Game saved successfully");
    }).fail(function() { alert("Failed saving game"); });
}
