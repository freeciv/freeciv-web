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


var client = {};
client.conn = {};

var client_frozen = false;
var chatbox_active = true;

var chatbox_text = " ";
var chatbox_text_dirty = false;
var previous_scroll = 0;
var phase_start_time = 0;

var debug_active = false;
var autostart = false;

var username = null;

var fc_seedrandom = null;

var music_list = [ "battle-epic",
                   "andrewbeck-ancient",
                   "into_the_shadows",
                   "andrewbeck-stings",
                   "trap_a_space_odyssey_battle_for_the_planet",
                   "elvish-theme",
                   "cullambruce-lockhart-dawning_fanfare"];
var audio = null;
var audio_enabled = false;

var last_turn_change_time = 0;
var turn_change_elapsed = 0;
var dialog_close_trigger = "";
var chatbox_panel_toggle = false;

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
  $.blockUI.defaults['css']['backgroundColor'] = "#222";
  $.blockUI.defaults['css']['color'] = "#fff";
  $.blockUI.defaults['theme'] = true;

  if ($.getUrlVar('action') == "observe") {
    observing = true;
    $("#civ_tab").remove();
    $("#cities_tab").remove();
    $("#pregame_buttons").remove();
    $("#game_unit_orders_default").remove();
    $("#civ_dialog").remove();
  }

  //initialize a seeded random number generator
  fc_seedrandom = new Math.seedrandom('freeciv-web');

  init_mapview();
  game_init();
  control_init();

  timeoutTimerId = setInterval(update_timeout, 1000);

  update_game_status_panel();
  statusTimerId = setInterval(update_game_status_panel, 6000);
  chatboxTimerId = setInterval(update_chatbox, 500);

  if (overviewTimerId == -1) {
    overviewTimerId = setInterval(redraw_overview, OVERVIEW_REFRESH);
  }

  motd_init();

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
	    };
  }


  $('#tabs').tabs({ heightStyle: "fill" });
  $('#tabs').css("height", $(window).height());
  $(".button").button();

  /* Initialze audio.js music player */
  audiojs.events.ready(function() {
    var as = audiojs.createAll({
          trackEnded: function() {
            if (!supports_mp3()) {
              audio.load("/music/" + music_list[Math.floor(Math.random() * music_list.length)] + ".ogg");
            } else {
              audio.load("/music/" + music_list[Math.floor(Math.random() * music_list.length)] + ".mp3");
            }
            audio.play();
          }
        });
    audio = as[0];

 });

}

/**************************************************************************
 Shows a intro dialog depending on game type.
**************************************************************************/
function init_common_intro_dialog() {
  if (observing) {
    show_intro_dialog("Welcome to Freeciv-web",
      "You have joined the game as an observer. Please enter your name:");
    $("#turn_done_button").button( "option", "disabled", true);

  } else if ($.getUrlVar('action') == "pbem") {
    show_pbem_dialog();

  } else if ($.getUrlVar('action') == "hotseat") {
    show_hotseat_dialog();

  } else if (is_small_screen()) {
    show_intro_dialog("Welcome to Freeciv-web",
      "You are about to join the game. Please enter your name:");
  } else if ($.getUrlVar('action') == "earthload") {
    show_intro_dialog("Welcome to Freeciv-web",
      "You can now play Freeciv-web on the earth map you have chosen. " +
      "Please enter your name: ");

  } else if ($.getUrlVar('action') == "load") {
    show_intro_dialog("Welcome to Freeciv-web",
      "You are about to join this game server, where you can " +
      "load a savegame, tutorial, custom map generated from an image or a historical scenario map. " +
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
      "play a singleplayer game against the Freeciv AI.<br> You can " +
      "start the game directly, or customize the game settings. " +
      "Please enter your name: ");
  }
}

/**************************************************************************
 ...
**************************************************************************/
function chatbox_resized()
{
  var newheight = $("#game_chatbox_panel").parent().height() - 50;
  $("#game_message_area").css("height", newheight);

}


/**************************************************************************
 ...
**************************************************************************/
function init_chatbox()
{

  chatbox_active = true;

  $("#game_chatbox_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "25%",
			height: "auto",
			resizable: true,
			dialogClass: 'chatbox_dialog no-close',
			resize: chatbox_resized,
			closeOnEscape: false,
			position: {my: 'left bottom', at: 'left bottom', of: window, within: $("#game_page")},
			close: function(event, ui) { chatbox_active = false;}
		});

  $("#game_chatbox_panel").dialog('open');
  $(".chatbox_dialog").css("top", "52px");


  if (is_small_screen()) {
    $(".chatbox_dialog").css("left", "2px");
    $(".chatbox_dialog").css("top", "37px");
    $("#game_chatbox_panel").parent().css("max-height", "22%");
    $("#game_chatbox_panel").parent().css("width", "95%");
    chatbox_resized();

    $("#game_message_area").click(function(e) {
      if (chatbox_panel_toggle) {
        $("#game_chatbox_panel").parent().css("max-height", "22%");
        $("#game_chatbox_panel").parent().css("height", "22%");
      } else {
        $("#game_chatbox_panel").parent().css("max-height", "65%");
        $("#game_chatbox_panel").parent().css("height", "65%");
      }
      chatbox_resized();
      chatbox_panel_toggle = !chatbox_panel_toggle;
    });
  }
}

/**************************************************************************
 This adds new text to the main message chatbox. See update_chatbox() which
 does the actual update to the screen.
**************************************************************************/
function add_chatbox_text(text)
{
    var scrollDiv;

    if (civclient_state <= C_S_PREPARING) {
      text = text.replace(/#FFFFFF/g, '#000000');
    }

    chatbox_text += text + "<br>";
    chatbox_text_dirty = true;

}


/**************************************************************************
 This is called at regular time intervals to update the chatbox text window
 with new messages. It is updated at interals for performance reasons
 when many messages appear.
**************************************************************************/
function update_chatbox()
{
  if (civclient_state <= C_S_PREPARING) {
      scrollDiv = document.getElementById('pregame_message_area');
  } else {
      scrollDiv = document.getElementById('game_message_area');
  }

  if (chatbox_text_dirty && scrollDiv != null && chatbox_text != null) {
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
      chatbox_text_dirty = false;
  }

}

/**************************************************************************
 Clips the chatbox text to a maximum number of lines.
**************************************************************************/
function chatbox_clip_messages()
{
  var max_chatbox_lines = 24;

  var new_chatbox_text = "";
  var chat_lines = chatbox_text.split("<br>");
  if (chat_lines.length <=  max_chatbox_lines) return;
  for (var i = chat_lines.length - max_chatbox_lines; i < chat_lines.length; i++) {
    new_chatbox_text += chat_lines[i];
    if ( i < chat_lines.length - 1) new_chatbox_text += "<br>";
  }

  chatbox_text = new_chatbox_text;

  update_chatbox();

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
  //quick fix to put the dialog on top of everything else.
  setTimeout("$('#dialog').parent().css('z-index', 2000)", 50);

  // automatically close dialog after 9 seconds, because sometimes the dilaog can't be closed manually.
  setTimeout("$('#dialog').dialog('close'); $('#game_text_input').blur();", 9000);

  speak(title);
  speak(message);

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

  var intro_html = message + "<br><br>Player name: <input id='username_req' type='text' size='25' maxlength='31'>"
	  + " <br><br><span id='username_validation_result'></span>";
  $("#dialog").html(intro_html);
  var stored_username = simpleStorage.get("username", "");
  if (stored_username != null && stored_username != false) {
    $("#username_req").val(stored_username);
  }
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "60%",
			beforeClose: function( event, ui ) {
			  // if intro dialog is closed, then check the username and connect to the server.
			  if (dialog_close_trigger != "button") {
			    if (validate_username()) {
			      if (!is_touch_device()) $("#pregame_text_input").focus();
			      return true;
			    } else {
			      return false;
			    }
			  }
			},
			buttons:
			{
				"Start Game" : function() {
					dialog_close_trigger = "button";
					autostart = true;
					if (validate_username()) {
						$("#dialog").dialog('close');
					}
				},
				  "Customize Game": function() {
					dialog_close_trigger = "button";
					if (validate_username()) {
					  if (!is_touch_device()) $("#pregame_text_input").focus();
					  $("#dialog").dialog('close');
					}
				}
			}

		});

  if (($.getUrlVar('action') == "load" || $.getUrlVar('action') == "multi" || $.getUrlVar('action') == "earthload")
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

  blur_input_on_touchdevice();
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
  if (game_info != null
      && current_turn_timeout() != null && current_turn_timeout() > 0) {
    var remaining = current_turn_timeout()
                    - Math.floor((now - phase_start_time) / 1000);
    if (remaining >= 0 && turn_change_elapsed == 0) {
      if (is_small_screen()) {
        $("#turn_done_button").button("option", "label", "Turn " + remaining);
        $("#turn_done_button .ui-button-text").css("padding", "3px");
      } else {
        $("#turn_done_button").button("option", "label", "Turn Done (" + remaining + "s)");
      }
      if (!is_touch_device()) $("#turn_done_button").tooltip({ disabled: false });
    }
  }
}

/**************************************************************************
 shows the remaining time of the turn change on the turn done button.
**************************************************************************/
function update_turn_change_timer()
{
  turn_change_elapsed += 1;
  if (turn_change_elapsed < last_turn_change_time) {
    setTimeout(update_turn_change_timer, 1000);
    $("#turn_done_button").button("option", "label", "Please wait (" 
        + (last_turn_change_time - turn_change_elapsed) + ")");
  } else {
    turn_change_elapsed = 0;
    $("#turn_done_button").button("option", "label", "Turn Done"); 
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
  send_message("/observe ");
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
    send_message("/surrender ");
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
  var savegames = simpleStorage.get("savegames");
  if (savegames == null) savegames = [];
  console.log("savegame count: " + savegames.length);
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
  for (var j = 0; j < debug_client_speed_list.length; j++) {
    sum += debug_client_speed_list[j];
    if (debug_client_speed_list[j] > max) max = debug_client_speed_list[j];
  }
  console.log("Network PING average (client): " + (sum / debug_client_speed_list.length) + " ms.  (Max: " + max +"ms.)");

}

/**************************************************************************
  This function can be used to display a message of the day to users.
  It is run on startup of the game, and every 30 minutes after that.
  The /motd.js Javascript file is fetched using AJAX, and executed
  so it can run any Javascript code. See motd.js also.
**************************************************************************/
function motd_init()
{
  $.getScript("/motd.js");
  setTimeout(motd_init, 1000*60*30);
}

/**************************************************************************
 Shows the authentication and password dialog.
**************************************************************************/
function show_auth_dialog(packet) {

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var intro_html = packet['message']
      + "<br><br> Password: <input id='password_req' type='text' size='25'>";
  $("#dialog").html(intro_html);
  $("#dialog").attr("title", "Private server needs password to enter");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "60%",
			buttons:
			{
				"Ok" : function() {
                                  var pwd_packet = {"pid" : packet_authentication_reply, "password" : $('#password_req').val()};
                                  var myJSONText = JSON.stringify(pwd_packet);
                                  send_request(myJSONText);

                                  $("#dialog").dialog('close');
				}
			}
		});


  $("#dialog").dialog('open');


}
