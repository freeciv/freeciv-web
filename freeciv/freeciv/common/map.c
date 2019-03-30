/***********************************************************************
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
#include <fc_config.h>
#endif
#include <string.h>             /* strlen */

/* utility */
#include "fcintl.h"
#include "iterator.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "ai.h"
#include "city.h"
#include "game.h"
#include "movement.h"
#include "nation.h"
#include "packets.h"
#include "road.h"
#include "unit.h"
#include "unitlist.h"

#include "map.h"

struct startpos {
  struct tile *location;
  bool exclude;
  struct nation_hash *nations;
};

static struct startpos *startpos_new(struct tile *ptile);
static void startpos_destroy(struct startpos *psp);

/* struct startpos_hash and related functions. */
#define SPECHASH_TAG startpos
#define SPECHASH_IKEY_TYPE struct tile *
#define SPECHASH_IDATA_TYPE struct startpos *
#define SPECHASH_IDATA_FREE startpos_destroy
#include "spechash.h"

/* Srart position iterator. */
struct startpos_iter {
  struct iterator vtable;
  const struct startpos *psp;
  /* 'struct nation_iter' really. See startpos_iter_sizeof(). */
  struct iterator nation_iter;
};

#define STARTPOS_ITER(p) ((struct startpos_iter *) (p))


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

static bool dir_cardinality[9]; /* Including invalid one */
static bool dir_validity[9];    /* Including invalid one */

static bool is_valid_dir_calculate(enum direction8 dir);
static bool is_cardinal_dir_calculate(enum direction8 dir);

static bool restrict_infra(const struct player *pplayer, const struct tile *t1,
                           const struct tile *t2);

/*******************************************************************//**
  Return a bitfield of the extras on the tile that are infrastructure.
***********************************************************************/
bv_extras get_tile_infrastructure_set(const struct tile *ptile,
                                      int *pcount)
{
  bv_extras pspresent;
  int count = 0;

  BV_CLR_ALL(pspresent);

  extra_type_iterate(pextra) {
    if (is_extra_removed_by(pextra, ERM_PILLAGE) && tile_has_extra(ptile, pextra)) {
      struct tile *missingset = tile_virtual_new(ptile);
      bool dependency = FALSE;

      tile_remove_extra(missingset, pextra);
      extra_type_iterate(pdependant) {
        if (tile_has_extra(ptile, pdependant)) {
          if (!are_reqs_active(NULL, NULL, NULL, NULL, missingset,
                               NULL, NULL, NULL, NULL, NULL,
                               &pdependant->reqs, RPT_POSSIBLE)) {
            dependency = TRUE;
            break;
          }
        }
      } extra_type_iterate_end;

      tile_virtual_destroy(missingset);

      if (!dependency) {
        BV_SET(pspresent, extra_index(pextra));
        count++;
      }
    }
  } extra_type_iterate_end;

  if (pcount) {
    *pcount = count;
  }

  return pspresent;
}

/*******************************************************************//**
  Returns TRUE if we are at a stage of the game where the map
  has not yet been generated/loaded.
  (To be precise, returns TRUE if map_allocate() has not yet been
  called.)
***********************************************************************/
bool map_is_empty(void)
{
  return wld.map.tiles == NULL;
}

/*******************************************************************//**
  Put some sensible values into the map structure
***********************************************************************/
void map_init(struct civ_map *imap, bool server_side)
{
  imap->topology_id = MAP_DEFAULT_TOPO;
  imap->num_continents = 0;
  imap->num_oceans = 0;
  imap->tiles = NULL;
  imap->startpos_table = NULL;
  imap->iterate_outwards_indices = NULL;

  /* The [xy]size values are set in map_init_topology.  It is initialized
   * to a non-zero value because some places erronously use these values
   * before they're initialized. */
  imap->xsize = MAP_DEFAULT_LINEAR_SIZE;
  imap->ysize = MAP_DEFAULT_LINEAR_SIZE;

  if (server_side) {
    imap->server.mapsize = MAP_DEFAULT_MAPSIZE;
    imap->server.size = MAP_DEFAULT_SIZE;
    imap->server.tilesperplayer = MAP_DEFAULT_TILESPERPLAYER;
    imap->server.seed_setting = MAP_DEFAULT_SEED;
    imap->server.seed = MAP_DEFAULT_SEED;
    imap->server.riches = MAP_DEFAULT_RICHES;
    imap->server.huts = MAP_DEFAULT_HUTS;
    imap->server.huts_absolute = -1;
    imap->server.animals = MAP_DEFAULT_ANIMALS;
    imap->server.landpercent = MAP_DEFAULT_LANDMASS;
    imap->server.wetness = MAP_DEFAULT_WETNESS;
    imap->server.steepness = MAP_DEFAULT_STEEPNESS;
    imap->server.generator = MAP_DEFAULT_GENERATOR;
    imap->server.startpos = MAP_DEFAULT_STARTPOS;
    imap->server.tinyisles = MAP_DEFAULT_TINYISLES;
    imap->server.separatepoles = MAP_DEFAULT_SEPARATE_POLES;
    imap->server.single_pole = MAP_DEFAULT_SINGLE_POLE;
    imap->server.alltemperate = MAP_DEFAULT_ALLTEMPERATE;
    imap->server.temperature = MAP_DEFAULT_TEMPERATURE;
    imap->server.have_huts = FALSE;
    imap->server.have_resources = FALSE;
    imap->server.team_placement = MAP_DEFAULT_TEAM_PLACEMENT;
  }
}

/*******************************************************************//**
  Fill the iterate_outwards_indices array.  This may depend on the topology.
***********************************************************************/
static void generate_map_indices(void)
{
  int i = 0, nat_x, nat_y, tiles;
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
  nat_center_x = wld.map.xsize / 2;
  nat_center_y = wld.map.ysize / 2;
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
  nat_min_x = (current_topo_has_flag(TF_WRAPX) ? 0 : (nat_center_x - wld.map.xsize + 1));
  nat_min_y = (current_topo_has_flag(TF_WRAPY) ? 0 : (nat_center_y - wld.map.ysize + 1));

  nat_max_x = (current_topo_has_flag(TF_WRAPX)
	       ? (wld.map.xsize - 1)
	       : (nat_center_x + wld.map.xsize - 1));
  nat_max_y = (current_topo_has_flag(TF_WRAPY)
	       ? (wld.map.ysize - 1)
	       : (nat_center_y + wld.map.ysize - 1));
  tiles = (nat_max_x - nat_min_x + 1) * (nat_max_y - nat_min_y + 1);

  fc_assert(NULL == wld.map.iterate_outwards_indices);
  wld.map.iterate_outwards_indices =
      fc_malloc(tiles * sizeof(*wld.map.iterate_outwards_indices));

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

      wld.map.iterate_outwards_indices[i].dx = dx;
      wld.map.iterate_outwards_indices[i].dy = dy;
      wld.map.iterate_outwards_indices[i].dist =
          map_vector_to_real_distance(dx, dy);
      i++;
    }
  }
  fc_assert(i == tiles);

  qsort(wld.map.iterate_outwards_indices, tiles,
        sizeof(*wld.map.iterate_outwards_indices), compare_iter_index);

#if 0
  for (i = 0; i < tiles; i++) {
    log_debug("%5d : (%3d,%3d) : %d", i,
              wld.map.iterate_outwards_indices[i].dx,
              wld.map.iterate_outwards_indices[i].dy,
              wld.map.iterate_outwards_indices[i].dist);
  }
#endif

  wld.map.num_iterate_outwards_indices = tiles;
}

/*******************************************************************//**
  map_init_topology needs to be called after map.topology_id is changed.

  map.xsize and map.ysize must be set before calling map_init_topology().
  This is done by the map generator code (server), when loading a savegame
  or a scenario with map (server), and packhand code (client).
***********************************************************************/
void map_init_topology(void)
{
  enum direction8 dir;

  /* sanity check for iso topologies*/
  fc_assert(!MAP_IS_ISOMETRIC || (wld.map.ysize % 2) == 0);

  /* The size and ratio must satisfy the minimum and maximum *linear*
   * restrictions on width */
  fc_assert(wld.map.xsize >= MAP_MIN_LINEAR_SIZE);
  fc_assert(wld.map.ysize >= MAP_MIN_LINEAR_SIZE);
  fc_assert(wld.map.xsize <= MAP_MAX_LINEAR_SIZE);
  fc_assert(wld.map.ysize <= MAP_MAX_LINEAR_SIZE);
  fc_assert(map_num_tiles() >= MAP_MIN_SIZE * 1000);
  fc_assert(map_num_tiles() <= MAP_MAX_SIZE * 1000);

  wld.map.num_valid_dirs = wld.map.num_cardinal_dirs = 0;

  /* Values for direction8_invalid() */
  fc_assert(direction8_invalid() == 8);
  dir_validity[8] = FALSE;
  dir_cardinality[8] = FALSE;

  /* Values for actual directions */
  for (dir = 0; dir < 8; dir++) {
    if (is_valid_dir_calculate(dir)) {
      wld.map.valid_dirs[wld.map.num_valid_dirs] = dir;
      wld.map.num_valid_dirs++;
      dir_validity[dir] = TRUE;
    } else {
      dir_validity[dir] = FALSE;
    }
    if (is_cardinal_dir_calculate(dir)) {
      wld.map.cardinal_dirs[wld.map.num_cardinal_dirs] = dir;
      wld.map.num_cardinal_dirs++;
      dir_cardinality[dir] = TRUE;
    } else {
      dir_cardinality[dir] = FALSE;
    }
  }
  fc_assert(wld.map.num_valid_dirs > 0 && wld.map.num_valid_dirs <= 8);
  fc_assert(wld.map.num_cardinal_dirs > 0
            && wld.map.num_cardinal_dirs <= wld.map.num_valid_dirs);
}

/*******************************************************************//**
  Initialize tile structure
***********************************************************************/
static void tile_init(struct tile *ptile)
{
  ptile->continent = 0;

  BV_CLR_ALL(ptile->extras);
  ptile->resource = NULL;
  ptile->terrain  = T_UNKNOWN;
  ptile->units    = unit_list_new();
  ptile->owner    = NULL; /* Not claimed by any player. */
  ptile->extras_owner = NULL;
  ptile->claimer  = NULL;
  ptile->worked   = NULL; /* No city working here. */
  ptile->spec_sprite = NULL;
}

/*******************************************************************//**
  Step from the given tile in the given direction.  The new tile is returned,
  or NULL if the direction is invalid or leads off the map.
***********************************************************************/
struct tile *mapstep(const struct civ_map *nmap,
                     const struct tile *ptile, enum direction8 dir)
{
  int dx, dy, tile_x, tile_y;

  if (!is_valid_dir(dir)) {
    return NULL;
  }

  index_to_map_pos(&tile_x, &tile_y, tile_index(ptile)); 
  DIRSTEP(dx, dy, dir);

  tile_x += dx;
  tile_y += dy;

  return map_pos_to_tile(&(wld.map), tile_x, tile_y);
}

/*******************************************************************//**
  Return the tile for the given native position, with wrapping.

  This is a backend function used by map_pos_to_tile and native_pos_to_tile.
  It is called extremely often so it is made inline.
***********************************************************************/
static inline struct tile *base_native_pos_to_tile(const struct civ_map *nmap,
                                                   int nat_x, int nat_y)
{
  /* Wrap in X and Y directions, as needed. */
  /* If the position is out of range in a non-wrapping direction, it is
   * unreal. */
  if (current_topo_has_flag(TF_WRAPX)) {
    nat_x = FC_WRAP(nat_x, wld.map.xsize);
  } else if (nat_x < 0 || nat_x >= wld.map.xsize) {
    return NULL;
  }
  if (current_topo_has_flag(TF_WRAPY)) {
    nat_y = FC_WRAP(nat_y, wld.map.ysize);
  } else if (nat_y < 0 || nat_y >= wld.map.ysize) {
    return NULL;
  }

  /* We already checked legality of native pos above, don't repeat */
  return nmap->tiles + native_pos_to_index_nocheck(nat_x, nat_y);
}

/*******************************************************************//**
  Return the tile for the given cartesian (map) position.
***********************************************************************/
struct tile *map_pos_to_tile(const struct civ_map *nmap, int map_x, int map_y)
{
  /* Instead of introducing new variables for native coordinates,
   * update the map coordinate variables = registers already in use.
   * This is one of the most performance-critical functions we have,
   * so taking measures like this makes sense. */
#define nat_x map_x
#define nat_y map_y

  if (nmap->tiles == NULL) {
    return NULL;
  }

  /* Normalization is best done in native coordinates. */
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, map_x, map_y);
  return base_native_pos_to_tile(nmap, nat_x, nat_y);

#undef nat_x
#undef nat_y
}

/*******************************************************************//**
  Return the tile for the given native position.
***********************************************************************/
struct tile *native_pos_to_tile(const struct civ_map *nmap,
                                int nat_x, int nat_y)
{
  if (nmap->tiles == NULL) {
    return NULL;
  }

  return base_native_pos_to_tile(nmap, nat_x, nat_y);
}

/*******************************************************************//**
  Return the tile for the given index position.
***********************************************************************/
struct tile *index_to_tile(const struct civ_map *imap, int mindex)
{
  if (!imap->tiles) {
    return NULL;
  }

  if (mindex >= 0 && mindex < MAP_INDEX_SIZE) {
    return imap->tiles + mindex;
  } else {
    /* Unwrapped index coordinates are impossible, so the best we can do is
     * return NULL. */
    return NULL;
  }
}

/*******************************************************************//**
  Free memory associated with one tile.
***********************************************************************/
static void tile_free(struct tile *ptile)
{
  unit_list_destroy(ptile->units);

  if (ptile->spec_sprite) {
    free(ptile->spec_sprite);
    ptile->spec_sprite = NULL;
  }

  if (ptile->label) {
    FC_FREE(ptile->label);
    ptile->label = NULL;
  }
}

/*******************************************************************//**
  Allocate space for map, and initialise the tiles.
  Uses current map.xsize and map.ysize.
***********************************************************************/
void map_allocate(struct civ_map *amap)
{
  log_debug("map_allocate (was %p) (%d,%d)",
            (void *) amap->tiles, amap->xsize, amap->ysize);

  fc_assert_ret(NULL == amap->tiles);
  amap->tiles = fc_calloc(MAP_INDEX_SIZE, sizeof(*amap->tiles));

  /* Note this use of whole_map_iterate may be a bit sketchy, since the
   * tile values (ptile->index, etc.) haven't been set yet.  It might be
   * better to do a manual loop here. */
  whole_map_iterate(amap, ptile) {
    ptile->index = ptile - amap->tiles;
    CHECK_INDEX(tile_index(ptile));
    tile_init(ptile);
  } whole_map_iterate_end;

  if (amap->startpos_table != NULL) {
    startpos_hash_destroy(amap->startpos_table);
  }
  amap->startpos_table = startpos_hash_new();
}

/*******************************************************************//**
  Allocate main map and related global structures.
***********************************************************************/
void main_map_allocate(void)
{
  map_allocate(&(wld.map));
  generate_city_map_indices();
  generate_map_indices();
  CALL_FUNC_EACH_AI(map_alloc);
}

/*******************************************************************//**
  Frees the allocated memory of the map.
***********************************************************************/
void map_free(struct civ_map *fmap)
{
  if (fmap->tiles) {
    /* it is possible that map_init was called but not map_allocate */

    whole_map_iterate(fmap, ptile) {
      tile_free(ptile);
    } whole_map_iterate_end;

    free(fmap->tiles);
    fmap->tiles = NULL;

    if (fmap->startpos_table) {
      startpos_hash_destroy(fmap->startpos_table);
      fmap->startpos_table = NULL;
    }

    FC_FREE(fmap->iterate_outwards_indices);
  }
}

/*******************************************************************//**
  Free main map and related global structures.
***********************************************************************/
void main_map_free(void)
{
  map_free(&(wld.map));
  CALL_FUNC_EACH_AI(map_free);
}

/*******************************************************************//**
  Return the "distance" (which is really the Manhattan distance, and should
  rarely be used) for a given vector.
***********************************************************************/
static int map_vector_to_distance(int dx, int dy)
{
  if (current_topo_has_flag(TF_HEX)) {
    /* Hex: all directions are cardinal so the distance is equivalent to
     * the real distance. */
    return map_vector_to_real_distance(dx, dy);
  } else {
    return abs(dx) + abs(dy);
  }
}

/*******************************************************************//**
  Return the "real" distance for a given vector.
***********************************************************************/
int map_vector_to_real_distance(int dx, int dy)
{
  const int absdx = abs(dx), absdy = abs(dy);

  if (current_topo_has_flag(TF_HEX)) {
    if (current_topo_has_flag(TF_ISO)) {
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

/*******************************************************************//**
  Return the sq_distance for a given vector.
***********************************************************************/
int map_vector_to_sq_distance(int dx, int dy)
{
  if (current_topo_has_flag(TF_HEX)) {
    /* Hex: The square distance is just the square of the real distance; we
     * don't worry about pythagorean calculations. */
    int dist = map_vector_to_real_distance(dx, dy);

    return dist * dist;
  } else {
    return dx * dx + dy * dy;
  }
}

/*******************************************************************//**
  Return real distance between two tiles.
***********************************************************************/
int real_map_distance(const struct tile *tile0, const struct tile *tile1)
{
  int dx, dy;

  map_distance_vector(&dx, &dy, tile0, tile1);
  return map_vector_to_real_distance(dx, dy);
}

/*******************************************************************//**
  Return squared distance between two tiles.
***********************************************************************/
int sq_map_distance(const struct tile *tile0, const struct tile *tile1)
{
  /* We assume map_distance_vector gives us the vector with the
     minimum squared distance. Right now this is true. */
  int dx, dy;

  map_distance_vector(&dx, &dy, tile0, tile1);
  return map_vector_to_sq_distance(dx, dy);
}

/*******************************************************************//**
  Return Manhattan distance between two tiles.
***********************************************************************/
int map_distance(const struct tile *tile0, const struct tile *tile1)
{
  /* We assume map_distance_vector gives us the vector with the
     minimum map distance. Right now this is true. */
  int dx, dy;

  map_distance_vector(&dx, &dy, tile0, tile1);
  return map_vector_to_distance(dx, dy);
}

/*******************************************************************//**
  Return TRUE if this ocean terrain is adjacent to a safe coastline.
***********************************************************************/
bool is_safe_ocean(const struct civ_map *nmap, const struct tile *ptile)
{
  adjc_iterate(nmap, ptile, adjc_tile) {
    if (tile_terrain(adjc_tile) != T_UNKNOWN
        && !terrain_has_flag(tile_terrain(adjc_tile), TER_UNSAFE_COAST)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/*******************************************************************//**
  This function returns true if the tile at the given location can be
  "reclaimed" from ocean into land.  This is the case only when there are
  a sufficient number of adjacent tiles that are not ocean.
***********************************************************************/
bool can_reclaim_ocean(const struct tile *ptile)
{
  int land_tiles = 100 - count_terrain_class_near_tile(ptile, FALSE, TRUE,
                                                       TC_OCEAN);

  return land_tiles >= terrain_control.ocean_reclaim_requirement_pct;
}

/*******************************************************************//**
  This function returns true if the tile at the given location can be
  "channeled" from land into ocean.  This is the case only when there are
  a sufficient number of adjacent tiles that are ocean.
***********************************************************************/
bool can_channel_land(const struct tile *ptile)
{
  int ocean_tiles = count_terrain_class_near_tile(ptile, FALSE, TRUE, TC_OCEAN);

  return ocean_tiles >= terrain_control.land_channel_requirement_pct;
}

/*******************************************************************//**
  Returns true if the tile at the given location can be thawed from
  terrain with a 'Frozen' flag to terrain without. This requires a
  sufficient number of adjacent unfrozen tiles.
***********************************************************************/
bool can_thaw_terrain(const struct tile *ptile)
{
  int unfrozen_tiles = 100 - count_terrain_flag_near_tile(ptile, FALSE, TRUE,
                                                          TER_FROZEN);

  return unfrozen_tiles >= terrain_control.terrain_thaw_requirement_pct;
}

/*******************************************************************//**
  Returns true if the tile at the given location can be turned from
  terrain without a 'Frozen' flag to terrain with. This requires a
  sufficient number of adjacent frozen tiles.
***********************************************************************/
bool can_freeze_terrain(const struct tile *ptile)
{
  int frozen_tiles = count_terrain_flag_near_tile(ptile, FALSE, TRUE,
                                                  TER_FROZEN);

  return frozen_tiles >= terrain_control.terrain_freeze_requirement_pct;
}

/*******************************************************************//**
  Returns FALSE if a terrain change to 'pterrain' would be prevented by not
  having enough similar terrain surrounding ptile.
***********************************************************************/
bool terrain_surroundings_allow_change(const struct tile *ptile,
                                       const struct terrain *pterrain)
{
  bool ret = TRUE;

  if (is_ocean(tile_terrain(ptile))
      && !is_ocean(pterrain) && !can_reclaim_ocean(ptile)) {
    ret = FALSE;
  } else if (!is_ocean(tile_terrain(ptile))
             && is_ocean(pterrain) && !can_channel_land(ptile)) {
    ret = FALSE;
  }

  if (ret) {
    if (terrain_has_flag(tile_terrain(ptile), TER_FROZEN)
        && !terrain_has_flag(pterrain, TER_FROZEN)
        && !can_thaw_terrain(ptile)) {
      ret = FALSE;
    } else if (!terrain_has_flag(tile_terrain(ptile), TER_FROZEN)
               && terrain_has_flag(pterrain, TER_FROZEN)
               && !can_freeze_terrain(ptile)) {
      ret = FALSE;
    }
  }

  return ret;
}

/*******************************************************************//**
  The basic cost to move punit from tile t1 to tile t2.
  That is, tile_move_cost(), with pre-calculated tile pointers;
  the tiles are assumed to be adjacent, and the (x,y)
  values are used only to get the river bonus correct.

  May also be used with punit == NULL, in which case punit
  tests are not done (for unit-independent results).
***********************************************************************/
int tile_move_cost_ptrs(const struct civ_map *nmap,
                        const struct unit *punit,
                        const struct unit_type *punittype,
                        const struct player *pplayer,
                        const struct tile *t1, const struct tile *t2)
{
  const struct unit_class *pclass = utype_class(punittype);
  int cost;
  bool cardinality_checked = FALSE;
  bool cardinal_move BAD_HEURISTIC_INIT(FALSE);
  bool ri;

  /* Try to exit early for detectable conditions */
  if (!uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    /* units without UCF_TERRAIN_SPEED have a constant cost. */
    return SINGLE_MOVE;

  } else if (!is_native_tile_to_class(pclass, t2)) {
    /* Loading to transport or entering port.
     * UTYF_IGTER units get move benefit. */
    return (utype_has_flag(punittype, UTYF_IGTER)
            ? MOVE_COST_IGTER : SINGLE_MOVE);

  } else if (!is_native_tile_to_class(pclass, t1)) {
    if (game.info.slow_invasions
        && !(punit && unit_has_type_flag(punit, UTYF_BEACH_LANDER))
        && tile_city(t1) == NULL) {
      /* If "slowinvasions" option is turned on, units moving from
       * non-native terrain (from transport) to native terrain lose all
       * their movement unless they have the BeachLander unit type flag.
       * e.g. ground units moving from sea to land */
      if (punit != NULL) {
        return punit->moves_left;
      } else {
        /* Needs to be bigger than SINGLE_MOVE * move_rate * MAX(1, fuel)
         * for the most mobile unit possible. */
        return FC_INFINITY;
      }
    } else {
      /* Disembarking from transport or leaving port.
       * UTYF_IGTER units get move benefit. */
      return (utype_has_flag(punittype, UTYF_IGTER)
              ? MOVE_COST_IGTER : SINGLE_MOVE);
    }
  }

  cost = tile_terrain(t2)->movement_cost * SINGLE_MOVE;
  ri = restrict_infra(pplayer, t1, t2);

  extra_type_list_iterate(pclass->cache.bonus_roads, pextra) {
    struct road_type *proad = extra_road_get(pextra);

    /* We check the destination tile first, as that's
     * the end of move that determines the cost.
     * If can avoid inner loop about integrating roads
     * completely if the destination road has too high cost. */

    if (cost > proad->move_cost
        && (!ri || road_has_flag(proad, RF_UNRESTRICTED_INFRA))
        && tile_has_extra(t2, pextra)) {
      extra_type_list_iterate(proad->integrators, iextra) {
        /* We have no unrestricted infra related check here,
         * destination road is the one that counts. */
        if (tile_has_extra(t1, iextra)
            && is_native_extra_to_uclass(iextra, pclass)) {
          if (proad->move_mode == RMM_FAST_ALWAYS) {
            cost = proad->move_cost;
          } else {
            if (!cardinality_checked) {
              cardinal_move = (ALL_DIRECTIONS_CARDINAL()
                               || is_move_cardinal(nmap, t1, t2));
              cardinality_checked = TRUE;
            }
            if (cardinal_move) {
              cost = proad->move_cost;
            } else {
              switch (proad->move_mode) {
              case RMM_CARDINAL:
                break;
              case RMM_RELAXED:
                if (cost > proad->move_cost * 2) {
                  cardinal_between_iterate(nmap, t1, t2, between) {
                    if (tile_has_extra(between, pextra)
                        || (pextra != iextra && tile_has_extra(between, iextra))) {
                      /* 'pextra != iextra' is there just to avoid tile_has_extra()
                       * in by far more common case that 'pextra == iextra' */
                      /* TODO: Should we restrict this more?
                       * Should we check against enemy cities on between tile?
                       * Should we check against non-native terrain on between tile?
                       */
                      cost = proad->move_cost * 2;
                    }
                  } cardinal_between_iterate_end;
                }
                break;
              case RMM_FAST_ALWAYS:
                fc_assert(proad->move_mode != RMM_FAST_ALWAYS); /* Already handled above */
                cost = proad->move_cost;
                break;
              }
            }
          }
        }
      } extra_type_list_iterate_end;
    }
  } extra_type_list_iterate_end;

  /* UTYF_IGTER units have a maximum move cost per step. */
  if (utype_has_flag(punittype, UTYF_IGTER) && MOVE_COST_IGTER < cost) {
    cost = MOVE_COST_IGTER;
  }

  if (terrain_control.pythagorean_diagonal) {
    if (!cardinality_checked) {
      cardinal_move = (ALL_DIRECTIONS_CARDINAL()
                       || is_move_cardinal(nmap, t1, t2));
    }
    if (!cardinal_move) {
      return (int) (cost * 1.41421356f);
    }
  }

  return cost;
}

/*******************************************************************//**
  Returns TRUE if there is a restriction with regard to the infrastructure,
  i.e. at least one of the tiles t1 and t2 is claimed by a unfriendly
  nation. This means that one can not use of the infrastructure (road,
  railroad) on this tile.
***********************************************************************/
static bool restrict_infra(const struct player *pplayer, const struct tile *t1,
                           const struct tile *t2)
{
  struct player *plr1 = tile_owner(t1), *plr2 = tile_owner(t2);

  if (!pplayer || !game.info.restrictinfra) {
    return FALSE;
  }

  if ((plr1 && pplayers_at_war(plr1, pplayer))
      || (plr2 && pplayers_at_war(plr2, pplayer))) {
    return TRUE;
  }

  return FALSE;
}

/*******************************************************************//**
  Are two tiles adjacent to each other.
***********************************************************************/
bool is_tiles_adjacent(const struct tile *tile0, const struct tile *tile1)
{
  return real_map_distance(tile0, tile1) == 1;
}

/*******************************************************************//**
  Are (x1,y1) and (x2,y2) really the same when adjusted?
  This function might be necessary ALOT of places...
***********************************************************************/
bool same_pos(const struct tile *tile1, const struct tile *tile2)
{
  fc_assert_ret_val(tile1 != NULL && tile2 != NULL, FALSE);

  /* In case of virtual tile, tile1 can be different from tile2,
   * but they have same index */
  return (tile1->index == tile2->index);
}

/*******************************************************************//**
  Is given position real position
***********************************************************************/
bool is_real_map_pos(const struct civ_map *nmap, int x, int y)
{
  return normalize_map_pos(nmap, &x, &y);
}

/*******************************************************************//**
  Returns TRUE iff the map position is normal. "Normal" here means that
  it is both a real/valid coordinate set and that the coordinates are in
  their canonical/proper form. In plain English: the coordinates must be
  on the map.
***********************************************************************/
bool is_normal_map_pos(int x, int y)
{
  int nat_x, nat_y;

  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);
  return nat_x >= 0 && nat_x < wld.map.xsize && nat_y >= 0 && nat_y < wld.map.ysize;
}

/*******************************************************************//**
  If the position is real, it will be normalized and TRUE will be returned.
  If the position is unreal, it will be left unchanged and FALSE will be
  returned.

  Note, we need to leave x and y with sane values even in the unreal case.
  Some callers may for instance call nearest_real_pos on these values.
***********************************************************************/
bool normalize_map_pos(const struct civ_map *nmap, int *x, int *y)
{
  struct tile *ptile = map_pos_to_tile(nmap, *x, *y);

  if (ptile) {
    index_to_map_pos(x, y, tile_index(ptile)); 
    return TRUE;
  } else {
    return FALSE;
  }
}

/*******************************************************************//**
  Twiddle *x and *y to point to the nearest real tile, and ensure that the
  position is normalized.
***********************************************************************/
struct tile *nearest_real_tile(const struct civ_map *nmap, int x, int y)
{
  int nat_x, nat_y;

  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);
  if (!current_topo_has_flag(TF_WRAPX)) {
    nat_x = CLIP(0, nat_x, wld.map.xsize - 1);
  }
  if (!current_topo_has_flag(TF_WRAPY)) {
    nat_y = CLIP(0, nat_y, wld.map.ysize - 1);
  }
  NATIVE_TO_MAP_POS(&x, &y, nat_x, nat_y);

  return map_pos_to_tile(nmap, x, y);
}

/*******************************************************************//**
  Returns the total number of (real) positions (or tiles) on the map.
***********************************************************************/
int map_num_tiles(void)
{
  return wld.map.xsize * wld.map.ysize;
}

/*******************************************************************//**
  Finds the difference between the two (unnormalized) positions, in
  cartesian (map) coordinates.  Most callers should use map_distance_vector
  instead.
***********************************************************************/
void base_map_distance_vector(int *dx, int *dy,
			      int x0dv, int y0dv, int x1dv, int y1dv)
{
  if (current_topo_has_flag(TF_WRAPX) || current_topo_has_flag(TF_WRAPY)) {
    /* Wrapping is done in native coordinates. */
    MAP_TO_NATIVE_POS(&x0dv, &y0dv, x0dv, y0dv);
    MAP_TO_NATIVE_POS(&x1dv, &y1dv, x1dv, y1dv);

    /* Find the "native" distance vector. This corresponds closely to the
     * map distance vector but is easier to wrap. */
    *dx = x1dv - x0dv;
    *dy = y1dv - y0dv;
    if (current_topo_has_flag(TF_WRAPX)) {
      /* Wrap dx to be in [-map.xsize/2, map.xsize/2). */
      *dx = FC_WRAP(*dx + wld.map.xsize / 2, wld.map.xsize) - wld.map.xsize / 2;
    }
    if (current_topo_has_flag(TF_WRAPY)) {
      /* Wrap dy to be in [-map.ysize/2, map.ysize/2). */
      *dy = FC_WRAP(*dy + wld.map.ysize / 2, wld.map.ysize) - wld.map.ysize / 2;
    }

    /* Convert the native delta vector back to a pair of map positions. */
    x1dv = x0dv + *dx;
    y1dv = y0dv + *dy;
    NATIVE_TO_MAP_POS(&x0dv, &y0dv, x0dv, y0dv);
    NATIVE_TO_MAP_POS(&x1dv, &y1dv, x1dv, y1dv);
  }

  /* Find the final (map) vector. */
  *dx = x1dv - x0dv;
  *dy = y1dv - y0dv;
}

/*******************************************************************//**
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
***********************************************************************/
void map_distance_vector(int *dx, int *dy,
                         const struct tile *tile0,
                         const struct tile *tile1)
{
  int tx0, ty0, tx1, ty1;

  index_to_map_pos(&tx0, &ty0, tile_index(tile0));
  index_to_map_pos(&tx1, &ty1, tile_index(tile1)); 
  base_map_distance_vector(dx, dy, tx0, ty0, tx1, ty1);
}

/*******************************************************************//**
  Random neighbouring square.
***********************************************************************/
struct tile *rand_neighbour(const struct civ_map *nmap,
                            const struct tile *ptile)
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
    enum direction8 choice = (enum direction8) fc_rand(n);

    /* this neighbour's OK */
    tile1 = mapstep(nmap, ptile, dirs[choice]);
    if (tile1) {
      return tile1;
    }

    /* Choice was bad, so replace it with the last direction in the list.
     * On the next iteration, one fewer choices will remain. */
    dirs[choice] = dirs[n - 1];
  }

  fc_assert(FALSE);     /* Are we on a 1x1 map with no wrapping??? */
  return NULL;
}

/*******************************************************************//**
  Random square anywhere on the map.  Only normal positions (for which
  is_normal_map_pos returns true) will be found.
***********************************************************************/
struct tile *rand_map_pos(const struct civ_map *nmap)
{
  int nat_x = fc_rand(wld.map.xsize), nat_y = fc_rand(wld.map.ysize);

  return native_pos_to_tile(nmap, nat_x, nat_y);
}

/*******************************************************************//**
  Give a random tile anywhere on the map for which the 'filter' function
  returns TRUE.  Return FALSE if none can be found.  The filter may be
  NULL if any position is okay; if non-NULL it shouldn't have any side
  effects.
***********************************************************************/
struct tile *rand_map_pos_filtered(const struct civ_map *nmap, void *data,
                                   bool (*filter)(const struct tile *ptile,
                                                  const void *data))
{
  struct tile *ptile;
  int tries = 0;
  const int max_tries = MAP_INDEX_SIZE / ACTIVITY_FACTOR;

  /* First do a few quick checks to find a spot.  The limit on number of
   * tries could use some tweaking. */
  do {
    ptile = nmap->tiles + fc_rand(MAP_INDEX_SIZE);
  } while (filter && !filter(ptile, data) && ++tries < max_tries);

  /* If that fails, count all available spots and pick one.
   * Slow but reliable. */
  if (tries == max_tries) {
    int count = 0, *positions;

    positions = fc_calloc(MAP_INDEX_SIZE, sizeof(*positions));

    whole_map_iterate(nmap, check_tile) {
      if (filter(check_tile, data)) {
	positions[count] = tile_index(check_tile);
	count++;
      }
    } whole_map_iterate_end;

    if (count == 0) {
      ptile = NULL;
    } else {
      ptile = wld.map.tiles + positions[fc_rand(count)];
    }

    FC_FREE(positions);
  }

  return ptile;
}

/*******************************************************************//**
  Return the debugging name of the direction.
***********************************************************************/
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

/*******************************************************************//**
  Returns the next direction clock-wise.
***********************************************************************/
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
    fc_assert(FALSE);
    return -1;
  }
}

/*******************************************************************//**
  Returns the next direction counter-clock-wise.
***********************************************************************/
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
    fc_assert(FALSE);
    return -1;
  }
}

/*******************************************************************//**
  Returns TRUE iff the given direction is a valid one. Does not use
  value from the cache, but can be used to calculate the cache.
***********************************************************************/
static bool is_valid_dir_calculate(enum direction8 dir)
{
  switch (dir) {
  case DIR8_SOUTHEAST:
  case DIR8_NORTHWEST:
    /* These directions are invalid in hex topologies. */
    return !(current_topo_has_flag(TF_HEX) && !current_topo_has_flag(TF_ISO));
  case DIR8_NORTHEAST:
  case DIR8_SOUTHWEST:
    /* These directions are invalid in iso-hex topologies. */
    return !(current_topo_has_flag(TF_HEX) && current_topo_has_flag(TF_ISO));
  case DIR8_NORTH:
  case DIR8_EAST:
  case DIR8_SOUTH:
  case DIR8_WEST:
    return TRUE;
  default:
    return FALSE;
  }
}

/*******************************************************************//**
  Returns TRUE iff the given direction is a valid one.

  If the direction could be out of range you should use
  map_untrusted_dir_is_valid() in stead.
***********************************************************************/
bool is_valid_dir(enum direction8 dir)
{
  fc_assert_ret_val(dir <= direction8_invalid(), FALSE);

  return dir_validity[dir];
}

/*******************************************************************//**
  Returns TRUE iff the given direction is a valid one.

  Doesn't trust the input. Can be used to validate a direction from an
  untrusted source.
***********************************************************************/
bool map_untrusted_dir_is_valid(enum direction8 dir)
{
  if (!direction8_is_valid(dir)) {
    /* Isn't even in range of direction8. */
    return FALSE;
  }

  return is_valid_dir(dir);
}

/*******************************************************************//**
  Returns TRUE iff the given direction is a cardinal one. Does not use
  value from the cache, but can be used to calculate the cache.

  Cardinal directions are those in which adjacent tiles share an edge not
  just a vertex.
***********************************************************************/
static bool is_cardinal_dir_calculate(enum direction8 dir)
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
    return current_topo_has_flag(TF_HEX) && current_topo_has_flag(TF_ISO);
  case DIR8_NORTHEAST:
  case DIR8_SOUTHWEST:
    /* These directions are cardinal in hexagonal topologies. */
    return current_topo_has_flag(TF_HEX) && !current_topo_has_flag(TF_ISO);
  }
  return FALSE;
}

/*******************************************************************//**
  Returns TRUE iff the given direction is a cardinal one.

  Cardinal directions are those in which adjacent tiles share an edge not
  just a vertex.
***********************************************************************/
bool is_cardinal_dir(enum direction8 dir)
{
  fc_assert_ret_val(dir <= direction8_invalid(), FALSE);

  return dir_cardinality[dir];
}

/*******************************************************************//**
  Return TRUE and sets dir to the direction of the step if (end_x,
  end_y) can be reached from (start_x, start_y) in one step. Return
  FALSE otherwise (value of dir is unchanged in this case).
***********************************************************************/
bool base_get_direction_for_step(const struct civ_map *nmap,
                                 const struct tile *start_tile,
                                 const struct tile *end_tile,
                                 enum direction8 *dir)
{
  adjc_dir_iterate(nmap, start_tile, test_tile, test_dir) {
    if (same_pos(end_tile, test_tile)) {
      *dir = test_dir;
      return TRUE;
    }
  } adjc_dir_iterate_end;

  return FALSE;
}

/*******************************************************************//**
  Return the direction which is needed for a step on the map from
 (start_x, start_y) to (end_x, end_y).
***********************************************************************/
int get_direction_for_step(const struct civ_map *nmap,
                           const struct tile *start_tile,
                           const struct tile *end_tile)
{
  enum direction8 dir;

  if (base_get_direction_for_step(nmap, start_tile, end_tile, &dir)) {
    return dir;
  }

  fc_assert(FALSE);
  return -1;
}

/*******************************************************************//**
  Returns TRUE iff the move from the position (start_x,start_y) to
  (end_x,end_y) is a cardinal one.
***********************************************************************/
bool is_move_cardinal(const struct civ_map *nmap,
                      const struct tile *start_tile,
                      const struct tile *end_tile)
{
  cardinal_adjc_dir_iterate(nmap, start_tile, test_tile, test_dir) {
    if (same_pos(end_tile, test_tile)) {
      return TRUE;
    }
  } cardinal_adjc_dir_iterate_end;

  return FALSE;
}

/*******************************************************************//**
  A "SINGULAR" position is any map position that has an abnormal number of
  tiles in the radius of dist.

  (map_x, map_y) must be normalized.

  dist is the "real" map distance.
***********************************************************************/
bool is_singular_tile(const struct tile *ptile, int dist)
{
  int tile_x, tile_y;

  index_to_map_pos(&tile_x, &tile_y, tile_index(ptile)); 
  do_in_natural_pos(ntl_x, ntl_y, tile_x, tile_y) {
    /* Iso-natural coordinates are doubled in scale. */
    dist *= MAP_IS_ISOMETRIC ? 2 : 1;

    return ((!current_topo_has_flag(TF_WRAPX) 
	     && (ntl_x < dist || ntl_x >= NATURAL_WIDTH - dist))
	    || (!current_topo_has_flag(TF_WRAPY)
		&& (ntl_y < dist || ntl_y >= NATURAL_HEIGHT - dist)));
  } do_in_natural_pos_end;
}

/*******************************************************************//**
  Create a new, empty start position.
***********************************************************************/
static struct startpos *startpos_new(struct tile *ptile)
{
  struct startpos *psp = fc_malloc(sizeof(*psp));

  psp->location = ptile;
  psp->exclude = FALSE;
  psp->nations = nation_hash_new();

  return psp;
}

/*******************************************************************//**
  Free all memory allocated by the start position.
***********************************************************************/
static void startpos_destroy(struct startpos *psp)
{
  fc_assert_ret(NULL != psp);
  nation_hash_destroy(psp->nations);
  free(psp);
}

/*******************************************************************//**
  Returns the start position associated to the given ID.
***********************************************************************/
struct startpos *map_startpos_by_number(int id)
{
  return map_startpos_get(index_to_tile(&(wld.map), id));
}

/*******************************************************************//**
  Returns the unique ID number for this start position. This is just the
  tile index of the tile where this start position is located.
***********************************************************************/
int startpos_number(const struct startpos *psp)
{
  fc_assert_ret_val(NULL != psp, -1);
  return tile_index(psp->location);
}

/*******************************************************************//**
  Allow the nation to start at the start position.
  NB: in "excluding" mode, this remove the nation from the excluded list.
***********************************************************************/
bool startpos_allow(struct startpos *psp, struct nation_type *pnation)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  fc_assert_ret_val(NULL != pnation, FALSE);

  if (0 == nation_hash_size(psp->nations) || !psp->exclude) {
    psp->exclude = FALSE; /* Disable "excluding" mode. */
    return nation_hash_insert(psp->nations, pnation, NULL);
  } else {
    return nation_hash_remove(psp->nations, pnation);
  }
}

/*******************************************************************//**
  Disallow the nation to start at the start position.
  NB: in "excluding" mode, this add the nation to the excluded list.
***********************************************************************/
bool startpos_disallow(struct startpos *psp, struct nation_type *pnation)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  fc_assert_ret_val(NULL != pnation, FALSE);

  if (0 == nation_hash_size(psp->nations) || psp->exclude) {
    psp->exclude = TRUE; /* Enable "excluding" mode. */
    return nation_hash_remove(psp->nations, pnation);
  } else {
    return nation_hash_insert(psp->nations, pnation, NULL);
  }
}

/*******************************************************************//**
  Returns the tile where this start position is located.
***********************************************************************/
struct tile *startpos_tile(const struct startpos *psp)
{
  fc_assert_ret_val(NULL != psp, NULL);
  return psp->location;
}

/*******************************************************************//**
  Returns TRUE if the given nation can start here.
***********************************************************************/
bool startpos_nation_allowed(const struct startpos *psp,
                             const struct nation_type *pnation)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  fc_assert_ret_val(NULL != pnation, FALSE);
  return XOR(psp->exclude, nation_hash_lookup(psp->nations, pnation, NULL));
}

/*******************************************************************//**
  Returns TRUE if any nation can start here.
***********************************************************************/
bool startpos_allows_all(const struct startpos *psp)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  return (0 == nation_hash_size(psp->nations));
}

/*******************************************************************//**
  Fills the packet with all of the information at this start position.
  Returns TRUE if the packet can be sent.
***********************************************************************/
bool startpos_pack(const struct startpos *psp,
                   struct packet_edit_startpos_full *packet)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  fc_assert_ret_val(NULL != packet, FALSE);

  packet->id = startpos_number(psp);
  packet->exclude = psp->exclude;
  BV_CLR_ALL(packet->nations);

  nation_hash_iterate(psp->nations, pnation) {
    BV_SET(packet->nations, nation_number(pnation));
  } nation_hash_iterate_end;
  return TRUE;
}

/*******************************************************************//**
  Fills the start position with the nation information in the packet.
  Returns TRUE if the start position was changed.
***********************************************************************/
bool startpos_unpack(struct startpos *psp,
                     const struct packet_edit_startpos_full *packet)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  fc_assert_ret_val(NULL != packet, FALSE);

  psp->exclude = packet->exclude;

  nation_hash_clear(psp->nations);
  if (!BV_ISSET_ANY(packet->nations)) {
    return TRUE;
  }
  nations_iterate(pnation) {
    if (BV_ISSET(packet->nations, nation_number(pnation))) {
      nation_hash_insert(psp->nations, pnation, NULL);
    }
  } nations_iterate_end;
  return TRUE;
}

/*******************************************************************//**
  Returns TRUE if the nations returned by startpos_raw_nations()
  are actually excluded from the nations allowed to start at this position.

  FIXME: This function exposes the internal implementation and should be
  removed when no longer needed by the property editor system.
***********************************************************************/
bool startpos_is_excluding(const struct startpos *psp)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  return psp->exclude;
}

/*******************************************************************//**
  Return a the nations hash, used for the property editor.

  FIXME: This function exposes the internal implementation and should be
  removed when no longer needed by the property editor system.
***********************************************************************/
const struct nation_hash *startpos_raw_nations(const struct startpos *psp)
{
  fc_assert_ret_val(NULL != psp, FALSE);
  return psp->nations;
}

/*******************************************************************//**
  Implementation of iterator 'sizeof' function.

  struct startpos_iter can be either a 'struct nation_hash_iter', a 'struct
  nation_iter' or 'struct startpos_iter'.
***********************************************************************/
size_t startpos_iter_sizeof(void)
{
  return MAX(sizeof(struct startpos_iter) + nation_iter_sizeof()
             - sizeof(struct iterator), nation_hash_iter_sizeof());
}

/*******************************************************************//**
  Implementation of iterator 'next' function.

  NB: This is only used for the case where 'exclude' is set in the start
  position.
***********************************************************************/
static void startpos_exclude_iter_next(struct iterator *startpos_iter)
{
  struct startpos_iter *iter = STARTPOS_ITER(startpos_iter);

  do {
    iterator_next(&iter->nation_iter);
  } while (iterator_valid(&iter->nation_iter)
           || !nation_hash_lookup(iter->psp->nations,
                                  iterator_get(&iter->nation_iter), NULL));
}

/*******************************************************************//**
  Implementation of iterator 'get' function. Returns a struct nation_type
  pointer.

  NB: This is only used for the case where 'exclude' is set in the start
  position.
***********************************************************************/
static void *startpos_exclude_iter_get(const struct iterator *startpos_iter)
{
  struct startpos_iter *iter = STARTPOS_ITER(startpos_iter);
  return iterator_get(&iter->nation_iter);
}

/*******************************************************************//**
  Implementation of iterator 'valid' function.
***********************************************************************/
static bool startpos_exclude_iter_valid(const struct iterator *startpos_iter)
{
  struct startpos_iter *iter = STARTPOS_ITER(startpos_iter);
  return iterator_valid(&iter->nation_iter);
}

/*******************************************************************//**
  Initialize and return an iterator for the nations at this start position.
***********************************************************************/
struct iterator *startpos_iter_init(struct startpos_iter *iter,
                                    const struct startpos *psp)
{
  if (!psp) {
    return invalid_iter_init(ITERATOR(iter));
  }

  if (startpos_allows_all(psp)) {
    return nation_iter_init((struct nation_iter *) iter);
  }

  if (!psp->exclude) {
    return nation_hash_key_iter_init((struct nation_hash_iter *) iter,
                                     psp->nations);
  }

  iter->vtable.next = startpos_exclude_iter_next;
  iter->vtable.get = startpos_exclude_iter_get;
  iter->vtable.valid = startpos_exclude_iter_valid;
  iter->psp = psp;
  (void) nation_iter_init((struct nation_iter *) &iter->nation_iter);

  return ITERATOR(iter);
}

/*******************************************************************//**
  Is there start positions set for map
***********************************************************************/
int map_startpos_count(void)
{
  if (NULL != wld.map.startpos_table) {
    return startpos_hash_size(wld.map.startpos_table);
  } else {
    return 0;
  }
}

/*******************************************************************//**
  Create a new start position at the given tile and return it. If a start
  position already exists there, it is first removed.
***********************************************************************/
struct startpos *map_startpos_new(struct tile *ptile)
{
  struct startpos *psp;

  fc_assert_ret_val(NULL != ptile, NULL);
  fc_assert_ret_val(NULL != wld.map.startpos_table, NULL);

  psp = startpos_new(ptile);
  startpos_hash_replace(wld.map.startpos_table, tile_hash_key(ptile), psp);

  return psp;
}

/*******************************************************************//**
  Returns the start position at the given tile, or NULL if none exists
  there.
***********************************************************************/
struct startpos *map_startpos_get(const struct tile *ptile)
{
  struct startpos *psp;

  fc_assert_ret_val(NULL != ptile, NULL);
  fc_assert_ret_val(NULL != wld.map.startpos_table, NULL);

  startpos_hash_lookup(wld.map.startpos_table, tile_hash_key(ptile), &psp);

  return psp;
}

/*******************************************************************//**
  Remove the start position at the given tile. Returns TRUE if the start
  position was removed.
***********************************************************************/
bool map_startpos_remove(struct tile *ptile)
{
  fc_assert_ret_val(NULL != ptile, FALSE);
  fc_assert_ret_val(NULL != wld.map.startpos_table, FALSE);

  return startpos_hash_remove(wld.map.startpos_table, tile_hash_key(ptile));
}

/*******************************************************************//**
  Implementation of iterator sizeof function.
  NB: map_sp_iter just wraps startpos_hash_iter.
***********************************************************************/
size_t map_startpos_iter_sizeof(void)
{
  return startpos_hash_iter_sizeof();
}

/*******************************************************************//**
  Implementation of iterator init function.
  NB: map_sp_iter just wraps startpos_hash_iter.
***********************************************************************/
struct iterator *map_startpos_iter_init(struct map_startpos_iter *iter)
{
  return startpos_hash_value_iter_init((struct startpos_hash_iter *) iter,
                                       wld.map.startpos_table);
}

/*******************************************************************//**
  Return random direction that is valid in current map.
***********************************************************************/
enum direction8 rand_direction(void)
{
  return wld.map.valid_dirs[fc_rand(wld.map.num_valid_dirs)];
}

/*******************************************************************//**
  Return direction that is opposite to given one.
***********************************************************************/
enum direction8 opposite_direction(enum direction8 dir)
{
  return direction8_max() - dir;
}
