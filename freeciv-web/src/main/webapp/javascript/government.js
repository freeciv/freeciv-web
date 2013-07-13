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

var governments = {};
var requested_gov = -1;

var REPORT_WONDERS_OF_THE_WORLD = 0;
var REPORT_TOP_5_CITIES = 1;
var REPORT_DEMOGRAPHIC = 2;


/**************************************************************************
   ...
**************************************************************************/
function show_revolution_dialog()
{
  var id = "#revolution_dialog";
  $(id).remove();
  $("<div id='revolution_dialog'></div>").appendTo("div#game_page");


  var dhtml = "<h2>Start a revolution!</h2>" 
  + "To start a revolution, select your nations new government:"
  + "<p><div id='governments' style='height: 180px; width: 250px; overflow: auto;'>"
  + "<div id='governments_list' style='cursor:pointer;cursor:hand'>"
  + "</div></div><br> ";
  
  $(id).html(dhtml);

  $(id).attr("title", "Revolution");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: "450",
			  buttons: {
				"Start revolution!" : function() {
					start_revolution();
					$(this).dialog('close');
				}
			  }

  });


  update_govt_dialog();

}

/**************************************************************************
   ...
**************************************************************************/
function init_civ_dialog()
{
  if (!client_is_observer() && client.conn.playing != null) {
    
    var pplayer = client.conn.playing;
    var civ_description = 
	    "<canvas id='flag_canvas' width='100' height='70' moz-opaque='true'></canvas>"
	    + "<br><div>" + pplayer['name'] + " rules the " + nations[pplayer['nation']]['adjective'] 
	    + " with the form of government: " + governments[client.conn.playing['government']]['name']
	    + "<br>Choose your command carefully:</div><br>"
            

    $("#civ_dialog_text").html(civ_description);


    $("#revolution_button").css("width", 190);
    $("#taxrates_button").css("width", 190);
    $("#wonders_report").css("width", 190);
    $("#top_cities_report").css("width", 190);
    $("#demography_report").css("width", 190);

    var pnation = nations[pplayer['nation']]; 
    var tag = "f." + pnation['graphic_str'];

    var flag_canvas = document.getElementById('flag_canvas');
    var flag_canvas_ctx = flag_canvas.getContext("2d");
    if ("mozImageSmoothingEnabled" in flag_canvas_ctx) {
      flag_canvas_ctx.mozImageSmoothingEnabled = true;
    } 

    flag_canvas_ctx.drawImage(sprites[tag], 0, 0, 29, 20, 0, 0, 100, 70);


  } else {
    $("#civ_dialog_text").html("This dialog isn't available as observer.");

  }

}


/**************************************************************************
   ...
**************************************************************************/
function update_govt_dialog()
{
  
  if (client_is_observer()) return;
  
  var governments_list_html = "";
  
  for (var govt_id in governments) {
    var govt = governments[govt_id];
    var bgcolor = "#111111;";   

    if (!can_player_get_gov(govt_id)) {
      bgcolor = "#000000; color:#333333;";
    } else if (requested_gov == govt_id) {
      bgcolor = "#53d000; color:#000000;";
    }

    if (client.conn.playing['government'] == govt_id) {
      bgcolor = "#ffffff; color:#000000;";
    } 
    
    governments_list_html = governments_list_html + "<div style='margin:4px;background-color:" + bgcolor + "' " 
                  + " onclick='set_req_government(" + govt['id'] + ");' "
                  + " title='" + govt['helptext'] + "'>" 
		  + "<img src='/images/" + govt['graphic_str'] + ".png'>"
		  + govt['name'] + "</div>";
  }

  $("#governments_list").html(governments_list_html);


}


/**************************************************************************
   ...
**************************************************************************/
function start_revolution()
{
  if (requested_gov != -1) {
    send_player_change_government(requested_gov);
    requested_gov = -1;
  }
}

/**************************************************************************
   ...
**************************************************************************/
function set_req_government(gov_id)
{
  requested_gov = gov_id;
  update_govt_dialog();
}

/**************************************************************************
 ...
**************************************************************************/
function send_player_change_government(govt_id)
{
  var packet = {"type" : packet_player_change_government, 
                "government" : govt_id };
  send_request (JSON.stringify(packet));
}


/**************************************************************************
 Returns the max tax rate for a given government.
 FIXME: This shouldn't be hardcoded, but instead fetched
 from the effects.
**************************************************************************/
function government_max_rate(govt_id)
{ 
  if (govt_id == 0) {
    // Anarchy
    return 100;
  } else if (govt_id == 1) {
    //Despotism
    return 60;
  } else if (govt_id == 2) {
    // Monarchy
    return 70;
  } else if (govt_id == 3) {
    //Communism
    return 80;
  } else if (govt_id == 4) {
    //Republic
    return 80;
  } else if (govt_id == 5) {
    //Democracy
    return 100;
  } else {
    // this should not happen
    return 100;
  }
    
          

}

/**************************************************************************
 ...
**************************************************************************/
function can_player_get_gov(govt_id)
{
  if (govt_id == 0) {	
	  return true;
  } else if (govt_id == 1) {
	  return true;
  } else if (govt_id == 2) {
	  return (player_invention_state(client.conn.playing, 53) == TECH_KNOWN);
  } else if (govt_id == 3) {
	  return (player_invention_state(client.conn.playing, 16) == TECH_KNOWN);
  } else if (govt_id == 4) {
	  return (player_invention_state(client.conn.playing, 80) == TECH_KNOWN);
  } else if (govt_id == 5) {
	  return (player_invention_state(client.conn.playing, 21) == TECH_KNOWN);
  } else {
    return false;
  }

}

/**************************************************************************
 ...
**************************************************************************/
function request_report(rtype)
{
  var packet = {"type" :  packet_report_req, 
                "report_type" : rtype};
  send_request (JSON.stringify(packet));

}
