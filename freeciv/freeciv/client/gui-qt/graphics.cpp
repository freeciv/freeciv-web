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

// client
#include "tilespec.h"

// gui-qt
#include "graphics.h"
#include "qtg_cxxside.h"

static struct sprite *intro_gfx_sprite = NULL;

/************************************************************************//**
  Return whether the client supports given view type.
****************************************************************************/
bool qtg_is_view_supported(enum ts_type type)
{
  switch (type) {
  case TS_ISOMETRIC:
  case TS_OVERHEAD:
    return true;
  case TS_3D:
    return false;
  }

  return false;
}

/************************************************************************//**
  Loading tileset of the specified type
****************************************************************************/
void qtg_tileset_type_set(enum ts_type type)
{
}

/************************************************************************//**
  Frees the introductory sprites.
****************************************************************************/
void qtg_free_intro_radar_sprites()
{
  if (intro_gfx_sprite) {
    free_sprite(intro_gfx_sprite);
    intro_gfx_sprite = NULL;
  }
}
