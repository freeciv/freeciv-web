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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unit.h"
#include "version.h"

/* client */
#include "climisc.h"
#include "colors.h"
#include "mapview_g.h"
#include "options.h"
#include "tilespec.h"

/* client/gui-gtk-3.0 */
#include "gui_main.h"

#include "graphics.h"

struct sprite *intro_gfx_sprite;

GdkCursor *fc_cursors[CURSOR_LAST][NUM_CURSOR_FRAMES];

/***********************************************************************//**
  Returns TRUE to indicate that gtk3-client supports given view type
***************************************************************************/
bool is_view_supported(enum ts_type type)
{
  switch (type) {
  case TS_ISOMETRIC:
  case TS_OVERHEAD:
    return TRUE;
  case TS_3D:
    return FALSE;
  }

  return FALSE;
}

/***********************************************************************//**
  Loading tileset of the specified type
***************************************************************************/
void tileset_type_set(enum ts_type type)
{
}

#define COLOR_MOTTO_FACE_R    0x2D
#define COLOR_MOTTO_FACE_G    0x71
#define COLOR_MOTTO_FACE_B    0xE3

/***********************************************************************//**
  Load cursor sprites
***************************************************************************/
void load_cursors(void)
{
  enum cursor_type cursor;
  GdkDisplay *display = gdk_display_get_default();
  int frame;

  for (cursor = 0; cursor < CURSOR_LAST; cursor++) {
    for (frame = 0; frame < NUM_CURSOR_FRAMES; frame++) {
      int hot_x, hot_y;
      struct sprite *sprite
	= get_cursor_sprite(tileset, cursor, &hot_x, &hot_y, frame);
      GdkPixbuf *pixbuf = sprite_get_pixbuf(sprite);

      fc_cursors[cursor][frame] = gdk_cursor_new_from_pixbuf(display, pixbuf,
							     hot_x, hot_y);
      g_object_unref(G_OBJECT(pixbuf));
    }
  }
}

/***********************************************************************//**
  This function is so that packhand.c can be gui-independent, and
  not have to deal with Sprites itself.
***************************************************************************/
void free_intro_radar_sprites(void)
{
  if (intro_gfx_sprite) {
    free_sprite(intro_gfx_sprite);
    intro_gfx_sprite = NULL;
  }
}
