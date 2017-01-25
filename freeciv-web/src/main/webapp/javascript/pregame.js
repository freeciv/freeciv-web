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

var observing = false;
var chosen_nation = -1;
var chosen_style = -1;
var choosing_player = -1;
var ai_skill_level = 2;
var nation_select_id = -1;
var metamessage_changed = false;
var logged_in_with_password = false;
var antialiasing_setting = true;

/****************************************************************************
  ...
****************************************************************************/
function pregame_start_game()
{
  if (client.conn['player_num'] == null) return;

  var test_packet = {"pid" : packet_player_ready, "is_ready" : true,
                     "player_no": client.conn['player_num']};
  var myJSONText = JSON.stringify(test_packet);
  send_request(myJSONText);

  setup_window_size ();
}

/****************************************************************************
  ...
****************************************************************************/
function observe()
{
  if (observing) {
    $("#observe_button").button("option", "label", "Observe Game");
    send_message("/detach");

  } else {
    $("#observe_button").button("option", "label", "Don't observe");
    send_message("/observe");
  }

  observing = !observing;
}

/****************************************************************************
  Show information about the current game
****************************************************************************/
function update_game_info_pregame()
{
  var game_info_html = "";

  if (C_S_PREPARING != client_state()) {
    /* The game has already started. */
    return;
  }

  if (scenario_info != null && scenario_info['is_scenario']) {
    /* Show the scenario description. */
    game_info_html += "<p>";
    game_info_html += scenario_info['description'].replace(/\n/g, "<br>");
    game_info_html += "</p>";

    if (scenario_info['authors']) {
      /* Show the scenario authors. */
      game_info_html += "<p>";
      game_info_html += "Scenario by ";
      game_info_html += scenario_info['authors'].replace(/\n/g, "<br>");
      game_info_html += "</p>";
    }

    if (scenario_info['prevent_new_cities']) {
      /* Make sure that the player is aware that cities can't be built. */
      game_info_html += "<p>";
      game_info_html += scenario_info['name']
                        + " forbids the founding of new cities.";
      game_info_html += "</p>";
    }
  }

  $("#pregame_game_info").html(game_info_html);

  /* Update pregame_message_area's height. */
  setup_window_size();
}

/****************************************************************************
  ...
****************************************************************************/
function update_player_info_pregame()
{
  var id;
  if (C_S_PREPARING == client_state()) {
    player_html = "";
    for (id in players) {
      player = players[id];
      if (player != null) {
        if (player['name'].indexOf("AI") != -1) {
          player_html += "<div id='pregame_plr_" + id
		        + "' class='pregame_player_name'><div id='pregame_ai_icon'></div><b>"
                        + player['name'] + "</b></div>";
        } else {
          player_html += "<div id='pregame_plr_" + id
		        + "' class='pregame_player_name'><div id='pregame_player_icon'></div><b>"
                        + player['name'] + "</b></div>";
        }
      }
    }
    $("#pregame_player_list").html(player_html);

    /* show player ready state in pregame dialog */
    for (id in players) {
      player = players[id];
      var nation_text = "";
      if (player['nation'] in nations) {
        nation_text = " - " + nations[player['nation']]['adjective'];
        var flag_url = get_player_flag_url(player);
	var flag_html = $("<img class='pregame_flags' src='" + flag_url +"'>");
	$("#pregame_plr_"+id).prepend(flag_html);
      }
      if (player['is_ready'] == true) {
        $("#pregame_plr_"+id).addClass("pregame_player_ready");
        $("#pregame_plr_"+id).attr("title", "Player ready" + nation_text);
      } else if (player['name'].indexOf("AI") == -1) {
          $("#pregame_plr_"+id).attr("title", "Player not ready" + nation_text);
      } else {
          $("#pregame_plr_"+id).attr("title", "AI Player (random nation)");
      }
      $("#pregame_plr_"+id).attr("name", player['name']);
      $("#pregame_plr_"+id).attr("playerid", player['playerno']);

    }
    $(".pregame_player_name").tooltip();

    var pregame_context_items = {
            "pick_nation": {name: "Pick nation"},
            "observe_player": {name: "Observe this player"},
            "take_player": {name: "Take this player"},
            "aitoggle_player": {name: "Aitoggle player"},
            "sep1": "---------",
            "novice": {name: "Novice"},
            "easy": {name: "Easy"},
            "normal": {name: "Normal"},
            "hard": {name: "Hard"},
            "sep2": "---------"
       };

    if (is_pbem()) {
      pregame_context_items = {
            "pick_nation": {name: "Pick nation"}};
    }

    $("#pregame_player_list").contextMenu({
        selector: '.pregame_player_name', 
        callback: function(key, options) {
            var name = $(this).attr('name');
            if (name != null && name.indexOf(" ") != -1) name = name.split(" ")[0];
            var playerid = parseInt($(this).attr('playerid'));
            if (key == "take_player") {
              send_message("/take " + name);
            } else if (key == "pick_nation") {
              pick_nation(playerid);
            } else if (key == "aitoggle_player") {
              send_message("/ai " + name);
            } else if (key == "observe_player") {
              send_message("/observe " + name);
            } else if (key == "novice") {
              send_message("/novice " + name);
            } else if (key == "easy") {
              send_message("/easy " + name);
            } else if (key == "normal") {
              send_message("/normal " + name);
            } else if (key == "hard") {
              send_message("/hard " + name);
            }  
        },
        items: pregame_context_items 
    });

    /* set state of Start game button depending on if user is ready. */
    if (client.conn['player_num'] != null  && client.conn['player_num'] in players
        && players[client.conn['player_num']]['is_ready'] == true) {
        $("#start_game_button").button( "option", "disabled", true);
    } else {
        $("#start_game_button").button( "option", "disabled", false);
    }

  }


}


/****************************************************************************
  Shows the pick nation dialog.
****************************************************************************/
function pick_nation(player_id)
{
  if (player_id == null) player_id = client.conn['player_num'];
  var pplayer = players[player_id]; 
  choosing_player = player_id;

  if (pplayer == null) return; 

  var nations_html = "<div id='nation_heading'><span>Select nation for " + pplayer['name'] + ":</span> <br>"
                  + "<input id='nation_autocomplete_box' type='text' size='20'>"
		  + "<div id='nation_choice'></div></div> <div id='nation_list'> ";

  /* prepare a list of flags and nations. */
  var nation_name_list = [];
  for (var nation_id in nations) {
    var pnation = nations[nation_id];
    if (pnation['is_playable']) {
      nations_html += "<div class='nation_pickme_line' onclick='select_nation(" + nation_id + ");'>"
             + "<div id='nation_" + nation_id + "' class='nation_choice'>" + pnation['adjective'] + "</div></div>";
      nation_name_list.push(pnation['adjective']);
    }
  }

  nations_html += "</div><div id='nation_style_choices'></div>"
               + "<div id='nation_legend'></div><div id='select_nation_flag'></div>";

  $("#pick_nation_dialog").html(nations_html);
  $("#pick_nation_dialog").attr("title", "What Nation Will You Be?");
  $("#pick_nation_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "99%" : "80%",
            height: $(window).height() - 40,
			buttons: {
				Ok: function() {
					$("#pick_nation_dialog").dialog('close');
					submit_nation_choice();
				}
			}
		});

  $("#nation_legend").html("Please choose nation for " + pplayer['name'] + ".");


  $("#nation_autocomplete_box").autocomplete({
      source: nation_name_list,
      close: function (event, ui) { update_nation_selection(); }
  });

  if (is_small_screen()) {
    $("#select_nation_flag").hide();
    $("#nation_legend").hide();
    $("#nation_style_choices").hide();
    $("#nation_list").width("80%");
  }

  nation_select_id = setTimeout (update_nation_selection, 150);
  $("#pick_nation_dialog").dialog('open');

  if (!is_small_screen()) {
    render_city_style_list();
  } else {
    $("#nation_autocomplete_box").blur();
  }


}

/****************************************************************************
 This function is called when the nation select autocomplete box has been used,
 and it will call select_nation based on the user's choice.
****************************************************************************/
function update_nation_selection()
{
  var nation_name = $("#nation_autocomplete_box").val();
  if (nation_name.length == 0) return;
  if (C_S_RUNNING == client_state()) return;

  for (var nation_id in nations) {
    var pnation = nations[nation_id];
    if (pnation['is_playable'] && pnation['adjective'].toLowerCase() == nation_name.toLowerCase()) {
      select_nation(nation_id);
      return;
    }
  }
}

/****************************************************************************
 Renders a list of city styles in the choose nation dialog.
****************************************************************************/
function render_city_style_list()
{
  /* prepare a list of city styles. */
  var city_style_html = "<b>City Styles:</b><br>";
  for (var style_id in city_rules) {
    if (style_id > 5) continue;
    var pstyle = city_rules[style_id];
    city_style_html += "<canvas id='city_style_" + style_id 
          + "' data-style-id='" + style_id + "' width='96' height='72' style='cursor: pointer;'></canvas><br>"
          + pstyle['rule_name'] + "<br>";
  }
  $("#nation_style_choices").html(city_style_html);
  for (var style_id in city_rules) {
    if (style_id > 5) continue;
    var pstyle = city_rules[style_id];
    var pcitystyle_canvas = document.getElementById('city_style_' + style_id);
    var ctx = pcitystyle_canvas.getContext("2d");
    var tag = pstyle['graphic'] + "_city_4";
    if (style_id == chosen_style) {
      ctx.fillStyle="#FFFFFF";
      ctx.fillRect(0,0, 96, 72);
    }
    ctx.drawImage(sprites[tag], 0, 0);

    $("#city_style_" + style_id).click(function() {
      chosen_style = parseFloat($(this).attr("data-style-id"));
      render_city_style_list();
    });

  }


}

/****************************************************************************
  Updates the choose nation dialog with the selected nation.
****************************************************************************/
function select_nation(new_nation_id)
{
  var pnation = nations[new_nation_id];
  $("#nation_legend").html(pnation['legend']);
  $("#nation_autocomplete_box").val(pnation['adjective']);
  $("#nation_" + chosen_nation).css("background-color", "transparent");
  $("#nation_choice").html("Chosen nation: " + pnation['adjective']);
  $("#select_nation_flag").html("<img style='nation_flag_choice' src='/images/flags/"
		            + pnation['graphic_str'] + "-web.png'>");

  if (chosen_nation != new_nation_id && $("#nation_" + new_nation_id).length > 0) {
    $("#nation_" + new_nation_id).get(0).scrollIntoView();
  }

  chosen_nation = parseFloat(new_nation_id);
  $("#nation_" + chosen_nation).css("background-color", "#FFFFFF");
}

/****************************************************************************
  ...
****************************************************************************/
function submit_nation_choice()
{
  if (chosen_nation == -1 || client.conn['player_num'] == null 
      || choosing_player == null || choosing_player < 0) return;

  var pplayer = players[choosing_player];

  var leader_name = pplayer['name']; 

  if (pplayer['flags'].isSet(PLRF_AI)) {
    leader_name = nations[chosen_nation]['leader_name'][0];
  }

  var style = nations[chosen_nation]['style'];
  if (chosen_style != -1) {
    style = chosen_style;
  }

  var test_packet = {"pid" : packet_nation_select_req,
                     "player_no" : choosing_player,
                     "nation_no" : chosen_nation,
                     "is_male" : true, /* FIXME */
                     "name" : leader_name,
                     "style" : style};
  send_request(JSON.stringify(test_packet));
  clearInterval(nation_select_id);
}

/***************************************************************************
  Returns the ruleset directory of the ruleset based on its name.
***************************************************************************/
function ruledir_from_ruleset_name(ruleset_name, fall_back_dir)
{
  /* HACK: find current ruleset dir based on its name. */
  switch (ruleset_name) {
  case "Classic ruleset":
    return "classic";
  case "Civ2Civ3 ruleset":
    return "civ2civ3";
  case "Webperimental":
    return "webperimental";
  default:
    console.log("Don't know the ruleset dir of \"" + ruleset_name
                + "\". Guessing \"" + fall_back_dir + "\".");
    return fall_back_dir;
  }
}

/****************************************************************************
 Shows the pregame settings dialog.
****************************************************************************/
function pregame_settings()
{
  var id = "#pregame_settings";
  $(id).remove();
  $("<div id='pregame_settings'></div>").appendTo("div#pregame_page");


  var dhtml = "<div id='pregame_settings_tabs'>" +
      "   <ul>" +
      "     <li><a href='#pregame_settings_tabs-1'>Game</a></li>" +
      "     <li><a href='#pregame_settings_tabs-2'>3D WebGL</a></li>" +
      "     <li><a href='#pregame_settings_tabs-3'>Other</a></li>" +
      "   </ul>" +
      "<div id='pregame_settings_tabs-1'><table id='settings_table'> " +
  	  "<tr title='Set metaserver info line'><td>Game title:</td>" +
	  "<td><input type='text' name='metamessage' id='metamessage' size='28' maxlength='42'></td></tr>" +
	  "<tr title='Enables music'><td>Music:</td>" +
          "<td><input type='checkbox' name='music_setting' id='music_setting'>Play Music</td></tr>" +
	  "<tr class='not_pbem' title='Total number of players (including AI players)'><td>Number of Players (including AI):</td>" +
	  "<td><input type='number' name='aifill' id='aifill' size='4' length='3' min='0' max='20' step='1'></td></tr>" +
	  "<tr class='not_pbem' title='Maximum seconds per turn'><td>Timeout (seconds per turn):</td>" +
	  "<td><input type='number' name='timeout' id='timeout' size='4' length='3' min='30' max='3600' step='1'></td></tr>" +
          "<tr class='not_pbem' title='Creates a private game where players need to know this password in order to join.'><td>Password for private game:</td>" +
	  "<td><input type='text' name='password' id='password' size='10' length='10'></td></tr>" +
	  "<tr title='Map size (in thousands of tiles)'><td>Map size:</td>" +
	  "<td><input type='number' name='mapsize' id='mapsize' size='4' length='3' min='1' max='18' step='1'></td></tr>" +
	  "<tr class='not_pbem' title='This setting sets the skill-level of the AI players'><td>AI skill level:</td>" +
	  "<td><select name='skill_level' id='skill_level'>" +
	  "<option value='1'>Handicapped</option>" +
	  "<option value='2'>Novice</option>" +
	  "<option value='3'>Easy</option>" +
          "<option value='4'>Normal</option>" +
          "<option value='5'>Hard</option>" +
          "<option value='6'>Cheating</option>" +
	  "</select></td></tr>"+
	  "<tr title='Number of initial techs per player'><td>Tech level:</td>" +
	  "<td><input type='number' name='techlevel' id='techlevel' size='3' length='3' min='0' max='100' step='10'></td></tr>" +
	  "<tr title='This setting gives the approximate percentage of the map that will be made into land.'><td>Landmass:</td>" +
	  "<td><input type='number' name='landmass' id='landmass' size='3' length='3' min='15' max='85' step='10'></td></tr>" +
	  "<tr title='Amount of special resource squares'><td>Specials:</td>" +
	  "<td><input type='number' name='specials' id='specials' size='4' length='4' min='0' max='1000' step='50'></td></tr>" +
	  "<tr title='Minimum distance between cities'><td>City mindist :</td>" +
	  "<td><input type='number' name='citymindist' id='citymindist' size='4' length='4' min='1' max='9' step='1'></td></tr>" +
          "<tr title='The game will end at the end of the given turn.'><td>End turn:</td>" +
	  "<td><input type='number' name='endturn' id='endturn' size='4' length='4' min='0' max='32767' step='1'></td></tr>" +
	  "<tr class='not_pbem' title='Enables score graphs for all players, showing score, population, techs and more."+
          " This will lead to information leakage about other players.'><td>Score graphs</td>" +
          "<td><input type='checkbox' name='scorelog_setting' id='scorelog_setting' checked>Enable score graphs</td></tr>" +
      "<tr id='killstack_area'><td id='killstack_label'></td>" +
          "<td><input type='checkbox' id='killstack_setting'>Enable killstack</td></tr>" +
      "<tr id='selct_multiple_units_area'><td id='select_multiple_units_label'></td>" +
          "<td><input type='checkbox' id='select_multiple_units_setting'>Right-click selects units</td></tr>" +
	  "<tr title='Method used to generate map'><td>Map generator:</td>" +
	  "<td><select name='generator' id='generator'>" +
          "<option value='RANDOM'>Fully random height</option>" +
          "<option value='FRACTAL'>Pseudo-fractal height</option>" +
          "<option value='ISLAND'>Island-based</option>" +
          "<option value='FAIR'>Fair islands</option>" +
          "<option value='FRACTURE'>Fracture map</option>" +
	  "</select></td></tr>"+
  	  "<tr class='not_pbem' title='Ruleset version'><td>Ruleset:</td>" +
	  "<td><select name='ruleset' id='ruleset'>" +
	  "<option value='classic'>Classic</option>" +
	  "<option value='civ2civ3'>Civ2Civ3</option>" +
      "<option value='webperimental'>Webperimental</option>" +
	  "</select></td></tr></table><br>"+
	  "<span id='settings_info'><i>Freeciv-web can be customized using the command line in many " +
          "other ways also. Type /help in the command line for more information.</i></span></div>" +

      "<div id='pregame_settings_tabs-2'>"+
      "<br><span id='settings_info'><i>3D WebGL requires a fast computer with 3D graphics card, such as Nvidia GeForce and at least 3GB of RAM. " +
      "The 3D WebGL version of Freeciv-web is under development. Here you can configure the 3D WebGL version:</i></span><br><br>" +
	    "<table id='settings_table'>" +
	    "<tr title='Graphics quality level'><td>Graphics quality:</td>" +
        	  "<td><select name='graphics_quality' id='graphics_quality'>" +
              "<option value='1'>Low</option>" +
              "<option value='2'>Medium</option>" +
              "<option value='3'>High</option>" +
              "</select></td></tr>"+
	    "<tr id='3d_antialiasing_enabled'><td id='3d_antialiasing_label' style='min-width: 150px;'></td>" +
        "<td><input type='checkbox' id='3d_antialiasing_setting' checked>Enable antialiasing (game looks nicer, but is slower)</td></tr>" +
        "<tr><td style='min-width: 150px;'>Benchmark of 3D WebGL version:</td>" +
                "<td><button id='bechmark_run' type='button' class='benchmark button'>Run benchmark</button></td></tr>" +
        "</table>" +
      "</div>" +

      "<div id='pregame_settings_tabs-3'>" +
	    "<table id='settings_table'><tr id='speech_enabled'><td id='speech_label'></td>" +
        "<td><input type='checkbox' id='speech_setting'>Enable speech audio messages</td></tr>" +
	    "<tr id='voice_row'><td id='voice_label'></td>" +
        "<td><select name='voice' id='voice'></select></td></tr>" +
        "</table>" +
      "</div>"
	  ;
  $(id).html(dhtml);

  $(id).attr("title", "Game Settings");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "98%" : "60%",
                        height: $(window).height() - 40,
			  buttons: {
				Ok: function() {
					$("#pregame_settings").dialog('close');
				}
			  }
  });

  $("#pregame_settings_tabs").tabs();

  if (game_info != null) {
    $("#aifill").val(game_info['aifill']);
    $("#timeout").val(game_info['timeout']);
    $("#skill_level").val(ai_skill_level);
  }

  if (server_settings['techlevel'] != null
      && server_settings['techlevel']['val'] != null) {
    $("#techlevel").val(server_settings['techlevel']['val']);
  }

  if (server_settings['landmass'] != null
      && server_settings['landmass']['val'] != null) {
    $("#landmass").val(server_settings['landmass']['val']);
  }

  if (server_settings['specials'] != null
      && server_settings['specials']['val'] != null) {
    $("#specials").val(server_settings['specials']['val']);
  }

  if (server_settings['citymindist'] != null
      && server_settings['citymindist']['val'] != null) {
    $("#citymindist").val(server_settings['citymindist']['val']);
  }

  if (server_settings['endturn'] != null
      && server_settings['endturn']['val'] != null) {
    $("#endturn").val(server_settings['endturn']['val']);
  }

  if (server_settings['size'] != null
      && server_settings['size']['val'] != null) {
    $("#mapsize").val(server_settings['size']['val']);
  }

  if (server_settings['killstack'] != null
      && server_settings['killstack']['val'] != null) {
    $("#killstack_setting").prop("checked",
                                 server_settings['killstack']['val']);
    $("#killstack_area").prop("title",
                              server_settings['killstack']['extra_help']);
    $("#killstack_label").prop("innerHTML",
                              server_settings['killstack']['short_help']);
  }

  if (server_settings['generator'] != null
      && server_settings['generator']['val'] != null) {
    /* TODO: Should probably be auto generated from setting so help text,
     * alternatives etc is kept up to date. */
    $("#generator").val(server_settings['generator']['support_names'][
                        server_settings['generator']['val']]);
  }

  $("#select_multiple_units_setting").prop("checked", map_select_setting_enabled);
  $("#select_multiple_units_area").prop("title", "Select multiple units with right-click and drag");
  $("#select_multiple_units_label").prop("innerHTML", "Select multiple units with right-click and drag");

  $("#3d_antialiasing_label").prop("innerHTML", "Antialiasing:");

  var stored_antialiasing_setting = simpleStorage.get("antialiasing_setting", "");
  if (stored_antialiasing_setting != null && stored_antialiasing_setting == "false") {
      $("#3d_antialiasing_setting").prop("checked", false);
      antialiasing_setting = false;
  } else if (graphics_quality == QUALITY_LOW) {
    antialiasing_setting = false;
    simpleStorage.set("antialiasing_setting", "false");
    $("#3d_antialiasing_setting").prop("checked", false);
  }


  $('#3d_antialiasing_setting').change(function() {
    antialiasing_setting = !antialiasing_setting;
    simpleStorage.set("antialiasing_setting", antialiasing_setting ? "true" : "false");
  });

  if (is_speech_supported()) {
    $("#speech_setting").prop("checked", speech_enabled);
    $("#speech_label").prop("innerHTML", "Speech messages:");
    $("#voice_label").prop("innerHTML", "Voice:");
  } else {
    $("#speech_label").prop("innerHTML", "Speech messages:");
    $("#speech_setting").parent().html("Speech Synthesis API is not supported or enabled in your browser.");
  }

  if (server_settings['metamessage'] != null
      && server_settings['metamessage']['val'] != null) {
    $("#metamessage").val(server_settings['metamessage']['val']);
  }

  if (ruleset_control != null) {
    $("#ruleset").val(ruledir_from_ruleset_name(ruleset_control['name'],
                                                "classic"));
  }

  if (scenario_info != null && scenario_info['is_scenario']) {
    /* A scenario may be bound to a ruleset. */
    $("#ruleset").prop("disabled", scenario_info['ruleset_locked']);
  }

  $(id).dialog('open');

  $('#aifill').change(function() {
    send_message("/set aifill " + $('#aifill').val());
  });

  $('#metamessage').change(function() {
    send_message("/metamessage " + $('#metamessage').val());
    metamessage_changed = true;
  });

  $('#metamessage').bind('keyup blur',function(){
    var cleaned_text = $(this).val().replace(/[^a-zA-Z\s\-]/g,'');
    if ($(this).val() != cleaned_text) {
      $(this).val( cleaned_text ); }
    }
  );

  $('#mapsize').change(function() {
    send_message("/set size " + $('#mapsize').val());
  });

  $('#timeout').change(function() {
    send_message("/set timeout " + $('#timeout').val());
  });

  $('#techlevel').change(function() {
    send_message("/set techlevel " + $('#techlevel').val());
  });

  $('#specials').change(function() {
    send_message("/set specials " + $('#specials').val());
  });

  $('#landmass').change(function() {
    send_message("/set landmass " + $('#landmass').val());
  });

  $('#citymindist').change(function() {
    send_message("/set citymindist " + $('#citymindist').val());
  });

  $('#endturn').change(function() {
    send_message("/set endturn " + $('#endturn').val());
  });

  $('#generator').change(function() {
    send_message("/set generator " + $('#generator').val());
  });

  $('#ruleset').change(function() {
    change_ruleset($('#ruleset').val());
  });

  $('#password').change(function() {

    swal({   
      title: "Really set game password?",   
      text: "Setting a password on this game means that other players can not join this game " +
            "unless they know this password. In multiplayer games, be sure to ask the other players " +
            "about setting a password.",   
      type: "warning",   showCancelButton: true,   
      confirmButtonColor: "#DD6B55",   
      confirmButtonText: "Yes, set game password",   
      closeOnConfirm: true }, 
      function(){   
        var pwd_packet = {"pid" : packet_authentication_reply, "password" : $('#password').val()};
        send_request(JSON.stringify(pwd_packet));
        send_message("/metamessage Private password-protected game");
        metamessage_changed = true;
        $("#metamessage").prop('readonly', true);
        $("#metamessage_setting").prop('readonly', true);
        $("#password").prop('readonly', true);
     });
   });


  $('#skill_level').change(function() {
    ai_skill_level = parseFloat($('#skill_level').val());
    if (ai_skill_level == 1) {
      send_message("/handicapped");
    } else if (ai_skill_level == 2) {
      send_message("/novice");
    } else if (ai_skill_level == 3) {
      send_message("/easy");
    } else if (ai_skill_level == 4) {
      send_message("/normal");
    } else if (ai_skill_level == 5) {
      send_message("/hard");
    } else if (ai_skill_level == 6) {
      send_message("/cheating");
    }
  });

  $('#music_setting').prop('checked', audio_enabled == true);

  $('#music_setting').change(function() {
    audio_enabled = !audio_enabled;
    if (audio_enabled) {
      if (!audio.source.src) {
        if (!supports_mp3()) {
          audio.load("/music/" + music_list[Math.floor(Math.random() * music_list.length)] + ".ogg");
        } else {
          audio.load("/music/" + music_list[Math.floor(Math.random() * music_list.length)] + ".mp3");
        }
      }
      audio.play();
    } else {
      audio.pause();
    }
  });

  $('#scorelog_setting').change(function() {
    var scorelog_enabled = $('#scorelog_setting').prop('checked');  
    if (scorelog_enabled) {
      send_message("/set scorelog enabled");
    } else {
      send_message("/set scorelog disabled");
    }
  });

  $('#killstack_setting').change(function() {
    if ($('#killstack_setting').prop('checked')) {
      send_message("/set killstack enabled");
    } else {
      send_message("/set killstack disabled");
    }
  });

  $('#select_multiple_units_setting').change(function() {
      if ($('#select_multiple_units_setting').prop('checked')) {
        map_select_setting_enabled = true;
      } else {
        map_select_setting_enabled = false;
      }
  });


  $('#graphics_quality').change(function() {
    graphics_quality = parseFloat($('#graphics_quality').val());
    simpleStorage.set("graphics_quality", graphics_quality);
    load_count = 0;
    webgl_preload();
  });

  $("#graphics_quality").val(graphics_quality);

  $(".benchmark").click(function() {
    webgl_benchmark_run();
  });
  if (renderer == RENDERER_WEBGL) {
    $(".benchmark").button();
  } else {
    $('[href="#pregame_settings_tabs-2"]').closest('li').hide();
  }

  $('#speech_setting').change(function() {
    if ($('#speech_setting').prop('checked')) {
      speech_enabled = true;
      speak_unfiltered("Speech enabled.");
    } else {
      speech_enabled = false;
    }
  });

  $('#voice').change(function() {
    voice = $('#voice').val();
  });

  if (is_speech_supported()) {
    load_voices();
    window.speechSynthesis.onvoiceschanged = function(e) {
      load_voices();
    };
  }

  if (is_pbem()) {
    $(".not_pbem").hide();
  }

  $("#settings_table").tooltip();

  if (is_touch_device() || is_small_screen()) {
      $('#metamessage').blur();
  }

}

/**************************************************************************
  Change the ruleset to
**************************************************************************/
function change_ruleset(to) {
    send_message("/rulesetdir " + to);
    // reset some ruleset defined settings.
    send_message("/set nationset all");
  }


/**************************************************************************
 Shows the Freeciv intro dialog.
**************************************************************************/
function show_intro_dialog(title, message) {

  if ($.getUrlVar('autostart') == "true") {
    autostart = true;
    return;
  }

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var intro_html = message + "<br><br><table><tr><td>Player name:</td><td><input id='username_req' type='text' size='25' maxlength='31'></td></tr>"
      +  "<tr id='password_row' style='display:none;'><td>Password:</td><td id='password_td'></td></tr></table>"
	  + " <br><br><span id='username_validation_result'></span>";

  if (renderer == RENDERER_WEBGL) {
    intro_html += "<span style='color: #800000;'>The 3D WebGL version of Freeciv-web requires WebGL 3D hardware support such as Nvidia GeForce and at least 3GB of RAM.<br>" +
    "Try tuning the 3D configuration by clicking Customize Game - Game settings - 3D WebGL. Current graphics level: ";
    if (graphics_quality == QUALITY_LOW) {
      intro_html += "Low quality.";
    } else if (graphics_quality == QUALITY_MEDIUM) {
      intro_html += "Medium quality.";
    } else {
      intro_html += "High quality.";
    }
    "</span>";
  }
  $("#dialog").html(intro_html);
  var stored_username = simpleStorage.get("username", "");
  if (stored_username != null && stored_username != false) {
    $("#username_req").val(stored_username);
  }
  var stored_password = simpleStorage.get("password", "");
  if (stored_password != null && stored_password != false) {
    $("#password_req").val(stored_password);
  }
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "60%",
			beforeClose: function( event, ui ) {
			  // if intro dialog is closed, then check the username and connect to the server.
			  if (dialog_close_trigger != "button") {
			    if (validate_username()) {
			      if (!is_touch_device()) $("#pregame_text_input").focus();
			      return true;
			    } else {
			      return false;
			    }
			  }
			},
			buttons:
			[
			  {
				  text : "Start Game",
				  click : function() {
                     if (is_touch_device() || is_small_screen()) {
                       BigScreen.toggle();
                     }
					dialog_close_trigger = "button";
					autostart = true;
					validate_username_callback();
				  },
				  icons: { primary: "ui-icon-play" }
			  },
			  {
				  text : $.getUrlVar('action') == "load" ? "Load games" : "Customize Game",
				  click : function() {
                    if (is_touch_device() || is_small_screen()) {
                      BigScreen.toggle();
                    }
					dialog_close_trigger = "button";
					validate_username_callback();
				},
				icons : { primary: "ui-icon-gear" }
			  },
              {
                  text : "New user account",
                  click : function() {
                    show_new_user_account_dialog();
                },
                icons : { primary: "ui-icon-person" }
              }
			]

		});

  if (($.getUrlVar('action') == "load" || $.getUrlVar('action') == "multi" || $.getUrlVar('action') == "earthload")
         && $.getUrlVar('load') != "tutorial") {
    $(".ui-dialog-buttonset button").first().hide();
  }
  if ($.getUrlVar('action') == "observe") {
    $(".ui-dialog-buttonset button").first().remove();
    $(".ui-dialog-buttonset button").first().button("option", "label", "Observe Game");
  }

  if (is_small_screen()) {
    /* some fixes for pregame screen on small devices.*/
    $("#freeciv_logo").remove();
    $("#pregame_message_area").css("width", "73%");
    $("#observe_button").remove();
  }

  $("#dialog").dialog('open');

  $('#dialog').keyup(function(e) {
    if (e.keyCode == 13) {
      autostart = true;
      validate_username_callback();
    }
  });

  blur_input_on_touchdevice();
}


/**************************************************************************
  Validate username callback
**************************************************************************/
function validate_username_callback()
{
  var check_username = $("#username_req").val();
  var result = false;
  $.ajax({
   type: 'POST',
   url: "/validate_user?userstring=" + check_username,
   success: function(data, textStatus, request){
      if (data == "user_does_not_exist") {
        if (validate_username()) {
          if (!is_touch_device()) $("#pregame_text_input").focus();
          $("#dialog").dialog('close');
        }
      } else {
        username = $("#username_req").val().trim();
        var password = $("#password_req").val();
        if (password == null) {
          var stored_password = simpleStorage.get("password", "");
          if (stored_password != null && stored_password != false) {
            password = stored_password;
          }
        }

        if (password != null && password.length > 2) {
          $.ajax({
           type: 'POST',
           url: "/login_user?username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password),
           success: function(data, textStatus, request){
               if (data != null && data == "OK") {
                 simpleStorage.set("username", username);
                 simpleStorage.set("password", password);
                 /* Login OK! */
                 if (validate_username()) {
                   if (!is_touch_device()) $("#pregame_text_input").focus();
                   $("#dialog").dialog('close');
                 }
                 logged_in_with_password = true;
               } else {
                 $("#username_validation_result").html("Incorrect username or password. Please try again!");
               }

             },
           error: function (request, textStatus, errorThrown) {
             swal("Login user failed.");
           }
          });
        } else {
          $("#username_validation_result").html("Please enter your password or create a new user account.");
        }

        $("#password_row").show();
        $("#password_req").focus();
        $("#password_td").html("<input id='password_req' type='password' size='25' maxlength='200'>");
      }
    },
   error: function (request, textStatus, errorThrown) {
     swal("Error. Please try again with a different name.");
   }
  });

}


/**************************************************************************
 Shows the create new user account with password dialog.
**************************************************************************/
function show_new_user_account_dialog()
{

  var title = "New user account";
  var message = "Create a new Freeciv-web user account with information about yourself:<br><br>"
                + "<table><tr><td>Username:</td><td><input id='username' type='text' size='25' maxlength='30' onkeyup='return forceLower(this);'></td></tr>"
                + "<tr><td>Email:</td><td><input id='email' type='email' size='25' maxlength='64' ></td></tr>"
                + "<tr><td>Password:</td><td><input id='password' type='password' size='25'></td></tr>"
                + "<tr><td>Confim password:</td><td><input id='confirm_password' type='password' size='25'></td></tr></table><br>"
                + "<div id='username_validation_result'></div><br>"
                + "Click to accept captcha:<br>"
                + "<div id='captcha_element'></div><br><br>"
                + "<div><small><ul><li>It is free and safe to create a new account on Freeciv-web.</li>"
                + "<li>A user account allows you to save and load games.</li>"
                + "<li>Other players can use your username to start Play-by-email games with you.</li>"
                + "<li>You will not receive any spam and your e-mail address will be kept safe.</li>"
                + "<li>You can cancel your account at any time if you want.</li>"
                + "</ul></small></div>";

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "60%",
			buttons:
			{
                "Cancel" : function() {
	                init_common_intro_dialog();
				},
				"Signup new user" : function() {
                    create_new_freeciv_user_account_request("normal");
				}
			}
		});

  $("#dialog").dialog('open');

  if (grecaptcha != null) {
    grecaptcha.render('captcha_element', {
          'sitekey' : '6LfpcgMTAAAAAPRAOqYy6ZUhuX6bOJ7-7-_1V0FL'
        });
  } else {
    swal("Captcha not available. This could be caused by a browser plugin.");
  }

  $("#username").blur(function() {
   $.ajax({
     type: 'POST',
     url: "/validate_user?userstring=" + $("#username").val(),
     success: function(data, textStatus, request) {
        if (data != "user_does_not_exist") {
          $("#username_validation_result").html("The username is already taken. Please choose another username.");
          $(".ui-dialog-buttonset button").button("disable");
        } else {
          $("#email").blur(function() {
          $.ajax({
            type: 'POST',
            url: "/validate_user?userstring=" + $("#email").val(),
            success: function(data, textStatus, request) {
               if (data == "invitation") {
                 $("#username_validation_result").html("");
                 $(".ui-dialog-buttonset button").button("enable");
               } else {
                 $("#username_validation_result").html("The e-mail is already registered. Please choose another.");
                 $(".ui-dialog-buttonset button").button("disable");

               }
             }
           });
         });
        }
      }
    });
  });
}

/**************************************************************************
  This will try to create a new Freeciv-web user account with password.
**************************************************************************/
function create_new_freeciv_user_account_request(action_type)
{
  username = $("#username").val().trim().toLowerCase();
  var password = $("#password").val().trim();
  var confirm_password = $("#confirm_password").val().trim();
  var email = $("#email").val().trim();
  var captcha = $("#g-recaptcha-response").val();

  var cleaned_username = username.replace(/[^a-zA-Z]/g,'');

  if (!validateEmail(email)) {
    $("#username_validation_result").html("Invalid email address.");
    return false;
  } else if (username == null || username.length == 0 || username == "pbem") {
    $("#username_validation_result").html("Your name can't be empty.");
    return false;
  } else if (username.length <= 2 ) {
    $("#username_validation_result").html("Your name is too short.");
    return false;
  } else if (password == null || password.length <= 2 ) {
    $("#username_validation_result").html("Your password is too short.");
    return false;
  } else if (username.length >= 32) {
    $("#username_validation_result").html("Your name is too long.");
    return false;
  } else if (username != cleaned_username) {
    $("#username_validation_result").html("Your name contains invalid characters, only the English alphabet is allowed.");
    return false;
  } else if (password.indexOf("%") != -1 || password.indexOf("#") != -1 || password.indexOf("&") != -1 || password.indexOf("?") != -1) {
    $("#username_validation_result").html("Please don't use these characters in passwords: % # & ?");
    return false;
  } else if (password != confirm_password) {
    $("#username_validation_result").html("The passwords do not match.");
    return false;
  } else if (captcha == null || captcha === undefined ) {
    $("#username_validation_result").html("Please fill in the captcha. You might have to disable some plugins to see the captcha.");
    return false;
  }

  $("#username_validation_result").html("");

  $("#dialog").parent().hide();

  $.ajax({
   type: 'POST',
   url: "/create_pbem_user?username=" + encodeURIComponent(username) + "&email=" + encodeURIComponent(email)
            + "&password=" + encodeURIComponent(password) + "&captcha=" + encodeURIComponent(captcha),
   success: function(data, textStatus, request){
       simpleStorage.set("username", username);
       simpleStorage.set("password", password);
       if (action_type == "pbem") {
         challenge_pbem_player_dialog();
       } else {
         $("#dialog").dialog('close');
         network_init();
         logged_in_with_password = true;
       }

      },
   error: function (request, textStatus, errorThrown) {
     $("#dialog").parent().show();
     swal("Creating new user failed.");
   }
  });
}