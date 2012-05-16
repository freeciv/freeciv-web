/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__PATH_FINDING_H
#define FC__PATH_FINDING_H

#include "map.h"
#include "tile.h"
#include "unit.h"
#include "unittype.h"

/* ========================= Explanations =============================== */

/*
 * Functions in this file help to find a path from A to B.
 *
 * 
 * 
 * DEFINITIONS:
 *   step: one movement step which brings us from one tile to an
 *   adjacent one
 *
 *   turn: turns spent to reach a tile.  Since movement rules involve
 *   randomness, we use different "turn modes" to get an estimate of
 *   this number
 *
 *   moves left: move points left upon reaching a tile.  Also depends
 *   on "turn mode".
 *
 *   turn mode (TM): method of emulating the randomness of movement
 *   (see enum turn_mode below)
 *
 *   path: a list of steps which leads from the start to the end
 *
 *   move cost (MC): move cost of a _single_ step.  MC is always >= 0. 
 *     [The parameter can specify what the MC of a step into the unknown is
 *      to be (this is a constant for each map).  This defaults to a
 *      slightly large value meaning unknown tiles are avoided slightly.
 *      It's also possible to use 0 here and use TB or EC to control
 *      movement through unknown tiles, or to use PF_IMPOSSIBLE_MC to
 *      easily avoid unknown tiles.]
 *
 *   extra cost (EC): extra cost of a _single_ tile.  EC is always >= 0.
 *   The intended meaning for EC is "how much we want to avoid this tile",
 *   see DISCUSSION below for more.
 *
 *   tile behaviour (TB): the extra information about a tile which
 *   tells us whether we can enter and leave tile as normal (see enum
 *   tile_behavior).
 *
 *   total_MC: (effective) move cost of the whole path.  Calculated
 *   depending on the turn mode (see FORMULAE below).
 *
 *   total_EC: extra cost of the whole path (just sum of ECs of all
 *   tiles).
 *
 *   total_CC: combined cost of the whole path (see below).
 *
 *   best path: a path which has the minimal total_CC. If two paths
 *   have equal total_CC the behavior is undefined.
 *
 *
 * DISCUSSION:
 * First of all it should be noted that path-finding is not about finding
 * shortest paths but about finding the "best" paths.  Now, each path
 * has two major characteristics:
 * (1) How long it is (or how much does it cost in terms of time)
 * and
 * (2) How dangerous it is (or how much does it cost in terms of
 *     resources).
 *
 * We use MC (and total_MC) to describe (1) and use EC (and total_EC) to
 * describe (2).  Of course, when it comes to selecting the "best" path,
 * we need to compromise between taking the shortest road and taking the
 * safest road.  To that end, we use the "combined cost", calculated as
 *   total_CC = PF_TURN_FACTOR * total_MC + move_rate * total_EC,
 * where PF_TURN_FACTOR is a large constant.  This means that each EC
 * point is equivalent to 1/PF_TURN_FACTOR-th of one turn, or,
 * equivalently, we are willing to spend one more turn on the road to
 * avoid PF_TURN_FACTOR worth of danger.  Note that if ECs are kept
 * significantly lower than PF_TURN_FACTOR, the total_EC will only act as a
 * tie-breaker between equally long paths.
 * 
 * Note, that the user is expected to ask "So this best path, how long will 
 * it take me to walk it?".  For that reason we keep our accounts of MCs and
 * ECs separately, as one cannot answer the above question basing on
 * total_CC alone.
 * 
 * The above setup allows us to find elegant solutions to rather involved
 * questions.  A good example would be designing a road from A to B:
 * ================
 * The future road should be as short as possible, but out of many
 * shortest roads we want to select the one which is fastest to
 * construct.  For example:
 * the initial conditions ("-"s mark existing road, "."s mark plains)
 *  A.....B
 *   .....
 *   .---.
 * a possible solution
 *  A-----B
 *   ..... 
 *   .---.
 * the best solution (utilize existing road)
 *  A.....B
 *   \.../
 *   .---.
 * 
 * To solve the problem we simply declare that
 *   MC between any two tiles shall be MOVE_COST_ROAD
 *   EC of any tile shall be the time to construct a road there
 * =================
 * 
 * In some cases we would like to impose extra restrictions on the
 * paths/tiles we want to consider.  For example, a trireme might want to
 * never enter deep sea.  A chariot, would like to find paths going to
 * enemy cities but not going _through_ them.  This can be achieved
 * through an additional tile_behaviour callback,  which would return
 * TB_IGNORE for tiles we don't want to visit and TB_DONT_LEAVE for tiles
 * we won't be able to leave (at least alive).
 *
 * Dangerous tiles are those on which we don't want to end a turn.  If
 * the danger callback is specified it is used to determine which tiles are
 * dangerous; no path that ends a turn on such a tile will ever be
 * considered.
 *
 * There is also support for fuel, and thus indirectly for air units.  If
 * the fuel parameters are provided then the unit is considered to have
 * that much fuel.  The net effect is that if a unit has N fuel then only
 * every Nth turn will be considered a stopping point.  To support air
 * units, then, all tiles that don't have airfields (or cities/carriers)
 * should be returned as dangerous (see danger, above).  Note: fuel support
 * will only work if all moves have equal MC.  Support for fuel is
 * experimental at this time (Jun 2005) and should not be used in the
 * server.  Setting fuel==1 in the pf_parameter disables fuel support.
 * 
 * There are few other options in the path-finding, including "omniscience" 
 * (if true, all tiles are assumed to be KNOWN) and "get_zoc" callback (if 
 * not NULL, we will consider restrictions imposed upon our movements by 
 * zones of control).
 *
 *  
 * FORMULAE:
 *   For calculating total_MC (given particular tile_behaviour)
 *     - TM_NONE: total_MC = sum of MC
 *     - TM_CAPPED: total_MC = sum of MIN(MC, move_rate)
 *     - TM_*_TIME: total_MC = ((turn + 1) * move_rate - moves_left)
 *  
 *   For calculating total_CC:
 *     total_CC = PF_TURN_FACTOR * total_MC + move_rate * total_EC
 *
 *
 * EXAMPLES:
 * The path-finding can be used in two major modes:
 * (A) We know where we want to go and need the best path there
 *      How many miles to Dublin?
 *      Three score and ten, sir.
 *      Will we be there by candlelight?
 * (B) We know what we want and need the nearest one
 *      Where is the nearest pub, gov?
 *
 * In the first case we use pf_get_path and pf_get_position (the latter
 * will only tell us the distance but not the path to take).  In the
 * second case we use pf_next to iterate through all tiles in the order
 * of increased distance and break when we found what we were looking
 * for.  In this case we use pf_next_get_* to get information about the
 * "next closest tile".
 *
 * A) the caller knows the map position of the goal and wants to know the 
 * path:
 *
 * struct pf_parameter parameter;
 * struct pf_map *pfm;
 * struct pf_path *path;
 * 
 * // fill parameter (see below)
 *
 * pfm = pf_map_new(&parameter);
 *
 * if ((path = pf_map_get_path(pfm, ptile))) {
 *   // success, use path
 *   pf_path_destroy(path);
 * } else {
 *   // no path could be found
 * }
 *
 * pf_map_destroy(pfm);
 *
 * You may call pf_map_get_path multiple times with the same pfm.
 *
 * B) the caller doesn't know the map position of the goal yet (but knows 
 * what he is looking for, e.g. a port) and wants to iterate over
 * all paths in order of increasing costs (total_CC):
 *
 * struct pf_parameter parameter;
 * struct pf_map *pfm;
 * 
 * // fill parameter (see below)
 *
 * pfm = pf_map_new(&parameter);
 *
 * pf_map_iterate_*(pfm, something, TRUE) {
 *
 * } pf_map_iterate_*_end;
 *
 * pf_map_destroy(pfm);
 *
 * Depending on the kind of information required, the iteration macro
 * should be:
 *
 *  1) tile iteration:
 *     pf_map_iterate_tiles(pfm, ptile, TRUE) {
 *       // do something
 *     } pf_map_iterate_tiles_end;
 *
 *  2) move cost iteration on the nearest position:
 *     pf_map_iterate_move_costs(pfm, ptile, move_cost, TRUE) {
 *       // do something
 *     } pf_map_iterate_move_costs_end;
 *
 *  3) information (for example the total MC and the number of turns) on the 
 *  next nearest position:
 *     pf_map_iterate_positions(pfm, pos, TRUE) {
 *       // do something
 *     } pf_map_iterate_positions_end;
 *
 *  4) information about the whole path (leading to the next nearest position):
 *     pf_map_iterate_pathes(pfm, path, TRUE) {
 *       // do something
 *       pf_path_destroy(path);
 *     } pf_map_iterate_pathes_end;
 *
 * The third argument passed to the iteration macros is a condition that
 * controls if the start tile of the pf_parameter should iterated or not.
 *
 *
 * FILLING the struct pf_parameter:
 * This can either be done by hand or using the pft_* functions from
 * path_finding_tools or a mix of these.
 *
 * Hints:
 * 1. It is useful to limit the expansion of unknown tiles with a get_TB
 * callback.  In this case you might set the unknown_MC to be 0.
 * 2. If there are two paths of the same cost to a tile, you are
 * not guaranteed to get the one with the least steps in it.  If you care,
 * specifying EC to be 1 will do the job.
 * 3. To prevent AI from thinking that it can pass through "chokepoints"
 * controlled by enemy cities, you can specify tile behaviour of each occupied 
 * enemy city to be TB_DONT_LEAVE.
 */

/* MC for an impossible step. If this value is returned by get_MC it
 * is treated like TB_IGNORE for this step. This won't change the TB
 * for any other step to this tile. */
#define PF_IMPOSSIBLE_MC -1

/* The factor which is used to multiple total_EC in the total_CC
 * calculation. See definition of total_CC above.
 * The number is chosen to be much larger than 0 and much smaller 
 * than MAX_INT (and a power of 2 for easy multiplication). */
#define PF_TURN_FACTOR  65536

/* ========================= Structures =============================== */

/* Specifies the way path-finding will treat a tile. */
enum tile_behavior {
  TB_IGNORE,			/* This one will be ignored */
  TB_DONT_LEAVE,		/* Paths can lead _to_ such tile, 
				 * but are not allowed to go _through_ */
  TB_NORMAL			/* Well, normal */
};

/* Specifies how total_MC, turn and moves_left fields in the positions
 * (struct pf_position) are computed. */
enum turn_mode {
  /* No turn numbers or moves_left are used at all. The fields "turn"
   * and "moves_left" of struct pf_position will always be set to -1 in 
   * this mode. */
  TM_NONE,

  /* Similar to TM_NONE. The MC, however, is capped at the
   * move_rate. The fields "turn" and "moves_left" will always be -1. */
  TM_CAPPED,

  /* Assumes that the unit is always lucky in the random rulings and
   * so yields the best travel time. */
  TM_BEST_TIME,

  /* Assumes that the unit is never lucky in the random rulings and so
   * yields the worst travel time. */
  TM_WORST_TIME
};

/* Full specification of a position and time to reach it. */
struct pf_position {
  struct tile *tile;
  int turn, moves_left, fuel_left;	/* See definitions above */

  int total_MC;			/* Total MC to reach this point */
  int total_EC;			/* Total EC to reach this point */

  enum direction8 dir_to_next_pos;	/* Unsed only in struct_path */
  enum direction8 dir_to_here;	/* Where did we come from */
};

/* Full specification of a path. */
struct pf_path {
  int length;    		/* Number of steps in the path */
  struct pf_position *positions;
};

/* Initial data for the path-finding.  Normally should use functions
 * from pf_tools.[ch] to fill the parameter.
 *
 * All callbacks get the parameter passed to pf_create_map as the last
 * argument. 
 *
 * Examples of callbacks can be found in pf_tools.c
 * NB: It should be safe to struct copy pf_parameter. */
struct pf_parameter {
  struct tile *start_tile;	/* Initial position */

  int moves_left_initially;
  int fuel_left_initially;      /* Ignored for non-air units. */

  int move_rate;		/* Move rate of the virtual unit */
  int fuel;                     /* Should be 1 for units without fuel. */

  struct player *owner;
  const struct unit_class *uclass;

  bv_flags unit_flags;          /* Like F_MARINE and F_TRIREME */
  bool omniscience;		/* Do we care if the tile is visible? */

  enum turn_mode turn_mode;	/* See definitions. */

  /* Callback to get MC of a move from (from_x, from_y) to (to_x,
   * to_y) and in the direction dir. Note that the callback can
   * calculate (to_x, to_y) by itself based on (from_x, from_y) and
   * dir. Excessive information (to_x, to_y) is provided to ease the 
   * implementation of the callback. */
  int (*get_MC) (const struct tile *from_tile, enum direction8 dir,
                 const struct tile *to_tile,
                 const struct pf_parameter *param);
  int unknown_MC; /* Move cost into unknown - very large by default */

  /* Callback which determines the behavior of a tile. If NULL
   * TB_NORMAL is assumed. It can be assumed that the implementation
   * of path_finding.h will cache this value. */
  enum tile_behavior (*get_TB) (const struct tile *ptile,
                                enum known_type known,
                                const struct pf_parameter *param);

  /* Callback which can be used to provide extra costs depending on
   * the tile. Can be NULL. It can be assumed that the implementation
   * of path_finding.h will cache this value. */
  int (*get_EC) (const struct tile *ptile, enum known_type known,
                 const struct pf_parameter *param);

  /* Callback to determines if we could invade the tile.  Returns TRUE
   * if we can enter in the territory of the tile owner. */
  bool (*can_invade_tile) (const struct player *pplayer,
                           const struct tile *ptile);

  /* Although the rules governing ZoC are universal, the amount of
   * information available at server and client is different. To 
   * compensate for it, we might need to supply our own version 
   * of "common" is_my_zoc.  Also AI might need to partially ignore 
   * ZoC for strategic planning purposes (take into account enemy cities 
   * but not units for example).
   * If this callback is NULL, ZoC are ignored.*/
  bool (*get_zoc) (const struct player *pplayer, const struct tile *ptile);

  /* If this callback is non-NULL and returns TRUE this position is
   * dangerous. The unit will never end a turn at a dangerous
   * position. Can be NULL. */
  bool (*is_pos_dangerous) (const struct tile *ptile, enum known_type,
                            const struct pf_parameter *param);

  /* If this callback is non-NULL and returns the required moves left to
   * move to this tile and to leave the position safely. Can be NULL. */
  int (*get_moves_left_req) (const struct tile *ptile, enum known_type,
                             const struct pf_parameter *param);

  /* This is a jumbo callback which overrides all previous ones.  It takes 
   * care of everything (ZOC, known, costs etc).  
   * Variables:
   *   from_tile             -- the source tile
   *   from_cost, from_extra -- costs of the source tile
   *   to_tile               -- the dest tile
   *   to_cost, to_extra     -- costs of the dest tile
   *   dir                   -- direction from source to dest
   *   param                 -- a pointer to this struct
   * If the dest tile hasn't been reached before, to_cost is -1.
   *
   * The callback should:
   * - evaluate the costs of the move
   * - calculate the cost of the whole path
   * - compare it to the ones recorded at dest tile
   * - if new cost are not better, return -1
   * - if new costs are better, record them in to_cost/to_extra and return
   *   the cost-of-the-path which is the overall measure of goodness of the 
   *   path (less is better) and used to order newly discovered locations. */
  int (*get_costs) (const struct tile *from_tile,
                    enum direction8 dir,
                    const struct tile *to_tile,
                    int from_cost, int from_extra,
                    int *to_cost, int *to_extra,
                    const struct pf_parameter *param);

  /* User provided data. Can be used to attach arbitrary information
   * to the map. */
  void *data;
};

/* The map itself.  Opaque type. */
struct pf_map;

/* The city map strucure.  Opaque type. */
struct pf_city_map;



/* ======================== Public Interface =========================== */

/* Returns a map which can be used to query for paths or to iterate
 * over all paths. Does not perform any computations itself, just sets
 * everything up. */
struct pf_map *pf_map_new(const struct pf_parameter *parameter);

/* After usage the map should be destroyed. */
void pf_map_destroy(struct pf_map *pfm);

/* Tries to find the best path in the given map to the position ptile.
 * If NULL is returned no path could be found.  The pf_path_get_last_position
 * of such path would be the same (almost) as the result of the call to 
 * pf_map_get_position(pfm, ptile, &pos) */
struct pf_path *pf_map_get_path(struct pf_map *pfm, struct tile *ptile);

/* Iterates the map until it reaches ptile.  Then fills the info
 * about it into pos.  Returns FALSE if position is unreachable.
 * Contents of pos in this case is not defined. */
bool pf_map_get_position(struct pf_map *pfm, struct tile *ptile,
			 struct pf_position *pos);

/* Iterates the path-finding algorithm one step further, to the next 
 * nearest position.  This full info on this position and the best path to 
 * it can be obtained using pf_map_iterator_get_position and
 * pf_map_iteratir_get_path, correspondingly.  Returns FALSE if no
 * further positions are available in this map.  If pf_map_get_path(pfm, ptile)
 * or pf_map_get_position(pfm, ptile, &pos) has been called before the call
 * to pf_map_iterate(pfm), the iteration will resume from ptile. */
bool pf_map_iterate(struct pf_map *pfm);

/* Return the map iterator tile. */
struct tile *pf_map_iterator_get_tile(struct pf_map *pfm);

/* Return the move cost at the map iterator tile. */
int pf_map_iterator_get_move_cost(struct pf_map *pfm);

/* Return the map iterator position.  This is equivalent to
 * pf_map_get_position(pfm, pf_map_iterator_get_tile(pfm), &pos). */
void pf_map_iterator_get_position(struct pf_map *pfm,
				  struct pf_position *pos);

/* Return the map iterator path.  This is equivalent to
 * pf_map_get_path(pfm, pf_map_iterator_get_tile(pfm)). */
struct pf_path *pf_map_iterator_get_path(struct pf_map *pfm);

/* Return the current parameters for the given map. */
const struct pf_parameter *pf_map_get_parameter(const struct pf_map *pfm);



/* Print the path via freelog and the given log-level. For
 * debugging purposes. Make sure the path is valid (if you
 * got it from pf_map_get_path()). */
void pf_path_print(const struct pf_path *path, int log_level);

/* After use, a path must be destroyed. pf_destroy_path will also
 * accept NULL (which is returned by pf_map_get_path in error case). */
void pf_path_destroy(struct pf_path *path);

/* Returns the last position of the given path. */
const struct pf_position *pf_path_get_last_position(const struct pf_path *path);



/* Create a city cost map.  It is used to calculate the cost units may
 * need to reach the city. */
struct pf_city_map *pf_city_map_new(const struct city *pcity);

/* Destroy a city cost map. */
void pf_city_map_destroy(struct pf_city_map *pfcm);

/* Get the move cost needed by the unit to reach the city. */
int pf_city_map_get_move_cost(struct pf_city_map *pfcm,
                              const struct unit_type *punittype,
                              struct tile *ptile);



/*
 * This macro iterates all reachable tiles.
 *
 * - pfm: A pf_map structure pointer.
 * - tile_iter: The name of the iterator to use (type struct tile *).
 * - from_start_tile: A boolean value which indicate if the start_tile
 * should be iterated or not.
 */
#define pf_map_iterate_tiles(pfm, tile_iter, from_start_tile)\
if ((from_start_tile) || pf_map_iterate((pfm))) {\
  struct pf_map *__pf_map = (pfm);\
  struct tile *tile_iter;\
  do {\
    tile_iter = pf_map_iterator_get_tile(__pf_map);
#define pf_map_iterate_tiles_end\
  } while (pf_map_iterate(__pf_map));\
}

/*
 * This macro iterates all reachable tiles and their move costs.
 *
 * - pfm: A pf_map structure pointer.
 * - tile_iter: The name of the iterator to use (type struct tile *).
 * - move_cost: The name of the variable containing the move cost info.
 * - from_start_tile: A boolean value which indicate if the start_tile
 * should be iterated or not.
 */
#define pf_map_iterate_move_costs(pfm, tile_iter, move_cost, from_start_tile)\
if ((from_start_tile) || pf_map_iterate((pfm))) {\
  struct pf_map *__pf_map = (pfm);\
  struct tile *tile_iter;\
  int move_cost;\
  do {\
    tile_iter = pf_map_iterator_get_tile(__pf_map);\
    move_cost = pf_map_iterator_get_move_cost(__pf_map);
#define pf_map_iterate_move_costs_end\
  } while (pf_map_iterate(__pf_map));\
}

/*
 * This macro iterates all reachable tiles and fill a pf_position
 * structure as info.
 *
 * - pfm: A pf_map structure pointer.
 * - pos_iter: The name of the iterator to use (type struct pf_position).
 * - from_start_tile: A boolean value which indicate if the start_tile
 * should be iterated or not.
 */
#define pf_map_iterate_positions(pfm, pos_iter, from_start_tile)\
if ((from_start_tile) || pf_map_iterate((pfm))) {\
  struct pf_map *__pf_map = (pfm);\
  struct pf_position pos_iter;\
  do {\
    pf_map_iterator_get_position(__pf_map, &pos_iter);
#define pf_map_iterate_positions_end\
  } while (pf_map_iterate(__pf_map));\
}

/*
 * This macro iterates all possible pathes.
 * NB: you need to free the pathes with pf_path_destroy(path_iter).
 *
 * - pfm: A pf_map structure pointer.
 * - path_iter: The name of the iterator to use (type struct pf_path *).
 * - from_start_tile: A boolean value which indicate if the start_tile
 * should be iterated or not.
 */
#define pf_map_iterate_pathes(pfm, path_iter, from_start_tile)\
if ((from_start_tile) || pf_map_iterate((pfm))) {\
  struct pf_map *__pf_map = (pfm);\
  struct pf_path *path_iter;\
  do {\
    path_iter = pf_map_iterator_get_path(__pf_map);
#define pf_map_iterate_pathes_end\
  } while (pf_map_iterate(__pf_map));\
}

#endif /* FC__PATH_FINDING_H */
