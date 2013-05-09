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
var techcoststyle1 = {};

var is_tech_tree_init = false;
var tech_dialog_active = false;

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

var tech_canvas = null;
var tech_canvas_ctx = null;

var tech_item_width = 208;
var tech_item_height = 52;
var maxleft = 0;

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
function init_tech_screen()
{
  $("#technologies").width($(window).width() - 20);
  
  if (is_tech_tree_init) return;

  tech_canvas = document.getElementById('tech_canvas');
  tech_canvas_ctx = tech_canvas.getContext("2d");
  if ("mozImageSmoothingEnabled" in tech_canvas_ctx) {
    // if this Boolean value is false, images won't be smoothed when scaled. This property is true by default.
    tech_canvas_ctx.mozImageSmoothingEnabled = true;
  } 

  if (is_small_screen()) {
    tech_canvas.width = Math.floor(tech_canvas.width * 0.6);
    tech_canvas.height = Math.floor(tech_canvas.height * 0.6);
    tech_canvas_ctx.scale(0.6,0.6);
  }

  is_tech_tree_init = true;

}

/**************************************************************************
 ...
**************************************************************************/
function update_tech_tree()
{
  var hy = 24;
  var hx = 48 + 160;


  tech_canvas_ctx.fillStyle = 'rgb(0, 1, 26)';
  tech_canvas_ctx.fillRect(0, 0, 5824, 726);


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
      
      tech_canvas_ctx.strokeStyle = 'rgb(255, 255, 255)';
      tech_canvas_ctx.lineWidth = 2;

      tech_canvas_ctx.beginPath();  
      tech_canvas_ctx.moveTo(sx, sy + hy);  
      tech_canvas_ctx.lineTo(dx + hx, dy + hy);  
      tech_canvas_ctx.stroke();
  

    }
  
  }

  tech_canvas_ctx.lineWidth = 1;

  for (var tech_id in techs) {
    var ptech = techs[tech_id];
    if (!(tech_id+'' in reqtree)) continue;
    
    var x = Math.floor(reqtree[tech_id+'']['x'] * tech_xscale)+2;  //scale in X direction.
    var y = reqtree[tech_id+'']['y']+2;  
    
    /* KNOWN TECHNOLOGY */
    if (player_invention_state(client.conn.playing, ptech['id']) == TECH_KNOWN) {   
        
      var tag = ptech['graphic_str'];
      tech_canvas_ctx.fillStyle = 'rgb(255, 255, 255)';
      tech_canvas_ctx.fillRect(x-2, y-2, tech_item_width, tech_item_height);
      tech_canvas_ctx.strokeStyle = 'rgb(225, 225, 225)';
      tech_canvas_ctx.strokeRect(x-2, y-2, tech_item_width, tech_item_height);
      mapview_put_tile(tech_canvas_ctx, tag, x+1, y)
           
      tech_canvas_ctx.font = canvas_text_font;
      tech_canvas_ctx.fillStyle = "rgba(0, 0, 0, 1)";
      tech_canvas_ctx.fillText(ptech['name'], x + 50, y + 30);  

      if (x > maxleft) maxleft = x;
       
      
    /* TECH WITH KNOWN PREREQS. */
    } else if (player_invention_state(client.conn.playing, ptech['id']) == TECH_PREREQS_KNOWN) {
      var bgcolor = is_tech_req_for_goal(ptech['id'], client.conn.playing['tech_goal']) ? "rgb(149, 37, 37)" : "rgb(70, 70, 70)";
      if (client.conn.playing['researching'] == ptech['id']) bgcolor = "rgb(255, 43, 0)";
        
      var tag = ptech['graphic_str'];
      tech_canvas_ctx.fillStyle = bgcolor;
      tech_canvas_ctx.fillRect(x-2, y-2, tech_item_width, tech_item_height);
      tech_canvas_ctx.strokeStyle = 'rgb(255, 255, 255)';
      tech_canvas_ctx.strokeRect(x-2, y-2, tech_item_width, tech_item_height);
      mapview_put_tile(tech_canvas_ctx, tag, x+1, y)

      tech_canvas_ctx.font = canvas_text_font;
      tech_canvas_ctx.fillStyle = "rgba(255, 255, 255, 1)";
      tech_canvas_ctx.fillText(ptech['name'], x + 50, y + 30);  
         
 
    /* UNKNOWN TECHNOLOGY. */
    } else if (player_invention_state(client.conn.playing, ptech['id']) == TECH_UNKNOWN) { 
      var bgcolor = is_tech_req_for_goal(ptech['id'], client.conn.playing['tech_goal']) ? "rgb(149, 37, 37)" : "rgb(70, 70, 70)";
        
      var tag = ptech['graphic_str'];
      tech_canvas_ctx.fillStyle =  bgcolor;
      tech_canvas_ctx.fillRect(x-2, y-2, tech_item_width, tech_item_height);
      tech_canvas_ctx.strokeStyle = 'rgb(255, 255, 255)';
      tech_canvas_ctx.strokeRect(x-2, y-2, tech_item_width, tech_item_height);

      mapview_put_tile(tech_canvas_ctx, tag, x+1, y)
           
      tech_canvas_ctx.font = canvas_text_font;
      tech_canvas_ctx.fillStyle = "rgba(255, 255, 255, 1)";
      tech_canvas_ctx.fillText(ptech['name'], x + 50, y + 30);  
 
    
    }
    
    
  }

}

/**************************************************************************
 ...
**************************************************************************/
function is_tech_req_for_goal(check_tech_id, goal_tech_id)
{
  if (check_tech_id == goal_tech_id) return true;
  if (goal_tech_id == 0 || check_tech_id == 0) return false;

    var goal_tech = techs[goal_tech_id];
    if (goal_tech == null) return false;

    for (var i = 0; i < goal_tech['req'].length; i++) {
      var rid = goal_tech['req'][i];
      if (check_tech_id == rid) {
        return true;
      } else if (is_tech_req_for_goal(check_tech_id, rid)) {
        return true;
      }
    }

    return false;

}

/**************************************************************************
 ...
**************************************************************************/
function update_tech_screen()
{
  
  if (client_is_observer()) return;

  init_tech_screen();
  update_tech_tree();


  var research_goal_text = "No research target selected.<br>Please select a technology now";
  if (techs[client.conn.playing['researching']] != null) {
    research_goal_text = "Researching: " + techs[client.conn.playing['researching']]['name'];
  }
  if (techs[client.conn.playing['tech_goal']] != null) {   
    research_goal_text = research_goal_text + "<br>Goal: " 
        + techs[client.conn.playing['tech_goal']]['name'];
  }
  $("#tech_goal_box").html(research_goal_text);
  
  $("#tech_progress_text").html("Research progress: " 
		         + client.conn.playing['bulbs_researched'] 
			 + " / " + client.conn.playing['current_research_cost']);

  var pct_progress = 100 * (client.conn.playing['bulbs_researched'] / client.conn.playing['current_research_cost']);

  $("#progress_fg").css("width", pct_progress  + "%");

  if (techs[client.conn.playing['researching']] != null) {
    var adv_txt = "";
    var prunits = get_units_from_tech(client.conn.playing['researching']);
    var pimprovements = get_improvements_from_tech(client.conn.playing['researching']);

    for (var i = 0; i < prunits.length; i++) {
      var punit = prunits[i];
      adv_txt += punit['name'] + ", "; 
    }

    for (var i = 0; i < pimprovements.length; i++) {
      var pimpr = pimprovements[i];
      adv_txt += pimpr['name'] + ", "; 
    }

  

    $("#tech_result_text").html(techs[client.conn.playing['researching']]['name'] 
		                + " gives these advances:<br>" + adv_txt);
  }
  
  $("#tech_tab_item").css("color", "#ffffff");

  /* scroll the tech tree, so that the current research targets are on the screen..  */
  maxleft = maxleft - 280;
  if (maxleft < 0) maxleft = 0;
  if (!tech_dialog_active) {
    setTimeout("scroll_tech_tree();",10);
  }
 
  tech_dialog_active = true;
 
}


/**************************************************************************
 ...
**************************************************************************/
function scroll_tech_tree()
{
  $("#technologies").scrollLeft(maxleft);
}


/**************************************************************************
 ...
**************************************************************************/
function send_player_research(tech_id)
{
  var packet = {"type" : packet_player_research, "tech" : tech_id};
  send_request (JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function send_player_tech_goal(tech_id)
{
  var packet = {"type" : packet_player_tech_goal, "tech" : tech_id};
  send_request (JSON.stringify(packet));
}


/****************************************************************************
  This function is triggered when the mouse is clicked on the tech canvas.
****************************************************************************/
function tech_mapview_mouse_click(e)
{
  //var x = e.offsetX || e.layerX;
  //var y = e.offsetY || e.layerY;
  var rightclick;
  if (!e) var e = window.event;
  if (e.which) {
    rightclick = (e.which == 3);
  } else if (e.button) {
    rightclick = (e.button == 2);
  }
  
   if (tech_canvas != null) {
    var tech_mouse_x = mouse_x - $("#technologies").offset().left + $("#technologies").scrollLeft();
    var tech_mouse_y = mouse_y - $("#technologies").offset().top + $("#technologies").scrollTop();
    
    for (var tech_id in techs) {
      var ptech = techs[tech_id];
      if (!(tech_id+'' in reqtree)) continue;
    
      var x = Math.floor(reqtree[tech_id+'']['x'] * tech_xscale)+2;  //scale in X direction.
      var y = reqtree[tech_id+'']['y']+2;  

      if (is_small_screen()) {
        x = x * 0.6;
        y = y * 0.6;
      }

      if (tech_mouse_x > x && tech_mouse_x < x + tech_item_width
          && tech_mouse_y > y && tech_mouse_y < y + tech_item_height) {
        if (player_invention_state(client.conn.playing, ptech['id']) == TECH_PREREQS_KNOWN) {
          send_player_research(ptech['id']);
	} else if (player_invention_state(client.conn.playing, ptech['id']) == TECH_UNKNOWN) {
          send_player_tech_goal(ptech['id']);
	}
      }

    }

  }
 
  update_tech_screen();

}

