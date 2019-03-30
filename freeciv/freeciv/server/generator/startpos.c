/***********************************************************************
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
#include <fc_config.h>
#endif

#include <math.h> /* sqrt, HUGE_VAL */

/* utility */
#include "log.h"
#include "fcintl.h"

/* common */
#include "game.h"
#include "map.h"
#include "movement.h"

/* server */
#include "maphand.h"

/* server/generator */
#include "mapgen_topology.h"
#include "mapgen_utils.h"
#include "startpos.h"
#include "temperature_map.h"

struct islands_data_type {
  Continent_id id;
  int size;
  int goodies;
  int starters;
  int total;
};
static struct islands_data_type *islands;
static int *islands_index;

/************************************************************************//**
  Return an approximation of the goodness of a tile to a civilization.
****************************************************************************/
static int get_tile_value(struct tile *ptile)
{
  int value;
  int irrig_bonus = 0;
  int mine_bonus = 0;
  struct tile *roaded;
  struct extra_type *nextra;

  /* Give one point for each food / shield / trade produced. */
  value = 0;
  output_type_iterate(o) {
    value += city_tile_output(NULL, ptile, FALSE, o);
  } output_type_iterate_end;

  roaded = tile_virtual_new(ptile);

  if (num_role_units(L_SETTLERS) > 0) {
    struct unit_type *start_worker = get_role_unit(L_SETTLERS, 0);

    extra_type_by_cause_iterate(EC_ROAD, pextra) {
      struct road_type *proad = extra_road_get(pextra);

      if (road_can_be_built(proad, roaded)
          && are_reqs_active(NULL, NULL, NULL, NULL, roaded,
                             NULL, start_worker, NULL, NULL, NULL,
                             &pextra->reqs, RPT_CERTAIN)) {
        tile_add_extra(roaded, pextra);
      }
    } extra_type_by_cause_iterate_end;
  }

  nextra = next_extra_for_tile(roaded, EC_IRRIGATION, NULL, NULL);

  if (nextra != NULL) {
    struct tile *vtile;

    vtile = tile_virtual_new(roaded);
    tile_apply_activity(vtile, ACTIVITY_IRRIGATE, nextra);
    irrig_bonus = -value;
    output_type_iterate(o) {
      irrig_bonus += city_tile_output(NULL, vtile, FALSE, o);
    } output_type_iterate_end;
    tile_virtual_destroy(vtile);
  }

  nextra = next_extra_for_tile(roaded, EC_MINE, NULL, NULL);

  /* Same set of roads used with mine as with irrigation. */
  if (nextra != NULL) {
    struct tile *vtile;

    vtile = tile_virtual_new(roaded);
    tile_apply_activity(vtile, ACTIVITY_MINE, nextra);
    mine_bonus = -value;
    output_type_iterate(o) {
      mine_bonus += city_tile_output(NULL, vtile, FALSE, o);
    } output_type_iterate_end;
    tile_virtual_destroy(vtile);
  }

  tile_virtual_destroy(roaded);

  value += MAX(0, MAX(mine_bonus, irrig_bonus)) / 2;

  return value;
}

struct start_filter_data {
  int min_value;
  struct unit_type *initial_unit;
  int *value;
};

/************************************************************************//**
  Check if number of reachable native tiles is sufficient.
  Initially given tile is assumed to be native (not checked by this function)
****************************************************************************/
static bool check_native_area(const struct unit_type *utype,
                              const struct tile *ptile,
                              int min_area)
{
  int tiles = 1; /* There's the central tile already. */
  struct tile_list *tlist = tile_list_new();
  struct tile *central = tile_virtual_new(ptile); /* Non-const virtual tile */
  struct dbv handled;

  dbv_init(&handled, MAP_INDEX_SIZE);

  tile_list_append(tlist, central);

  while (tile_list_size(tlist) > 0 && tiles < min_area) {
    tile_list_iterate(tlist, ptile2) {
      adjc_iterate(&(wld.map), ptile2, ptile3) {
        int idx = tile_index(ptile3);

        if (!dbv_isset(&handled, idx) && is_native_tile(utype, ptile3)) {
          tiles++;
          tile_list_append(tlist, ptile3);
          dbv_set(&handled, idx);
          if (tiles >= min_area) {
            /* Break out when we already know that area is sufficient. */
            break;
          }
        }
      } adjc_iterate_end;

      tile_list_remove(tlist, ptile2);

      if (tiles >= min_area) {
        /* Break out when we already know that area is sufficient. */
        break;
      }
    } tile_list_iterate_end;
  }

  tile_list_destroy(tlist);

  dbv_free(&handled);

  tile_virtual_destroy(central);

  return tiles >= min_area;
}

/************************************************************************//**
  Return TRUE if (x,y) is a good starting position.

  Bad places:
  - Islands with no room.
  - Non-suitable terrain;
  - On a hut;
  - Too close to another starter on the same continent:
    'dist' is too close (real_map_distance)
    'nr' is the number of other start positions to check for too closeness.
****************************************************************************/
static bool is_valid_start_pos(const struct tile *ptile, const void *dataptr)
{
  const struct start_filter_data *pdata = dataptr;
  struct islands_data_type *island;
  int cont_size, cont = tile_continent(ptile);

  /* Only start on certain terrain types. */  
  if (pdata->value[tile_index(ptile)] < pdata->min_value) {
      return FALSE;
  } 

  fc_assert_ret_val(cont > 0, FALSE);
  if (islands[islands_index[cont]].starters == 0) {
    return FALSE;
  }

  /* Don't start on a hut. */
  if (tile_has_cause_extra(ptile, EC_HUT)) {
    return FALSE;
  }

  /* Has to be native tile for initial unit */
  if (!is_native_tile(pdata->initial_unit, ptile)) {
    return FALSE;
  }

  /* Check native area size. */
  if (!check_native_area(pdata->initial_unit, ptile,
                         terrain_control.min_start_native_area)) {
    return FALSE;
  }

  if (game.server.start_city && terrain_has_flag(tile_terrain(ptile), TER_NO_CITIES)) {
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
  map_startpos_iterate(psp) {
    struct tile *tile1 = startpos_tile(psp);

    if ((tile_continent(ptile) == tile_continent(tile1)
         && (real_map_distance(ptile, tile1) * 1000 / pdata->min_value
             <= (sqrt(cont_size / island->total))))
        || (real_map_distance(ptile, tile1) * 1000 / pdata->min_value < 5)) {
      return FALSE;
    }
  } map_startpos_iterate_end;
  return TRUE;
}

/************************************************************************//**
  Helper function for qsort
****************************************************************************/
static int compare_islands(const void *A_, const void *B_)
{
  const struct islands_data_type *A = A_, *B = B_;

  return B->goodies - A->goodies;
}

/************************************************************************//**
  Initialize islands data.
****************************************************************************/
static void initialize_isle_data(void)
{
  int nr;

  islands = fc_malloc((wld.map.num_continents + 1) * sizeof(*islands));
  islands_index = fc_malloc((wld.map.num_continents + 1)
                            * sizeof(*islands_index));

  /* islands[0] is unused. */
  for (nr = 1; nr <= wld.map.num_continents; nr++) {
    islands[nr].id = nr;
    islands[nr].size = get_continent_size(nr);
    islands[nr].goodies = 0;
    islands[nr].starters = 0;
    islands[nr].total = 0;
  }
}

/************************************************************************//**
  A function that filters for TER_STARTER tiles.
****************************************************************************/
static bool filter_starters(const struct tile *ptile, const void *data)
{
  return terrain_has_flag(tile_terrain(ptile), TER_STARTER);
}

/************************************************************************//**
  where do the different nations start on the map? well this function tries
  to spread them out on the different islands.

  MAPSTARTPOS_SINGLE: one player per isle.
  MAPSTARTPOS_2or3: 2 players per isle (maybe one isle with 3).
  MAPSTARTPOS_ALL: all players in asingle isle.
  MAPSTARTPOS_VARIABLE: at least 2 player per isle.
  
  Assumes assign_continent_numbers() has already been done!
  Returns true on success
****************************************************************************/
bool create_start_positions(enum map_startpos mode,
                            struct unit_type *initial_unit)
{
  struct tile *ptile;
  int k, sum;
  struct start_filter_data data;
  int *tile_value_aux = NULL;
  int *tile_value = NULL;
  int min_goodies_per_player = 1500;
  int total_goodies = 0;
  /* this is factor is used to maximize land used in extreme little maps */
  float efactor =  player_count() / map_size_checked() / 4; 
  bool failure = FALSE;
  bool is_tmap = temperature_is_initialized();

  if (wld.map.num_continents < 1) {
    /* Currently we can only place starters on land terrain, so fail
     * immediately if there isn't any on the map. */
    log_verbose("Map has no land, so cannot assign start positions!");
    return FALSE;
  }

  if (!is_tmap) {
    /* The temperature map has already been destroyed by the time start
     * positions have been placed.  We check for this and then create a
     * false temperature map. This is used in the tmap_is() call above.
     * We don't create a "real" map here because that requires the height
     * map and other information which has already been destroyed. */
    create_tmap(FALSE);
  }

  /* If the default is given, just use MAPSTARTPOS_VARIABLE. */
  if (MAPSTARTPOS_DEFAULT == mode) {
    log_verbose("Using startpos=VARIABLE");
    mode = MAPSTARTPOS_VARIABLE;
  }

  tile_value_aux = fc_calloc(MAP_INDEX_SIZE, sizeof(*tile_value_aux));
  tile_value = fc_calloc(MAP_INDEX_SIZE, sizeof(*tile_value));

  /* get the tile value */
  whole_map_iterate(&(wld.map), value_tile) {
    tile_value_aux[tile_index(value_tile)] = get_tile_value(value_tile);
  } whole_map_iterate_end;

  /* select the best tiles */
  whole_map_iterate(&(wld.map), value_tile) {
    int this_tile_value = tile_value_aux[tile_index(value_tile)];
    int lcount = 0, bcount = 0;

    /* check all tiles within the default city radius */
    city_tile_iterate(CITY_MAP_DEFAULT_RADIUS_SQ, value_tile, ptile1) {
      if (this_tile_value > tile_value_aux[tile_index(ptile1)]) {
        lcount++;
      } else if (this_tile_value < tile_value_aux[tile_index(ptile1)]) {
        bcount++;
      }
    } city_tile_iterate_end;

    if (lcount <= bcount) {
      this_tile_value = 0;
    }
    tile_value[tile_index(value_tile)] = 100 * this_tile_value;
  } whole_map_iterate_end;
  /* get an average value */
  smooth_int_map(tile_value, TRUE);

  initialize_isle_data();

  /* Only consider tiles marked as 'starter terrains' by ruleset */
  whole_map_iterate(&(wld.map), starter_tile) {
    if (!filter_starters(starter_tile, NULL)) {
      tile_value[tile_index(starter_tile)] = 0;
    } else {
      /* Oceanic terrain cannot be starter terrain currently */
      fc_assert_action(tile_continent(starter_tile) > 0, continue);
      islands[tile_continent(starter_tile)].goodies += tile_value[tile_index(starter_tile)];
      total_goodies += tile_value[tile_index(starter_tile)];
    }
  } whole_map_iterate_end;

  /* evaluate the best places on the map */
  adjust_int_map_filtered(tile_value, 1000, NULL, filter_starters);

  /* Sort the islands so the best ones come first.  Note that islands[0] is
   * unused so we just skip it. */
  qsort(islands + 1, wld.map.num_continents,
        sizeof(*islands), compare_islands);

  /* If we can't place starters according to the first choice, change the
   * choice. */
  if (MAPSTARTPOS_SINGLE == mode
      && wld.map.num_continents < player_count() + 3) {
    log_verbose("Not enough continents; falling back to startpos=2or3");
    mode = MAPSTARTPOS_2or3;
  }

  if (MAPSTARTPOS_2or3 == mode
      && wld.map.num_continents < player_count() / 2 + 4) {
    log_verbose("Not enough continents; falling back to startpos=VARIABLE");
    mode = MAPSTARTPOS_VARIABLE;
  }

  if (MAPSTARTPOS_ALL == mode
      && (islands[1].goodies < player_count() * min_goodies_per_player
	  || islands[1].goodies < total_goodies * (0.5 + 0.8 * efactor)
	  / (1 + efactor))) {
    log_verbose("No good enough island; falling back to startpos=VARIABLE");
    mode = MAPSTARTPOS_VARIABLE;
  }

  /* the variable way is the last possibility */
  if (MAPSTARTPOS_VARIABLE == mode) {
    min_goodies_per_player = total_goodies * (0.65 + 0.8 * efactor) 
      / (1 + efactor)  / player_count();
  }

  { 
    int nr, to_place = player_count(), first = 1;

    /* inizialize islands_index */
    for (nr = 1; nr <= wld.map.num_continents; nr++) {
      islands_index[islands[nr].id] = nr;
    }

    /* When placing a fixed number of players per island, for fairness, try
     * to avoid sets of populated islands where there is more than 10%
     * variation in "goodness" within the entire set. (Fallback if that fails:
     * place players on the worst available islands.) */
    if (MAPSTARTPOS_SINGLE == mode || MAPSTARTPOS_2or3 == mode) {
      float var_goodies, best = HUGE_VAL;
      int num_islands = (MAPSTARTPOS_SINGLE == mode
                         ? player_count() : player_count() / 2);

      for (nr = 1; nr <= 1 + wld.map.num_continents - num_islands; nr++) {
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
    if (MAPSTARTPOS_ALL == mode) {
      islands[1].starters = to_place;
      islands[1].total = to_place;
      to_place = 0;
    }
    for (nr = 1; nr <= wld.map.num_continents; nr++) {
      if (MAPSTARTPOS_SINGLE == mode && 0 < to_place && nr >= first) {
        islands[nr].starters = 1;
        islands[nr].total = 1;
        to_place--;
      }
      if (MAPSTARTPOS_2or3 == mode && 0 < to_place && nr >= first) {
        islands[nr].starters = 2 + (nr == 1 ? (player_count() % 2) : 0);
        to_place -= islands[nr].total = islands[nr].starters;
      }

      if (MAPSTARTPOS_VARIABLE == mode && 0 < to_place) {
        islands[nr].starters = MAX(1, islands[nr].goodies
                                   / MAX(1, min_goodies_per_player));
        to_place -= islands[nr].total = islands[nr].starters;
      }
    }
  }

  data.value = tile_value;
  data.min_value = 900;
  data.initial_unit = initial_unit;
  sum = 0;
  for (k = 1; k <= wld.map.num_continents; k++) {
    sum += islands[islands_index[k]].starters;
    if (islands[islands_index[k]].starters != 0) {
      log_verbose("starters on isle %i", k);
    }
  }
  fc_assert_ret_val(player_count() <= sum, FALSE);

  /* now search for the best place and set start_positions */
  while (map_startpos_count() < player_count()) {
    if ((ptile = rand_map_pos_filtered(&(wld.map), &data,
                                       is_valid_start_pos))) {
      islands[islands_index[(int) tile_continent(ptile)]].starters--;
      log_debug("Adding (%d, %d) as starting position %d, %d goodies on "
                "islands.", TILE_XY(ptile), map_startpos_count(),
                islands[islands_index[(int) tile_continent(ptile)]].goodies);
      (void) map_startpos_new(ptile);
    } else {
      data.min_value *= 0.95;
      if (data.min_value <= 10) {
        log_normal(_("The server appears to have gotten into an infinite "
                     "loop in the allocation of starting positions.\nMaybe "
                     "the number of players is too high for this map."));
        failure = TRUE;
        break;
      }
    }
  }

  free(islands);
  free(islands_index);
  islands = NULL;
  islands_index = NULL;

  if (!is_tmap) {
    destroy_tmap();
  }

  FC_FREE(tile_value_aux);
  FC_FREE(tile_value);

  return !failure;
}
