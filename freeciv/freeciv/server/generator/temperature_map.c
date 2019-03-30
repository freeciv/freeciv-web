/***********************************************************************
 Freeciv - Copyright (C) 2004 - Marcelo J. Burda
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

/* utilities */
#include "log.h"

/* common */
#include "map.h"

/* generator */
#include "height_map.h"
#include "mapgen_topology.h"
#include "mapgen_utils.h"

#include "temperature_map.h"

static int *temperature_map;

#define tmap(_tile) (temperature_map[tile_index(_tile)])

/**********************************************************************//**
  Returns one line (given by the y coordinate) of the temperature map.
**************************************************************************/
#ifdef FREECIV_DEBUG
static char *tmap_y2str(int ycoor)
{
  static char buf[MAP_MAX_LINEAR_SIZE + 1];
  char *p = buf;
  int i, idx;

  for (i = 0; i < wld.map.xsize; i++) {
    idx = ycoor * wld.map.xsize + i;

    if (idx > wld.map.xsize * wld.map.ysize) {
      break;
    }

    switch (temperature_map[idx]) {
    case TT_TROPICAL:
      *p++ = 't'; /* tropical */
      break;
    case TT_TEMPERATE:
      *p++ = 'm'; /* medium */
      break;
    case TT_COLD:
      *p++ = 'c'; /* cold */
      break;
    case TT_FROZEN:
      *p++ = 'f'; /* frozen */
      break;
    }
  }

  *p = '\0';

  return buf;
}
#endif /* FREECIV_DEBUG */

/**********************************************************************//**
  Return TRUE if temperateure_map is initialized
**************************************************************************/
bool temperature_is_initialized(void)
{
  return temperature_map != NULL;
}

/**********************************************************************//**
  Return true if the tile has tt temperature type
**************************************************************************/
bool tmap_is(const struct tile *ptile, temperature_type tt)
{
  return BOOL_VAL(tmap(ptile) & (tt));
}

/**********************************************************************//**
  Return true if at least one tile has tt temperature type
**************************************************************************/
bool is_temperature_type_near(const struct tile *ptile, temperature_type tt) 
{
  adjc_iterate(&(wld.map), ptile, tile1) {
    if (BOOL_VAL(tmap(tile1) & (tt))) {
      return TRUE;
    };
  } adjc_iterate_end;

  return FALSE;
}

/**********************************************************************//**
   Free the tmap
**************************************************************************/
void destroy_tmap(void)
{
  fc_assert_ret(NULL != temperature_map);
  free(temperature_map);
  temperature_map = NULL;
}

/**********************************************************************//**
  Initialize the temperature_map
  if arg is FALSE, create a dummy tmap == map_colatitude
  to be used if hmap or oceans are not placed gen 2-4
**************************************************************************/
void create_tmap(bool real)
{
  int i;

  /* if map is defined this is not changed */
  /* TODO: load if from scenario game with tmap */
  /* to debug, never load a this time */
  fc_assert_ret(NULL == temperature_map);

  temperature_map = fc_malloc(sizeof(*temperature_map) * MAP_INDEX_SIZE);
  whole_map_iterate(&(wld.map), ptile) {
    /* the base temperature is equal to base map_colatitude */
    int t = map_colatitude(ptile);

    if (!real) {
      tmap(ptile) = t;
    } else {
      /* high land can be 30% cooler */
      float height = - 0.3 * MAX(0, hmap(ptile) - hmap_shore_level) 
	  / (hmap_max_level - hmap_shore_level); 
      /* near ocean temperature can be 15% more "temperate" */
      float temperate = (0.15 * (wld.map.server.temperature / 100 - t
                                 / MAX_COLATITUDE)
                         * 2 * MIN(50, count_terrain_class_near_tile(ptile,
                                                                     FALSE,
                                                                     TRUE,
                                                                     TC_OCEAN))
                         / 100);

      tmap(ptile) =  t * (1.0 + temperate) * (1.0 + height);
    }
  } whole_map_iterate_end;
  /* adjust to get well sizes frequencies */
  /* Notice: if colatitude is loaded from a scenario never call adjust.
             Scenario may have an odd colatitude distribution and adjust will
	     break it */
  if (!wld.map.server.alltemperate) {
    adjust_int_map(temperature_map, MAX_COLATITUDE);
  }
  /* now simplify to 4 base values */ 
  for (i = 0; i < MAP_INDEX_SIZE; i++) {
    int t = temperature_map[i];

    if (t >= TROPICAL_LEVEL) {
      temperature_map[i] = TT_TROPICAL;
    } else if (t >= COLD_LEVEL) {
      temperature_map[i] = TT_TEMPERATE;
    } else if (t >= 2 * ICE_BASE_LEVEL) {
      temperature_map[i] = TT_COLD;
    } else {
      temperature_map[i] = TT_FROZEN;
    }
  }

  log_debug("%stemperature map ({f}rozen, {c}old, {m}edium, {t}ropical):",
            real ? "real " : "");
  for (i = 0; i < wld.map.ysize; i++) {
    log_debug("%5d: %s", i, tmap_y2str(i));
  }
}
