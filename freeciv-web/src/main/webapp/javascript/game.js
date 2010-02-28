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
  ...
**************************************************************************/
function get_player_population() {
  var population = 0;
  var playerno = client.conn.playing.playerno;
  
  for (city_id in cities) {
    var pcity = cities[city_id];
    if (playerno == pcity['owner']) {
      population += pcity['size'];
    }
  }
  return population * 10000;
}


/**************************************************************************
  ...
**************************************************************************/
function update_game_status_panel() {
  // Turkmen Population: 10,000  Turn: 1  Gold: 51  Tax: 40 Lux: 0 Sci: 60
  
  if (C_S_RUNNING != client_state()) return;
  
  var status_html = "";

  if (client.conn.playing != null) {
    var pplayer = client.conn.playing;
    var tax = client.conn.playing['tax'];
    var lux = client.conn.playing['luxury'];
    var sci = client.conn.playing['science'];
  
    status_html += nations[pplayer['nation']]['adjective'] + " Population: ";
    status_html += get_player_population() + "  ";
    status_html += "Turn: " + game_info['turn'] + "  ";
    status_html += "Gold: " + pplayer['gold'] + "  "; 
    status_html += "Tax: " + tax + " ";
    status_html += "Lux: " + lux + " ";
    status_html += "Sci: " + sci + " ";
  } else {
    status_html += "Observing - "; 
    status_html += "Turn: " + game_info['turn'] + "  ";
  }
  
  $("#game_status_panel").html(status_html); 

}