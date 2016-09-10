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

var governments = {};
var requested_gov = -1;

var REPORT_WONDERS_OF_THE_WORLD = 0;
var REPORT_TOP_5_CITIES = 1;
var REPORT_DEMOGRAPHIC = 2;
var REPORT_ACHIEVEMENTS = 3;


/**************************************************************************
   ...
**************************************************************************/
function show_revolution_dialog()
{
  var id = "#revolution_dialog";
  $(id).remove();
  $("<div id='revolution_dialog'></div>").appendTo("div#game_page");


  var dhtml = "Current form of government: " + governments[client.conn.playing['government']]['name']
	  + "<br>To start a revolution, select the new form of government:"
  + "<p><div id='governments' >"
  + "<div id='governments_list'>"
  + "</div></div><br> ";

  $(id).html(dhtml);

  $(id).attr("title", "Start a Revolution!");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "99%" : "450",
			height: $(window).height() - 40,
			  buttons: {
				"Start revolution!" : function() {
					start_revolution();
					$("#revolution_dialog").dialog('close');
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
    var pnation = nations[pplayer['nation']];
    var tag = pnation['graphic_str'];

    var civ_description =
	    "<img src='/images/flags/" + tag + "-web.png' width='180'>"
	    + "<br><div>" + pplayer['name'] + " rules the " + nations[pplayer['nation']]['adjective']
	    + " with the form of government: " + governments[client.conn.playing['government']]['name']
	    + "</div><br>";
    $("#nation_title").html("The " + nations[pplayer['nation']]['adjective'] + " nation");
    $("#civ_dialog_text").html(civ_description);

  } else {
    $("#civ_dialog_text").html("This dialog isn't available as observer.");

  }

}


/**************************************************************************
   ...
**************************************************************************/
function update_govt_dialog()
{
  var govt;
  var govt_id;
  if (client_is_observer()) return;

  var governments_list_html = "";

  for (govt_id in governments) {
    govt = governments[govt_id];
    governments_list_html += "<button class='govt_button' id='govt_id_" + govt['id'] + "' "
	                  + "onclick='set_req_government(" + govt['id'] + ");' "
			  + "title='" + govt['helptext'] + "'>" +  govt['name'] + "</button>";
  }

  $("#governments_list").html(governments_list_html);

  for (govt_id in governments) {
    govt = governments[govt_id];
    if (!can_player_get_gov(govt_id)) {
      $("#govt_id_" + govt['id'] ).button({ disabled: true, label: govt['name'], icons: {primary: govt['rule_name']} });
    } else if (requested_gov == govt_id) {
    $("#govt_id_" + govt['id'] ).button({label: govt['name'], icons: {primary: govt['rule_name']}}).css("background", "green");
    } else if (client.conn.playing['government'] == govt_id) {
      $("#govt_id_" + govt['id'] ).button({label: govt['name'], icons: {primary: govt['rule_name']}}).css("background", "#BBBBFF").css("font-weight", "bolder");
    } else {
      $("#govt_id_" + govt['id'] ).button({label: govt['name'], icons: {primary: govt['rule_name']}});
    }

  }
  $(".govt_button").tooltip();

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
  var packet = {"pid" : packet_player_change_government,
                "government" : govt_id };
  send_request(JSON.stringify(packet));
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
  Returns true iff the player can get the specified governement.

  Uses the JavaScript implementation of the requirement system. Is
  therefore limited to the requirement types and ranges the JavaScript
  requirement system can evaluate.
**************************************************************************/
function can_player_get_gov(govt_id)
{
  return (player_has_wonder(client.conn.playing.playerno, 63) //hack for statue of liberty
          || are_reqs_active(client.conn.playing,
                      null,
                      null,
                      null,
                      null,
                      null,
                      null,
                      governments[govt_id]["reqs"],
                      RPT_CERTAIN));
}

/**************************************************************************
 ...
**************************************************************************/
function request_report(rtype)
{
  var packet = {"pid"  : packet_report_req,
                "type" : rtype};
  send_request(JSON.stringify(packet));
}
