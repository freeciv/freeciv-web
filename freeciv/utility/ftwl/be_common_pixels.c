/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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
#include <stdio.h>

#include "log.h"
#include "mem.h"
#include "support.h"

#include "back_end.h"
#include "be_common_pixels.h"

#include <ft2build.h>
#include FT_FREETYPE_H

/*************************************************************************
  Blit one osda onto another, using transparency value given on all 
  pixels that have alpha mask set. dest_pos and src_pos can be NULL.
*************************************************************************/
void be_copy_osda_to_osda(struct osda *dest,
			  struct osda *src,
			  const struct ct_size *size,
			  const struct ct_point *dest_pos,
			  const struct ct_point *src_pos)
{
  struct ct_point tmp_pos = { 0, 0 };

  if (!src_pos) {
    src_pos = &tmp_pos;
  }

  if (!dest_pos) {
    dest_pos = &tmp_pos;
  }

  if (!ct_size_empty(size)) {
    image_copy(dest->image, src->image, size, dest_pos, src_pos);
  }
}

/*************************************************************************
  Draw the given bitmap (ie a 1bpp pixmap) on the given osda in the given
  color and at the given position.
*************************************************************************/
void be_draw_bitmap(struct osda *target, be_color color,
		    const struct ct_point *position,
		    struct FT_Bitmap_ *bitmap)
{
  if (bitmap->pixel_mode == ft_pixel_mode_mono) {
    draw_mono_bitmap(target->image, color, position, bitmap);
  } else if (bitmap->pixel_mode == ft_pixel_mode_grays) {
    assert(bitmap->num_grays == 256);
    draw_alpha_bitmap(target->image, color, position, bitmap);
  } else {
    assert(0);
  }
}

/*************************************************************************
  Allocate and initialize an osda (off-screen drawing area).
*************************************************************************/
struct osda *be_create_osda(int width, int height)
{
  struct osda *result = fc_malloc(sizeof(*result));

  result->image = image_create(width, height);
  result->magic = 11223344;

  return result;
}

/*************************************************************************
  Free an allocated osda.
*************************************************************************/
void be_free_osda(struct osda *osda)
{
    /*
  assert(osda->magic == 11223344);
  osda->magic = 0;
  image_destroy(osda->image);
  free(osda);
    */
}

/*************************************************************************
  Put size of osda into size.
*************************************************************************/
void be_osda_get_size(struct ct_size *size, const struct osda *osda)
{
  size->width = image_get_width(osda->image);
  size->height = image_get_height(osda->image);
}

/*************************************************************************
  ...
*************************************************************************/
void be_multiply_alphas(struct sprite *dest, const struct sprite *src,
                        const struct ct_point *src_pos)
{
  image_multiply_alphas(dest->image, src->image, src_pos);
}

/*************************************************************************
  ...
*************************************************************************/
static struct sprite *ctor_sprite(struct image *image)
{
  struct sprite *result = fc_malloc(sizeof(struct sprite));
  result->image = image;
  return result;
}

/*************************************************************************
  ...
*************************************************************************/
void be_free_sprite(struct sprite *sprite)
{
  free(sprite);
}

/*************************************************************************
  ...
*************************************************************************/
struct sprite *be_crop_sprite(struct sprite *source,
			      int x, int y, int width, int height)
{
  struct sprite *result = ctor_sprite(image_create(width, height));
  struct ct_rect region = { x, y, width, height };

  ct_clip_rect(&region, image_get_full_rect(source->image));

  image_copy_full(source->image, result->image, &region);

  return result;
}

/*************************************************************************
  ...
*************************************************************************/
struct sprite *be_load_gfxfile(const char *filename)
{
  struct image *gfx = image_load_gfxfile(filename);

  return ctor_sprite(gfx);
}

/*************************************************************************
  ...
*************************************************************************/
void be_sprite_get_size(struct ct_size *size, const struct sprite *sprite)
{
  size->width = image_get_width(sprite->image);
  size->height = image_get_height(sprite->image);
}


/* ========== drawing ================ */

/*************************************************************************
  size, dest_pos and src_pos can be NULL
*************************************************************************/
void be_draw_sprite(struct osda *target, 
		    const struct sprite *sprite,
		    const struct ct_size *size,
		    const struct ct_point *dest_pos,
		    const struct ct_point *src_pos)
{
  struct ct_size tmp_size;
  struct ct_point tmp_pos = { 0, 0 };

  if (!src_pos) {
    src_pos = &tmp_pos;
  }

  if (!dest_pos) {
    dest_pos = &tmp_pos;
  }

  if (!size) {
    tmp_size.width = image_get_width(sprite->image);
    tmp_size.height = image_get_height(sprite->image);
    size = &tmp_size;
  }

  image_copy(target->image, sprite->image, size, dest_pos, src_pos);
}
