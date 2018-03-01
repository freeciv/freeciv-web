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

var replay_enabled = true;
var replay_gif;

/**************************************************************************
...
**************************************************************************/
function init_replay()
{
  if (is_small_screen()) replay_enabled = false;
  if (!replay_enabled) return;

  replay_gif = new GIF({
    workers: 1,
    quality: 1,
    workerScript: "/javascript/libs/gif.worker.js"
  });



}

/**************************************************************************
...
**************************************************************************/
function add_replay_frame()
{
  if (!replay_enabled) return;

  if ($("#overview_map").length > 0 && $("#overview_map").width() > 0 && $("#overview_map").height() > 0) {
    replay_gif.addFrame(document.getElementById("overview_img"), {delay: 1000, copy: true});
  }

}


/**************************************************************************
...
**************************************************************************/
function show_replay()
{
  if (!replay_enabled) return;

  var replay_gif_clone = jQuery.extend(true, {}, replay_gif);
  replay_gif_clone.on('finished', function(blob) {
      show_replay_dialog(blob);
  });
  replay_gif_clone.render();




}


/**************************************************************************
...
**************************************************************************/
function show_replay_dialog(blob) {

  var title = "Replay";
  var message = "<img id='replay_result' style='width: 100%; height: 100%;'>";

  // reset dialog page.
  $("#replay_dialog").remove();
  $("<div id='replay_dialog'></div>").appendTo("div#game_page");

  $("#replay_dialog").html(message);

  var replay_image = document.getElementById("replay_result")
  replay_image.src = URL.createObjectURL(blob);

  $("#replay_dialog").attr("title", title);
  $("#replay_dialog").dialog({
			bgiframe: true,
			modal: false,
			height: 600,
			width: is_small_screen() ? "90%" : "50%",
			buttons: {
				Ok: function() {
					$("#replay_dialog").dialog('close');
					$("#game_text_input").blur();
				}
			}
		});

  $("#replay_dialog").dialog('open');
  $("#game_text_input").blur();



}
