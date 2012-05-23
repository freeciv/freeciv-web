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

var nations = {};
var nation_groups = [];
var diplstates = {};



/**************************************************************************
 ...
**************************************************************************/
function update_nation_screen()
{
  var nation_list_html = "<table width=90% border=0 cellspacing=0><tr style='background: #444444;'><td>Flag</td><td>Player Name:</td>"
	  + "<td>Nation:</td><td>Attitude</td><td>Score</td><td>AI/Human</td><td>Alive/Dead</td><td>Diplomatic state</td><td>Action</td></tr>";
  
  for (var player_id in players) {
    var pplayer = players[player_id];
    
    var sprite = get_player_fplag_sprite(pplayer);
    
    nation_list_html = nation_list_html  
           + "<tr><td><div style='background: transparent url("
           + sprite['image-src'] 
           + "); background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '>"
           + "</div></td>";
    

    nation_list_html = nation_list_html 
           + "<td>" + pplayer['name'] + "</td><td>" 
           + nations[pplayer['nation']]['adjective']  + "</td><td>" 
	   + col_love(pplayer) + "</td><td>" 
	   + get_score_text(pplayer) + "</td><td>" 
	   + (pplayer['ai'] ? "AI" : "Human") + "</td><td>"
	   + (pplayer['is_alive'] ? "Alive" : "Dead") +  "</td>";


    if (!client_is_observer() && diplstates[player_id] != null) {
      nation_list_html = nation_list_html + "<td>" + get_diplstate_text(diplstates[player_id]) + "</td><td>";
      if (diplstates[player_id] != DS_NO_CONTACT) {
        nation_list_html = nation_list_html + " <button type='button' class='nation_button' onClick='diplomacy_init_meeting_req(" + player_id + ");' >Meet</button>";
      }
      if (diplstates[player_id] != DS_WAR && diplstates[player_id] != DS_NO_CONTACT) {
        nation_list_html = nation_list_html + "  <button type='button' class='nation_button' onClick='diplomacy_cancel_treaty(" + player_id + ");' >Cancel Treaty</button>";

      }
      nation_list_html = nation_list_html + "</td>";
    }

    nation_list_html = nation_list_html + "</tr>";


  }
  nation_list_html = nation_list_html + "</table>";

  $("#nations_list").html(nation_list_html);
  $(".nation_button").button();
  $(".nation_button").css("font-size", "11px");
}


/**************************************************************************
 ...
**************************************************************************/
function col_love(pplayer)
{
  if (client_is_observer() || pplayer['playerno'] == client.conn.playing['playerno'] 
      || pplayer['ai'] == false) {
    return "-";
  } else {
    return love_text(pplayer['love'][client.conn.playing['playerno']] - MAX_AI_LOVE);
  }

}

/**************************************************************************
 ...
**************************************************************************/
function get_score_text(player)
{

  if (player['score'] > 0 || client_is_observer() 
      || player['playerno'] == client.conn.playing['playerno']) {
    return player['score'];
  } else {
    return "?";
  }

}

/**************************************************************************
  Return a text describing an AI's love for you.  (Oooh, kinky!!)
  These words should be adjectives which can fit in the sentence
  "The x are y towards us"
  "The Babylonians are respectful towards us"
**************************************************************************/
function love_text(love)
{
  if (love <= - MAX_AI_LOVE * 90 / 100) {
    return "Genocidal";
  } else if (love <= - MAX_AI_LOVE * 70 / 100) {
    return "Belligerent";
  } else if (love <= - MAX_AI_LOVE * 50 / 100) {
    return "Hostile";
  } else if (love <= - MAX_AI_LOVE * 25 / 100) {
    return "Uncooperative";
  } else if (love <= - MAX_AI_LOVE * 10 / 100) {
    return "Uneasy";
  } else if (love <= MAX_AI_LOVE * 10 / 100) {
    return "Neutral";
  } else if (love <= MAX_AI_LOVE * 25 / 100) {
    return "Respectful";
  } else if (love <= MAX_AI_LOVE * 50 / 100) {
    return "Helpful";
  } else if (love <= MAX_AI_LOVE * 70 / 100) {
    return "Enthusiastic";
  } else if (love <= MAX_AI_LOVE * 90 / 100) {
    return "Admiring";
  } else {
    return "Worshipful";
  }
}

