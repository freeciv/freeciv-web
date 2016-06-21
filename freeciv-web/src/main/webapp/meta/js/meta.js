/* javascript for metaserver page. */
$(document).ready(function() {

	$( ".button").button();
	$( ".button").css("font-size", "13px");
	$(".button").css("margin", "7px");
	if (window.location.href.indexOf("tab") != -1) {
		$( "#tabs" ).tabs({ active: 1 });
	 } else { 
		$( "#tabs" ).tabs();
	}

  $.ajax({
    url: "/mailstatus",
    dataType: "html",
    cache: true,
    async: true
  }).done(function( data ) {
    handle_pbem(data);
  });

  $.ajax({
    url: "/meta/pbem_games_report.php",
    dataType: "html",
    cache: true,
    async: true
  }).done(function( data ) {
    handle_pbem_results(data);
  });


});

/****************************************************************************
  Updates metaserver with information about running games
****************************************************************************/
function handle_pbem(data)
{
  if (data.indexOf("[[") != -1) {
    var gamestats_txt = data.substring(data.indexOf("[["), data.indexOf("]]") + 2);
    var gamestats = jQuery.parseJSON(gamestats_txt);
    $("#pbemplr").html($("#pbemplr").html()+" ("+gamestats.length+")");
    for (var i = 0; i < gamestats.length; i++) {
      var game = gamestats[gamestats.length-i-1];
      var turn = game[0];
      var phase = game[1];
      var player_one = game[2][0];
      var player_two = game[2][1];
      if (player_one.indexOf("@") != -1 || player_two.indexOf("@") != -1) continue;
      var current_player = game[2][phase];
      var last_played = game[3];
      var time_left = game[4];
      $("#pbem_table").append("<tr><td>" + player_one + " - " + player_two + "</td><td>" + current_player 
           + "</td><td>" + turn + "</td><td>" + last_played + "</td><td>" + time_left + " hours</td></tr>");
    }
  } else {
    $("#pbem_table").hide();
  }
}


/****************************************************************************
  Shows scores on the game details page.
****************************************************************************/
function show_scores(port) {
  $.ajax({
    url: "/data/scorelogs/score-" + port + ".log",
    dataType: "html",
    cache: false,
    async: true
  }).fail(function() {
    $("#scores_heading").html("Score graphs not enabled.")
    console.log("Unable to load scorelog file.");
  }).done(function( data ) {
    handle_scorelog(data);
  });
}

/****************************************************************************
 Handles the scorelog file
****************************************************************************/
function handle_scorelog(scorelog) {
  var start_turn = 1;
  var scoreitems = scorelog.split("\n");
  var scoreplayers = {};
  var playerslist = [];
  var playernames = [];
  var scoretags = {};
  var resultdata = {};
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
        } else if (resultdata[tag] != null && resultdata[tag][turn-start_turn] == null) {
          var s = {};
          s["turn"] = turn;
          s[player] = parseInt(value);
          resultdata[tag][turn - start_turn] = s;
        } else if (resultdata[tag][turn-start_turn] != null) {
          resultdata[tag][turn-start_turn][player] = parseInt(value);
        }
      }
    }
  }

  var ps = 4;
  if (scoreitems.length >1000) ps = 0;

  $("#scores").show();
  Morris.Line({
      element: 'scores',
      data: resultdata[0],
      xkey: 'turn',
      ykeys: playerslist,
      labels: playernames,
      parseTime: false,
      pointSize: ps
  });

  $("#scores_tabs").tabs();
  $(".scores_tabber").css("padding", "1px");
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

function toTitleCase(str)
{
  return str.replace(/\w\S*/g, function(txt){return txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase();});
}

/****************************************************************************
  Updates metaserver with information about completed PBEM games. 
****************************************************************************/
function handle_pbem_results(data)
{
  var games = data.split(";");
  for (var i = 0; i < games.length -1; i++) {
    var game = games[i].split(",");
    $("#pbem_result_table").append("<tr><td>" + game[1] + "</td><td>" + game[0]
      + "</td><td>" + game[2] + " (wins: " + game[4] + ")</td><td>"    
      + game[3] + " (wins: " + game[5] + ")</td></tr>");
  }

}

