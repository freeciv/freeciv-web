
$(document).ready(function() {

  var savegames = simpleStorage.get("savegames");

  if (savegames == null || savegames.length == 0) {  
    $('#load-button').addClass("disabled");
    $('#load-button').attr("href", "#");
  } else {
    $('#load-button').addClass("btn-success");
  }


$.ajax({
  url: "/meta/fpinfo.php",
  cache: true
})
  .done(function( html ) {
    $( "#metalink" ).html("Live Games: " +  html );
  });

});

