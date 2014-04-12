
$(document).ready(function() {

  var savegame_count = simpleStorage.get("savegame-count", 0);

  if (savegame_count == 0) {  
    $('#load-button').addClass("disabled");
    $('#load-button').attr("href", "#");
  } else {
    $('#load-button').addClass("btn-success");
  }

});

