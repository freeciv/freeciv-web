/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
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

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include <png.h>

/* common & utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "movement.h"
#include "shared.h"
#include "support.h"
#include "unit.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "colors.h"
#include "gui_main.h"
#include "mapview_g.h"
#include "options.h"
#include "tilespec.h"

#include "graphics.h"

struct sprite *intro_gfx_sprite;
struct sprite *radar_gfx_sprite;

Cursor cursors[CURSOR_LAST];

static struct sprite *ctor_sprite(Pixmap mypixmap, int width, int height);
static struct sprite *ctor_sprite_mask(Pixmap mypixmap, Pixmap mask, 
 				       int width, int height);

/***************************************************************************
...
***************************************************************************/
bool isometric_view_supported(void)
{
  return TRUE;
}

/***************************************************************************
...
***************************************************************************/
bool overhead_view_supported(void)
{
  return TRUE;
}

/***************************************************************************
...
***************************************************************************/
#define COLOR_MOTTO_FACE    "#2D71E3"

void load_intro_gfx(void)
{
  int tot, lin, y, w;
  char s[64];
  XColor face;
  int have_face;
  const char *motto = freeciv_motto();
  XFontSetExtents *exts;

  /* metrics */

  exts = XExtentsOfFontSet(main_font_set);
  lin = exts->max_logical_extent.height;

  /* get colors */

  if(XParseColor(display, cmap, COLOR_MOTTO_FACE, &face) &&
     XAllocColor(display, cmap, &face)) {
    have_face = TRUE;
  } else {
    face.pixel = get_color(tileset, COLOR_OVERVIEW_VIEWRECT)->color.pixel;
    have_face = FALSE;
  }

  /* Main graphic */

  intro_gfx_sprite = load_gfxfile(tileset_main_intro_filename(tileset));
  tot = intro_gfx_sprite->width;

  y = intro_gfx_sprite->height - (2 * lin);

  w = XmbTextEscapement(main_font_set, motto, strlen(motto));
  XSetForeground(display, font_gc, face.pixel);
  XmbDrawString(display, intro_gfx_sprite->pixmap,
      		main_font_set, font_gc,
		tot / 2 - w / 2, y,
		motto, strlen(motto));

  /* Minimap graphic */

  radar_gfx_sprite = load_gfxfile(tileset_mini_intro_filename(tileset));
  tot = radar_gfx_sprite->width;

  y = radar_gfx_sprite->height - (lin +
      1.5 * (exts->max_logical_extent.height + exts->max_logical_extent.y));

  w = XmbTextEscapement(main_font_set, word_version(), strlen(word_version()));
  XSetForeground(display, font_gc,
		 get_color(tileset, COLOR_OVERVIEW_UNKNOWN)->color.pixel);
  XmbDrawString(display, radar_gfx_sprite->pixmap,
		main_font_set, font_gc,
		(tot / 2 - w / 2) + 1, y + 1,
		word_version(), strlen(word_version()));
  XSetForeground(display, font_gc,
		 get_color(tileset, COLOR_OVERVIEW_VIEWRECT)->color.pixel);
  XmbDrawString(display, radar_gfx_sprite->pixmap,
		main_font_set, font_gc,
		tot / 2 - w / 2, y,
		word_version(), strlen(word_version()));

  y += lin;

  my_snprintf(s, sizeof(s), "%d.%d.%d%s",
	      MAJOR_VERSION, MINOR_VERSION,
	      PATCH_VERSION, VERSION_LABEL);
  w = XmbTextEscapement(main_font_set, s, strlen(s));
  XSetForeground(display, font_gc,
		 get_color(tileset, COLOR_OVERVIEW_UNKNOWN)->color.pixel);
  XmbDrawString(display, radar_gfx_sprite->pixmap,
		main_font_set, font_gc,
		(tot / 2 - w / 2) + 1, y + 1, s, strlen(s));
  XSetForeground(display, font_gc,
		 get_color(tileset, COLOR_OVERVIEW_VIEWRECT)->color.pixel);
  XmbDrawString(display, radar_gfx_sprite->pixmap,
		main_font_set, font_gc,
		tot / 2 - w / 2, y, s, strlen(s));

  /* free colors */

  if (have_face) {
    XFreeColors(display, cmap, &(face.pixel), 1, 0);
  }

  /* done */

  return;
}

/****************************************************************************
  Create a new sprite by cropping and taking only the given portion of
  the image.
****************************************************************************/
struct sprite *crop_sprite(struct sprite *source,
			   int x, int y, int width, int height,
			   struct sprite *mask,
			   int mask_offset_x, int mask_offset_y)
{
  Pixmap mypixmap, mymask;
  GC plane_gc;

  mypixmap = XCreatePixmap(display, root_window,
			   width, height, display_depth);
  XCopyArea(display, source->pixmap, mypixmap, civ_gc, 
	    x, y, width, height, 0, 0);

  if (source->has_mask) {
    mymask = XCreatePixmap(display, root_window, width, height, 1);

    plane_gc = XCreateGC(display, mymask, 0, NULL);
    XCopyArea(display, source->mask, mymask, plane_gc, 
	      x, y, width, height, 0, 0);
    XFreeGC(display, plane_gc);

    if (mask) {
      XGCValues values;

      values.function = GXand;

      plane_gc = XCreateGC(display, mymask, GCFunction, &values);
      XCopyArea(display, mask->mask, mymask, plane_gc,
		x - mask_offset_x, y - mask_offset_y, width, height, 0, 0);
      XFreeGC(display, plane_gc);
    }

    return ctor_sprite_mask(mypixmap, mymask, width, height);
  } else if (mask) {
    mymask = XCreatePixmap(display, root_window, width, height, 1);

    plane_gc = XCreateGC(display, mymask, 0, NULL);
    XCopyArea(display, source->mask, mymask, plane_gc, 
	      x, y, width, height, 0, 0);
    XFreeGC(display, plane_gc);
    return ctor_sprite_mask(mypixmap, mymask, width, height);
  } else {
    return ctor_sprite(mypixmap, width, height);
  }
}

/****************************************************************************
  Find the dimensions of the sprite.
****************************************************************************/
void get_sprite_dimensions(struct sprite *sprite, int *width, int *height)
{
  *width = sprite->width;
  *height = sprite->height;
}

/***************************************************************************
...
***************************************************************************/
void load_cursors(void)
{
  enum cursor_type cursor;
  XColor white, black;
  struct sprite *sprite;
  int hot_x, hot_y;

  white.pixel = get_color(tileset, COLOR_OVERVIEW_VIEWRECT)->color.pixel;
  black.pixel = get_color(tileset, COLOR_OVERVIEW_UNKNOWN)->color.pixel;
  XQueryColor(display, cmap, &white);
  XQueryColor(display, cmap, &black);

  for (cursor = 0; cursor < CURSOR_LAST; cursor++) {
    sprite = get_cursor_sprite(tileset, cursor, &hot_x, &hot_y, 0);

    /* FIXME: this is entirely wrong.  It should be rewritten using
     * XcursorImageLoadCursor.  See gdkcursor-x11.c in the GTK sources for
     * examples. */
    cursors[cursor] = XCreatePixmapCursor(display,
					  sprite->mask, sprite->mask,
					  &white, &black, hot_x, hot_y);
  }
}

/***************************************************************************
...
***************************************************************************/
static struct sprite *ctor_sprite(Pixmap mypixmap, int width, int height)
{
  struct sprite *mysprite=fc_malloc(sizeof(struct sprite));
  mysprite->pixmap=mypixmap;
  mysprite->width=width;
  mysprite->height=height;
  mysprite->pcolorarray = NULL;
  mysprite->has_mask=0;
  return mysprite;
}

/***************************************************************************
...
***************************************************************************/
static struct sprite *ctor_sprite_mask(Pixmap mypixmap, Pixmap mask, 
				       int width, int height)
{
  struct sprite *mysprite=fc_malloc(sizeof(struct sprite));
  mysprite->pixmap=mypixmap;
  mysprite->mask=mask;

  mysprite->width=width;
  mysprite->height=height;
  mysprite->pcolorarray = NULL;
  mysprite->has_mask=1;
  return mysprite;
}

/***************************************************************************
 Returns the filename extensions the client supports
 Order is important.
***************************************************************************/
const char **gfx_fileextensions(void)
{
  static const char *ext[] =
  {
    "png",
    NULL
  };

  return ext;
}

/***************************************************************************
  Converts an image to a pixmap...
***************************************************************************/
static Pixmap image2pixmap(XImage *xi)
{
  Pixmap ret;
  XGCValues values;
  GC gc;
  
  ret = XCreatePixmap(display, root_window, xi->width, xi->height, xi->depth);

  values.foreground = 1;
  values.background = 0;
  gc = XCreateGC(display, ret, GCForeground | GCBackground, &values);

  XPutImage(display, ret, gc, xi, 0, 0, 0, 0, xi->width, xi->height);
  XFreeGC(display, gc);
  return ret;
}

/***************************************************************************
...
***************************************************************************/
struct sprite *load_gfxfile(const char *filename)
{
  png_structp pngp;
  png_infop infop;
  png_int_32 width, height, x, y;
  FILE *fp;
  int npalette, ntrans;
  png_colorp palette;
  png_bytep trans;
  png_bytep buf, pb;
  png_uint_32 stride;
  unsigned long *pcolorarray;
  bool *ptransarray;
  struct sprite *mysprite;
  XImage *xi;
  int has_mask;
  png_byte color_type;
  png_byte alpha;
  bool pixel, reported;

  fp = fopen(filename, "rb");
  if (!fp) {
    freelog(LOG_FATAL, "Failed reading PNG file: \"%s\"", filename);
    exit(EXIT_FAILURE);
  }

  pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pngp) {
    freelog(LOG_FATAL, "Failed creating PNG struct");
    exit(EXIT_FAILURE);
  }

  infop = png_create_info_struct(pngp);
  if (!infop) {
    freelog(LOG_FATAL, "Failed creating PNG struct");
    exit(EXIT_FAILURE);
  }
  
  if (setjmp(pngp->jmpbuf)) {
    freelog(LOG_FATAL, "Failed while reading PNG file: \"%s\"", filename);
    exit(EXIT_FAILURE);
  }

  png_init_io(pngp, fp);

  png_set_strip_16(pngp);
  png_set_packing(pngp);

  png_read_info(pngp, infop);
  width = png_get_image_width(pngp, infop);
  height = png_get_image_height(pngp, infop);
  color_type = png_get_color_type(pngp, infop);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    if (png_get_PLTE(pngp, infop, &palette, &npalette)) {
      int i;
      XColor *mycolors;

      pcolorarray = fc_malloc(npalette * sizeof(*pcolorarray));

      mycolors = fc_malloc(npalette * sizeof(*mycolors));

      for (i = 0; i < npalette; i++) {
	mycolors[i].red  = palette[i].red << 8;
	mycolors[i].green = palette[i].green << 8;
	mycolors[i].blue = palette[i].blue << 8;
      }

      alloc_colors(mycolors, npalette);

      for (i = 0; i < npalette; i++) {
	pcolorarray[i] = mycolors[i].pixel;
      }

      free(mycolors);
    } else {
      freelog(LOG_FATAL, "PNG file has no palette: \"%s\"", filename);
      exit(EXIT_FAILURE);
    }

    has_mask = png_get_tRNS(pngp, infop, &trans, &ntrans, NULL);

    if (has_mask) {
      int i;

      ptransarray = fc_calloc(npalette, sizeof(*ptransarray));

      reported = FALSE;
      for (i = 0; i < ntrans; i++) {
	if (trans[i] < npalette) {
	  ptransarray[trans[i]] = TRUE;
	} else if (!reported) {
	  freelog(LOG_VERBOSE,
		  "PNG: Transparent array entry is out of palette: \"%s\"",
		  filename);
	  reported = TRUE;
	}
      }
    } else {
      ptransarray = NULL;
    }

  } else {
    pcolorarray = NULL;
    ptransarray = NULL;
    npalette = 0;
    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
      has_mask = 1;
    } else {
      has_mask = 0;
    }
    ntrans = 0;
  }

  png_read_update_info(pngp, infop);

  {
    png_bytep *row_pointers;

    stride = png_get_rowbytes(pngp, infop);
    buf = fc_malloc(stride * height);

    row_pointers = fc_malloc(height * sizeof(png_bytep));

    for (y = 0, pb = buf; y < height; y++, pb += stride) {
      row_pointers[y] = pb;
    }

    png_read_image(pngp, row_pointers);
    png_read_end(pngp, infop);
    fclose(fp);

    free(row_pointers);
    if (infop != NULL) {
      png_destroy_read_struct(&pngp, &infop, (png_infopp)NULL);
    } else {
      freelog(LOG_ERROR,
	      "PNG info struct is NULL (non-fatal): \"%s\"",
	      filename);
      png_destroy_read_struct(&pngp, (png_infopp)NULL, (png_infopp)NULL);
    }
  }

  mysprite = fc_malloc(sizeof(*mysprite));


  xi = XCreateImage(display, DefaultVisual(display, screen_number),
		    display_depth, ZPixmap, 0, NULL, width, height, 32, 0);
  xi->data = fc_calloc(xi->bytes_per_line * xi->height, 1);

  pb = buf;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      if (pcolorarray) {
	XPutPixel(xi, x, y, pcolorarray[pb[x]]);
      } else {
	if (has_mask) {
	  XPutPixel(xi, x, y,
		    (pb[4 * x] << 16) + (pb[4 * x + 1] << 8)
		    + pb[4 * x + 2]);
	} else {
	  XPutPixel(xi, x, y,
		    (pb[3 * x] << 16) + (pb[3 * x + 1] << 8)
		    + pb[3 * x + 2]);
	}
      }
    }
    pb += stride;
  }
  mysprite->pixmap = image2pixmap(xi);
  XDestroyImage(xi);

  if (has_mask) {
    XImage *xm;

    xm = XCreateImage(display, DefaultVisual(display, screen_number),
		      1, XYBitmap, 0, NULL, width, height, 8, 0);
    xm->data = fc_calloc(xm->bytes_per_line * xm->height, 1);

    pb = buf;
    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	if (ptransarray) {
	  XPutPixel(xm, x, y, !ptransarray[pb[x]]);
	} else {
	  alpha = pb[4 * x + 3];
	  if (alpha > 204) {
	    pixel = FALSE;
	  } else if (alpha > 153) {
	    if ((y + x * 2) % 4 == 0) {
	      pixel = TRUE;
	    } else {
	      pixel = FALSE;
	    }
	  } else if (alpha > 102) {
	    if ((y + x) % 2 == 0) {
	      pixel = TRUE;
	    } else {
	      pixel = FALSE;
	    }
	  } else if (alpha > 51) {
	    if ((y + x * 2) % 4 == 0) {
	      pixel = FALSE;
	    } else {
	      pixel = TRUE;
	    }
	  } else {
	    pixel = TRUE;
	  }
	  XPutPixel(xm, x, y, !pixel);
	}
      }
      pb += stride;
    }
    mysprite->mask = image2pixmap(xm);
    XDestroyImage(xm);
  }

  mysprite->has_mask = has_mask;
  mysprite->width = width;
  mysprite->height = height;
  mysprite->pcolorarray = pcolorarray;
  mysprite->ncols = npalette;

  if (ptransarray) {
    free(ptransarray);
  }
  free(buf);
  return mysprite;
}

/***************************************************************************
   Deletes a sprite.  These things can use a lot of memory.
***************************************************************************/
void free_sprite(struct sprite *s)
{
  XFreePixmap(display, s->pixmap);
  if (s->has_mask) {
    XFreePixmap(display, s->mask);
  }
  if (s->pcolorarray) {
    free_colors(s->pcolorarray, s->ncols);
    free(s->pcolorarray);
    s->pcolorarray = NULL;
  }
  free(s);
}

/***************************************************************************
...
***************************************************************************/
Pixmap create_overlay_unit(const struct unit_type *punittype)
{
  Pixmap pm;
  enum color_std bg_color;
  
  pm=XCreatePixmap(display, root_window, 
		   tileset_full_tile_width(tileset), tileset_full_tile_height(tileset), display_depth);

  /* Give tile a background color, based on the type of unit */
  /* Should there be colors like COLOR_MAPVIEW_LAND etc? -ev */
  switch (unit_color_type(punittype)) {
    case UNIT_BG_LAND:
      bg_color = COLOR_OVERVIEW_LAND;
      break;
    case UNIT_BG_SEA:
      bg_color = COLOR_OVERVIEW_OCEAN;
      break;
    case UNIT_BG_HP_LOSS:
    case UNIT_BG_AMPHIBIOUS:
      bg_color = COLOR_OVERVIEW_MY_UNIT;
      break;
    case UNIT_BG_FLYING:
      bg_color = COLOR_OVERVIEW_ENEMY_CITY;
      break;
    default:
      bg_color = COLOR_OVERVIEW_UNKNOWN;
      break;
  }
  XSetForeground(display, fill_bg_gc,
		 get_color(tileset, bg_color)->color.pixel);
  XFillRectangle(display, pm, fill_bg_gc, 0,0, 
		 tileset_full_tile_width(tileset), tileset_full_tile_height(tileset));

  /* If we're using flags, put one on the tile */
  if(!solid_color_behind_units)  {
    struct sprite *flag = get_nation_flag_sprite(tileset, nation_of_player(client.conn.playing));

    XSetClipOrigin(display, civ_gc, 0,0);
    XSetClipMask(display, civ_gc, flag->mask);
    XCopyArea(display, flag->pixmap, pm, civ_gc, 0,0, 
    	      flag->width,flag->height, 0,0);
    XSetClipMask(display, civ_gc, None);
  }

  /* Finally, put a picture of the unit in the tile */
/*  if(i<utype_count()) */ {
    struct sprite *s = get_unittype_sprite(tileset, punittype);

    XSetClipOrigin(display,civ_gc,0,0);
    XSetClipMask(display,civ_gc,s->mask);
    XCopyArea(display, s->pixmap, pm, civ_gc,
	      0,0, s->width,s->height, 0,0 );
    XSetClipMask(display,civ_gc,None);
  }
  return(pm);
}


/***************************************************************************
  This function is so that packhand.c can be gui-independent, and
  not have to deal with Sprites itself.
***************************************************************************/
void free_intro_radar_sprites(void)
{
  if (intro_gfx_sprite) {
    free_sprite(intro_gfx_sprite);
    intro_gfx_sprite=NULL;
  }
  if (radar_gfx_sprite) {
    free_sprite(radar_gfx_sprite);
    radar_gfx_sprite=NULL;
  }
}
