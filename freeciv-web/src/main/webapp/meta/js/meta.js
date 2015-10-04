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
});


function show_scores(port) {
  $.ajax({
    url: "/scorelogs/score-" + port + ".log",
    dataType: "html",
    cache: false,
    async: true
  }).fail(function() {
    console.log("Unable to load scorelog file.");
  }).done(function( data ) {
    handle_scorelog(data);
  });



}




/****************************************************************************
 Handles the scorelog file
****************************************************************************/
function handle_scorelog(scorelog) {
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
          resultdata[tag][turn] = s;
        } else if (resultdata[tag] != null && resultdata[tag][turn] == null) {
          var s = {};
          s["turn"] = turn;
          s[player] = parseInt(value);
          resultdata[tag][turn] = s;
        } else if (resultdata[tag][turn] != null) {
          resultdata[tag][turn][player] = parseInt(value);
        }
      }
    }
  }

  var tagname = scoretags[0];
  Morris.Line({
      element: 'scores',
      data: resultdata[0],
      xkey: 'turn',
      ykeys: playerslist,
      labels: playernames,
      parseTime: false
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

