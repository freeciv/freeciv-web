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




/**************************************************************************
 ...
**************************************************************************/
function update_nation_screen()
{
  var nation_list_html = "<table width=90% border=0 cellspacing=0><tr style='background: #444444;'><td>Flag</td><td>Player Name:</td>"
	  + "<td>Nation:</td><td>AI/Human</td><td>Alive/Dead</td><td>Diplomatic state</td><td>Action</td></tr>";
  
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
	   + (pplayer['ai'] ? "AI" : "Human") + "</td><td>"
	   + (pplayer['is_alive'] ? "Alive" : "Dead") +  "</td>";


    if (!client_is_observer() && client.conn.playing['diplstates'] != null) {
      nation_list_html = nation_list_html + "<td>" + get_diplstate_text(client.conn.playing['diplstates'][player_id]) + "</td><td>";
      if (client.conn.playing['diplstates'][player_id] != DS_NO_CONTACT) {
        nation_list_html = nation_list_html + "<a href='#' onclick='diplomacy_init_meeting_req(" + player_id + ");'>Meet</a> ";
      }
      if (client.conn.playing['diplstates'][player_id] != DS_WAR && client.conn.playing['diplstates'][player_id] != DS_NO_CONTACT) {
        nation_list_html = nation_list_html + " - <a href='#' onclick='diplomacy_cancel_treaty(" + player_id + ");'>Cancel Treaty</a>";

      }
      nation_list_html = nation_list_html + "</td>";
    }

    nation_list_html = nation_list_html + "</tr>";


  }
  nation_list_html = nation_list_html + "</table>";

  $("#nations_list").html(nation_list_html);

}
