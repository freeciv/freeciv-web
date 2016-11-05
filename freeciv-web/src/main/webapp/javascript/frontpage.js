/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

$(document).ready(function() {
  $("#feedback_submit").click(function() {
    $.ajax({
      url: "/feedbackform.php?feedback=" + $("#feedback_form").val()
    }).done(function( html ) {
      alert("Thanks for your feedback!");
    });
  });

    if (navigator.userAgent.toLowerCase().indexOf('android') > -1) {
      $("#chromews").hide();
      $("#windowsstore").hide();
      $("#playws").show();
    } else if (navigator.userAgent.toLowerCase().indexOf('chrome') > -1) {
      $("#chromews").show();
      $("#windowsstore").hide();
      $("#playws").hide();
    } else if (navigator.userAgent.toLowerCase().indexOf('trident') > -1) {
      $("#chromews").hide();
      $("#windowsstore").show();
      $("#playws").hide();
    } else {
      $("#chromews").hide();
      $("#windowsstore").hide();
      $("#playws").hide();
    }


  $.ajax({
    url: "/meta/fpinfo.php",
    cache: true
  }).done(function( game_stats ) {
    var stats = game_stats.split(";");
    if (stats.length == 3) {
      $( "#metalink" ).html("Online Games: " +  stats[0]);
      $( "#single_count" ).html(stats[1]);
      $( "#multi_count" ).html(stats[2]);
    }
  });


$.ajax({
  url: "/meta/fpmultimeta.php",
  cache: true
})
  .done(function( html ) {
    $( "#fpmultimeta" ).html(html);
  });


  $.ajax({
    url: "/meta/pbem_top_players.php",
    dataType: "html",
    cache: true,
    async: true
  }).done(function( data ) {
    handle_top_pbem_players(data);
  });


       var count= 5;
        $.getJSON("/fpfeed.json", function(data){
                $.each(data, function(i,item){
                        if(i < count) {
                        $("#myUL").append("<li><a href='"+item.permalink+"'>"+item.title+"</a></li>");
                        }
                });
        });
});


$( window ).resize(function() {
  $("#mysearchform").get(0).scrollIntoView();
});


function handle_top_pbem_players(data)
{
  var games = data.split(";");
  for (var i = 0; i < games.length -1; i++) {
    var player = games[i].split(",");
    $("#pbem_top_players").append("<tr style='background-color: rgba(230,230,230, 0.5);'><td>" 
      + player[0] + "</td><td>" + player[1]
      + "</td></tr>");
  }

}

