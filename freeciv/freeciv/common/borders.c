/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "tile.h"
#include "unit.h"

#include "borders.h"

/*************************************************************************
  Border radius sq from given border source tile.
*************************************************************************/
int tile_border_source_radius_sq(struct tile *ptile)
{
  struct city *pcity;
  int radius_sq = 0;

  if (game.info.borders == 0) {
    return 0;
  }

  pcity = tile_city(ptile);

  if (pcity) {
    radius_sq = game.info.border_city_radius_sq;
    radius_sq += pcity->size * game.info.border_size_effect;
  } else {
    base_type_iterate(pbase) {
      if (tile_has_base(ptile, pbase) && territory_claiming_base(pbase)) {
        radius_sq = pbase->border_sq;
        break;
      }
    } base_type_iterate_end;
  }

  return radius_sq;
}

/*************************************************************************
  Border source strength
*************************************************************************/
int tile_border_source_strength(struct tile *ptile)
{
  struct city *pcity;
  int strength = 0;

  if (game.info.borders == 0) {
    return 0;
  }

  pcity = tile_city(ptile);

  if (pcity) {
    strength = pcity->size + 2;
  } else {
    base_type_iterate(pbase) {
      if (tile_has_base(ptile, pbase) && territory_claiming_base(pbase)) {
        strength = 1;
        break;
      }
    } base_type_iterate_end;
  }

  return strength;
}

/*************************************************************************
  Border source strength at tile
*************************************************************************/
int tile_border_strength(struct tile *ptile, struct tile *source)
{
  int full_strength = tile_border_source_strength(source);
  int sq_dist = sq_map_distance(ptile, source);

  if (sq_dist > 0) {
    return full_strength * full_strength / sq_dist;
  } else {
    return FC_INFINITY;
  }
}

/*************************************************************************
  Is given tile source to borders.
*************************************************************************/
bool is_border_source(struct tile *ptile)
{
  if (tile_city(ptile)) {
    return TRUE;
  }

  if (tile_owner(ptile) != NULL) {
    base_type_iterate(pbase) {
      if (tile_has_base(ptile, pbase) && territory_claiming_base(pbase)) {
        return TRUE;
      }
    } base_type_iterate_end;
  }

  return FALSE;
}
