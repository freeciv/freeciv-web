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
var renderer_3d = false;

L.mapbox.accessToken = 'pk.eyJ1IjoiYW5kcmVhc3Jvc2RhbCIsImEiOiJjaWo2d2piZnMwMDF0MHJsdjZ2NHBxdmhhIn0.36znzLi2ExS8gwXauDgt3w';
var map = L.mapbox.map('map', 'mapbox.satellite', {
    maxZoom: 15
})
    .setView([35, 25], 2);

document.getElementById('snap').addEventListener('click', function() {
    $("#snap_status").html("<b>Please wait while generating map for you!</b>");
    $("#snap").hide();
    $("#snap3d").hide();
    leafletImage(map, doImage);
});

document.getElementById('snap3d').addEventListener('click', function() {
    $("#snap_status").html("<b>Please wait while generating map for you!</b>");
    $("#snap").hide();
    $("#snap3d").hide();
    renderer_3d = true;
    leafletImage(map, doImage);
});

$("#geolocate").click(function() {
  map.locate();
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
    if (!renderer_3d) {
      window.location.href="/webclient/?action=earthload&savegame=" + responsedata;
    } else {
      window.location.href="/webclient/?action=earthload&savegame=" + responsedata + "&renderer=webgl";
    }
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

function get_map_terrain_type(x, y, pixel) 
{
  var lat_1 = map.getBounds().getNorthEast().lat;
  var lat_2 = map.getBounds().getSouthWest().lat;

  var near_equator = false;

  var dy = (90 - lat_1) / 180 + (y / height) * Math.abs(lat_1 - lat_2) / 180;
  // near_equator is used to create desert tiles near equator and tundra near
  // the poles, since these have similar colors.
  if (dy > 0.25 && dy < 0.75) near_equator = true; 

  var rnd = Math.random();
  if (pixel_color(255,255,255, pixel, 20)) return "a";  //arctic
  if (pixel_color(64, 124, 38, pixel, 40)) return "g"; //grassland
  if (pixel_color(84, 73, 38, pixel, 20) && !near_equator) return "t";   //tundra
  if (pixel_color(240, 220, 155, pixel, 100) && near_equator) return "d"; //desert
  if (pixel_color(44, 52, 28, pixel, 10)) return "f"; //forest
  if (pixel_color(37, 64, 27, pixel, 10)) return "g"; //jungle
  if (pixel_color(60, 89, 38, pixel, 30)) return "g"; //jungle
  if (pixel_color(204, 99, 40, pixel, 100) && near_equator) return "d"; //desert
  if (pixel_color(67, 72, 31, pixel, 10)) return "m"; //mountains
  if (pixel_color(61, 79, 31, pixel, 10)) return "h"; //hills
  if (pixel_color(4, 10, 20, pixel, 15)) return ":";  //ocean
  if (pixel_color(8, 20, 36, pixel, 15)) return " ";  //coast
  if (pixel_color(39, 57, 61, pixel, 35)) return " ";  //coast

  if (rnd < 4) return "p"; //plains
  if (rnd >= 4 && rnd <= 6) return "h";  //hills
  if (rnd >= 6 && rnd < 7) return "t";  //tundra 
  if (rnd >= 7) return "g";  //grassland 

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
      mapline += get_map_terrain_type(x, y, pixel.data);
    }
    mapline += '"\n';
  }
  console.log(mapline)
  return mapline;

}


// Once we've got a position, zoom and center the map
// on it, and add a single marker.
map.on('locationfound', function(e) {
    map.fitBounds(e.bounds);
});

