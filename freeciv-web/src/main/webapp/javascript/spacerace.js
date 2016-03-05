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


var spaceship_info = {};


var SSHIP_NONE = 0;
var SSHIP_STARTED = 1;
var SSHIP_LAUNCHED = 2;
var SSHIP_ARRIVED = 3;

var SSHIP_PLACE_STRUCTURAL = 0;
var SSHIP_PLACE_FUEL = 1;
var SSHIP_PLACE_PROPULSION = 2;
var SSHIP_PLACE_HABITATION = 3;
var SSHIP_PLACE_LIFE_SUPPORT = 4;
var SSHIP_PLACE_SOLAR_PANELS = 5;

/**************************************************************************
 ...
**************************************************************************/
function show_spaceship_dialog()
{
  var title = "Spaceship";
  var message = "";

  if (client_is_observer()) return;

  console.log(spaceship_info[client.conn.playing['playerno']]);

  var spaceship = spaceship_info[client.conn.playing['playerno']];

  message += "Spaceship progress: " + get_spaceship_state_text(spaceship['sship_state']) + "<br>";
  message += "Success probability: " + Math.floor(spaceship['success_rate'] * 100) + "%<br>";
  message += "Travel time: " + Math.floor(spaceship['travel_time']) + " years<br>";
  message += "Components: " + spaceship['components'] + "<br>";
  message += "Energy Rate: " + Math.floor(spaceship['energy_rate'] * 100) + "%<br>";
  message += "Support Rate: " + Math.floor(spaceship['support_rate'] * 100) + "%<br>";
  message += "Habitation: " + spaceship['habitation'] + "<br>";
  message += "Life Support: " + spaceship['life_support'] + "<br>";
  message += "Mass: " + spaceship['mass'] + " tons<br>";
  message += "Modules: " + spaceship['modules'] + "<br>";
  message += "Population: " + spaceship['population'] + "<br>";
  message += "Propulsion: " + spaceship['propulsion'] + "<br>";
  message += "Solar Panels: " + spaceship['solar_panels'] + "<br>";
  message += "Structurals: " + spaceship['structurals'] + "<br>";
  if (spaceship['launch_year'] != 9999) message += "Launch year: " + spaceship['launch_year'] + "<br>";

  if (game_info['victory_conditions'] == 0) message = "Spaceship victory disabled.<br>";

  message += "<br>For help, see the Space Race page in the manual.<br>";

  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "40%",
			buttons: {
				Close: function() {
					$("#dialog").dialog('close');
			        },
			         Launch : function() {
					$("#dialog").dialog('close');
					launch_spaceship();
					set_default_mapview_active();
				}
			}
		});

  $("#dialog").dialog('open');

  if (spaceship['sship_state'] != SSHIP_STARTED || spaceship['success_rate'] == 0) $(".ui-dialog-buttonpane button:contains('Launch')").button("disable");

}

/**************************************************************************
 ...
**************************************************************************/
function launch_spaceship()
{
  var test_packet = {"pid" : packet_spaceship_launch};
  var myJSONText = JSON.stringify(test_packet);
  send_request(myJSONText);

}

/**************************************************************************
 ...
**************************************************************************/
function get_spaceship_state_text(state_id)
{
 if (state_id == SSHIP_NONE) return "Not started";
 if (state_id == SSHIP_STARTED) return "Started";
 if (state_id == SSHIP_LAUNCHED) return "Launched";
 if (state_id == SSHIP_ARRIVED) return "Arrived";
}


