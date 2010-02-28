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


/**************************************************************************
 Send a load game command, if requested by user.
**************************************************************************/
function load_game_check()
{
  var load_file_name = $.getUrlVar('load');
  if (load_file_name != null) {
    loadTimerId = setTimeout("load_game_real('" + load_file_name + "');", 
                             2000);
  }
}


/**************************************************************************
 Send a load game command, if requested by user.
**************************************************************************/
function load_game_real(filename)
{
    
    var scenario = $.getUrlVar('scenario');
    if (scenario == "true") {
      var test_packet = [{"packet_type" : "chat_msg_req", 
                         "message" : "/load " + filename},
                         {"packet_type" : "chat_msg_req", 
                         "message" : "/take AI*1"},
                         {"packet_type" : "chat_msg_req", 
                         "message" : "/aitoggle AI*1"}];    
    } else {
      var test_packet = [{"packet_type" : "chat_msg_req", 
                         "message" : "/load " + filename}];
    }
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
}

/**************************************************************************
 Show the save game dialog.
**************************************************************************/
function show_savegame_dialog()
{
  $.ajax({url: "/savegames/save.jsp", 
  		  success: function(msg){
			     	$("#save_game_dialog").html(msg);
			      	}
  		});
  		
    $("#save_button").hide();

}

/**************************************************************************
 Save the game
**************************************************************************/
function save_game()
{
  var save_name = $("#save_name").val();
  
  if (save_name == null || save_name.length < 5 || !alpha_numeric_check(save_name)) {
    $("#save_button").show();
    $("#save_game_dialog").html("Invalid save game name. The name can only consist of "
                    + "characters and numbers. It must also be at least 5 characters.");
    return;
  }
  
  if (save_name != null && save_name.length >= 5) {
    var test_packet = [{"packet_type" : "chat_msg_req", 
                 "message" : "/save " + username + "-savegame-" + save_name}];
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
    
    $("#save_button").show();
    $("#save_game_dialog").html("Game saved.");
  }
}

/**************************************************************************
 Check filename of savegame
**************************************************************************/
function alpha_numeric_check(savegame){
  var regex=/^[0-9A-Za-z]+$/; 
  if  (regex.test(savegame)){
    return true;
  } else {
    return false;
  }
}