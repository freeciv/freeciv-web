
$(document).ready(function() {


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
  Updates metaserver with information about completed PBEM games. 
****************************************************************************/
function handle_pbem_results(data)
{
  var games = data.split(";");
  for (var i = 0; i < games.length -1; i++) {
    var game = games[i].split(",");
    $("#pbem_table").append("<tr><td>" + game[1] + "</td><td>" + game[0]
      + "</td><td>" + game[2] + " (wins: " + game[4] + ")</td><td>"    
      + game[3] + " (wins: " + game[5] + ")</td></tr>");
  }

}



