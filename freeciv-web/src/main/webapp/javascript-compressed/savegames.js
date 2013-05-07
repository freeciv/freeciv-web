/* handles the load game screen. */
$(document).ready(function() {
  $( ".button").button();
  $( ".load").css("width", "400px");
  $( ".load").css("margin", "10px");

  var savegame_count = $.jStorage.get("savegame-count", 0);

  if (savegame_count == 0) {
      var saveHtml = "<tr><td>No savegames found. Please <a href='/wireframe.jsp?do=menu'>start a new game</a>.</td></tr>";

    $('#saveTable tr:last').html(saveHtml);
  
  } else {
    for (var i = 1; i <= savegame_count; i++) {
      var savename = $.jStorage.get("savegame-savename-" + i);
      var username = $.jStorage.get("savegame-username-" + i);
      var saveHtml = "<tr><td><a class='button load' href='/preload.jsp?redir=/civclientlauncher?action=load&load="+ i +"' title='Load savegame'>" + savename + " (" + username + ")</a></td></tr>";
      $('#saveTable tr:last').after(saveHtml);
    }
  }

});

