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
#ifndef FC__MAP_H
#define FC__MAP_H

#include <assert.h>
#include <math.h>

#include "fc_types.h"

#include "tile.h"

/*
 * The value of MOVE_COST_FOR_VALID_SEA_STEP has no particular
 * meaning. The value is only used for comparison. The value must be
 * <0.
 */
#define MOVE_COST_FOR_VALID_SEA_STEP	(-3)
#define MOVE_COST_FOR_VALID_AIR_STEP	(-3)

/****************************************************************
miscellaneous terrain information
*****************************************************************/
#define terrain_misc packet_ruleset_terrain_control

struct hash_table;

struct civ_map {
  int topology_id;
  enum direction8 valid_dirs[8], cardinal_dirs[8];
  int num_valid_dirs, num_cardinal_dirs;
  struct iter_index *iterate_outwards_indices;
  int num_iterate_outwards_indices;
  int xsize, ysize; /* native dimensions */
  int num_continents;
  int num_oceans;               /* not updated at the client */
  struct tile *tiles;
  struct hash_table *startpos_table;

  union {
    struct {
      /* Nothing yet. */
    } client;

    struct {
      int size; /* used to calculate [xy]size */
      int seed;
      int riches;
      int huts;
      int landpercent;
      int generator;
      int startpos;
      bool tinyisles;
      bool separatepoles;
      bool alltemperate;
      int temperature;
      int wetness;
      int steepness;
      int num_start_positions;
      bool have_resources;
      bool ocean_resources;         /* Resources in the middle of the ocean */
      bool have_huts;
      bool have_rivers_overlay;	/* only applies if !have_resources */

      struct start_position {
        struct tile *tile;
        struct nation_type *nation; /* May be NO_NATION_SELECTED. */
      } *start_positions;	/* allocated at runtime */
    } server;
  };
};

enum topo_flag {
  /* Bit-values. */
  /* Changing these values will break map_init_topology. */
  TF_WRAPX = 1,
  TF_WRAPY = 2,
  TF_ISO = 4,
  TF_HEX = 8
};

/* Parameters for terrain counting functions. */
static const bool C_ADJACENT = FALSE;
static const bool C_CARDINAL = TRUE;
static const bool C_NUMBER = FALSE;
static const bool C_PERCENT = TRUE;

#define MAP_IS_ISOMETRIC (topo_has_flag(TF_ISO) || topo_has_flag(TF_HEX))

#define CURRENT_TOPOLOGY (map.topology_id)

#define topo_has_flag(flag) ((CURRENT_TOPOLOGY & (flag)) != 0)

bool map_is_empty(void);
void map_init(void);
void map_init_topology(bool set_sizes);
void map_allocate(void);
void map_free(void);

int map_vector_to_real_distance(int dx, int dy);
int map_vector_to_sq_distance(int dx, int dy);
int map_distance(const struct tile *tile0, const struct tile *tile1);
int real_map_distance(const struct tile *tile0, const struct tile *tile1);
int sq_map_distance(const struct tile *tile0,const  struct tile *tile1);

bool same_pos(const struct tile *tile0, const struct tile *tile1);
bool base_get_direction_for_step(const struct tile *src_tile,
				 const struct tile *dst_tile,
				 enum direction8 *dir);
int get_direction_for_step(const struct tile *src_tile,
			   const struct tile *dst_tile);

void map_set_startpos(const struct tile *ptile,
                      const struct nation_type *pnation);
const struct nation_type *map_get_startpos(const struct tile *ptile);
void map_clear_startpos(const struct tile *ptile);


/* Number of index coordinates (for sanity checks and allocations) */
#define MAP_INDEX_SIZE (map.xsize * map.ysize)

#ifdef DEBUG
#define CHECK_MAP_POS(x,y) assert(is_normal_map_pos((x),(y)))
#define CHECK_NATIVE_POS(x, y) assert((x) >= 0 && (x) < map.xsize \
				      && (y) >= 0 && (y) < map.ysize)
#define CHECK_INDEX(index) assert((index) >= 0 && (index) < MAP_INDEX_SIZE)
#else
#define CHECK_MAP_POS(x,y) ((void)0)
#define CHECK_NATIVE_POS(x, y) ((void)0)
#define CHECK_INDEX(index) ((void)0)
#endif

#define native_pos_to_index(nat_x, nat_y)                                   \
  (CHECK_NATIVE_POS((nat_x), (nat_y)),					    \
   (nat_x) + (nat_y) * map.xsize)
#define index_to_native_pos(pnat_x, pnat_y, index)                          \
  (*(pnat_x) = (index) % map.xsize,                                         \
   *(pnat_y) = (index) / map.xsize)

/* Obscure math.  See explanation in doc/HACKING. */
#define NATIVE_TO_MAP_POS(pmap_x, pmap_y, nat_x, nat_y)                     \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pmap_x) = ((nat_y) + ((nat_y) & 1)) / 2 + (nat_x),                  \
      *(pmap_y) = (nat_y) - *(pmap_x) + map.xsize)                          \
   : (*(pmap_x) = (nat_x), *(pmap_y) = (nat_y)))

#define MAP_TO_NATIVE_POS(pnat_x, pnat_y, map_x, map_y)                     \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pnat_y) = (map_x) + (map_y) - map.xsize,                            \
      *(pnat_x) = (2 * (map_x) - *(pnat_y) - (*(pnat_y) & 1)) / 2)          \
   : (*(pnat_x) = (map_x), *(pnat_y) = (map_y)))

#define NATURAL_TO_MAP_POS(pmap_x, pmap_y, nat_x, nat_y)                    \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pmap_x) = ((nat_y) + (nat_x)) / 2,                                  \
      *(pmap_y) = (nat_y) - *(pmap_x) + map.xsize)                          \
   : (*(pmap_x) = (nat_x), *(pmap_y) = (nat_y)))

#define MAP_TO_NATURAL_POS(pnat_x, pnat_y, map_x, map_y)                    \
  (MAP_IS_ISOMETRIC							    \
   ? (*(pnat_y) = (map_x) + (map_y) - map.xsize,                            \
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
#define NATIVE_WIDTH map.xsize
#define NATIVE_HEIGHT map.ysize

/* Width and height of the map, in natural coordinates. */
#define NATURAL_WIDTH (MAP_IS_ISOMETRIC ? 2 * map.xsize : map.xsize)
#define NATURAL_HEIGHT map.ysize

#define MAP_WIDTH  \
  (MAP_IS_ISOMETRIC ? (map.xsize + map.ysize / 2) : map.xsize)
#define MAP_HEIGHT \
  (MAP_IS_ISOMETRIC ? (map.xsize + map.ysize / 2) : map.ysize)
  
static inline int map_pos_to_index(int map_x, int map_y);

/* index_to_map_pos(int *, int *, int) inverts map_pos_to_index */
#define index_to_map_pos(pmap_x, pmap_y, index) \
  (CHECK_INDEX(index),                          \
   index_to_native_pos(pmap_x, pmap_y, index),  \
   NATIVE_TO_MAP_POS(pmap_x, pmap_y, *(pmap_x), *(pmap_y)))

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
struct tile *mapstep(const struct tile *ptile, enum direction8 dir);

struct tile *map_pos_to_tile(int x, int y);
struct tile *native_pos_to_tile(int nat_x, int nat_y);
struct tile *index_to_tile(int index);

bool is_real_map_pos(int x, int y);
bool is_normal_map_pos(int x, int y);

bool is_singular_tile(const struct tile *ptile, int dist);
bool normalize_map_pos(int *x, int *y);
struct tile *nearest_real_tile(int x, int y);
void base_map_distance_vector(int *dx, int *dy,
			      int x0, int y0, int x1, int y1);
void map_distance_vector(int *dx, int *dy, const struct tile *ptile0,
			 const struct tile *ptile1);
int map_num_tiles(void);

struct tile *rand_neighbour(const struct tile *ptile);
struct tile *rand_map_pos(void);
struct tile *rand_map_pos_filtered(void *data,
				   bool (*filter)(const struct tile *ptile,
						  const void *data));

bool is_water_adjacent_to_tile(const struct tile *ptile);
bool is_tiles_adjacent(const struct tile *ptile0, const struct tile *ptile1);
bool is_move_cardinal(const struct tile *src_tile,
		      const struct tile *dst_tile);
int map_move_cost_unit(struct unit *punit, const struct tile *ptile);
int map_move_cost_ai(const struct tile *tile0, const struct tile *tile1);
int map_move_cost(const struct tile *src_tile, const struct tile *dst_tile);
bool is_safe_ocean(const struct tile *ptile);
bool is_cardinally_adj_to_ocean(const struct tile *ptile);
bv_special get_tile_infrastructure_set(const struct tile *ptile,
					  int *count);

bool can_channel_land(const struct tile *ptile);
bool can_reclaim_ocean(const struct tile *ptile);

extern struct civ_map map;

extern struct terrain_misc terrain_control;

/* This iterates outwards from the starting point. Every tile within max_dist
 * will show up exactly once, in an outward (based on real map distance)
 * order.  The returned values are always real and are normalized.  The
 * starting position must be normal.
 *
 * See also iterate_outward() */
#define iterate_outward_dxy(start_tile, max_dist, _tile, _x, _y)	    \
{									    \
  int _x, _y, _tile##_x, _tile##_y;					    \
  struct tile *_tile;							    \
  const struct tile *_tile##_start = (start_tile);			    \
  int _tile##_max = (max_dist);						    \
  bool _tile##_is_border = is_border_tile(_tile##_start, _tile##_max);	    \
  int _tile##_index = 0;						    \
  for (;								    \
       _tile##_index < map.num_iterate_outwards_indices;		    \
       _tile##_index++) { 						    \
    if (map.iterate_outwards_indices[_tile##_index].dist > _tile##_max) {   \
      break;								    \
    }									    \
    _x = map.iterate_outwards_indices[_tile##_index].dx;		    \
    _y = map.iterate_outwards_indices[_tile##_index].dy;		    \
    _tile##_x = _x + _tile##_start->x;					    \
    _tile##_y = _y + _tile##_start->y;					    \
    if (_tile##_is_border && !normalize_map_pos(&_tile##_x, &_tile##_y)) {  \
      continue;								    \
    }									    \
    _tile = map.tiles + map_pos_to_index(_tile##_x, _tile##_y);

#define iterate_outward_dxy_end						    \
  }									    \
}

/* See iterate_outward_dxy() */
#define iterate_outward(start_tile, max_dist, itr_tile)			    \
  iterate_outward_dxy(start_tile, max_dist, itr_tile, _dx_itr, _dy_itr)

#define iterate_outward_end iterate_outward_dxy_end

/* 
 * Iterate through all tiles in a square with given center and radius.
 * The position (x_itr, y_itr) that is returned will be normalized;
 * unreal positions will be automatically discarded. (dx_itr, dy_itr)
 * is the standard distance vector between the position and the center
 * position. Note that when the square is larger than the map the
 * distance vector may not be the minimum distance vector.
 */
#define square_dxy_iterate(center_tile, radius, tile_itr, dx_itr, dy_itr)   \
  iterate_outward_dxy(center_tile, radius, tile_itr, dx_itr, dy_itr)

#define square_dxy_iterate_end iterate_outward_dxy_end

/*
 * Iterate through all tiles in a square with given center and radius.
 * Positions returned will have adjusted x, and positions with illegal
 * y will be automatically discarded.
 */
#define square_iterate(center_tile, radius, tile_itr)			    \
  square_dxy_iterate(center_tile, radius, tile_itr, _dummy_x, dummy_y)

#define square_iterate_end  square_dxy_iterate_end

/* 
 * Iterate through all tiles in a circle with given center and squared
 * radius.  Positions returned will have adjusted (x, y); unreal
 * positions will be automatically discarded. 
 */
#define circle_iterate(center_tile, sq_radius, tile_itr)		    \
  circle_dxyr_iterate(center_tile, sq_radius, tile_itr, _dx, _dy, _dr)

#define circle_iterate_end                                                  \
  circle_dxyr_iterate_end

/* dx, dy, dr are distance from center to tile in x, y and square distance;
 * do not rely on x, y distance, since they do not work for hex topologies */
#define circle_dxyr_iterate(center_tile, sq_radius,			    \
			    _tile, dx, dy, dr)				    \
{									    \
  const int _tile##_sq_radius = (sq_radius);				    \
  const int _tile##_cr_radius = (int)sqrt((double)MAX(_tile##_sq_radius, 0)); \
									    \
  square_dxy_iterate(center_tile, _tile##_cr_radius, _tile, dx, dy) {	    \
    const int dr = map_vector_to_sq_distance(dx, dy);			    \
									    \
    if (dr <= _tile##_sq_radius) {

#define circle_dxyr_iterate_end						    \
    }									    \
  } square_dxy_iterate_end;						    \
}

/* Iterate through all map positions adjacent to the given center map
 * position, with normalization.  The order of positions is unspecified. */
#define adjc_iterate(center_tile, itr_tile)				    \
{									    \
  /* Written as a wrapper to adjc_dir_iterate since it's the cleanest and   \
   * most efficient. */							    \
  adjc_dir_iterate(center_tile, itr_tile, ADJC_ITERATE_dir_itr) {

#define adjc_iterate_end                                                    \
  } adjc_dir_iterate_end;                                                   \
}

#define adjc_dir_iterate(center_tile, itr_tile, dir_itr)		    \
  adjc_dirlist_iterate(center_tile, itr_tile, dir_itr,			    \
		       map.valid_dirs, map.num_valid_dirs)

#define adjc_dir_iterate_end adjc_dirlist_iterate_end

#define cardinal_adjc_iterate(center_tile, itr_tile)			    \
  adjc_dirlist_iterate(center_tile, itr_tile, _dir_itr,			    \
		       map.cardinal_dirs, map.num_cardinal_dirs)

#define cardinal_adjc_iterate_end adjc_dirlist_iterate_end

#define cardinal_adjc_dir_iterate(center_tile, itr_tile, dir_itr)	    \
  adjc_dirlist_iterate(center_tile, itr_tile, dir_itr,			    \
		       map.cardinal_dirs, map.num_cardinal_dirs)

#define cardinal_adjc_dir_iterate_end adjc_dirlist_iterate_end

/* Iterate through all tiles adjacent to a tile using the given list of
 * directions.  _dir is the directional value, (center_x, center_y) is
 * the center tile (which must be normalized).
 *
 * This macro should not be used directly.  Instead, use adjc_dir_iterate
 * or cartesian_adjacent_iterate. */
#define adjc_dirlist_iterate(center_tile, _tile, _dir,			    \
			     dirlist, dircount)				    \
{									    \
  enum direction8 _dir;							    \
  int _tile##_x, _tile##_y;						    \
  struct tile *_tile;							    \
  const struct tile *_tile##_center = (center_tile);			    \
  bool _tile##_is_border = is_border_tile(_tile##_center, 1);		    \
  int _tile##_index = 0;						    \
  for (;								    \
       _tile##_index < (dircount);					    \
       _tile##_index++) {						    \
    _dir = dirlist[_tile##_index];					    \
    DIRSTEP(_tile##_x, _tile##_y, _dir);				    \
    _tile##_x += _tile##_center->x;					    \
    _tile##_y += _tile##_center->y;					    \
    if (_tile##_is_border && !normalize_map_pos(&_tile##_x, &_tile##_y)) {  \
      continue;								    \
    }									    \
    _tile = map.tiles + map_pos_to_index(_tile##_x, _tile##_y);

#define adjc_dirlist_iterate_end					    \
    }									    \
}

/* Iterate over all positions on the globe.
 * Use index positions for cache efficiency. */
#define whole_map_iterate(_tile)					    \
{									    \
  struct tile *_tile;							    \
  int _tile##_index = 0;						    \
  for (;								    \
       _tile##_index < MAP_INDEX_SIZE;					    \
       _tile##_index++) {						    \
    _tile = map.tiles + _tile##_index;

#define whole_map_iterate_end						    \
  }									    \
}

BV_DEFINE(dir_vector, 8);

/* return the reverse of the direction */
#define DIR_REVERSE(dir) (7 - (dir))

enum direction8 dir_cw(enum direction8 dir);
enum direction8 dir_ccw(enum direction8 dir);
const char* dir_get_name(enum direction8 dir);
bool is_valid_dir(enum direction8 dir);
bool is_cardinal_dir(enum direction8 dir);

extern const int DIR_DX[8];
extern const int DIR_DY[8];

/* Used for network transmission; do not change. */
#define MAP_TILE_OWNER_NULL	 MAX_UINT8

#define MAP_DEFAULT_HUTS         50
#define MAP_MIN_HUTS             0
#define MAP_MAX_HUTS             500

/* Size of the map in thousands of tiles */
#define MAP_DEFAULT_SIZE         3
#define MAP_MIN_SIZE             1
#define MAP_MAX_SIZE             15

/* This defines the maximum linear size in _map_ coordinates.
 * This must be smaller than 255 because of the way coordinates are sent
 * across the network. */
#define MAP_MAX_LINEAR_SIZE      254
#define MAP_MIN_LINEAR_SIZE      8
#define MAP_MAX_WIDTH            MAP_MAX_LINEAR_SIZE
#define MAP_MAX_HEIGHT           MAP_MAX_LINEAR_SIZE

#define MAP_ORIGINAL_TOPO        TF_WRAPX
#define MAP_DEFAULT_TOPO         TF_WRAPX
#define MAP_MIN_TOPO             0
#define MAP_MAX_TOPO             15

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

#define MAP_DEFAULT_GENERATOR    1
#define MAP_MIN_GENERATOR        1
#define MAP_MAX_GENERATOR        3

#define MAP_DEFAULT_STARTPOS     0
#define MAP_MIN_STARTPOS         0
#define MAP_MAX_STARTPOS         4

#define MAP_DEFAULT_TINYISLES    FALSE
#define MAP_MIN_TINYISLES        FALSE
#define MAP_MAX_TINYISLES        TRUE

#define MAP_DEFAULT_SEPARATE_POLES   TRUE
#define MAP_MIN_SEPARATE_POLES       FALSE
#define MAP_MAX_SEPARATE_POLES       TRUE

#define MAP_DEFAULT_ALLTEMPERATE   FALSE
#define MAP_MIN_ALLTEMPERATE       FALSE
#define MAP_MAX_ALLTEMPERATE       TRUE

#define MAP_DEFAULT_TEMPERATURE   50
#define MAP_MIN_TEMPERATURE       0
#define MAP_MAX_TEMPERATURE       100

/*
 * Inline function definitions.  These are at the bottom because they may use
 * elements defined above.
 */

static inline int map_pos_to_index(int map_x, int map_y)
{
  /* Note: writing this as a macro is hard; it needs temp variables. */
  int nat_x, nat_y;

  CHECK_MAP_POS(map_x, map_y);
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, map_x, map_y);
  return native_pos_to_index(nat_x, nat_y);
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

  return (ptile->nat_x < xdist 
	  || ptile->nat_y < ydist
	  || ptile->nat_x >= map.xsize - xdist
	  || ptile->nat_y >= map.ysize - ydist);
}

#endif  /* FC__MAP_H */
