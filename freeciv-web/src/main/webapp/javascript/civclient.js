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

/**************************************************************************
 This function is called on page load. 
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

  // Register keyboard listener using JQuery.
  $(document).keydown (keyboard_listener);

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

  $('#tabs').tabs();
  $(".button").button();

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
function chatbox_resized()
{
  var newheight = $("#game_chatbox_panel").parent().height() - 58;
  $("#game_message_area").css("height", newheight);

}


/**************************************************************************
 ...
**************************************************************************/
function init_chatbox()
{

  chatbox_active = true;

  $("#game_chatbox_panel").attr("title", "Messages");		
  $("#game_chatbox_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "60%",
			height: "auto",
			resizable: true,
			dialogClass: 'chatbox_dialog',
			resize: chatbox_resized,
			position: ["center", "top"],
			close: function(event, ui) { chatbox_active = false;}
		});
	
  $("#game_chatbox_panel").dialog('open');		
  $(".chatbox_dialog div.ui-dialog-titlebar").css("height", "5px");
  $(".chatbox_dialog div.ui-dialog-content").css("padding", "5px 0");
  $("#ui-dialog-title-game_chatbox_panel").css("margin-top", "-5px");
  $("#ui-dialog-title-game_chatbox_panel").css("font-size", "10px");
  $("#game_chatbox_panel").parent().css("background", "rgba(0,0,0,0.6)")		
  $("#game_chatbox_panel").parent().css("background-color", "rgba(0,0,0, 0.6)")		
  $("#game_chatbox_panel").parent().css("top", "60px");


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
      $("#turn_done_button").button("option", "label", "Turn Done (" + remaining + "s)");
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
  		
    	
  $(".manual-tab").height( mapview['height'] - 200);   

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
    var test_packet = [{"packet_type" : "chat_msg_req", 
                         "message" : "/surrender "}];
    var myJSONText = JSON.stringify(test_packet);
    send_request (myJSONText);
  }
}

/**************************************************************************
...
**************************************************************************/
function prepare_share_game_map()
{
  $("#share_button").html("Please wait while generating image...");

  setTimeout("share_game_map();", 100);
}

/**************************************************************************
...
**************************************************************************/
function share_game_map()
{
  var fullmap_canvas = document.createElement('canvas');
  var w = map['xsize'] * tileset_tile_width * 0.8;
  var h = map['ysize'] * tileset_tile_height * 1.1;

  fullmap_canvas.width = w;
  fullmap_canvas.height = h;

  mapview['width'] = w;
  mapview['height'] = h; 
  mapview['store_width'] = w;
  mapview['store_height'] = h;

  center_tile_mapcanvas(map_pos_to_tile(map['xsize']/2, map['ysize']/2));

  mapview_canvas = fullmap_canvas;
  mapview_canvas_ctx = mapview_canvas.getContext("2d");
    
  if (!has_canvas_text_support) {
    CanvasTextFunctions.enable(mapview_canvas_ctx);
  }
    
  update_map_canvas_full();

  // save mapview canvas image as PNG!
  var mapImageData = Canvas2Image.saveAsPNG(mapview_canvas, true);
  mapImageData.id = "map_image_data";
  $("#map_image").append(mapImageData);
  $("#map_image_data").css("width", 500);
  $("#map_image_data").css("height", 300);
  $("#map_image").append($("<p>Right click on image to save it</p>"));

  // we're done. set default mapview canvas active again.
  setup_window_size ();
  set_default_mapview_active();
  $("#share_button").html("Image ready.");
  allow_right_click = true;

}


