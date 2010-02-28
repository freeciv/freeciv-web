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

#include <stdlib.h>

#include "tilespec.h"

#include "graphics.h"

/****************************************************************************
  Return whether the client supports isometric view (isometric tilesets).
****************************************************************************/
bool isometric_view_supported(void)
{
  /* PORTME */
  return TRUE;
}

/****************************************************************************
  Return whether the client supports "overhead" (non-isometric) view.
****************************************************************************/
bool overhead_view_supported(void)
{
  /* PORTME */
  return TRUE;
}

/****************************************************************************
  Load the introductory graphics.
****************************************************************************/
void load_intro_gfx(void)
{
  /* PORTME */
}

/****************************************************************************
  Load the cursors (mouse substitute sprites), including a goto cursor,
  an airdrop cursor, a nuke cursor, and a patrol cursor.
****************************************************************************/
void load_cursors(void)
{
  /* PORTME */
}

/****************************************************************************
  Frees the introductory sprites.
****************************************************************************/
void free_intro_radar_sprites(void)
{
  /* PORTME */
}
