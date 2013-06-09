
function help_single() {
  $("#menu_help").html("<b>Help:</b><br> Start new single player game. Here you can play against a "
   + " number of artificial intelligence (AI) players.");
}

function help_multi() {
  $("#menu_help").html("<b>Help:</b><br> Join a multiplayer game against other players. "
  + " Multiplayer games requires at least 2 human players before they can begin. "
  + " Before the game has begun, it will be in a pregame state, where you have to wait until "
  + " enough players have joined. Once all players have joined, all players have to click "
  + " the start game button.");
}

function help_scenario() {
  $("#menu_help").html("<b>Help:</b><br> Start new scenario-game, which allows you to play a prepared game,"
  + " on the whole World, the British Aisles or the Iberian peninsula.");
}

function help_load() {
  $("#menu_help").html("<b>Help:</b><br> You can resume playing games that you have "
   + "played before, by clicking the <Load saved game> button. This requires HTML5 localstorage.");
}


function help_tutorial() {
  $("#menu_help").html("<b>Help:</b><br> Play this tutorial scenario to get an introduction to Freeciv-web. This is recommended the first time you play Freeciv-web.");
}

$(document).ready(function() {
$( ".button").button();
$( ".button").css("width", "230px");
$( ".button").css("margin", "10px");

});

