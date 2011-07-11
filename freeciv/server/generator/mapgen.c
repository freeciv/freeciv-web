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
#include <time.h>

#include "fcintl.h"
#include "log.h"
#include "map.h"
#include "maphand.h" /* assign_continent_numbers(), MAP_NCONT */
#include "mem.h"
#include "rand.h"
#include "shared.h"

#include "height_map.h"
#include "mapgen.h"
#include "mapgen_topology.h"
#include "startpos.h"
#include "temperature_map.h"
#include "utilities.h"


/* Wrappers for easy access.  They are a macros so they can be a lvalues.*/
#define rmap(ptile) (river_map[tile_index(ptile)])

static void make_huts(int number);
static void add_resources(int prob);
static void mapgenerator2(void);
static void mapgenerator3(void);
static void mapgenerator4(void);
static void adjust_terrain_param(void);

#define RIVERS_MAXTRIES 32767
enum river_map_type {RS_BLOCKED = 0, RS_RIVER = 1};

/* Array needed to mark tiles as blocked to prevent a river from
   falling into itself, and for storing rivers temporarly.
   A value of 1 means blocked.
   A value of 2 means river.                            -Erik Sigra */
static int *river_map;

#define HAS_POLES (map.server.temperature < 70 && !map.server.alltemperate)

/* These are the old parameters of terrains types in %
   TODO: they depend on the hardcoded terrains */
static int forest_pct = 0;
static int desert_pct = 0;
static int swamp_pct = 0;
static int mountain_pct = 0;
static int jungle_pct = 0;
static int river_pct = 0;
 
/****************************************************************************
 * Conditions used mainly in rand_map_pos_characteristic()
 ****************************************************************************/
/* WETNESS */

/* necessary condition of deserts placement */
#define map_pos_is_dry(ptile)						\
  (map_colatitude((ptile)) <= DRY_MAX_LEVEL				\
   && map_colatitude((ptile)) > DRY_MIN_LEVEL				\
   && count_ocean_near_tile((ptile), FALSE, TRUE) <= 35)
typedef enum { WC_ALL = 200, WC_DRY, WC_NDRY } wetness_c;

/* MISCELANEOUS (OTHER CONDITIONS) */

/* necessary condition of swamp placement */
static int hmap_low_level = 0;
#define ini_hmap_low_level() \
{ \
hmap_low_level = (4 * swamp_pct  * \
     (hmap_max_level - hmap_shore_level)) / 100 + hmap_shore_level; \
}
/* should be used after having hmap_low_level initialized */
#define map_pos_is_low(ptile) ((hmap((ptile)) < hmap_low_level))

typedef enum { MC_NONE, MC_LOW, MC_NLOW } miscellaneous_c;

/***************************************************************************
 These functions test for conditions used in rand_map_pos_characteristic 
***************************************************************************/

/***************************************************************************
  Checks if the given location satisfy some wetness condition
***************************************************************************/
static bool test_wetness(const struct tile *ptile, wetness_c c)
{
  switch(c) {
  case WC_ALL:
    return TRUE;
  case WC_DRY:
    return map_pos_is_dry(ptile);
  case WC_NDRY:
    return !map_pos_is_dry(ptile);
  }
  assert(0);
  return FALSE;
}

/***************************************************************************
  Checks if the given location satisfy some miscellaneous condition
***************************************************************************/
static bool test_miscellaneous(const struct tile *ptile, miscellaneous_c c)
{
  switch(c) {
  case MC_NONE:
    return TRUE;
  case MC_LOW:
    return map_pos_is_low(ptile);
  case MC_NLOW:
    return !map_pos_is_low(ptile);
  }
  assert(0);
  return FALSE;
}

/***************************************************************************
  Passed as data to rand_map_pos_filtered() by rand_map_pos_characteristic()
***************************************************************************/
struct DataFilter {
  wetness_c wc;
  temperature_type tc;
  miscellaneous_c mc;
};

/****************************************************************************
  A filter function to be passed to rand_map_pos_filtered().  See
  rand_map_pos_characteristic for more explanation.
****************************************************************************/
static bool condition_filter(const struct tile *ptile, const void *data)
{
  const struct DataFilter *filter = data;

  return  not_placed(ptile) 
       && tmap_is(ptile, filter->tc) 
       && test_wetness(ptile, filter->wc) 
       && test_miscellaneous(ptile, filter->mc) ;
}

/****************************************************************************
  Return random map coordinates which have some conditions and which are
  not yet placed on pmap.
  Returns FALSE if there is no such position.
****************************************************************************/
static struct tile *rand_map_pos_characteristic(wetness_c wc,
						temperature_type tc,
						miscellaneous_c mc )
{
  struct DataFilter filter;

  filter.wc = wc;
  filter.tc = tc;
  filter.mc = mc;
  return rand_map_pos_filtered(&filter, condition_filter);
}

/**************************************************************************
  we don't want huge areas of grass/plains, 
  so we put in a hill here and there, where it gets too 'clean' 

  Return TRUE if the terrain at the given map position is "clean".  This
  means that all the terrain for 2 squares around it is not mountain or hill.
****************************************************************************/
static bool terrain_is_too_flat(struct tile *ptile, 
				int thill, int my_height)
{
  int higher_than_me = 0;
  square_iterate(ptile, 2, tile1) {
    if (hmap(tile1) > thill) {
      return FALSE;
    }
    if (hmap(tile1) > my_height) {
      if (map_distance(ptile, tile1) == 1) {
	return FALSE;
      }
      if (++higher_than_me > 2) {
	return FALSE;
      }
    }
  } square_iterate_end;

  if ((thill - hmap_shore_level) * higher_than_me  > 
      (my_height - hmap_shore_level) * 4) {
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  we don't want huge areas of hill/mountains, 
  so we put in a plains here and there, where it gets too 'heigh' 

  Return TRUE if the terrain at the given map position is too heigh.
****************************************************************************/
static bool terrain_is_too_high(struct tile *ptile,
				int thill, int my_height)
{
  square_iterate(ptile, 1, tile1) {
    if (hmap(tile1) + (hmap_max_level - hmap_mountain_level) / 5 < thill) {
      return FALSE;
    }
  } square_iterate_end;
  return TRUE;
}

/****************************************************************************
  Return a random terrain that has the specified flag.
  Returns T_UNKNOWN when there is no matching terrain.
****************************************************************************/
static struct terrain *pick_terrain_by_flag(enum terrain_flag_id flag)
{
  bool has_flag[terrain_count()];
  int count = 0;

  terrain_type_iterate(pterrain) {
    if ((has_flag[terrain_index(pterrain)] = terrain_has_flag(pterrain, flag))) {
      count++;
    }
  } terrain_type_iterate_end;

  count = myrand(count);
  terrain_type_iterate(pterrain) {
    if (has_flag[terrain_index(pterrain)]) {
      if (count == 0) {
	return pterrain;
      }
      count--;
    }
  } terrain_type_iterate_end;
#if 0
  die("Reached end of pick_terrain_by_flag!");
#endif
  return T_UNKNOWN;
}

/****************************************************************************
  Pick a terrain based on the target property and a property to avoid.

  If the target property is given, then all terrains with that property
  will be considered and one will be picked at random based on the amount
  of the property each terrain has.  If no target property is given all
  terrains will be assigned equal likelihood.

  If the preferred property is given, only terrains with (some of) that
  property will be chosen.

  If the avoid property is given, then any terrain with (any of) that
  property will be avoided.

  This function must always return a valid terrain.
****************************************************************************/
static struct terrain *pick_terrain(enum mapgen_terrain_property target,
				    enum mapgen_terrain_property prefer,
				    enum mapgen_terrain_property avoid)
{
  int sum = 0;

  /* Find the total weight. */
  terrain_type_iterate(pterrain) {
    if (avoid != MG_LAST && pterrain->property[avoid] > 0) {
      continue;
    }
    if (prefer != MG_LAST && pterrain->property[prefer] == 0) {
      continue;
    }

    if (target != MG_LAST) {
      sum += pterrain->property[target];
    } else {
      sum++;
    }
  } terrain_type_iterate_end;

  /* Now pick. */
  sum = myrand(sum);

  /* Finally figure out which one we picked. */
  terrain_type_iterate(pterrain) {
    int property;

    if (avoid != MG_LAST && pterrain->property[avoid] > 0) {
      continue;
    }
    if (prefer != MG_LAST && pterrain->property[prefer] == 0) {
      continue;
    }

    if (target != MG_LAST) {
      property = pterrain->property[target];
    } else {
      property = 1;
    }
    if (sum < property) {
      return pterrain;
    }
    sum -= property;
  } terrain_type_iterate_end;

  /* This can happen with sufficient quantities of preferred and avoided
   * characteristics.  Drop a requirement and try again. */
  if (prefer != MG_LAST) {
    return pick_terrain(target, MG_LAST, avoid);
  } else if (avoid != MG_LAST) {
    return pick_terrain(target, prefer, MG_LAST);
  } else {
    return pick_terrain(MG_LAST, prefer, avoid);
  }
}

/**************************************************************************
  make_relief() will convert all squares that are higher than thill to
  mountains and hills. Note that thill will be adjusted according to
  the map.server.steepness value, so increasing map.mountains will result
  in more hills and mountains.
**************************************************************************/
static void make_relief(void)
{
  /* Calculate the mountain level.  map.server.mountains specifies the
   * percentage of land that is turned into hills and mountains. */
  hmap_mountain_level = (((hmap_max_level - hmap_shore_level)
                          * (100 - map.server.steepness))
                         / 100 + hmap_shore_level);

  whole_map_iterate(ptile) {
    if (not_placed(ptile) &&
	((hmap_mountain_level < hmap(ptile) && 
	  (myrand(10) > 5 
	   || !terrain_is_too_high(ptile, hmap_mountain_level, hmap(ptile))))
	 || terrain_is_too_flat(ptile, hmap_mountain_level, hmap(ptile)))) {
      struct terrain *pterrain
	= pick_terrain(MG_MOUNTAINOUS, MG_LAST,
		       (tmap_is(ptile, TT_NHOT) ? MG_GREEN : MG_LAST));

      tile_set_terrain(ptile, pterrain);
      map_set_placed(ptile);
    }
  } whole_map_iterate_end;
}

/****************************************************************************
  Add arctic and tundra squares in the arctic zone (that is, the coolest
  10% of the map).  We also texture the pole (adding arctic, tundra, and
  mountains).  This is used in generators 2-4.
****************************************************************************/
static void make_polar(void)
{
  whole_map_iterate(ptile) {  
    if (tmap_is(ptile, TT_FROZEN)
	|| (tmap_is(ptile, TT_COLD)
	    && (myrand(10) > 7)
	    && is_temperature_type_near(ptile, TT_FROZEN))) { 
      tile_set_terrain(ptile, pick_terrain(MG_FROZEN, MG_LAST, MG_LAST));
    }
  } whole_map_iterate_end;
}

/*************************************************************************
 if separatepoles is set, return false if this tile has to keep ocean
************************************************************************/
static bool ok_for_separate_poles(struct tile *ptile)
{
  if (!map.server.separatepoles) {
    return TRUE;
  }
  adjc_iterate(ptile, tile1) {
    if (tile_continent(tile1) > 0) {
      return FALSE;
    }
  } adjc_iterate_end;
  return TRUE;
}

/****************************************************************************
  Place untextured land at the poles.  This is used by generators 1 and 5.
****************************************************************************/
static void make_polar_land(void)
{
  assign_continent_numbers();
  whole_map_iterate(ptile) {
    if ((tmap_is(ptile, TT_FROZEN ) &&
	ok_for_separate_poles(ptile))
	||
	(tmap_is(ptile, TT_COLD ) &&
	 (myrand(10) > 7) &&
	 is_temperature_type_near(ptile, TT_FROZEN) &&
	 ok_for_separate_poles(ptile))) {
      tile_set_terrain(ptile, T_UNKNOWN);
      tile_set_continent(ptile, 0);
    } 
  } whole_map_iterate_end;
}

/**************************************************************************
  Recursively generate terrains.
**************************************************************************/
static void place_terrain(struct tile *ptile, int diff, 
                           struct terrain *pterrain, int *to_be_placed,
			   wetness_c        wc,
			   temperature_type tc,
			   miscellaneous_c  mc)
{
  if (*to_be_placed <= 0) {
    return;
  }
  assert(not_placed(ptile));
  tile_set_terrain(ptile, pterrain);
  map_set_placed(ptile);
  (*to_be_placed)--;
  
  cardinal_adjc_iterate(ptile, tile1) {
    int Delta = abs(map_colatitude(tile1) - map_colatitude(ptile)) / L_UNIT
	+ abs(hmap(tile1) - (hmap(ptile))) /  H_UNIT;
    if (not_placed(tile1) 
	&& tmap_is(tile1, tc) 
	&& test_wetness(tile1, wc)
 	&& test_miscellaneous(tile1, mc)
	&& Delta < diff 
	&& myrand(10) > 4) {
	place_terrain(tile1, diff - 1 - Delta, pterrain,
		      to_be_placed, wc, tc, mc);
    }
  } cardinal_adjc_iterate_end;
}

/**************************************************************************
  a simple function that adds plains grassland or tundra to the 
  current location.
**************************************************************************/
static void make_plain(struct tile *ptile, int *to_be_placed )
{
  /* in cold place we get tundra instead */
  if (tmap_is(ptile, TT_FROZEN)) {
    tile_set_terrain(ptile,
		     pick_terrain(MG_FROZEN, MG_LAST, MG_MOUNTAINOUS));
  } else if (tmap_is(ptile, TT_COLD)) {
    tile_set_terrain(ptile,
		     pick_terrain(MG_COLD, MG_LAST, MG_MOUNTAINOUS)); 
  } else {
    tile_set_terrain(ptile,
		     pick_terrain(MG_TEMPERATE, MG_GREEN, MG_MOUNTAINOUS));
  }
  map_set_placed(ptile);
  (*to_be_placed)--;
}

/**************************************************************************
  make_plains converts all not yet placed terrains to plains (tundra, grass) 
  used by generators 2-4
**************************************************************************/
static void make_plains(void)
{
  int to_be_placed;
  whole_map_iterate(ptile) {
    if (not_placed(ptile)) {
      to_be_placed = 1;
      make_plain(ptile, &to_be_placed);
    }
  } whole_map_iterate_end;
}
/**************************************************************************
 This place randomly a cluster of terrains with some characteristics
 **************************************************************************/
#define PLACE_ONE_TYPE(count, alternate, ter, wc, tc, mc, weight) \
  if((count) > 0) {                                       \
    struct tile *ptile;					  \
    /* Place some terrains */                             \
    if ((ptile = rand_map_pos_characteristic((wc), (tc), (mc)))) {	\
      place_terrain(ptile, (weight), (ter), &(count), (wc),(tc), (mc));   \
    } else {                                                             \
      /* If rand_map_pos_temperature returns FALSE we may as well stop*/ \
      /* looking for this time and go to alternate type. */              \
      (alternate) += (count); \
      (count) = 0;            \
    }                         \
  }

/**************************************************************************
  make_terrains calls make_forest, make_dessert,etc  with random free 
  locations until there  has been made enough.
 Comment: funtions as make_swamp, etc. has to have a non 0 probability
 to place one terrains in called position. Else make_terrains will get
 in a infinite loop!
**************************************************************************/
static void make_terrains(void)
{
  int total = 0;
  int forests_count = 0;
  int jungles_count = 0;
  int deserts_count = 0;
  int alt_deserts_count = 0;
  int plains_count = 0;
  int swamps_count = 0;

  whole_map_iterate(ptile) {
    if (not_placed(ptile)) {
     total++;
    }
  } whole_map_iterate_end;

  forests_count = total * forest_pct / (100 - mountain_pct);
  jungles_count = total * jungle_pct / (100 - mountain_pct);
 
  deserts_count = total * desert_pct / (100 - mountain_pct); 
  swamps_count = total * swamp_pct  / (100 - mountain_pct);

  /* grassland, tundra,arctic and plains is counted in plains_count */
  plains_count = total - forests_count - deserts_count
      - swamps_count - jungles_count;

  /* the placement loop */
  do {
    
    PLACE_ONE_TYPE(forests_count , plains_count,
		   pick_terrain(MG_FOLIAGE, MG_TEMPERATE, MG_LAST),
		   WC_ALL, TT_NFROZEN, MC_NONE, 60);
    PLACE_ONE_TYPE(jungles_count, forests_count,
		   pick_terrain(MG_FOLIAGE, MG_TROPICAL, MG_COLD),
		   WC_ALL, TT_TROPICAL, MC_NONE, 50);
    PLACE_ONE_TYPE(swamps_count, forests_count,
		   pick_terrain(MG_FOLIAGE, MG_WET, MG_TROPICAL),
		   WC_NDRY, TT_HOT, MC_LOW, 50);
    PLACE_ONE_TYPE(deserts_count, alt_deserts_count,
		   pick_terrain(MG_DRY, MG_TROPICAL, MG_COLD),
		   WC_DRY, TT_NFROZEN, MC_NLOW, 80);
    PLACE_ONE_TYPE(alt_deserts_count, plains_count,
		   pick_terrain(MG_DRY, MG_TROPICAL, MG_COLD),
		   WC_ALL, TT_NFROZEN, MC_NLOW, 40);
 
  /* make the plains and tundras */
    if (plains_count > 0) {
      struct tile *ptile;

      /* Don't use any restriction here ! */
      if ((ptile = rand_map_pos_characteristic(WC_ALL, TT_ALL, MC_NONE))) {
	make_plain(ptile, &plains_count);
      } else {
	/* If rand_map_pos_temperature returns FALSE we may as well stop
	 * looking for plains. */
	plains_count = 0;
      }
    }
  } while (forests_count > 0 || jungles_count > 0 
	   || deserts_count > 0 || alt_deserts_count > 0 
	   || plains_count > 0 || swamps_count > 0 );
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_blocked(struct tile *ptile)
{
  if (TEST_BIT(rmap(ptile), RS_BLOCKED))
    return 1;

  /* any un-blocked? */
  cardinal_adjc_iterate(ptile, tile1) {
    if (!TEST_BIT(rmap(tile1), RS_BLOCKED))
      return 0;
  } cardinal_adjc_iterate_end;

  return 1; /* none non-blocked |- all blocked */
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_rivergrid(struct tile *ptile)
{
  return (count_special_near_tile(ptile, TRUE, FALSE, S_RIVER) > 1) ? 1 : 0;
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_highlands(struct tile *ptile)
{
  return tile_terrain(ptile)->property[MG_MOUNTAINOUS];
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_adjacent_ocean(struct tile *ptile)
{
  return 100 - count_ocean_near_tile(ptile, TRUE, TRUE);
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_adjacent_river(struct tile *ptile)
{
  return 100 - count_special_near_tile(ptile, TRUE, TRUE, S_RIVER);
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_adjacent_highlands(struct tile *ptile)
{
  int sum = 0;

  adjc_iterate(ptile, ptile2) {
    sum += tile_terrain(ptile2)->property[MG_MOUNTAINOUS];
  } adjc_iterate_end;

  return sum;
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_swamp(struct tile *ptile)
{
  return FC_INFINITY - tile_terrain(ptile)->property[MG_WET];
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_adjacent_swamp(struct tile *ptile)
{
  int sum = 0;

  adjc_iterate(ptile, ptile2) {
    sum += tile_terrain(ptile2)->property[MG_WET];
  } adjc_iterate_end;

  return FC_INFINITY - sum;
}

/*********************************************************************
 Help function used in make_river(). See the help there.
*********************************************************************/
static int river_test_height_map(struct tile *ptile)
{
  return hmap(ptile);
}

/*********************************************************************
 Called from make_river. Marks all directions as blocked.  -Erik Sigra
*********************************************************************/
static void river_blockmark(struct tile *ptile)
{
  freelog(LOG_DEBUG, "Blockmarking (%d, %d) and adjacent tiles.",
	  ptile->x, ptile->y);

  rmap(ptile) |= (1u << RS_BLOCKED);

  cardinal_adjc_iterate(ptile, tile1) {
    rmap(tile1) |= (1u << RS_BLOCKED);
  } cardinal_adjc_iterate_end;
}

struct test_func {
  int (*func)(struct tile *ptile);
  bool fatal;
};

#define NUM_TEST_FUNCTIONS 9
static struct test_func test_funcs[NUM_TEST_FUNCTIONS] = {
  {river_test_blocked,            TRUE},
  {river_test_rivergrid,          TRUE},
  {river_test_highlands,          FALSE},
  {river_test_adjacent_ocean,     FALSE},
  {river_test_adjacent_river,     FALSE},
  {river_test_adjacent_highlands, FALSE},
  {river_test_swamp,              FALSE},
  {river_test_adjacent_swamp,     FALSE},
  {river_test_height_map,         FALSE}
};

/********************************************************************
 Makes a river starting at (x, y). Returns 1 if it succeeds.
 Return 0 if it fails. The river is stored in river_map.
 
 How to make a river path look natural
 =====================================
 Rivers always flow down. Thus rivers are best implemented on maps
 where every tile has an explicit height value. However, Freeciv has a
 flat map. But there are certain things that help the user imagine
 differences in height between tiles. The selection of direction for
 rivers should confirm and even amplify the user's image of the map's
 topology.
 
 To decide which direction the river takes, the possible directions
 are tested in a series of test until there is only 1 direction
 left. Some tests are fatal. This means that they can sort away all
 remaining directions. If they do so, the river is aborted. Here
 follows a description of the test series.
 
 * Falling into itself: fatal
     (river_test_blocked)
     This is tested by looking up in the river_map array if a tile or
     every tile surrounding the tile is marked as blocked. A tile is
     marked as blocked if it belongs to the current river or has been
     evaluated in a previous iteration in the creation of the current
     river.
     
     Possible values:
     0: Is not falling into itself.
     1: Is falling into itself.
     
 * Forming a 4-river-grid: optionally fatal
     (river_test_rivergrid)
     A minimal 4-river-grid is formed when an intersection in the map
     grid is surrounded by 4 river tiles. There can be larger river
     grids consisting of several overlapping minimal 4-river-grids.
     
     Possible values:
     0: Is not forming a 4-river-grid.
     1: Is forming a 4-river-grid.

 * Highlands:
     (river_test_highlands)
     Rivers must not flow up in mountains or hills if there are
     alternatives.
     
     Possible values:
     0: Is not hills and not mountains.
     1: Is hills.
     2: Is mountains.

 * Adjacent ocean:
     (river_test_adjacent_ocean)
     Rivers must flow down to coastal areas when possible:

     Possible values: 0-100

 * Adjacent river:
     (river_test_adjacent_river)
     Rivers must flow down to areas near other rivers when possible:

     Possible values: 0-100
					
 * Adjacent highlands:
     (river_test_adjacent_highlands)
     Rivers must not flow towards highlands if there are alternatives. 
     
 * Swamps:
     (river_test_swamp)
     Rivers must flow down in swamps when possible.
     
     Possible values:
     0: Is swamps.
     1: Is not swamps.
     
 * Adjacent swamps:
     (river_test_adjacent_swamp)
     Rivers must flow towards swamps when possible.

 * height_map:
     (river_test_height_map)
     Rivers must flow in the direction which takes it to the tile with
     the lowest value on the height_map.
     
     Possible values:
     n: height_map[...]
     
 If these rules haven't decided the direction, the random number
 generator gets the desicion.                              -Erik Sigra
*********************************************************************/
static bool make_river(struct tile *ptile)
{
  /* Comparison value for each tile surrounding the current tile.  It is
   * the suitability to continue a river to the tile in that direction;
   * lower is better.  However rivers may only run in cardinal directions;
   * the other directions are ignored entirely. */
  int rd_comparison_val[8];

  bool rd_direction_is_valid[8];
  int num_valid_directions, func_num, direction;

  while (TRUE) {
    /* Mark the current tile as river. */
    rmap(ptile) |= (1u << RS_RIVER);
    freelog(LOG_DEBUG,
	    "The tile at (%d, %d) has been marked as river in river_map.\n",
	    ptile->x, ptile->y);

    /* Test if the river is done. */
    /* We arbitrarily make rivers end at the poles. */
    if (count_special_near_tile(ptile, TRUE, TRUE, S_RIVER) > 0
	|| count_ocean_near_tile(ptile, TRUE, TRUE) > 0
        || (tile_terrain(ptile)->property[MG_FROZEN] > 0
	    && map_colatitude(ptile) < 0.8 * COLD_LEVEL)) { 

      freelog(LOG_DEBUG,
	      "The river ended at (%d, %d).\n", ptile->x, ptile->y);
      return TRUE;
    }

    /* Else choose a direction to continue the river. */
    freelog(LOG_DEBUG,
	    "The river did not end at (%d, %d). Evaluating directions...\n",
	    ptile->x, ptile->y);

    /* Mark all available cardinal directions as available. */
    memset(rd_direction_is_valid, 0, sizeof(rd_direction_is_valid));
    cardinal_adjc_dir_iterate(ptile, tile1, dir) {
      rd_direction_is_valid[dir] = TRUE;
    } cardinal_adjc_dir_iterate_end;

    /* Test series that selects a direction for the river. */
    for (func_num = 0; func_num < NUM_TEST_FUNCTIONS; func_num++) {
      int best_val = -1;

      /* first get the tile values for the function */
      cardinal_adjc_dir_iterate(ptile, tile1, dir) {
	if (rd_direction_is_valid[dir]) {
	  rd_comparison_val[dir] = (test_funcs[func_num].func) (tile1);
	  assert(rd_comparison_val[dir] >= 0);
	  if (best_val == -1) {
	    best_val = rd_comparison_val[dir];
	  } else {
	    best_val = MIN(rd_comparison_val[dir], best_val);
	  }
	}
      } cardinal_adjc_dir_iterate_end;
      assert(best_val != -1);

      /* should we abort? */
      if (best_val > 0 && test_funcs[func_num].fatal) {
	return FALSE;
      }

      /* mark the less attractive directions as invalid */
      cardinal_adjc_dir_iterate(ptile, tile1, dir) {
	if (rd_direction_is_valid[dir]) {
	  if (rd_comparison_val[dir] != best_val) {
	    rd_direction_is_valid[dir] = FALSE;
	  }
	}
      } cardinal_adjc_dir_iterate_end;
    }

    /* Directions evaluated with all functions. Now choose the best
       direction before going to the next iteration of the while loop */
    num_valid_directions = 0;
    cardinal_adjc_dir_iterate(ptile, tile1, dir) {
      if (rd_direction_is_valid[dir]) {
	num_valid_directions++;
      }
    } cardinal_adjc_dir_iterate_end;

    if (num_valid_directions == 0) {
      return FALSE; /* river aborted */
    }

    /* One or more valid directions: choose randomly. */
    freelog(LOG_DEBUG, "mapgen.c: Had to let the random number"
	    " generator select a direction for a river.");
    direction = myrand(num_valid_directions);
    freelog(LOG_DEBUG, "mapgen.c: direction: %d", direction);

    /* Find the direction that the random number generator selected. */
    cardinal_adjc_dir_iterate(ptile, tile1, dir) {
      if (rd_direction_is_valid[dir]) {
	if (direction > 0) {
	  direction--;
	} else {
	  river_blockmark(ptile);
	  ptile = tile1;
	  break;
	}
      }
    } cardinal_adjc_dir_iterate_end;
    assert(direction == 0);

  } /* end while; (Make a river.) */
}

/**************************************************************************
  Calls make_river until there are enough river tiles on the map. It stops
  when it has tried to create RIVERS_MAXTRIES rivers.           -Erik Sigra
**************************************************************************/
static void make_rivers(void)
{
  struct tile *ptile;
  struct terrain *pterrain;

  /* Formula to make the river density similar om different sized maps. Avoids
     too few rivers on large maps and too many rivers on small maps. */
  int desirable_riverlength =
    river_pct *
      /* The size of the map (poles counted in river_pct). */
      map_num_tiles() *
      /* Rivers need to be on land only. */
      map.server.landpercent /
      /* Adjustment value. Tested by me. Gives no rivers with 'set
	 rivers 0', gives a reasonable amount of rivers with default
	 settings and as many rivers as possible with 'set rivers 100'. */
      5325;

  /* The number of river tiles that have been set. */
  int current_riverlength = 0;

  /* Counts the number of iterations (should increase with 1 during
     every iteration of the main loop in this function).
     Is needed to stop a potentially infinite loop. */
  int iteration_counter = 0;

  create_placed_map(); /* needed bu rand_map_characteristic */
  set_all_ocean_tiles_placed();

  river_map = fc_malloc(sizeof(*river_map) * MAP_INDEX_SIZE);

  /* The main loop in this function. */
  while (current_riverlength < desirable_riverlength
	 && iteration_counter < RIVERS_MAXTRIES) {

    if (!(ptile = rand_map_pos_characteristic(WC_ALL, TT_NFROZEN,
					      MC_NLOW))) {
	break; /* mo more spring places */
    }
    pterrain = tile_terrain(ptile);

    /* Check if it is suitable to start a river on the current tile.
     */
    if (
	/* Don't start a river on ocean. */
	!is_ocean(pterrain)

	/* Don't start a river on river. */
	&& !tile_has_special(ptile, S_RIVER)

	/* Don't start a river on a tile is surrounded by > 1 river +
	   ocean tile. */
	&& (count_special_near_tile(ptile, TRUE, FALSE, S_RIVER)
	    + count_ocean_near_tile(ptile, TRUE, FALSE) <= 1)

	/* Don't start a river on a tile that is surrounded by hills or
	   mountains unless it is hard to find somewhere else to start
	   it. */
	&& (count_terrain_property_near_tile(ptile, TRUE, TRUE,
					     MG_MOUNTAINOUS) < 90
	    || iteration_counter >= RIVERS_MAXTRIES / 10 * 5)

	/* Don't start a river on hills unless it is hard to find
	   somewhere else to start it. */
	&& (pterrain->property[MG_MOUNTAINOUS] == 0
	    || iteration_counter >= RIVERS_MAXTRIES / 10 * 6)

	/* Don't start a river on arctic unless it is hard to find
	   somewhere else to start it. */
	&& (pterrain->property[MG_FROZEN] == 0
	    || iteration_counter >= RIVERS_MAXTRIES / 10 * 8)

	/* Don't start a river on desert unless it is hard to find
	   somewhere else to start it. */
	&& (pterrain->property[MG_DRY] == 0
	    || iteration_counter >= RIVERS_MAXTRIES / 10 * 9)) {

      /* Reset river_map before making a new river. */
      memset(river_map, 0, MAP_INDEX_SIZE * sizeof(*river_map));

      freelog(LOG_DEBUG,
	      "Found a suitable starting tile for a river at (%d, %d)."
	      " Starting to make it.",
	      ptile->x, ptile->y);

      /* Try to make a river. If it is OK, apply it to the map. */
      if (make_river(ptile)) {
	whole_map_iterate(tile1) {
	  if (TEST_BIT(rmap(tile1), RS_RIVER)) {
	    struct terrain *pterrain = tile_terrain(tile1);

	    if (!terrain_has_flag(pterrain, TER_CAN_HAVE_RIVER)) {
	      /* We have to change the terrain to put a river here. */
	      pterrain = pick_terrain_by_flag(TER_CAN_HAVE_RIVER);
	      if (pterrain) {
		tile_set_terrain(tile1, pterrain);
	      }
	    }
	    tile_set_special(tile1, S_RIVER);
	    current_riverlength++;
	    map_set_placed(tile1);
	    freelog(LOG_DEBUG, "Applied a river to (%d, %d).",
		    tile1->x, tile1->y);
	  }
	} whole_map_iterate_end;
      } else {
	freelog(LOG_DEBUG,
		"mapgen.c: A river failed. It might have gotten stuck in a helix.");
      }
    } /* end if; */
    iteration_counter++;
    freelog(LOG_DEBUG,
	    "current_riverlength: %d; desirable_riverlength: %d; iteration_counter: %d",
	    current_riverlength, desirable_riverlength, iteration_counter);
  } /* end while; */
  free(river_map);
  destroy_placed_map();
  river_map = NULL;
}

/**************************************************************************
  make land simply does it all based on a generated heightmap
  1) with map.server.landpercent it generates a ocean/unknown map
  2) it then calls the above functions to generate the different terrains
**************************************************************************/
static void make_land(void)
{
  struct terrain *land_fill = NULL;

  if (HAS_POLES) {
    normalize_hmap_poles();
  }

  /* Pick a non-ocean terrain just once and fill all land tiles with "
   * that terrain. We must set some terrain (and not T_UNKNOWN) so that "
   * continent number assignment works. */
  terrain_type_iterate(pterrain) {
    if (!is_ocean(pterrain)) {
      land_fill = pterrain;
      break;
    }
  } terrain_type_iterate_end;
  if (land_fill == NULL) {
    freelog(LOG_FATAL, "No land terrain type could be found for the "
            "purpose of temporarily filling in land tiles during map "
            "generation. This could be an error in freeciv, or a "
            "mistake in the terrain.ruleset file. Please make sure "
            "there is at least one land terrain type in the ruleset, "
            "or use a different map generator. If this error persists, "
            "please report it at: %s", BUG_URL);
    assert(land_fill != NULL);
  }

  hmap_shore_level = (hmap_max_level * (100 - map.server.landpercent)) / 100;
  ini_hmap_low_level();
  whole_map_iterate(ptile) {
    tile_set_terrain(ptile, T_UNKNOWN); /* set as oceans count is used */
    if (hmap(ptile) < hmap_shore_level) {
      int depth = (hmap_shore_level - hmap(ptile)) * 100 / hmap_shore_level;
      int ocean = 0;
      int land = 0;

      /* This is to make shallow connection between continents less likely */
      adjc_iterate(ptile, other) {
        if (hmap(other) < hmap_shore_level) {
          ocean++;
        } else {
          land++;
          break;
        }
      } adjc_iterate_end;

      depth += 30 * (ocean - land) / (ocean + land);

      depth = MIN(depth, TERRAIN_OCEAN_DEPTH_MAXIMUM);

      tile_set_terrain(ptile, pick_ocean(depth));
    } else {
      /* See note above for 'land_fill'. */
      tile_set_terrain(ptile, land_fill);
    }
  } whole_map_iterate_end;

  if (HAS_POLES) {
    renormalize_hmap_poles();
  } 

  create_tmap(TRUE); /* base temperature map, need hmap and oceans */
  
  if (HAS_POLES) { /* this is a hack to terrains set with not frizzed oceans*/
    make_polar_land(); /* make extra land at poles*/
  }

  create_placed_map(); /* here it means land terrains to be placed */
  set_all_ocean_tiles_placed();
  make_relief(); /* base relief on map */
  make_terrains(); /* place all exept mountains and hill */
  destroy_placed_map();

  make_rivers(); /* use a new placed_map. destroy older before call */
}

/**************************************************************************
  Returns if this is a 1x1 island
**************************************************************************/
static bool is_tiny_island(struct tile *ptile) 
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (is_ocean(pterrain) || pterrain->property[MG_FROZEN] > 0) {
    /* The arctic check is needed for iso-maps: the poles may not have
     * any cardinally adjacent land tiles, but that's okay. */
    return FALSE;
  }

  cardinal_adjc_iterate(ptile, tile1) {
    if (!is_ocean_tile(tile1)) {
      return FALSE;
    }
  } cardinal_adjc_iterate_end;

  return TRUE;
}

/**************************************************************************
  Removes all 1x1 islands (sets them to ocean).
**************************************************************************/
static void remove_tiny_islands(void)
{
  struct terrain *shallow = most_shallow_ocean();

  assert(NULL != shallow);
  whole_map_iterate(ptile) {
    if (is_tiny_island(ptile)) {
      tile_set_terrain(ptile, shallow);
      tile_clear_special(ptile, S_RIVER);
      tile_set_continent(ptile, 0);
    }
  } whole_map_iterate_end;
}

/**************************************************************************
  Debugging function to print information about the map that's been
  generated.
**************************************************************************/
static void print_mapgen_map(void)
{
  const int loglevel = LOG_DEBUG;
  int terrain_counts[terrain_count()];
  int total = 0;

  terrain_type_iterate(pterrain) {
    terrain_counts[terrain_index(pterrain)] = 0;
  } terrain_type_iterate_end;

  whole_map_iterate(ptile) {
    struct terrain *pterrain = tile_terrain(ptile);

    terrain_counts[terrain_index(pterrain)]++;
    if (!is_ocean(pterrain)) {
      total++;
    }
  } whole_map_iterate_end;

  terrain_type_iterate(pterrain) {
    freelog(loglevel, "%20s : %4d %d%%  ",
	    terrain_rule_name(pterrain),
	    terrain_counts[terrain_index(pterrain)],
	    (terrain_counts[terrain_index(pterrain)] * 100 + 50) / total);
  } terrain_type_iterate_end;
}

/**************************************************************************
  See stdinhand.c for information on map generation methods.

FIXME: Some continent numbers are unused at the end of this function, fx
       removed completely by remove_tiny_islands.
       When this function is finished various data is written to "islands",
       indexed by continent numbers, so a simple renumbering would not
       work...

  If "autosize" is specified then mapgen will automatically size the map
  based on the map.server.size server parameter and the specified topology.
  If not map.xsize and map.ysize will be used.
**************************************************************************/
void map_fractal_generate(bool autosize, struct unit_type *initial_unit)
{
  /* save the current random state: */
  RANDOM_STATE rstate = get_myrand_state();
 
  if (map.server.seed == 0) {
    /* Create a "random" map seed.  Note the call to myrand() which will
     * depend on the game seed. */
    map.server.seed = (myrand(MAX_UINT32) ^ time(NULL)) & (MAX_UINT32 >> 1);
    freelog(LOG_DEBUG, "Setting map.seed:%d", map.server.seed);
  }

  mysrand(map.server.seed);

  /* don't generate tiles with mapgen==0 as we've loaded them from file */
  /* also, don't delete (the handcrafted!) tiny islands in a scenario */
  if (map.server.generator != 0) {
    generator_init_topology(autosize);
    map_allocate();
    adjust_terrain_param();
    /* if one mapgenerator fails, it will choose another mapgenerator */
    /* with a lower number to try again */
    
    if (map.server.generator == 3) {
      /* 2 or 3 players per isle? */
      if (map.server.startpos == 2 || (map.server.startpos == 3)) { 
	mapgenerator4();
      }
      if (map.server.startpos <= 1) {
	/* single player per isle */
	mapgenerator3();
      }
      if (map.server.startpos == 4) {
	/* "variable" single player */
	mapgenerator2();
      }

      if (map.server.generator == 3) {
        smooth_water_depth();
      }
    }

    if (map.server.generator == 2) {
      make_pseudofractal1_hmap(1 + ((map.server.startpos == 0
				     || map.server.startpos == 3)
				    ? 0 : player_count()));
    }

    if (map.server.generator == 1) {
      make_random_hmap(MAX(1, 1 + get_sqsize() 
			   - (map.server.startpos ? player_count() / 4 : 0)));
    }

    /* if hmap only generator make anything else */
    if (map.server.generator == 1 || map.server.generator == 2) {
      make_land();
      free(height_map);
      height_map = NULL;
    }
    if (!map.server.tinyisles) {
      remove_tiny_islands();
    }

    /* Continent numbers must be assigned before regenerate_lakes() */
    assign_continent_numbers();

    /* Make second pass on water. */
    regenerate_lakes(NULL);
  } else {
    assign_continent_numbers();
  }

  if (!temperature_is_initialized()) {
    create_tmap(FALSE);
  }

  /* some scenarios already provide specials */
  if (!map.server.have_resources) {
    add_resources(map.server.riches);
  }

  if (!map.server.have_huts) {
    make_huts(map.server.huts); 
  }

  /* restore previous random state: */
  set_myrand_state(rstate);
  destroy_tmap();

  /* We don't want random start positions in a scenario which already
   * provides them. */
  if (map.server.num_start_positions == 0) {
    enum start_mode mode = MT_ALL;
    bool success;
    
    switch (map.server.generator) {
    case 0:
    case 1:
      mode = map.server.startpos;
      break;
    case 2:
      if (map.server.startpos == 0) {
        mode = MT_ALL;
      } else {
        mode = map.server.startpos;
      }
      break;
    case 3:
      if (map.server.startpos <= 1 || (map.server.startpos == 4)) {
        mode = MT_SINGLE;
      } else {
	mode = MT_2or3;
      }
      break;
    }
    
    for(;;) {
      success = create_start_positions(mode, initial_unit);
      if (success) {
        break;
      }
      
      switch(mode) {
        case MT_SINGLE:
	  mode = MT_2or3;
	  break;
	case MT_2or3:
	  mode = MT_ALL;
	  break;
	case MT_ALL:
	  mode = MT_VARIABLE;
	  break;
	default:
	  assert(0);
	  die("The server couldn't allocate starting positions.");
      }
    }
  }

  print_mapgen_map();
}

/**************************************************************************
 Convert parameters from the server into terrains percents parameters for
 the generators
**************************************************************************/
static void adjust_terrain_param(void)
{
  int polar = 2 * ICE_BASE_LEVEL * map.server.landpercent / MAX_COLATITUDE;
  float factor = (100.0 - polar - map.server.steepness * 0.8 ) / 10000;


  mountain_pct = factor * map.server.steepness * 90;

  /* 27 % if wetness == 50 & */
  forest_pct = factor * (map.server.wetness * 40 + 700) ; 
  jungle_pct = forest_pct * (MAX_COLATITUDE - TROPICAL_LEVEL) /
               (MAX_COLATITUDE * 2);
  forest_pct -= jungle_pct;

  /* 3 - 11 % */
  river_pct = (100 - polar) * (3 + map.server.wetness / 12) / 100;

  /* 6 %  if wetness == 50 && temperature == 50 */
  swamp_pct = factor * MAX(0,  (map.server.wetness * 9 - 150
                                + map.server.temperature * 6));
  desert_pct =factor * MAX(0, (map.server.temperature * 15 - 250
                               + (100 - map.server.wetness) * 10)) ;
}

/****************************************************************************
  Return TRUE if a safe tile is in a radius of 1.  This function is used to
  test where to place specials on the sea.
****************************************************************************/
static bool near_safe_tiles(struct tile *ptile)
{
  square_iterate(ptile, 1, tile1) {
    if (!terrain_has_flag(tile_terrain(tile1), TER_UNSAFE_COAST)) {
      return TRUE;
    }	
  } square_iterate_end;

  return FALSE;
}

/**************************************************************************
  this function spreads out huts on the map, a position can be used for a
  hut if there isn't another hut close and if it's not on the ocean.
**************************************************************************/
static void make_huts(int number)
{
  int count = 0;
  struct tile *ptile;

  create_placed_map(); /* here it means placed huts */

  while (number * map_num_tiles() >= 2000 && count++ < map_num_tiles() * 2) {

    /* Add a hut.  But not on a polar area, on an ocean, or too close to
     * another hut. */
    if ((ptile = rand_map_pos_characteristic(WC_ALL, TT_NFROZEN, MC_NONE))) {
      if (is_ocean_tile(ptile)) {
	map_set_placed(ptile); /* not good for a hut */
      } else {
	number--;
	tile_set_special(ptile, S_HUT);
	set_placed_near_pos(ptile, 3);
      }
    }
  }
  destroy_placed_map();
}

/****************************************************************************
  Return TRUE iff there's a resource within one tile of the given map
  position.
****************************************************************************/
static bool is_resource_close(const struct tile *ptile)
{
  square_iterate(ptile, 1, tile1) {
    if (NULL != tile_resource(tile1)) {
      return TRUE;
    }
  } square_iterate_end;

  return FALSE;
}

/****************************************************************************
  Add specials to the map with given probability (out of 1000).
****************************************************************************/
static void add_resources(int prob)
{
  whole_map_iterate(ptile)  {
    const struct terrain *pterrain = tile_terrain(ptile);

    if (is_resource_close (ptile) || myrand (1000) > prob) {
      continue;
    }
    if (!is_ocean(pterrain) || near_safe_tiles (ptile)
        || map.server.ocean_resources) {
      int i = 0;
      struct resource **r;

      for (r = pterrain->resources; *r; r++) {
	/* This is a standard way to get a random element from the
	 * pterrain->resources list, without computing its length in
	 * advance. Note that if *(pterrain->resources) == NULL, then
	 * this loop is a no-op. */
	if (!myrand (++i)) {
	  tile_set_resource(ptile, *r);
	}
      }
    }
  } whole_map_iterate_end;
  
  map.server.have_resources = TRUE;
}

/**************************************************************************
  common variables for generator 2, 3 and 4
**************************************************************************/
struct gen234_state {
  int isleindex, n, e, s, w;
  long int totalmass;
};

/**************************************************************************
Returns a random position in the rectangle denoted by the given state.
**************************************************************************/
static struct tile *get_random_map_position_from_state(
					       const struct gen234_state
					       *const pstate)
{
  int xn, yn;

  assert((pstate->e - pstate->w) > 0);
  assert((pstate->e - pstate->w) < map.xsize);
  assert((pstate->s - pstate->n) > 0);
  assert((pstate->s - pstate->n) < map.ysize);

  xn = pstate->w + myrand(pstate->e - pstate->w);
  yn = pstate->n + myrand(pstate->s - pstate->n);

  return native_pos_to_tile(xn, yn);
}

/**************************************************************************
  fill an island with up four types of terrains, rivers have extra code
**************************************************************************/
static void fill_island(int coast, long int *bucket,
			int warm0_weight, int warm1_weight,
			int cold0_weight, int cold1_weight,
			struct terrain *warm0,
			struct terrain *warm1,
			struct terrain *cold0,
			struct terrain *cold1,
			const struct gen234_state *const pstate)
{
  int i, k, capac;
  long int failsafe;

  if (*bucket <= 0 ) return;
  capac = pstate->totalmass;
  i = *bucket / capac;
  i++;
  *bucket -= i * capac;

  k= i;
  failsafe = i * (pstate->s - pstate->n) * (pstate->e - pstate->w);
  if (failsafe<0) {
    failsafe= -failsafe;
  }

  if (warm0_weight + warm1_weight + cold0_weight + cold1_weight <= 0)
    i= 0;

  while (i > 0 && (failsafe--) > 0) {
    struct tile *ptile =  get_random_map_position_from_state(pstate);

    if (tile_continent(ptile) == pstate->isleindex &&
	not_placed(ptile)) {

      /* the first condition helps make terrain more contiguous,
	 the second lets it avoid the coast: */
      if ( ( i*3>k*2 
	     || is_terrain_near_tile(ptile, warm0, FALSE) 
	     || is_terrain_near_tile(ptile, warm1, FALSE)
	     || myrand(100)<50 
	     || is_terrain_near_tile(ptile, cold0, FALSE)
	     || is_terrain_near_tile(ptile, cold1, FALSE)
	     )
	   &&( !is_cardinally_adj_to_ocean(ptile) || myrand(100) < coast )) {
	if (map_colatitude(ptile) < COLD_LEVEL) {
	  tile_set_terrain(ptile, (myrand(cold0_weight
					+ cold1_weight) < cold0_weight) 
			  ? cold0 : cold1);
	  map_set_placed(ptile);
	} else {
	  tile_set_terrain(ptile, (myrand(warm0_weight
					+ warm1_weight) < warm0_weight) 
			  ? warm0 : warm1);
	  map_set_placed(ptile);
	}
      }
      if (!not_placed(ptile)) i--;
    }
  }
}

/**************************************************************************
  Returns TRUE if ptile is suitable for a river mouth.
**************************************************************************/
static bool island_river_mouth_suitability(const struct tile *ptile)
{
  int num_card_ocean, pct_adj_ocean, num_adj_river;

  num_card_ocean = count_ocean_near_tile(ptile, C_CARDINAL, C_NUMBER);
  pct_adj_ocean = count_ocean_near_tile(ptile, C_ADJACENT, C_PERCENT);
  num_adj_river = count_special_near_tile(ptile, C_ADJACENT, C_NUMBER,
                                          S_RIVER);

  return (num_card_ocean == 1 && pct_adj_ocean <= 35
          && num_adj_river == 0);
}

/**************************************************************************
  Returns TRUE if there is a river in a cardinal direction near the tile
  and the tile is suitable for extending it.
**************************************************************************/
static bool island_river_suitability(const struct tile *ptile)
{
  int pct_adj_ocean, num_card_ocean, pct_adj_river, num_card_river;

  num_card_river = count_special_near_tile(ptile, C_CARDINAL, C_NUMBER,
                                           S_RIVER);
  num_card_ocean = count_ocean_near_tile(ptile, C_CARDINAL, C_NUMBER);
  pct_adj_ocean = count_ocean_near_tile(ptile, C_ADJACENT, C_PERCENT);
  pct_adj_river = count_special_near_tile(ptile, C_ADJACENT, C_PERCENT,
                                          S_RIVER);

  return (num_card_river == 1 && num_card_ocean == 0
          && pct_adj_ocean < 20 && pct_adj_river < 35
          /* The following expression helps with straightness,
           * ocean avoidance, and reduces forking. */
          && (pct_adj_river + pct_adj_ocean * 2) < myrand(25) + 25);
}

/**************************************************************************
  Fill an island with rivers.
**************************************************************************/
static void fill_island_rivers(int coast, long int *bucket,
                               const struct gen234_state *const pstate)
{
  long int failsafe, capac, i, k;
  struct tile *ptile;

  if (*bucket <= 0) {
    return;
  }
  capac = pstate->totalmass;
  i = *bucket / capac;
  i++;
  *bucket -= i * capac;

  /* generate 75% more rivers than generator 1 */
  i = (i * 175) / 100;

  k = i;
  failsafe = i * (pstate->s - pstate->n) * (pstate->e - pstate->w) * 5;
  if (failsafe < 0) {
    failsafe = -failsafe;
  }

  while (i > 0 && failsafe-- > 0) {
    ptile = get_random_map_position_from_state(pstate);
    if (tile_continent(ptile) != pstate->isleindex
        || tile_has_special(ptile, S_RIVER)) {
      continue;
    }

    if ((island_river_mouth_suitability(ptile)
         && (myrand(100) < coast || i == k))
        || island_river_suitability(ptile)) {
      tile_set_special(ptile, S_RIVER);
      i--;
    }
  }
}

/****************************************************************************
  Return TRUE if the ocean position is near land.  This is used in the
  creation of islands, so it differs logically from near_safe_tiles().
****************************************************************************/
static bool is_near_land(struct tile *ptile)
{
  /* Note this function may sometimes be called on land tiles. */
  adjc_iterate(ptile, tile1) {
    if (!is_ocean(tile_terrain(tile1))) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

static long int checkmass;

/**************************************************************************
  finds a place and drop the island created when called with islemass != 0
**************************************************************************/
static bool place_island(struct gen234_state *pstate)
{
  int i=0, xn, yn;
  struct tile *ptile;

  ptile = rand_map_pos();

  /* this helps a lot for maps with high landmass */
  for (yn = pstate->n, xn = pstate->w;
       yn < pstate->s && xn < pstate->e;
       yn++, xn++) {
    struct tile *tile0 = native_pos_to_tile(xn, yn);
    struct tile *tile1 = native_pos_to_tile(xn + ptile->nat_x - pstate->w,
					    yn + ptile->nat_y - pstate->n);

    if (!tile0 || !tile1) {
      return FALSE;
    }
    if (hmap(tile0) != 0 && is_near_land(tile1)) {
      return FALSE;
    }
  }
		       
  for (yn = pstate->n; yn < pstate->s; yn++) {
    for (xn = pstate->w; xn < pstate->e; xn++) {
      struct tile *tile0 = native_pos_to_tile(xn, yn);
      struct tile *tile1 = native_pos_to_tile(xn + ptile->nat_x - pstate->w,
					      yn + ptile->nat_y - pstate->n);

      if (!tile0 || !tile1) {
	return FALSE;
      }
      if (hmap(tile0) != 0 && is_near_land(tile1)) {
	return FALSE;
      }
    }
  }

  for (yn = pstate->n; yn < pstate->s; yn++) {
    for (xn = pstate->w; xn < pstate->e; xn++) {
      if (hmap(native_pos_to_tile(xn, yn)) != 0) {
	struct tile *tile1
	  = native_pos_to_tile(xn + ptile->nat_x - pstate->w,
			       yn + ptile->nat_y - pstate->n);

	checkmass--; 
	if (checkmass <= 0) {
	  freelog(LOG_ERROR, "mapgen.c: mass doesn't sum up.");
	  return i != 0;
	}

        tile_set_terrain(tile1, T_UNKNOWN);
	map_unset_placed(tile1);

	tile_set_continent(tile1, pstate->isleindex);
        i++;
      }
    }
  }
  pstate->s += ptile->nat_y - pstate->n;
  pstate->e += ptile->nat_x - pstate->w;
  pstate->n = ptile->nat_y;
  pstate->w = ptile->nat_x;
  return i != 0;
}

/****************************************************************************
  Returns the number of cardinally adjacent tiles have a non-zero elevation.
****************************************************************************/
static int count_card_adjc_elevated_tiles(struct tile *ptile)
{
  int count = 0;

  cardinal_adjc_iterate(ptile, tile1) {
    if (hmap(tile1) != 0) {
      count++;
    }
  } cardinal_adjc_iterate_end;

  return count;
}

/**************************************************************************
  finds a place and drop the island created when called with islemass != 0
**************************************************************************/
static bool create_island(int islemass, struct gen234_state *pstate)
{
  int i;
  long int tries=islemass*(2+islemass/20)+99;
  bool j;
  struct tile *ptile = native_pos_to_tile(map.xsize / 2, map.ysize / 2);

  memset(height_map, '\0', MAP_INDEX_SIZE * sizeof(*height_map));
  hmap(native_pos_to_tile(map.xsize / 2, map.ysize / 2)) = 1;
  pstate->n = ptile->nat_y - 1;
  pstate->w = ptile->nat_x - 1;
  pstate->s = ptile->nat_y + 2;
  pstate->e = ptile->nat_x + 2;
  i = islemass - 1;
  while (i > 0 && tries-->0) {
    ptile = get_random_map_position_from_state(pstate);

    if ((!near_singularity(ptile) || myrand(50) < 25 ) 
	&& hmap(ptile) == 0 && count_card_adjc_elevated_tiles(ptile) > 0) {
      hmap(ptile) = 1;
      i--;
      if (ptile->nat_y >= pstate->s - 1 && pstate->s < map.ysize - 2) {
	pstate->s++;
      }
      if (ptile->nat_x >= pstate->e - 1 && pstate->e < map.xsize - 2) {
	pstate->e++;
      }
      if (ptile->nat_y <= pstate->n && pstate->n > 2) {
	pstate->n--;
      }
      if (ptile->nat_x <= pstate->w && pstate->w > 2) {
	pstate->w--;
      }
    }
    if (i < islemass / 10) {
      int xn, yn;

      for (yn = pstate->n; yn < pstate->s; yn++) {
	for (xn = pstate->w; xn < pstate->e; xn++) {
	  ptile = native_pos_to_tile(xn, yn);
	  if (hmap(ptile) == 0 && i > 0
	      && count_card_adjc_elevated_tiles(ptile) == 4) {
	    hmap(ptile) = 1;
            i--; 
          }
	}
      }
    }
  }
  if (tries<=0) {
    freelog(LOG_ERROR, "create_island ended early with %d/%d.",
	    islemass-i, islemass);
  }
  
  tries = map_num_tiles() / 4;	/* on a 40x60 map, there are 2400 places */
  while (!(j = place_island(pstate)) && (--tries) > 0) {
    /* nothing */
  }
  return j;
}

/*************************************************************************/

/**************************************************************************
  make an island, fill every tile type except plains
  note: you have to create big islands first.
  Return TRUE if successful.
  min_specific_island_size is a percent value.
***************************************************************************/
static bool make_island(int islemass, int starters,
			struct gen234_state *pstate,
			int min_specific_island_size)
{
  /* int may be only 2 byte ! */
  static long int tilefactor, balance, lastplaced;
  static long int riverbuck, mountbuck, desertbuck, forestbuck, swampbuck;

  int i;

  if (islemass == 0) {
    /* this only runs to initialise static things, not to actually
     * create an island. */
    balance = 0;
    pstate->isleindex = map.num_continents + 1;	/* 0= none, poles, then isles */

    checkmass = pstate->totalmass;

    /* caveat: this should really be sent to all players */
    if (pstate->totalmass > 3000)
      freelog(LOG_NORMAL, _("High landmass - this may take a few seconds."));

    i = river_pct + mountain_pct + desert_pct + forest_pct + swamp_pct;
    i = (i <= 90) ? 100 : i * 11 / 10;
    tilefactor = pstate->totalmass / i;
    riverbuck = -(long int) myrand(pstate->totalmass);
    mountbuck = -(long int) myrand(pstate->totalmass);
    desertbuck = -(long int) myrand(pstate->totalmass);
    forestbuck = -(long int) myrand(pstate->totalmass);
    swampbuck = -(long int) myrand(pstate->totalmass);
    lastplaced = pstate->totalmass;
  } else {

    /* makes the islands this big */
    islemass = islemass - balance;

    if (islemass > lastplaced + 1 + lastplaced / 50) {
      /* don't create big isles we can't place */
      islemass = lastplaced + 1 + lastplaced / 50;
    }

    /* isle creation does not perform well for nonsquare islands */
    if (islemass > (map.ysize - 6) * (map.ysize - 6)) {
      islemass = (map.ysize - 6) * (map.ysize - 6);
    }

    if (islemass > (map.xsize - 2) * (map.xsize - 2)) {
      islemass = (map.xsize - 2) * (map.xsize - 2);
    }

    i = islemass;
    if (i <= 0) {
      return FALSE;
    }
    assert(starters >= 0);
    freelog(LOG_VERBOSE, "island %i", pstate->isleindex);

    /* keep trying to place an island, and decrease the size of
     * the island we're trying to create until we succeed.
     * If we get too small, return an error. */
    while (!create_island(i, pstate)) {
      if (i < islemass * min_specific_island_size / 100) {
	return FALSE;
      }
      i--;
    }
    i++;
    lastplaced = i;
    if (i * 10 > islemass) {
      balance = i - islemass;
    } else{
      balance = 0;
    }

    freelog(LOG_VERBOSE, "ini=%d, plc=%d, bal=%ld, tot=%ld",
	    islemass, i, balance, checkmass);

    i *= tilefactor;

    riverbuck += river_pct * i;
    fill_island_rivers(1, &riverbuck, pstate);

    mountbuck += mountain_pct * i;
    fill_island(20, &mountbuck,
		3, 1, 3,1,
		pick_terrain(MG_MOUNTAINOUS, MG_GREEN, MG_LAST),
		pick_terrain(MG_MOUNTAINOUS, MG_LAST, MG_GREEN),
		pick_terrain(MG_MOUNTAINOUS, MG_GREEN, MG_LAST),
		pick_terrain(MG_MOUNTAINOUS, MG_LAST, MG_GREEN),
		pstate);
    desertbuck += desert_pct * i;
    fill_island(40, &desertbuck,
		1, 1, 1, 1,
		pick_terrain(MG_DRY, MG_TROPICAL, MG_LAST),
		pick_terrain(MG_DRY, MG_TROPICAL, MG_LAST),
		pick_terrain(MG_DRY, MG_TROPICAL, MG_LAST),
		pick_terrain(MG_COLD, MG_LAST, MG_LAST),
		pstate);
    forestbuck += forest_pct * i;
    fill_island(60, &forestbuck,
		forest_pct, swamp_pct, forest_pct, swamp_pct,
		pick_terrain(MG_FOLIAGE, MG_TEMPERATE, MG_LAST),
		pick_terrain(MG_FOLIAGE, MG_TROPICAL, MG_COLD),
		pick_terrain(MG_FOLIAGE, MG_TEMPERATE, MG_LAST),
		pick_terrain(MG_FOLIAGE, MG_TROPICAL, MG_COLD),
		pstate);
    swampbuck += swamp_pct * i;
    fill_island(80, &swampbuck,
		1, 1, 1, 1,
		pick_terrain(MG_WET, MG_LAST, MG_FOLIAGE),
		pick_terrain(MG_WET, MG_LAST, MG_FOLIAGE),
		pick_terrain(MG_WET, MG_LAST, MG_FOLIAGE),
		pick_terrain(MG_WET, MG_LAST, MG_FOLIAGE),
		pstate);

    pstate->isleindex++;
    map.num_continents++;
  }
  return TRUE;
}

/**************************************************************************
  fill ocean and make polar
**************************************************************************/
static void initworld(struct gen234_state *pstate)
{
  struct terrain *deepest_ocean = pick_ocean(TERRAIN_OCEAN_DEPTH_MAXIMUM);

  assert(NULL != deepest_ocean);
  height_map = fc_malloc(MAP_INDEX_SIZE * sizeof(*height_map));
  create_placed_map(); /* land tiles which aren't placed yet */
  create_tmap(FALSE);
  
  whole_map_iterate(ptile) {
    tile_set_terrain(ptile, deepest_ocean);
    tile_set_continent(ptile, 0);
    map_set_placed(ptile); /* not a land tile */
    tile_clear_all_specials(ptile);
    tile_set_owner(ptile, NULL, NULL);
  } whole_map_iterate_end;
  
  if (HAS_POLES) {
    make_polar();
  }
  
  /* Set poles numbers.  After the map is generated continents will 
   * be renumbered. */
  make_island(0, 0, pstate, 0);
}  

/* This variable is the Default Minimum Specific Island Size, 
 * ie the smallest size we'll typically permit our island, as a % of
 * the size we wanted. So if we ask for an island of size x, the island 
 * creation will return if it would create an island smaller than
 *  x * DMSIS / 100 */
#define DMSIS 10

/**************************************************************************
  island base map generators
**************************************************************************/
static void mapgenerator2(void)
{
  long int totalweight;
  struct gen234_state state;
  struct gen234_state *pstate = &state;
  int i;
  bool done = FALSE;
  int spares= 1; 
  /* constant that makes up that an island actually needs additional space */

  /* put 70% of land in big continents, 
   *     20% in medium, and 
   *     10% in small. */ 
  int bigfrac = 70, midfrac = 20, smallfrac = 10;

  if (map.server.landpercent > 85) {
    map.server.generator = 1;
    return;
  }

  pstate->totalmass = ((map.ysize - 6 - spares) * map.server.landpercent 
                       * (map.xsize - spares)) / 100;
  totalweight = 100 * player_count();

  assert(!placed_map_is_initialized());

  while (!done && bigfrac > midfrac) {
    done = TRUE;

    if (placed_map_is_initialized()) {
      destroy_placed_map();
      destroy_tmap();
    }

    initworld(pstate);

    /* Create one big island for each player. */
    for (i = player_count(); i > 0; i--) {
      if (!make_island(bigfrac * pstate->totalmass / totalweight,
                      1, pstate, 95)) {
	/* we couldn't make an island at least 95% as big as we wanted,
	 * and since we're trying hard to be fair, we need to start again,
	 * with all big islands reduced slightly in size.
	 * Take the size reduction from the big islands and add it to the 
	 * small islands to keep overall landmass unchanged.
	 * Note that the big islands can get very small if necessary, and
	 * the smaller islands will not exist if we can't place them 
         * easily. */
	freelog(LOG_VERBOSE,
		"Island too small, trying again with all smaller islands.\n");
	midfrac += bigfrac * 0.01;
	smallfrac += bigfrac * 0.04;
	bigfrac *= 0.95;
	done = FALSE;	
	break;
      }
    }
  }

  if (bigfrac <= midfrac) {
    /* We could never make adequately big islands. */
    freelog(LOG_NORMAL, _("Falling back to generator %d."), 1);
    map.server.generator = 1;

    /* init world created this map, destroy it before abort */
    destroy_placed_map();
    free(height_map);
    height_map = NULL;
    return;
  }

  /* Now place smaller islands, but don't worry if they're small,
   * or even non-existent. One medium and one small per player. */
  for (i = player_count(); i > 0; i--) {
    make_island(midfrac * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }
  for (i = player_count(); i > 0; i--) {
    make_island(smallfrac * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }

  make_plains();  
  destroy_placed_map();
  free(height_map);
  height_map = NULL;

  if (checkmass > map.xsize + map.ysize + totalweight) {
    freelog(LOG_VERBOSE, "%ld mass left unplaced", checkmass);
  }
}

/**************************************************************************
On popular demand, this tries to mimick the generator 3 as best as possible.
**************************************************************************/
static void mapgenerator3(void)
{
  int spares= 1;
  int j=0;
  
  long int islandmass,landmass, size;
  long int maxmassdiv6=20;
  int bigislands;
  struct gen234_state state;
  struct gen234_state *pstate = &state;

  if (map.server.landpercent > 80) {
    map.server.generator = 2;
    return;
  }

  pstate->totalmass = (((map.ysize - 6 - spares) * map.server.landpercent
                        * (map.xsize - spares)) / 100);

  bigislands= player_count();

  landmass = (map.xsize * (map.ysize - 6) * map.server.landpercent)/100;
  /* subtracting the arctics */
  if (landmass > 3 * map.ysize + player_count() * 3){
    landmass -= 3 * map.ysize;
  }


  islandmass= (landmass)/(3 * bigislands);
  if (islandmass < 4 * maxmassdiv6) {
    islandmass = (landmass)/(2 * bigislands);
  }
  if (islandmass < 3 * maxmassdiv6 && player_count() * 2 < landmass) {
    islandmass= (landmass)/(bigislands);
  }

  if (map.xsize < 40 || map.ysize < 40 || map.server.landpercent > 80) { 
    freelog(LOG_NORMAL, _("Falling back to generator %d."), 2); 
    map.server.generator = 2;
    return; 
  }

  if (islandmass < 2) {
    islandmass = 2;
  }
  if(islandmass > maxmassdiv6 * 6) {
    islandmass = maxmassdiv6 * 6;/* !PS: let's try this */
  }

  initworld(pstate);

  while (pstate->isleindex - 2 <= bigislands && checkmass > islandmass &&
         ++j < 500) {
    make_island(islandmass, 1, pstate, DMSIS);
  }

  if (j == 500){
    freelog(LOG_NORMAL, _("Generator 3 didn't place all big islands."));
  }
  
  islandmass= (islandmass * 11)/8;
  /*!PS: I'd like to mult by 3/2, but starters might make trouble then*/
  if (islandmass < 2) {
    islandmass= 2;
  }

  while (checkmass > islandmass && ++j < 1500) {
    if (j < 1000) {
      size = myrand((islandmass+1)/2+1)+islandmass/2;
    } else {
      size = myrand((islandmass+1)/2+1);
    }
    if (size < 2) {
      size=2;
    }

    make_island(size, (pstate->isleindex - 2 <= player_count()) ? 1 : 0,
		pstate, DMSIS);
  }

  make_plains();  
  destroy_placed_map();
  free(height_map);
  height_map = NULL;
    
  if (j == 1500) {
    freelog(LOG_NORMAL, _("Generator 3 left %li landmass unplaced."), checkmass);
  } else if (checkmass > map.xsize + map.ysize) {
    freelog(LOG_VERBOSE, "%ld mass left unplaced", checkmass);
  }
}

/**************************************************************************
...
**************************************************************************/
static void mapgenerator4(void)
{
  int bigweight=70;
  int spares= 1;
  int i;
  long int totalweight;
  struct gen234_state state;
  struct gen234_state *pstate = &state;


  /* no islands with mass >> sqr(min(xsize,ysize)) */

  if (player_count() < 2 || map.server.landpercent > 80) {
    map.server.startpos = 1;
    return;
  }

  if (map.server.landpercent > 60) {
    bigweight=30;
  } else if (map.server.landpercent > 40) {
    bigweight=50;
  } else {
    bigweight=70;
  }

  spares = (map.server.landpercent - 5) / 30;

  pstate->totalmass = (((map.ysize - 6 - spares) * map.server.landpercent
                        * (map.xsize - spares)) / 100);

  /*!PS: The weights NEED to sum up to totalweight (dammit) */
  totalweight = (30 + bigweight) * player_count();

  initworld(pstate);

  i = player_count() / 2;
  if ((player_count() % 2) == 1) {
    make_island(bigweight * 3 * pstate->totalmass / totalweight, 3, 
		pstate, DMSIS);
  } else {
    i++;
  }
  while ((--i) > 0) {
    make_island(bigweight * 2 * pstate->totalmass / totalweight, 2,
		pstate, DMSIS);
  }
  for (i = player_count(); i > 0; i--) {
    make_island(20 * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }
  for (i = player_count(); i > 0; i--) {
    make_island(10 * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }
  make_plains();  
  destroy_placed_map();
  free(height_map);
  height_map = NULL;

  if (checkmass > map.xsize + map.ysize + totalweight) {
    freelog(LOG_VERBOSE, "%ld mass left unplaced", checkmass);
  }
}

#undef DMSIS
