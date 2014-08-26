
$(document).ready(function() {

  var savegame_count = simpleStorage.get("savegame-count");

  if (savegame_count == null || savegame_count == 0) {  
    $('#load-button').addClass("disabled");
    $('#load-button').attr("href", "#");
  } else {
    $('#load-button').addClass("btn-success");
  }

});

