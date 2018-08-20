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


/****************************************************************************
 Shows the game scores dialog.
****************************************************************************/
function view_game_scores() {
  set_default_mapview_active();
  $("#scores_dialog").remove();
  $("<div id='scores_dialog'></div>").appendTo("div#game_page");


  var dialog_html = "<center><div id='scores_wait'>Please wait while generating score graphs...</div></center>"
    +"</div><div id='scores_tabs'><ul id='scores_ul'></ul></div>";

  $("#scores_dialog").html(dialog_html);
  $("#scores_dialog").attr("title", "Game Scores");
  $("#scores_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "80%",
                        height: is_small_screen() ? 560 : 710,
			buttons: {
				Ok: function() {
					$("#scores_dialog").dialog('close');
                                        $("#scores_tabs").remove();
  					$("#scores_dialog").remove();
				}
			}
		});

  $("#scores_dialog").dialog('open');


  $.ajax({
    url: "/data/scorelogs/score-" + civserverport + ".log",
    dataType: "html",
    cache: false,
    async: true
  }).fail(function() {
    $("#scores_wait").html("Score graphs not shown, because server 'scorelog' variable is disabled,"
      + " or because of problem loading the score file.");
    $("#game_scores_button").button( "option", "disabled", true);
  }).done(function( data ) {
    handle_scorelog(data);
  });


}

/****************************************************************************
 Handles the scorelog file
****************************************************************************/
function handle_scorelog(scorelog) {
  var start_turn = 0;
  var scoreitems = scorelog.split("\n");
  var scoreplayers = {};
  var playerslist = [];
  var playernames = [];
  var scoretags = {};
  var resultdata = {};
  var scorecolors = [];
  for (var i = 0; i < scoreitems.length; i++) {
    var scoreitem = scoreitems[i];
    var scoredata = scoreitem.split(" ");
    if (scoredata.length >= 3) {
      if (scoredata[0] == "addplayer") {
        var pname = scoredata[3];
        for (var s = 4; s < scoredata.length; s++) {
          pname += " " + scoredata[s];
        }
        scoreplayers[scoredata[2]] = pname;
        playerslist.push(scoredata[2]);
        playernames.push(pname);
        var pplayer = player_by_name(pname);
        if (pplayer == null) {
          scorecolors.push("#ff0000"); 
        } else {
          scorecolors.push(nations[pplayer['nation']]['color']); 
        }

      } else if (scoredata[0] == "turn") {
        if (start_turn==0) start_turn = scoredata[1];

      } else if (scoredata[0] == "tag") {
        scoretags[scoredata[1]] = scoredata[2];

      } else if (scoredata[0] == "data") {
        var turn = scoredata[1];
        var tag = scoredata[2];
        var player = scoredata[3];
        var value = scoredata[4];
        if (resultdata[tag] == null) {
          var s = {};
          s["turn"] = turn;
          s[player] = parseInt(value);
          resultdata[tag] = [];
          resultdata[tag][turn - start_turn] = s;
        } else if (resultdata[tag] != null && resultdata[tag][turn - start_turn] == null) {
          var s = {};
          s["turn"] = turn;
          s[player] = parseInt(value);
          resultdata[tag][turn - start_turn] = s;
        } else if (resultdata[tag][turn - start_turn] != null) {
          resultdata[tag][turn - start_turn][player] = parseInt(value);
        }
      }
    }
  }
  if (is_small_screen()) scoretags = {"0" : "score"};

  for (var key in scoretags) {
    var tagname = scoretags[key];
    $("#scores_ul").append("<li><a href='#scores-tabs-" + key + "' class='scores_tabber'>" + get_scorelog_name(tagname) + "</a></li>");
    $("#scores_tabs").append("<div id='scores-tabs-" + key + "'><div id='scoreschart-" + key +  "' class='scorechart'></div>"
      + "<center><b>" + get_scorelog_name(tagname) + "</b></center></div>");
  }

  var ps = 4;
  if (scoreitems.length >1000) ps = 0;

  for (var key in scoretags) {
    try {
      Morris.Line({
        element: 'scoreschart-' + key,
        data: resultdata[key],
        xkey: 'turn',
        ykeys: playerslist,
        labels: playernames,
        parseTime: false,
        lineColors : scorecolors,
        pointSize: ps
      });
    } catch(err) {
      console.log("Problem showing score log graph: " + err);
    }
  }

  $("#scores_tabs").tabs();
  $(".scores_tabber").css("padding", "1px");
  $("#scores_wait").hide();
  if (is_small_screen()) {
    $(".scorechart").height($("#scores_dialog").height() - 10);
  }
}

/****************************************************************************
...
****************************************************************************/
function get_scorelog_name(tag) {
  var names = {
  "score" : "Score",
  "pop" : "Population",
  "bnp" : "Economics",
  "mfg" : "Production",
  "cities" : "Cities",
  "techs" : "Techs",
  "munits" : "Military units", 
  "wonders" : "Wonders",
  "techout" : "Tech output", 
  "landarea" : "Land area",
  "settledarea" : "Settled area",
  "gold" : "Gold",
  "unitsbuilt" : "Units built",
  "unitskilled" : "Units killed",
  "unitslost" : "Units lost" 
  };
  return names[tag];
}

