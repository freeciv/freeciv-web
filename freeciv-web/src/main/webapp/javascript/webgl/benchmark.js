/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

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

var benchmark_start;
var benchmark_enabled = false;
var benchmark_frames_count = 0;
var initial_benchmark_enabled = true;

/****************************************************************************
  Runs a benchmark of the WebGL version
****************************************************************************/
function webgl_benchmark_run()
{
  benchmark_enabled = true;
  $("#dialog").dialog('close');
  $("#pregame_settings").dialog('close');
  send_message("/set mapseed 420");
  send_message("/set gameseed 420");
  send_message("/start");
  setTimeout(benchmark_check, 1000);
}


/****************************************************************************
...
****************************************************************************/
function benchmark_check()
{
  try {
    $("#dialog").dialog('close');
  } catch (err)  {}
  try {
    $(".diplomacy_dialog").dialog('close');
  } catch (err)  {}
  try {
    $("#tech_dialog").dialog('close');
  } catch (err)  {}

  if (game_info != null && game_info['turn'] >= 30) {
    var time_elapsed =  (new Date().getTime() - benchmark_start) / 1000;
    var fps = Math.floor(benchmark_frames_count / time_elapsed);

    $("#benchmark_dialog").remove();
    $("<div id='benchmark_dialog'></div>").appendTo("div#game_page");

    $("#benchmark_dialog").html("Total time: " + time_elapsed + " seconds.<br>"
                                                       + "Frames per second: " + fps);
    $("#benchmark_dialog").attr("title", "Benchmark results:");
    $("#benchmark_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "50%",
			buttons: {
				Ok: function() {
					$("#benchmark_dialog").dialog('close');
				}
			}
		});

    $("#benchmark_dialog").dialog('open');

    benchmark_enabled = false;
    return;
  }
  if (game_info != null && game_info['turn'] == 1 && units[101] != null) {
    var packet = {"pid" : packet_unit_do_action,
                                          "actor_id" : 101,
                                          "target_id": units[101]['tile'],
                                          "value" : 0,
                                          "name" : "Hello world",
                                          "action_type": ACTION_FOUND_CITY};
    send_request(JSON.stringify(packet));
  }

  key_unit_auto_explore();
  if (game_info != null) send_end_turn();

  setTimeout(benchmark_check, 1000);
}

/****************************************************************************
 Measure the fps for the first 10 seconds of the game, and show an error
 if the fps is too low to play.
****************************************************************************/
function initial_benchmark_check()
{
  var time_elapsed =  (new Date().getTime() - benchmark_start) / 1000;
  var fps = Math.floor(benchmark_frames_count / time_elapsed);
  initial_benchmark_enabled = false;
  if (fps < 4) {
    var renderer_name = "-";
    var gl = document.createElement('canvas').getContext('webgl');
    if (gl != null) {
      var extension = gl.getExtension('WEBGL_debug_renderer_info');
      if (extension != undefined) {
        renderer_name = gl.getParameter(extension.UNMASKED_RENDERER_WEBGL);
      }
    }

    var quality_string = "";
    if (graphics_quality == QUALITY_LOW) quality_string = "Low quality";
    if (graphics_quality == QUALITY_MEDIUM) quality_string = "Medium quality";
    if (graphics_quality == QUALITY_HIGH) quality_string = "High quality";

    var message = "The game is running too slowly! Please check if the drivers for your graphics driver needs to be updated and that you have a graphics card which supports WebGL 3D. "
    + "3D WebGL rendering requires updated drivers and a good 3D graphics card with 3D hardware acceleration. The current WebGL renderer is: " + renderer_name + ".<br>";
    if (renderer_name == "Google SwiftShader") message += "<br>Warning: Google SwiftShader is a software renderer which has very poor performance and will not work with this game. ";
    message += "<br><b>You can try playing the 2D version instead, or configure lower graphics quality.</b><br><br>";
    message += "Current graphics level: " + quality_string;

    show_slow_game_warning_message("3D game is running very slowly!", message);

    console.error("WebGL 3D is running slowly. FPS: " + fps + ", Quality:" + quality_string + ", Renderer: " + renderer_name);

  }

}


/**************************************************************************
 Shows a generic message dialog.
**************************************************************************/
function show_slow_game_warning_message(title, message) {

  // reset dialog page.
  $("#generic_dialog").remove();
  $("<div id='generic_dialog'></div>").appendTo("div#game_page");

  speak(title);
  speak(message);

  if (cardboard_vr_enabled) return;

  $("#generic_dialog").html(message);
  $("#generic_dialog").attr("title", title);
  $("#generic_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "50%",
			close: closing_dialog_message,
			buttons: {
			    "Play 2D version - Restart!" : function() {
                  window.location.href = '/';
                 },
			    "Configure graphics settings" : function() {
			      $("#generic_dialog").remove();
                  $("<div id='pregame_page'></div>").appendTo("div#game_page");
			      $("<div id='pregame_settings'></div>").appendTo("div#pregame_page");
                  pregame_settings();
                  $("#pregame_settings_tabs").tabs({ active: 1 });
                 },
				"Play anyway": close_dialog_message

			}
		});

  $("#generic_dialog").dialog('open');
  $("#game_text_input").blur();

  $('#generic_dialog').css("max-height", "450px");

}
