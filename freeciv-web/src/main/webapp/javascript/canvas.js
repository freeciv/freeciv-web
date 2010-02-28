/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


/****************************************************************************
  Draw a filled-in colored rectangle onto the mapview or citydialog canvas.
****************************************************************************/
function canvas_put_rectangle(canvas_context, pcolor, canvas_x, canvas_y, width, height)
{
 if (is_canvas_supported()) {
   canvas_context.fillStyle = pcolor;
   canvas_context.fillRect (canvas_x, canvas_y, canvas_x + width, canvas_y + height);
 }

}


/****************************************************************************
  Draw some or all of a sprite onto the mapview or citydialog canvas.
****************************************************************************/
function canvas_put_sprite(pcanvas, canvas_x, canvas_y, sprite, offset_x, offset_y)
{
  if (is_canvas_supported()) {
    mapview_put_tile(pcanvas, sprite['key'], canvas_x + offset_x, canvas_y + offset_y);
  } else {
    mapview_put_tile_ie(pcanvas, sprite['key'], canvas_x + offset_x, canvas_y + offset_y);
  }
}

/****************************************************************************
  Determines if the browser supports the HTML5 canvas element.
****************************************************************************/
function is_canvas_supported() {
  //FIXME:
  //return false;
  
  return (window.CanvasRenderingContext2D);
}

/****************************************************************************
  Determines if the browser supports image clipping with Canvas.
****************************************************************************/
function is_canvas_clipping_supported() {
  return !jQuery.browser.opera;
}