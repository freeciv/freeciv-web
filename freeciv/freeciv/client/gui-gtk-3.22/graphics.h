/***********************************************************************
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
#ifndef FC__GRAPHICS_H
#define FC__GRAPHICS_H

#include <gtk/gtk.h>

/* client */
#include "graphics_g.h"
#include "mapview_common.h"

/* client/gui-gtk-3.22 */
#include "canvas.h"
#include "sprite.h"

extern struct sprite *intro_gfx_sprite;

/* This name is to avoid a naming conflict with a global 'cursors'
 * variable in GTK+-2.6.  See PR#12459. */
extern GdkCursor *fc_cursors[CURSOR_LAST][NUM_CURSOR_FRAMES];

#endif  /* FC__GRAPHICS_H */
