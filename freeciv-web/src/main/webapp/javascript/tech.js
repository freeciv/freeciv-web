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


var techs = {};

var is_tech_tree_init = false;

var tech_xscale = 1.2;

/* TECH_KNOWN is self-explanatory, TECH_PREREQS_KNOWN are those for which all 
 * requirements are fulfilled; all others (including those which can never 
 * be reached) are TECH_UNKNOWN */
var TECH_UNKNOWN = 0;
var TECH_PREREQS_KNOWN = 1;
var TECH_KNOWN = 2;

var AR_ONE = 0;
var AR_TWO = 1;
var AR_ROOT = 2;
var AR_SIZE = 3;


var TF_BONUS_TECH = 0; /* player gets extra tech if rearched first */
var TF_BRIDGE = 1;    /* "Settler" unit types can build bridges over rivers */
var TF_RAILROAD = 2;  /* "Settler" unit types can build rail roads */
var TF_POPULATION_POLLUTION_INC = 3;  /* Increase the pollution factor created by population by one */
var TF_FARMLAND = 4;  /* "Settler" unit types can build farmland */
var TF_BUILD_AIRBORNE = 5; /* Player can build air units */
var TF_LAST = 6;

/*
  [kept for amusement and posterity]
typedef int Tech_type_id;
  Above typedef replaces old "enum tech_type_id"; see comments about
  Unit_type_id in unit.h, since mainly apply here too, except don't
  use Tech_type_id very widely, and don't use (-1) flag values. (?)
*/
/* [more accurately]
 * Unlike most other indices, the Tech_type_id is widely used, because it 
 * so frequently passed to packet and scripting.  The client menu routines 
 * sometimes add and substract these numbers.
 */
var A_NONE = 0;
var A_FIRST = 1;
var A_LAST = MAX_NUM_ITEMS;
var A_UNSET = (A_LAST-1);
var A_FUTURE  = (A_LAST-2);
var A_UNKNOWN = (A_LAST-3);
var A_LAST_REAL = A_UNKNOWN;

var  A_NEVER = null;



/**************************************************************************
  Returns state of the tech for current pplayer.
  This can be: TECH_KNOWN, TECH_UNKNOWN, or TECH_PREREQS_KNOWN
  Should be called with existing techs or A_FUTURE

  If pplayer is NULL this checks whether any player knows the tech (used
  by the client).
**************************************************************************/
function player_invention_state(pplayer, tech_id)
{

  if (pplayer == null) {
    return TECH_UNKNOWN;
    /* FIXME: add support for global advances
    if (tech != A_FUTURE && game.info.global_advances[tech_id]) {
      return TECH_KNOWN;
    } else {
      return TECH_UNKNOWN;
    }*/
  } else {
    /* Research can be null in client when looking for tech_leakage
     * from player not yet received. */
    if (pplayer['inventions'] != null && pplayer['inventions'][tech_id] != null) {
      return pplayer['inventions'][tech_id];
    } else {
      return TECH_UNKNOWN;
    }
  }
}


/**************************************************************************
 ...
**************************************************************************/
function update_tech_screen()
{
  
  if (client_is_observer()) return;
  
  var technology_heading_html = "<div><b>";
  
  
  if (techs[client.conn.playing['researching']] != null) {
    technology_heading_html = technology_heading_html + "Researching:" 
         + techs[client.conn.playing['researching']]['name'];
  } 
     
  if (techs[client.conn.playing['tech_goal']] != null) {   
     technology_heading_html = technology_heading_html + " <br> Technology goal: "
     + techs[client.conn.playing['tech_goal']]['name'];
   }
   
  technology_heading_html = technology_heading_html + "</b></div>";
   
  $("#technology_heading").html(technology_heading_html);
  
  var technology_list_html = "";

  for (var tech_id in techs) {
    var ptech = techs[tech_id];
    if (!(tech_id+'' in reqtree)) continue;
    
    var x = Math.floor(reqtree[tech_id+'']['x'] * tech_xscale);  //scale in X direction.
    var y = reqtree[tech_id+'']['y'];  
    
    /* KNOWN TECHNOLOGY */
    if (player_invention_state(client.conn.playing, ptech['id']) == TECH_KNOWN) {   
        
      var sprite = get_technology_image_sprite(ptech);
      
      technology_list_html = technology_list_html +
       "<div style='z-index: 1000; cursor:pointer;cursor:hand; position: absolute; left: " + x + "px; top: " + y + "px; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '"
           + "><div style='background-color:#ffffff; color:#000000; margin-left: 48px; width:160px; height: 48px; font-size: 110%'>" + ptech['name'] + "</div></div>";
           
           
      
    /* TECH WITH KNOWN PREREQS. */
    } else if (player_invention_state(client.conn.playing, ptech['id']) == TECH_PREREQS_KNOWN) {
      var bgcolor = (client.conn.playing['researching'] == ptech['id']) ? "#992222" : "#222266";
        
      var sprite = get_technology_image_sprite(ptech);
      
      technology_list_html = technology_list_html +
       "<div style='z-index: 1000; cursor:pointer;cursor:hand; position: absolute; left: " + x + "px; top: " + y + "px; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '"
           + " onclick='send_player_research(" + ptech['id'] + ");'"
           + "><div style='background-color:" + bgcolor+ "; margin-left: 48px; width:160px; height: 48px; font-size: 110%'>" + ptech['name'] + "</div></div>";
    
    /* UNKNOWN TECHNOLOGY. */
    } else if (player_invention_state(client.conn.playing, ptech['id']) == TECH_UNKNOWN) { 
      var bgcolor = (client.conn.playing['tech_goal'] == ptech['id']) ? "#330000" : "#222222";
        
      var sprite = get_technology_image_sprite(ptech);
      
      technology_list_html = technology_list_html +
       "<div style='z-index: 1000; cursor:pointer;cursor:hand; position: absolute; left: " + x + "px; top: " + y + "px; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px; margin: 5px; '"
           + " onclick='send_player_tech_goal(" + ptech['id'] + ");'"
           + "><div style='background-color:" + bgcolor+ "; margin-left: 48px; width:160px; height: 48px; font-size: 110%'>" + ptech['name'] + "</div></div>";
    
    }
    
    
  }
  
  $("#technology_list").html(technology_list_html);
  

  init_tech_tree();  
 
  
}

/**************************************************************************
 Draw dependency lines.
**************************************************************************/
function init_tech_tree() 
{

  if (is_tech_tree_init == true) return;
  is_tech_tree_init = true;

  /* Draw dependency lines. */
  var jg = new jsGraphics("technologies");
  jg.setColor("#eeeeee");
  var hy = 24;
  var hx = 48 + 160;

  for (var tech_id in techs) {
    var ptech = techs[tech_id];
    if (!(tech_id+'' in reqtree)) continue;
    
    var sx = Math.floor(reqtree[tech_id+'']['x'] * tech_xscale);  //scale in X direction.
    var sy = reqtree[tech_id+'']['y'];
    for (var i = 0; i < ptech['req'].length; i++) {
      var rid = ptech['req'][i];
      if (rid == 0) continue;
      
      var dx = Math.floor(reqtree[rid+'']['x'] * tech_xscale);  //scale in X direction.
      var dy = reqtree[rid+'']['y'];
      
      jg.drawLine(sx, sy + hy, dx + hx, dy + hy);
    }
  
  }
  jg.paint();
  
}

/**************************************************************************
 ...
**************************************************************************/
function send_player_research(tech_id)
{
  var packet = [{"packet_type" : "player_research", "tech" : tech_id}];
  send_request (JSON.stringify(packet));
  setTimeout ( "update_tech_screen();", 800);
}

/**************************************************************************
 ...
**************************************************************************/
function send_player_tech_goal(tech_id)
{
  var packet = [{"packet_type" : "player_tech_goal", "tech" : tech_id}];
  send_request (JSON.stringify(packet));
  setTimeout ( "update_tech_screen();", 800);
}
