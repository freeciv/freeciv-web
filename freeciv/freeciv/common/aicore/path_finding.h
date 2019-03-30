/***********************************************************************
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* utility */
#include "log.h"        /* enum log_level */

/* common */
#include "map.h"
#include "tile.h"
#include "unit.h"
#include "unittype.h"

/* ========================== Explanations =============================== */

/*
 * Functions in this file help to find a path from A to B.
 *
 * DEFINITIONS:
 *   step: one movement step which brings us from one tile to an
 *   adjacent one
 *
 *   turn: turns spent to reach a tile. Since movement rules involve
 *   randomness, we use different "turn modes" to get an estimate of
 *   this number
 *
 *   moves left: move points left upon reaching a tile.
 *
 *   path: a list of steps which leads from the start to the end
 *
 *   move cost (MC): move cost of a _single_ step. MC is always >= 0.
 *     [The parameter can specify what the MC of a step into the unknown is
 *      to be (this is a constant for each map). This defaults to a
 *      slightly large value meaning unknown tiles are avoided slightly.
 *      It's also possible to use 0 here and use TB or EC to control
 *      movement through unknown tiles, or to use PF_IMPOSSIBLE_MC to
 *      easily avoid unknown tiles.]
 *
 *   extra cost (EC): extra cost of a _single_ tile. EC is always >= 0.
 *   The intended meaning for EC is "how much we want to avoid this tile",
 *   see DISCUSSION below for more.
 *
 *   tile behaviour (TB): the extra information about a tile which
 *   tells us whether we can enter and leave tile as normal (see enum
 *   tile_behavior).
 *
 *   total_MC: (effective) move cost of the whole path.
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
 * shortest paths but about finding the "best" paths. Now, each path has
 * two major characteristics:
 * (1) How long it is (or how much does it cost in terms of time)
 * and
 * (2) How dangerous it is (or how much does it cost in terms of resources).
 *
 * We use MC (and total_MC) to describe (1) and use EC (and total_EC) to
 * describe (2). Of course, when it comes to selecting the "best" path,
 * we need to compromise between taking the shortest road and taking the
 * safest road. To that end, we use the "combined cost", calculated as
 *   total_CC = PF_TURN_FACTOR * total_MC + move_rate * total_EC,
 * where PF_TURN_FACTOR is a large constant. This means that each EC
 * point is equivalent to 1/PF_TURN_FACTOR-th of one turn, or,
 * equivalently, we are willing to spend one more turn on the road to
 * avoid PF_TURN_FACTOR worth of danger. Note that if ECs are kept
 * significantly lower than PF_TURN_FACTOR, the total_EC will only act as a
 * tie-breaker between equally long paths.
 *
 * Note, that the user is expected to ask "So this best path, how long will
 * it take me to walk it?". For that reason we keep our accounts of MCs and
 * ECs separately, as one cannot answer the above question basing on
 * total_CC alone.
 *
 * The above setup allows us to find elegant solutions to rather involved
 * questions. A good example would be designing a road from A to B:
 * ================
 * The future road should be as short as possible, but out of many
 * shortest roads we want to select the one which is fastest to
 * construct. For example:
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
 * paths/tiles we want to consider. For example, a trireme might want to
 * never enter deep sea. A chariot, would like to find paths going to
 * enemy cities but not going _through_ them. This can be achieved
 * through an additional tile_behaviour callback, which would return
 * TB_IGNORE for tiles we don't want to visit and TB_DONT_LEAVE for tiles
 * we won't be able to leave (at least alive).
 *
 * Dangerous tiles are those on which we don't want to end a turn. If the
 * danger callback is specified it is used to determine which tiles are
 * dangerous; no path that ends a turn on such a tile will ever be
 * considered.
 *
 * There is also support for fuel, and thus indirectly for air units. If
 * the fuel parameters are provided then the unit is considered to have
 * that much fuel. The net effect is that if a unit has N fuel then only
 * every Nth turn will be considered a stopping point. To support air
 * units, then, all tiles that don't have airfields (or cities/carriers)
 * should be returned as dangerous (see danger, above). Setting fuel == 1
 * in the pf_parameter disables fuel support.
 *
 * There are few other options in the path-finding. For further info about
 * them, see comment for the pf_parameter structure below.
 *
 *
 * FORMULAE:
 *   For calculating total_MC (given particular tile_behaviour)
 *     total_MC = ((turn + 1) * move_rate - moves_left)
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
 * In the first case we use pf_map_move_cost(), pf_map_path(), and
 * pf_map_position() (the latter will only tell us the distance but not the
 * path to take). In the second case we use pf_map_iterate() to iterate
 * through all tiles in the order of increased distance and break when we
 * found what we were looking for. In this case we use pf_map_iter_*() to
 * get information about the "closest tile".
 *
 * A) the caller knows the map position of the goal and wants to know the
 * path:
 *
 *    struct pf_parameter parameter;
 *    struct pf_map *pfm;
 *    struct pf_path *path;
 *
 *    // fill parameter (see below)
 *
 *    // create the map: the main path-finding object.
 *    pfm = pf_map_new(&parameter);
 *
 *    // create the path to the tile 'ptile'.
 *    if ((path = pf_map_path(pfm, ptile))) {
 *      // success, use path
 *      pf_path_destroy(path);
 *    } else {
 *      // no path could be found
 *    }
 *
 *    // don't forget to destroy the map after usage.
 *    pf_map_destroy(pfm);
 *
 * You may call pf_map_path() multiple times with the same pfm.
 *
 * B) the caller doesn't know the map position of the goal yet (but knows
 * what he is looking for, e.g. a port) and wants to iterate over
 * all paths in order of increasing costs (total_CC):
 *
 *    struct pf_parameter parameter;
 *    struct pf_map *pfm;
 *
 *    // fill parameter (see below)
 *
 *    // create the map: the main path-finding object.
 *    pfm = pf_map_new(&parameter);
 *
 *    // iterate the map, using macros defined on this header.
 *    pf_map_*_iterate(pfm, something, TRUE) {
 *      // do something
 *    } pf_map_*_iterate_end;
 *
 *    // don't forget to destroy the map after usage.
 *    pf_map_destroy(pfm);
 *
 * Depending on the kind of information required, the iteration macro
 * should be:
 *
 *  1) tile iteration:
 *     pf_map_tiles_iterate(pfm, ptile, TRUE) {
 *       // do something
 *     } pf_map_tiles_iterate_end;
 *
 *  2) move cost iteration on the nearest position:
 *     pf_map_move_costs_iterate(pfm, ptile, move_cost, TRUE) {
 *       // do something
 *     } pf_map_move_costs_iterate_end;
 *
 *  3) information (for example the total MC and the number of turns) on the
 *  next nearest position:
 *     pf_map_positions_iterate(pfm, pos, TRUE) {
 *       // do something
 *     } pf_map_positions_iterate_end;
 *
 *  4) information about the whole path (leading to the next nearest
 *  position):
 *     pf_map_paths_iterate(pfm, path, TRUE) {
 *       // do something
 *       pf_path_destroy(path);
 *     } pf_map_paths_iterate_end;
 *
 * The third argument passed to the iteration macros is a condition that
 * controls if the start tile of the pf_parameter should iterated or not.
 *
 *
 * FILLING the struct pf_parameter:
 * This can either be done by hand or using the pft_* functions from
 * "common/aicore/pf_tools.h" or a mix of these.
 *
 * Hints:
 * 1. It is useful to limit the expansion of unknown tiles with a get_TB
 * callback.  In this case you might set the unknown_MC to be 0.
 * 2. If there are two paths of the same cost to a tile, you are
 * not guaranteed to get the one with the least steps in it. If you care,
 * specifying EC to be 1 will do the job.
 * 3. To prevent AI from thinking that it can pass through "chokepoints"
 * controlled by enemy cities, you can specify tile behaviour of each
 * occupied enemy city to be TB_DONT_LEAVE.
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

/* =========================== Structures ================================ */

/* Specifies the type of the action. */
enum pf_action {
  PF_ACTION_NONE = 0,
  PF_ACTION_ATTACK,
  PF_ACTION_DIPLOMAT,
  PF_ACTION_TRADE_ROUTE,
  PF_ACTION_IMPOSSIBLE = -1
};

/* Specifies the actions to consider. */
enum pf_action_account {
  PF_AA_NONE = 0,
  PF_AA_UNIT_ATTACK = 1 << 0,
  /* When this option is turned on, the move to a city will be considered
   * as attacking a unit, even if we ignore if there is a unit in the city.
   * This does not mean we are considering taking over a city! */
  PF_AA_CITY_ATTACK = 1 << 1,
  PF_AA_DIPLOMAT = 1 << 2,
  PF_AA_TRADE_ROUTE = 1 << 3
};

/* Specifies the way path-finding will treat a tile. */
enum tile_behavior {
  TB_NORMAL = 0,        /* Well, normal .*/
  TB_IGNORE,            /* This one will be ignored. */
  TB_DONT_LEAVE         /* Paths can lead _to_ such tile, but are not
                         * allowed to go _through_. This move cost is
                         * always evaluated to a constant single move cost,
                         * prefering straight paths because we don't need
                         * moves left to leave this tile. */
};

/* Specifies the possibility to move from/to a tile. */
enum pf_move_scope {
  PF_MS_NONE = 0,
  PF_MS_NATIVE = 1 << 0,
  PF_MS_CITY = 1 << 1,
  PF_MS_TRANSPORT = 1 << 2
};

/* Full specification of a position and time to reach it. */
struct pf_position {
  struct tile *tile;     /* The tile. */
  int turn;              /* The number of turns to the target. */
  int moves_left;       /* The number of moves left the unit would have when
                         * reaching the tile. */
  int fuel_left;        /* The number of turns of fuel left the unit would
                         * have when reaching the tile. It is always set to
                         * 1 when unit types are not fueled. */

  int total_MC;         /* Total MC to reach this point */
  int total_EC;         /* Total EC to reach this point */

  enum direction8 dir_to_next_pos; /* Used only in 'struct pf_path'. */
  enum direction8 dir_to_here; /* Where did we come from. */
};

/* Full specification of a path. */
struct pf_path {
  int length;                   /* Number of steps in the path */
  struct pf_position *positions;
};

/* Initial data for the path-finding. Normally should use functions
 * from "pf_tools.[ch]" to fill the parameter.
 *
 * All callbacks get the parameter passed to pf_map_new() as the last
 * argument.
 *
 * Examples of callbacks can be found in "pf_tools.c"
 * NB: It should be safe to struct copy pf_parameter. */
struct pf_parameter {
  const struct civ_map *map;
  struct tile *start_tile;      /* Initial position */

  int moves_left_initially;
  int fuel_left_initially;      /* Ignored for non-air units. */
  /* Set if the unit is transported. */
  const struct unit_type *transported_by_initially;
  /* See unit_cargo_depth(). */
  int cargo_depth;
  /* All cargo unit types. */
  bv_unit_types cargo_types;

  int move_rate;                /* Move rate of the virtual unit */
  int fuel;                     /* Should be 1 for units without fuel. */

  const struct unit_type *utype;
  const struct player *owner;

  bool omniscience;             /* Do we care if the tile is visible? */

  /* Callback to get MC of a move from 'from_tile' to 'to_tile' and in the
   * direction 'dir'. Note that the callback can calculate 'to_tile' by
   * itself based on 'from_tile' and 'dir'. Excessive information 'to_tile'
   * is provided to ease the implementation of the callback. */
  int (*get_MC) (const struct tile *from_tile,
                 enum pf_move_scope src_move_scope,
                 const struct tile *to_tile,
                 enum pf_move_scope dst_move_scope,
                 const struct pf_parameter *param);

  /* Callback which determines if we can move from/to 'ptile'. */
  enum pf_move_scope (*get_move_scope) (const struct tile *ptile,
                                        bool *can_disembark,
                                        enum pf_move_scope previous_scope,
                                        const struct pf_parameter *param);
  bool ignore_none_scopes;

  /* Callback which determines the behavior of a tile. If NULL
   * TB_NORMAL is assumed. It can be assumed that the implementation
   * of "path_finding.h" will cache this value. */
  enum tile_behavior (*get_TB) (const struct tile *ptile,
                                enum known_type known,
                                const struct pf_parameter *param);

  /* Callback which can be used to provide extra costs depending on the
   * tile. Can be NULL. It can be assumed that the implementation of
   * "path_finding.h" will cache this value. */
  int (*get_EC) (const struct tile *ptile, enum known_type known,
                 const struct pf_parameter *param);

  /* Callback which determines whether an action would be performed at
   * 'ptile' instead of moving to it. */
  enum pf_action (*get_action) (const struct tile *ptile,
                                enum known_type known,
                                const struct pf_parameter *param);
  enum pf_action_account actions;

  /* Callback which determines whether the action from 'from_tile' to
   * 'to_tile' is effectively possible. */
  bool (*is_action_possible) (const struct tile *from_tile,
                              enum pf_move_scope src_move_scope,
                              const struct tile *to_tile,
                              enum pf_action action,
                              const struct pf_parameter *param);

  /* Although the rules governing ZoC are universal, the amount of
   * information available at server and client is different. To
   * compensate for it, we might need to supply our own version
   * of "common" is_my_zoc. Also AI might need to partially ignore
   * ZoC for strategic planning purposes (take into account enemy cities
   * but not units for example).
   * If this callback is NULL, ZoC are ignored. */
  bool (*get_zoc) (const struct player *pplayer, const struct tile *ptile,
                   const struct civ_map *zmap);

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

/* The map itself. Opaque type. */
struct pf_map;

/* The reverse map strucure. Opaque type. */
struct pf_reverse_map;



/* ========================= Public Interface ============================ */

/* Create and free. */
struct pf_map *pf_map_new(const struct pf_parameter *parameter)
               fc__warn_unused_result;
void pf_map_destroy(struct pf_map *pfm);

/* Method A) functions. */
int pf_map_move_cost(struct pf_map *pfm, struct tile *ptile);
struct pf_path *pf_map_path(struct pf_map *pfm, struct tile *ptile)
                fc__warn_unused_result;
bool pf_map_position(struct pf_map *pfm, struct tile *ptile,
                     struct pf_position *pos)
                     fc__warn_unused_result;

/* Method B) functions. */
bool pf_map_iterate(struct pf_map *pfm);
struct tile *pf_map_iter(struct pf_map *pfm);
int pf_map_iter_move_cost(struct pf_map *pfm);
struct pf_path *pf_map_iter_path(struct pf_map *pfm)
                fc__warn_unused_result;
void pf_map_iter_position(struct pf_map *pfm, struct pf_position *pos);

/* Other related functions. */
const struct pf_parameter *pf_map_parameter(const struct pf_map *pfm);


/* Paths functions. */
void pf_path_destroy(struct pf_path *path);
struct pf_path *pf_path_concat(struct pf_path *dest_path,
                               const struct pf_path *src_path);
bool pf_path_advance(struct pf_path *path, struct tile *ptile);
bool pf_path_backtrack(struct pf_path *path, struct tile *ptile);
const struct pf_position *pf_path_last_position(const struct pf_path *path);
void pf_path_print_real(const struct pf_path *path, enum log_level level,
                        const char *file, const char *function, int line);
#define pf_path_print(path, level)                                          \
  if (log_do_output_for_level(level)) {                                     \
    pf_path_print_real(path, level, __FILE__, __FUNCTION__, __FC_LINE__);   \
  }


/* Reverse map functions (Costs to go to start tile). */
struct pf_reverse_map *pf_reverse_map_new(const struct player *pplayer,
                                          struct tile *start_tile,
                                          int max_turns, bool omniscient,
                                          const struct civ_map *map)
                       fc__warn_unused_result;
struct pf_reverse_map *pf_reverse_map_new_for_city(const struct city *pcity,
                                                   const struct player *attacker,
                                                   int max_turns, bool omniscient,
                                                   const struct civ_map *map)
                       fc__warn_unused_result;
void pf_reverse_map_destroy(struct pf_reverse_map *prfm);

int pf_reverse_map_utype_move_cost(struct pf_reverse_map *pfrm,
                                   const struct unit_type *punittype,
                                   struct tile *ptile);
int pf_reverse_map_unit_move_cost(struct pf_reverse_map *pfrm,
                                  const struct unit *punit);
bool pf_reverse_map_utype_position(struct pf_reverse_map *pfrm,
                                   const struct unit_type *punittype,
                                   struct tile *ptile,
                                   struct pf_position *pos);
bool pf_reverse_map_unit_position(struct pf_reverse_map *pfrm,
                                  const struct unit *punit,
                                  struct pf_position *pos);



/* This macro iterates all reachable tiles.
 *
 * ARG_pfm - A pf_map structure pointer.
 * NAME_tile - The name of the iterator to use (type struct tile *). This
 *             is defined inside the macro.
 * COND_from_start - A boolean value (or equivalent, it can be a function)
 *                   which indicate if the start tile should be iterated or
 *                   not. */
#define pf_map_tiles_iterate(ARG_pfm, NAME_tile, COND_from_start)           \
if (COND_from_start || pf_map_iterate((ARG_pfm))) {                         \
  struct pf_map *_MY_pf_map_ = (ARG_pfm);                                   \
  struct tile *NAME_tile;                                                   \
  do {                                                                      \
    NAME_tile = pf_map_iter(_MY_pf_map_);

#define pf_map_tiles_iterate_end                                            \
  } while (pf_map_iterate(_MY_pf_map_));                                    \
}

/* This macro iterates all reachable tiles and their move costs.
 *
 * ARG_pfm - A pf_map structure pointer.
 * NAME_tile - The name of the iterator to use (type struct tile *). This
 *             is defined inside the macro.
 * NAME_cost - The name of the variable containing the move cost info (type
 *             integer).  This is defined inside the macro.
 * COND_from_start - A boolean value (or equivalent, it can be a function)
 *                   which indicate if the start tile should be iterated or
 *                   not. */
#define pf_map_move_costs_iterate(ARG_pfm, NAME_tile, NAME_cost,            \
                                  COND_from_start)                          \
if (COND_from_start || pf_map_iterate((ARG_pfm))) {                         \
  struct pf_map *_MY_pf_map_ = (ARG_pfm);                                   \
  struct tile *NAME_tile;                                                   \
  int NAME_cost;                                                            \
  do {                                                                      \
    NAME_tile = pf_map_iter(_MY_pf_map_);                                   \
    NAME_cost = pf_map_iter_move_cost(_MY_pf_map_);

#define pf_map_move_costs_iterate_end                                       \
  } while (pf_map_iterate(_MY_pf_map_));                                    \
}

/* This macro iterates all reachable tiles and fill a pf_position structure.
 * structure as info.
 *
 * ARG_pfm - A pf_map structure pointer.
 * NAME_pos - The name of the iterator to use (type struct pf_position).
 *            This is defined inside the macro.
 * COND_from_start - A boolean value (or equivalent, it can be a function)
 *                   which indicate if the start tile should be iterated or
 *                   not. */
#define pf_map_positions_iterate(ARG_pfm, NAME_pos, COND_from_start)        \
if (COND_from_start || pf_map_iterate((ARG_pfm))) {                         \
  struct pf_map *_MY_pf_map_ = (ARG_pfm);                                   \
  struct pf_position NAME_pos;                                              \
  do {                                                                      \
    pf_map_iter_position(_MY_pf_map_, &NAME_pos);

#define pf_map_positions_iterate_end                                        \
  } while (pf_map_iterate(_MY_pf_map_));                                    \
}

/* This macro iterates all possible pathes.
 * NB: you need to free the pathes with pf_path_destroy(path_iter).
 *
 * ARG_pfm - A pf_map structure pointer.
 * NAME_path - The name of the iterator to use (type struct pf_path *). This
 *             is defined inside the macro.
 * COND_from_start - A boolean value (or equivalent, it can be a function)
 *                   which indicate if the start tile should be iterated or
 *                   not. */
#define pf_map_paths_iterate(ARG_pfm, NAME_path, COND_from_start)           \
if (COND_from_start || pf_map_iterate((ARG_pfm))) {                         \
  struct pf_map *_MY_pf_map_ = (ARG_pfm);                                   \
  struct pf_path *NAME_path;\
  do {\
    NAME_path = pf_map_iter_path(_MY_pf_map_);

#define pf_map_paths_iterate_end                                            \
  } while (pf_map_iterate(_MY_pf_map_));                                    \
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__PATH_FINDING_H */
