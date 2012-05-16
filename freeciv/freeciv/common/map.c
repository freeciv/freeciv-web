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
#include <string.h>		/* strlen */

#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "movement.h"
#include "packets.h"
#include "rand.h"
#include "shared.h"
#include "support.h"
#include "unit.h"
#include "unitlist.h"

#include "map.h"

/* the very map */
struct civ_map map;

/* An entry in the start position table. */
struct startpos_entry {
  int key;
  const struct nation_type *nation;
};

/* these are initialized from the terrain ruleset */
struct terrain_misc terrain_control;

/* used to compute neighboring tiles.
 *
 * using
 *   x1 = x + DIR_DX[dir];
 *   y1 = y + DIR_DY[dir];
 * will give you the tile as shown below.
 *   -------
 *   |0|1|2|
 *   |-+-+-|
 *   |3| |4|
 *   |-+-+-|
 *   |5|6|7|
 *   -------
 * Note that you must normalize x1 and y1 yourself.
 */
const int DIR_DX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
const int DIR_DY[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

/****************************************************************************
  Return a bitfield of the specials on the tile that are infrastructure.
****************************************************************************/
bv_special get_tile_infrastructure_set(const struct tile *ptile,
					  int *pcount)
{
  bv_special pspresent;
  int i, count = 0;

  BV_CLR_ALL(pspresent);
  for (i = 0; infrastructure_specials[i] != S_LAST; i++) {
    if (tile_has_special(ptile, infrastructure_specials[i])) {
      BV_SET(pspresent, infrastructure_specials[i]);
      count++;
    }
  }
  if (pcount) {
    *pcount = count;
  }
  return pspresent;
}

/***************************************************************
  Returns 1 if we are at a stage of the game where the map
  has not yet been generated/loaded.
  (To be precise, returns 1 if map_allocate() has not yet been
  called.)
***************************************************************/
bool map_is_empty(void)
{
  return !map.tiles;
}

/***************************************************************
 put some sensible values into the map structure
***************************************************************/
void map_init(void)
{
  map.topology_id = MAP_DEFAULT_TOPO;
  map.num_continents = 0;
  map.num_oceans = 0;
  map.tiles = NULL;
  map.startpos_table = NULL;

  /* The [xy]size values are set in map_init_topology.  It is initialized
   * to a non-zero value because some places erronously use these values
   * before they're initialized. */
  map.xsize = MAP_MIN_LINEAR_SIZE;  
  map.ysize = MAP_MIN_LINEAR_SIZE;

  if (is_server()) {
    map.server.size = MAP_DEFAULT_SIZE;
    map.server.seed = MAP_DEFAULT_SEED;
    map.server.riches = MAP_DEFAULT_RICHES;
    map.server.huts = MAP_DEFAULT_HUTS;
    map.server.landpercent = MAP_DEFAULT_LANDMASS;
    map.server.wetness = MAP_DEFAULT_WETNESS;
    map.server.steepness = MAP_DEFAULT_STEEPNESS;
    map.server.generator = MAP_DEFAULT_GENERATOR;
    map.server.startpos = MAP_DEFAULT_STARTPOS;
    map.server.tinyisles = MAP_DEFAULT_TINYISLES;
    map.server.separatepoles = MAP_DEFAULT_SEPARATE_POLES;
    map.server.alltemperate = MAP_DEFAULT_ALLTEMPERATE;
    map.server.temperature = MAP_DEFAULT_TEMPERATURE;
    map.server.num_start_positions = 0;
    map.server.have_resources = FALSE;
    map.server.have_rivers_overlay = FALSE;
    map.server.have_huts = FALSE;
  }
}

/**************************************************************************
  Fill the iterate_outwards_indices array.  This may depend on the topology.
***************************************************************************/
static void generate_map_indices(void)
{
  int i = 0, nat_x, nat_y, tiles;
  struct iter_index *array = map.iterate_outwards_indices;
  int nat_center_x, nat_center_y, nat_min_x, nat_min_y, nat_max_x, nat_max_y;
  int map_center_x, map_center_y;

  /* These caluclations are done via tricky native math.  We need to make
   * sure that when "exploring" positions in the iterate_outward we hit each
   * position within the distance exactly once.
   *
   * To do this we pick a center position (at the center of the map, for
   * convenience).  Then we iterate over all of the positions around it,
   * accounting for wrapping, in native coordinates.  Note that some of the
   * positions iterated over before will not even be real; the point is to
   * use the native math so as to get the right behavior under different
   * wrapping conditions.
   *
   * Thus the "center" position below is just an arbitrary point.  We choose
   * the center of the map to make the min/max values (below) simpler. */
  nat_center_x = map.xsize / 2;
  nat_center_y = map.ysize / 2;
  NATIVE_TO_MAP_POS(&map_center_x, &map_center_y,
		    nat_center_x, nat_center_y);

  /* If we wrap in a particular direction (X or Y) we only need to explore a
   * half of a map-width in that direction before we hit the wrap point.  If
   * not we need to explore the full width since we have to account for the
   * worst-case where we start at one edge of the map.  Of course if we try
   * to explore too far the extra steps will just be skipped by the
   * normalize check later on.  So the purpose at this point is just to
   * get the right set of positions, relative to the start position, that
   * may be needed for the iteration.
   *
   * If the map does wrap, we go map.Nsize / 2 in each direction.  This
   * gives a min value of 0 and a max value of Nsize-1, because of the
   * center position chosen above.  This also avoids any off-by-one errors.
   *
   * If the map doesn't wrap, we go map.Nsize-1 in each direction.  In this
   * case we're not concerned with going too far and wrapping around, so we
   * just have to make sure we go far enough if we're at one edge of the
   * map. */
  nat_min_x = (topo_has_flag(TF_WRAPX) ? 0 : (nat_center_x - map.xsize + 1));
  nat_min_y = (topo_has_flag(TF_WRAPY) ? 0 : (nat_center_y - map.ysize + 1));

  nat_max_x = (topo_has_flag(TF_WRAPX)
	       ? (map.xsize - 1)
	       : (nat_center_x + map.xsize - 1));
  nat_max_y = (topo_has_flag(TF_WRAPY)
	       ? (map.ysize - 1)
	       : (nat_center_y + map.ysize - 1));
  tiles = (nat_max_x - nat_min_x + 1) * (nat_max_y - nat_min_y + 1);

  array = fc_realloc(array, tiles * sizeof(*array));

  for (nat_x = nat_min_x; nat_x <= nat_max_x; nat_x++) {
    for (nat_y = nat_min_y; nat_y <= nat_max_y; nat_y++) {
      int map_x, map_y, dx, dy;

      /* Now for each position, we find the vector (in map coordinates) from
       * the center position to that position.  Then we calculate the
       * distance between the two points.  Wrapping is ignored at this
       * point since the use of native positions means we should always have
       * the shortest vector. */
      NATIVE_TO_MAP_POS(&map_x, &map_y, nat_x, nat_y);
      dx = map_x - map_center_x;
      dy = map_y - map_center_y;

      array[i].dx = dx;
      array[i].dy = dy;
      array[i].dist = map_vector_to_real_distance(dx, dy);
      i++;
    }
  }
  assert(i == tiles);

  qsort(array, tiles, sizeof(*array), compare_iter_index);

#if 0
  for (i = 0; i < tiles; i++) {
    freelog(LOG_DEBUG, "%5d : (%3d,%3d) : %d",
	    i, array[i].dx, array[i].dy, array[i].dist);
  }
#endif

  map.num_iterate_outwards_indices = tiles;
  map.iterate_outwards_indices = array;
}

/****************************************************************************
  map_init_topology needs to be called after map.topology_id is changed.

  If map.size is changed, map.xsize and map.ysize must be set before
  calling map_init_topology(TRUE).  This is done by the mapgen code
  (server) and packhand code (client).

  If map.xsize and map.ysize are changed, call map_init_topology(FALSE) to
  calculate map.size.  This should be done in the client or when loading
  savegames, since the [xy]size values are already known.
****************************************************************************/
void map_init_topology(bool set_sizes)
{
  enum direction8 dir;

  if (!set_sizes && is_server()) {
    /* Set map.size based on map.xsize and map.ysize. */
    map.server.size = (float)(map_num_tiles()) / 1000.0 + 0.5;
  }
  
  /* sanity check for iso topologies*/
  assert(!MAP_IS_ISOMETRIC || (map.ysize % 2) == 0);

  /* The size and ratio must satisfy the minimum and maximum *linear*
   * restrictions on width */
  assert(MAP_WIDTH >= MAP_MIN_LINEAR_SIZE);
  assert(MAP_HEIGHT >= MAP_MIN_LINEAR_SIZE);
  assert(MAP_WIDTH <= MAP_MAX_LINEAR_SIZE);
  assert(MAP_HEIGHT <= MAP_MAX_LINEAR_SIZE);

  map.num_valid_dirs = map.num_cardinal_dirs = 0;
  for (dir = 0; dir < 8; dir++) {
    if (is_valid_dir(dir)) {
      map.valid_dirs[map.num_valid_dirs] = dir;
      map.num_valid_dirs++;
    }
    if (is_cardinal_dir(dir)) {
      map.cardinal_dirs[map.num_cardinal_dirs] = dir;
      map.num_cardinal_dirs++;
    }
  }
  assert(map.num_valid_dirs > 0 && map.num_valid_dirs <= 8);
  assert(map.num_cardinal_dirs > 0
	 && map.num_cardinal_dirs <= map.num_valid_dirs);
}

/***************************************************************
...
***************************************************************/
static void tile_init(struct tile *ptile)
{
  ptile->continent = 0;

  BV_CLR_ALL(ptile->tile_known);
  vision_layer_iterate(v) {
    BV_CLR_ALL(ptile->tile_seen[v]);
  } vision_layer_iterate_end;

  tile_clear_all_specials (ptile);
  ptile->resource = NULL;
  ptile->terrain  = T_UNKNOWN;
  ptile->units    = unit_list_new();
  ptile->owner    = NULL; /* Not claimed by any player. */
  ptile->claimer  = NULL;
  ptile->worked   = NULL; /* No city working here. */
  ptile->spec_sprite = NULL;
}

/****************************************************************************
  Step from the given tile in the given direction.  The new tile is returned,
  or NULL if the direction is invalid or leads off the map.
****************************************************************************/
struct tile *mapstep(const struct tile *ptile, enum direction8 dir)
{
  int x, y;

  if (!is_valid_dir(dir)) {
    return NULL;
  }

  DIRSTEP(x, y, dir);
  x += ptile->x;
  y += ptile->y;

  return map_pos_to_tile(x, y);
}

/****************************************************************************
  Return the tile for the given native position, with wrapping.

  This is a backend function used by map_pos_to_tile and native_pos_to_tile.
  It is called extremely often so it is made inline.
****************************************************************************/
static inline struct tile *base_native_pos_to_tile(int nat_x, int nat_y)
{
  /* If the position is out of range in a non-wrapping direction, it is
   * unreal. */
  if (!((topo_has_flag(TF_WRAPX) || (nat_x >= 0 && nat_x < map.xsize))
	&& (topo_has_flag(TF_WRAPY) || (nat_y >= 0 && nat_y < map.ysize)))) {
    return NULL;
  }

  /* Wrap in X and Y directions, as needed. */
  if (topo_has_flag(TF_WRAPX)) {
    nat_x = FC_WRAP(nat_x, map.xsize);
  }
  if (topo_has_flag(TF_WRAPY)) {
    nat_y = FC_WRAP(nat_y, map.ysize);
  }

  return map.tiles + native_pos_to_index(nat_x, nat_y);
}

/****************************************************************************
  Return the tile for the given cartesian (map) position.
****************************************************************************/
struct tile *map_pos_to_tile(int map_x, int map_y)
{
  int nat_x, nat_y;

  if (!map.tiles) {
    return NULL;
  }

  /* Normalization is best done in native coordinates. */
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, map_x, map_y);
  return base_native_pos_to_tile(nat_x, nat_y);
}

/****************************************************************************
  Return the tile for the given native position.
****************************************************************************/
struct tile *native_pos_to_tile(int nat_x, int nat_y)
{
  if (!map.tiles) {
    return NULL;
  }

  return base_native_pos_to_tile(nat_x, nat_y);
}

/****************************************************************************
  Return the tile for the given index position.
****************************************************************************/
struct tile *index_to_tile(int index)
{
  if (!map.tiles) {
    return NULL;
  }

  if (index >= 0 && index < MAP_INDEX_SIZE) {
    return map.tiles + index;
  } else {
    /* Unwrapped index coordinates are impossible, so the best we can do is
     * return NULL. */
    return NULL;
  }
}

/***************************************************************
...
***************************************************************/
static void tile_free(struct tile *ptile)
{
  unit_list_free(ptile->units);
  if (ptile->spec_sprite) {
    free(ptile->spec_sprite);
    ptile->spec_sprite = NULL;
  }
}

/**************************************************************************
  Allocate space for map, and initialise the tiles.
  Uses current map.xsize and map.ysize.
**************************************************************************/
void map_allocate(void)
{
  freelog(LOG_DEBUG, "map_allocate (was %p) (%d,%d)",
	  (void *)map.tiles, map.xsize, map.ysize);

  assert(map.tiles == NULL);
  map.tiles = fc_calloc(MAP_INDEX_SIZE, sizeof(*map.tiles));

  /* Note this use of whole_map_iterate may be a bit sketchy, since the
   * tile values (ptile->index, etc.) haven't been set yet.  It might be
   * better to do a manual loop here. */
  whole_map_iterate(ptile) {
    ptile->index = ptile - map.tiles;
    index_to_map_pos(&ptile->x, &ptile->y, tile_index(ptile));
    index_to_native_pos(&ptile->nat_x, &ptile->nat_y, tile_index(ptile));
    CHECK_INDEX(tile_index(ptile));
    CHECK_MAP_POS(ptile->x, ptile->y);
    CHECK_NATIVE_POS(ptile->nat_x, ptile->nat_y);

    tile_init(ptile);
  } whole_map_iterate_end;

  generate_city_map_indices();
  generate_map_indices();

  if (map.startpos_table != NULL) {
    hash_free(map.startpos_table);
  }
  map.startpos_table = hash_new_full(hash_fval_int, hash_fcmp_int,
                                     NULL, free);
}

/***************************************************************
  Frees the allocated memory of the map.
***************************************************************/
void map_free(void)
{
  if (map.tiles) {
    /* it is possible that map_init was called but not map_allocate */

    whole_map_iterate(ptile) {
      tile_free(ptile);
    } whole_map_iterate_end;

    free(map.tiles);
    map.tiles = NULL;

    if (map.startpos_table) {
      hash_free(map.startpos_table);
      map.startpos_table = NULL;
    }
  }
}

/****************************************************************************
  Return the "distance" (which is really the Manhattan distance, and should
  rarely be used) for a given vector.
****************************************************************************/
static int map_vector_to_distance(int dx, int dy)
{
  if (topo_has_flag(TF_HEX)) {
    /* Hex: all directions are cardinal so the distance is equivalent to
     * the real distance. */
    return map_vector_to_real_distance(dx, dy);
  } else {
    return abs(dx) + abs(dy);
  }
}

/****************************************************************************
  Return the "real" distance for a given vector.
****************************************************************************/
int map_vector_to_real_distance(int dx, int dy)
{
  const int absdx = abs(dx), absdy = abs(dy);

  if (topo_has_flag(TF_HEX)) {
    if (topo_has_flag(TF_ISO)) {
      /* Iso-hex: you can't move NE or SW. */
      if ((dx < 0 && dy > 0)
	  || (dx > 0 && dy < 0)) {
	/* Diagonal moves in this direction aren't allowed, so it will take
	 * the full number of moves. */
        return absdx + absdy;
      } else {
	/* Diagonal moves in this direction *are* allowed. */
        return MAX(absdx, absdy);
      }
    } else {
      /* Hex: you can't move SE or NW. */
      if ((dx > 0 && dy > 0)
	  || (dx < 0 && dy < 0)) {
	/* Diagonal moves in this direction aren't allowed, so it will take
	 * the full number of moves. */
	return absdx + absdy;
      } else {
	/* Diagonal moves in this direction *are* allowed. */
        return MAX(absdx, absdy);
      }
    }
  } else {
    return MAX(absdx, absdy);
  }
}

/****************************************************************************
  Return the sq_distance for a given vector.
****************************************************************************/
int map_vector_to_sq_distance(int dx, int dy)
{
  if (topo_has_flag(TF_HEX)) {
    /* Hex: The square distance is just the square of the real distance; we
     * don't worry about pythagorean calculations. */
    int dist = map_vector_to_real_distance(dx, dy);

    return dist * dist;
  } else {
    return dx * dx + dy * dy;
  }
}

/***************************************************************
...
***************************************************************/
int real_map_distance(const struct tile *tile0, const struct tile *tile1)
{
  int dx, dy;

  map_distance_vector(&dx, &dy, tile0, tile1);
  return map_vector_to_real_distance(dx, dy);
}

/***************************************************************
...
***************************************************************/
int sq_map_distance(const struct tile *tile0, const struct tile *tile1)
{
  /* We assume map_distance_vector gives us the vector with the
     minimum squared distance. Right now this is true. */
  int dx, dy;

  map_distance_vector(&dx, &dy, tile0, tile1);
  return map_vector_to_sq_distance(dx, dy);
}

/***************************************************************
...
***************************************************************/
int map_distance(const struct tile *tile0, const struct tile *tile1)
{
  /* We assume map_distance_vector gives us the vector with the
     minimum map distance. Right now this is true. */
  int dx, dy;

  map_distance_vector(&dx, &dy, tile0, tile1);
  return map_vector_to_distance(dx, dy);
}

/*************************************************************************
  This is used in mapgen for rivers going into ocen.  The name is 
  intentionally made awkward to prevent people from using it in place of
  is_ocean_near_tile
*************************************************************************/
bool is_cardinally_adj_to_ocean(const struct tile *ptile)
{
  return count_terrain_flag_near_tile(ptile, TRUE, FALSE, TER_OCEANIC) > 0;
}

/****************************************************************************
  Return TRUE if this ocean terrain is adjacent to a safe coastline.
****************************************************************************/
bool is_safe_ocean(const struct tile *ptile)
{
  adjc_iterate(ptile, adjc_tile) {
    if (tile_terrain(adjc_tile) != T_UNKNOWN
        && !terrain_has_flag(tile_terrain(adjc_tile), TER_UNSAFE_COAST)) {
      return TRUE;
    }
  } adjc_iterate_end;
  return FALSE;
}

/***************************************************************
...
***************************************************************/
bool is_water_adjacent_to_tile(const struct tile *ptile)
{
  struct terrain* pterrain = tile_terrain(ptile);

  if (T_UNKNOWN == pterrain) {
    return FALSE;
  }

  if (tile_has_special(ptile, S_RIVER)
   || tile_has_special(ptile, S_IRRIGATION)
   || terrain_has_flag(pterrain, TER_OCEANIC)) {
    return TRUE;
  }

  cardinal_adjc_iterate(ptile, tile1) {
    struct terrain* pterrain1 = tile_terrain(tile1);

    if (T_UNKNOWN == pterrain1) {
      continue;
    }

    if (tile_has_special(tile1, S_RIVER)
     || tile_has_special(tile1, S_IRRIGATION)
     || terrain_has_flag(pterrain1, TER_OCEANIC)) {
      return TRUE;
    }
  } cardinal_adjc_iterate_end;

  return FALSE;
}

/**************************************************************************
This function returns true if the tile at the given location can be
"reclaimed" from ocean into land.  This is the case only when there are
a sufficient number of adjacent tiles that are not ocean.
**************************************************************************/
bool can_reclaim_ocean(const struct tile *ptile)
{
  int land_tiles = 100 - count_ocean_near_tile(ptile, FALSE, TRUE);

  return land_tiles >= terrain_control.ocean_reclaim_requirement_pct;
}

/**************************************************************************
This function returns true if the tile at the given location can be
"channeled" from land into ocean.  This is the case only when there are
a sufficient number of adjacent tiles that are ocean.
**************************************************************************/
bool can_channel_land(const struct tile *ptile)
{
  int ocean_tiles = count_ocean_near_tile(ptile, FALSE, TRUE);

  return ocean_tiles >= terrain_control.land_channel_requirement_pct;
}

/***************************************************************
  The basic cost to move punit from tile t1 to tile t2.
  That is, tile_move_cost(), with pre-calculated tile pointers;
  the tiles are assumed to be adjacent, and the (x,y)
  values are used only to get the river bonus correct.

  May also be used with punit==NULL, in which case punit
  tests are not done (for unit-independent results).
***************************************************************/
static int tile_move_cost_ptrs(struct unit *punit,
			       const struct tile *t1, const struct tile *t2)
{
  bool cardinal_move;
  struct unit_class *pclass = NULL;
  bool native = TRUE;

  if (punit) {
    pclass = unit_class(punit);
    native = is_native_tile(unit_type(punit), t2);
  }

  if (game.info.slow_invasions
      && punit
      && tile_city(t1) == NULL
      && !is_native_tile(unit_type(punit), t1)
      && is_native_tile(unit_type(punit), t2)) {
    /* If "slowinvasions" option is turned on, units moving from
     * non-native terrain (from transport) to native terrain lose all their
     * movement.
     * e.g. ground units moving from sea to land */
    return punit->moves_left;
  }

  if (punit && !uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    return SINGLE_MOVE;
  }

  /* Railroad check has to be before F_IGTER check so that F_IGTER
   * units are not penalized. F_IGTER affects also entering and
   * leaving ships, so F_IGTER check has to be before native terrain
   * check. We want to give railroad bonus only to native units. */
  if (tile_has_special(t1, S_RAILROAD) && tile_has_special(t2, S_RAILROAD)
      && native) {
    return MOVE_COST_RAIL;
  }
  if (punit && unit_has_type_flag(punit, F_IGTER)) {
    return SINGLE_MOVE/3;
  }
  if (!native) {
    /* Loading to transport or entering port */
    return SINGLE_MOVE;
  }
  if (tile_has_special(t1, S_ROAD) && tile_has_special(t2, S_ROAD)) {
    return MOVE_COST_ROAD;
  }

  if (tile_has_special(t1, S_RIVER) && tile_has_special(t2, S_RIVER)) {
    cardinal_move = is_move_cardinal(t1, t2);
    switch (terrain_control.river_move_mode) {
    case RMV_NORMAL:
      break;
    case RMV_FAST_STRICT:
      if (cardinal_move)
	return MOVE_COST_RIVER;
      break;
    case RMV_FAST_RELAXED:
      if (cardinal_move)
	return MOVE_COST_RIVER;
      else
	return 2 * MOVE_COST_RIVER;
    case RMV_FAST_ALWAYS:
      return MOVE_COST_RIVER;
    default:
      break;
    }
  }

  return tile_terrain(t2)->movement_cost * SINGLE_MOVE;
}

/****************************************************************************
  map_move_cost_ai returns the move cost as
  calculated by tile_move_cost_ptrs (with no unit pointer to get
  unit-independent results) EXCEPT if either of the source or
  destination tile is an ocean tile. Then the result of the method
  shows if a ship can take the step from the source position to the
  destination position (return value is MOVE_COST_FOR_VALID_SEA_STEP)
  or not.  An arbitrarily high value will be returned if the move is
  impossible.

  FIXME: this function can't be used for air units because it returns
  sea<->land moves as impossible.
****************************************************************************/
int map_move_cost_ai(const struct tile *tile0, const struct tile *tile1)
{
  const int maxcost = 72; /* Arbitrary. */

  assert(!is_server()
	 || (tile_terrain(tile0) != T_UNKNOWN 
	  && tile_terrain(tile1) != T_UNKNOWN));

  /* A ship can take the step if:
   * - both tiles are ocean or
   * - one of the tiles is ocean and the other is a city or is unknown
   *
   * Note tileX->terrain will only be T_UNKNOWN at the client. */
  if (is_ocean_tile(tile0) && is_ocean_tile(tile1)) {
    return MOVE_COST_FOR_VALID_SEA_STEP;
  }

  if (is_ocean_tile(tile0)
      && (tile_city(tile1) || tile_terrain(tile1) == T_UNKNOWN)) {
    return MOVE_COST_FOR_VALID_SEA_STEP;
  }

  if (is_ocean_tile(tile1)
      && (tile_city(tile0) || tile_terrain(tile0) == T_UNKNOWN)) {
    return MOVE_COST_FOR_VALID_SEA_STEP;
  }

  if (is_ocean_tile(tile0) || is_ocean_tile(tile1)) {
    /* FIXME: Shouldn't this return MOVE_COST_FOR_VALID_AIR_STEP?
     * Note that MOVE_COST_FOR_VALID_AIR_STEP is currently equal to
     * MOVE_COST_FOR_VALID_SEA_STEP. */
    return maxcost;
  }

  return tile_move_cost_ptrs(NULL, tile0, tile1);
}

/***************************************************************
  The cost to move punit from where it is to tile x,y.
  It is assumed the move is a valid one, e.g. the tiles are adjacent.
***************************************************************/
int map_move_cost_unit(struct unit *punit, const struct tile *ptile)
{
  return tile_move_cost_ptrs(punit, punit->tile, ptile);
}

/***************************************************************
  Move cost between two tiles
***************************************************************/
int map_move_cost(const struct tile *src_tile, const struct tile *dst_tile)
{
  return tile_move_cost_ptrs(NULL, src_tile, dst_tile);
}

/***************************************************************
...
***************************************************************/
bool is_tiles_adjacent(const struct tile *tile0, const struct tile *tile1)
{
  return real_map_distance(tile0, tile1) == 1;
}

/***************************************************************
  Are (x1,y1) and (x2,y2) really the same when adjusted?
  This function might be necessary ALOT of places...
***************************************************************/
bool same_pos(const struct tile *tile1, const struct tile *tile2)
{
  assert(tile1 != NULL && tile2 != NULL);
  return (tile1 == tile2);
}

bool is_real_map_pos(int x, int y)
{
  return normalize_map_pos(&x, &y);
}

/**************************************************************************
Returns TRUE iff the map position is normal. "Normal" here means that
it is both a real/valid coordinate set and that the coordinates are in
their canonical/proper form. In plain English: the coordinates must be
on the map.
**************************************************************************/
bool is_normal_map_pos(int x, int y)
{
  int nat_x, nat_y;

  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);
  return nat_x >= 0 && nat_x < map.xsize && nat_y >= 0 && nat_y < map.ysize;
}

/**************************************************************************
  If the position is real, it will be normalized and TRUE will be returned.
  If the position is unreal, it will be left unchanged and FALSE will be
  returned.

  Note, we need to leave x and y with sane values even in the unreal case.
  Some callers may for instance call nearest_real_pos on these values.
**************************************************************************/
bool normalize_map_pos(int *x, int *y)
{
  struct tile *ptile = map_pos_to_tile(*x, *y);

  if (ptile) {
    *x = ptile->x;
    *y = ptile->y;
    return TRUE;
  } else {
    return FALSE;
  }
}

/**************************************************************************
Twiddle *x and *y to point the the nearest real tile, and ensure that the
position is normalized.
**************************************************************************/
struct tile *nearest_real_tile(int x, int y)
{
  int nat_x, nat_y;

  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);
  if (!topo_has_flag(TF_WRAPX)) {
    nat_x = CLIP(0, nat_x, map.xsize - 1);
  }
  if (!topo_has_flag(TF_WRAPY)) {
    nat_y = CLIP(0, nat_y, map.ysize - 1);
  }
  NATIVE_TO_MAP_POS(&x, &y, nat_x, nat_y);

  return map_pos_to_tile(x, y);
}

/**************************************************************************
Returns the total number of (real) positions (or tiles) on the map.
**************************************************************************/
int map_num_tiles(void)
{
  return map.xsize * map.ysize;
}

/****************************************************************************
  Finds the difference between the two (unnormalized) positions, in
  cartesian (map) coordinates.  Most callers should use map_distance_vector
  instead.
****************************************************************************/
void base_map_distance_vector(int *dx, int *dy,
			      int x0, int y0, int x1, int y1)
{
  if (topo_has_flag(TF_WRAPX) || topo_has_flag(TF_WRAPY)) {
    /* Wrapping is done in native coordinates. */
    MAP_TO_NATIVE_POS(&x0, &y0, x0, y0);
    MAP_TO_NATIVE_POS(&x1, &y1, x1, y1);

    /* Find the "native" distance vector. This corresponds closely to the
     * map distance vector but is easier to wrap. */
    *dx = x1 - x0;
    *dy = y1 - y0;
    if (topo_has_flag(TF_WRAPX)) {
      /* Wrap dx to be in [-map.xsize/2, map.xsize/2). */
      *dx = FC_WRAP(*dx + map.xsize / 2, map.xsize) - map.xsize / 2;
    }
    if (topo_has_flag(TF_WRAPY)) {
      /* Wrap dy to be in [-map.ysize/2, map.ysize/2). */
      *dy = FC_WRAP(*dy + map.ysize / 2, map.ysize) - map.ysize / 2;
    }

    /* Convert the native delta vector back to a pair of map positions. */
    x1 = x0 + *dx;
    y1 = y0 + *dy;
    NATIVE_TO_MAP_POS(&x0, &y0, x0, y0);
    NATIVE_TO_MAP_POS(&x1, &y1, x1, y1);
  }

  /* Find the final (map) vector. */
  *dx = x1 - x0;
  *dy = y1 - y0;
}

/****************************************************************************
  Topology function to find the vector which has the minimum "real"
  distance between the map positions (x0, y0) and (x1, y1).  If there is
  more than one vector with equal distance, no guarantee is made about
  which is found.

  Real distance is defined as the larger of the distances in the x and y
  direction; since units can travel diagonally this is the "real" distance
  a unit has to travel to get from point to point.

  (See also: real_map_distance, map_distance, and sq_map_distance.)

  With the standard topology the ranges of the return value are:
    -map.xsize/2 <= dx <= map.xsize/2
    -map.ysize   <  dy <  map.ysize
****************************************************************************/
void map_distance_vector(int *dx, int *dy,
			 const struct tile *tile0,
			 const struct tile *tile1)
{
  base_map_distance_vector(dx, dy,
			   tile0->x, tile0->y, tile1->x, tile1->y);
}

/**************************************************************************
Random neighbouring square.
**************************************************************************/
struct tile *rand_neighbour(const struct tile *ptile)
{
  int n;
  struct tile *tile1;

  /* 
   * list of all 8 directions 
   */
  enum direction8 dirs[8] = {
    DIR8_NORTHWEST, DIR8_NORTH, DIR8_NORTHEAST, DIR8_WEST, DIR8_EAST,
    DIR8_SOUTHWEST, DIR8_SOUTH, DIR8_SOUTHEAST
  };

  /* This clever loop by Trent Piepho will take no more than
   * 8 tries to find a valid direction. */
  for (n = 8; n > 0; n--) {
    enum direction8 choice = (enum direction8) myrand(n);

    /* this neighbour's OK */
    tile1 = mapstep(ptile, dirs[choice]);
    if (tile1) {
      return tile1;
    }

    /* Choice was bad, so replace it with the last direction in the list.
     * On the next iteration, one fewer choices will remain. */
    dirs[choice] = dirs[n - 1];
  }

  assert(0);			/* Are we on a 1x1 map with no wrapping??? */
  return NULL;
}

/**************************************************************************
 Random square anywhere on the map.  Only normal positions (for which
 is_normal_map_pos returns true) will be found.
**************************************************************************/
struct tile *rand_map_pos(void)
{
  int nat_x = myrand(map.xsize), nat_y = myrand(map.ysize);

  return native_pos_to_tile(nat_x, nat_y);
}

/**************************************************************************
  Give a random tile anywhere on the map for which the 'filter' function
  returns TRUE.  Return FALSE if none can be found.  The filter may be
  NULL if any position is okay; if non-NULL it shouldn't have any side
  effects.
**************************************************************************/
struct tile *rand_map_pos_filtered(void *data,
				   bool (*filter)(const struct tile *ptile,
						  const void *data))
{
  struct tile *ptile;
  int tries = 0;
  const int max_tries = MAP_INDEX_SIZE / ACTIVITY_FACTOR;

  /* First do a few quick checks to find a spot.  The limit on number of
   * tries could use some tweaking. */
  do {
    ptile = map.tiles + myrand(MAP_INDEX_SIZE);
  } while (filter && !filter(ptile, data) && ++tries < max_tries);

  /* If that fails, count all available spots and pick one.
   * Slow but reliable. */
  if (tries == max_tries) {
    int count = 0, positions[MAP_INDEX_SIZE];

    whole_map_iterate(ptile) {
      if (filter(ptile, data)) {
	positions[count] = tile_index(ptile);
	count++;
      }
    } whole_map_iterate_end;

    if (count == 0) {
      return NULL;
    }

    return map.tiles + positions[myrand(count)];
  } else {
    return ptile;
  }
}

/**************************************************************************
Return the debugging name of the direction.
**************************************************************************/
const char *dir_get_name(enum direction8 dir)
{
  /* a switch statement is used so the ordering can be changed easily */
  switch (dir) {
  case DIR8_NORTH:
    return "N";
  case DIR8_NORTHEAST:
    return "NE";
  case DIR8_EAST:
    return "E";
  case DIR8_SOUTHEAST:
    return "SE";
  case DIR8_SOUTH:
    return "S";
  case DIR8_SOUTHWEST:
    return "SW";
  case DIR8_WEST:
    return "W";
  case DIR8_NORTHWEST:
    return "NW";
  default:
    return "[Undef]";
  }
}

/**************************************************************************
  Returns the next direction clock-wise.
**************************************************************************/
enum direction8 dir_cw(enum direction8 dir)
{
  /* a switch statement is used so the ordering can be changed easily */
  switch (dir) {
  case DIR8_NORTH:
    return DIR8_NORTHEAST;
  case DIR8_NORTHEAST:
    return DIR8_EAST;
  case DIR8_EAST:
    return DIR8_SOUTHEAST;
  case DIR8_SOUTHEAST:
    return DIR8_SOUTH;
  case DIR8_SOUTH:
    return DIR8_SOUTHWEST;
  case DIR8_SOUTHWEST:
    return DIR8_WEST;
  case DIR8_WEST:
    return DIR8_NORTHWEST;
  case DIR8_NORTHWEST:
    return DIR8_NORTH;
  default:
    assert(0);
    return -1;
  }
}

/**************************************************************************
  Returns the next direction counter-clock-wise.
**************************************************************************/
enum direction8 dir_ccw(enum direction8 dir)
{
  /* a switch statement is used so the ordering can be changed easily */
  switch (dir) {
  case DIR8_NORTH:
    return DIR8_NORTHWEST;
  case DIR8_NORTHEAST:
    return DIR8_NORTH;
  case DIR8_EAST:
    return DIR8_NORTHEAST;
  case DIR8_SOUTHEAST:
    return DIR8_EAST;
  case DIR8_SOUTH:
    return DIR8_SOUTHEAST;
  case DIR8_SOUTHWEST:
    return DIR8_SOUTH;
  case DIR8_WEST:
    return DIR8_SOUTHWEST;
  case DIR8_NORTHWEST:
    return DIR8_WEST;
  default:
    assert(0);
    return -1;
  }
}

/**************************************************************************
  Returns TRUE iff the given direction is a valid one.
**************************************************************************/
bool is_valid_dir(enum direction8 dir)
{
  switch (dir) {
  case DIR8_SOUTHEAST:
  case DIR8_NORTHWEST:
    /* These directions are invalid in hex topologies. */
    return !(topo_has_flag(TF_HEX) && !topo_has_flag(TF_ISO));
  case DIR8_NORTHEAST:
  case DIR8_SOUTHWEST:
    /* These directions are invalid in iso-hex topologies. */
    return !(topo_has_flag(TF_HEX) && topo_has_flag(TF_ISO));
  case DIR8_NORTH:
  case DIR8_EAST:
  case DIR8_SOUTH:
  case DIR8_WEST:
    return TRUE;
  default:
    return FALSE;
  }
}

/**************************************************************************
  Returns TRUE iff the given direction is a cardinal one.

  Cardinal directions are those in which adjacent tiles share an edge not
  just a vertex.
**************************************************************************/
bool is_cardinal_dir(enum direction8 dir)
{
  switch (dir) {
  case DIR8_NORTH:
  case DIR8_SOUTH:
  case DIR8_EAST:
  case DIR8_WEST:
    return TRUE;
  case DIR8_SOUTHEAST:
  case DIR8_NORTHWEST:
    /* These directions are cardinal in iso-hex topologies. */
    return topo_has_flag(TF_HEX) && topo_has_flag(TF_ISO);
  case DIR8_NORTHEAST:
  case DIR8_SOUTHWEST:
    /* These directions are cardinal in hexagonal topologies. */
    return topo_has_flag(TF_HEX) && !topo_has_flag(TF_ISO);
  }
  return FALSE;
}

/**************************************************************************
Return true and sets dir to the direction of the step if (end_x,
end_y) can be reached from (start_x, start_y) in one step. Return
false otherwise (value of dir is unchanged in this case).
**************************************************************************/
bool base_get_direction_for_step(const struct tile *start_tile,
				 const struct tile *end_tile,
				 enum direction8 *dir)
{
  adjc_dir_iterate(start_tile, test_tile, test_dir) {
    if (same_pos(end_tile, test_tile)) {
      *dir = test_dir;
      return TRUE;
    }
  } adjc_dir_iterate_end;

  return FALSE;
}

/**************************************************************************
Return the direction which is needed for a step on the map from
(start_x, start_y) to (end_x, end_y).
**************************************************************************/
int get_direction_for_step(const struct tile *start_tile,
			   const struct tile *end_tile)
{
  enum direction8 dir;

  if (base_get_direction_for_step(start_tile, end_tile, &dir)) {
    return dir;
  }

  assert(0);
  return -1;
}

/**************************************************************************
  Returns TRUE iff the move from the position (start_x,start_y) to
  (end_x,end_y) is a cardinal one.
**************************************************************************/
bool is_move_cardinal(const struct tile *start_tile,
		      const struct tile *end_tile)
{
  return is_cardinal_dir(get_direction_for_step(start_tile, end_tile));
}

/****************************************************************************
  A "SINGULAR" position is any map position that has an abnormal number of
  tiles in the radius of dist.

  (map_x, map_y) must be normalized.

  dist is the "real" map distance.
****************************************************************************/
bool is_singular_tile(const struct tile *ptile, int dist)
{
  do_in_natural_pos(ntl_x, ntl_y, ptile->x, ptile->y) {
    /* Iso-natural coordinates are doubled in scale. */
    dist *= MAP_IS_ISOMETRIC ? 2 : 1;

    return ((!topo_has_flag(TF_WRAPX) 
	     && (ntl_x < dist || ntl_x >= NATURAL_WIDTH - dist))
	    || (!topo_has_flag(TF_WRAPY)
		&& (ntl_y < dist || ntl_y >= NATURAL_HEIGHT - dist)));
  } do_in_natural_pos_end;
}

/****************************************************************************
  Set a start position at the given tile for the given nation. Clears any
  existing start position at the tile. The 'pnation' argument may be NULL,
  in which case this function has the same effect as map_clear_startpos().
****************************************************************************/
void map_set_startpos(const struct tile *ptile,
                      const struct nation_type *pnation)
{
  struct startpos_entry *spe;

  if (!map.startpos_table || !ptile) {
    return;
  }
  map_clear_startpos(ptile);

  if (!pnation) {
    return;
  }

  spe = fc_calloc(1, sizeof(*spe));
  spe->key = tile_index(ptile);
  spe->nation = pnation;

  hash_insert(map.startpos_table, &spe->key, spe);
}

/****************************************************************************
  Returns the nation of the start position at the given tile, or NULL if
  none exists there.
****************************************************************************/
const struct nation_type *map_get_startpos(const struct tile *ptile)
{
  struct startpos_entry *spe;
  int key;

  if (!map.startpos_table || !ptile) {
    return NULL;
  }

  key = tile_index(ptile);
  spe = hash_lookup_data(map.startpos_table, &key);
  if (!spe) {
    return NULL;
  }

  return spe->nation;
}

/****************************************************************************
  Remove a start position at the given tile.
****************************************************************************/
void map_clear_startpos(const struct tile *ptile)
{
  int key;

  if (!map.startpos_table || !ptile) {
    return;
  }

  key = tile_index(ptile);
  hash_delete_entry(map.startpos_table, &key);
}
