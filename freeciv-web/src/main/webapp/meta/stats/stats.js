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
    if (cells[1] == "") cells[1] = 0;
    if (cells[2] == "") cells[2] = 0;
    if (cells[3] == "") cells[3] = 0;
    if (cells[4] == "") cells[4] = 0;
    if (cells[5] == "") cells[5] = 0;
    var data_item = {y : cells[0], a : parseInt(cells[1]), b : parseInt(cells[2]), c : parseInt(cells[3]), d : parseInt(cells[4]), e: parseInt(cells[5])};
    mydata.push(data_item);
  }

Morris.Line({
  element: 'myfirstchart',
  data: mydata,
  xkey: 'y',
  ykeys: ['a', 'b', 'c', 'd','e'],
  labels: ['Freeciv-web singleplayer', 'Freeciv-web multiplayer', 'Freeciv-web PBEM', 'Freeciv desktop multiplayer', 'Freeciv-web hotseat']
});
}
