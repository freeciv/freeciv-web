/********************************************************************** 
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "log.h"
#include "back_end.h"
#include "colors.h"
#include "gui_main.h"		/* for enum_color_to_be_color() */
#include "widget.h"
#include "mapview.h"		/* for text_templates */

#include "canvas.h"

/**************************************************************************
  ...
**************************************************************************/
struct canvas *canvas_create(int width, int height)
{
  struct canvas *result = fc_malloc(sizeof(*result));

  result->osda = be_create_osda(width, height);
  result->widget=NULL;
  return result;
}

/**************************************************************************
  ...
**************************************************************************/
void canvas_free(struct canvas *store)
{
  be_free_osda(store->osda);
  assert(store->widget == NULL);
  free(store);
}

/**************************************************************************
  ...
**************************************************************************/
void canvas_copy(struct canvas *dest, struct canvas *src,
		 int src_x, int src_y, int dest_x, int dest_y, int width,
		 int height)
{
  struct ct_size size = { width, height };
  struct ct_point src_pos = { src_x, src_y };
  struct ct_point dest_pos = { dest_x, dest_y };
  struct ct_rect dest_rect = {dest_x, dest_y, width, height};

  freelog(LOG_DEBUG, "canvas_copy(src=%p,dest=%p)",src,dest);

  be_copy_osda_to_osda(dest->osda, src->osda, &size, &dest_pos, &src_pos);
  if (dest->widget) {
    sw_window_canvas_background_region_needs_repaint(dest->widget, &dest_rect);
  }
}

/**************************************************************************
  Draw some or all of a sprite onto the mapview or citydialog canvas.
**************************************************************************/
void canvas_put_sprite(struct canvas *pcanvas,
		       int canvas_x, int canvas_y,
		       struct sprite *sprite,
		       int offset_x, int offset_y, int width, int height)
{
  struct osda *osda = pcanvas->osda;
  struct ct_point dest_pos = { canvas_x, canvas_y };
  struct ct_point src_pos = { offset_x, offset_y };
  struct ct_size size = { width, height };
  struct ct_rect dest_rect = {canvas_x, canvas_y, width, height};

  freelog(LOG_DEBUG, "gui_put_sprite canvas=%p",pcanvas);
  be_draw_sprite(osda, sprite, &size, &dest_pos, &src_pos);
  if (pcanvas->widget) {
    sw_window_canvas_background_region_needs_repaint(pcanvas->widget, &dest_rect);
  }
}

/**************************************************************************
  Draw a full sprite onto the mapview or citydialog canvas.
**************************************************************************/
void canvas_put_sprite_full(struct canvas *pcanvas,
			    int canvas_x, int canvas_y,
			    struct sprite *sprite)
{
  struct ct_size size;

  freelog(LOG_DEBUG, "gui_put_sprite_full");
  be_sprite_get_size(&size, sprite);
  canvas_put_sprite(pcanvas, canvas_x, canvas_y,
		    sprite, 0, 0, size.width, size.height);
}

/****************************************************************************
  Draw a full sprite onto the canvas.  If "fog" is specified draw it with
  fog.
****************************************************************************/
void canvas_put_sprite_fogged(struct canvas *pcanvas,
                              int canvas_x, int canvas_y,
                              struct sprite *psprite,
                              bool fog, int fog_x, int fog_y)
{
  /* PORTME */
}

/**************************************************************************
  Draw a filled-in colored rectangle onto the mapview or citydialog canvas.
**************************************************************************/
void canvas_put_rectangle(struct canvas *pcanvas,
			  struct color *pcolor,
			  int canvas_x, int canvas_y, int width, int height)
{
  struct ct_rect rect = { canvas_x, canvas_y, width, height };
  
  freelog(LOG_DEBUG, "gui_put_rectangle(,%lu,%d,%d,%d,%d)", pcolor->color,
          canvas_x, canvas_y, width, height);
  
  be_draw_region(pcanvas->osda, &rect, pcolor->color);
  if (pcanvas->widget) {
    sw_window_canvas_background_region_needs_repaint(pcanvas->widget,
						     &rect);
  }
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fill_sprite_area(struct canvas *pcanvas,
                             struct sprite *psprite, struct color *pcolor,
                             int canvas_x, int canvas_y)
{
  /* PORTME */
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fog_sprite_area(struct canvas *pcanvas, struct sprite *psprite,
                            int canvas_x, int canvas_y)
{
  /* PORTME */
}

/**************************************************************************
  Draw a 1-pixel-width colored line onto the mapview or citydialog canvas.
**************************************************************************/
void canvas_put_line(struct canvas *pcanvas, struct color *pcolor,
		     enum line_type ltype, int start_x, int start_y,
		     int dx, int dy)
{
  struct ct_point start = { start_x, start_y };
  struct ct_point end = { start_x + dx, start_y + dy};
  struct ct_rect rect;

  ct_rect_fill_on_2_points(&rect,&start,&end);

  freelog(LOG_DEBUG, "gui_put_line(...)");

  if (ltype == LINE_NORMAL) {
    be_draw_line(pcanvas->osda, &start, &end, 1, FALSE, pcolor->color);
  } else if (ltype == LINE_BORDER) {
    be_draw_line(pcanvas->osda, &start, &end, 2, TRUE, pcolor->color);
  } else {
    assert(0);
  }
      
  if (pcanvas->widget) {
    sw_window_canvas_background_region_needs_repaint(pcanvas->widget,
						     &rect);
  }
}

/**************************************************************************
  Draw a 1-pixel-width curved colored line onto the mapview or 
  citydialog canvas.
**************************************************************************/
void canvas_put_curved_line(struct canvas *pcanvas, struct color *pcolor,
                            enum line_type ltype, int start_x, int start_y,
                            int dx, int dy)
{
  /* FIXME: Implement curved line drawing. */
  canvas_put_line(pcanvas, pcolor, ltype, start_x, start_y, dx, dy);
}

/****************************************************************************
  Return the size of the given text in the given font.  This size should
  include the ascent and descent of the text.  Either of width or height
  may be NULL in which case those values simply shouldn't be filled out.
****************************************************************************/
void get_text_size(int *width, int *height,
		   enum client_font font, const char *text)
{
  struct ct_size size;
  struct ct_string *string = ct_string_clone3(text_templates[font], text);

  be_string_get_size(&size, string);

  if (width) {
    *width = size.width;
  }
  if (height) {
    *height = size.height;
  }
}

/****************************************************************************
  Draw the text onto the canvas in the given color and font.  The canvas
  position does not account for the ascent of the text; this function must
  take care of this manually.  The text will not be NULL but may be empty.
****************************************************************************/
void canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
		     enum client_font font, struct color *pcolor,
		     const char *text)
{
    struct ct_point position={canvas_x, canvas_y};
    struct ct_string *string=ct_string_clone4(text_templates[font], text,
                                                              pcolor->color);
    tr_draw_string(pcanvas->osda,&position,string);
    ct_string_destroy(string);
}
