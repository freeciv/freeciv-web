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

var nations = {};
var nation_groups = [];
var diplstates = {};
var selected_player = -1;


/**************************************************************************
 ...
**************************************************************************/
function update_nation_screen()
{
  var nation_list_html = "<table class='tablesorter' id='nation_table' width='95%' border=0 cellspacing=0 >"
	  + "<thead><td>Flag</td><td>Color</td><td>Player Name:</td>"
	  + "<td>Nation:</td><td>Attitude</td><td>Score</td><td>AI/Human</td><td>Alive?</td>"
	  + "<td>Diplomatic state</td><td>Team</td><td>State</td></thead>";

  for (var player_id in players) {
    var pplayer = players[player_id];

    var flag_url = get_player_flag_url(pplayer);
    var flag_html = "<img class='nation_flags' src='" + flag_url +"'>";

    var plr_class = "";
    if (!client_is_observer() && player_id == client.conn.playing['playerno']) plr_class = "nation_row_self";
    if (!pplayer['is_alive']) plr_class = "nation_row_dead";
    if (!client_is_observer() && diplstates[player_id] != null && diplstates[player_id] == DS_WAR) plr_class = "nation_row_war";

    nation_list_html += "<tr data-plrid='" + player_id + "' class='" + plr_class
	   + "'><td>" + flag_html + "</td>";
    nation_list_html += "<td><div style='background-color: " + nations[pplayer['nation']]['color']
           + "; margin: 5px; width: 25px; height: 25px;'>"
           + "</div></td>";

    nation_list_html += "<td>" + pplayer['name'] + "</td><td>"
           + nations[pplayer['nation']]['adjective']  + "</td><td>"
	   + col_love(pplayer) + "</td><td>"
	   + get_score_text(pplayer) + "</td><td>"
     + (pplayer['flags'].isSet(PLRF_AI) ?
          get_ai_level_text(pplayer) + " AI" : "Human") + "</td><td>"
	   + (pplayer['is_alive'] ? "Alive" : "Dead") +  "</td>";

    if (!client_is_observer() && diplstates[player_id] != null && player_id != client.conn.playing['playerno']) {
      nation_list_html += "<td>" + get_diplstate_text(diplstates[player_id]) + "</td>";
    } else {
      nation_list_html += "<td>-</td>";
    }

    nation_list_html += "<td>Team " + pplayer['team'] + "</td>";
    var pstate = " ";
    if (pplayer['phase_done'] && !pplayer['flags'].isSet(PLRF_AI)) {
      pstate = "Done";
    } else if (!pplayer['flags'].isSet(PLRF_AI)
               && pplayer['nturns_idle'] > 1) {
      pstate += "Idle for " + pplayer['nturns_idle'] + " turns";
    } else if (!pplayer['phase_done']
               && !pplayer['flags'].isSet(PLRF_AI)) {
      pstate = "Moving";
    }
    nation_list_html += "<td>" + pstate + "</td>";
    nation_list_html += "</tr>";


  }
  nation_list_html += "</table>";

  $("#nations_list").html(nation_list_html);
  $(".nation_button").button();
  if (!is_touch_device()) {
    $("#nation_table").tablesorter({theme: "dark"});
  } else if (is_small_screen()) {
    $("#nations").height( mapview['height'] - 150);
    $("#nations").width( mapview['width']);
  }

  $("#nation_table").selectable({ filter: "tr",
       selected: function( event, ui ) {handle_nation_table_select(ui); } });

  selected_player = -1;

  if (is_pbem()) {
    /* TODO: PBEM games do not support diplomacy.*/
    $('#meet_player_button').hide();
    $('#cancel_treaty_button').hide();
    $('#take_player_button').hide();
    $('#toggle_ai_button').hide();
    $('#game_scores_button').hide();
  } else {
    $('#view_player_button').button("disable");
    $('#meet_player_button').button("disable");
    $('#cancel_treaty_button').button("disable");
    $('#take_player_button').button("disable");
    $('#toggle_ai_button').button("disable");
  }

  if (is_small_screen) {
    $('#take_player_button').hide();
  }

}


/**************************************************************************
 ...
**************************************************************************/
function col_love(pplayer)
{
  if (client_is_observer() || pplayer['playerno'] == client.conn.playing['playerno']
      || pplayer['flags'].isSet(PLRF_AI) == false) {
    return "-";
  } else {
    return love_text(pplayer['love'][client.conn.playing['playerno']]);
  }

}

/**************************************************************************
 ...
**************************************************************************/
function handle_nation_table_select( ui )
{
  selected_player = parseFloat($(ui.selected).data("plrid"));
  var player_id = selected_player;
  var pplayer = players[selected_player];

  if (pplayer != null && pplayer['is_alive'] && (client_is_observer() || player_id == client.conn.playing['playerno']
      || (diplstates[player_id] != null && diplstates[player_id] != DS_NO_CONTACT))) {
    $('#view_player_button').button("enable");
  } else {
    $('#view_player_button').button("disable");
  }

  if (!client_is_observer() && diplstates[player_id] != null
      && diplstates[player_id] != DS_NO_CONTACT) {
    $('#meet_player_button').button("enable");
  } else {
    $('#meet_player_button').button("disable");
  }
  if (!is_hotseat()
      && !pplayer['flags'].isSet(PLRF_AI)
      && (diplstates[player_id] != null && diplstates[player_id] == DS_NO_CONTACT)) {
    $('#meet_player_button').button("disable");
  }

  if (!client_is_observer() && player_id != client.conn.playing['playerno']
      && diplstates[player_id] != null
      && diplstates[player_id] != DS_WAR && diplstates[player_id] != DS_NO_CONTACT) {
    $('#cancel_treaty_button').button("enable");

  } else {
    $('#cancel_treaty_button').button("disable");
  }
  
  if (!client_is_observer() && player_id != client.conn.playing['playerno']) {
    if (diplstates[player_id] == DS_CEASEFIRE || diplstates[player_id] == DS_ARMISTICE) {
      $("#cancel_treaty_button").button("option", "label", "Declare war");
    } else {
      $("#cancel_treaty_button").button("option", "label", "Cancel treaty");
    }
  }

  if (!is_hotseat() && client_is_observer() && pplayer['flags'].isSet(PLRF_AI)
      && nations[pplayer['nation']]['is_playable']
               && $.getUrlVar('multi') == "true") {
    $('#take_player_button').button("enable");
  } else {
    $('#take_player_button').button("disable");
  }

  $('#toggle_ai_button').button("enable");

}

/**************************************************************************
 ...
**************************************************************************/
function nation_meet_clicked()
{
  if (selected_player == -1) return;
  diplomacy_init_meeting_req(selected_player);
  set_default_mapview_active();
}

/**************************************************************************
 ...
**************************************************************************/
function cancel_treaty_clicked()
{
  if (selected_player == -1) return;
  diplomacy_cancel_treaty(selected_player);
  set_default_mapview_active();
}

/**************************************************************************
 ...
**************************************************************************/
function take_player_clicked()
{
  if (selected_player == -1) return;
  var pplayer = players[selected_player];
  take_player(pplayer['name']);
  set_default_mapview_active();
}

/**************************************************************************
 ...
**************************************************************************/
function toggle_ai_clicked()
{
  if (selected_player == -1) return;
  var pplayer = players[selected_player];
  aitoggle_player(pplayer['name']);
  set_default_mapview_active();
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

/**************************************************************************
 ...
**************************************************************************/
function take_player(player_name)
{
  send_message("/take " + player_name.substring(0,3));
  observing = false;
}

/**************************************************************************
 ...
**************************************************************************/
function aitoggle_player(player_name)
{
  send_message("/ai " + player_name.substring(0,3));
  observing = false;
}

/**************************************************************************
 ...
**************************************************************************/
function center_on_player()
{
  if (selected_player == -1) return;

    /* find a city to focus on. */
    for (var city_id in cities) {
      var pcity = cities[city_id];
      if (city_owner_player_id(pcity) == selected_player) {
        center_tile_mapcanvas(city_tile(pcity));
        set_default_mapview_active();
        return;
      }
    }
}
