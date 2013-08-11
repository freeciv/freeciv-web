
function help_single() {
  $("#intro-message").html("<b>Single-player:</b><br> Start new single-player game. "
   + "Here you can play against a number of artificial intelligence (AI) players.");
}

function help_multi() {
  $("#intro-message").html("<b>Multi-player:</b><br> Join a multiplayer game against other players. "
  + " Multiplayer games requires at least 2 human players before they can begin. "
  + " Before the game has begun, it will be in a pregame state, where you have to wait until "
  + " enough players have joined. Once all players have joined, all players have to click "
  + " the start game button.");
}

function help_scenario() {
  $("#intro-message").html("<b>Scenario Game:</b><br> Start new scenario-game, which allows you to play a prepared game,"
  + " on the whole World, the British Aisles or the Iberian peninsula.");
}

function help_load() {
  $("#intro-message").html("<b>Load Savegame:</b><br> You can resume playing games that you have "
   + "played before, by clicking the <Load saved game> button. This requires HTML5 localstorage.");
}


function help_tutorial() {
  $("#intro-message").html("<b>Play Tutorial Game:</b><br> Play this tutorial scenario to get an introduction to Freeciv-web. This is recommended the first time you play Freeciv-web.");
}

$(document).ready(function() {

  if (!('ontouchstart' in document.documentElement)) {
    $('#single-button').mouseover(help_single);
    $('#multi-button').mouseover(help_multi);
    $('#scenarios-button').mouseover(help_scenario);
    $('#tutorial-button').mouseover(help_tutorial);
    $('#scenarios-button').mouseover(help_scenario);
    $('#load-button').mouseover(help_load);
  }

  var savegame_count = $.jStorage.get("savegame-count", 0);

  if (savegame_count == 0) {  
    $('#load-button').addClass("disabled");
    $('#load-button').attr("href", "#");
  } else {
    $('#load-button').addClass("btn-success");
  }

});

