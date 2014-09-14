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
var chatbox_active = true;

var chatbox_text = " ";
var previous_scroll = 0;
var phase_start_time = 0;

var debug_active = false;
var autostart = false;

var username = null;

/**************************************************************************
 Main starting point for Freeciv-web 
**************************************************************************/
$(document).ready(function() {

  if ($("#canvas").length == 1) {
    civclient_init();
  } else {
    console.log("Freeciv-web not started. missing canvas element in body.");
  }
});

/**************************************************************************
 This function is called on page load. 
**************************************************************************/
function civclient_init() 
{
  https_redirect_check();
   
  $.blockUI.defaults['css']['backgroundColor'] = "#222";
  $.blockUI.defaults['css']['color'] = "#fff";
  $.blockUI.defaults['theme'] = true;

  if ($.getUrlVar('action') == "observe") observing = true;

  game_init();
  init_mapview();
  control_init();

  timeoutTimerId = setInterval("update_timeout()", 1000);
  
  update_game_status_panel();
  statusTimerId = setInterval("update_game_status_panel()", 6000);

  if (overviewTimerId == -1) {
    overviewTimerId = setInterval("redraw_overview()", OVERVIEW_REFRESH);
  }

  // Tells the browser that you wish to perform an animation; this 
  // requests that the browser schedule a repaint of the window for the 
  // next animation frame.
  window.requestAnimFrame = (function(){
      return  window.requestAnimationFrame       || 
              window.webkitRequestAnimationFrame || 
              window.mozRequestAnimationFrame    || 
              window.oRequestAnimationFrame      || 
              window.msRequestAnimationFrame     || 
              function(/* function */ callback, /* DOMElement */ element){
                window.setTimeout(callback, MAPVIEW_REFRESH_INTERVAL);
              };
    })();
 
  requestAnimFrame(update_map_canvas_check, mapview_canvas);


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


  $('#tabs').tabs({ heightStyle: "fill" });
  $('#tabs').css("height", $(window).height());
  $(".button").button();
}

/**************************************************************************
 ...
**************************************************************************/
function init_common_intro_dialog() {
  if (observing) {
    show_intro_dialog("Welcome to Freeciv-web", 
      "You have joined the game as an observer. Please enter your name:");
    $("#turn_done_button").button( "option", "disabled", true); 

  } else if (is_small_screen()) {
    show_intro_dialog("Welcome to Freeciv-web", 
      "You are about to join the game. Please enter your name:");
  } else if ($.getUrlVar('action') == "load") {
    show_intro_dialog("Welcome to Freeciv-web",
      "You are about to join this game server, where you can " +
      "load a savegame, tutorial or scenario. " +
      "Please enter your name: ");

  } else if ($.getUrlVar('action') == "multi") {
    show_intro_dialog("Welcome to Freeciv-web",
      "You are about to join this game server, where you can "  +
      "participate in a multiplayer game. You can customize the game " +
      "settings, and wait for the minimum number of players before " +
      "the game can start. " +
      "Please enter your name: ");
  } else {
    show_intro_dialog("Welcome to Freeciv-web",
      "You are about to join this game server, where you can " +
      "play a singleplayer game against the Freeciv AI. You can " +
      "start the game directly, or customize the game settings. " +
      "Please enter your name: ");
  } 
}

/**************************************************************************
 ...
**************************************************************************/
function chatbox_resized()
{
  if (is_small_screen()) {
    $("#game_message_area").css("height", "75%");

  } else {
    var newheight = $("#game_chatbox_panel").parent().height() - 40;
    $("#game_message_area").css("height", newheight);
  }
}


/**************************************************************************
 ...
**************************************************************************/
function init_chatbox()
{

  chatbox_active = true;

  if (is_small_screen()) {
    $("#game_chatbox_panel").detach().prependTo("#tabs-chat");
  }

  $("#game_chatbox_panel").attr("title", "Messages");		
  $("#game_chatbox_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "60%",
			height: "auto",
			resizable: true,
			dialogClass: 'chatbox_dialog',
			resize: chatbox_resized,
			position: {my: 'center bottom', at: 'center bottom', of: window},
			close: function(event, ui) { chatbox_active = false;}
		});
	
  $("#game_chatbox_panel").dialog('open');		
  $(".chatbox_dialog").css("top", "60px");


  if (is_small_screen()) {
    $("#game_chatbox_panel").css("position", "relative");
    $("#game_chatbox_panel").css("float", "left");
    $("#game_chatbox_panel").css("height", "75%");
    $("#game_chatbox_panel").css("width", "99%");
    $("#game_chatbox_panel").show();
    chatbox_resized();
  }
}

/**************************************************************************
 ...
**************************************************************************/
function add_chatbox_text(text)
{
    var scrollDiv;
    
    if (civclient_state <= C_S_PREPARING) {
      scrollDiv = document.getElementById('pregame_message_area');
    } else {
      scrollDiv = document.getElementById('game_message_area');
    }

    chatbox_text += text + "<br>";

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


/**************************************************************************
 ...
**************************************************************************/
function chatbox_scroll_down () 
{
    var scrollDiv;
    
    if (civclient_state <= C_S_PREPARING) {
      scrollDiv = document.getElementById('pregame_message_area');
    } else {
      scrollDiv = document.getElementById('game_message_area');
    }

    if (scrollDiv != null) {
      var currentHeight = 0;
        
      if (scrollDiv.scrollHeight > 0) {
        currentHeight = scrollDiv.scrollHeight;
      } else if (scrollDiv.offsetHeight > 0) {
        currentHeight = scrollDiv.offsetHeight;
      }

      scrollDiv.scrollTop = currentHeight;

      previous_scroll = currentHeight;
    }

}

/**************************************************************************
 Shows a generic message dialog.
**************************************************************************/
function show_dialog_message(title, message) {

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "50%",
			buttons: {
				Ok: function() {
					$("#dialog").dialog('close');
					$("#game_text_input").blur();
				}
			}
		});
	
  $("#dialog").dialog('open');		
  $("#game_text_input").blur();

}


/**************************************************************************
 ...
**************************************************************************/
function validate_username() {
  username = $("#username_req").val();

  var cleaned_username = username.replace(/[^a-zA-Z]/g,'');
 
  if (username == null || username.length == 0) {
    $("#username_validation_result").html("Your name can't be empty.");
    return false;
  } else if (username.length <= 2 ) {
    $("#username_validation_result").html("Your name is too short.");
    return false;
  } else if (username.length >= 32) {
    $("#username_validation_result").html("Your name is too long.");
    return false;
  } else if (username != cleaned_username) {
    $("#username_validation_result").html("Your name contains invalid characters, only the English alphabet is allowed.");
    return false;
  }

  simpleStorage.set("username", username);
  network_init();

  return true;
}

/**************************************************************************
 Shows the Freeciv intro dialog.
**************************************************************************/
function show_intro_dialog(title, message) {

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var intro_html = message + "<br><br>Player name: <input id='username_req' type='text' size='25'>"
	  + " <br><br><span id='username_validation_result'></span>";
  $("#dialog").html(intro_html);
  $("#username_req").val(simpleStorage.get("username", ""));
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "40%",
			buttons: 
			{
				"Start Game" : function() {
					autostart = true;
					if (validate_username()) {
						$("#dialog").dialog('close');
					}
				}, 
				  "Customize Game": function() {
					if (validate_username()) {
					  $("#pregame_text_input").focus();
					  $("#dialog").dialog('close');
					}
				}
			}	
			
		});

  if (($.getUrlVar('action') == "load" || $.getUrlVar('action') == "multi") 
         && $.getUrlVar('load') != "tutorial") {
    $(".ui-dialog-buttonset button").first().hide();
  }
  if ($.getUrlVar('action') == "observe") {
    $(".ui-dialog-buttonset button").first().remove();
    $(".ui-dialog-buttonset button").first().button("option", "label", "Observe Game");
  }

  if (is_small_screen()) {
    /* some fixes for pregame screen on small devices.*/
    $("#freeciv_logo").remove();
    $("#pregame_message_area").css("width", "73%");
    $("#observe_button").remove();
  }
	
  $("#dialog").dialog('open');		

  $('#dialog').keyup(function(e) {
    if (e.keyCode == 13) {
      autostart = true;
      if (validate_username()) {
        $("#dialog").dialog('close');
        $("#dialog").remove();
      }
    }
  });

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
      if (is_small_screen()) {
        $("#turn_done_button").button("option", "label", "Turn" + remaining);
        $("#turn_done_button .ui-button-text").css("padding", "3px");
      } else {
        $("#turn_done_button").button("option", "label", "Turn Done (" + remaining + "s)");
      }
       $("#turn_done_button").tooltip({ disabled: false });
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
function request_observe_game()
{
  var test_packet = {"type" : packet_chat_msg_req, 
                         "message" : "/observe "};
  var myJSONText = JSON.stringify(test_packet);
  send_request (myJSONText);

}

/**************************************************************************
...
**************************************************************************/
function surrender_game()
{
  send_surrender_game();

  $("#tabs-map").removeClass("ui-tabs-hide");
  $("#tabs-opt").addClass("ui-tabs-hide");
  $("#map_tab").addClass("ui-state-active");
  $("#map_tab").addClass("ui-tabs-selected");
  $("#map_tab").removeClass("ui-state-default");

}

/**************************************************************************
...
**************************************************************************/
function send_surrender_game()
{
  if (!client_is_observer()) {
    var test_packet = {"type" : packet_chat_msg_req, 
                         "message" : "/surrender "};
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  }
}

/**************************************************************************
...
**************************************************************************/
function show_fullscreen_window()
{
  if (BigScreen.enabled) {
    BigScreen.toggle();
  } else {
   show_dialog_message('Fullscreen', 'Press F11 for fullscreen mode.');
  }

}

/**************************************************************************
...
**************************************************************************/
function show_debug_info()
{
  console.log("Freeciv version: " + freeciv_version);
  console.log("Browser useragent: " + navigator.userAgent);
  console.log("jQuery version: " + $().jquery);
  console.log("jQuery UI version: " + $.ui.version);
  console.log("simpleStorage version: " + simpleStorage.version);
  console.log("savegame count: " + simpleStorage.get("savegame-count"));
  console.log("Touch device: " + is_touch_device());
  console.log("HTTP protocol: " + document.location.protocol);
  if (ws != null && ws.url != null) console.log("WebSocket URL: " + ws.url);

  debug_active = true;
  /* Show average network latency PING (server to client, and back). */
  var sum = 0;
  var max = 0;
  for (var i = 0; i < debug_ping_list.length; i++) {
    sum += debug_ping_list[i];
    if (debug_ping_list[i] > max) max = debug_ping_list[i];
  }
  console.log("Network PING average (server): " + (sum / debug_ping_list.length) + " ms. (Max: " + max +"ms.)");

  /* Show average network latency PING (client to server, and back). */
  sum = 0;
  max = 0;
  for (var i = 0; i < debug_client_speed_list.length; i++) {
    sum += debug_client_speed_list[i];
    if (debug_client_speed_list[i] > max) max = debug_client_speed_list[i];
  }
  console.log("Network PING average (client): " + (sum / debug_client_speed_list.length) + " ms.  (Max: " + max +"ms.)");

  console.log("mozDash support: " + mozDashSupport);

}

/**************************************************************************
 - HTTPS is the default, so redirect from HTTP to HTTPS. 
 - HTTP default on localhost.
 - Also prevent redirect loops. 
**************************************************************************/
function https_redirect_check()
{
  if ('http:' == document.location.protocol && document.location.hostname != "localhost"
      && $.getUrlVar("http") != "true" && $.getUrlVar("https") != "true" ) {
    https_redirect();
  }

}
/**************************************************************************
  Redirect between the HTTP and HTTPS urls of Freeciv-web.
**************************************************************************/
function https_redirect()
{
  if ('https:' == document.location.protocol) {
    console.log("Redirecting from HTTPS to HTTP.");
    window.location.href = "http:" + window.location.href.substring(window.location.protocol.length) + "&http=true";
  } else if ('http:' == document.location.protocol) {
    console.log("Redirecting from HTTP to HTTPS.");
    window.location.href = "https:" + window.location.href.substring(window.location.protocol.length) + "&https=true";
  }
}

