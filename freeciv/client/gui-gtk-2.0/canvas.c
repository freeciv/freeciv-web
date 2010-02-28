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

#include "canvas.h"
#include "colors.h"
#include "gui_main.h"
#include "mapview.h"

/****************************************************************************
  Create a canvas of the given size.
****************************************************************************/
struct canvas *canvas_create(int width, int height)
{
  struct canvas *result = fc_malloc(sizeof(*result));

  result->type = CANVAS_PIXMAP;
  result->v.pixmap = gdk_pixmap_new(root_window, width, height, -1);
  return result;
}

/****************************************************************************
  Free any resources associated with this canvas and the canvas struct
  itself.
****************************************************************************/
void canvas_free(struct canvas *store)
{
  if (store->type == CANVAS_PIXMAP) {
    g_object_unref(store->v.pixmap);
  }
  free(store);
}

/****************************************************************************
  Copies an area from the source canvas to the destination canvas.
****************************************************************************/
void canvas_copy(struct canvas *dest, struct canvas *src,
		 int src_x, int src_y, int dest_x, int dest_y,
		 int width, int height)
{
  if (dest->type == src->type) {
    if (src->type == CANVAS_PIXMAP) {
      gdk_draw_drawable(dest->v.pixmap, fill_bg_gc, src->v.pixmap,
			src_x, src_y, dest_x, dest_y, width, height);
    }
  }
}

/****************************************************************************
  Place part of a (possibly masked) sprite on a pixmap.
****************************************************************************/
static void pixmap_put_sprite(GdkDrawable *pixmap,
			      int pixmap_x, int pixmap_y,
			      struct sprite *ssprite,
			      int offset_x, int offset_y,
			      int width, int height)
{
  if (ssprite->pixmap) {
    if (ssprite->mask) {
      gdk_gc_set_clip_origin(civ_gc, pixmap_x, pixmap_y);
      gdk_gc_set_clip_mask(civ_gc, ssprite->mask);
    }

    gdk_draw_drawable(pixmap, civ_gc, ssprite->pixmap,
		      offset_x, offset_y,
		      pixmap_x + offset_x, pixmap_y + offset_y,
		      MIN(width, MAX(0, ssprite->width - offset_x)),
		      MIN(height, MAX(0, ssprite->height - offset_y)));

    gdk_gc_set_clip_mask(civ_gc, NULL);
  } else {
    gdk_draw_pixbuf(pixmap, civ_gc, ssprite->pixbuf,
		    offset_x, offset_y,
		    pixmap_x + offset_x, pixmap_y + offset_y,
		    MIN(width, MAX(0, ssprite->width - offset_x)),
		    MIN(height, MAX(0, ssprite->height - offset_y)),
		    GDK_RGB_DITHER_NONE, 0, 0);
  }
}

/****************************************************************************
  Draw some or all of a sprite onto the mapview or citydialog canvas.
****************************************************************************/
void canvas_put_sprite(struct canvas *pcanvas,
		       int canvas_x, int canvas_y,
		       struct sprite *sprite,
		       int offset_x, int offset_y, int width, int height)
{
  switch (pcanvas->type) {
    case CANVAS_PIXMAP:
      pixmap_put_sprite(pcanvas->v.pixmap, canvas_x, canvas_y,
	  sprite, offset_x, offset_y, width, height);
      break;
    case CANVAS_PIXCOMM:
      gtk_pixcomm_copyto(pcanvas->v.pixcomm, sprite, canvas_x, canvas_y);
      break;
    case CANVAS_PIXBUF:
      {
	GdkPixbuf *src, *dst;

	/* FIXME: is this right??? */
	if (canvas_x < 0) {
	  offset_x -= canvas_x;
	  canvas_x = 0;
	}
	if (canvas_y < 0) {
	  offset_y -= canvas_y;
	  canvas_y = 0;
	}


	src = sprite_get_pixbuf(sprite);
	dst = pcanvas->v.pixbuf;
	gdk_pixbuf_composite(src, dst, canvas_x, canvas_y,
	    MIN(width,
	      MIN(gdk_pixbuf_get_width(dst), gdk_pixbuf_get_width(src))),
	    MIN(height,
	      MIN(gdk_pixbuf_get_height(dst), gdk_pixbuf_get_height(src))),
	    canvas_x - offset_x, canvas_y - offset_y,
	    1.0, 1.0, GDK_INTERP_NEAREST, 255);
      }
      break;
    default:
      break;
  } 
}

/****************************************************************************
  Draw a full sprite onto the mapview or citydialog canvas.
****************************************************************************/
void canvas_put_sprite_full(struct canvas *pcanvas,
			    int canvas_x, int canvas_y,
			    struct sprite *sprite)
{
  canvas_put_sprite(pcanvas, canvas_x, canvas_y, sprite,
		    0, 0, sprite->width, sprite->height);
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
  if (pcanvas->type == CANVAS_PIXMAP) {
    pixmap_put_overlay_tile_draw(pcanvas->v.pixmap, canvas_x, canvas_y,
				 psprite, fog);
  }
}

/****************************************************************************
  Draw a filled-in colored rectangle onto the mapview or citydialog canvas.
****************************************************************************/
void canvas_put_rectangle(struct canvas *pcanvas,
			  struct color *pcolor,
			  int canvas_x, int canvas_y, int width, int height)
{
  GdkColor *col = &pcolor->color;

  switch (pcanvas->type) {
    case CANVAS_PIXMAP:
      gdk_gc_set_foreground(fill_bg_gc, col);
      gdk_draw_rectangle(pcanvas->v.pixmap, fill_bg_gc, TRUE,
	  canvas_x, canvas_y, width, height);
      break;
    case CANVAS_PIXCOMM:
      gtk_pixcomm_fill(pcanvas->v.pixcomm, col);
      break;
    case CANVAS_PIXBUF:
      gdk_pixbuf_fill(pcanvas->v.pixbuf,
	  ((guint32)(col->red & 0xff00) << 16)
	  | ((col->green & 0xff00) << 8) | (col->blue & 0xff00) | 0xff);
      break;
    default:
      break;
  }
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fill_sprite_area(struct canvas *pcanvas,
			     struct sprite *psprite,
			     struct color *pcolor,
			     int canvas_x, int canvas_y)
{
  if (pcanvas->type == CANVAS_PIXMAP) {
    gdk_gc_set_clip_origin(fill_bg_gc, canvas_x, canvas_y);
    gdk_gc_set_clip_mask(fill_bg_gc, sprite_get_mask(psprite));
    gdk_gc_set_foreground(fill_bg_gc, &pcolor->color);

    gdk_draw_rectangle(pcanvas->v.pixmap, fill_bg_gc, TRUE,
		       canvas_x, canvas_y, psprite->width, psprite->height);

    gdk_gc_set_clip_mask(fill_bg_gc, NULL);
  }
}

/****************************************************************************
  Fill the area covered by the sprite with the given color.
****************************************************************************/
void canvas_fog_sprite_area(struct canvas *pcanvas, struct sprite *psprite,
			    int canvas_x, int canvas_y)
{
  if (pcanvas->type == CANVAS_PIXMAP) {
    gdk_gc_set_clip_origin(fill_tile_gc, canvas_x, canvas_y);
    gdk_gc_set_clip_mask(fill_tile_gc, sprite_get_mask(psprite));
    gdk_gc_set_foreground(fill_tile_gc,
			  &get_color(tileset, COLOR_MAPVIEW_UNKNOWN)->color);
    gdk_gc_set_stipple(fill_tile_gc, black50);
    gdk_gc_set_ts_origin(fill_tile_gc, canvas_x, canvas_y);

    gdk_draw_rectangle(pcanvas->v.pixmap, fill_tile_gc, TRUE,
		       canvas_x, canvas_y, psprite->width, psprite->height);

    gdk_gc_set_clip_mask(fill_tile_gc, NULL); 
  }
}

/****************************************************************************
  Draw a colored line onto the mapview or citydialog canvas.
****************************************************************************/
void canvas_put_line(struct canvas *pcanvas,
		     struct color *pcolor,
		     enum line_type ltype, int start_x, int start_y,
		     int dx, int dy)
{
  if (pcanvas->type == CANVAS_PIXMAP) {
    GdkGC *gc = NULL;

    switch (ltype) {
    case LINE_NORMAL:
      gc = thin_line_gc;
      break;
    case LINE_BORDER:
      gc = border_line_gc;
      break;
    case LINE_TILE_FRAME:
      gc = thick_line_gc;
      break;
    case LINE_GOTO:
      gc = thick_line_gc;
      break;
    }

    gdk_gc_set_foreground(gc, &pcolor->color);
    gdk_draw_line(pcanvas->v.pixmap, gc,
		  start_x, start_y, start_x + dx, start_y + dy);
  }
}

/****************************************************************************
  Draw a colored curved line for the Technology Tree connectors
  A curved line is: 1 horizontal line, 2 arcs, 1 horizontal line
****************************************************************************/
void canvas_put_curved_line(struct canvas *pcanvas,
                            struct color *pcolor,
                            enum line_type ltype, int start_x, int start_y,
                            int dx, int dy)
{
  if (pcanvas->type == CANVAS_PIXMAP) {
    GdkGC *gc = NULL;
    
    switch (ltype) {
    case LINE_NORMAL:
      gc = thin_line_gc;
      break;
    case LINE_BORDER:
      gc = border_line_gc;
      break;
    case LINE_TILE_FRAME:
      gc = thick_line_gc;
      break;
    case LINE_GOTO:
      gc = thick_line_gc;
      break;
    }

    gdk_gc_set_foreground(gc, &pcolor->color);

    /* To begin, work out the endpoints of the curve */
    /* and initial horizontal stroke */
    int line1end_x = start_x + 10;
    int end_x = start_x + dx;
    int end_y = start_y + dy;
    
    int arcwidth = dx - 10;
    /* draw a short horizontal line */
    gdk_draw_line(pcanvas->v.pixmap, gc,
                  start_x, start_y, line1end_x, start_y);

    if (end_y < start_y) {
      /* if end_y is above start_y then the first curve is rising */
      int archeight = -dy + 1;

      /* position the BB of the arc */
      int arcstart_x = line1end_x-arcwidth/2 -1;
      int arcstart_y = end_y-1;

      /* Draw arc curving up */
      gdk_draw_arc(pcanvas->v.pixmap, gc, FALSE,
                   arcstart_x, arcstart_y, 
                   arcwidth, archeight, 0, -90*64);

      /* Shift BB to the right */
      arcstart_x = arcstart_x + arcwidth;

      /* draw arc curving across */
      gdk_draw_arc(pcanvas->v.pixmap, gc, FALSE,
                   arcstart_x, arcstart_y, 
                   arcwidth, archeight, 180*64, -90*64);

    } else { /* end_y is below start_y */

      int archeight = dy + 1;

      /* position BB */
      int arcstart_x = line1end_x - arcwidth/2 -1;
      int arcstart_y = start_y-1;

      /* Draw arc curving down */
      gdk_draw_arc(pcanvas->v.pixmap, gc, FALSE,
                   arcstart_x, arcstart_y, 
                   arcwidth, archeight, 0, 90*64);

      /* shift BB right */
      arcstart_x = arcstart_x + arcwidth;

      /* Draw arc curving across */
      gdk_draw_arc(pcanvas->v.pixmap, gc, FALSE,
                   arcstart_x, arcstart_y, 
                   arcwidth, archeight, 270*64, -90*64);

    }
    /* Draw short horizontal line */
    gdk_draw_line(pcanvas->v.pixmap, gc,
                  end_x - 10, start_y + dy, end_x, end_y);
  }
}

static PangoLayout *layout;
static struct {
  PangoFontDescription **font;
  bool shadowed;
} fonts[FONT_COUNT] = {
  {&main_font, TRUE},
  {&city_productions_font, TRUE},
  {&city_productions_font, FALSE} /* FIXME: should use separate font */
};

/****************************************************************************
  Return the size of the given text in the given font.  This size should
  include the ascent and descent of the text.  Either of width or height
  may be NULL in which case those values simply shouldn't be filled out.
****************************************************************************/
void get_text_size(int *width, int *height,
		   enum client_font font, const char *text)
{
  PangoRectangle rect;

  if (!layout) {
    layout = pango_layout_new(gdk_pango_context_get());
  }

  pango_layout_set_font_description(layout, *fonts[font].font);
  pango_layout_set_text(layout, text, -1);

  pango_layout_get_pixel_extents(layout, &rect, NULL);
  if (width) {
    *width = rect.width;
  }
  if (height) {
    *height = rect.height;
  }
}

/****************************************************************************
  Draw the text onto the canvas in the given color and font.  The canvas
  position does not account for the ascent of the text; this function must
  take care of this manually.  The text will not be NULL but may be empty.
****************************************************************************/
void canvas_put_text(struct canvas *pcanvas, int canvas_x, int canvas_y,
		     enum client_font font,
		     struct color *pcolor,
		     const char *text)
{
  PangoRectangle rect;

  if (pcanvas->type != CANVAS_PIXMAP) {
    return;
  }
  if (!layout) {
    layout = pango_layout_new(gdk_pango_context_get());
  }

  gdk_gc_set_foreground(civ_gc, &pcolor->color);
  pango_layout_set_font_description(layout, *fonts[font].font);
  pango_layout_set_text(layout, text, -1);

  pango_layout_get_pixel_extents(layout, &rect, NULL);
  if (fonts[font].shadowed) {
    gtk_draw_shadowed_string(pcanvas->v.pixmap,
			     toplevel->style->black_gc, civ_gc,
			     canvas_x,
			     canvas_y + PANGO_ASCENT(rect), layout);
  } else {
    gdk_draw_layout(pcanvas->v.pixmap, civ_gc,
		    canvas_x, canvas_y + PANGO_ASCENT(rect),
		    layout);
  }
}
