/********************************************************************** 
 Freeciv - Copyright (C) 1996 - 2004 The Freeciv Project Team 
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

#include "log.h"
#include "fcintl.h"
#include "movement.h"

#include "map.h"

#include "maphand.h"

#include "mapgen_topology.h"
#include "startpos.h"
#include "temperature_map.h"
#include "utilities.h"

struct islands_data_type {
  Continent_id id;
  int size;
  int goodies;
  int starters;
  int total;
};
static struct islands_data_type *islands;
static int *islands_index;

/****************************************************************************
  Return an approximation of the goodness of a tile to a civilization.
****************************************************************************/
static int get_tile_value(struct tile *ptile)
{
  struct terrain *old_terrain;
  bv_special old_special;
  int value, irrig_bonus, mine_bonus;

  /* Give one point for each food / shield / trade produced. */
  value = 0;
  output_type_iterate(o) {
    value += city_tile_output(NULL, ptile, FALSE, o);
  } output_type_iterate_end;

  old_terrain = ptile->terrain;
  old_special = ptile->special;

  tile_set_special(ptile, S_ROAD);
  tile_apply_activity(ptile, ACTIVITY_IRRIGATE);
  irrig_bonus = -value;
  output_type_iterate(o) {
    irrig_bonus += city_tile_output(NULL, ptile, FALSE, o);
  } output_type_iterate_end;

  ptile->terrain = old_terrain;
  ptile->special = old_special;
  tile_set_special(ptile, S_ROAD);
  tile_apply_activity(ptile, ACTIVITY_MINE);
  mine_bonus = -value;
  output_type_iterate(o) {
    mine_bonus += city_tile_output(NULL, ptile, FALSE, o);
  } output_type_iterate_end;

  ptile->terrain = old_terrain;
  ptile->special = old_special;

  value += MAX(0, MAX(mine_bonus, irrig_bonus)) / 2;

  return value;
}

struct start_filter_data {
  int count;			/* Number of existing start positions. */
  int min_value;
  struct unit_type *initial_unit;
  int *value;
};

/**************************************************************************
  Return TRUE if (x,y) is a good starting position.

  Bad places:
  - Islands with no room.
  - Non-suitable terrain;
  - On a hut;
  - Too close to another starter on the same continent:
    'dist' is too close (real_map_distance)
    'nr' is the number of other start positions in
    map.server.start_positions to check for too closeness.
**************************************************************************/
static bool is_valid_start_pos(const struct tile *ptile, const void *dataptr)
{
  const struct start_filter_data *pdata = dataptr;
  int i;
  struct islands_data_type *island;
  int cont_size, cont = tile_continent(ptile);

  /* Only start on certain terrain types. */  
  if (pdata->value[tile_index(ptile)] < pdata->min_value) {
      return FALSE;
  } 

  assert(cont > 0);
  if (islands[islands_index[cont]].starters == 0) {
    return FALSE;
  }

  /* Don't start on a hut. */
  if (tile_has_special(ptile, S_HUT)) {
    return FALSE;
  }

  /* Has to be native tile for initial unit */
  if (!is_native_tile(pdata->initial_unit, ptile)) {
    return FALSE;
  }

  /* A longstanding bug allowed starting positions to exist on poles,
   * sometimes.  This hack prevents it by setting a fixed distance from
   * the pole (dependent on map temperature) that a start pos must be.
   * Cold and frozen tiles are not allowed for start pos placement. */
  if (tmap_is(ptile, TT_NHOT)) {
    return FALSE;
  }

  /* Don't start too close to someone else. */
  cont_size = get_continent_size(cont);
  island = islands + islands_index[cont];
  for (i = 0; i < pdata->count; i++) {
    struct tile *tile1 = map.server.start_positions[i].tile;

    if ((tile_continent(ptile) == tile_continent(tile1)
	 && (real_map_distance(ptile, tile1) * 1000 / pdata->min_value
	     <= (sqrt(cont_size / island->total))))
	|| (real_map_distance(ptile, tile1) * 1000 / pdata->min_value < 5)) {
      return FALSE;
    }
  }
  return TRUE;
}

/*************************************************************************
 help function for qsort
 *************************************************************************/
static int compare_islands(const void *A_, const void *B_)
{
  const struct islands_data_type *A = A_, *B = B_;

  return B->goodies - A->goodies;
}

/****************************************************************************
  Initialize islands data.
****************************************************************************/
static void initialize_isle_data(void)
{
  int nr;

  islands = fc_malloc((map.num_continents + 1) * sizeof(*islands));
  islands_index = fc_malloc((map.num_continents + 1)
			    * sizeof(*islands_index));

  /* islands[0] is unused. */
  for (nr = 1; nr <= map.num_continents; nr++) {
    islands[nr].id = nr;
    islands[nr].size = get_continent_size(nr);
    islands[nr].goodies = 0;
    islands[nr].starters = 0;
    islands[nr].total = 0;
  }
}

/****************************************************************************
  A function that filters for TER_STARTER tiles.
****************************************************************************/
static bool filter_starters(const struct tile *ptile, const void *data)
{
  return terrain_has_flag(tile_terrain(ptile), TER_STARTER);
}

/**************************************************************************
  where do the different nations start on the map? well this function tries
  to spread them out on the different islands.

  MT_SIGLE: one player per isle
  MT_2or3: 2 players per isle (maybe one isle with 3)
  MT_ALL: all players in asingle isle
  MT_VARIABLE: at least 2 player per isle
  
  Assumes assign_continent_numbers() has already been done!
  Returns true on success
**************************************************************************/
bool create_start_positions(enum start_mode mode,
			    struct unit_type *initial_unit)
{
  struct tile *ptile;
  int k, sum;
  struct start_filter_data data;
  int tile_value_aux[MAP_INDEX_SIZE], tile_value[MAP_INDEX_SIZE];
  int min_goodies_per_player = 1500;
  int total_goodies = 0;
  /* this is factor is used to maximize land used in extreme little maps */
  float efactor =  player_count() / map.server.size / 4; 
  bool failure = FALSE;
  bool is_tmap = temperature_is_initialized();

  if (!is_tmap) {
    /* The temperature map has already been destroyed by the time start
     * positions have been placed.  We check for this and then create a
     * false temperature map. This is used in the tmap_is() call above.
     * We don't create a "real" map here because that requires the height
     * map and other information which has already been destroyed. */
    create_tmap(FALSE);
  }

  /* If the default is given, just use MT_VARIABLE. */
  if (mode == MT_DEFAULT) {
    mode = MT_VARIABLE;
  }

  /* get the tile value */
  whole_map_iterate(ptile) {
    tile_value_aux[tile_index(ptile)] = get_tile_value(ptile);
  } whole_map_iterate_end;

  /* select the best tiles */
  whole_map_iterate(ptile) {
    int this_tile_value = tile_value_aux[tile_index(ptile)];
    int lcount = 0, bcount = 0;

    city_tile_iterate(ptile, ptile1) {
      if (this_tile_value > tile_value_aux[tile_index(ptile1)]) {
	lcount++;
      } else if (this_tile_value < tile_value_aux[tile_index(ptile1)]) {
	bcount++;
      }
    } city_tile_iterate_end;

    if (lcount <= bcount) {
      this_tile_value = 0;
    }
    tile_value[tile_index(ptile)] = 100 * this_tile_value;
  } whole_map_iterate_end;
  /* get an average value */
  smooth_int_map(tile_value, TRUE);

  initialize_isle_data();

  /* oceans are not good for starters; discard them */
  whole_map_iterate(ptile) {
    if (!filter_starters(ptile, NULL)) {
      tile_value[tile_index(ptile)] = 0;
    } else {
      islands[tile_continent(ptile)].goodies += tile_value[tile_index(ptile)];
      total_goodies += tile_value[tile_index(ptile)];
    }
  } whole_map_iterate_end;

  /* evaluate the best places on the map */
  adjust_int_map_filtered(tile_value, 1000, NULL, filter_starters);
 
  /* Sort the islands so the best ones come first.  Note that islands[0] is
   * unused so we just skip it. */
  qsort(islands + 1, map.num_continents,
	sizeof(*islands), compare_islands);

  /* If we can't place starters according to the first choice, change the
   * choice. */
  if (mode == MT_SINGLE && map.num_continents < player_count() + 3) {
    mode = MT_2or3;
  }

  if (mode == MT_2or3 && map.num_continents < player_count() / 2 + 4) {
    mode = MT_VARIABLE;
  }

  if (mode == MT_ALL 
      && (islands[1].goodies < player_count() * min_goodies_per_player
	  || islands[1].goodies < total_goodies * (0.5 + 0.8 * efactor)
	  / (1 + efactor))) {
    mode = MT_VARIABLE;
  }

  /* the variable way is the last posibility */
  if (mode == MT_VARIABLE) {
    min_goodies_per_player = total_goodies * (0.65 + 0.8 * efactor) 
      / (1 + efactor)  / player_count();
  }

  { 
    int nr, to_place = player_count(), first = 1;

    /* inizialize islands_index */
    for (nr = 1; nr <= map.num_continents; nr++) {
      islands_index[islands[nr].id] = nr;
    }

    /* searh for best first island for fairness */    
    if ((mode == MT_SINGLE) || (mode == MT_2or3)) {
      float var_goodies, best = HUGE_VAL;
      int num_islands
	= (mode == MT_SINGLE) ? player_count() : (player_count() / 2);

      for (nr = 1; nr <= 1 + map.num_continents - num_islands; nr++) {
	if (islands[nr + num_islands - 1].goodies < min_goodies_per_player) {
	  break;
	}
	var_goodies
	    = (islands[nr].goodies - islands[nr + num_islands - 1].goodies)
	    / (islands[nr + num_islands - 1].goodies);

	if (var_goodies < best * 0.9) {
	  best = var_goodies;
	  first = nr;
	}
      }
    }

    /* set starters per isle */
    if (mode == MT_ALL) {
      islands[1].starters = to_place;
      islands[1].total = to_place;
      to_place = 0;
    }
    for (nr = 1; nr <= map.num_continents; nr++) {
      if (mode == MT_SINGLE && to_place > 0 && nr >= first) {
	islands[nr].starters = 1;
	islands[nr].total = 1;
	to_place--;
      }
      if (mode == MT_2or3 && to_place > 0 && nr >= first) {
	islands[nr].starters = 2 + (nr == 1 ? (player_count() % 2) : 0);
	to_place -= islands[nr].total = islands[nr].starters;
      }

      if (mode == MT_VARIABLE && to_place > 0) {
	islands[nr].starters = MAX(1, islands[nr].goodies 
				   / min_goodies_per_player);
	to_place -= islands[nr].total = islands[nr].starters;
      }
    }
  }

  data.count = 0;
  data.value = tile_value;
  data.min_value = 900;
  data.initial_unit = initial_unit;
  sum = 0;
  for (k = 1; k <= map.num_continents; k++) {
    sum += islands[islands_index[k]].starters;
    if (islands[islands_index[k]].starters != 0) {
      freelog(LOG_VERBOSE, "starters on isle %i", k);
    }
  }
  assert(player_count() <= data.count + sum);

  /* now search for the best place and set start_positions */
  map.server.start_positions =
    fc_realloc(map.server.start_positions, player_count()
               * sizeof(*map.server.start_positions));
  while (data.count < player_count()) {
    if ((ptile = rand_map_pos_filtered(&data, is_valid_start_pos))) {
      islands[islands_index[(int) tile_continent(ptile)]].starters--;
      map.server.start_positions[data.count].tile = ptile;
      map.server.start_positions[data.count].nation = NO_NATION_SELECTED;
      freelog(LOG_DEBUG,
	      "Adding %d,%d as starting position %d, %d goodies on islands.",
	      TILE_XY(ptile), data.count,
	      islands[islands_index[(int) tile_continent(ptile)]].goodies);
      data.count++;

    } else {
      data.min_value *= 0.95;
      if (data.min_value <= 10) {
	freelog(LOG_ERROR,
	        _("The server appears to have gotten into an infinite loop "
	          "in the allocation of starting positions.\n"
	          "Maybe the number of players is too high for this map."));
	freelog(LOG_ERROR,
		/* TRANS: No full stop after the URL, could cause confusion. */
		_("Please report this message at %s"),
		BUG_URL);
	failure = TRUE;
	break;
      }
    }
  }
  map.server.num_start_positions = player_count();

  free(islands);
  free(islands_index);
  islands = NULL;
  islands_index = NULL;

  if (!is_tmap) {
    destroy_tmap();
  }

  return !failure;
}
