$(document).ready(function() {
 $.ajax({
    url: "/meta/games-stats.php",
    dataType: "html",
    cache: false,
    async: true
  }).done(function( data ) {
    handle_stats(data);
  });



});


function handle_stats(data)
{
  var mydata = [];
  var rows = data.split(";");
  for (var i = 0; i < rows.length; i++) {
    var cells = rows[i].split(",");
    if (cells[0].length == 0) continue;
    var data_item = {y : cells[0], a : cells[1], b : cells[2], c : cells[3], d : cells[4]};
    mydata.push(data_item);
  }

Morris.Line({
  element: 'myfirstchart',
  data: mydata,
  xkey: 'y',
  ykeys: ['a', 'b', 'c', 'd'],
  labels: ['Freeciv-web singleplayer', 'Freeciv-web multiplayer', 'Freeciv-web PBEM', 'Freeciv desktop multiplayer']
});

}
