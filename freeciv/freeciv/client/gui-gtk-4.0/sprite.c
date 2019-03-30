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

/* utility */
#include "log.h"
#include "mem.h"
#include "shared.h"

/* client/gui-gtk-4.0 */
#include "colors.h"
#include "mapview.h"

#include "sprite.h"

#define MAX_FILE_EXTENSIONS 50

/************************************************************************//**
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

  scale gives scale of new tileset
  smooth means if scaling might be bilinear, if set to false use nearest
  neighbor
****************************************************************************/
struct sprite *crop_sprite(struct sprite *source,
                           int x, int y,
                           int width, int height,
                           struct sprite *mask, int mask_offset_x, int mask_offset_y,
                           float scale, bool smooth)
{
  struct sprite *new = fc_malloc(sizeof(*new));
  cairo_t *cr;

  fc_assert_ret_val(source, NULL);

  new->surface = cairo_surface_create_similar(source->surface,
          CAIRO_CONTENT_COLOR_ALPHA, width, height);
  cr = cairo_create(new->surface);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_clip(cr);
  
  cairo_set_source_surface(cr, source->surface, -x, -y);
  cairo_paint(cr);
  if (mask) {
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
    cairo_set_source_surface(cr, mask->surface, mask_offset_x-x, mask_offset_y-y);
    cairo_paint(cr);
  }
  cairo_destroy(cr);

  return new;
}

/************************************************************************//**
  Create a sprite with the given height, width and color.
****************************************************************************/
struct sprite *create_sprite(int width, int height, struct color *pcolor)
{
  struct sprite *sprite = fc_malloc(sizeof(*sprite));
  cairo_t *cr;

  fc_assert_ret_val(width > 0, NULL);
  fc_assert_ret_val(height > 0, NULL);
  fc_assert_ret_val(pcolor != NULL, NULL);

  sprite->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               width, height);

  cr = cairo_create(sprite->surface);
  gdk_cairo_set_source_rgba(cr, &pcolor->color);
  cairo_paint(cr);
  cairo_destroy(cr);

  return sprite;
}

/************************************************************************//**
  Find the dimensions of the sprite.
****************************************************************************/
void get_sprite_dimensions(struct sprite *sprite, int *width, int *height)
{
  *width = cairo_image_surface_get_width(sprite->surface);
  *height = cairo_image_surface_get_height(sprite->surface);
}

/************************************************************************//**
  Returns the filename extensions the client supports
  Order is important.
****************************************************************************/
const char **gfx_fileextensions(void)
{
  /* Includes space for hardcoded 'png' and termination NULL */
  static const char *ext[MAX_FILE_EXTENSIONS + 2] =
  {
    NULL
  };

  if (ext[0] == NULL) {
    int count = 0;
    GSList *formats = gdk_pixbuf_get_formats();
    GSList *next = formats;

    while ((next = g_slist_next(next)) != NULL && count < MAX_FILE_EXTENSIONS) {
      GdkPixbufFormat *format = g_slist_nth_data(next, 0);
      gchar **mimes = gdk_pixbuf_format_get_mime_types(format);
      int i;

      /* Consider .png to be supported even when there's no mime-type called "png" */
      ext[count++] = fc_strdup("png");

      for (i = 0; mimes[i] != NULL && count < MAX_FILE_EXTENSIONS; i++) {
        char *end = strstr(mimes[i], "/");

        if (end != NULL) {
          ext[count++] = fc_strdup(end + 1);
        }
      }

      g_strfreev(mimes);
    }

    g_slist_free(formats);

    ext[count] = NULL;
  }

  return ext;
}

/************************************************************************//**
  Called when the cairo surface with freeciv allocated data is destroyed.
****************************************************************************/
static void surf_destroy_callback(void *data)
{
  free(data);
}

/************************************************************************//**
  Load the given graphics file into a sprite.  This function loads an
  entire image file, which may later be broken up into individual sprites
  with crop_sprite.
****************************************************************************/
struct sprite *load_gfxfile(const char *filename)
{
  struct sprite *spr;
  GError *err = NULL;;
  GdkPixbuf *pb = gdk_pixbuf_new_from_file(filename, &err);
  int width;
  int height;
  unsigned char *pbdata;
  int rs;
  unsigned char *cairo_data;
  unsigned char *data;
  int i, j;
  int cairo_stride;
  bool has_alpha;
  int channels;

  if (pb == NULL) {
    log_error(_("Can't load %s: %s"), filename, err->message);
    return NULL;
  }

  spr = fc_malloc(sizeof(*spr));
  width = gdk_pixbuf_get_width(pb);
  height = gdk_pixbuf_get_height(pb);
  pbdata = gdk_pixbuf_get_pixels(pb);
  rs = gdk_pixbuf_get_rowstride(pb);
  has_alpha = gdk_pixbuf_get_has_alpha(pb);
  channels = gdk_pixbuf_get_n_channels(pb);

  cairo_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  if (cairo_stride <= 0) {
    log_error("Cairo does not give stride for width %d", width);
    free(spr);
    return NULL;
  }

  cairo_data = fc_malloc(height * cairo_stride * 4);
  data = cairo_data;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      if (has_alpha) {
        unsigned char tmp;

#define MULTI_UNc(a,b) ((a * b - (b / 2)) / 0xFF)

        if (is_bigendian()) {
          tmp = pbdata[j * channels + 3];
          data[j * 4 + 3] = MULTI_UNc(pbdata[j * channels + 2], tmp);
          data[j * 4 + 2] = MULTI_UNc(pbdata[j * channels + 1], tmp);
          data[j * 4 + 1] = MULTI_UNc(pbdata[j * channels + 0], tmp);
          data[j * 4 + 0] = tmp;
        } else {
          tmp = MULTI_UNc(pbdata[j * channels + 2], pbdata[j * channels + 3]);
          data[j * 4 + 1] = MULTI_UNc(pbdata[j * channels + 1], pbdata[j * channels + 3]);
          data[j * 4 + 2] = MULTI_UNc(pbdata[j * channels + 0], pbdata[j * channels + 3]);
          data[j * 4 + 0] = tmp;
          data[j * 4 + 3] = pbdata[j * channels + 3];
        }

#undef MULTI_UNc

      } else {
        data[j * 4 + 3] = 255;
        data[j * 4 + 0] = pbdata[j * channels + 2];
        data[j * 4 + 1] = pbdata[j * channels + 1];
        data[j * 4 + 2] = pbdata[j * channels + 0];
      }
    }

    data += cairo_stride;
    pbdata += rs;
  }

  g_object_unref(pb);

  spr->surface = cairo_image_surface_create_for_data(cairo_data, CAIRO_FORMAT_ARGB32,
                                                     width, height, cairo_stride);
  if (spr->surface == NULL || cairo_surface_status(spr->surface) != CAIRO_STATUS_SUCCESS) {
    log_error("Cairo image surface creation error");
    free(spr);
    free(cairo_data);

    return NULL;
  }

  cairo_surface_set_user_data(spr->surface, NULL, cairo_data, surf_destroy_callback);

  fc_assert(cairo_image_surface_get_format(spr->surface) == CAIRO_FORMAT_ARGB32);

  if (cairo_surface_status(spr->surface) != CAIRO_STATUS_SUCCESS) {
    log_fatal("Failed reading graphics file: \"%s\"", filename);

    exit(EXIT_FAILURE);
  }

  return spr;
}

/************************************************************************//**
  Free a sprite and all associated image data.
****************************************************************************/
void free_sprite(struct sprite * s)
{
  cairo_surface_destroy(s->surface);
  free(s);
}

/************************************************************************//**
  Scales a sprite. If the sprite contains a mask, the mask is scaled
  as as well.
****************************************************************************/
struct sprite *sprite_scale(struct sprite *src, int new_w, int new_h)
{
  cairo_t *cr;
  struct sprite *new = fc_malloc(sizeof(*new));
  int width, height;

  get_sprite_dimensions(src, &width, &height);

  new->surface = cairo_surface_create_similar(src->surface, 
      CAIRO_CONTENT_COLOR_ALPHA, new_w, new_h);

  cr = cairo_create(new->surface);
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_restore(cr);
  cairo_scale(cr, (double) new_w / (double) width, (double) new_h / (double) height);
  cairo_set_source_surface(cr, src->surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(cr);

  return new;
}

/************************************************************************//**
  Method returns the bounding box of a sprite. It assumes a rectangular
  object/mask. The bounding box contains the border (pixel which have
  unset pixel as neighbours) pixel.
****************************************************************************/
void sprite_get_bounding_box(struct sprite * sprite, int *start_x,
			     int *start_y, int *end_x, int *end_y)
{
  unsigned char *data = cairo_image_surface_get_data(sprite->surface);
  int width = cairo_image_surface_get_width(sprite->surface);
  int height = cairo_image_surface_get_height(sprite->surface);
  int i, j;
  int endian;

  if (is_bigendian()) {
    endian = 0;
  } else {
    endian = 3;
  }

  fc_assert(cairo_image_surface_get_format(sprite->surface) == CAIRO_FORMAT_ARGB32);

  /* parses mask image for the first column that contains a visible pixel */
  *start_x = -1;
  for (i = 0; i < width && *start_x == -1; i++) {
    for (j = 0; j < height; j++) {
      if (data[(j * width + i) * 4 + endian]) {
	*start_x = i;
	break;
      }
    }
  }

  /* parses mask image for the last column that contains a visible pixel */
  *end_x = -1;
  for (i = width - 1; i >= *start_x && *end_x == -1; i--) {
    for (j = 0; j < height; j++) {
      if (data[(j * width + i) * 4 + endian]) {
	*end_x = i;
	break;
      }
    }
  }

  /* parses mask image for the first row that contains a visible pixel */
  *start_y = -1;
  for (i = 0; i < height && *start_y == -1; i++) {
    for (j = *start_x; j <= *end_x; j++) {
      if (data[(i * width + j) * 4 + endian]) {
	*start_y = i;
	break;
      }
    }
  }

  /* parses mask image for the last row that contains a visible pixel */
  *end_y = -1;
  for (i = height - 1; i >= *end_y && *end_y == -1; i--) {
    for (j = *start_x; j <= *end_x; j++) {
      if (data[(i * width + j) * 4 + endian]) {
	*end_y = i;
	break;
      }
    }
  }
}

/************************************************************************//**
  Crops all blankspace from a sprite (insofar as is possible as a rectangle)
****************************************************************************/
struct sprite *crop_blankspace(struct sprite *s)
{
  int x1, y1, x2, y2;

  sprite_get_bounding_box(s, &x1, &y1, &x2, &y2);

  return crop_sprite(s, x1, y1, x2 - x1 + 1, y2 - y1 + 1, NULL, -1, -1,
                     1.0, FALSE);
}

/************************************************************************//**
  Render a pixbuf from the sprite.

  NOTE: the pixmap and mask of a sprite must not change after this
        function is called!
****************************************************************************/
GdkPixbuf *sprite_get_pixbuf(struct sprite *sprite)
{
  int width, height;

  if (!sprite) {
    return NULL;
  }

  get_sprite_dimensions(sprite, &width, &height);

  return surface_get_pixbuf(sprite->surface, width, height);
}

/************************************************************************//**
  Render a pixbuf from the cairo surface
****************************************************************************/
GdkPixbuf *surface_get_pixbuf(cairo_surface_t *surf, int width, int height)
{
  cairo_t *cr;
  cairo_surface_t *tmpsurf;
  GdkPixbuf *pb;
  unsigned char *pixels;
  int rowstride;
  int i;

  pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  pixels = gdk_pixbuf_get_pixels(pb);
  rowstride = gdk_pixbuf_get_rowstride(pb);

  tmpsurf = cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
						width, height, rowstride);

  cr = cairo_create(tmpsurf);
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_restore(cr);
  cairo_set_source_surface(cr, surf, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);

  for (i = height; i > 0; i--) {
    unsigned char *p = pixels;
    unsigned char *end = p + 4 * width;
    unsigned char tmp;

#define DIV_UNc(a,b) (((guint16) (a) * 0xFF + ((b) / 2)) / (b))

    while (p < end) {
      tmp = p[0];

      if (is_bigendian()) {
        if (tmp != 0) {
          p[0] = DIV_UNc(p[1], tmp);
          p[1] = DIV_UNc(p[2], tmp);
          p[2] = DIV_UNc(p[3], tmp);
          p[3] = tmp;
        } else {
          p[1] = p[2] = p[3] = 0;
        }
      } else {
        if (p[3] != 0) {
          p[0] = DIV_UNc(p[2], p[3]);
          p[1] = DIV_UNc(p[1], p[3]);
          p[2] = DIV_UNc(tmp, p[3]);
        } else {
          p[0] = p[1] = p[2] = 0;
        }
      }

      p += 4;
    }

#undef DIV_UNc

    pixels += rowstride;
  }

  cairo_surface_destroy(tmpsurf);

  return pb;
}

/************************************************************************//**
  Create a pixbuf containing a representative image for the given extra
  type, to be used as an icon in the GUI.

  May return NULL on error.

  NB: You must call g_object_unref on the non-NULL return value when you
  no longer need it.
****************************************************************************/
GdkPixbuf *create_extra_pixbuf(const struct extra_type *pextra)
{
  struct drawn_sprite sprs[80];
  int count, w, h, canvas_x, canvas_y;
  GdkPixbuf *pixbuf;
  struct canvas canvas = FC_STATIC_CANVAS_INIT;
  cairo_t *cr;

  w = tileset_tile_width(tileset);
  h = tileset_tile_height(tileset);

  canvas.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  canvas_x = 0;
  canvas_y = 0;

  cr = cairo_create(canvas.surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_destroy(cr);

  count = fill_basic_extra_sprite_array(tileset, sprs, pextra);
  put_drawn_sprites(&canvas, 1.0, canvas_x, canvas_y, count, sprs, FALSE);

  pixbuf = surface_get_pixbuf(canvas.surface, w, h);
  cairo_surface_destroy(canvas.surface);

  return pixbuf;
}

/************************************************************************//**
  Create a GtkImage from cairo surface.
****************************************************************************/
GtkWidget *image_new_from_surface(cairo_surface_t *surf)
{
  GdkPixbuf *pb;
  GtkWidget *image;

  pb = surface_get_pixbuf(surf,
                          cairo_image_surface_get_width(surf),
                          cairo_image_surface_get_height(surf));

  image = gtk_image_new_from_pixbuf(pb);
  g_object_unref(pb);

  return image;
}

/************************************************************************//**
  Set a GtkImage from cairo surface.
****************************************************************************/
void image_set_from_surface(GtkImage *image, cairo_surface_t *surf)
{
  GdkPixbuf *pb;

  pb = surface_get_pixbuf(surf,
                          cairo_image_surface_get_width(surf),
                          cairo_image_surface_get_height(surf));

  gtk_image_set_from_pixbuf(image, pb);
  g_object_unref(pb);
}
