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


var client = {};
client.conn = {};

var client_frozen = false;

var chatbox_text = " ";
var previous_scroll = 0;
var phase_start_time = 0;

/**************************************************************************
 ...
**************************************************************************/
function civclient_init() 
{

  setup_crash_reporting();

  if (window['loadFirebugConsole']) {
	window.loadFirebugConsole();
  } else {
	if (!window['console']) {
         console = {
            log: function() {},
            debug: function() {},
            info: function() {},
            warn: function() {},
            assert: function() {}
          }

	}
  }


  game_init();
  network_init();
  init_mapview();


  // Register keyboard listener.
  if(window.addEventListener){ // Mozilla, Netscape, Firefox
    document.addEventListener('keyup', keyboard_listener, false);
  } else { // IE
    document.attachEvent('onkeyup', keyboard_listener);
  }

  timeoutTimerId = setInterval("update_timeout()", 1000);
  
  update_game_status_panel();
  statusTimerId = setInterval("update_game_status_panel()", 6000);
  


  /*
   * Interner Explorer doesn't support Array.indexOf
   * http://soledadpenades.com/2007/05/17/arrayindexof-in-internet-explorer/
   */
  if(!Array.indexOf){
	    Array.prototype.indexOf = function(obj){
	        for(var i=0; i<this.length; i++){
	            if(this[i]==obj){
	                return i;
	            }
	        }
	        return -1;
	    }
  }


  load_game_check();
  observe_game_check();

  if (observing) {
    show_dialog_message("Welcome to Freeciv.net", 
      "Please wait while you are being logged in as an observer in the game.");
  } else if (!is_iphone()) { 
    show_dialog_message("Welcome to Freeciv.net", 
      "You have now joined the game. Before the game begins, " +
      "you can change game options or wait for more players " +
      "to join the game.  Type /help in the command line " +
      "below to find out how to customize the game. <br><br>" +  
      "Click the start game button to begin the game.");
  } else {
    autostartTimerId = setTimeout("iphone_autostart();", 1000);
    
  }

}

/**************************************************************************
 ...
**************************************************************************/
function add_chatbox_text(text){
    var scrollDiv;
    
    if (civclient_state <= C_S_PREPARING) {
      scrollDiv = document.getElementById('pregame_message_area');
    } else {
      scrollDiv = document.getElementById('game_message_area');
    }

    chatbox_text = chatbox_text + text + "<br>";

    if (scrollDiv != null) {
      scrollDiv.innerHTML = chatbox_text; 

      var currentHeight = 0;
        
      if (scrollDiv.scrollHeight > 0) {
        currentHeight = scrollDiv.scrollHeight;
      } else if (scrollDiv.offsetHeight > 0) {
        currentHeight = scrollDiv.offsetHeight;
      }

      if (previous_scroll < currentHeight) {
        scrollDiv.scrollTop = currentHeight;
      }

      previous_scroll = currentHeight;
    }

  }

function show_dialog_message(title, message) {

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "40%",
			buttons: {
				Ok: function() {
					$(this).dialog('close');
				}
			}
		});
	
  $("#dialog").dialog('open');		
}


/* Webclient is always client. */
function is_server()
{
  return false;
}

/**************************************************************************
 ...
**************************************************************************/
function update_timeout()
{
  var now = new Date().getTime();
  if (game_info != null && game_info['timeout'] != null && game_info['timeout'] > 0) {
    var remaining = game_info['timeout'] - Math.floor((now - phase_start_time) / 1000);
    if (remaining >= 0) {
      $("#turn_done_button").text("Turn Done (" + remaining + "s)");
    }
  } 
}

/**************************************************************************
 ...
**************************************************************************/
function set_phase_start()
{
  phase_start_time = new Date().getTime();
}

/**************************************************************************
 ...
**************************************************************************/
function show_help()
{
   $.ajax({url: "/manual",
   	       async: false, 
  		   success: function(msg){
			     	$("#game_manual").html(msg);
			     	$('#tabs_manual').tabs();
			      	}
  		});
  		
    		  
}


/**************************************************************************
 Determines if this is a observer game, and if so, sends an observer cmd.
**************************************************************************/
function observe_game_check()
{
  var mode_def = $.getUrlVar('mode');
  if (mode_def != null && mode_def == 'observe') {
    loadTimerId = setTimeout("request_observe_game();", 2000);
    observing = true;
  }
}

/**************************************************************************
...
**************************************************************************/
function request_observe_game()
{
  var test_packet = [{"packet_type" : "chat_msg_req", 
                         "message" : "/observe "}];
  var myJSONText = JSON.stringify(test_packet);
  send_request (myJSONText);

}
