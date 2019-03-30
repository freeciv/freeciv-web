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
#ifndef FC__MAP_H
#define FC__MAP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <math.h> /* sqrt */

/* utility */
#include "bitvector.h"
#include "iterator.h"
#include "log.h"                /* fc_assert */

/* common */
#include "fc_types.h"
#include "game.h"
#include "map_types.h"
#include "tile.h"
#include "packets.h"

/* Parameters for terrain counting functions. */
static const bool C_ADJACENT = FALSE;
static const bool C_CARDINAL = TRUE;
static const bool C_NUMBER = FALSE;
static const bool C_PERCENT = TRUE;

#define MAP_IS_ISOMETRIC (CURRENT_TOPOLOGY & (TF_ISO + TF_HEX))

#define CURRENT_TOPOLOGY (wld.map.topology_id)

#define topo_has_flag(topo, flag) (((topo) & (flag)) != 0)
#define current_topo_has_flag(flag) topo_has_flag((CURRENT_TOPOLOGY), (flag))

#define ALL_DIRECTIONS_CARDINAL() topo_has_flag((CURRENT_TOPOLOGY), TF_HEX)

bool map_is_empty(void);
void map_init(struct civ_map *imap, bool server_side);
void map_init_topology(void);
void map_allocate(struct civ_map *amap);
void main_map_allocate(void);
void map_free(struct civ_map *fmap);
void main_map_free(void);

int map_vector_to_real_distance(int dx, int dy);
int map_vector_to_sq_distance(int dx, int dy);
int map_distance(const struct tile *tile0, const struct tile *tile1);
int real_map_distance(const struct tile *tile0, const struct tile *tile1);
int sq_map_distance(const struct tile *tile0,const  struct tile *tile1);

bool same_pos(const struct tile *tile0, const struct tile *tile1);
bool base_get_direction_for_step(const struct civ_map *nmap,
                                 const struct tile *src_tile,
                                 const struct tile *dst_tile,
                                 enum direction8 *dir);
int get_direction_for_step(const struct civ_map *nmap,
                           const struct tile *src_tile,
                           const struct tile *dst_tile);


/* Specific functions for start positions. */
struct startpos *map_startpos_by_number(int id);
int startpos_number(const struct startpos *psp);

bool startpos_allow(struct startpos *psp, struct nation_type *pnation);
bool startpos_disallow(struct startpos *psp, struct nation_type *pnation);

struct tile *startpos_tile(const struct startpos *psp);
bool startpos_nation_allowed(const struct startpos *psp,
                             const struct nation_type *pnation);
bool startpos_allows_all(const struct startpos *psp);

bool startpos_pack(const struct startpos *psp,
                   struct packet_edit_startpos_full *packet);
bool startpos_unpack(struct startpos *psp,
                     const struct packet_edit_startpos_full *packet);

/* See comment in "common/map.c". */
bool startpos_is_excluding(const struct startpos *psp);
const struct nation_hash *startpos_raw_nations(const struct startpos *psp);

/****************************************************************************
  Iterate over all nations at the start position for which the function
  startpos_nation_allowed() would return TRUE. This automatically takes into
  account the value of startpos_is_excluding() and startpos_allows_all() to
  iterate over the correct set of nations.
****************************************************************************/
struct startpos_iter;
size_t startpos_iter_sizeof(void);
struct iterator *startpos_iter_init(struct startpos_iter *it,
                                    const struct startpos *psp);
#define startpos_nations_iterate(ARG_psp, NAME_pnation)                     \
  generic_iterate(struct startpos_iter, const struct nation_type *,         \
                  NAME_pnation, startpos_iter_sizeof,                       \
                  startpos_iter_init, (ARG_psp))
#define startpos_nations_iterate_end generic_iterate_end


/* General map start positions functions. */
int map_startpos_count(void);
struct startpos *map_startpos_new(struct tile *ptile);
struct startpos *map_startpos_get(const struct tile *ptile);
bool map_startpos_remove(struct tile *ptile);

/****************************************************************************
  Iterate over all start positions placed on the map.
****************************************************************************/
struct map_startpos_iter;
size_t map_startpos_iter_sizeof(void);
struct iterator *map_startpos_iter_init(struct map_startpos_iter *iter);

#define map_startpos_iterate(NAME_psp)                                      \
  generic_iterate(struct map_startpos_iter, struct startpos *,              \
                  NAME_psp, map_startpos_iter_sizeof, map_startpos_iter_init)
#define map_startpos_iterate_end generic_iterate_end


/* Number of index coordinates (for sanity checks and allocations) */
#define MAP_INDEX_SIZE (wld.map.xsize * wld.map.ysize)

#ifdef FREECIV_DEBUG
#define CHECK_MAP_POS(x,y) \
  fc_assert(is_normal_map_pos((x),(y)))
#define CHECK_NATIVE_POS(x, y) \
  fc_assert((x) >= 0 && (x) < wld.map.xsize && (y) >= 0 && (y) < wld.map.ysize)
#define CHECK_INDEX(mindex) \
  fc_assert((mindex) >= 0 && (mindex) < MAP_INDEX_SIZE)
#else  /* FREECIV_DEBUG */
#define CHECK_MAP_POS(x,y) ((void)0)
#define CHECK_NATIVE_POS(x, y) ((void)0)
#define CHECK_INDEX(mindex) ((void)0)
#endif /* FREECIV_DEBUG */

#define native_pos_to_index_nocheck(nat_x, nat_y)                            \
  ((nat_x) + (nat_y) * wld.map.xsize)
#define native_pos_to_index(nat_x, nat_y)                                    \
  (CHECK_NATIVE_POS((nat_x), (nat_y)),                                       \
   native_pos_to_index_nocheck(nat_x, nat_y))
#define index_to_native_pos(pnat_x, pnat_y, mindex)                          \
  (*(pnat_x) = index_to_native_pos_x(mindex),                                \
   *(pnat_y) = index_to_native_pos_y(mindex))
#define index_to_native_pos_x(mindex)                                        \
  ((mindex) % wld.map.xsize) 
#define index_to_native_pos_y(mindex)                                        \
  ((mindex) / wld.map.xsize)

/* Obscure math.  See explanation in doc/HACKING. */
#define NATIVE_TO_MAP_POS(pmap_x, pmap_y, nat_x, nat_y)                     \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pmap_x) = ((nat_y) + ((nat_y) & 1)) / 2 + (nat_x),                  \
      *(pmap_y) = (nat_y) - *(pmap_x) + wld.map.xsize)                      \
   : (*(pmap_x) = (nat_x), *(pmap_y) = (nat_y)))

#define MAP_TO_NATIVE_POS(pnat_x, pnat_y, map_x, map_y)                     \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pnat_y) = (map_x) + (map_y) - wld.map.xsize,                        \
      *(pnat_x) = (2 * (map_x) - *(pnat_y) - (*(pnat_y) & 1)) / 2)          \
   : (*(pnat_x) = (map_x), *(pnat_y) = (map_y)))

#define NATURAL_TO_MAP_POS(pmap_x, pmap_y, nat_x, nat_y)                    \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pmap_x) = ((nat_y) + (nat_x)) / 2,                                  \
      *(pmap_y) = (nat_y) - *(pmap_x) + wld.map.xsize)                      \
   : (*(pmap_x) = (nat_x), *(pmap_y) = (nat_y)))

#define MAP_TO_NATURAL_POS(pnat_x, pnat_y, map_x, map_y)                    \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pnat_y) = (map_x) + (map_y) - wld.map.xsize,                        \
      *(pnat_x) = 2 * (map_x) - *(pnat_y))                                  \
   : (*(pnat_x) = (map_x), *(pnat_y) = (map_y)))


/* Provide a block to convert from map to native coordinates.  This allows
 * you to use a native version of the map position within the block.  Note
 * that the native position is declared as const and can't be changed
 * inside the block. */
#define do_in_native_pos(nat_x, nat_y, map_x, map_y)                        \
{                                                                           \
  int _nat_x, _nat_y;                                                       \
  MAP_TO_NATIVE_POS(&_nat_x, &_nat_y, map_x, map_y);			    \
  {                                                                         \
    const int nat_x = _nat_x, nat_y = _nat_y;

#define do_in_native_pos_end                                                \
  }                                                                         \
}

/* Provide a block to convert from map to natural coordinates. This allows
 * you to use a natural version of the map position within the block.  Note
 * that the natural position is declared as const and can't be changed
 * inside the block. */
#define do_in_natural_pos(ntl_x, ntl_y, map_x, map_y)                        \
{                                                                           \
  int _ntl_x, _ntl_y;                                                       \
  MAP_TO_NATURAL_POS(&_ntl_x, &_ntl_y, map_x, map_y);			    \
  {                                                                         \
    const int ntl_x = _ntl_x, ntl_y = _ntl_y;

#define do_in_natural_pos_end                                                \
  }                                                                         \
}

/* Width and height of the map, in native coordinates. */
#define NATIVE_WIDTH wld.map.xsize
#define NATIVE_HEIGHT wld.map.ysize

/* Width and height of the map, in natural coordinates. */
#define NATURAL_WIDTH (MAP_IS_ISOMETRIC ? 2 * wld.map.xsize : wld.map.xsize)
#define NATURAL_HEIGHT wld.map.ysize

static inline int map_pos_to_index(struct civ_map *nmap,
                                   int map_x, int map_y);

/* index_to_map_pos(int *, int *, int) inverts map_pos_to_index */
#define index_to_map_pos(pmap_x, pmap_y, mindex) \
  (CHECK_INDEX(mindex),                          \
   index_to_native_pos(pmap_x, pmap_y, mindex),  \
   NATIVE_TO_MAP_POS(pmap_x, pmap_y, *(pmap_x), *(pmap_y)))
static inline int index_to_map_pos_x(int mindex);
static inline int index_to_map_pos_y(int mindex);

#define DIRSTEP(dest_x, dest_y, dir)	\
(    (dest_x) = DIR_DX[(dir)],      	\
     (dest_y) = DIR_DY[(dir)])

/*
 * Steps from the tile in the given direction, yielding a new tile (or NULL).
 *
 * Direct calls to DIR_DXY should be avoided and DIRSTEP should be
 * used. But to allow dest and src to be the same, as in
 *    MAPSTEP(x, y, x, y, dir)
 * we bend this rule here.
 */
struct tile *mapstep(const struct civ_map *nmap, const struct tile *ptile,
                     enum direction8 dir);

struct tile *map_pos_to_tile(const struct civ_map *nmap, int x, int y);
struct tile *native_pos_to_tile(const struct civ_map *nmap,
                                int nat_x, int nat_y);
struct tile *index_to_tile(const struct civ_map *imap, int mindex);

bool is_real_map_pos(const struct civ_map *nmap, int x, int y);
bool is_normal_map_pos(int x, int y);

bool is_singular_tile(const struct tile *ptile, int dist);
bool normalize_map_pos(const struct civ_map *nmap, int *x, int *y);
struct tile *nearest_real_tile(const struct civ_map *nmap, int x, int y);
void base_map_distance_vector(int *dx, int *dy,
			      int x0, int y0, int x1, int y1);
void map_distance_vector(int *dx, int *dy, const struct tile *ptile0,
			 const struct tile *ptile1);
int map_num_tiles(void);
#define map_size_checked()  MAX(map_num_tiles() / 1000, 1)

struct tile *rand_neighbour(const struct civ_map *nmap,
                            const struct tile *ptile);
struct tile *rand_map_pos(const struct civ_map *nmap);
struct tile *rand_map_pos_filtered(const struct civ_map *nmap, void *data,
                                   bool (*filter)(const struct tile *ptile,
                                                  const void *data));

bool is_tiles_adjacent(const struct tile *ptile0, const struct tile *ptile1);
bool is_move_cardinal(const struct civ_map *nmap,
                      const struct tile *src_tile,
		      const struct tile *dst_tile);

int tile_move_cost_ptrs(const struct civ_map *nmap,
                        const struct unit *punit,
                        const struct unit_type *punittype,
                        const struct player *pplayer,
                        const struct tile *t1, const struct tile *t2);

/***************************************************************
  The cost to move punit from where it is to tile x,y.
  It is assumed the move is a valid one, e.g. the tiles are adjacent.
***************************************************************/
static inline int map_move_cost_unit(const struct civ_map *nmap,
                                     struct unit *punit,
                                     const struct tile *ptile)
{
  return tile_move_cost_ptrs(nmap, punit,
                             unit_type_get(punit), unit_owner(punit),
                             unit_tile(punit), ptile);
}

/***************************************************************
  Move cost between two tiles
***************************************************************/
static inline int map_move_cost(const struct civ_map *nmap,
                                const struct player *pplayer,
                                const struct unit_type *punittype,
                                const struct tile *src_tile,
                                const struct tile *dst_tile)
{
  return tile_move_cost_ptrs(nmap, NULL, punittype, pplayer,
                             src_tile, dst_tile);
}

bool is_safe_ocean(const struct civ_map *nmap, const struct tile *ptile);
bv_extras get_tile_infrastructure_set(const struct tile *ptile,
                                      int *count);

bool can_channel_land(const struct tile *ptile);
bool can_reclaim_ocean(const struct tile *ptile);
bool can_thaw_terrain(const struct tile *ptile);
bool can_freeze_terrain(const struct tile *ptile);
bool terrain_surroundings_allow_change(const struct tile *ptile,
                                       const struct terrain *pterrain);

extern struct terrain_misc terrain_control;

/* This iterates outwards from the starting point.  Every tile within max_dist
 * (including the starting tile) will show up exactly once, in an outward
 * (based on real map distance) order.  The returned values are always real
 * and are normalized.  The starting position must be normal.
 *
 * See also iterate_outward() */
#define iterate_outward_dxy(nmap, start_tile, max_dist, _tile, _x, _y)      \
{									    \
  int _x, _y, _tile##_x, _tile##_y, _start##_x, _start##_y;                 \
  struct tile *_tile;							    \
  const struct tile *_tile##_start = (start_tile);			    \
  int _tile##_max = (max_dist);						    \
  int _tile##_index = 0;						    \
  index_to_map_pos(&_start##_x, &_start##_y, tile_index(_tile##_start));    \
  for (;								    \
       _tile##_index < wld.map.num_iterate_outwards_indices;		    \
       _tile##_index++) { 						    \
    if (wld.map.iterate_outwards_indices[_tile##_index].dist > _tile##_max) {   \
      break;								    \
    }									    \
    _x = wld.map.iterate_outwards_indices[_tile##_index].dx;		    \
    _y = wld.map.iterate_outwards_indices[_tile##_index].dy;		    \
    _tile##_x = _x + _start##_x;                                            \
    _tile##_y = _y + _start##_y;                                            \
    _tile = map_pos_to_tile(nmap, _tile##_x, _tile##_y);                    \
    if (NULL == _tile) {                                                    \
      continue;                                                             \
    }

#define iterate_outward_dxy_end						    \
  }									    \
}

/* See iterate_outward_dxy() */
#define iterate_outward(nmap, start_tile, max_dist, itr_tile)            \
  iterate_outward_dxy(nmap, start_tile, max_dist, itr_tile, _dx_itr, _dy_itr)

#define iterate_outward_end iterate_outward_dxy_end

/* 
 * Iterate through all tiles in a square with given center and radius.
 * The position (x_itr, y_itr) that is returned will be normalized;
 * unreal positions will be automatically discarded. (dx_itr, dy_itr)
 * is the standard distance vector between the position and the center
 * position. Note that when the square is larger than the map the
 * distance vector may not be the minimum distance vector.
 */
#define square_dxy_iterate(nmap, center_tile, radius, tile_itr, dx_itr, dy_itr) \
  iterate_outward_dxy(nmap, center_tile, radius, tile_itr, dx_itr, dy_itr)

#define square_dxy_iterate_end iterate_outward_dxy_end

/*
 * Iterate through all tiles in a square with given center and radius.
 * Positions returned will have adjusted x, and positions with illegal
 * y will be automatically discarded.
 */
#define square_iterate(nmap, center_tile, radius, tile_itr)              \
  square_dxy_iterate(nmap, center_tile, radius, tile_itr, _dummy_x, dummy_y)

#define square_iterate_end  square_dxy_iterate_end

/* 
 * Iterate through all tiles in a circle with given center and squared
 * radius.  Positions returned will have adjusted (x, y); unreal
 * positions will be automatically discarded. 
 */
#define circle_iterate(nmap, center_tile, sq_radius, tile_itr)           \
  circle_dxyr_iterate(nmap, center_tile, sq_radius, tile_itr, _dx, _dy, _dr)

#define circle_iterate_end                                                  \
  circle_dxyr_iterate_end

/* dx, dy, dr are distance from center to tile in x, y and square distance;
 * do not rely on x, y distance, since they do not work for hex topologies */
#define circle_dxyr_iterate(nmap, center_tile, sq_radius,                   \
			    _tile, dx, dy, dr)				    \
{									    \
  const int _tile##_sq_radius = (sq_radius);				    \
  const int _tile##_cr_radius = (int)sqrt((double)MAX(_tile##_sq_radius, 0)); \
									    \
  square_dxy_iterate(nmap, center_tile, _tile##_cr_radius, _tile, dx, dy) { \
    const int dr = map_vector_to_sq_distance(dx, dy);			    \
									    \
    if (dr <= _tile##_sq_radius) {

#define circle_dxyr_iterate_end						    \
    }									    \
  } square_dxy_iterate_end;						    \
}

/* Iterate itr_tile through all map tiles adjacent to the given center map
 * position, with normalization.  Does not include the center position.
 * The order of positions is unspecified. */
#define adjc_iterate(nmap, center_tile, itr_tile)                           \
{									    \
  /* Written as a wrapper to adjc_dir_iterate since it's the cleanest and   \
   * most efficient. */							    \
  adjc_dir_iterate(nmap, center_tile, itr_tile, ADJC_ITERATE_dir_itr##itr_tile) {

#define adjc_iterate_end                                                    \
  } adjc_dir_iterate_end;                                                   \
}

/* As adjc_iterate() but also set direction8 iterator variable dir_itr */
#define adjc_dir_iterate(nmap, center_tile, itr_tile, dir_itr)              \
  adjc_dirlist_iterate(nmap, center_tile, itr_tile, dir_itr,                \
		       wld.map.valid_dirs, wld.map.num_valid_dirs)

#define adjc_dir_iterate_end adjc_dirlist_iterate_end

/* Only set direction8 dir_itr (not tile) */
#define adjc_dir_base_iterate(nmap, center_tile, dir_itr)                \
  adjc_dirlist_base_iterate(nmap, center_tile, dir_itr,                  \
                            wld.map.valid_dirs, wld.map.num_valid_dirs)

#define adjc_dir_base_iterate_end                                              \
  adjc_dirlist_base_iterate_end

/* Iterate itr_tile through all map tiles cardinally adjacent to the given
 * center map position, with normalization.  Does not include the center
 * position.  The order of positions is unspecified. */
#define cardinal_adjc_iterate(nmap, center_tile, itr_tile)                  \
  adjc_dirlist_iterate(nmap, center_tile, itr_tile, _dir_itr##center_tile,  \
		       wld.map.cardinal_dirs, wld.map.num_cardinal_dirs)

#define cardinal_adjc_iterate_end adjc_dirlist_iterate_end

/* As cardinal_adjc_iterate but also set direction8 variable dir_itr */
#define cardinal_adjc_dir_iterate(nmap, center_tile, itr_tile, dir_itr)  \
  adjc_dirlist_iterate(nmap, center_tile, itr_tile, dir_itr,             \
		       wld.map.cardinal_dirs, wld.map.num_cardinal_dirs)

#define cardinal_adjc_dir_iterate_end adjc_dirlist_iterate_end

/* Only set direction8 dir_itr (not tile) */
#define cardinal_adjc_dir_base_iterate(nmap, center_tile, dir_itr)       \
  adjc_dirlist_base_iterate(nmap, center_tile, dir_itr,                  \
                            wld.map.cardinal_dirs, wld.map.num_cardinal_dirs)

#define cardinal_adjc_dir_base_iterate_end                                     \
  adjc_dirlist_base_iterate_end

/* Iterate through all tiles cardinally adjacent to both tile1 and tile2 */
#define cardinal_between_iterate(nmap, tile1, tile2, between)                  \
  cardinal_adjc_iterate(nmap, tile1, between) {                                \
    cardinal_adjc_iterate(nmap, between, second) {                             \
    if (same_pos(second, tile2)) {

#define cardinal_between_iterate_end                                           \
      }                                                                        \
    } cardinal_adjc_iterate_end;                                               \
  } cardinal_adjc_iterate_end;

/* Iterate through all tiles adjacent to a tile using the given list of
 * directions.  _dir is the directional value, (center_x, center_y) is
 * the center tile (which must be normalized).  The center tile is not
 * included in the iteration.
 *
 * This macro should not be used directly.  Instead, use adjc_iterate,
 * cardinal_adjc_iterate, or related iterators. */
#define adjc_dirlist_iterate(nmap, center_tile, _tile, _dir,                \
			     dirlist, dircount)				    \
{									    \
  enum direction8 _dir;							    \
  int _tile##_x, _tile##_y, _tile##_cx, _tile##_cy;                         \
  struct tile *_tile;							    \
  const struct tile *_tile##_center = (center_tile);			    \
  int _tile##_index = 0;						    \
  index_to_map_pos(&_tile##_cx, &_tile##_cy, tile_index(_tile##_center));   \
  for (;								    \
       _tile##_index < (dircount);					    \
       _tile##_index++) {						    \
    _dir = dirlist[_tile##_index];					    \
    DIRSTEP(_tile##_x, _tile##_y, _dir);				    \
    _tile##_x += _tile##_cx;                                                \
    _tile##_y += _tile##_cy;                                                \
    _tile = map_pos_to_tile(nmap, _tile##_x, _tile##_y);                    \
    if (NULL == _tile) {                                                    \
      continue;                                                             \
    }

#define adjc_dirlist_iterate_end					    \
    }									    \
}

/* Same as above but without setting the tile. */
#define adjc_dirlist_base_iterate(nmap, center_tile, _dir, dirlist, dircount)  \
{                                                                              \
  enum direction8 _dir;                                                        \
  int _tile##_x, _tile##_y, _center##_x, _center##_y;                          \
  const struct tile *_tile##_center = (center_tile);                           \
  bool _tile##_is_border = is_border_tile(_tile##_center, 1);                  \
  int _tile##_index = 0;                                                       \
  index_to_map_pos(&_center##_x, &_center##_y, tile_index(_tile##_center));    \
  for (;                                                                       \
       _tile##_index < (dircount);                                             \
       _tile##_index++) {                                                      \
    _dir = dirlist[_tile##_index];                                             \
    DIRSTEP(_tile##_x, _tile##_y, _dir);                                       \
    _tile##_x += _center##_x;                                                  \
    _tile##_y += _center##_y;                                                  \
    if (_tile##_is_border && !normalize_map_pos(nmap, &_tile##_x, &_tile##_y)) { \
      continue;                                                                \
    }

#define adjc_dirlist_base_iterate_end                                          \
  }                                                                            \
}

/* Iterate over all positions on the globe.
 * Use index positions for cache efficiency. */
#define whole_map_iterate(_map, _tile)                                      \
{									    \
  struct tile *_tile;							    \
  int _tile##_index = 0;						    \
  for (;								    \
       _tile##_index < MAP_INDEX_SIZE;					    \
       _tile##_index++) {						    \
    _tile = (_map)->tiles + _tile##_index;

#define whole_map_iterate_end						    \
  }									    \
}

BV_DEFINE(dir_vector, 8);

/* return the reverse of the direction */
#define DIR_REVERSE(dir) (7 - (dir))

enum direction8 dir_cw(enum direction8 dir);
enum direction8 dir_ccw(enum direction8 dir);
const char* dir_get_name(enum direction8 dir);
bool map_untrusted_dir_is_valid(enum direction8 dir);
bool is_valid_dir(enum direction8 dir);
bool is_cardinal_dir(enum direction8 dir);

extern const int DIR_DX[8];
extern const int DIR_DY[8];

/* Used for network transmission; do not change. */
#define MAP_TILE_OWNER_NULL	 MAX_UINT8

#define MAP_DEFAULT_HUTS         15
#define MAP_MIN_HUTS             0
#define MAP_MAX_HUTS             500

#define MAP_DEFAULT_ANIMALS      20
#define MAP_MIN_ANIMALS          0
#define MAP_MAX_ANIMALS          500

#define MAP_DEFAULT_MAPSIZE     MAPSIZE_FULLSIZE

/* Size of the map in thousands of tiles. If MAP_MAX_SIZE is increased, 
 * MAX_DBV_LENGTH in bitvector.c must be checked; see the static assertion
 * below. */
#ifdef FREECIV_WEB
#define MAP_DEFAULT_SIZE         3
#define MAP_MIN_SIZE             0
#define MAP_MAX_SIZE             38
#else  /* FREECIV_WEB */
#define MAP_DEFAULT_SIZE         4
#define MAP_MIN_SIZE             0
#define MAP_MAX_SIZE             2048
#endif /* FREECIV_WEB */

FC_STATIC_ASSERT(MAP_MAX_SIZE * 1000 <= MAX_DBV_LENGTH,
                 map_too_big_for_bitvector);
/* We communicate through the network with signed 32-bits integers. */
FC_STATIC_ASSERT((long unsigned) MAP_MAX_SIZE * 1000
                 < (long unsigned) 1 << 31,
                 map_too_big_for_network);

#define MAP_DEFAULT_TILESPERPLAYER      100
#define MAP_MIN_TILESPERPLAYER            1
#define MAP_MAX_TILESPERPLAYER         1000

/* This defines the maximum linear size in _native_ coordinates. */
#define MAP_DEFAULT_LINEAR_SIZE  64
#define MAP_MAX_LINEAR_SIZE      (MAP_MAX_SIZE * 1000 / MAP_MIN_LINEAR_SIZE)
#define MAP_MIN_LINEAR_SIZE      16

/* The distance between two points at a map shouldn't be larger than this.
Adds MAP_MIN_LINEAR_SIZE because hex topologies forbids certain diagonal
moves. Includes MAP_MAX_LINEAR_SIZE because a map can be non wrapping. */
#define MAP_DISTANCE_MAX (MAP_MAX_LINEAR_SIZE + MAP_MIN_LINEAR_SIZE)

#define MAP_ORIGINAL_TOPO        TF_WRAPX
#ifdef FREECIV_WEB
/* Freeciv-web doesn't support isometric maps yet. */
#define MAP_DEFAULT_TOPO         TF_WRAPX
#else /* FREECIV_WEB */
#define MAP_DEFAULT_TOPO         (TF_WRAPX|TF_ISO|TF_HEX)
#endif /* FREECIV_WEB */

#define MAP_DEFAULT_SEED         0
#define MAP_MIN_SEED             0
#define MAP_MAX_SEED             (MAX_UINT32 >> 1)

#define MAP_DEFAULT_LANDMASS     30
#define MAP_MIN_LANDMASS         15
#define MAP_MAX_LANDMASS         85

#define MAP_DEFAULT_RICHES       250
#define MAP_MIN_RICHES           0
#define MAP_MAX_RICHES           1000

#define MAP_DEFAULT_STEEPNESS    30
#define MAP_MIN_STEEPNESS        0
#define MAP_MAX_STEEPNESS        100

#define MAP_DEFAULT_WETNESS      50
#define MAP_MIN_WETNESS          0
#define MAP_MAX_WETNESS          100

#define MAP_DEFAULT_GENERATOR    MAPGEN_RANDOM

#define MAP_DEFAULT_STARTPOS     MAPSTARTPOS_DEFAULT

#define MAP_DEFAULT_TINYISLES    FALSE
#define MAP_MIN_TINYISLES        FALSE
#define MAP_MAX_TINYISLES        TRUE

#define MAP_DEFAULT_SEPARATE_POLES   TRUE
#define MAP_MIN_SEPARATE_POLES       FALSE
#define MAP_MAX_SEPARATE_POLES       TRUE

#define MAP_DEFAULT_FLATPOLES     100
#define MAP_MIN_FLATPOLES         0
#define MAP_MAX_FLATPOLES         100

#define MAP_DEFAULT_SINGLE_POLE    FALSE
#define MAP_MIN_SINGLE_POLE        FALSE
#define MAP_MAX_SINGLE_POLE        TRUE

#define MAP_DEFAULT_ALLTEMPERATE   FALSE
#define MAP_MIN_ALLTEMPERATE       FALSE
#define MAP_MAX_ALLTEMPERATE       TRUE

#define MAP_DEFAULT_TEMPERATURE   50
#define MAP_MIN_TEMPERATURE       0
#define MAP_MAX_TEMPERATURE       100

#define MAP_DEFAULT_TEAM_PLACEMENT  TEAM_PLACEMENT_CLOSEST

/*
 * Inline function definitions.  These are at the bottom because they may use
 * elements defined above.
 */

static inline int map_pos_to_index(struct civ_map *nmap, int map_x, int map_y)
{
  /* Note: writing this as a macro is hard; it needs temp variables. */
  int nat_x, nat_y;

  CHECK_MAP_POS(map_x, map_y);
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, map_x, map_y);
  return native_pos_to_index(nat_x, nat_y);
}

static inline int index_to_map_pos_x(int mindex)
{
  /* Note: writing this as a macro is hard; it needs temp variables. */
  int map_x, map_y;

  index_to_map_pos(&map_x, &map_y, mindex);
  return map_x;
}

static inline int index_to_map_pos_y(int mindex)
{
  /* Note: writing this as a macro is hard; it needs temp variables. */
  int map_x, map_y;

  index_to_map_pos(&map_x, &map_y, mindex);
  return map_y;
}

/****************************************************************************
  A "border position" is any map position that _may have_ positions within
  real map distance dist that are non-normal.  To see its correctness,
  consider the case where dist is 1 or 0.
****************************************************************************/
static inline bool is_border_tile(const struct tile *ptile, int dist)
{
  /* HACK: An iso-map compresses the value in the X direction but not in
   * the Y direction.  Hence (x+1,y) is 1 tile away while (x,y+2) is also
   * one tile away. */
  int xdist = dist;
  int ydist = (MAP_IS_ISOMETRIC ? (2 * dist) : dist);
  int nat_x, nat_y;

  index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));

  return (nat_x < xdist 
          || nat_y < ydist
          || nat_x >= wld.map.xsize - xdist
          || nat_y >= wld.map.ysize - ydist);
}

enum direction8 rand_direction(void);
enum direction8 opposite_direction(enum direction8 dir);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__MAP_H */
