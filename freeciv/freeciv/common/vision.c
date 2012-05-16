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

#include <assert.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "shared.h"

/* common */
#include "game.h"
#include "player.h"
#include "tile.h"
#include "vision.h"


/****************************************************************************
  Create a new vision source.
****************************************************************************/
struct vision *vision_new(struct player *pplayer, struct tile *ptile)
{
  struct vision *vision = fc_malloc(sizeof(*vision));

  vision->player = pplayer;
  vision->tile = ptile;
  vision->can_reveal_tiles = TRUE;
  vision_layer_iterate(v) {
    vision->radius_sq[v] = -1;
  } vision_layer_iterate_end;

  return vision;
}

/****************************************************************************
  Free the vision source.
****************************************************************************/
void vision_free(struct vision *vision)
{
  assert(vision->radius_sq[V_MAIN] < 0);
  assert(vision->radius_sq[V_INVIS] < 0);
  free(vision);
}

/****************************************************************************
  Sets the can_reveal_tiles flag.
  Returns the old flag.
****************************************************************************/
bool vision_reveal_tiles(struct vision *vision, bool reveal_tiles)
{
  bool was = vision->can_reveal_tiles;

  vision->can_reveal_tiles = reveal_tiles;
  return was;
}

/****************************************************************************
  Returns the sight points (radius_sq) that this vision source has.
****************************************************************************/
int vision_get_sight(const struct vision *vision, enum vision_layer vlayer)
{
  return vision->radius_sq[vlayer];
}

/****************************************************************************
  Frees vision site structure.
****************************************************************************/
void free_vision_site(struct vision_site *psite)
{
  free(psite);
}

/****************************************************************************
  Returns the basic structure.
****************************************************************************/
struct vision_site *create_vision_site(int identity, struct tile *location,
				       struct player *owner)
{
  struct vision_site *psite = fc_calloc(1, sizeof(*psite));

  psite->identity = identity;
  psite->location = location;
  psite->owner = owner;

  return psite;
}

/****************************************************************************
  Returns the basic structure filled with initial elements.
****************************************************************************/
struct vision_site *create_vision_site_from_city(const struct city *pcity)
{
  struct vision_site *psite =
    create_vision_site(pcity->id, city_tile(pcity), city_owner(pcity));

  psite->size = pcity->size;
  sz_strlcpy(psite->name, city_name(pcity));

  return psite;
}

/****************************************************************************
  Returns the basic structure filled with current elements.
****************************************************************************/
void update_vision_site_from_city(struct vision_site *psite,
				  const struct city *pcity)
{
  /* should be same identity and location */
  psite->owner = city_owner(pcity);

  psite->size = pcity->size;
  sz_strlcpy(psite->name, city_name(pcity));
}
