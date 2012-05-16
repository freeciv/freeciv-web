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

#include "log.h"

#include "sprite.h"

/****************************************************************************
  Create a new sprite by cropping and taking only the given portion of
  the image.

  source gives the sprite that is to be cropped.

  x,y, width, height gives the rectangle to be cropped.  The pixel at
  position of the source sprite will be at (0,0) in the new sprite, and
  the new sprite will have dimensions (width, height).

  mask gives an additional mask to be used for clipping the new sprite.

  mask_offset_x, mask_offset_y is the offset of the mask relative to the
  origin of the source image.  The pixel at (mask_offset_x,mask_offset_y)
  in the mask image will be used to clip pixel (0,0) in the source image
  which is pixel (-x,-y) in the new image.
****************************************************************************/
struct sprite *crop_sprite(struct sprite *source,
			   int x, int y,
			   int width, int height,
			   struct sprite *mask,
			   int mask_offset_x, int mask_offset_y)
{
  GdkPixbuf *mypixbuf, *sub, *mask_pixbuf;

  /* First just crop the image. */
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  width = CLIP(0, width, source->width - x);
  height = CLIP(0, height, source->height - y);
  sub = gdk_pixbuf_new_subpixbuf(sprite_get_pixbuf(source), x, y,
				 width, height);
  mypixbuf = gdk_pixbuf_copy(sub);
  g_object_unref(sub);

  /* Now mask.  This reduces the alpha of the final image proportional to the
   * alpha of the mask.  Thus if the mask has 50% alpha the final image will
   * be reduced by 50% alpha.  Note that the mask offset is in coordinates
   * relative to the clipped image not the final image. */
  if (mask
      && (mask_pixbuf = sprite_get_pixbuf(mask))
      && gdk_pixbuf_get_has_alpha(mask_pixbuf)) {
    int x1, y1;

    /* The mask offset is the offset of the mask relative to the origin
     * of the original source image.  For instance when cropping with
     * blending sprites the offset is always 0.  Here we convert the
     * coordinates so that they are relative to the origin of the new
     * (cropped) image. */
    mask_offset_x -= x;
    mask_offset_y -= y;

    width = CLIP(0, width, mask->width + mask_offset_x);
    height = CLIP(0, height, mask->height + mask_offset_y);

    if (!gdk_pixbuf_get_has_alpha(mypixbuf)) {
      GdkPixbuf *p2 = mypixbuf;

      mypixbuf = gdk_pixbuf_add_alpha(mypixbuf, FALSE, 0, 0, 0);
      g_object_unref(p2);
    }

    for (x1 = 0; x1 < width; x1++) {
      for (y1 = 0; y1 < height; y1++) {
	int mask_x = x1 - mask_offset_x, mask_y = y1 - mask_offset_y;
	guchar *alpha = gdk_pixbuf_get_pixels(mypixbuf)
	  + y1 * gdk_pixbuf_get_rowstride(mypixbuf)
	  + x1 * gdk_pixbuf_get_n_channels(mypixbuf)
	  + 3;
	guchar *mask_alpha = gdk_pixbuf_get_pixels(mask_pixbuf)
	  + mask_y * gdk_pixbuf_get_rowstride(mask_pixbuf)
	  + mask_x * gdk_pixbuf_get_n_channels(mask_pixbuf)
	  + 3;

	*alpha = (*alpha) * (*mask_alpha) / 255;
      }
    }
  }

  return ctor_sprite(mypixbuf);
}

/****************************************************************************
  Find the dimensions of the sprite.
****************************************************************************/
void get_sprite_dimensions(struct sprite *sprite, int *width, int *height)
{
  *width = sprite->width;
  *height = sprite->height;
}

/****************************************************************************
  Create a new sprite with the given pixmap, dimensions, and
  (optional) mask.

  FIXME: should be renamed as sprite_new or some such.
****************************************************************************/
struct sprite *ctor_sprite(GdkPixbuf *pixbuf)
{
  struct sprite *sprite = fc_malloc(sizeof(*sprite));
  bool has_alpha = FALSE, has_mask = FALSE;

  sprite->width = gdk_pixbuf_get_width(pixbuf);
  sprite->height = gdk_pixbuf_get_height(pixbuf);

  /* Check to see if this pixbuf has an alpha layer. */
  if (gdk_pixbuf_get_has_alpha(pixbuf)) {
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    int x, y, rowstride = gdk_pixbuf_get_rowstride(pixbuf);

    for (y = 0; y < sprite->height; y++) {
      for (x = 0; x < sprite->width; x++) {
	int i = y * rowstride + 4 * x + 3;
	guchar pixel = pixels[i];

	if (pixel > 0 && pixel < 255) {
	  has_alpha = TRUE;
	}
	if (pixel == 0) {
	  has_mask = TRUE;
	}
      }
    }
  }

  sprite->pixbuf_fogged = NULL;
  sprite->pixmap_fogged = NULL;
  if (has_alpha) {
    sprite->pixbuf = pixbuf;
    sprite->pixmap = NULL;
    sprite->mask = NULL;
  } else {
    gdk_pixbuf_render_pixmap_and_mask(pixbuf, &sprite->pixmap,
				      &sprite->mask, 1);
    if (!has_mask && sprite->mask) {
      g_object_unref(sprite->mask);
      sprite->mask = NULL;
    }
    g_object_unref(pixbuf);
    sprite->pixbuf = NULL;
  }
  return sprite;
}

/****************************************************************************
  Returns the filename extensions the client supports
  Order is important.
****************************************************************************/
const char **gfx_fileextensions(void)
{
  static const char *ext[] =
  {
    "png",
    "xpm",
    NULL
  };

  return ext;
}

/****************************************************************************
  Load the given graphics file into a sprite.  This function loads an
  entire image file, which may later be broken up into individual sprites
  with crop_sprite.
****************************************************************************/
struct sprite *load_gfxfile(const char *filename)
{
  GdkPixbuf *im;

  if (!(im = gdk_pixbuf_new_from_file(filename, NULL))) {
    freelog(LOG_FATAL, "Failed reading graphics file: \"%s\"", filename);
    exit(EXIT_FAILURE);
  }

  return ctor_sprite(im);
}

/****************************************************************************
  Free a sprite and all associated image data.
****************************************************************************/
void free_sprite(struct sprite * s)
{
  if (s->pixmap) {
    g_object_unref(s->pixmap);
    s->pixmap = NULL;
  }
  if (s->mask) {
    g_object_unref(s->mask);
    s->mask = NULL;
  }
  if (s->pixbuf) {
    g_object_unref(s->pixbuf);
    s->pixbuf = NULL;
  }
  if (s->pixmap_fogged) {
    g_object_unref(s->pixmap_fogged);
  }
  if (s->pixbuf_fogged) {
    g_object_unref(s->pixbuf_fogged);
  }
  free(s);
}

/****************************************************************************
  Scales a sprite. If the sprite contains a mask, the mask is scaled
  as as well.
****************************************************************************/
struct sprite *sprite_scale(struct sprite *src, int new_w, int new_h)
{
  return ctor_sprite(gdk_pixbuf_scale_simple(sprite_get_pixbuf(src),
					     new_w, new_h,
					     GDK_INTERP_BILINEAR));
}

/****************************************************************************
  Method returns the bounding box of a sprite. It assumes a rectangular
  object/mask. The bounding box contains the border (pixel which have
  unset pixel as neighbours) pixel.
****************************************************************************/
void sprite_get_bounding_box(struct sprite * sprite, int *start_x,
			     int *start_y, int *end_x, int *end_y)
{
  GdkImage *mask_image;
  GdkBitmap *mask = sprite_get_mask(sprite);
  int i, j;

  if (!mask) {
    *start_x = 0;
    *start_y = 0;
    *end_x = sprite->width - 1;
    *end_y = sprite->height - 1;
    return;
  }

  mask_image
    = gdk_drawable_get_image(mask, 0, 0, sprite->width, sprite->height);


  /* parses mask image for the first column that contains a visible pixel */
  *start_x = -1;
  for (i = 0; i < sprite->width && *start_x == -1; i++) {
    for (j = 0; j < sprite->height; j++) {
      if (gdk_image_get_pixel(mask_image, i, j) != 0) {
	*start_x = i;
	break;
      }
    }
  }

  /* parses mask image for the last column that contains a visible pixel */
  *end_x = -1;
  for (i = sprite->width - 1; i >= *start_x && *end_x == -1; i--) {
    for (j = 0; j < sprite->height; j++) {
      if (gdk_image_get_pixel(mask_image, i, j) != 0) {
	*end_x = i;
	break;
      }
    }
  }

  /* parses mask image for the first row that contains a visible pixel */
  *start_y = -1;
  for (i = 0; i < sprite->height && *start_y == -1; i++) {
    for (j = *start_x; j <= *end_x; j++) {
      if (gdk_image_get_pixel(mask_image, j, i) != 0) {
	*start_y = i;
	break;
      }
    }
  }

  /* parses mask image for the last row that contains a visible pixel */
  *end_y = -1;
  for (i = sprite->height - 1; i >= *end_y && *end_y == -1; i--) {
    for (j = *start_x; j <= *end_x; j++) {
      if (gdk_image_get_pixel(mask_image, j, i) != 0) {
	*end_y = i;
	break;
      }
    }
  }

  g_object_unref(mask_image);
}

/****************************************************************************
  Crops all blankspace from a sprite (insofar as is possible as a rectangle)
****************************************************************************/
struct sprite *crop_blankspace(struct sprite *s)
{
  int x1, y1, x2, y2;

  sprite_get_bounding_box(s, &x1, &y1, &x2, &y2);

  return crop_sprite(s, x1, y1, x2 - x1 + 1, y2 - y1 + 1, NULL, -1, -1);
}

/****************************************************************************
  Converts a pixmap/mask sprite to a GdkPixbuf.

  This is just a helper function for sprite_get_pixbuf().  Most callers
  should use that function instead.
****************************************************************************/
static GdkPixbuf *gdk_pixbuf_new_from_pixmap_sprite(struct sprite *src)
{
  GdkPixbuf *dst;
  int w, h;

  w = src->width;
  h = src->height;
  
  /* convert pixmap */
  dst = gdk_pixbuf_new(GDK_COLORSPACE_RGB, src->mask != NULL, 8, w, h);
  gdk_pixbuf_get_from_drawable(dst, src->pixmap, NULL, 0, 0, 0, 0, w, h);

  /* convert mask */
  if (src->mask) {
    GdkImage *img;
    int x, y, rowstride;
    guchar *pixels;

    img = gdk_drawable_get_image(src->mask, 0, 0, w, h);

    pixels = gdk_pixbuf_get_pixels(dst);
    rowstride = gdk_pixbuf_get_rowstride(dst);

    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
	guchar *pixel = pixels + y * rowstride + x * 4 + 3;

	if (gdk_image_get_pixel(img, x, y)) {
	  *pixel = 255;
	} else {
	  *pixel = 0;
	}
      }
    }
    g_object_unref(img);
  }

  return dst;
}

/********************************************************************
  Render a pixbuf from the sprite.

  NOTE: the pixmap and mask of a sprite must not change after this
        function is called!
********************************************************************/
GdkPixbuf *sprite_get_pixbuf(struct sprite *sprite)
{
  if (!sprite) {
    return NULL;
  }
  
  if (!sprite->pixbuf) {
    sprite->pixbuf = gdk_pixbuf_new_from_pixmap_sprite(sprite);
  }
  return sprite->pixbuf;
}

/****************************************************************************
  Render a mask from the sprite.

  NOTE: the pixbuf of a sprite must not change after this function is called!
****************************************************************************/
GdkBitmap *sprite_get_mask(struct sprite *sprite)
{
  if (!sprite->pixmap && !sprite->mask) {
    /* If we're not in pixmap mode and we don't yet have a mask, render
     * the pixbuf to a mask. */
    gdk_pixbuf_render_pixmap_and_mask(sprite->pixbuf, NULL,
				      &sprite->mask, 1);
  }
  return sprite->mask;
}
