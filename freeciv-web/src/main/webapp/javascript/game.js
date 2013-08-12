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

var game_info = null;
var game_rules = null;

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
  
  for (city_id in cities) {
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
    if (is_small_screen()) {
      status_html = chatbox_text;

    } else {
      var pplayer = client.conn.playing;
      var tax = client.conn.playing['tax'];
      var lux = client.conn.playing['luxury'];
      var sci = client.conn.playing['science'];
  
      var net_income = pplayer['net_income'];
      if (pplayer['net_income'] > 0) {
        net_income = "+" + pplayer['net_income'];
      } 

      var year_string = "";
      if (game_info['year'] < 0) year_string = Math.abs(game_info['year']) + "BCE ";
      if (game_info['year'] >= 0) year_string = game_info['year'] + "CE ";
      year_string += "(T" + game_info['turn'] + ")";

      status_html += "<b>" + nations[pplayer['nation']]['adjective'] + "</b> Population: ";
      status_html += "<b>" + civ_population(client.conn.playing.playerno) + "</b>  ";
      status_html += "Year: <b>" + year_string + "</b> ";
      status_html += "Gold: <b>" + pplayer['gold'] + " (" + net_income + ")</b>  "; 
      status_html += "Tax: <b>" + tax + "</b> ";
      status_html += "Lux: <b>" + lux + "</b> ";
      status_html += "Sci: <b>" + sci + "</b> ";
    }
  } else {
    status_html += "Observing - "; 
    status_html += "Turn: <b>" + game_info['turn'] + "</b>  ";
  }
  
  $("#game_status_panel").html(status_html); 

  document.title = "Freeciv-web - " + username + "  (turn:" + game_info['turn'] + ", port:" + civserverport + ")";


}
