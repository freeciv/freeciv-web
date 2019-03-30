/***********************************************************************
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
#include <fc_config.h>
#endif

/* gui-gtk-3.22 */
#include "colors.h"
#include "gui_main.h"
#include "mapview.h"

#include "canvas.h"

/************************************************************************//**
  Create a canvas of the given size.
****************************************************************************/
struct canvas *canvas_create(int width, int height)
{
  struct canvas *result = fc_malloc(sizeof(*result));

  result->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               width, height);
  result->drawable = NULL;
  result->zoom = 1.0;

  return result;
}

/************************************************************************//**
  Free any resources associated with this canvas and the canvas struct
  itself.
****************************************************************************/
void canvas_free(struct canvas *store)
{
  cairo_surface_destroy(store->surface);
  free(store);
}

/************************************************************************//**
  Set canvas zoom for future drawing operations.
****************************************************************************/
void canvas_set_zoom(struct canvas *store, float zoom)
{
  store->zoom = zoom;
}

/************************************************************************//**
  This gui has zoom support.
****************************************************************************/
bool has_zoom_support(void)
{
  return TRUE;
}

/************************************************************************//**
  Copies an area from the source canvas to the destination canvas.
****************************************************************************/
void canvas_copy(struct canvas *dest, struct canvas *src,
		 int src_x, int src_y, int dest_x, int dest_y,
		 int width, int height)
{
  cairo_t *cr;

  if (!dest->drawable) {
    cr = cairo_create(dest->surface);
  } else {
    cr = dest->drawable;
  }

  if (dest->drawable) {
    cairo_save(cr);
  }

  cairo_scale(cr, dest->zoom / src->zoom, dest->zoom / src->zoom);
  cairo_set_source_surface(cr, src->surface, dest_x - src_x, dest_y - src_y);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  cairo_rectangle(cr, dest_x, dest_y, width, height);
  cairo_fill(cr);

  if (!dest->drawable) {
    cairo_destroy(cr);
  } else {
    cairo_restore(cr);
  }
}

/************************************************************************//**
  Draw some or all of a sprite onto the mapview or citydialog canvas.
  Supplied coordinates are prior to any canvas zoom.
****************************************************************************/
void canvas_put_sprite(struct canvas *pcanvas,
		       int canvas_x, int canvas_y,
		       struct sprite *sprite,
		       int offset_x, int offset_y, int width, int height)
{
  int sswidth, ssheight;
  cairo_t *cr;

  get_sprite_dimensions(sprite, &sswidth, &ssheight);

  if (!pcanvas->drawable) {
    cr = cairo_create(pcanvas->surface);
  } else {
    cr = pcanvas->drawable;
  }

  if (pcanvas->drawable) {
    cairo_save(cr);
  }

  cairo_scale(cr, pcanvas->zoom, pcanvas->zoom);
  cairo_set_source_surface(cr, sprite->surface, canvas_x - offset_x, canvas_y - offset_y);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  cairo_rectangle(cr, canvas_x - offset_x, canvas_y - offset_y,
                  MIN(width, MAX(0, sswidth - offset_x)),
                  MIN(height, MAX(0, ssheight - offset_y)));
  cairo_fill(cr);

  if (!pcanvas->drawable) {
    cairo_destroy(cr);
  } else {
    cairo_restore(cr);
  }
}

/************************************************************************//**
  Draw a full sprite onto the mapview or citydialog canvas.
  Supplied canvas_x/y are prior to any canvas zoom.
****************************************************************************/
void canvas_put_sprite_full(struct canvas *pcanvas,
			    int canvas_x, int canvas_y,
			    struct sprite *sprite)
{
  int width, height;

  get_sprite_dimensions(sprite, &width, &height);
  canvas_put_sprite(pcanvas, canvas_x, canvas_y, sprite,
		    0, 0, width, height);
}

/************************************************************************//**
  Draw a full sprite onto the canvas.  If "fog" is specified draw it with
  fog.
****************************************************************************/
void canvas_put_sprite_fogged(struct canvas *pcanvas,
			      int canvas_x, int canvas_y,
			      struct sprite *psprite,
			      bool fog, int fog_x, int fog_y)
{
    pixmap_put_overlay_tile_draw(pcanvas, canvas_x, canvas_y,
				 psprite, fog);
}

/************************************************************************//**
  Draw a filled-in colored rectangle onto the mapview or citydialog canvas.
  Supplied coordinates are prior to any canvas zoom.
****************************************************************************/
void canvas_put_rectangle(struct canvas *pcanvas,
			  struct color *pcolor,
			  int canvas_x, int canvas_y, int width, int height)
{
  cairo_t *cr;

  if (!pcanvas->drawable) {
    cr = cairo_create(pcanvas->surface);
  } else {
    cr = pcanvas->drawable;
  }

  if (pcanvas->drawable) {
    cairo_save(cr);
  }

  cairo_scale(cr, pcanvas->zoom, pcanvas->zoom);
  gdk_cairo_set_source_rgba(cr, &pcolor->color);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  cairo_rectangle(cr, canvas_x, canvas_y, width, height);
  cairo_fill(cr);

  if (!pcanvas->drawable) {
    cairo_destroy(cr);
  } else {
    cairo_restore(cr);
  }
}

/************************************************************************//**
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fill_sprite_area(struct canvas *pcanvas,
			     struct sprite *psprite,
			     struct color *pcolor,
			     int canvas_x, int canvas_y)
{
  int width, height;
  get_sprite_dimensions(psprite, &width, &height);
  canvas_put_rectangle(pcanvas, pcolor, canvas_x, canvas_y, width, height);
}

/************************************************************************//**
  Draw a colored line onto the mapview or citydialog canvas.
  XXX: unlike other canvas_put functions, supplied x/y are *not* prior to
  any canvas zoom.
****************************************************************************/
void canvas_put_line(struct canvas *pcanvas,
		     struct color *pcolor,
		     enum line_type ltype, int start_x, int start_y,
		     int dx, int dy)
{
  cairo_t *cr;
  double dashes[2] = {4.0, 4.0};

  if (!pcanvas->drawable) {
    cr = cairo_create(pcanvas->surface);
  } else {
    cr = pcanvas->drawable;
  }

  if (pcanvas->drawable) {
    cairo_save(cr);
  }

  switch (ltype) {
  case LINE_NORMAL:
    cairo_set_line_width(cr, 1.);
    break;
  case LINE_BORDER:
    cairo_set_line_width(cr, (double)BORDER_WIDTH);
    cairo_set_dash(cr, dashes, 2, 0);
    break;
  case LINE_TILE_FRAME:
    cairo_set_line_width(cr, 2.);
    break;
  case LINE_GOTO:
    cairo_set_line_width(cr, 2.);
    break;
  }

  gdk_cairo_set_source_rgba(cr, &pcolor->color);
  cairo_move_to(cr, start_x, start_y);
  cairo_line_to(cr, start_x + dx, start_y + dy);
  cairo_stroke(cr);

  if (!pcanvas->drawable) {
    cairo_destroy(cr);
  } else {
    cairo_restore(cr);
  }
}

/************************************************************************//**
  Draw a colored curved line for the Technology Tree connectors
  A curved line is: 1 horizontal line, 2 arcs, 1 horizontal line
****************************************************************************/
void canvas_put_curved_line(struct canvas *pcanvas,
                            struct color *pcolor,
                            enum line_type ltype, int start_x, int start_y,
                            int dx, int dy)
{
  int end_x = start_x + dx;
  int end_y = start_y + dy;
  cairo_t *cr;
  double dashes[2] = {4.0, 4.0};

  if (!pcanvas->drawable) {
    cr = cairo_create(pcanvas->surface);
  } else {
    cr = pcanvas->drawable;
  }

  if (pcanvas->drawable) {
    cairo_save(cr);
  }

  switch (ltype) {
  case LINE_NORMAL:
    cairo_set_line_width(cr, 1.);
    break;
  case LINE_BORDER:
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_set_line_width(cr, (double)BORDER_WIDTH);
    break;
  case LINE_TILE_FRAME:
    cairo_set_line_width(cr, 2.);
    break;
  case LINE_GOTO:
    cairo_set_line_width(cr, 2.);
    break;
  }

  gdk_cairo_set_source_rgba(cr, &pcolor->color);
  cairo_move_to(cr, start_x, start_y);
  cairo_curve_to(cr, end_x, start_y, start_x, end_y, end_x, end_y);
  cairo_stroke(cr);

  if (!pcanvas->drawable) {
    cairo_destroy(cr);
  } else {
    cairo_restore(cr);
  }
}

static PangoLayout *layout;
static struct {
  PangoFontDescription **styles;
  bool shadowed;
} fonts[FONT_COUNT] = {
  {&city_names_style, TRUE},
  {&city_productions_style, TRUE},
  {&reqtree_text_style, FALSE}
};
#define FONT(font) (*fonts[font].styles)

/************************************************************************//**
  Return the size of the given text in the given font.  This size should
  include the ascent and descent of the text.  Either of width or height
  may be NULL in which case those values simply shouldn't be filled out.
****************************************************************************/
void get_text_size(int *width, int *height,
		   enum client_font font, const char *text)
{
  PangoRectangle rect;

  if (!layout) {
    layout = pango_layout_new(gdk_pango_context_get_for_screen(gdk_screen_get_default()));
  }

  pango_layout_set_font_description(layout, FONT(font));
  pango_layout_set_text(layout, text, -1);

  pango_layout_get_pixel_extents(layout, NULL, &rect);
  if (width) {
    *width = rect.width;
  }
  if (height) {
    *height = rect.height;
  }
}

/************************************************************************//**
  Draw the text onto the canvas in the given color and font.  The canvas
  position does not account for the ascent of the text; this function must
  take care of this manually.  The text will not be NULL but may be empty.
  Supplied canvas_x/y are prior to any cavas zoom.
****************************************************************************/
void canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
		     enum client_font font,
		     struct color *pcolor,
		     const char *text)
{
  cairo_t *cr;

  if (!layout) {
    layout = pango_layout_new(gdk_pango_context_get_for_screen(gdk_screen_get_default()));
  }

  if (!pcanvas->drawable) {
    cr = cairo_create(pcanvas->surface);
  } else {
    cr = pcanvas->drawable;
  }

  if (pcanvas->drawable) {
    cairo_save(cr);
  }

  pango_layout_set_font_description(layout, FONT(font));
  pango_layout_set_text(layout, text, -1);

  if (fonts[font].shadowed) {
    /* Suppress drop shadow for black text */
    const GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };

    if (!gdk_rgba_equal(&pcolor->color, &black)) {
      gdk_cairo_set_source_rgba(cr, &black);
      cairo_move_to(cr, canvas_x * pcanvas->zoom + 1,
                    canvas_y * pcanvas->zoom + 1);
      pango_cairo_show_layout (cr, layout);
    }
  }

  cairo_move_to(cr, canvas_x * pcanvas->zoom, canvas_y * pcanvas->zoom);
  gdk_cairo_set_source_rgba(cr, &pcolor->color);
  pango_cairo_show_layout(cr, layout);

  if (!pcanvas->drawable) {
    cairo_destroy(cr);
  } else {
    cairo_restore(cr);
  }
}
