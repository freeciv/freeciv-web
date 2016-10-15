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

var game_info = null;
var calendar_info = null;
var game_rules = null;
var ruleset_control = null;
var ruleset_summary = null;
var ruleset_description = null;

var IDENTITY_NUMBER_ZERO = 0;

function game_init()
{
  map = {};
  terrains = {};
  resources = {};
  players = {};
  units = {};
  unit_types = {};
  connections = {};
  client.conn = {};

}

function game_find_city_by_number(id)
{
  return cities[id];
}

/**************************************************************************
  Find unit out of all units in game: now uses fast idex method,
  instead of looking through all units of all players.
**************************************************************************/
function game_find_unit_by_number(id)
{
  return units[id];
}

/**************************************************************************
 Count the # of thousand citizen in a civilisation.
**************************************************************************/
function civ_population(playerno) {
  var population = 0;

  for (var city_id in cities) {
    var pcity = cities[city_id];
    if (playerno == pcity['owner']) {
      population += city_population(pcity);
    }
  }
  return numberWithCommas(population * 1000);
}


/**************************************************************************
  ...
**************************************************************************/
function update_game_status_panel() {

  if (C_S_RUNNING != client_state()) return;

  var status_html = "";

  if (client.conn.playing != null) {
    var pplayer = client.conn.playing;
    var tax = client.conn.playing['tax'];
    var lux = client.conn.playing['luxury'];
    var sci = client.conn.playing['science'];

    var net_income = pplayer['net_income'];
    if (pplayer['net_income'] > 0) {
      net_income = "+" + pplayer['net_income'];
    }

    if (!is_small_screen()) status_html += "<b>" + nations[pplayer['nation']]['adjective'] + "</b> &nbsp;&nbsp; <i class='fa fa-child' aria-hidden='true' title='Population'></i>: ";
    if (!is_small_screen()) status_html += "<b>" + civ_population(client.conn.playing.playerno) + "</b>  &nbsp;&nbsp;";
    if (!is_small_screen()) status_html += "<i class='fa fa-calendar' aria-hidden='true' title='Year (turn)'></i>: <b>" + get_year_string() + "</b> &nbsp;&nbsp;";
    status_html += "<i class='fa fa-money' aria-hidden='true' title='Gold (net income)'></i>: <b>" + pplayer['gold'] + " (" + net_income + ")</b>  &nbsp;&nbsp;";
    status_html += "<i class='fa fa-btc' aria-hidden='true' title='Tax rate'></i>: <b>" + tax + "</b>% ";
    status_html += "<i class='fa fa-music' aria-hidden='true' title='Luxury rate'></i>: <b>" + lux + "</b>% ";
    status_html += "<i class='fa fa-flask' aria-hidden='true' title='Science rate'></i>: <b>" + sci + "</b>% ";
  } else if (server_settings != null && server_settings['metamessage'] != null) {
    status_html += "Observing - " + server_settings['metamessage']['val']
                   + " - ";
    status_html += "Turn: <b>" + game_info['turn'] + "</b>  ";
  }

  if ($("#game_status_panel").length) {
    $("#game_status_panel").html(status_html);
  }

  var page_title = "Freeciv-web - " + username
                                    + "  (turn:" + game_info['turn'] + ", port:"
                                    + civserverport + ") ";
  if (server_settings['metamessage'] != null) {
    page_title += server_settings['metamessage']['val'];
  }
  document.title = page_title;


}

/**************************************************************************
  Returns the year and turn as a string.
**************************************************************************/
function get_year_string()
{
  var year_string = "";
  if (game_info['year'] < 0) {
    year_string = Math.abs(game_info['year'])
                  + calendar_info['negative_year_label'] + " ";
  } else if (game_info['year'] >= 0) {
    year_string = game_info['year']
                  + calendar_info['positive_year_label'] + " ";
  }
  year_string += "(T" + game_info['turn'] + ")";
  return year_string;
}

/**************************************************************************
  Return timeout value for the current turn.
**************************************************************************/
function current_turn_timeout()
{
  if (game_info['turn'] == 1 && game_info['first_timeout'] != -1) {
    return game_info['first_timeout'];
  } else {
    return game_info['timeout'];
  }
}
