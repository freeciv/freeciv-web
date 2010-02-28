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

#include <stdlib.h>		/* for NULL */

#include "back_end.h"

#include "sprite.h"

/**************************************************************************
  Return a NULL-terminated, permanently allocated array of possible
  graphics types extensions.  Extensions listed first will be checked
  first.
**************************************************************************/
const char **gfx_fileextensions(void)
{
  static const char *ext[] = {
    "png",	/* png should be the default. */
    NULL
  };

  return ext;
}

/**************************************************************************
  Load the given graphics file into a sprite.  This function loads an
  entire image file, which may later be broken up into individual sprites
  with crop_sprite.
**************************************************************************/
struct sprite *load_gfxfile(const char *filename)
{
  return be_load_gfxfile(filename);
}

/**************************************************************************
  Create a new sprite by cropping and taking only the given portion of
  the image.
**************************************************************************/
struct sprite *crop_sprite(struct sprite *source,
			   int x, int y, int width, int height,
			   struct sprite *mask, int mask_offset_x,
			   int mask_offset_y)
{
  struct sprite *result = be_crop_sprite(source, x, y, width, height);

  if (mask) {
    struct ct_point pos = { x - mask_offset_x, y - mask_offset_y };

    be_multiply_alphas(result, mask, &pos);
  }
  return result;
}

/****************************************************************************
  Find the dimensions of the sprite.
****************************************************************************/
void get_sprite_dimensions(struct sprite *sprite, int *width, int *height)
{
  struct ct_size size;

  be_sprite_get_size(&size, sprite);
  *width = size.width;
  *height = size.height;
}

/**************************************************************************
  Free a sprite and all associated image data.
**************************************************************************/
void free_sprite(struct sprite *s)
{
  be_free_sprite(s);
}
