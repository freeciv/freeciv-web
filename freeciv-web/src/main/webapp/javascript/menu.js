
$(document).ready(function() {

  var savegames = simpleStorage.get("savegames");

  if (savegames == null || savegames.length == 0) {  
    $('#load-button').addClass("disabled");
    $('#load-button').attr("href", "#");
  } else {
    $('#load-button').addClass("btn-success");
  }

  if ($(window).width() <= 768) {
    $(".nav").hide();
  }

    if (navigator.userAgent.toLowerCase().indexOf('android') > -1) {
      $("#chromews").hide();
      $("#mozws").hide();
      $("#windowsstore").hide();
      $("#playws").show();
    } else if (navigator.userAgent.toLowerCase().indexOf('chrome') > -1) {
      $("#chromews").show();
      $("#mozws").hide();
      $("#windowsstore").hide();
      $("#playws").hide();
    } else if (navigator.userAgent.toLowerCase().indexOf('firefox') > -1) {
      $("#mozws").show();
      $("#chromews").hide();
      $("#windowsstore").hide();
      $("#playws").hide();
    } else if (navigator.userAgent.toLowerCase().indexOf('trident') > -1) {
      $("#mozws").hide();
      $("#chromews").hide();
      $("#windowsstore").show();
      $("#playws").hide();
    } else {
      $("#mozws").hide();
      $("#chromews").hide();
      $("#windowsstore").hide();
      $("#playws").hide();
    }



$.ajax({
  url: "/meta/fpinfo.php",
  cache: true
})
  .done(function( html ) {
    $( "#metalink" ).html("Live Games: " +  html );
  });

});

