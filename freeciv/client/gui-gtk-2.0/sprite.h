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
#ifndef FC__SPRITE_H
#define FC__SPRITE_H

#include <gtk/gtk.h>

#include "sprite_g.h"

struct sprite
{
  /* A pixmap + mask is used if there's a 1-bit alpha channel.  mask may be
   * NULL if there's no alpha.  For multi-bit alpha levels, a pixbuf will be
   * used instead.  For consistency a pixbuf may be generated on-demand when
   * doing drawing (into a gtkpixcomm or gtkimage), so it's important that
   * the sprite data not be changed after the sprite is loaded. */
  GdkPixmap *pixmap, *pixmap_fogged;
  GdkBitmap *mask;
  GdkPixbuf *pixbuf, *pixbuf_fogged;

  int	     width;
  int	     height;
};

struct sprite *ctor_sprite(GdkPixbuf *pixbuf);
struct sprite *sprite_scale(struct sprite *src, int new_w, int new_h);
void sprite_get_bounding_box(struct sprite *sprite, int *start_x,
			     int *start_y, int *end_x, int *end_y);
struct sprite *crop_blankspace(struct sprite *s);

/********************************************************************
  Note: a sprite cannot be changed after these functions are called!
********************************************************************/
GdkPixbuf *sprite_get_pixbuf(struct sprite *sprite);
GdkBitmap *sprite_get_mask(struct sprite *sprite);

#endif  /* FC__SPRITE_H */

