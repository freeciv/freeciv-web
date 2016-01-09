/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2016  The Freeciv-web project

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

/* Freeciv-web Earth map processor in JS. Andreas R. */

var width = 120;
var height = 80;

L.mapbox.accessToken = 'pk.eyJ1IjoiYW5kcmVhc3Jvc2RhbCIsImEiOiJjaWo2d2piZnMwMDF0MHJsdjZ2NHBxdmhhIn0.36znzLi2ExS8gwXauDgt3w';
var map = L.mapbox.map('map', 'mapbox.satellite')
    .setView([35, 25], 2);
document.getElementById('snap').addEventListener('click', function() {
    $("#snap_status").html("<b>Please wait while generating map for you!</b>");
    $("#snap").hide();
    leafletImage(map, doImage);
});
  
function doImage(err, canvas) {
  var dimensions = map.getSize();
  var mycanvas = document.createElement("canvas");
  mycanvas.width = dimensions.x;
  mycanvas.height = dimensions.y;
  var ctx = mycanvas.getContext("2d");
  ctx.drawImage(canvas, 0, 0, dimensions.x, dimensions.y, 0,0, width, height);

  var savetxt = process_image(ctx);

 /* Post map to server.*/
$.ajax({
  type: "POST",
  url: "/freeciv-earth-mapgen",
  data: savetxt 
}).done(function(responsedata) {
    window.location.href="/webclient/?action=earthload&savegame=" + responsedata;
  })
  .fail(function() {
    $("#snap_status").html("Something failed. Please try again later!");
  })

}

function pixel_color(colorRed,colorGreen,colorBlue,pixel, threshold)
{
    var diffR,diffG,diffB;
    diffR=(colorRed - pixel[0]);
    diffG=(colorGreen - pixel[1]);
    diffB=(colorBlue - pixel[2]);
    return (Math.sqrt(diffR*diffR + diffG*diffG + diffB*diffB) < threshold);
}

function get_map_terrain_type(pixel) 
{
  var rnd = Math.random();
  if (pixel_color(255,255,255, pixel, 20)) return "a";  //arctic
  if (pixel_color(64, 83, 3538, pixel, 40)) return "g"; //grassland
  if (pixel_color(84, 73, 38, pixel, 20)) return "t";   //tundra
  if (pixel_color(240, 220, 155, pixel, 100)) return "d"; //desert
  if (pixel_color(44, 52, 28, pixel, 10)) return "f"; //forest
  if (pixel_color(37, 64, 27, pixel, 10)) return "g"; //jungle
  if (pixel_color(60, 89, 38, pixel, 30)) return "g"; //jungle
  if (pixel_color(204, 99, 40, pixel, 100)) return "d"; //desert
  if (pixel_color(67, 72, 31, pixel, 10)) return "m"; //mountains
  if (pixel_color(61, 79, 31, pixel, 10)) return "h"; //hills
  if (pixel_color(4, 10, 20, pixel, 10)) return ":";  //ocean
  if (pixel_color(8, 20, 36, pixel, 10)) return " ";  //coast
  if (pixel_color(39, 57, 61, pixel, 30)) return " ";  //coast

  if (rnd < 3) return "p"; //plains
  if (rnd >= 3 && rnd <= 5) return "h";  //hills
  if (rnd >= 5 && rnd < 8) return "t";  //tundra 
  if (rnd >= 8) return "g";  //grassland 
}

function zeroFill( number, width )
{
  width -= number.toString().length;
  if ( width > 0 )
  {
    return new Array( width + (/\./.test( number ) ? 2 : 1) ).join( '0' ) + number;
  }
  return number + ""; // always return a string
}

function process_image(ctx)
{
 var mapline = "";
 /* process image pixels which are drawn on the canvas.*/
  for (var y = 0; y < height; y++) {
    mapline += "t" + zeroFill(y, 4) + '="';
    for (var x = 0; x < width; x++) {
      var pixel = ctx.getImageData(x, y, 1, 1);
      mapline += get_map_terrain_type(pixel.data);
    }
    mapline += '"\n';
  }
  console.log(mapline)
  return mapline;

}


