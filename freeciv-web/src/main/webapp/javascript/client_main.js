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


var C_S_INITIAL = 0;    /* Client boot, only used once on program start. */
var C_S_PREPARING = 1;  /* Main menu (disconnected) and connected in pregame. */
var C_S_RUNNING = 2;    /* Connected with game in progress. */
var C_S_OVER = 3;       /* Connected with game over. */

var civclient_state = C_S_INITIAL;

var endgame_player_info = [];
var height_offset = 52;
var width_offset = 10;

/**************************************************************************
 Sets the client state (initial, pre, running, over etc).
**************************************************************************/
function set_client_state(newstate)
{
  var connect_error = (C_S_PREPARING == civclient_state)
      && (C_S_PREPARING == newstate);
  var oldstate = civclient_state;

  if (civclient_state != newstate) {
    civclient_state = newstate;

    switch (civclient_state) {
    case C_S_RUNNING:
      chatbox_text = " ";
      $.unblockUI();
      show_new_game_message();

      set_client_page(PAGE_GAME);
      setup_window_size();

      if (observing) center_tile_mapcanvas(map_pos_to_tile(15,15));
      update_metamessage_on_gamestart();
      deduplicate_player_colors();
      if (is_pbem()) {
        setTimeout(function () {
          set_human_pbem_players();
          advance_unit_focus();
        }, 1500);
      }

      if (ruleset_control['name'] == "Civ2Civ3 ruleset") {
        ROAD_ROAD = ROAD_ROAD_CIV2CIV3;
      }
      /* remove context menu from pregame. */
      $(".context-menu-root").remove();

      if (renderer == RENDERER_WEBGL) {
        init_webgl_mapview();
      }

      break;
    case C_S_OVER:
      setTimeout(show_endgame_dialog, 500);
      break;
    case C_S_PREPARING:
      if (is_small_screen() && $.getUrlVar('action') == "new") {
        // disable borders on mobile devices for performance reasons.
        send_message_delayed("/set borders disabled", 200);
      }
      break;
    default:
      break;
    }
  }


}

/**************************************************************************
  Refreshes the size of UI elements based on new window and screen size.
**************************************************************************/
function setup_window_size ()
{
  var winWidth = $(window).width();
  var winHeight = $(window).height();
  var new_mapview_width = winWidth - width_offset;
  var new_mapview_height = winHeight - height_offset;

  if (renderer == RENDERER_2DCANVAS) {
    mapview_canvas.width = new_mapview_width;
    mapview_canvas.height = new_mapview_height;
    buffer_canvas.width = Math.floor(new_mapview_width * 1.5);
    buffer_canvas.height = Math.floor(new_mapview_height * 1.5);

    mapview['width'] = new_mapview_width;
    mapview['height'] = new_mapview_height;
    mapview['store_width'] = new_mapview_width;
    mapview['store_height'] = new_mapview_height;

    mapview_canvas_ctx.font = canvas_text_font;
    buffer_canvas_ctx.font = canvas_text_font;
  }

  $("#game_status_panel").css("width", new_mapview_width);
  $("#pregame_message_area").height( new_mapview_height - 80
                                        - $("#pregame_game_info").getTotalHeight());
  $("#pregame_player_list").height( new_mapview_height - 80);
  $("#technologies").height( new_mapview_height - 50);
  $("#technologies").width( new_mapview_width - 20);

  $("#nations").height( new_mapview_height - 100);
  $("#nations").width( new_mapview_width);

  $('#tabs').css("height", $(window).height());
  $("#tabs-map").height("auto");


  $("#city_viewport").height( new_mapview_height - 20);

  $("#opt_tab").show();
  $("#players_tab").show();
  $("#cities_tab").show();
  $("#freeciv_logo").show();
  $("#tabs-hel").hide();

  if (is_small_screen()) {
    $("#map_tab").children().html("<i class='fa fa-globe' aria-hidden='true'></i>");
    $("#opt_tab").children().html("<i class='fa fa-cogs' aria-hidden='true'></i>");
    $("#players_tab").children().html("<i class='fa fa-flag' aria-hidden='true'></i>");
    $("#cities_tab").children().html("<i class='fa fa-fort-awesome' aria-hidden='true'></i>");
    $("#tech_tab").children().html("<i class='fa fa-flask' aria-hidden='true'></i>");
    $("#civ_tab").children().html("<i class='fa fa-university' aria-hidden='true'></i>");
    $("#hel_tab").children().html("<i class='fa fa-question-circle-o' aria-hidden='true'></i>");


    $(".ui-tabs-anchor").css("padding", "7px");
    $(".overview_dialog").hide();
    $(".ui-dialog-titlebar").hide();
    $("#freeciv_logo").hide();

    overview_active = false;
    $("#game_unit_orders_default").css("bottom", "-5px");
    $("#game_status_panel").css("font-size", "0.8em");
    $(".order_button").css("padding-right", "5px");
  }

  if (overview_active) init_overview();
  if (unitpanel_active) init_game_unit_panel();

}

function client_state()
{
  return civclient_state;
}


/**************************************************************************
  Return TRUE if the client can change the view; i.e. if the mapview is
  active.  This function should be called each time before allowing the
  user to do mapview actions.
**************************************************************************/
function can_client_change_view()
{
  return ((client.conn.playing != null || client_is_observer())
      && (C_S_RUNNING == client_state()
	      || C_S_OVER == client_state()));
}

/**************************************************************************
  Webclient does have observer support.
**************************************************************************/
function client_is_observer()
{
  return client.conn['observer'] || observing;
}

/**************************************************************************
  Intro message
**************************************************************************/
function show_new_game_message()
{

  if (observing || $.getUrlVar('autostart') == "true") {
    return;

  } else if (is_hotseat()) {
    show_hotseat_new_phase();
  } else if (is_pbem()) {
    show_dialog_message("Welcome to Freeciv-web: Play-By-Email.",
      "Welcome " + username + "! It is now your turn to play. Each player will " + 
      "get an e-mail when it is their turn to play, and can only play one turn at a time. " +
      "Click the end turn button to end your turn and let the next opponent play.");
    setTimeout(check_queued_tech_gained_dialog, 2500);

  } else if (is_small_screen()) {
    show_dialog_message("Welcome to Freeciv-web",
      "Welcome " + username + "! You lead a great civilization. Your task is to conquer the world!\n" +
      "Click on units for giving them orders, and drag units on the map to move them.\n" +
      "Good luck, and have a lot of fun!");

  } else if (client.conn.playing != null) {
    var pplayer = client.conn.playing;
    var player_nation_text = "Welcome, " + username + " ruler of the " 
        + nations[pplayer['nation']]['adjective'] + " empire.";

    if (is_touch_device()) {
      show_dialog_message("Welcome to Freeciv-web",
      player_nation_text + " Your\n" +
      "task is to create a great empire! You should start by\n" +
      "exploring the land around you with your explorer,\n" +
      "and using your settlers to find a good place to build\n" +
      "a city. Click on units to get a list of available orders. \n" +
      "To move your units around, carefully drag the units to the \n" +
      "place you want it to go.\n" +
      "Good luck, and have a lot of fun!");

    } else {
      show_dialog_message("Welcome to Freeciv-web",
      player_nation_text + " Your\n" +
      "task is to create a great empire! You should start by\n" +
      "exploring the land around you with your explorer,\n" +
      "and using your settlers to find a good place to build\n" +
      "a city. Right-click with the mouse on your units for a list of available \n" +
      "orders such as move, explore, build cities and attack. \n" +
      "Good luck, and have a lot of fun!");

    }

  }
}

/**************************************************************************
...
**************************************************************************/
function alert_war(player_no)
{
  var pplayer = players[player_no];
  show_dialog_message("War!", "You are now at war with the "
	+ nations[pplayer['nation']]['adjective']
    + " leader " + pplayer['name'] + "!");
}

/**************************************************************************
 Shows the endgame dialog
**************************************************************************/
function show_endgame_dialog()
{
  var title = "Final Report: The Greatest Civilizations in the world!";
  var message = "";
  for (var i = 0; i < endgame_player_info.length; i++) {
    var pplayer = players[endgame_player_info[i]['player_id']];
    var nation_adj = nations[pplayer['nation']]['adjective'];
    message += (i+1) + ": The " + nation_adj + " ruler " + pplayer['name'] 
      + " scored " + endgame_player_info[i]['score'] + " points" + "<br>";
  }

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
				"Show Scores" : function() {
					$("#dialog").dialog('close');
					view_game_scores();
				},
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

**************************************************************************/
function update_metamessage_on_gamestart()
{
  if (!observing && !metamessage_changed && client.conn.playing != null
      && client.conn.playing['pid'] == players[0]['pid'] 
      && $.getUrlVar('action') == "new") {
    var pplayer = client.conn.playing;
    var metasuggest = username + " ruler of the " + nations[pplayer['nation']]['adjective'] + ".";
    send_message("/metamessage " + metasuggest);
    setTimeout(function () {
                 chatbox_text = ' ';
                 add_chatbox_text('');
               }, 800);
  }

  if ($.getUrlVar('action') == "new" || $.getUrlVar('action') == "earthload" 
      || $.getUrlVar('scenario') == "true") {
    if (renderer == RENDERER_2DCANVAS) {
      $.post("/freeciv_time_played_stats?type=single2d").fail(function() {});
    } else {
      $.post("/freeciv_time_played_stats?type=single3d").fail(function() {});
    }
  }
  if ($.getUrlVar('action') == "multi" && client.conn.playing != null
      && client.conn.playing['pid'] == players[0]['pid']) {
    $.post("/freeciv_time_played_stats?type=multi").fail(function() {});
  }
  if ($.getUrlVar('action') == "hotseat") {
    $.post("/freeciv_time_played_stats?type=hotseat").fail(function() {});
    send_message("/metamessage hotseat game" );
  }


}


/**************************************************************************
  ...
**************************************************************************/
function set_default_mapview_active()
{
  if (renderer == RENDERER_2DCANVAS) {
    mapview_canvas_ctx = mapview_canvas.getContext("2d");
    mapview_canvas_ctx.font = canvas_text_font;
  }

  chatbox_scroll_down();

  if (!is_small_screen() && overview_active) {
    $("#game_overview_panel").parent().show();
    $(".overview_dialog").position({my: 'left bottom', at: 'left bottom', of: window, within: $("#game_page")});
  }

  if (unitpanel_active) {
    update_active_units_dialog();
  }

  if (chatbox_active) $("#game_chatbox_panel").parent().show();

  $("#tabs").tabs("option", "active", 0);
  $("#tabs-map").height("auto");

  tech_dialog_active = false;
  allow_right_click = false;
  keyboard_input = true;
}