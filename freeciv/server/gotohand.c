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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "rand.h"

#include "combat.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "unitlist.h"

#include "maphand.h"
#include "settlers.h"
#include "unithand.h"
#include "unittools.h"

#include "aidata.h"
#include "aitools.h"

#include "gotohand.h"

struct move_cost_map warmap;

/* 
 * These settings should be either true or false.  They are used for
 * finding routes for airplanes - the airplane doesn't want to fly
 * through occupied territory, but most territory will be either
 * fogged or unknown entirely.  See airspace_looks_safe().  Note that
 * this is currently only used by the move-counting function
 * air_can_move_between(), not by the path-finding function (whatever
 * that may be).
 */
#define AIR_ASSUMES_UNKNOWN_SAFE        TRUE
#define AIR_ASSUMES_FOGGED_SAFE         TRUE

static bool airspace_looks_safe(struct tile *ptile, struct player *pplayer);


/* These are used for all GOTO's */

/* A byte must be able to hold this value i.e. is must be less than
   256. */
#define MAXCOST 255
#define MAXARRAYS 10000
#define ARRAYLENGTH 10

struct mappos_array {
  int first_pos;
  int last_pos;
  struct tile *tile[ARRAYLENGTH];
  struct mappos_array *next_array;
};

struct array_pointer {
  struct mappos_array *first_array;
  struct mappos_array *last_array;
};

static struct mappos_array *mappos_arrays[MAXARRAYS];
static struct array_pointer cost_lookup[MAXCOST];
static int array_count;
static int lowest_cost;
static int highest_cost;


/*************************************************************************
 Used to check if the warmap distance to a position is "finite".
*************************************************************************/
bool is_dist_finite(int dist)
{

  return (dist < MAXCOST);
}

/**************************************************************************
...
**************************************************************************/
static void init_queue(void)
{
  int i;
  static bool is_initialized = FALSE;

  if (!is_initialized) {
    for (i = 0; i < MAXARRAYS; i++) {
      mappos_arrays[i] = NULL;
    }
    is_initialized = TRUE;
  }

  for (i = 0; i < MAXCOST; i++) {
    cost_lookup[i].first_array = NULL;
    cost_lookup[i].last_array = NULL;
  }
  array_count = 0;
  lowest_cost = 0;
  highest_cost = 0;
}

/**************************************************************************
  Free mapqueue arrays
**************************************************************************/
void free_mapqueue(void)
{
  int i;

  for (i = 0; i < MAXARRAYS; i++) {
    if (mappos_arrays[i] != NULL) {
      free(mappos_arrays[i]);
      mappos_arrays[i] = NULL;
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static struct mappos_array *get_empty_array(void)
{
  struct mappos_array *parray;

  if (!mappos_arrays[array_count]) {
    mappos_arrays[array_count]
      = fc_malloc(sizeof(*mappos_arrays[array_count]));
  }
  parray = mappos_arrays[array_count++];
  parray->first_pos = 0;
  parray->last_pos = -1;
  parray->next_array = NULL;
  return parray;
}

/**************************************************************************
...
**************************************************************************/
static void add_to_mapqueue(int cost, struct tile *ptile)
{
  struct mappos_array *our_array;

  assert(cost < MAXCOST && cost >= 0);

  our_array = cost_lookup[cost].last_array;
  if (!our_array) {
    our_array = get_empty_array();
    cost_lookup[cost].first_array = our_array;
    cost_lookup[cost].last_array = our_array;
  } else if (our_array->last_pos == ARRAYLENGTH-1) {
    our_array->next_array = get_empty_array();
    our_array = our_array->next_array;
    cost_lookup[cost].last_array = our_array;
  }

  our_array->tile[++(our_array->last_pos)] = ptile;
  if (cost > highest_cost)
    highest_cost = cost;
  freelog(LOG_DEBUG, "adding cost:%i at %i,%i", cost, ptile->x, ptile->y);
}

/**************************************************************************
...
**************************************************************************/
static struct tile *get_from_mapqueue(void)
{
  struct mappos_array *our_array;
  struct tile *ptile;

  freelog(LOG_DEBUG, "trying get");
  while (lowest_cost < MAXCOST) {
    if (lowest_cost > highest_cost)
      return FALSE;
    our_array = cost_lookup[lowest_cost].first_array;
    if (!our_array) {
      lowest_cost++;
      continue;
    }
    if (our_array->last_pos < our_array->first_pos) {
      if (our_array->next_array) {
	cost_lookup[lowest_cost].first_array = our_array->next_array;
	continue; /* note NOT "lowest_cost++;" */
      } else {
	cost_lookup[lowest_cost].first_array = NULL;
	lowest_cost++;
	continue;
      }
    }
    ptile = our_array->tile[our_array->first_pos];
    our_array->first_pos++;
    return ptile;
  }
  return NULL;
}

/**************************************************************************
Reset the movecosts of the warmap.
**************************************************************************/
static void init_warmap(struct tile *orig_tile, enum unit_move_type move_type)
{
  if (warmap.size != MAP_INDEX_SIZE) {
    warmap.cost = fc_realloc(warmap.cost,
			     MAP_INDEX_SIZE * sizeof(*warmap.cost));
    warmap.seacost = fc_realloc(warmap.seacost,
				MAP_INDEX_SIZE * sizeof(*warmap.seacost));
    warmap.vector = fc_realloc(warmap.vector,
			       MAP_INDEX_SIZE * sizeof(*warmap.vector));
    warmap.size = MAP_INDEX_SIZE;
  }

  init_queue();

  switch (move_type) {
  case LAND_MOVING:
  case BOTH_MOVING:
    assert(sizeof(*warmap.cost) == sizeof(char));
    memset(warmap.cost, MAXCOST, MAP_INDEX_SIZE * sizeof(char));
    warmap.cost[tile_index(orig_tile)] = 0;
    break;
  case SEA_MOVING:
    assert(sizeof(*warmap.seacost) == sizeof(char));
    memset(warmap.seacost, MAXCOST, MAP_INDEX_SIZE * sizeof(char));
    warmap.seacost[tile_index(orig_tile)] = 0;
    break;
  default:
    freelog(LOG_ERROR, "Bad move_type in init_warmap().");
  }
}  


/**************************************************************************
This creates a movecostmap (warmap), which is a two-dimentional array
corresponding to the real map. The value of a position is the number of
moves it would take to get there. If the function found no route the cost
is MAXCOST. (the value it is initialized with)
For sea units we let the map overlap onto the land one field, to allow
transporters and shore bombardment (ships can target the shore).
This map does NOT take enemy units onto account, nor ZOC.
Let generate_warmap interface to this function. Sometimes (in the AI when
using transports) this function will be called directly.

It is an search via Dijkstra's Algorithm
(see fx http://odin.ee.uwa.edu.au/~morris/Year2/PLDS210/dijkstra.html)
ie it piles tiles on a queue, and then use those tiles to find more tiles,
until all tiles we can reach are visited. To avoid making the warmap
potentially too big (heavy to calculate), the warmap is initialized with
a maxcost value, limiting the maximum length.

Note that this function is responsible for 20% of the CPU usage of freeciv...

This function is used by the AI in various cases when it is searching for
something to do with a unit. Maybe we could make a version that processed
the tiles in order after how many moves it took the unit to get to them,
and then gave them to the unit. This would achive 2 things:
-The unit could stop the function the instant it found what it was looking
for, or when it had already found something reasonably close and the distance
to the tiles later being searched was reasoable big. In both cases avoiding
to build the full warmap.
-The warmap would avoid examining a tile multiple times due to a series of
path with succesively smaller cost, since we would always have the smallest
possible cost the first time.
This would be done by inserting the tiles in a list after their move_cost
as they were found.
**************************************************************************/
static void really_generate_warmap(struct city *pcity, struct unit *punit,
                                   enum unit_move_type move_type)
{
  int move_cost;
  struct tile *orig_tile;
  bool igter, terrain_speed;
  int maxcost = THRESHOLD * 6 + 2; /* should be big enough without being TOO big */
  struct tile *ptile;
  struct player *pplayer;
  struct unit_class *pclass = NULL;
  int unit_speed = SINGLE_MOVE;

  if (pcity) {
    orig_tile = pcity->tile;
    pplayer = city_owner(pcity);
  } else {
    orig_tile = punit->tile;
    pplayer = unit_owner(punit);
  }

  init_warmap(orig_tile, move_type);
  add_to_mapqueue(0, orig_tile);

  igter = FALSE;
  if (punit) {
    pclass = unit_class(punit);

    terrain_speed = uclass_has_flag(unit_class(punit), UCF_TERRAIN_SPEED);
    if (!terrain_speed /* Igter meaningless without terrain_speed */
        && unit_has_type_flag(punit, F_IGTER)) {
      igter = TRUE;
    }
    unit_speed = unit_move_rate(punit);
  } else {
    /* Guess - can result in bad city warmaps with custom rulesets */
    if (move_type == LAND_MOVING || move_type == SEA_MOVING) {
      terrain_speed = TRUE;
    } else {
      terrain_speed = FALSE;
    }
  }

  /* FIXME: Should this apply only to F_CITIES units? -- jjm */
  if (punit
      && unit_has_type_flag(punit, F_SETTLERS)
      && unit_move_rate(punit)==3)
    maxcost /= 2;
  /* (?) was unit_type(punit) == U_SETTLERS -- dwp */

  while ((ptile = get_from_mapqueue())) {
    /* Just look up the cost value once.  This is a minor optimization but
     * it makes a big difference since this code is called so much. */
    unsigned char cost = ((move_type == SEA_MOVING)
			  ? WARMAP_SEACOST(ptile) : WARMAP_COST(ptile));

    adjc_dir_iterate(ptile, tile1, dir) {
      struct terrain *pterrain1 = tile_terrain(tile1);

      if ((move_type == LAND_MOVING && WARMAP_COST(tile1) <= cost)
          || (move_type == SEA_MOVING && WARMAP_SEACOST(tile1) <= cost)) {
        continue; /* No need for calculations */
      }
  
      if (!terrain_speed) {
        if (punit) {
          if (!can_unit_exist_at_tile(punit, tile1)) {

	    if (can_attack_non_native(unit_type(punit))
		&& is_native_tile(unit_type(punit), ptile)) {
	      /* Able to shore bombardment or similar activity,
	       * but not from city in non-native terrain. */

	      /* Attack cost is SINGLE_MOVE */
	      move_cost = SINGLE_MOVE;
	    } else if (unit_class_transporter_capacity(tile1, pplayer, pclass) > 0) {
	      /* Enters transport */
	      move_cost = SINGLE_MOVE;
	    } else {
	      /* Can't enter tile */
	      continue;
	    }
          } else {
            /* Punit can_exist_at_tile() */
            move_cost = SINGLE_MOVE;
          }
        } else {
          /* No punit. Crude guess */
          if ((move_type == SEA_MOVING && !is_ocean(pterrain1))
              || (move_type == LAND_MOVING && is_ocean(pterrain1))) {
            /* Can't enter tile */
            continue;
          }
          move_cost = SINGLE_MOVE;
        }
      } else {
  
        /* Speed determined by terrain */
        if (punit) {
          if (!can_unit_exist_at_tile(punit, tile1)) {
	    if (can_attack_non_native(unit_type(punit))
		&& is_native_tile(unit_type(punit), ptile)) {
	      /* Shore bombardment. Impossible if already in city
	       * in non-native terrain. */
	      move_cost = SINGLE_MOVE;
	      
	    } else if (unit_class_transporter_capacity(tile1, pplayer, pclass) > 0) {
	      /* Go on board */
              move_cost = SINGLE_MOVE;
            } else {
	      /* Can't enter tile */
              continue;
            }
          } else if (is_native_tile(unit_type(punit), tile1)) {
            /* can_exist_at_tile() */ 
            int base_cost = map_move_cost(ptile, tile1);
  
            base_cost = base_cost > unit_speed ? unit_speed : base_cost;
            move_cost = igter ? MIN(MOVE_COST_ROAD, base_cost) : base_cost;
          } else {
            /* can_exist_at_tile(), but !is_native_tile() -> entering port */
            move_cost = SINGLE_MOVE;
          }
        } else if ((move_type == LAND_MOVING && is_ocean(pterrain1))
                   || (move_type == SEA_MOVING && !is_ocean(pterrain1))) {
          continue;     /* City warmap ignores possible transports */
        } else {
          move_cost = map_move_cost(ptile, tile1);
        }
      }
  
      move_cost += cost;
 
      if (move_type != SEA_MOVING) {
        if (WARMAP_COST(tile1) > move_cost && move_cost < maxcost) {
          warmap.cost[tile_index(tile1)] = move_cost;
          add_to_mapqueue(move_cost, tile1);
        }
      } else if (move_type == SEA_MOVING) {
        if (WARMAP_SEACOST(tile1) > move_cost && move_cost < maxcost) {
          /* by adding the move_cost to the warmap regardless if we
           * can move between we allow for shore bombardment/transport
           * to inland positions/etc. */
          warmap.seacost[tile_index(tile1)] = move_cost;
          if ((punit && can_unit_exist_at_tile(punit, tile1))
              || (!punit && (is_ocean(pterrain1) || tile_city(tile1)))) {
            /* Unit can actually move to tile */
            add_to_mapqueue(move_cost, tile1);
  	  }
        }
      }
    } adjc_dir_iterate_end;
  }

  freelog(LOG_DEBUG, "Generated warmap for (%d,%d).",
	  TILE_XY(orig_tile)); 
}

/**************************************************************************
This is a wrapper for really_generate_warmap that checks if the warmap we
want is allready in existence. Also calls correctly depending on whether
pcity and/or punit is nun-NULL.
FIXME: Why is the movetype not used initialized on the warmap? Leaving it
for now.
**************************************************************************/
void generate_warmap(struct city *pcity, struct unit *punit)
{
  freelog(LOG_DEBUG, "Generating warmap, pcity = %s, punit = %s",
	  (pcity ? city_name(pcity) : "NULL"),
	  (punit ? unit_rule_name(punit) : "NULL"));

  if (punit) {
    /* 
     * If the previous warmap was for the same unit and it's still
     * correct (warmap.(sea)cost[x][y] == 0), reuse it.
     */
    if (warmap.warunit == punit &&
	(is_sailing_unit(punit) ? (WARMAP_SEACOST(punit->tile) == 0)
	 : (WARMAP_COST(punit->tile) == 0))) {
      return;
    }

    pcity = NULL;
  }

  warmap.warcity = pcity;
  warmap.warunit = punit;

  warmap.invalid = FALSE;

  if (punit) {
    if (is_sailing_unit(punit)) {
      really_generate_warmap(pcity, punit, SEA_MOVING);
    } else {
      really_generate_warmap(pcity, punit, LAND_MOVING);
    }
    warmap.orig_tile = punit->tile;
  } else {
    really_generate_warmap(pcity, punit, LAND_MOVING);
    really_generate_warmap(pcity, punit, SEA_MOVING);
    warmap.orig_tile = pcity->tile;
  }
}

/**************************************************************************
Return the direction that takes us most directly to dest_x,dest_y.
The direction is a value for use in DIR_DX[] and DIR_DY[] arrays.

FIXME: This is a bit crude; this only gives one correct path, but sometimes
       there can be more than one straight path (fx going NW then W is the
       same as going W then NW)
**************************************************************************/
static int straightest_direction(struct tile *src_tile,
				 struct tile *dst_tile)
{
  int best_dir;
  int diff_x, diff_y;

  /* Should we go up or down, east or west: diff_x/y is the "step" in x/y. */
  map_distance_vector(&diff_x, &diff_y, src_tile, dst_tile);

  if (diff_x == 0) {
    best_dir = (diff_y > 0) ? DIR8_SOUTH : DIR8_NORTH;
  } else if (diff_y == 0) {
    best_dir = (diff_x > 0) ? DIR8_EAST : DIR8_WEST;
  } else if (diff_x > 0) {
    best_dir = (diff_y > 0) ? DIR8_SOUTHEAST : DIR8_NORTHEAST;
  } else if (diff_x < 0) {
    best_dir = (diff_y > 0) ? DIR8_SOUTHWEST : DIR8_NORTHWEST;
  } else {
    assert(0);
    best_dir = 0;
  }

  return (best_dir);
}

/**************************************************************************
  Basic checks as to whether a GOTO is possible. The target (x,y) should
  be on the same continent as punit is, up to embarkation/disembarkation.
**************************************************************************/
bool goto_is_sane(struct unit *punit, struct tile *ptile, bool omni)
{  
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(ptile);
  Continent_id my_cont = tile_continent(punit->tile);
  Continent_id target_cont = tile_continent(ptile);

  if (same_pos(punit->tile, ptile)) {
    return TRUE;
  }

  if (!(omni || map_is_known_and_seen(ptile, pplayer, V_MAIN))) {
    /* The destination is in unknown -- assume sane */
    return TRUE;
  }

  switch (uclass_move_type(unit_class(punit))) {

  case LAND_MOVING:
    if (is_ocean_tile(ptile)) {
      /* Going to a sea tile, the target should be next to our continent 
       * and with a boat */
      if (unit_class_transporter_capacity(ptile, pplayer,
                                          unit_class(punit)) > 0) {
        adjc_iterate(ptile, tmp_tile) {
          if (tile_continent(tmp_tile) == my_cont) {
            /* The target is adjacent to our continent! */
            return TRUE;
          }
        } adjc_iterate_end;
      }
    } else {
      /* Going to a land tile: better be our continent */
      if (my_cont == target_cont) {
        return TRUE;
      } else {
        /* Well, it's not our continent, but maybe we are on a boat
         * adjacent to the target continent? */
	adjc_iterate(punit->tile, tmp_tile) {
	  if (tile_continent(tmp_tile) == target_cont) {
	    return TRUE;
          }
	} adjc_iterate_end;
      }
    }
      
    return FALSE;

  case SEA_MOVING:
    if (!is_ocean_tile(punit->tile)) {
      /* Oops, we are not in the open waters.  Pick an ocean that we have
       * access to.  We can assume we are in a city, and any oceans adjacent
       * are connected, so it does not matter which one we pick. */
      adjc_iterate(punit->tile, tmp_tile) {
        if (is_ocean_tile(tmp_tile)) {
          my_cont = tile_continent(tmp_tile);
          break;
        }
      } adjc_iterate_end;
    }
    if (is_ocean_tile(ptile)) {
      if (ai_channel(pplayer, target_cont, my_cont)) {
        return TRUE; /* Ocean -> Ocean travel ok. */
      }
    } else if ((pcity && pplayers_allied(city_owner(pcity), pplayer))
               || !unit_has_type_flag(punit, F_NO_LAND_ATTACK)) {
      /* Not ocean, but allied city or can bombard, checking if there is
       * good ocean adjacent */
      adjc_iterate(ptile, tmp_tile) {
        if (is_ocean_tile(tmp_tile)
            && ai_channel(pplayer, my_cont, tile_continent(tmp_tile))) {
          return TRUE;
        }
      } adjc_iterate_end;
    }
    return FALSE; /* Not ok. */

  default:
    return TRUE;
  }
}

/**************************************************************************
 Returns true if the airspace at given map position _looks_ safe to
 the given player. The airspace is unsafe if the player believes
 there is an enemy unit on it. This is tricky, since we have to
 consider what the player knows/doesn't know about the tile.
**************************************************************************/
static bool airspace_looks_safe(struct tile *ptile, struct player *pplayer)
{
  /* 
   * We do handicap checks for the player with ai_handicap(). This
   * function returns true if the player is handicapped. For human
   * players they'll always return true. This is the desired behavior.
   */

  /* If the tile's unknown, we (may) assume it's safe. */
  if (ai_handicap(pplayer, H_MAP) && !map_is_known(ptile, pplayer)) {
    return AIR_ASSUMES_UNKNOWN_SAFE;
  }

  /* This is bad: there could be a city there that the player doesn't
      know about.  How can we check that? */
  if (is_non_allied_city_tile(ptile, pplayer)) {
    return FALSE;
  }

  /* If the tile's fogged we again (may) assume it's safe. */
  if (ai_handicap(pplayer, H_FOG) &&
      !map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
    return AIR_ASSUMES_FOGGED_SAFE;
  }

  /* The tile is unfogged so we can check for enemy units on the
     tile. */
  return !is_non_allied_unit_tile(ptile, pplayer);
}

/**************************************************************************
 An air unit starts (src_x,src_y) with moves moves and want to go to
 (dest_x,dest_y). It returns the number of moves left if this is
 possible without running out of moves. It returns -1 if it is
 impossible.

  Note that the 'moves' passed to this function should be the number of
  steps the air unit has left.  The caller should *already* have
  divided by SINGLE_MOVE.

The function has 3 stages:
Try to rule out the possibility in O(1) time              else
Try to quickly verify in O(moves) time                    else
Do an A* search using the warmap to completely search for the path.
**************************************************************************/
int air_can_move_between(int moves, struct tile *src_tile,
			 struct tile *dest_tile, struct player *pplayer)
{
  struct tile *ptile;
  int dist, total_distance = real_map_distance(src_tile, dest_tile);

  freelog(LOG_DEBUG,
	  "air_can_move_between(moves=%d, src=(%i,%i), "
	  "dest=(%i,%i), player=%s)", moves, TILE_XY(src_tile),
	  TILE_XY(dest_tile), player_name(pplayer));

  /* First we do some very simple O(1) checks. */
  if (total_distance > moves) {
    return -1;
  }
  if (total_distance == 0) {
    assert(moves >= 0);
    return moves;
  }

  /* 
   * Then we do a fast O(n) straight-line check.  It'll work so long
   * as the straight-line doesn't cross any unreal tiles, unknown
   * tiles, or enemy-controlled tiles.  So, it should work most of the
   * time. 
   */
  ptile = src_tile;
  
  /* 
   * We don't check the endpoint of the goto, since it is possible
   * that the endpoint is a tile which has an enemy which should be
   * attacked. But we do check that all points in between are safe.
   */
  for (dist = total_distance; dist > 1; dist--) {
    /* Warning: straightest_direction may not actually follow the
       straight line. */
    int dir = straightest_direction(ptile, dest_tile);
    struct tile *new_tile;

    if (!(new_tile = mapstep(ptile, dir))
	|| !airspace_looks_safe(new_tile, pplayer)) {
      break;
    }
    ptile = new_tile;
  }
  if (dist == 1) {
    /* Looks like the O(n) quicksearch worked. */
    assert(real_map_distance(ptile, dest_tile) == 1);
    assert(moves - total_distance >= 0);
    return moves - total_distance;
  }

  /* 
   * Finally, we do a full A* search if this isn't one of the specical
   * cases from above. This is copied from find_the_shortest_path but
   * we use real_map_distance as a minimum distance estimator for the
   * A* search. This distance estimator is used for the cost value in
   * the queue, but is not stored in the warmap itself.
   *
   * Note, A* is possible here but not in a normal FreeCiv path
   * finding because planes always take 1 movement unit to move -
   * which is not true of land units.
   */
  freelog(LOG_DEBUG,
	  "air_can_move_between: quick search didn't work. Lets try full.");

  warmap.warunit = NULL;
  warmap.warcity = NULL;

  init_warmap(src_tile, BOTH_MOVING);

  /* The 0 is inaccurate under A*, but it doesn't matter. */
  add_to_mapqueue(0, src_tile);

  while ((ptile = get_from_mapqueue())) {
    adjc_dir_iterate(ptile, tile1, dir) {
      if (WARMAP_COST(tile1) != MAXCOST) {
	continue;
      }

      /*
       * This comes before the airspace_looks_safe check because it's
       * okay to goto into an enemy. 
       */
      if (same_pos(tile1, dest_tile)) {
	/* We're there! */
	freelog(LOG_DEBUG, "air_can_move_between: movecost: %i",
		WARMAP_COST(ptile) + 1);
	/* The -1 is because we haven't taken the final
	   step yet. */
	assert(moves - WARMAP_COST(ptile) - 1 >= 0);
	return moves - WARMAP_COST(ptile) - 1;
      }

      /* We refuse to goto through unsafe airspace. */
      if (airspace_looks_safe(tile1, pplayer)) {
	int cost = WARMAP_COST(ptile) + 1;

	warmap.cost[tile_index(tile1)] = cost;

	/* Now for A* we find the minimum total cost. */
	cost += real_map_distance(tile1, dest_tile);
	if (cost <= moves) {
	  add_to_mapqueue(cost, tile1);
	}
      }
    } adjc_dir_iterate_end;
  }

  freelog(LOG_DEBUG, "air_can_move_between: no route found");
  return -1;
}
