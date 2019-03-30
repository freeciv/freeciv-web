/***********************************************************************
 Freeciv - Copyright (C) 2003 - Per I. Mathisen
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
#include <string.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"

#include "citymap.h"

/* CITYMAP - reserve space for cities
 *
 * The citymap is a large int double array that corresponds to
 * the freeciv main map. For each tile, it stores three different 
 * and exclusive values in a single int: A positive int tells you
 * how many cities can use this tile (a crowdedness inidicator). A 
 * value of zero indicates that the tile is presently unused and 
 * available. A negative value means that this tile is occupied 
 * and reserved by some city or unit: in this case the value gives
 * the negative of the ID of the city or unit that has reserved the
 * tile.
 *
 * Code that uses the citymap should modify its behaviour based on
 * positive values encountered, and never attempt to steal a tile
 * which has a negative value.
 */

static int *citymap = NULL;

#define log_citymap log_debug

/**********************************************************************//**
  Initialize citymap by reserving worked tiles and establishing the
  crowdedness of (virtual) cities.
**************************************************************************/
void citymap_turn_init(struct player *pplayer)
{
  /* The citymap is reinitialized at the start of ever turn.  This includes
   * a call to realloc, which only really matters if this is the first turn
   * of the game (but it's easier than a separate function to do this). */
  citymap = fc_realloc(citymap, MAP_INDEX_SIZE * sizeof(*citymap));
  memset(citymap, 0, MAP_INDEX_SIZE * sizeof(*citymap));

  players_iterate(pother) {
    city_list_iterate(pother->cities, pcity) {
      struct tile *pcenter = city_tile(pcity);

      /* reserve at least the default (squared) city radius */
      city_tile_iterate(MAX(city_map_radius_sq_get(pcity),
                            CITY_MAP_DEFAULT_RADIUS_SQ),
                        pcenter, ptile) {
        struct city *pwork = tile_worked(ptile);

        if (NULL != pwork) {
          citymap[tile_index(ptile)] = -(pwork->id);
        } else {
	  citymap[tile_index(ptile)]++;
        }
      } city_tile_iterate_end;
    } city_list_iterate_end;
  } players_iterate_end;

  unit_list_iterate(pplayer->units, punit) {
    if (unit_is_cityfounder(punit)
        && punit->server.adv->task == AUT_BUILD_CITY) {

      /* use default (squared) city radius */
      city_tile_iterate(CITY_MAP_DEFAULT_RADIUS_SQ, punit->goto_tile,
                        ptile) {
        if (citymap[tile_index(ptile)] >= 0) {
          citymap[tile_index(ptile)]++;
        }
      } city_tile_iterate_end;

      citymap[tile_index(punit->goto_tile)] = -(punit->id);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Free resources allocated for citymap.
**************************************************************************/
void citymap_free(void)
{
  if (citymap != NULL) {
    FC_FREE(citymap);
  }
}

/**********************************************************************//**
  This function reserves a single tile for a (possibly virtual) city with
  a settler's or a city's id. Then it 'crowds' tiles that this city can 
  use to make them less attractive to other cities we may consider making.
**************************************************************************/
void citymap_reserve_city_spot(struct tile *ptile, int id)
{
#ifdef FREECIV_DEBUG
  log_citymap("id %d reserving (%d, %d), was %d", 
              id, TILE_XY(ptile), citymap[tile_index(ptile)]);
  fc_assert_ret(0 <= citymap[tile_index(ptile)]);
#endif /* FREECIV_DEBUG */

  /* Tiles will now be "reserved" by actual workers, so free excess
   * reservations. Also mark tiles for city overlapping, or 'crowding'.
   * Uses the default city map size / squared city radius. */
  city_tile_iterate(CITY_MAP_DEFAULT_RADIUS_SQ, ptile, ptile1) {
    if (citymap[tile_index(ptile1)] == -id) {
      citymap[tile_index(ptile1)] = 0;
    }
    if (citymap[tile_index(ptile1)] >= 0) {
      citymap[tile_index(ptile1)]++;
    }
  } city_tile_iterate_end;

  citymap[tile_index(ptile)] = -(id);
}

/**********************************************************************//**
  Reverse any reservations we have made in the surrounding area.
**************************************************************************/
void citymap_free_city_spot(struct tile *ptile, int id)
{
  city_tile_iterate(CITY_MAP_DEFAULT_RADIUS_SQ, ptile, ptile1) {
    if (citymap[tile_index(ptile1)] == -(id)) {
      citymap[tile_index(ptile1)] = 0;
    } else if (citymap[tile_index(ptile1)] > 0) {
      citymap[tile_index(ptile1)]--;
    }
  } city_tile_iterate_end;
}

/**********************************************************************//**
  Reserve additional tiles as desired (eg I would reserve best available
  food tile in addition to adjacent tiles)
**************************************************************************/
void citymap_reserve_tile(struct tile *ptile, int id)
{
#ifdef FREECIV_DEBUG
  fc_assert_ret(!citymap_is_reserved(ptile));
#endif

  citymap[tile_index(ptile)] = -id;
}

/**********************************************************************//**
  Returns a positive value if within a city radius, which is 1 x number of
  cities you are within the radius of, or zero or less if not. A negative
  value means this tile is reserved by a city and should not be taken.
**************************************************************************/
int citymap_read(struct tile *ptile)
{
  return citymap[tile_index(ptile)];
}

/**********************************************************************//**
  A tile is reserved if it contains a city or unit id, or a worker is
  assigned to it.
**************************************************************************/
bool citymap_is_reserved(struct tile *ptile)
{
  if (NULL != tile_worked(ptile) /*|| tile_city(ptile)*/) {
    return TRUE;
  }
  return (citymap[tile_index(ptile)] < 0);
}
