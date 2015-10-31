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
var choosing_player = -1;
var ai_skill_level = 2;
var nation_select_id = -1;
var metamessage_changed = false;

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
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/detach"};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);

  } else {
    $("#observe_button").button("option", "label", "Don't observe");
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/observe"};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  }

  observing = !observing;
}

/****************************************************************************
  ...
****************************************************************************/
function update_player_info_pregame()
{
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

    $("#pregame_player_list").contextMenu({
        selector: '.pregame_player_name', 
        callback: function(key, options) {
            var name = $(this).attr('name');
            if (name != null && name.indexOf(" ") != -1) name = name.split(" ")[0]
            var playerid = parseInt($(this).attr('playerid'));
            if (key == "take_player") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/take " + name};
              send_request(JSON.stringify(msg_packet));
            } else if (key == "pick_nation") {
              pick_nation(playerid);
            } else if (key == "aitoggle_player") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/ai " + name};
              send_request(JSON.stringify(msg_packet));
            } else if (key == "observe_player") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/observe " + name};
              send_request(JSON.stringify(msg_packet));
            } else if (key == "novice") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/novice " + name};
              send_request(JSON.stringify(msg_packet));
            } else if (key == "easy") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/easy " + name};
              send_request(JSON.stringify(msg_packet));
            } else if (key == "normal") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/normal " + name};
              send_request(JSON.stringify(msg_packet));
            } else if (key == "hard") {
              var msg_packet = {"pid" : packet_chat_msg_req, "message" : "/hard " + name};
              send_request(JSON.stringify(msg_packet));
            }  
        },
        items: {
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
        }
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
  ...
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
  var nation_name_list = [];
  for (var nation_id in nations) {
    var pnation = nations[nation_id];
    if (pnation['is_playable']) {
      var flag_url = get_nation_flag_url(pnation);
      var flag_html = "<img class='pick_nation_flags' src='" + flag_url +"'>"
      nations_html += "<div class='nation_pickme_line' onclick='select_nation(" + nation_id + ");'>"
             + flag_html + "<div id='nation_" + nation_id + "' class='nation_choice'>" + pnation['adjective'] + "</div></div>";
      nation_name_list.push(pnation['adjective']);
    }
  }

  nations_html += "</div><div id='nation_legend'></div><div id='select_nation_flag'></div>";

  $("#pick_nation_dialog").html(nations_html);
  $("#pick_nation_dialog").attr("title", "Pick nation");
  $("#pick_nation_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "60%",
			buttons: {
				Ok: function() {
					$("#pick_nation_dialog").dialog('close');
					submit_nation_choice();
				}
			}
		});

  $("#nation_legend").html("Please choose nation for " + pplayer['name'] + " carefully.");


  $("#nation_autocomplete_box").autocomplete({
      source: nation_name_list
  });

  if (is_small_screen()) {
    $("#select_nation_flag").hide();
    $("#nation_legend").hide();
    $("#nation_list").width("80%");
  }

  nation_select_id = setTimeout ("update_nation_selection();", 200);
  $("#pick_nation_dialog").dialog('open');

}

/****************************************************************************
  ...
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
  nation_select_id = setTimeout ("update_nation_selection();", 200);
}

/****************************************************************************
  ...
****************************************************************************/
function select_nation(new_nation_id)
{
  var pnation = nations[new_nation_id];
  $("#nation_legend").html(pnation['legend']);
  $("#nation_autocomplete_box").val(pnation['adjective']);
  $("#nation_" + chosen_nation).css("background-color", "transparent");
  $("#nation_choice").html("Chosen nation: " + pnation['adjective']);
  $("#select_nation_flag").html("<img src='/images/flags/"
		            + pnation['graphic_str'] + "-web.png' width='180'>");

  if (chosen_nation != new_nation_id && $("#nation_" + new_nation_id).length > 0) {
    $("#nation_" + new_nation_id).get(0).scrollIntoView();
  }

  chosen_nation = parseFloat(new_nation_id);
  $("#nation_" + chosen_nation).css("background-color", "#AAFFAA");
}

/****************************************************************************
  ...
****************************************************************************/
function submit_nation_choice()
{
  if (chosen_nation == -1 || client.conn['player_num'] == null 
      || choosing_player == null || choosing_player < 0) return;

  var leader_name = nations[chosen_nation]['leader_name'][0];
  if (choosing_player == client.conn['player_num']) {
    leader_name = client.conn['username'];
  }

  var test_packet = {"pid" : packet_nation_select_req,
                     "player_no" : choosing_player,
                     "nation_no" : chosen_nation,
                     "is_male" : true, /* FIXME */
                     "name" : leader_name,
                     "style" : nations[chosen_nation]['style']};
  send_request(JSON.stringify(test_packet));
  clearInterval(nation_select_id);
}



/****************************************************************************
  ...
****************************************************************************/
function pregame_settings()
{
  var id = "#pregame_settings";
  $(id).remove();
  $("<div id='pregame_settings'></div>").appendTo("div#pregame_page");


  var dhtml = "<table id='settings_table'>" +
  	  "<tr title='Set metaserver info line'><td>Game title:</td>" +
	  "<td><input type='text' name='metamessage' id='metamessage' size='28' maxlength='42'></td></tr>" +
	  "<tr title='Enables music'><td>Music</td>" +
          "<td><input type='checkbox' name='music_setting' id='music_setting'>Play Music</td></tr>" +
	  "<tr title='Total number of players (including AI players)'><td>Number of Players (including AI):</td>" +
	  "<td><input type='number' name='aifill' id='aifill' size='4' length='3' min='0' max='20' step='1'></td></tr>" +
	  "<tr title='Maximum seconds per turn'><td>Timeout (seconds per turn):</td>" +
	  "<td><input type='number' name='timeout' id='timeout' size='4' length='3' min='30' max='3600' step='1'></td></tr>" +
          "<tr title='Creates a private game where players need to know this password in order to join.'><td>Password for private game:</td>" +
	  "<td><input type='text' name='password' id='password' size='10' length='10'></td></tr>" +
	  "<tr title='Map size (in thousands of tiles)'><td>Map size:</td>" +
	  "<td><input type='number' name='mapsize' id='mapsize' size='4' length='3' min='1' max='15' step='1'></td></tr>" +
	  "<tr title='This setting sets the skill-level of the AI players'><td>AI skill level:</td>" +
	  "<td><select name='skill_level' id='skill_level'>" +
	  "<option value='0'>Handicapped</option>" +
	  "<option value='1'>Novice</option>" +
	  "<option value='2'>Easy</option>" +
          "<option value='3'>Normal</option>" +
          "<option value='4'>Hard</option>" +
          "<option value='5'>Cheating</option>" +
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
	  "<tr title='Enables score graphs for all players, showing score, population, techs and more."+
          " This will lead to information leakage about other players.'><td>Score graphs</td>" +
          "<td><input type='checkbox' name='scorelog_setting' id='scorelog_setting' checked>Enable score graphs</td></tr>" +
	  "<tr title='Method used to generate map'><td>Map generator:</td>" +
	  "<td><select name='generator' id='generator'>" +
	  "<option value='random'>Fully random height</option>" +
	  "<option value='fractal'>Pseudo-fractal height</option>" +
          "<option value='island'>Island-based</option>" +
          "<option value='fair'>Fair islands</option>" +
	  "</select></td></tr>"+
  	  "<tr title='Ruleset version'><td>Ruleset:</td>" +
	  "<td><select name='ruleset' id='ruleset'>" +
	  "<option value='fcweb'>Default Fcweb</option>" +
	  "<option value='webperimental'>Webperimental</option>" +
	  "</select></td></tr>"+
          "</table><br>" +
	  "<span id='settings_info'><i>Freeciv-web can be customized using the command line in many other ways also. Type /help in the command line for more information.</i></span>"
	  ;
  $(id).html(dhtml);

  $(id).attr("title", "Game Settings");
  $(id).dialog({
			bgiframe: true,
			modal: false,
			width: "680",
			  buttons: {
				Ok: function() {
					$("#pregame_settings").dialog('close');
				}
			  }

  });

  if (game_info != null) {
    $("#aifill").val(game_info['aifill']);
    $("#mapsize").val(game_info['mapsize']);
    $("#timeout").val(game_info['timeout']);
    $("#skill_level").val(ai_skill_level);
    $("#metamessage").val(game_info['meta_message']);
    $("#techlevel").val("0");
    $("#landmass").val("30");
    $("#specials").val("250");
    $("#citymindist").val("2");
    $("#endturn").val("5000");
  }

  $(id).dialog('open');

  $('#aifill').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set aifill " + $('#aifill').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#metamessage').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/metamessage " + $('#metamessage').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
    metamessage_changed = true;
  });

  $('#metamessage').bind('keyup blur',function(){
    var cleaned_text = $(this).val().replace(/[^a-zA-Z\s\-]/g,'');
    if ($(this).val() != cleaned_text) {
      $(this).val( cleaned_text ); }
    }
  );

  $('#mapsize').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set size " + $('#mapsize').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#timeout').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set timeout " + $('#timeout').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#techlevel').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set techlevel " + $('#techlevel').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#specials').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set specials " + $('#specials').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#landmass').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set landmass " + $('#landmass').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#citymindist').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set citymindist " + $('#citymindist').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#endturn').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set endturn " + $('#endturn').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
  });

  $('#generator').change(function() {
    var test_packet = {"pid" : packet_chat_msg_req, "message" : "/set generator " + $('#generator').val()};
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);

  });

  $('#ruleset').change(function() {
    var packet = {"pid" : packet_chat_msg_req, "message" : "/rulesetdir " + $('#ruleset').val()};
    send_request(JSON.stringify(packet));

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
      closeOnConfirm: false }, 
      function(){   
        var pwd_packet = {"pid" : packet_authentication_reply, "password" : $('#password').val()};
        send_request(JSON.stringify(pwd_packet));

        var pwd_packet = {"pid" : packet_chat_msg_req, "message" : "/metamessage Private password-protected game"};
        send_request(JSON.stringify(pwd_packet));
        metamessage_changed = true;
        $("#metamessage").prop('readonly', true);
        $("#metamessage_setting").prop('readonly', true);
        $("#password").prop('readonly', true);
     });
   });


  $('#skill_level').change(function() {
    ai_skill_level = parseFloat($('#skill_level').val());
    var test_packet = "";
    if (ai_skill_level == 0) {
      test_packet = {"pid" : packet_chat_msg_req, "message" : "/handicapped"};
    } else if (ai_skill_level == 1) {
      test_packet = {"pid" : packet_chat_msg_req, "message" : "/novice"};
    } else if (ai_skill_level == 2) {
      test_packet = {"pid" : packet_chat_msg_req, "message" : "/easy"};
    } else if (ai_skill_level == 3) {
      test_packet = {"pid" : packet_chat_msg_req, "message" : "/normal"};
    } else if (ai_skill_level == 4) {
      test_packet = {"pid" : packet_chat_msg_req, "message" : "/hard"};
    } else if (ai_skill_level == 5) {
      test_packet = {"pid" : packet_chat_msg_req, "message" : "/cheating"};
    }
    var myJSONText = JSON.stringify(test_packet);
    send_request(myJSONText);
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
      var net_packet = {"pid" : packet_chat_msg_req, "message" : "/set scorelog enabled"};
      send_request(JSON.stringify(net_packet));
    } else {
      var net_packet = {"pid" : packet_chat_msg_req, "message" : "/set scorelog disabled"};
      send_request(JSON.stringify(net_packet));
    }
  });


  $("#settings_table").tooltip();
}

