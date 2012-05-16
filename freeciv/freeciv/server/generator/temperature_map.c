/********************************************************************** 
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
#include <config.h>
#endif

#include "map.h"

#include "height_map.h"
#include "temperature_map.h"
#include "mapgen_topology.h"
#include "utilities.h" 

static int *temperature_map;

#define tmap(_tile) (temperature_map[tile_index(_tile)])

/**************************************************************
  Return TRUE if temperateure_map is initialized
**************************************************************/
bool temperature_is_initialized(void)
{
  return temperature_map != NULL;
}
/*********************************************************
 return true if the tile has tt temperature type
**********************************************************/
bool tmap_is(const struct tile *ptile, temperature_type tt)
{
  return BOOL_VAL(tmap(ptile) & (tt));
}

/*****************************************************************
 return true if at last one tile has tt temperature type
****************************************************************/
bool is_temperature_type_near(const struct tile *ptile, temperature_type tt) 
{
  adjc_iterate(ptile, tile1) {
    if (BOOL_VAL(tmap(tile1) & (tt))) {
      return TRUE;
    };
  } adjc_iterate_end;
  return FALSE;
}

/**************************************************************************** 
   Free the tmap
 ****************************************************************************/
void destroy_tmap(void)
{
  assert(temperature_map != NULL);
  free(temperature_map);
  temperature_map = NULL;
}

/***************************************************************************
 * create_tmap initialize the temperature_map
 * if arg is FALSE, create a dumy tmap == map_colattitude
 * to be used if hmap or oceans are not placed gen 2-4
 ***************************************************************************/
void create_tmap(bool real)
{
  int i;

  /* if map is defined this is not changed */
  /* TO DO load if from scenario game with tmap */
  assert(temperature_map == NULL); /* to debug, never load a this time */
  if (temperature_map != NULL) {
    return;
  }

  temperature_map = fc_malloc(sizeof(*temperature_map) * MAP_INDEX_SIZE);
  whole_map_iterate(ptile) {
  
     /* the base temperature is equal to base map_colatitude */
    int t = map_colatitude(ptile);
    if (!real) {
      tmap(ptile) = t;
    } else {
      /* height land can be 30% collest */
      float height = - 0.3 * MAX(0, hmap(ptile) - hmap_shore_level) 
	  / (hmap_max_level - hmap_shore_level); 
      /* near ocean temperature can be 15 % more "temperate" */
      float temperate = (0.15 * (map.server.temperature / 100 - t
                                 / MAX_COLATITUDE)
                         * 2 * MIN(50 , count_ocean_near_tile(ptile,
                                                              FALSE, TRUE))
                         / 100);

      tmap(ptile) =  t * (1.0 + temperate) * (1.0 + height);
    }
  } whole_map_iterate_end;
  /* adjust to get well sizes frequencies */
  /* Notice: if colatitude is load from a scenario never call adjust has
             scenario maybe has a odd colatitude ditribution and adjust will
	     brack it */
  if (!map.server.alltemperate) {
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
}
