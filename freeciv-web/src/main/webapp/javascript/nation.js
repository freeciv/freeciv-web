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
  var nation_list_html = "";
  
  for (var player_id in players) {
    var pplayer = players[player_id];
    
    var sprite = get_player_fplag_sprite(pplayer);
    
    nation_list_html = nation_list_html  
           + "<div style='background: transparent url("
           + sprite['image-src'] 
           + "); background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '>"
           + "<div style='width: 600px; margin-left: 60px;'>" + pplayer['name'] + " - " 
                       + pplayer['username'] + "  - " + nations[pplayer['nation']]['adjective']  + "</div></div>";
    
  }

  $("#nations_list").html(nation_list_html);

}