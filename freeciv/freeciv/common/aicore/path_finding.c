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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "bitvector.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "game.h"
#include "map.h"
#include "movement.h"

/* common/aicore */
#include "pf_tools.h"

#include "path_finding.h"

/* For explanations on how to use this module, see "path_finding.h". */

#define SPECPQ_TAG map_index
#define SPECPQ_DATA_TYPE int
#define SPECPQ_PRIORITY_TYPE int
#include "specpq.h"
#define INITIAL_QUEUE_SIZE 100

#ifdef FREECIV_DEBUG
/* Extra checks for debugging. */
#define PF_DEBUG
#endif

/* ======================== Internal structures ========================== */

#ifdef PF_DEBUG
/* The mode we use the pf_map. Used for cast converion checks. */
enum pf_mode {
  PF_NORMAL = 1,        /* Usual goto */
  PF_DANGER,            /* Goto with dangerous positions */
  PF_FUEL               /* Goto for fueled units */
};
#endif /* PF_DEBUG */

enum pf_node_status {
  NS_UNINIT = 0,        /* memory is calloced, hence zero means
                         * uninitialised. */
  NS_INIT,              /* node initialized, but we didn't search a route
                         * yet. */
  NS_NEW,               /* the optimal route isn't found yet. */
  NS_WAITING,           /* the optimal route is found, considering waiting.
                         * Only used for pf_danger_map and pf_fuel_map, it
                         * means we are waiting on a safe place for full
                         * moves before crossing a dangerous area. */
  NS_PROCESSED          /* the optimal route is found. */
};

enum pf_zoc_type {
  ZOC_MINE = 0,         /* My ZoC. */
  ZOC_ALLIED,           /* Allied ZoC. */
  ZOC_NO                /* No ZoC. */
};

/* Abstract base class for pf_normal_map, pf_danger_map, and pf_fuel_map. */
struct pf_map {
#ifdef PF_DEBUG
  enum pf_mode mode;    /* The mode of the map, for conversion checking. */
#endif /* PF_DEBUG */

  /* "Virtual" function table. */
  void (*destroy) (struct pf_map *pfm); /* Destructor. */
  int (*get_move_cost) (struct pf_map *pfm, struct tile *ptile);
  struct pf_path * (*get_path) (struct pf_map *pfm, struct tile *ptile);
  bool (*get_position) (struct pf_map *pfm, struct tile *ptile,
                        struct pf_position *pos);
  bool (*iterate) (struct pf_map *pfm);

  /* Private data. */
  struct tile *tile;          /* The current position (aka iterator). */
  struct pf_parameter params; /* Initial parameters. */
};

/* Down-cast macro. */
#define PF_MAP(pfm) ((struct pf_map *) (pfm))

/* ========================== Common functions =========================== */

/************************************************************************//**
  Return the number of "moves" started with.

  This is different from the moves_left_initially because of fuel. For
  units with fuel > 1 all moves on the same tank of fuel are considered to
  be one turn. Thus the rest of the PF code doesn't actually know that the
  unit has fuel, it just thinks it has that many more MP.
****************************************************************************/
static inline int pf_moves_left_initially(const struct pf_parameter *param)
{
  return (param->moves_left_initially
          + (param->fuel_left_initially - 1) * param->move_rate);
}

/************************************************************************//**
  Return the "move rate".

  This is different from the parameter's move_rate because of fuel. For
  units with fuel > 1 all moves on the same tank of fuel are considered
  to be one turn. Thus the rest of the PF code doesn't actually know that
  the unit has fuel, it just thinks it has that many more MP.
****************************************************************************/
static inline int pf_move_rate(const struct pf_parameter *param)
{
  return param->move_rate * param->fuel;
}

/************************************************************************//**
  Number of turns required to reach node. See comment in the body of
  pf_normal_map_new(), pf_danger_map_new(), and pf_fuel_map_new() functions
  about the usage of moves_left_initially().
****************************************************************************/
static inline int pf_turns(const struct pf_parameter *param, int cost)
{
  /* Negative cost can happen when a unit initially has more MP than its
   * move-rate (due to wonders transfer etc). Although this may be a bug,
   * we'd better be ready.
   *
   * Note that cost == 0 corresponds to the current turn with full MP. */
  if (param->fuel_left_initially < param->fuel) {
    cost -= (param->fuel - param->fuel_left_initially) * param->move_rate;
  }
  if (cost <= 0) {
    return 0;
  } else if (param->move_rate <= 0) {
    return FC_INFINITY; /* This unit cannot move by itself. */
  } else {
    return cost / param->move_rate;
  }
}

/************************************************************************//**
  Moves left after node is reached.
****************************************************************************/
static inline int pf_moves_left(const struct pf_parameter *param, int cost)
{
  int move_rate = pf_move_rate(param);

  /* Cost may be negative; see pf_turns(). */
  if (cost <= 0) {
    return move_rate - cost;
  } else if (move_rate <= 0) {
    return 0; /* This unit never have moves left. */
  } else {
    return move_rate - (cost % move_rate);
  }
}

/************************************************************************//**
  Obtain cost-of-path from pure cost and extra cost
****************************************************************************/
static inline int pf_total_CC(const struct pf_parameter *param,
                              int cost, int extra)
{
  return PF_TURN_FACTOR * cost + extra * pf_move_rate(param);
}

/************************************************************************//**
  Take a position previously filled out (as by fill_position) and "finalize"
  it by reversing all fuel multipliers.

  See pf_moves_left_initially() and pf_move_rate().
****************************************************************************/
static inline void pf_finalize_position(const struct pf_parameter *param,
                                        struct pf_position *pos)
{
  int move_rate = param->move_rate;

  if (0 < move_rate) {
    pos->moves_left %= move_rate;
  }
}

static struct pf_path *
pf_path_new_to_start_tile(const struct pf_parameter *param);
static void pf_position_fill_start_tile(struct pf_position *pos,
                                        const struct pf_parameter *param);


/* ================ Specific pf_normal_* mode structures ================= */

/* Normal path-finding maps are used for most of units with standard rules.
 * See what units make pf_map_new() to pick danger or fuel maps instead. */

/* Node definition. Note we try to have the smallest data as possible. */
struct pf_normal_node {
  signed short cost;    /* total_MC. 'cost' may be negative, see comment in
                         * pf_turns(). */
  unsigned extra_cost;  /* total_EC. Can be huge, (higher than 'cost'). */
  unsigned dir_to_here : 4; /* Direction from which we came. It's
                             * an 'enum direction8' including
                             * possibility of direction8_invalid (so we need
                             * 4 bits) */
  unsigned status : 3;  /* 'enum pf_node_status' really. */

  /* Cached values */
  unsigned move_scope : 3;      /* 'enum pf_move_scope really. */
  unsigned action : 2;          /* 'enum pf_action really. */
  unsigned node_known_type : 2; /* 'enum known_type' really. */
  unsigned behavior : 2;        /* 'enum tile_behavior' really. */
  unsigned zoc_number : 2;      /* 'enum pf_zoc_type' really. */
  unsigned short extra_tile;    /* EC */
};

/* Derived structure of struct pf_map. */
struct pf_normal_map {
  struct pf_map base_map;   /* Base structure, must be the first! */

  struct map_index_pq *queue; /* Queue of nodes we have reached but not
                               * processed yet (NS_NEW), sorted by their
                               * total_CC. */
  struct pf_normal_node *lattice; /* Lattice of nodes. */
};

/* Up-cast macro. */
#ifdef PF_DEBUG
static inline struct pf_normal_map *
pf_normal_map_check(struct pf_map *pfm, const char *file, 
                    const char *function, int line)
{
  fc_assert_full(file, function, line,
                 NULL != pfm && PF_NORMAL == pfm->mode,
                 return NULL, "Wrong pf_map to pf_normal_map conversion.");
  return (struct pf_normal_map *) pfm;
}
#define PF_NORMAL_MAP(pfm)                                                  \
  pf_normal_map_check(pfm, __FILE__, __FUNCTION__, __FC_LINE__)
#else
#define PF_NORMAL_MAP(pfm) ((struct pf_normal_map *) (pfm))
#endif /* PF_DEBUG */

/* ================  Specific pf_normal_* mode functions ================= */

/************************************************************************//**
  Calculates cached values of the target node. Set the node status to
  NS_INIT to avoid recalculating all values. Returns FALSE if we cannot
  enter node (in this case, most of the cached values are not set).
****************************************************************************/
static inline bool pf_normal_node_init(struct pf_normal_map *pfnm,
                                       struct pf_normal_node *node,
                                       struct tile *ptile,
                                       enum pf_move_scope previous_scope)
{
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfnm));
  enum known_type node_known_type;
  enum pf_action action;

#ifdef PF_DEBUG
  fc_assert(NS_UNINIT == node->status);
  /* Else, not a critical problem, but waste of time. */
#endif

  node->status = NS_INIT;

  /* Establish the "known" status of node. */
  if (params->omniscience) {
    node_known_type = TILE_KNOWN_SEEN;
  } else {
    node_known_type = tile_get_known(ptile, params->owner);
  }
  node->node_known_type = node_known_type;

  /* Establish the tile behavior. */
  if (NULL != params->get_TB) {
    node->behavior = params->get_TB(ptile, node_known_type, params);
    if (TB_IGNORE == node->behavior && params->start_tile != ptile) {
      return FALSE;
    }
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* The default. */
    node->behavior = TB_NORMAL;
#endif
  }

  if (TILE_UNKNOWN != node_known_type) {
    bool can_disembark;

    /* Test if we can invade tile. */
    if (!utype_has_flag(params->utype, UTYF_CIVILIAN)
        && !player_can_invade_tile(params->owner, ptile)) {
      /* Maybe overwrite node behavior. */
      if (params->start_tile != ptile) {
        node->behavior = TB_IGNORE;
        return FALSE;
      } else if (TB_NORMAL == node->behavior) {
        node->behavior = TB_IGNORE;
      }
    }

    /* Test the possibility to perform an action. */
    if (NULL != params->get_action) {
      action = params->get_action(ptile, node_known_type, params);
      if (PF_ACTION_IMPOSSIBLE == action) {
        /* Maybe overwrite node behavior. */
        if (params->start_tile != ptile) {
          node->behavior = TB_IGNORE;
          return FALSE;
        } else if (TB_NORMAL == node->behavior) {
          node->behavior = TB_IGNORE;
        }
        action = PF_ACTION_NONE;
      } else if (PF_ACTION_NONE != action
                 && TB_DONT_LEAVE != node->behavior) {
        /* Overwrite node behavior. */
        node->behavior = TB_DONT_LEAVE;
      }
      node->action = action;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    } else {
      /* Nodes are allocated by fc_calloc(), so should be already set to
       * 0. */
      node->action = PF_ACTION_NONE;
#endif
    }

    /* Test the possibility to move from/to 'ptile'. */
    node->move_scope = params->get_move_scope(ptile, &can_disembark,
                                              previous_scope, params);
    if (PF_MS_NONE == node->move_scope
        && PF_ACTION_NONE == node->action
        && params->ignore_none_scopes) {
      /* Maybe overwrite node behavior. */
      if (params->start_tile != ptile) {
        node->behavior = TB_IGNORE;
        return FALSE;
      } else if (TB_NORMAL == node->behavior) {
        node->behavior = TB_IGNORE;
      }
    } else if (PF_MS_TRANSPORT == node->move_scope
               && !can_disembark
               && (params->start_tile != ptile
                   || NULL == params->transported_by_initially)) {
      /* Overwrite node behavior. */
      node->behavior = TB_DONT_LEAVE;
    }

    /* ZOC_MINE means can move unrestricted from/into it, ZOC_ALLIED means
     * can move unrestricted into it, but not necessarily from it. */
    if (NULL != params->get_zoc
        && NULL == tile_city(ptile)
        && !terrain_has_flag(tile_terrain(ptile), TER_NO_ZOC)
        && !params->get_zoc(params->owner, ptile, params->map)) {
      node->zoc_number = (0 < unit_list_size(ptile->units)
                          ? ZOC_ALLIED : ZOC_NO);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    } else {
      /* Nodes are allocated by fc_calloc(), so should be already set to
       * 0. */
      node->zoc_number = ZOC_MINE;
#endif
    }
  } else {
    node->move_scope = PF_MS_NATIVE;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    /* Nodes are allocated by fc_calloc(), so  should be already set to 0. */
    node->action = PF_ACTION_NONE;
    node->zoc_number = ZOC_MINE;
#endif
  }

  /* Evaluate the extra cost of the destination */
  if (NULL != params->get_EC) {
    node->extra_tile = params->get_EC(ptile, node_known_type, params);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so  should be already set to 0. */
    node->extra_tile = 0;
#endif
  }

  return TRUE;
}

/************************************************************************//**
  Fill in the position which must be discovered already. A helper
  for pf_normal_map_position(). This also "finalizes" the position.
****************************************************************************/
static void pf_normal_map_fill_position(const struct pf_normal_map *pfnm,
                                        struct tile *ptile,
                                        struct pf_position *pos)
{
  int tindex = tile_index(ptile);
  struct pf_normal_node *node = pfnm->lattice + tindex;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfnm));

#ifdef PF_DEBUG
  fc_assert_ret_msg(NS_PROCESSED == node->status,
                    "Unreached destination (%d, %d).", TILE_XY(ptile));
#endif /* PF_DEBUG */

  pos->tile = ptile;
  pos->total_EC = node->extra_cost;
  pos->total_MC = (node->cost - pf_move_rate(params)
                   + pf_moves_left_initially(params));
  pos->turn = pf_turns(params, node->cost);
  pos->moves_left = pf_moves_left(params, node->cost);
#ifdef PF_DEBUG
  fc_assert(params->fuel == 1);
  fc_assert(params->fuel_left_initially == 1);
#endif /* PF_DEBUG */
  pos->fuel_left = 1;
  pos->dir_to_here = node->dir_to_here;
  pos->dir_to_next_pos = direction8_invalid();   /* This field does not apply. */

  if (node->cost > 0) {
    pf_finalize_position(params, pos);
  }
}

/************************************************************************//**
  Read off the path to the node dest_tile, which must already be discovered.
  A helper for pf_normal_map_path functions.
****************************************************************************/
static struct pf_path *
pf_normal_map_construct_path(const struct pf_normal_map *pfnm,
                             struct tile *dest_tile)
{
  struct pf_normal_node *node = pfnm->lattice + tile_index(dest_tile);
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfnm));
  enum direction8 dir_next = direction8_invalid();
  struct pf_path *path;
  struct tile *ptile;
  int i;

#ifdef PF_DEBUG
  fc_assert_ret_val_msg(NS_PROCESSED == node->status, NULL,
                        "Unreached destination (%d, %d).",
                        TILE_XY(dest_tile));
#endif /* PF_DEBUG */

  ptile = dest_tile;
  path = fc_malloc(sizeof(*path));

  /* 1: Count the number of steps to get here.
   * To do it, backtrack until we hit the starting point */
  for (i = 0; ; i++) {
    if (ptile == params->start_tile) {
      /* Ah-ha, reached the starting point! */
      break;
    }

    ptile = mapstep(params->map, ptile, DIR_REVERSE(node->dir_to_here));
    node = pfnm->lattice + tile_index(ptile);
  }

  /* 2: Allocate the memory */
  path->length = i + 1;
  path->positions = fc_malloc((i + 1) * sizeof(*(path->positions)));

  /* 3: Backtrack again and fill the positions this time */
  ptile = dest_tile;
  node = pfnm->lattice + tile_index(ptile);

  for (; i >= 0; i--) {
    pf_normal_map_fill_position(pfnm, ptile, &path->positions[i]);
    /* fill_position doesn't set direction */
    path->positions[i].dir_to_next_pos = dir_next;

    dir_next = node->dir_to_here;

    if (i > 0) {
      /* Step further back, if we haven't finished yet */
      ptile = mapstep(params->map, ptile, DIR_REVERSE(dir_next));
      node = pfnm->lattice + tile_index(ptile);
    }
  }

  return path;
}

/************************************************************************//**
  Adjust MC to reflect the move_rate.
****************************************************************************/
static int pf_normal_map_adjust_cost(int cost, int moves_left)
{
  fc_assert_ret_val(cost >= 0, PF_IMPOSSIBLE_MC);

  return MIN(cost, moves_left);
}

/************************************************************************//**
  Bare-bones PF iterator. All Freeciv rules logic is hidden in 'get_costs'
  callback (compare to pf_normal_map_iterate function). This function is
  called when the pf_map was created with a 'get_cost' callback.

  Plan: 1. Process previous position.
        2. Get new nearest position and return it.

  During the iteration, the node status will be changed:
  A. NS_UNINIT: The node is not initialized, we didn't reach it at all.
  (NS_INIT not used here)
  B. NS_NEW: We have reached this node, but we are not sure it was the best
     path.
  (NS_WAITING not used here)
  C. NS_PROCESSED: Now, we are sure we have the best path. Then, we won't
     do anything more with this node.
****************************************************************************/
static bool pf_jumbo_map_iterate(struct pf_map *pfm)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);
  struct tile *tile = pfm->tile;
  int tindex = tile_index(tile);
  struct pf_normal_node *node = pfnm->lattice + tindex;
  const struct pf_parameter *params = pf_map_parameter(pfm);

  /* Processing Stage */

  /* The previous position is defined by 'tile' (tile pointer), 'node'
   * (the data of the tile for the pf_map), and index (the index of the
   * position in the Freeciv map). */

  adjc_dir_iterate(params->map, tile, tile1, dir) {
    /* Calculate the cost of every adjacent position and set them in the
     * priority queue for next call to pf_jumbo_map_iterate(). */
    int tindex1 = tile_index(tile1);
    struct pf_normal_node *node1 = pfnm->lattice + tindex1;
    int priority, cost1, extra_cost1;

    /* As for the previous position, 'tile1', 'node1' and 'tindex1' are
     * defining the adjacent position. */

    if (node1->status == NS_PROCESSED) {
      /* This gives 15% speedup */
      continue;
    }

    if (NS_UNINIT == node1->status) {
      /* Set cost as impossible for initializing, params->get_costs(), will
       * overwrite with the right value. */
      cost1 = PF_IMPOSSIBLE_MC;
      extra_cost1 = 0;
    } else {
      cost1 = node1->cost;
      extra_cost1 = node1->extra_cost;
    }

    /* User-supplied callback 'get_costs' takes care of everything (ZOC,
     * known, costs etc). See explanations in "path_finding.h". */
    priority = params->get_costs(tile, dir, tile1, node->cost,
                                 node->extra_cost, &cost1,
                                 &extra_cost1, params);
    if (priority >= 0) {
      /* We found a better route to 'tile1', record it (the costs are
       * recorded already). Node status step A. to B. */
      if (NS_NEW == node1->status) {
        map_index_pq_replace(pfnm->queue, tindex1, -priority);
      } else {
        map_index_pq_insert(pfnm->queue, tindex1, -priority);
      }
      node1->cost = cost1;
      node1->extra_cost = extra_cost1;
      node1->status = NS_NEW;
      node1->dir_to_here = dir;
    }
  } adjc_dir_iterate_end;

  /* Get the next node (the index with the highest priority). */
  if (!map_index_pq_remove(pfnm->queue, &tindex)) {
    /* No more indexes in the priority queue, iteration end. */
    return FALSE;
  }

#ifdef PF_DEBUG
  fc_assert(NS_NEW == pfnm->lattice[tindex].status);
#endif

  /* Change the pf_map iterator. Node status step B. to C. */
  pfm->tile = index_to_tile(params->map, tindex);
  pfnm->lattice[tindex].status = NS_PROCESSED;

  return TRUE;
}

/************************************************************************//**
  Primary method for iterative path-finding.

  Plan: 1. Process previous position.
        2. Get new nearest position and return it.

  During the iteration, the node status will be changed:
  A. NS_UNINIT: The node is not initialized, we didn't reach it at all.
  B. NS_INIT: We have initialized the cached values, however, we failed to
     reach this node.
  C. NS_NEW: We have reached this node, but we are not sure it was the best
     path.
  (NS_WAITING not used here)
  D. NS_PROCESSED: Now, we are sure we have the best path. Then, we won't
     do anything more with this node.
****************************************************************************/
static bool pf_normal_map_iterate(struct pf_map *pfm)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);
  struct tile *tile = pfm->tile;
  int tindex = tile_index(tile);
  struct pf_normal_node *node = pfnm->lattice + tindex;
  const struct pf_parameter *params = pf_map_parameter(pfm);
  int cost_of_path;
  enum pf_move_scope scope = node->move_scope;

  /* There is no exit from DONT_LEAVE tiles! */
  if (node->behavior != TB_DONT_LEAVE
      && scope != PF_MS_NONE
      && (params->move_rate > 0 || node->cost < 0)) {
    /* Processing Stage */

    /* The previous position is defined by 'tile' (tile pointer), 'node'
     * (the data of the tile for the pf_map), and index (the index of the
     * position in the Freeciv map). */

    adjc_dir_iterate(params->map, tile, tile1, dir) {
      /* Calculate the cost of every adjacent position and set them in the
       * priority queue for next call to pf_normal_map_iterate(). */
      int tindex1 = tile_index(tile1);
      struct pf_normal_node *node1 = pfnm->lattice + tindex1;
      int cost;
      int extra = 0;

      /* As for the previous position, 'tile1', 'node1' and 'tindex1' are
       * defining the adjacent position. */

      if (node1->status == NS_PROCESSED) {
        /* This gives 15% speedup. Node status already at step D. */
        continue;
      }

      /* Initialise target tile if necessary. */
      if (node1->status == NS_UNINIT) {
        /* Only initialize once. See comment for pf_normal_node_init().
         * Node status step A. to B. */
        if (!pf_normal_node_init(pfnm, node1, tile1, scope)) {
          continue;
        }
      } else if (TB_IGNORE == node1->behavior) {
        /* We cannot enter this tile at all! */
        continue;
      }

      /* Is the move ZOC-ok? */
      if (node->zoc_number != ZOC_MINE && node1->zoc_number == ZOC_NO) {
        continue;
      }

      /* Evaluate the cost of the move. */
      if (PF_ACTION_NONE != node1->action) {
        if (NULL != params->is_action_possible
            && !params->is_action_possible(tile, scope, tile1, node1->action,
                                           params)) {
          continue;
        }
        /* action move cost depends on action and unit type. */
        if (node1->action == PF_ACTION_ATTACK
            && (utype_has_flag(params->utype, UTYF_ONEATTACK)
                || utype_can_do_action(params->utype,
                                       ACTION_SUICIDE_ATTACK))) {
          /* Assume that the attack will be a suicide attack even if a
           * regular attack may be legal. */
          cost = params->move_rate;
        } else {
          cost = SINGLE_MOVE;
        }
      } else if (node1->node_known_type == TILE_UNKNOWN) {
        cost = params->utype->unknown_move_cost;
      } else {
        cost = params->get_MC(tile, scope, tile1, node1->move_scope, params);
      }
      if (cost == PF_IMPOSSIBLE_MC) {
        continue;
      }
      cost = pf_normal_map_adjust_cost(cost,
                                       pf_moves_left(params, node->cost));
      if (cost == PF_IMPOSSIBLE_MC) {
        continue;
      }

      /* Total cost at tile1. Cost may be negative; see pf_turns(). */
      cost += node->cost;

      /* Evaluate the extra cost if it's relevant */
      if (NULL != params->get_EC) {
        extra = node->extra_cost;
        /* Add the cached value */
        extra += node1->extra_tile;
      }

      /* Update costs. */
      cost_of_path = pf_total_CC(params, cost, extra);

      if (NS_INIT == node1->status) {
        /* We are reaching this node for the first time. */
        node1->status = NS_NEW;
        node1->extra_cost = extra;
        node1->cost = cost;
        node1->dir_to_here = dir;
        /* As we prefer lower costs, let's reverse the cost of the path. */
        map_index_pq_insert(pfnm->queue, tindex1, -cost_of_path);
      } else if (cost_of_path < pf_total_CC(params, node1->cost,
                                            node1->extra_cost)) {
        /* We found a better route to 'tile1'. Let's register 'tindex1' to
         * the priority queue. Node status step B. to C. */
        node1->status = NS_NEW;
        node1->extra_cost = extra;
        node1->cost = cost;
        node1->dir_to_here = dir;
        /* As we prefer lower costs, let's reverse the cost of the path. */
        map_index_pq_replace(pfnm->queue, tindex1, -cost_of_path);
      }
    } adjc_dir_iterate_end;
  }

  /* Get the next node (the index with the highest priority). */
  if (!map_index_pq_remove(pfnm->queue, &tindex)) {
    /* No more indexes in the priority queue, iteration end. */
    return FALSE;
  }

#ifdef PF_DEBUG
  fc_assert(NS_NEW == pfnm->lattice[tindex].status);
#endif

  /* Change the pf_map iterator. Node status step C. to D. */
  pfm->tile = index_to_tile(params->map, tindex);
  pfnm->lattice[tindex].status = NS_PROCESSED;

  return TRUE;
}

/************************************************************************//**
  Iterate the map until 'ptile' is reached.
****************************************************************************/
static inline bool pf_normal_map_iterate_until(struct pf_normal_map *pfnm,
                                               struct tile *ptile)
{
  struct pf_map *pfm = PF_MAP(pfnm);
  struct pf_normal_node *node = pfnm->lattice + tile_index(ptile);

  if (NULL == pf_map_parameter(pfm)->get_costs) {
    /* Start position is handled in every function calling this function. */
    if (NS_UNINIT == node->status) {
      /* Initialize the node, for doing the following tests. */
      if (!pf_normal_node_init(pfnm, node, ptile, PF_MS_NONE)) {
        return FALSE;
      }
    } else if (TB_IGNORE == node->behavior) {
      /* Simpliciation: if we cannot enter this node at all, don't iterate
       * the whole map. */
      return FALSE;
    }
  } /* Else, this is a jumbo map, not dealing with normal nodes. */

  while (NS_PROCESSED != node->status) {
    if (!pf_map_iterate(pfm)) {
      /* All reachable destination have been iterated, 'ptile' is
       * unreachable. */
      return FALSE;
    }
  }

  return TRUE;
}

/************************************************************************//**
  Return the move cost at ptile. If ptile has not been reached
  yet, iterate the map until we reach it or run out of map. This function
  returns PF_IMPOSSIBLE_MC on unreachable positions.
****************************************************************************/
static int pf_normal_map_move_cost(struct pf_map *pfm, struct tile *ptile)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    return 0;
  } else if (pf_normal_map_iterate_until(pfnm, ptile)) {
    return (pfnm->lattice[tile_index(ptile)].cost
            - pf_move_rate(pf_map_parameter(pfm))
            + pf_moves_left_initially(pf_map_parameter(pfm)));
  } else {
    return PF_IMPOSSIBLE_MC;
  }
}

/************************************************************************//**
  Return the path to ptile. If ptile has not been reached yet, iterate the
  map until we reach it or run out of map.
****************************************************************************/
static struct pf_path *pf_normal_map_path(struct pf_map *pfm,
                                          struct tile *ptile)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    return pf_path_new_to_start_tile(pf_map_parameter(pfm));
  } else if (pf_normal_map_iterate_until(pfnm, ptile)) {
    return pf_normal_map_construct_path(pfnm, ptile);
  } else {
    return NULL;
  }
}

/************************************************************************//**
  Get info about position at ptile and put it in pos. If ptile has not been
  reached yet, iterate the map until we reach it. Should _always_ check the
  return value, forthe position might be unreachable.
****************************************************************************/
static bool pf_normal_map_position(struct pf_map *pfm, struct tile *ptile,
                                   struct pf_position *pos)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    pf_position_fill_start_tile(pos, pf_map_parameter(pfm));
    return TRUE;
  } else if (pf_normal_map_iterate_until(pfnm, ptile)) {
    pf_normal_map_fill_position(pfnm, ptile, pos);
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  'pf_normal_map' destructor.
****************************************************************************/
static void pf_normal_map_destroy(struct pf_map *pfm)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  free(pfnm->lattice);
  map_index_pq_destroy(pfnm->queue);
  free(pfnm);
}

/************************************************************************//**
  'pf_normal_map' constructor.
****************************************************************************/
static struct pf_map *pf_normal_map_new(const struct pf_parameter *parameter)
{
  struct pf_normal_map *pfnm;
  struct pf_map *base_map;
  struct pf_parameter *params;
  struct pf_normal_node *node;

  pfnm = fc_malloc(sizeof(*pfnm));
  base_map = &pfnm->base_map;
  params = &base_map->params;
#ifdef PF_DEBUG
  /* Set the mode, used for cast check. */
  base_map->mode = PF_NORMAL;
#endif /* PF_DEBUG */

  /* Allocate the map. */
  pfnm->lattice = fc_calloc(MAP_INDEX_SIZE, sizeof(struct pf_normal_node));
  pfnm->queue = map_index_pq_new(INITIAL_QUEUE_SIZE);

  if (NULL == parameter->get_costs) {
    /* 'get_MC' callback must be set. */
    fc_assert_ret_val(NULL != parameter->get_MC, NULL);

    /* 'get_move_scope' callback must be set. */
    fc_assert_ret_val(parameter->get_move_scope != NULL, NULL);
  }

  /* Copy parameters. */
  *params = *parameter;

  /* Initialize virtual function table. */
  base_map->destroy = pf_normal_map_destroy;
  base_map->get_move_cost = pf_normal_map_move_cost;
  base_map->get_path = pf_normal_map_path;
  base_map->get_position = pf_normal_map_position;
  if (NULL != params->get_costs) {
    base_map->iterate = pf_jumbo_map_iterate;
  } else {
    base_map->iterate = pf_normal_map_iterate;
  }

  /* Initialise starting node. */
  node = pfnm->lattice + tile_index(params->start_tile);
  if (NULL == params->get_costs) {
    if (!pf_normal_node_init(pfnm, node, params->start_tile, PF_MS_NONE)) {
      /* Always fails. */
      fc_assert(TRUE == pf_normal_node_init(pfnm, node, params->start_tile,
                                            PF_MS_NONE));
    }

    if (NULL != params->transported_by_initially) {
      /* Overwrite. It is safe because we cannot return to start tile with
       * pf_normal_map. */
      node->move_scope |= PF_MS_TRANSPORT;
      if (!utype_can_freely_unload(params->utype,
                                   params->transported_by_initially)
          && NULL == tile_city(params->start_tile)
          && !tile_has_native_base(params->start_tile,
                                   params->transported_by_initially)) {
        /* Cannot disembark, don't leave transporter. */
        node->behavior = TB_DONT_LEAVE;
      }
    }
  }

  /* Initialise the iterator. */
  base_map->tile = params->start_tile;

  /* This makes calculations of turn/moves_left more convenient, but we
   * need to subtract this value before we return cost to the user. Note
   * that cost may be negative if moves_left_initially > move_rate
   * (see pf_turns()). */
  node->cost = pf_move_rate(params) - pf_moves_left_initially(params);
  node->extra_cost = 0;
  node->dir_to_here = direction8_invalid();
  node->status = NS_PROCESSED;

  return PF_MAP(pfnm);
}


/* ================ Specific pf_danger_* mode structures ================= */

/* Danger path-finding maps are used for units which can cross some areas
 * but not ending their turn there. It used to be used for triremes notably.
 * But since Freeciv 2.2, units with the "Trireme" flag just have
 * restricted moves, then it is not use anymore. */

/* Node definition. Note we try to have the smallest data as possible. */
struct pf_danger_node {
  signed short cost;    /* total_MC. 'cost' may be negative, see comment in
                         * pf_turns(). */
  unsigned extra_cost;  /* total_EC. Can be huge, (higher than 'cost'). */
  unsigned dir_to_here : 4; /* Direction from which we came. It's
                             * an 'enum direction8' including
                             * possibility of direction8_invalid (so we need
                             * 4 bits) */
  unsigned status : 3;  /* 'enum pf_node_status' really. */

  /* Cached values */
  unsigned move_scope : 3;      /* 'enum pf_move_scope really. */
  unsigned action : 2;          /* 'enum pf_action really. */
  unsigned node_known_type : 2; /* 'enum known_type' really. */
  unsigned behavior : 2;        /* 'enum tile_behavior' really. */
  unsigned zoc_number : 2;      /* 'enum pf_zoc_type' really. */
  bool is_dangerous : 1;        /* Whether we cannot end the turn there. */
  bool waited : 1;              /* TRUE if waited to get there. */
  unsigned short extra_tile;    /* EC */

  /* Segment leading across the danger area back to the nearest safe node:
   * need to remeber costs and stuff. */
  struct pf_danger_pos {
    signed short cost;          /* See comment above. */
    unsigned extra_cost;        /* See comment above. */
    signed dir_to_here : 4;     /* See comment above. */
  } *danger_segment;
};

/* Derived structure of struct pf_map. */
struct pf_danger_map {
  struct pf_map base_map;       /* Base structure, must be the first! */

  struct map_index_pq *queue;   /* Queue of nodes we have reached but not
                                 * processed yet (NS_NEW and NS_WAITING),
                                 * sorted by their total_CC. */
  struct map_index_pq *danger_queue; /* Dangerous positions. */
  struct pf_danger_node *lattice; /* Lattice of nodes. */
};

/* Up-cast macro. */
#ifdef PF_DEBUG
static inline struct pf_danger_map *
pf_danger_map_check(struct pf_map *pfm, const char *file,
                    const char *function, int line)
{
  fc_assert_full(file, function, line,
                 NULL != pfm && PF_DANGER == pfm->mode,
                 return NULL, "Wrong pf_map to pf_danger_map conversion.");
  return (struct pf_danger_map *) pfm;
}
#define PF_DANGER_MAP(pfm)                                                  \
  pf_danger_map_check(pfm, __FILE__, __FUNCTION__, __FC_LINE__)
#else
#define PF_DANGER_MAP(pfm) ((struct pf_danger_map *) (pfm))
#endif /* PF_DEBUG */

/* ===============  Specific pf_danger_* mode functions ================== */

/************************************************************************//**
  Calculates cached values of the target node. Set the node status to
  NS_INIT to avoid recalculating all values. Returns FALSE if we cannot
  enter node (in this case, most of the cached values are not set).
****************************************************************************/
static inline bool pf_danger_node_init(struct pf_danger_map *pfdm,
                                       struct pf_danger_node *node,
                                       struct tile *ptile,
                                       enum pf_move_scope previous_scope)
{
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfdm));
  enum known_type node_known_type;
  enum pf_action action;

#ifdef PF_DEBUG
  fc_assert(NS_UNINIT == node->status);
  /* Else, not a critical problem, but waste of time. */
#endif

  node->status = NS_INIT;

  /* Establish the "known" status of node. */
  if (params->omniscience) {
    node_known_type = TILE_KNOWN_SEEN;
  } else {
    node_known_type = tile_get_known(ptile, params->owner);
  }
  node->node_known_type = node_known_type;

  /* Establish the tile behavior. */
  if (NULL != params->get_TB) {
    node->behavior = params->get_TB(ptile, node_known_type, params);
    if (TB_IGNORE == node->behavior && params->start_tile != ptile) {
      return FALSE;
    }
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* The default. */
    node->behavior = TB_NORMAL;
#endif
  }

  if (TILE_UNKNOWN != node_known_type) {
    bool can_disembark;

    /* Test if we can invade tile. */
    if (!utype_has_flag(params->utype, UTYF_CIVILIAN)
        && !player_can_invade_tile(params->owner, ptile)) {
      /* Maybe overwrite node behavior. */
      if (params->start_tile != ptile) {
        node->behavior = TB_IGNORE;
        return FALSE;
      } else if (TB_NORMAL == node->behavior) {
        node->behavior = TB_IGNORE;
      }
    }

    /* Test the possibility to perform an action. */
    if (NULL != params->get_action) {
      action = params->get_action(ptile, node_known_type, params);
      if (PF_ACTION_IMPOSSIBLE == action) {
        /* Maybe overwrite node behavior. */
        if (params->start_tile != ptile) {
          node->behavior = TB_IGNORE;
          return FALSE;
        } else if (TB_NORMAL == node->behavior) {
          node->behavior = TB_IGNORE;
        }
        action = PF_ACTION_NONE;
      } else if (PF_ACTION_NONE != action
                 && TB_DONT_LEAVE != node->behavior) {
        /* Overwrite node behavior. */
        node->behavior = TB_DONT_LEAVE;
      }
      node->action = action;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    } else {
      /* Nodes are allocated by fc_calloc(), so should be already set to
       * 0. */
      node->action = PF_ACTION_NONE;
#endif
    }

    /* Test the possibility to move from/to 'ptile'. */
    node->move_scope = params->get_move_scope(ptile, &can_disembark,
                                              previous_scope, params);
    if (PF_MS_NONE == node->move_scope
        && PF_ACTION_NONE == node->action
        && params->ignore_none_scopes) {
      /* Maybe overwrite node behavior. */
      if (params->start_tile != ptile) {
        node->behavior = TB_IGNORE;
        return FALSE;
      } else if (TB_NORMAL == node->behavior) {
        node->behavior = TB_IGNORE;
      }
    } else if (PF_MS_TRANSPORT == node->move_scope
               && !can_disembark
               && (params->start_tile != ptile
                   || NULL == params->transported_by_initially)) {
      /* Overwrite node behavior. */
      node->behavior = TB_DONT_LEAVE;
    }

    /* ZOC_MINE means can move unrestricted from/into it, ZOC_ALLIED means
     * can move unrestricted into it, but not necessarily from it. */
    if (NULL != params->get_zoc
        && NULL == tile_city(ptile)
        && !terrain_has_flag(tile_terrain(ptile), TER_NO_ZOC)
        && !params->get_zoc(params->owner, ptile, params->map)) {
      node->zoc_number = (0 < unit_list_size(ptile->units)
                          ? ZOC_ALLIED : ZOC_NO);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    } else {
      /* Nodes are allocated by fc_calloc(), so should be already set to
       * 0. */
      node->zoc_number = ZOC_MINE;
#endif
    }
  } else {
    node->move_scope = PF_MS_NATIVE;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    /* Nodes are allocated by fc_calloc(), so  should be already set to 0. */
    node->action = PF_ACTION_NONE;
    node->zoc_number = ZOC_MINE;
#endif
  }

  /* Evaluate the extra cost of the destination. */
  if (NULL != params->get_EC) {
    node->extra_tile = params->get_EC(ptile, node_known_type, params);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->extra_tile = 0;
#endif
  }

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  /* Nodes are allocated by fc_calloc(), so should be already set to
   * FALSE. */
  node->waited = FALSE;
#endif

  node->is_dangerous =
    params->is_pos_dangerous(ptile, node_known_type, params);

  return TRUE;
}

/************************************************************************//**
  Fill in the position which must be discovered already. A helper
  for pf_danger_map_position(). This also "finalizes" the position.
****************************************************************************/
static void pf_danger_map_fill_position(const struct pf_danger_map *pfdm,
                                        struct tile *ptile,
                                        struct pf_position *pos)
{
  int tindex = tile_index(ptile);
  struct pf_danger_node *node = pfdm->lattice + tindex;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfdm));

#ifdef PF_DEBUG
  fc_assert_ret_msg(NS_PROCESSED == node->status
                    || NS_WAITING == node->status,
                    "Unreached destination (%d, %d).", TILE_XY(ptile));
#endif /* PF_DEBUG */

  pos->tile = ptile;
  pos->total_EC = node->extra_cost;
  pos->total_MC = (node->cost - pf_move_rate(params)
                   + pf_moves_left_initially(params));
  pos->turn = pf_turns(params, node->cost);
  pos->moves_left = pf_moves_left(params, node->cost);
#ifdef PF_DEBUG
  fc_assert(params->fuel == 1);
  fc_assert(params->fuel_left_initially == 1);
#endif /* PF_DEBUG */
  pos->fuel_left = 1;
  pos->dir_to_here = node->dir_to_here;
  pos->dir_to_next_pos = direction8_invalid();   /* This field does not apply. */

  if (node->cost > 0) {
    pf_finalize_position(params, pos);
  }
}

/************************************************************************//**
  This function returns the fills the cost needed for a position, to get
  full moves at the next turn. This would be called only when the status is
  NS_WAITING.
****************************************************************************/
static inline int
pf_danger_map_fill_cost_for_full_moves(const struct pf_parameter *param,
                                       int cost)
{
  int moves_left = pf_moves_left(param, cost);

  if (moves_left < pf_move_rate(param)) {
    return cost + moves_left;
  } else {
    return cost;
  }
}

/************************************************************************//**
  Read off the path to the node 'ptile', but with dangers.
  NB: will only find paths to safe tiles!
****************************************************************************/
static struct pf_path *
pf_danger_map_construct_path(const struct pf_danger_map *pfdm,
                             struct tile *ptile)
{
  struct pf_path *path = fc_malloc(sizeof(*path));
  enum direction8 dir_next = direction8_invalid();
  struct pf_danger_pos *danger_seg = NULL;
  bool waited = FALSE;
  struct pf_danger_node *node = pfdm->lattice + tile_index(ptile);
  int length = 1;
  struct tile *iter_tile = ptile;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfdm));
  struct pf_position *pos;
  int i;

#ifdef PF_DEBUG
  fc_assert_ret_val_msg(NS_PROCESSED == node->status
                        || NS_WAITING == node->status, NULL,
                        "Unreached destination (%d, %d).",
                        TILE_XY(ptile));
#endif /* PF_DEBUG */

  /* First iterate to find path length. */
  while (iter_tile != params->start_tile) {
    if (!node->is_dangerous && node->waited) {
      length += 2;
    } else {
      length++;
    }

    if (!node->is_dangerous || !danger_seg) {
      /* We are in the normal node and dir_to_here field is valid */
      dir_next = node->dir_to_here;
      /* d_node->danger_segment is the indicator of what lies ahead
       * if it's non-NULL, we are entering a danger segment,
       * if it's NULL, we are not on one so danger_seg should be NULL. */
      danger_seg = node->danger_segment;
    } else {
      /* We are in a danger segment. */
      dir_next = danger_seg->dir_to_here;
      danger_seg++;
    }

    /* Step backward. */
    iter_tile = mapstep(params->map, iter_tile, DIR_REVERSE(dir_next));
    node = pfdm->lattice + tile_index(iter_tile);
  }

  /* Allocate memory for path. */
  path->positions = fc_malloc(length * sizeof(struct pf_position));
  path->length = length;

  /* Reset variables for main iteration. */
  iter_tile = ptile;
  node = pfdm->lattice + tile_index(ptile);
  danger_seg = NULL;
  waited = FALSE;

  for (i = length - 1; i >= 0; i--) {
    bool old_waited = FALSE;

    /* 1: Deal with waiting. */
    if (!node->is_dangerous) {
      if (waited) {
        /* Waited at _this_ tile, need to record it twice in the
         * path. Here we record our state _after_ waiting (e.g.
         * full move points). */
        pos = path->positions + i;
        pos->tile = iter_tile;
        pos->total_EC = node->extra_cost;
        pos->turn = pf_turns(params,
            pf_danger_map_fill_cost_for_full_moves(params, node->cost));
        pos->moves_left = params->move_rate;
        pos->fuel_left = params->fuel;
        pos->total_MC = ((pos->turn - 1) * params->move_rate
                         + params->moves_left_initially);
        pos->dir_to_next_pos = dir_next;
        pf_finalize_position(params, pos);
        /* Set old_waited so that we record direction8_invalid() as a direction at
         * the step we were going to wait. */
        old_waited = TRUE;
        i--;
      }
      /* Update "waited" (node->waited means "waited to get here"). */
      waited = node->waited;
    }

    /* 2: Fill the current position. */
    pos = path->positions + i;
    pos->tile = iter_tile;
    if (!node->is_dangerous || !danger_seg) {
      pos->total_MC = node->cost;
      pos->total_EC = node->extra_cost;
    } else {
      /* When on dangerous tiles, must have a valid danger segment. */
      fc_assert_ret_val(danger_seg != NULL, NULL);
      pos->total_MC = danger_seg->cost;
      pos->total_EC = danger_seg->extra_cost;
    }
    pos->turn = pf_turns(params, pos->total_MC);
    pos->moves_left = pf_moves_left(params, pos->total_MC);
#ifdef PF_DEBUG
    fc_assert(params->fuel == 1);
    fc_assert(params->fuel_left_initially == 1);
#endif /* PF_DEBUG */
    pos->fuel_left = 1;
    pos->total_MC -= (pf_move_rate(params)
                      - pf_moves_left_initially(params));
    pos->dir_to_next_pos = (old_waited ? direction8_invalid() : dir_next);
    if (node->cost > 0) {
      pf_finalize_position(params, pos);
    }

    /* 3: Check if we finished. */
    if (i == 0) {
      /* We should be back at the start now! */
      fc_assert_ret_val(iter_tile == params->start_tile, NULL);
      return path;
    }

    /* 4: Calculate the next direction. */
    if (!node->is_dangerous || !danger_seg) {
      /* We are in the normal node and dir_to_here field is valid. */
      dir_next = node->dir_to_here;
      /* d_node->danger_segment is the indicator of what lies ahead.
       * If it's non-NULL, we are entering a danger segment,
       * If it's NULL, we are not on one so danger_seg should be NULL. */
      danger_seg = node->danger_segment;
    } else {
      /* We are in a danger segment. */
      dir_next = danger_seg->dir_to_here;
      danger_seg++;
    }

    /* 5: Step further back. */
    iter_tile = mapstep(params->map, iter_tile, DIR_REVERSE(dir_next));
    node = pfdm->lattice + tile_index(iter_tile);
  }

  fc_assert_msg(FALSE, "Cannot get to the starting point!");
  return NULL;
}

/************************************************************************//**
  Creating path segment going back from node1 to a safe tile. We need to
  remember the whole segment because any node can be crossed by many danger
  segments.

  Example: be A, B, C and D points safe positions, E a dangerous one.
    A B
     E 
    C D
  We have found dangerous path from A to D, and later one from C to B:
    A B             A B
     \               / 
    C D             C D
  If we didn't save the segment from A to D when a new segment passing by E
  is found, then finding the path from A to D will produce an error. (The
  path is always done in reverse order.) D->dir_to_here will point to E,
  which is correct, but E->dir_to_here will point to C.
****************************************************************************/
static void pf_danger_map_create_segment(struct pf_danger_map *pfdm,
                                         struct pf_danger_node *node1)
{
  struct tile *ptile = PF_MAP(pfdm)->tile;
  struct pf_danger_node *node = pfdm->lattice + tile_index(ptile);
  struct pf_danger_pos *pos;
  int length = 0, i;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pfdm));

#ifdef PF_DEBUG
  if (NULL != node1->danger_segment) {
    log_error("Possible memory leak in pf_danger_map_create_segment().");
  }
#endif /* PF_DEBUG */

  /* First iteration for determining segment length */
  while (node->is_dangerous && direction8_is_valid(node->dir_to_here)) {
    length++;
    ptile = mapstep(params->map, ptile, DIR_REVERSE(node->dir_to_here));
    node = pfdm->lattice + tile_index(ptile);
  }

  /* Allocate memory for segment */
  node1->danger_segment = fc_malloc(length * sizeof(struct pf_danger_pos));

  /* Reset tile and node pointers for main iteration */
  ptile = PF_MAP(pfdm)->tile;
  node = pfdm->lattice + tile_index(ptile);

  /* Now fill the positions */
  for (i = 0, pos = node1->danger_segment; i < length; i++, pos++) {
    /* Record the direction */
    pos->dir_to_here = node->dir_to_here;
    pos->cost = node->cost;
    pos->extra_cost = node->extra_cost;
    if (i == length - 1) {
      /* The last dangerous node contains "waiting" info */
      node1->waited = node->waited;
    }

    /* Step further down the tree */
    ptile = mapstep(params->map, ptile, DIR_REVERSE(node->dir_to_here));
    node = pfdm->lattice + tile_index(ptile);
  }

#ifdef PF_DEBUG
  /* Make sure we reached a safe node or the start point */
  fc_assert_ret(!node->is_dangerous || !direction8_is_valid(node->dir_to_here));
#endif
}

/************************************************************************//**
  Adjust cost taking into account possibility of making the move.
****************************************************************************/
static inline int
pf_danger_map_adjust_cost(const struct pf_parameter *params,
                          int cost, bool to_danger, int moves_left)
{
  if (cost == PF_IMPOSSIBLE_MC) {
    return PF_IMPOSSIBLE_MC;
  }

  cost = MIN(cost, pf_move_rate(params));

  if (to_danger && cost >= moves_left) {
    /* We would have to end the turn on a dangerous tile! */
    return PF_IMPOSSIBLE_MC;
  } else {
    return MIN(cost, moves_left);
  }
}

/************************************************************************//**
  Primary method for iterative path-finding in presence of danger
  Notes:
  1. Whenever the path-finding stumbles upon a dangerous location, it goes
     into a sub-Dijkstra which processes _only_ dangerous locations, by
     means of a separate queue. When this sub-Dijkstra reaches a safe
     location, it records the segment of the path going across the dangerous
     tiles. Hence danger_segment is an extended (and reversed) version of
     the dir_to_here field (see comment for pf_danger_map_create_segment()).
     It can be re-recorded multiple times as we find shorter and shorter
     routes.
  2. Waiting is realised by inserting the (safe) tile back into the queue
     with a lower priority P. This tile might pop back sooner than P,
     because there might be several copies of it in the queue already. But
     that does not seem to present any problems.
  3. For some purposes, NS_WAITING is just another flavour of NS_PROCESSED,
     since the path to a NS_WAITING tile has already been found.
  4. This algorithm cannot guarantee the best safe segments across dangerous
     region. However it will find a safe segment if there is one. To
     gurantee the best (in terms of total_CC) safe segments across danger,
     supply 'get_EC' which returns small extra on dangerous tiles.

  During the iteration, the node status will be changed:
  A. NS_UNINIT: The node is not initialized, we didn't reach it at all.
  B. NS_INIT: We have initialized the cached values, however, we failed to
     reach this node.
  C. NS_NEW: We have reached this node, but we are not sure it was the best
     path. Dangerous tiles never get upper status.
  D. NS_PROCESSED: Now, we are sure we have the best path.
  E. NS_WAITING: The safe node (never the dangerous ones) is re-inserted in
     the priority queue, as explained above (2.). We need to consider if
     waiting for full moves open or not new possibilities for moving accross
     dangerous areas.
  F. NS_PROCESSED: When finished to consider waiting at the node, revert the
     status to NS_PROCESSED.
  In points D., E., and F., the best path to the node can be considered as
  found (only safe nodes).
****************************************************************************/
static bool pf_danger_map_iterate(struct pf_map *pfm)
{
  struct pf_danger_map *const pfdm = PF_DANGER_MAP(pfm);
  const struct pf_parameter *const params = pf_map_parameter(pfm);
  struct tile *tile = pfm->tile;
  int tindex = tile_index(tile);
  struct pf_danger_node *node = pfdm->lattice + tindex;
  enum pf_move_scope scope = node->move_scope;

  /* The previous position is defined by 'tile' (tile pointer), 'node'
   * (the data of the tile for the pf_map), and index (the index of the
   * position in the Freeciv map). */

  if (!direction8_is_valid(node->dir_to_here)
      && NULL != params->transported_by_initially) {
#ifdef PF_DEBUG
    fc_assert(tile == params->start_tile);
#endif
    scope |= PF_MS_TRANSPORT;
    if (!utype_can_freely_unload(params->utype,
                                 params->transported_by_initially)
        && NULL == tile_city(tile)
        && !tile_has_native_base(tile, params->transported_by_initially)) {
      /* Cannot disembark, don't leave transporter. */
      node->behavior = TB_DONT_LEAVE;
    }
  }

  for (;;) {
    /* There is no exit from DONT_LEAVE tiles! */
    if (node->behavior != TB_DONT_LEAVE
        && scope != PF_MS_NONE
        && (params->move_rate > 0 || node->cost < 0)) {
      /* Cost at tile but taking into account waiting. */
      int loc_cost;
      if (node->status != NS_WAITING) {
        loc_cost = node->cost;
      } else {
        /* We have waited, so we have full moves. */
        loc_cost = pf_danger_map_fill_cost_for_full_moves(params,
                                                          node->cost);
      }

      adjc_dir_iterate(params->map, tile, tile1, dir) {
        /* Calculate the cost of every adjacent position and set them in
         * the priority queues for next call to pf_danger_map_iterate(). */
        int tindex1 = tile_index(tile1);
        struct pf_danger_node *node1 = pfdm->lattice + tindex1;
        int cost;
        int extra = 0;

        /* As for the previous position, 'tile1', 'node1' and 'tindex1' are
         * defining the adjacent position. */

        if (node1->status == NS_PROCESSED || node1->status == NS_WAITING) {
          /* This gives 15% speedup. Node status already at step D., E.
           * or F. */
          continue;
        }

        /* Initialise target tile if necessary. */
        if (node1->status == NS_UNINIT) {
          /* Only initialize once. See comment for pf_danger_node_init().
           * Node status step A. to B. */
          if (!pf_danger_node_init(pfdm, node1, tile1, scope)) {
            continue;
          }
        } else if (TB_IGNORE == node1->behavior) {
          /* We cannot enter this tile at all! */
          continue;
        }

        /* Is the move ZOC-ok? */
        if (node->zoc_number != ZOC_MINE && node1->zoc_number == ZOC_NO) {
          continue;
        }

        /* Evaluate the cost of the move. */
        if (PF_ACTION_NONE != node1->action) {
          if (NULL != params->is_action_possible
              && !params->is_action_possible(tile, scope, tile1,
                                             node1->action, params)) {
            continue;
          }
          /* action move cost depends on action and unit type. */
          if (node1->action == PF_ACTION_ATTACK
              && (utype_has_flag(params->utype, UTYF_ONEATTACK)
                  || utype_can_do_action(params->utype,
                                         ACTION_SUICIDE_ATTACK))) {
            /* Assume that the attack will be a suicide attack even if a
             * regular attack may be legal. */
            cost = params->move_rate;
          } else {
            cost = SINGLE_MOVE;
          }
        } else if (node1->node_known_type == TILE_UNKNOWN) {
          cost = params->utype->unknown_move_cost;
        } else {
          cost = params->get_MC(tile, scope, tile1, node1->move_scope,
                                params);
        }
        if (cost == PF_IMPOSSIBLE_MC) {
          continue;
        }
        cost = pf_danger_map_adjust_cost(params, cost, node1->is_dangerous,
                                         pf_moves_left(params, loc_cost));

        if (cost == PF_IMPOSSIBLE_MC) {
          /* This move is deemed impossible. */
          continue;
        }

        /* Total cost at 'tile1'. */
        cost += loc_cost;

        /* Evaluate the extra cost of the destination, if it's relevant. */
        if (NULL != params->get_EC) {
          extra = node1->extra_tile + node->extra_cost;
        }

        /* Update costs and add to queue, if this is a better route
         * to 'tile1'. */
        if (!node1->is_dangerous) {
          int cost_of_path = pf_total_CC(params, cost, extra);

          if (NS_INIT == node1->status
              || (cost_of_path < pf_total_CC(params, node1->cost,
                                             node1->extra_cost))) {
            /* We are reaching this node for the first time, or we found a
             * better route to 'tile1'. Let's register 'tindex1' to the
             * priority queue. Node status step B. to C. */
            node1->extra_cost = extra;
            node1->cost = cost;
            node1->dir_to_here = dir;
            if (NULL != node1->danger_segment) {
              /* Clear the previously recorded path back. */
              free(node1->danger_segment);
              node1->danger_segment = NULL;
            }
            if (node->is_dangerous) {
              /* We came from a dangerous tile. So we need to record the
               * path we came from until the previous safe position is
               * found. See comment for pf_danger_map_create_segment(). */
              pf_danger_map_create_segment(pfdm, node1);
            } else {
              /* Maybe clear previously "waited" status of the node. */
              node1->waited = FALSE;
            }
            if (NS_INIT == node1->status) {
              node1->status = NS_NEW;
              map_index_pq_insert(pfdm->queue, tindex1, -cost_of_path);
            } else {
#ifdef PF_DEBUG
              fc_assert(NS_NEW == node1->status);
#endif
              map_index_pq_replace(pfdm->queue, tindex1, -cost_of_path);
            }
          }
        } else {
          /* The procedure is slightly different for dangerous nodes.
           * We will update costs if:
           * 1. we are here for the first time;
           * 2. we can possibly go further across dangerous area; or
           * 3. we can have lower extra and will not overwrite anything
           * useful. Node status step B. to C. */
          if (node1->status == NS_INIT) {
            /* case 1. */
            node1->extra_cost = extra;
            node1->cost = cost;
            node1->dir_to_here = dir;
            node1->status = NS_NEW;
            node1->waited = (node->status == NS_WAITING);
            /* Extra costs of all nodes in danger_queue are equal! */
            map_index_pq_insert(pfdm->danger_queue, tindex1, -cost);
          } else if ((pf_moves_left(params, cost)
                      > pf_moves_left(params, node1->cost))
                     || (node1->status == NS_PROCESSED
                         && (pf_total_CC(params, cost, extra)
                             < pf_total_CC(params, node1->cost,
                                           node1->extra_cost)))) {
            /* case 2 or 3. */
            node1->extra_cost = extra;
            node1->cost = cost;
            node1->dir_to_here = dir;
            node1->status = NS_NEW;
            node1->waited = (node->status == NS_WAITING);
            /* Extra costs of all nodes in danger_queue are equal! */
            map_index_pq_replace(pfdm->danger_queue, tindex1, -cost);
          }
        }
      } adjc_dir_iterate_end;
    }

    if (NS_WAITING == node->status) {
      /* Node status final step E. to F. */
#ifdef PF_DEBUG
      fc_assert(!node->is_dangerous);
#endif
      node->status = NS_PROCESSED;
    } else if (!node->is_dangerous
               && (pf_moves_left(params, node->cost)
                   < pf_move_rate(params))) {
      int fc, cc;
      /* Consider waiting at this node. To do it, put it back into queue.
       * Node status final step D. to E. */
      fc = pf_danger_map_fill_cost_for_full_moves(params, node->cost);
      cc = pf_total_CC(params, fc, node->extra_cost);
      node->status = NS_WAITING;
      map_index_pq_insert(pfdm->queue, tindex, -cc);
    }

    /* Get the next node (the index with the highest priority). First try
     * to get it from danger_queue. */
    if (map_index_pq_remove(pfdm->danger_queue, &tindex)) {
      /* Change the pf_map iterator and reset data. */
      tile = index_to_tile(params->map, tindex);
      pfm->tile = tile;
      node = pfdm->lattice + tindex;
    } else {
      /* No dangerous nodes to process, go for a safe one. */
      if (!map_index_pq_remove(pfdm->queue, &tindex)) {
        /* No more indexes in the priority queue, iteration end. */
        return FALSE;
      }

#ifdef PF_DEBUG
      fc_assert(NS_PROCESSED != pfdm->lattice[tindex].status);
#endif

      /* Change the pf_map iterator and reset data. */
      tile = index_to_tile(params->map, tindex);
      pfm->tile = tile;
      node = pfdm->lattice + tindex;
      if (NS_WAITING != node->status) {
        /* Node status step C. and D. */
#ifdef PF_DEBUG
        fc_assert(!node->is_dangerous);
#endif
        node->status = NS_PROCESSED;
        return TRUE;
      }
    }

#ifdef PF_DEBUG
    fc_assert(NS_INIT < node->status);

    if (NS_WAITING == node->status) {
      /* We've already returned this node once, skip it. */
      log_debug("Considering waiting at (%d, %d)", TILE_XY(tile));
    } else if (node->is_dangerous) {
      /* We don't return dangerous tiles. */
      log_debug("Reached dangerous tile (%d, %d)", TILE_XY(tile));
    }
#endif

    scope = node->move_scope;
  }

  log_error("%s(): internal error.", __FUNCTION__);
  return FALSE;
}

/************************************************************************//**
  Iterate the map until 'ptile' is reached.
****************************************************************************/
static inline bool pf_danger_map_iterate_until(struct pf_danger_map *pfdm,
                                               struct tile *ptile)
{
  struct pf_map *pfm = PF_MAP(pfdm);
  struct pf_danger_node *node = pfdm->lattice + tile_index(ptile);

  /* Start position is handled in every function calling this function. */

  if (NS_UNINIT == node->status) {
    /* Initialize the node, for doing the following tests. */
    if (!pf_danger_node_init(pfdm, node, ptile, PF_MS_NONE)
        || node->is_dangerous) {
      return FALSE;
    }
  } else if (TB_IGNORE == node->behavior || node->is_dangerous) {
    /* Simpliciation: if we cannot enter this node at all, or we cannot
     * stay at this position, don't iterate the whole map. */
    return FALSE;
  }

  while (NS_PROCESSED != node->status && NS_WAITING != node->status) {
    if (!pf_map_iterate(pfm)) {
      /* All reachable destination have been iterated, 'ptile' is
       * unreachable. */
      return FALSE;
    }
  }

  return TRUE;
}

/************************************************************************//**
  Return the move cost at ptile. If ptile has not been reached yet, iterate
  the map until we reach it or run out of map. This function returns
  PF_IMPOSSIBLE_MC on unreachable positions.
****************************************************************************/
static int pf_danger_map_move_cost(struct pf_map *pfm, struct tile *ptile)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    return 0;
  } else if (pf_danger_map_iterate_until(pfdm, ptile)) {
    return (pfdm->lattice[tile_index(ptile)].cost
            - pf_move_rate(pf_map_parameter(pfm))
            + pf_moves_left_initially(pf_map_parameter(pfm)));
  } else {
    return PF_IMPOSSIBLE_MC;
  }
}

/************************************************************************//**
  Return the path to ptile. If ptile has not been reached yet, iterate the
  map until we reach it or run out of map.
****************************************************************************/
static struct pf_path *pf_danger_map_path(struct pf_map *pfm,
                                          struct tile *ptile)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    return pf_path_new_to_start_tile(pf_map_parameter(pfm));
  } else if (pf_danger_map_iterate_until(pfdm, ptile)) {
    return pf_danger_map_construct_path(pfdm, ptile);
  } else {
    return NULL;
  }
}

/************************************************************************//**
  Get info about position at ptile and put it in pos . If ptile has not been
  reached yet, iterate the map until we reach it. Should _always_ check the
  return value, for the position might be unreachable.
****************************************************************************/
static bool pf_danger_map_position(struct pf_map *pfm, struct tile *ptile,
                                   struct pf_position *pos)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    pf_position_fill_start_tile(pos, pf_map_parameter(pfm));
    return TRUE;
  } else if (pf_danger_map_iterate_until(pfdm, ptile)) {
    pf_danger_map_fill_position(pfdm, ptile, pos);
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  'pf_danger_map' destructor.
****************************************************************************/
static void pf_danger_map_destroy(struct pf_map *pfm)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);
  struct pf_danger_node *node;
  int i;

  /* Need to clean up the dangling danger segments. */
  for (i = 0, node = pfdm->lattice; i < MAP_INDEX_SIZE; i++, node++) {
    if (node->danger_segment) {
      free(node->danger_segment);
    }
  }
  free(pfdm->lattice);
  map_index_pq_destroy(pfdm->queue);
  map_index_pq_destroy(pfdm->danger_queue);
  free(pfdm);
}

/************************************************************************//**
  'pf_danger_map' constructor.
****************************************************************************/
static struct pf_map *pf_danger_map_new(const struct pf_parameter *parameter)
{
  struct pf_danger_map *pfdm;
  struct pf_map *base_map;
  struct pf_parameter *params;
  struct pf_danger_node *node;

  pfdm = fc_malloc(sizeof(*pfdm));
  base_map = &pfdm->base_map;
  params = &base_map->params;
#ifdef PF_DEBUG
  /* Set the mode, used for cast check. */
  base_map->mode = PF_DANGER;
#endif /* PF_DEBUG */

  /* Allocate the map. */
  pfdm->lattice = fc_calloc(MAP_INDEX_SIZE, sizeof(struct pf_danger_node));
  pfdm->queue = map_index_pq_new(INITIAL_QUEUE_SIZE);
  pfdm->danger_queue = map_index_pq_new(INITIAL_QUEUE_SIZE);

  /* 'get_MC' callback must be set. */
  fc_assert_ret_val(parameter->get_MC != NULL, NULL);

  /* 'is_pos_dangerous' callback must be set. */
  fc_assert_ret_val(parameter->is_pos_dangerous != NULL, NULL);

  /* 'get_move_scope' callback must be set. */
  fc_assert_ret_val(parameter->get_move_scope != NULL, NULL);

  /* Copy parameters */
  *params = *parameter;

  /* Initialize virtual function table. */
  base_map->destroy = pf_danger_map_destroy;
  base_map->get_move_cost = pf_danger_map_move_cost;
  base_map->get_path = pf_danger_map_path;
  base_map->get_position = pf_danger_map_position;
  base_map->iterate = pf_danger_map_iterate;

  /* Initialise starting node. */
  node = pfdm->lattice + tile_index(params->start_tile);
  if (!pf_danger_node_init(pfdm, node, params->start_tile, PF_MS_NONE)) {
    /* Always fails. */
    fc_assert(TRUE == pf_danger_node_init(pfdm, node, params->start_tile,
                                          PF_MS_NONE));
  }

  /* NB: do not handle params->transported_by_initially because we want to
   * handle only at start, not when crossing over the start tile for a
   * second time. See pf_danger_map_iterate(). */

  /* Initialise the iterator. */
  base_map->tile = params->start_tile;

  /* This makes calculations of turn/moves_left more convenient, but we
   * need to subtract this value before we return cost to the user. Note
   * that cost may be negative if moves_left_initially > move_rate
   * (see pf_turns()). */
  node->cost = pf_move_rate(params) - pf_moves_left_initially(params);
  node->extra_cost = 0;
  node->dir_to_here = direction8_invalid();
  node->status = (node->is_dangerous ? NS_NEW : NS_PROCESSED);

  return PF_MAP(pfdm);
}


/* ================= Specific pf_fuel_* mode structures ================== */

/* Fuel path-finding maps are used for units which need to refuel. Usually
 * for air units such as planes or missiles.
 *
 * A big difference with the danger path-finding maps is that the tiles
 * which are not refuel points are not considered as dangerous because the
 * server uses to move the units at the end of the turn to refuels points. */

struct pf_fuel_pos;

/* Node definition. Note we try to have the smallest data as possible. */
struct pf_fuel_node {
  signed short cost;    /* total_MC. 'cost' may be negative, see comment in
                         * pf_turns(). */
  unsigned extra_cost;  /* total_EC. Can be huge, (higher than 'cost'). */
  unsigned moves_left : 12; /* Moves left at this position. */
  unsigned dir_to_here : 4; /* Direction from which we came. It's
                             * an 'enum direction8' including
                             * possibility of direction8_invalid (so we need
                             * 4 bits) */
  unsigned status : 3;  /* 'enum pf_node_status' really. */

  /* Cached values */
  unsigned move_scope : 3;      /* 'enum pf_move_scope really. */
  unsigned action : 2;          /* 'enum pf_action really. */
  unsigned node_known_type : 2; /* 'enum known_type' really. */
  unsigned behavior : 2;        /* 'enum tile_behavior' really. */
  unsigned zoc_number : 2;      /* 'enum pf_zoc_type' really. */
  signed moves_left_req : 13;   /* The minimum required moves left to reach
                                 * this tile. It the number of moves we need
                                 * to reach the nearest refuel point. A
                                 * value of 0 means this is a refuel point.
                                 * FIXME: this is right only for units with
                                 * constant move costs! */
  unsigned short extra_tile;    /* EC */
  unsigned char cost_to_here[DIR8_MAGIC_MAX]; /* Step cost[dir to here] */

  /* Segment leading across the danger area back to the nearest safe node:
   * need to remeber costs and stuff. */
  struct pf_fuel_pos *pos;
  /* Optimal segment to follow to get there (when node is processed). */
  struct pf_fuel_pos *segment;
};

/* We need to remember how we could get to there (until the previous refuel
 * point, or start position), because we could re-process the nodes after
 * having waiting somewhere. */
struct pf_fuel_pos {
  signed short cost;
  unsigned extra_cost;
  unsigned moves_left : 12;
  signed dir_to_here : 4;
  unsigned ref_count : 4;
  struct pf_fuel_pos *prev;
};

/* Derived structure of struct pf_map. */
struct pf_fuel_map {
  struct pf_map base_map;       /* Base structure, must be the first! */

  struct map_index_pq *queue;   /* Queue of nodes we have reached but not
                                 * processed yet (NS_NEW), sorted by their
                                 * total_CC */
  struct map_index_pq *waited_queue; /* Queue of nodes to reach farer
                                      * positions after having refueled. */
  struct pf_fuel_node *lattice; /* Lattice of nodes */
};

/* Up-cast macro. */
#ifdef PF_DEBUG
static inline struct pf_fuel_map *
pf_fuel_map_check(struct pf_map *pfm, const char *file,
                  const char *function, int line)
{
  fc_assert_full(file, function, line,
                 NULL != pfm && PF_FUEL == pfm->mode,
                 return NULL, "Wrong pf_map to pf_fuel_map conversion.");
  return (struct pf_fuel_map *) pfm;
}
#define PF_FUEL_MAP(pfm) \
  pf_fuel_map_check(pfm, __FILE__, __FUNCTION__, __FC_LINE__)
#else
#define PF_FUEL_MAP(pfm) ((struct pf_fuel_map *) (pfm))
#endif /* PF_DEBUG */

/* =================  Specific pf_fuel_* mode functions ================== */

/************************************************************************//**
  Obtain cost-of-path from pure cost, extra cost and safety.
****************************************************************************/
static inline int pf_fuel_total_CC(const struct pf_parameter *param,
                                   int cost, int extra, int safety)
{
  return pf_total_CC(param, cost, extra) - safety;
}

/************************************************************************//**
  Obtain cost-of-path for constant extra cost (used for node after waited).
****************************************************************************/
static inline int pf_fuel_waited_total_CC(int cost, int safety)
{
  return PF_TURN_FACTOR * (cost + 1) - safety - 1;
}

/************************************************************************//**
  Calculates cached values of the target node. Set the node status to
  NS_INIT to avoid recalculating all values. Returns FALSE if we cannot
  enter node (in this case, most of the cached values are not set).
****************************************************************************/
static inline bool pf_fuel_node_init(struct pf_fuel_map *pffm,
                                     struct pf_fuel_node *node,
                                     struct tile *ptile,
                                     enum pf_move_scope previous_scope)
{
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pffm));
  enum known_type node_known_type;
  enum pf_action action;

#ifdef PF_DEBUG
  fc_assert(NS_UNINIT == node->status);
  /* Else, not a critical problem, but waste of time. */
#endif

  node->status = NS_INIT;

  /* Establish the "known" status of node. */
  if (params->omniscience) {
    node_known_type = TILE_KNOWN_SEEN;
  } else {
    node_known_type = tile_get_known(ptile, params->owner);
  }
  node->node_known_type = node_known_type;

  /* Establish the tile behavior. */
  if (NULL != params->get_TB) {
    node->behavior = params->get_TB(ptile, node_known_type, params);
    if (TB_IGNORE == node->behavior && params->start_tile != ptile) {
      return FALSE;
    }
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* The default. */
    node->behavior = TB_NORMAL;
#endif
  }

  if (TILE_UNKNOWN != node_known_type) {
    bool can_disembark;

    /* Test if we can invade tile. */
    if (!utype_has_flag(params->utype, UTYF_CIVILIAN)
        && !player_can_invade_tile(params->owner, ptile)) {
      /* Maybe overwrite node behavior. */
      if (params->start_tile != ptile) {
        node->behavior = TB_IGNORE;
        return FALSE;
      } else if (TB_NORMAL == node->behavior) {
        node->behavior = TB_IGNORE;
      }
    }

    /* Test the possibility to perform an action. */
    if (NULL != params->get_action
        && PF_ACTION_NONE != (action =
                              params->get_action(ptile, node_known_type,
                                                 params))) {
      if (PF_ACTION_IMPOSSIBLE == action) {
        /* Maybe overwrite node behavior. */
        if (params->start_tile != ptile) {
          node->behavior = TB_IGNORE;
          return FALSE;
        } else if (TB_NORMAL == node->behavior) {
          node->behavior = TB_IGNORE;
        }
        action = PF_ACTION_NONE;
      } else if (TB_DONT_LEAVE != node->behavior) {
        /* Overwrite node behavior. */
        node->behavior = TB_DONT_LEAVE;
      }
      node->action = action;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
      node->moves_left_req = 0; /* Attack is always possible theorically. */
#endif
    } else {
#ifdef ZERO_VARIABLES_FOR_SEARCHING
      /* Nodes are allocated by fc_calloc(), so should be already set to
       * 0. */
      node->action = PF_ACTION_NONE;
#endif
      node->moves_left_req =
        params->get_moves_left_req(ptile, node_known_type, params);
      if (PF_IMPOSSIBLE_MC == node->moves_left_req) {
        /* Overwrite node behavior. */
        if (params->start_tile == ptile) {
          node->behavior = TB_DONT_LEAVE;
        } else {
          node->behavior = TB_IGNORE;
          return FALSE;
        }
      }
    }

    /* Test the possibility to move from/to 'ptile'. */
    node->move_scope = params->get_move_scope(ptile, &can_disembark,
                                              previous_scope, params);
    if (PF_MS_NONE == node->move_scope
        && PF_ACTION_NONE == node->action
        && params->ignore_none_scopes) {
      /* Maybe overwrite node behavior. */
      if (params->start_tile != ptile) {
        node->behavior = TB_IGNORE;
        return FALSE;
      } else if (TB_NORMAL == node->behavior) {
        node->behavior = TB_IGNORE;
      }
    } else if (PF_MS_TRANSPORT == node->move_scope
               && !can_disembark
               && (params->start_tile != ptile
                   || NULL == params->transported_by_initially)) {
      /* Overwrite node behavior. */
      node->behavior = TB_DONT_LEAVE;
    }

    /* ZOC_MINE means can move unrestricted from/into it, ZOC_ALLIED means
     * can move unrestricted into it, but not necessarily from it. */
    if (NULL != params->get_zoc
        && NULL == tile_city(ptile)
        && !terrain_has_flag(tile_terrain(ptile), TER_NO_ZOC)
        && !params->get_zoc(params->owner, ptile, params->map)) {
      node->zoc_number = (0 < unit_list_size(ptile->units)
                          ? ZOC_ALLIED : ZOC_NO);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    } else {
      /* Nodes are allocated by fc_calloc(), so should be already set to
       * 0. */
      node->zoc_number = ZOC_MINE;
#endif
    }
  } else {
    node->moves_left_req =
      params->get_moves_left_req(ptile, node_known_type, params);
    if (PF_IMPOSSIBLE_MC == node->moves_left_req) {
      /* Overwrite node behavior. */
      if (params->start_tile == ptile) {
        node->behavior = TB_DONT_LEAVE;
      } else {
        node->behavior = TB_IGNORE;
        return FALSE;
      }
    }

    node->move_scope = PF_MS_NATIVE;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    /* Nodes are allocated by fc_calloc(), so  should be already set to 0. */
    node->action = PF_ACTION_NONE;
    node->zoc_number = ZOC_MINE;
#endif
  }

  /* Evaluate the extra cost of the destination. */
  if (NULL != params->get_EC) {
    node->extra_tile = params->get_EC(ptile, node_known_type, params);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->extra_tile = 0;
#endif
  }

#ifdef ZERO_VARIABLES_FOR_SEARCHING
  /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
  node->pos = NULL;
  node->segment = NULL;
#endif

  return TRUE;
}

/************************************************************************//**
  Returns whether this node is dangerous or not.
****************************************************************************/
static inline bool pf_fuel_node_dangerous(const struct pf_fuel_node *node)
{
  return (NULL == node->pos
          || (node->pos->moves_left < node->moves_left_req
              && PF_ACTION_NONE == node->action));
}

/************************************************************************//**
  Forget how we went to position. Maybe destroy the position, and previous
  ones.
****************************************************************************/
static inline struct pf_fuel_pos *pf_fuel_pos_ref(struct pf_fuel_pos *pos)
{
#ifdef PF_DEBUG
  /* Unsure we have enough space to store the new count. Maximum is 10
   * (node->pos, node->segment, and 8 for other_pos->prev). */
  fc_assert(15 > pos->ref_count);
#endif
  pos->ref_count++;
  return pos;
}

/************************************************************************//**
  Forget how we went to position. Maybe destroy the position, and previous
  ones.
****************************************************************************/
static inline void pf_fuel_pos_unref(struct pf_fuel_pos *pos)
{
  while (NULL != pos && 0 == --pos->ref_count) {
    struct pf_fuel_pos *prev = pos->prev;

    free(pos);
    pos = prev;
  }
}

/************************************************************************//**
  Replace the position (unreferences it). Instead of destroying, re-use the
  memory, else return a newly allocated position.
****************************************************************************/
static inline struct pf_fuel_pos *
pf_fuel_pos_replace(struct pf_fuel_pos *pos, const struct pf_fuel_node *node)
{
  if (NULL == pos) {
    pos = fc_malloc(sizeof(*pos));
    pos->ref_count = 1;
  } else if (1 < pos->ref_count) {
    pos->ref_count--;
    pos = fc_malloc(sizeof(*pos));
    pos->ref_count = 1;
  } else {
#ifdef PF_DEBUG
    fc_assert(1 == pos->ref_count);
#endif
    pf_fuel_pos_unref(pos->prev);
  }
  pos->cost = node->cost;
  pos->extra_cost = node->extra_cost;
  pos->moves_left = node->moves_left;
  pos->dir_to_here = node->dir_to_here;
  pos->prev = NULL;

  return pos;
}

/************************************************************************//**
  Finalize the fuel position.
****************************************************************************/
static inline void
pf_fuel_finalize_position_base(const struct pf_parameter *param,
                               struct pf_position *pos,
                               int cost, int moves_left)
{
  int move_rate = param->move_rate;

  pos->turn = pf_turns(param, cost);
  if (move_rate > 0 && param->start_tile != pos->tile) {
    pos->fuel_left = moves_left / move_rate;
    pos->moves_left = moves_left % move_rate;
  } else if (param->start_tile == pos->tile) {
    pos->fuel_left = param->fuel_left_initially;
    pos->moves_left = param->moves_left_initially;
  } else {
    pos->fuel_left = param->fuel_left_initially;
    pos->moves_left = moves_left;
  }
}

/************************************************************************//**
  Finalize the fuel position. If we have a fuel segment, then use it.
****************************************************************************/
static inline void
pf_fuel_finalize_position(struct pf_position *pos,
                          const struct pf_parameter *params,
                          const struct pf_fuel_node *node,
                          const struct pf_fuel_pos *head)
{
  if (head) {
    pf_fuel_finalize_position_base(params, pos,
                                   head->cost, head->moves_left);
  } else {
    pf_fuel_finalize_position_base(params, pos,
                                   node->cost, node->moves_left);
  }
}

/************************************************************************//**
  Fill in the position which must be discovered already. A helper
  for pf_fuel_map_position(). This also "finalizes" the position.
****************************************************************************/
static void pf_fuel_map_fill_position(const struct pf_fuel_map *pffm,
                                      struct tile *ptile,
                                      struct pf_position *pos)
{
  int tindex = tile_index(ptile);
  struct pf_fuel_node *node = pffm->lattice + tindex;
  struct pf_fuel_pos *head = node->segment;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pffm));

#ifdef PF_DEBUG
  fc_assert_ret_msg(NULL != head,
                    "Unreached destination (%d, %d).", TILE_XY(ptile));
#endif /* PF_DEBUG */

  pos->tile = ptile;
  pos->total_EC = head->extra_cost;
  pos->total_MC = (head->cost - pf_move_rate(params)
                   + pf_moves_left_initially(params));
  pos->dir_to_here = head->dir_to_here;
  pos->dir_to_next_pos = direction8_invalid();   /* This field does not apply. */
  pf_fuel_finalize_position(pos, params, node, head);
}

/************************************************************************//**
  This function returns the fill cost needed for a position, to get full
  moves at the next turn. This would be called only when the status is
  NS_WAITING.
****************************************************************************/
static inline int
pf_fuel_map_fill_cost_for_full_moves(const struct pf_parameter *param,
                                     int cost, int moves_left)
{
#ifdef PF_DEBUG
  fc_assert(0 < param->move_rate);
#endif /* PF_DEBUG */
  return cost + moves_left % param->move_rate;
}

/************************************************************************//**
  Read off the path to the node 'ptile', but with fuel danger.
****************************************************************************/
static struct pf_path *
pf_fuel_map_construct_path(const struct pf_fuel_map *pffm,
                           struct tile *ptile)
{
  struct pf_path *path = fc_malloc(sizeof(*path));
  enum direction8 dir_next = direction8_invalid();
  struct pf_fuel_node *node = pffm->lattice + tile_index(ptile);
  struct pf_fuel_pos *segment = node->segment;
  int length = 1;
  struct tile *iter_tile = ptile;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pffm));
  struct pf_position *pos;
  int i;

#ifdef PF_DEBUG
  fc_assert_ret_val_msg(NULL != segment, NULL,
                        "Unreached destination (%d, %d).",
                        TILE_XY(ptile));
#endif /* PF_DEBUG */

  /* First iterate to find path length. */
  /* NB: the start point could be reached in the middle of a segment.
   * See comment for pf_fuel_map_create_segment(). */
  while (direction8_is_valid(segment->dir_to_here)) {
    if (node->moves_left_req == 0) {
      /* A refuel point. */
      if (segment != node->segment) {
        length += 2;
        segment = node->segment;
      } else {
        length++;
      }
    } else {
      length++;
    }

    /* Step backward. */
    iter_tile = mapstep(params->map, iter_tile,
                        DIR_REVERSE(segment->dir_to_here));
    node = pffm->lattice + tile_index(iter_tile);
    segment = segment->prev;
#ifdef PF_DEBUG
    fc_assert(NULL != segment);
#endif /* PF_DEBUG */
  }
  if (node->moves_left_req == 0 && segment != node->segment) {
    /* We wait at the start point */
    length++;
  }

  /* Allocate memory for path. */
  path->positions = fc_malloc(length * sizeof(struct pf_position));
  path->length = length;

  /* Reset variables for main iteration. */
  iter_tile = ptile;
  node = pffm->lattice + tile_index(ptile);
  segment = node->segment;

  for (i = length - 1; i >= 0; i--) {
    /* 1: Deal with waiting. */
    if (node->moves_left_req == 0 && segment != node->segment) {
      /* Waited at _this_ tile, need to record it twice in the
       * path. Here we record our state _after_ waiting (e.g.
       * full move points). */
      pos = path->positions + i;
      pos->tile = iter_tile;
      pos->total_EC = segment->extra_cost;
      pos->turn = pf_turns(params, segment->cost);
      pos->total_MC = ((pos->turn - 1) * params->move_rate
                       + params->moves_left_initially);
      pos->moves_left = params->move_rate;
      pos->fuel_left = params->fuel;
      pos->dir_to_next_pos = dir_next;
      dir_next = direction8_invalid();
      segment = node->segment;
      i--;
      if (NULL == segment) {
        /* We waited at start tile, then 'node->segment' is not set. */
#ifdef PF_DEBUG
        fc_assert(iter_tile == params->start_tile);
        fc_assert(0 == i);
#endif /* PF_DEBUG */
        pf_position_fill_start_tile(path->positions, params);
        return path;
      }
    }

    /* 2: Fill the current position. */
    pos = path->positions + i;
    pos->tile = iter_tile;
    pos->total_MC = (pf_moves_left_initially(params)
                     - pf_move_rate(params) + segment->cost);
    pos->total_EC = segment->extra_cost;
    pos->dir_to_next_pos = dir_next;
    pf_fuel_finalize_position(pos, params, node, segment);

    /* 3: Check if we finished. */
    if (i == 0) {
      /* We should be back at the start now! */
      fc_assert_ret_val(iter_tile == params->start_tile, NULL);
      return path;
    }

    /* 4: Calculate the next direction. */
    dir_next = segment->dir_to_here;

    /* 5: Step further back. */
    iter_tile = mapstep(params->map, iter_tile, DIR_REVERSE(dir_next));
    node = pffm->lattice + tile_index(iter_tile);
    segment = segment->prev;
#ifdef PF_DEBUG
    fc_assert(NULL != segment);
#endif /* PF_DEBUG */
  }

  fc_assert_msg(FALSE, "Cannot get to the starting point!");
  return NULL;
}

/************************************************************************//**
  Creating path segment going back from node1 to a safe tile. We need to
  remember the whole segment because any node can be crossed by many fuel
  segments.

  Example: be A, a refuel point, B and C not. We start the path from B and
  have only (3 * SINGLE_MOVE) moves lefts:
    A B C
  B cannot move to C because we would have only (1 * SINGLE_MOVE) move left
  reaching it, and the refuel point is too far. So C->moves_left_req =
  (4 * SINGLE_MOVE).
  The path would be to return to A, wait the end of turn to get full moves,
  and go to C. In a single line: B A B C. So, the point B would be reached
  twice. but, it needs to stores different data for B->cost, B->extra_cost,
  B->moves_left, and B->dir_to_here. That's why we need to record every
  path to unsafe nodes (not for refuel points).
****************************************************************************/
static inline void pf_fuel_map_create_segment(struct pf_fuel_map *pffm,
                                              struct tile *ptile,
                                              struct pf_fuel_node *node)
{
  struct pf_fuel_pos *pos, *next;
  const struct pf_parameter *params = pf_map_parameter(PF_MAP(pffm));

  pos = pf_fuel_pos_replace(node->pos, node);
  node->pos = pos;

   /* Iterate until we reach any built segment. */
  do {
    next = pos;
    ptile = mapstep(params->map, ptile, DIR_REVERSE(node->dir_to_here));
    node = pffm->lattice + tile_index(ptile);
    pos = node->pos;
    if (NULL != pos) {
      if (pos->cost == node->cost
          && pos->dir_to_here == node->dir_to_here
          && pos->extra_cost == node->extra_cost
          && pos->moves_left == node->moves_left) {
        /* Reached an usable segment. */
        next->prev = pf_fuel_pos_ref(pos);
        break;
      }
    }
    /* Update position. */
    pos = pf_fuel_pos_replace(pos, node);
    node->pos = pos;
    next->prev = pf_fuel_pos_ref(pos);
  } while (0 != node->moves_left_req && direction8_is_valid(node->dir_to_here));
}

/************************************************************************//**
  Adjust cost for move_rate and fuel usage.
****************************************************************************/
static inline int pf_fuel_map_adjust_cost(int cost, int moves_left,
                                          int move_rate)
{
  if (move_rate > 0) {
    int remaining_moves = moves_left % move_rate;

    if (remaining_moves == 0) {
      remaining_moves = move_rate;
    }

    return MIN(cost, remaining_moves);
  } else {
    return MIN(cost, moves_left);
  }
}

/************************************************************************//**
  This function returns whether a unit with or without fuel can attack.

  moves_left: moves left before the attack.
  moves_left_req: required moves left to hold on the tile after attacking.
****************************************************************************/
static inline bool
pf_fuel_map_attack_is_possible(const struct pf_parameter *param,
                               int moves_left, int moves_left_req)
{
  if (utype_can_do_action(param->utype, ACTION_SUICIDE_ATTACK)) {
    /* Case missile */
    return TRUE;
  } else if (utype_has_flag(param->utype, UTYF_ONEATTACK)) {
    /* Case Bombers */
    if (moves_left <= param->move_rate) {
      /* We are in the last turn of fuel, don't attack */
      return FALSE;
    } else {
      return TRUE;
    }
  } else {
    /* Case fighters */
    if (moves_left - SINGLE_MOVE < moves_left_req) {
      return FALSE;
    } else {
      return TRUE;
    }
  }
}

/************************************************************************//**
  Primary method for iterative path-finding for fuel units.
  Notes:
  1. We process nodes in the main queue, like for normal maps. Because we
     process in a different queue common tiles (!= refuel points), we needed
     to register every path to any tile from a refuel point or the start tile
     (see comment for pf_fuel_map_create_segment()).
  2. Waiting is realised by inserting the refuel point back into the main
     queue with a lower priority P. Because this tile might pop back sooner
     than P, because there might be several copies of it in the queue already,
     we *must* delete all these copies, to preserve the priority of the
     process.
  3. For some purposes, NS_WAITING is just another flavour of NS_PROCESSED,
     since the path to a NS_WAITING tile has already been found.
  4. This algorithm cannot guarantee the best safe segments across dangerous
     region. However it will find a safe segment if there is one. To
     gurantee the best (in terms of total_CC) safe segments across danger,
     supply 'get_EC' which returns small extra on dangerous tiles.

  During the iteration, the node status will be changed:
  A. NS_UNINIT: The node is not initialized, we didn't reach it at all.
  B. NS_INIT: We have initialized the cached values, however, we failed to
     reach this node.
  C. NS_NEW: We have reached this node, but we are not sure it was the best
     path.
  D. NS_PROCESSED: Now, we are sure we have the best path. Not refuel node
     can even be processed.
  E. NS_WAITING: The refuel node is re-inserted in the priority queue, as
     explained above (2.). We need to consider if waiting for full moves
     open or not new possibilities for moving.
  F. NS_PROCESSED: When finished to consider waiting at the node, revert the
     status to NS_PROCESSED.
  In points D., E., and F., the best path to the node can be considered as
  found.
****************************************************************************/
static bool pf_fuel_map_iterate(struct pf_map *pfm)
{
  struct pf_fuel_map *const pffm = PF_FUEL_MAP(pfm);
  const struct pf_parameter *const params = pf_map_parameter(pfm);
  struct tile *tile = pfm->tile;
  int tindex = tile_index(tile);
  struct pf_fuel_node *node = pffm->lattice + tindex;
  enum pf_move_scope scope = node->move_scope;
  int priority, waited_priority;
  bool waited = FALSE;

  /* The previous position is defined by 'tile' (tile pointer), 'node'
   * (the data of the tile for the pf_map), and index (the index of the
   * position in the Freeciv map). */

  if (!direction8_is_valid(node->dir_to_here)
      && NULL != params->transported_by_initially) {
#ifdef PF_DEBUG
    fc_assert(tile == params->start_tile);
#endif
    scope |= PF_MS_TRANSPORT;
    if (!utype_can_freely_unload(params->utype,
                                 params->transported_by_initially)
        && NULL == tile_city(tile)
        && !tile_has_native_base(tile, params->transported_by_initially)) {
      /* Cannot disembark, don't leave transporter. */
      node->behavior = TB_DONT_LEAVE;
    }
  }

  for (;;) {
    /* There is no exit from DONT_LEAVE tiles! */
    if (node->behavior != TB_DONT_LEAVE
        && scope != PF_MS_NONE
        && (params->move_rate > 0 || node->cost < 0)) {
      int loc_cost = node->cost;
      int loc_moves_left = node->moves_left;

      if (0 == node->moves_left_req
          && 0 < params->move_rate
          && 0 == loc_moves_left % params->move_rate
          && loc_cost >= params->moves_left_initially) {
        /* We have implicitly refueled at the end of the turn. Update also
         * 'node->moves_left' to ensure to wait there in paths. */
        loc_moves_left = pf_move_rate(params);
        node->moves_left = loc_moves_left;
      }

      adjc_dir_iterate(params->map, tile, tile1, dir) {
        /* Calculate the cost of every adjacent position and set them in
         * the priority queues for next call to pf_fuel_map_iterate(). */
        int tindex1 = tile_index(tile1);
        struct pf_fuel_node *node1 = pffm->lattice + tindex1;
        int cost, extra = 0;
        int moves_left;
        int cost_of_path, old_cost_of_path;
        struct pf_fuel_pos *pos;

        /* As for the previous position, 'tile1', 'node1' and 'tindex1' are
         * defining the adjacent position. */

        /* Non-full fuel tiles can be updated even after being processed. */
        if ((NS_PROCESSED == node1->status || NS_WAITING == node1->status)
            && 0 == node1->moves_left_req) {
          /* This gives 15% speedup. */
          continue;
        }

        /* Initialise target tile if necessary */
        if (node1->status == NS_UNINIT) {
          /* Only initialize once. See comment for pf_fuel_node_init().
           * Node status step A. to B. */
          if (!pf_fuel_node_init(pffm, node1, tile1, scope)) {
            continue;
          }
        } else if (TB_IGNORE == node1->behavior) {
          /* We cannot enter this tile at all! */
          continue;
        }

        /* Is the move ZOC-ok? */
        if (node->zoc_number != ZOC_MINE && node1->zoc_number == ZOC_NO) {
          continue;
        }

        cost = node1->cost_to_here[dir];
        if (0 == cost) {
          /* Evaluate the cost of the move. */
          if (PF_ACTION_NONE != node1->action) {
            if (NULL != params->is_action_possible
                && !params->is_action_possible(tile, scope, tile1,
                                               node1->action, params)) {
              node1->cost_to_here[dir] = PF_IMPOSSIBLE_MC + 2;
              continue;
            }
            /* action move cost depends on action and unit type. */
            if (node1->action == PF_ACTION_ATTACK
                && (utype_has_flag(params->utype, UTYF_ONEATTACK)
                    || utype_can_do_action(params->utype,
                                           ACTION_SUICIDE_ATTACK))) {
              /* Assume that the attack will be a suicide attack even if a
               * regular attack may be legal. */
              cost = params->move_rate;
            } else {
              cost = SINGLE_MOVE;
            }
          } else if (node1->node_known_type == TILE_UNKNOWN) {
            cost = params->utype->unknown_move_cost;
          } else {
            cost = params->get_MC(tile, scope, tile1, node1->move_scope,
                                  params);
          }
#ifdef PF_DEBUG
          fc_assert(1 << (8 * sizeof(node1->cost_to_here[dir])) > cost + 2);
          fc_assert(0 < cost + 2);
#endif
          node1->cost_to_here[dir] = cost + 2;
          if (cost == PF_IMPOSSIBLE_MC) {
            continue;
          }
        } else if (cost == PF_IMPOSSIBLE_MC - 2) {
          continue;
        } else {
          cost -= 2;
        }

        cost = pf_fuel_map_adjust_cost(cost, loc_moves_left,
                                       params->move_rate);

        moves_left = loc_moves_left - cost;
        if (moves_left < node1->moves_left_req
            && (!utype_can_do_action(params->utype, ACTION_SUICIDE_ATTACK)
                || 0 > moves_left)) {
          /* We don't have enough moves left, but missiles
           * can do suicidal attacks. */
          continue;
        }

        if (PF_ACTION_ATTACK == node1->action
            && !pf_fuel_map_attack_is_possible(params, loc_moves_left,
                                               node->moves_left_req)) {
          /* We wouldn't have enough moves left after attacking. */
          continue;
        }

        /* Total cost at 'tile1' */
        cost += loc_cost;

        /* Evaluate the extra cost of the destination, if it's relevant. */
        if (NULL != params->get_EC) {
          extra = node1->extra_tile + node->extra_cost;
        }

        /* Update costs and add to queue, if this is a better route
         * to tile1. Case safe tiles or reached directly without waiting. */
        pos = node1->segment;
        cost_of_path = pf_fuel_total_CC(params, cost, extra,
                                        moves_left - node1->moves_left_req);
        if (node1->status == NS_INIT) {
          /* Not calculated yet. */
          old_cost_of_path = 0;
        } else if (pos) {
          /* We have a path to this tile. The default values could have been
           * overwritten if we had more moves left to deal with waiting.
           * Then, we have to get back the value of this node to calculate
           * the cost. */
          old_cost_of_path =
              pf_fuel_total_CC(params, pos->cost, pos->extra_cost,
                               pos->moves_left - node1->moves_left_req);
        } else {
          /* Default cost */
          old_cost_of_path =
              pf_fuel_total_CC(params, node1->cost, node1->extra_cost,
                               node1->moves_left - node1->moves_left_req);
        }

        /* Step 1: We test if this route is the best to this tile, by a
         * direct way, not taking in account waiting. */

        if (NS_INIT == node1->status || cost_of_path < old_cost_of_path) {
          /* We are reaching this node for the first time, or we found a
           * better route to 'tile1', or we would have more moves lefts
           * at previous position. Let's register 'tindex1' to the
           * priority queue. */
          node1->extra_cost = extra;
          node1->cost = cost;
          node1->moves_left = moves_left;
          node1->dir_to_here = dir;
          /* Always record the segment, including when it is not dangerous
           * to move there. */
          pf_fuel_map_create_segment(pffm, tile1, node1);
          if (NS_INIT == node1->status) {
            /* Node status B. to C. */
            node1->status = NS_NEW;
            map_index_pq_insert(pffm->queue, tindex1, -cost_of_path);
          } else {
            /* else staying at D. */
#ifdef PF_DEBUG
            fc_assert(NS_NEW == node1->status);
#endif
            if (cost_of_path < old_cost_of_path) {
              map_index_pq_replace(pffm->queue, tindex1, -cost_of_path);
            }
          }
          continue;     /* adjc_dir_iterate() */
        }

        /* Step 2: We test if this route could open better routes for other
         * tiles, if we waited somewhere. */

        if (!waited
            || NS_NEW == node1->status
            || 0 == node1->moves_left_req) {
          /* Stops there if:
           * 1. we didn't wait to get there ;
           * 2. we didn't process yet the node ;
           * 3. this is a refuel point. */
          continue;     /* adjc_dir_iterate() */
        }

#ifdef PF_DEBUG
        fc_assert(NS_PROCESSED == node1->status);
#endif

        if (moves_left > node1->moves_left
            || (moves_left == node1->moves_left
                && extra < node1->extra_cost)) {
          /* We will update costs if:
           * 1. we would have more moves left than previously on this node.
           * 2. we can have lower extra and will not overwrite anything
           *    useful. */
          node1->extra_cost = extra;
          node1->cost = cost;
          node1->moves_left = moves_left;
          node1->dir_to_here = dir;
          map_index_pq_insert(pffm->waited_queue, tindex1,
                              -pf_fuel_waited_total_CC(cost,
                                  moves_left - node1->moves_left_req));
        }
      } adjc_dir_iterate_end;
    }

    if (NS_WAITING == node->status) {
      /* Node status final step E. to F. */
#ifdef PF_DEBUG
      fc_assert(0 == node->moves_left_req);
#endif
      node->status = NS_PROCESSED;
    } else if (0 == node->moves_left_req
               && PF_ACTION_NONE == node->action
               && node->moves_left < pf_move_rate(params)
#ifdef PF_DEBUG
               && (fc_assert(0 < params->move_rate), 0 < params->move_rate)
#endif
               && (0 != node->moves_left % params->move_rate
                   || node->cost < params->moves_left_initially)) {
      /* Consider waiting at this node. To do it, put it back into queue.
       * Node status final step D. to E. */
      node->status = NS_WAITING;
      node->cost = pf_fuel_map_fill_cost_for_full_moves(params, node->cost,
                                                        node->moves_left);
      node->moves_left = pf_move_rate(params);
      map_index_pq_insert(pffm->queue, tindex,
                          -pf_fuel_waited_total_CC(node->cost,
                                                   node->moves_left));
    }

    /* Get the next node (the index with the highest priority). First try
     * to get it from waited_queue. */
    if (!map_index_pq_priority(pffm->queue, &priority)
        || (map_index_pq_priority(pffm->waited_queue, &waited_priority)
            && priority < waited_priority)) {
      if (!map_index_pq_remove(pffm->waited_queue, &tindex)) {
        /* End of the iteration. */
        return FALSE;
      }

      /* Change the pf_map iterator and reset data. */
      tile = index_to_tile(params->map, tindex);
      pfm->tile = tile;
      node = pffm->lattice + tindex;
      waited = TRUE;
#ifdef PF_DEBUG
      fc_assert(0 < node->moves_left_req);
      fc_assert(NS_PROCESSED == node->status);
#endif
    } else {
#ifdef PF_DEBUG
      bool success = map_index_pq_remove(pffm->queue, &tindex);
      fc_assert(TRUE == success);
#else
      map_index_pq_remove(pffm->queue, &tindex);
#endif

      /* Change the pf_map iterator and reset data. */
      tile = index_to_tile(params->map, tindex);
      pfm->tile = tile;
      node = pffm->lattice + tindex;
#ifdef PF_DEBUG
      fc_assert(NS_PROCESSED != node->status);
#endif

      if (NS_WAITING != node->status && !pf_fuel_node_dangerous(node)) {
        /* Node status step C. and D. */
        node->status = NS_PROCESSED;
        node->segment = pf_fuel_pos_ref(node->pos);
        return TRUE;
      }
      waited = (NS_WAITING == node->status);
    }

#ifdef PF_DEBUG
    fc_assert(NS_INIT < node->status);

    if (NS_WAITING == node->status) {
      /* We've already returned this node once, skip it. */
      log_debug("Considering waiting at (%d, %d)", TILE_XY(tile));
    } else if (NS_PROCESSED == node->status) {
      /* We've already returned this node once, skip it. */
      log_debug("Reprocessing tile (%d, %d)", TILE_XY(tile));
    } else if (pf_fuel_node_dangerous(node)) {
      /* We don't return dangerous tiles. */
      log_debug("Reached dangerous tile (%d, %d)", TILE_XY(tile));
    }
#endif /* PF_DEBUG */

    scope = node->move_scope;
  }

  log_error("%s(): internal error.", __FUNCTION__);
  return FALSE;
}

/************************************************************************//**
  Iterate the map until 'ptile' is reached.
****************************************************************************/
static inline bool pf_fuel_map_iterate_until(struct pf_fuel_map *pffm,
                                             struct tile *ptile)
{
  struct pf_map *pfm = PF_MAP(pffm);
  struct pf_fuel_node *node = pffm->lattice + tile_index(ptile);

  /* Start position is handled in every function calling this function. */

  if (NS_UNINIT == node->status) {
    /* Initialize the node, for doing the following tests. */
    if (!pf_fuel_node_init(pffm, node, ptile, PF_MS_NONE)) {
      return FALSE;
    }
  } else if (TB_IGNORE == node->behavior) {
    /* Simpliciation: if we cannot enter this node at all, don't iterate the
     * whole map. */
    return FALSE;
  }

  while (NULL == node->segment) {
    if (!pf_map_iterate(pfm)) {
      /* All reachable destination have been iterated, 'ptile' is
       * unreachable. */
      return FALSE;
    }
  }

  return TRUE;
}

/************************************************************************//**
  Return the move cost at ptile. If 'ptile' has not been reached yet,
  iterate the map until we reach it or run out of map. This function
  returns PF_IMPOSSIBLE_MC on unreachable positions.
****************************************************************************/
static int pf_fuel_map_move_cost(struct pf_map *pfm, struct tile *ptile)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    return 0;
  } else if (pf_fuel_map_iterate_until(pffm, ptile)) {
    const struct pf_fuel_node *node = pffm->lattice + tile_index(ptile);

    return (node->segment->cost
            - pf_move_rate(pf_map_parameter(pfm))
            + pf_moves_left_initially(pf_map_parameter(pfm)));
  } else {
    return PF_IMPOSSIBLE_MC;
  }
}

/************************************************************************//**
  Return the path to ptile. If 'ptile' has not been reached yet, iterate
  the map until we reach it or run out of map.
****************************************************************************/
static struct pf_path *pf_fuel_map_path(struct pf_map *pfm,
                                        struct tile *ptile)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    return pf_path_new_to_start_tile(pf_map_parameter(pfm));
  } else if (pf_fuel_map_iterate_until(pffm, ptile)) {
    return pf_fuel_map_construct_path(pffm, ptile);
  } else {
    return NULL;
  }
}

/************************************************************************//**
  Get info about position at ptile and put it in pos. If 'ptile' has not
  been reached yet, iterate the map until we reach it. Should _always_
  check the return value, forthe position might be unreachable.
****************************************************************************/
static bool pf_fuel_map_position(struct pf_map *pfm, struct tile *ptile,
                                 struct pf_position *pos)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);

  if (ptile == pfm->params.start_tile) {
    pf_position_fill_start_tile(pos, pf_map_parameter(pfm));
    return TRUE;
  } else if (pf_fuel_map_iterate_until(pffm, ptile)) {
    pf_fuel_map_fill_position(pffm, ptile, pos);
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  'pf_fuel_map' destructor.
****************************************************************************/
static void pf_fuel_map_destroy(struct pf_map *pfm)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);
  struct pf_fuel_node *node;
  int i;

  /* Need to clean up the dangling fuel segments. */
  for (i = 0, node = pffm->lattice; i < MAP_INDEX_SIZE; i++, node++) {
    pf_fuel_pos_unref(node->pos);
    pf_fuel_pos_unref(node->segment);
  }
  free(pffm->lattice);
  map_index_pq_destroy(pffm->queue);
  map_index_pq_destroy(pffm->waited_queue);
  free(pffm);
}

/************************************************************************//**
  'pf_fuel_map' constructor.
****************************************************************************/
static struct pf_map *pf_fuel_map_new(const struct pf_parameter *parameter)
{
  struct pf_fuel_map *pffm;
  struct pf_map *base_map;
  struct pf_parameter *params;
  struct pf_fuel_node *node;

  pffm = fc_malloc(sizeof(*pffm));
  base_map = &pffm->base_map;
  params = &base_map->params;
#ifdef PF_DEBUG
  /* Set the mode, used for cast check. */
  base_map->mode = PF_FUEL;
#endif /* PF_DEBUG */

  /* Allocate the map. */
  pffm->lattice = fc_calloc(MAP_INDEX_SIZE, sizeof(struct pf_fuel_node));
  pffm->queue = map_index_pq_new(INITIAL_QUEUE_SIZE);
  pffm->waited_queue = map_index_pq_new(INITIAL_QUEUE_SIZE);

  /* 'get_MC' callback must be set. */
  fc_assert_ret_val(parameter->get_MC != NULL, NULL);

  /* 'get_moves_left_req' callback must be set. */
  fc_assert_ret_val(parameter->get_moves_left_req != NULL, NULL);

  /* 'get_move_scope' callback must be set. */
  fc_assert_ret_val(parameter->get_move_scope != NULL, NULL);

  /* Copy parameters. */
  *params = *parameter;

  /* Initialize virtual function table. */
  base_map->destroy = pf_fuel_map_destroy;
  base_map->get_move_cost = pf_fuel_map_move_cost;
  base_map->get_path = pf_fuel_map_path;
  base_map->get_position = pf_fuel_map_position;
  base_map->iterate = pf_fuel_map_iterate;

  /* Initialise starting node. */
  node = pffm->lattice + tile_index(params->start_tile);
  if (!pf_fuel_node_init(pffm, node, params->start_tile, PF_MS_NONE)) {
    /* Always fails. */
    fc_assert(TRUE == pf_fuel_node_init(pffm, node, params->start_tile,
                                        PF_MS_NONE));
  }

  /* NB: do not handle params->transported_by_initially because we want to
   * handle only at start, not when crossing over the start tile for a
   * second time. See pf_danger_map_iterate(). */

  /* Initialise the iterator. */
  base_map->tile = params->start_tile;

  /* This makes calculations of turn/moves_left more convenient, but we
   * need to subtract this value before we return cost to the user. Note
   * that cost may be negative if moves_left_initially > move_rate
   * (see pf_turns()). */
  node->moves_left = pf_moves_left_initially(params);
  node->cost = pf_move_rate(params) - node->moves_left;
  node->extra_cost = 0;
  node->dir_to_here = direction8_invalid();
  node->status = NS_PROCESSED;

  return PF_MAP(pffm);
}



/* ====================== pf_map public functions ======================= */

/************************************************************************//**
  Factory function to create a new map according to the parameter.
  Does not do any iterations.
****************************************************************************/
struct pf_map *pf_map_new(const struct pf_parameter *parameter)
{
  if (parameter->is_pos_dangerous) {
    if (parameter->get_moves_left_req) {
      log_error("path finding code cannot deal with dangers "
                "and fuel together.");
    }
    if (parameter->get_costs) {
      log_error("jumbo callbacks for danger maps are not yet implemented.");
    }
    return pf_danger_map_new(parameter);
  } else if (parameter->get_moves_left_req) {
    if (parameter->get_costs) {
      log_error("jumbo callbacks for fuel maps are not yet implemented.");
    }
    return pf_fuel_map_new(parameter);
  }

  return pf_normal_map_new(parameter);
}

/************************************************************************//**
  After usage the map must be destroyed.
****************************************************************************/
void pf_map_destroy(struct pf_map *pfm)
{
#ifdef PF_DEBUG
  fc_assert_ret(NULL != pfm);
#endif
  pfm->destroy(pfm);
}

/************************************************************************//**
  Tries to find the minimal move cost to reach ptile. Returns
  PF_IMPOSSIBLE_MC if not reachable. If ptile has not been reached yet,
  iterate the map until we reach it or run out of map.
****************************************************************************/
int pf_map_move_cost(struct pf_map *pfm, struct tile *ptile)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, PF_IMPOSSIBLE_MC);
  fc_assert_ret_val(NULL != ptile, PF_IMPOSSIBLE_MC);
#endif
  return pfm->get_move_cost(pfm, ptile);
}

/************************************************************************//**
  Tries to find the best path in the given map to the position ptile.
  If NULL is returned no path could be found. The pf_path_last_position()
  of such path would be the same (almost) as the result of the call to 
  pf_map_position(). If ptile has not been reached yet, iterate the map
  until we reach it or run out of map.
****************************************************************************/
struct pf_path *pf_map_path(struct pf_map *pfm, struct tile *ptile)
{
#ifdef PF_DEBUG
  struct pf_path *path;

  fc_assert_ret_val(NULL != pfm, NULL);
  fc_assert_ret_val(NULL != ptile, NULL);
  path = pfm->get_path(pfm, ptile);

  if (path != NULL) {
    const struct pf_parameter *param = pf_map_parameter(pfm);
    const struct pf_position *pos = &path->positions[0];

    fc_assert(path->length >= 1);
    fc_assert(pos->tile == param->start_tile);
    fc_assert(pos->moves_left == param->moves_left_initially);
    fc_assert(pos->fuel_left == param->fuel_left_initially);
  }

  return path;
#else
  return pfm->get_path(pfm, ptile);
#endif
}

/************************************************************************//**
  Get info about position at ptile and put it in pos. If ptile has not been
  reached yet, iterate the map until we reach it. Should _always_ check the
  return value, forthe position might be unreachable.
****************************************************************************/
bool pf_map_position(struct pf_map *pfm, struct tile *ptile,
                     struct pf_position *pos)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, FALSE);
  fc_assert_ret_val(NULL != ptile, FALSE);
#endif
  return pfm->get_position(pfm, ptile, pos);
}

/************************************************************************//**
  Iterates the path-finding algorithm one step further, to the next nearest
  position. This full info on this position and the best path to it can be
  obtained using pf_map_iter_move_cost(), pf_map_iter_path(), and
  pf_map_iter_position() correspondingly. Returns FALSE if no further
  positions are available in this map.

  NB: If pf_map_move_cost(pfm, ptile), pf_map_path(pfm, ptile), or
  pf_map_position(pfm, ptile) has been called before the call to
  pf_map_iterate(), the iteration will resume from 'ptile'.
****************************************************************************/
bool pf_map_iterate(struct pf_map *pfm)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, FALSE);
#endif

  if (NULL == pfm->tile) {
    /* The end of the iteration was already reached. Don't try to iterate
     * again. */
    return FALSE;
  }

  if (!pfm->iterate(pfm)) {
    /* End of iteration. */
    pfm->tile = NULL;
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Return the current tile.
****************************************************************************/
struct tile *pf_map_iter(struct pf_map *pfm)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, NULL);
#endif
  return pfm->tile;
}

/************************************************************************//**
  Return the move cost at the current position. This is equivalent to
  pf_map_move_cost(pfm, pf_map_iter(pfm)).
****************************************************************************/
int pf_map_iter_move_cost(struct pf_map *pfm)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, PF_IMPOSSIBLE_MC);
  fc_assert_ret_val(NULL != pfm->tile, PF_IMPOSSIBLE_MC);
#endif
  return pfm->get_move_cost(pfm, pfm->tile);
}

/************************************************************************//**
  Return the path to our current position.This is equivalent to
  pf_map_path(pfm, pf_map_iter(pfm)).
****************************************************************************/
struct pf_path *pf_map_iter_path(struct pf_map *pfm)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, NULL);
  fc_assert_ret_val(NULL != pfm->tile, NULL);
#endif
  return pfm->get_path(pfm, pfm->tile);
}

/************************************************************************//**
  Read all info about the current position into pos. This is equivalent to
  pf_map_position(pfm, pf_map_iter(pfm), &pos).
****************************************************************************/
void pf_map_iter_position(struct pf_map *pfm, struct pf_position *pos)
{
#ifdef PF_DEBUG
  fc_assert_ret(NULL != pfm);
  fc_assert_ret(NULL != pfm->tile);
#endif
  if (!pfm->get_position(pfm, pfm->tile, pos)) {
    /* Always fails. */
    fc_assert(pfm->get_position(pfm, pfm->tile, pos));
  }
}

/************************************************************************//**
  Return the pf_parameter for given pf_map.
****************************************************************************/
const struct pf_parameter *pf_map_parameter(const struct pf_map *pfm)
{
#ifdef PF_DEBUG
  fc_assert_ret_val(NULL != pfm, NULL);
#endif
  return &pfm->params;
}


/* ====================== pf_path public functions ======================= */

/************************************************************************//**
  Fill the position for the start tile of a parameter.
****************************************************************************/
static void pf_position_fill_start_tile(struct pf_position *pos,
                                        const struct pf_parameter *param)
{
  pos->tile = param->start_tile;
  pos->turn = 0;
  pos->moves_left = param->moves_left_initially;
  pos->fuel_left = param->fuel_left_initially;
  pos->total_MC = 0;
  pos->total_EC = 0;
  pos->dir_to_next_pos = direction8_invalid();
  pos->dir_to_here = direction8_invalid();
}

/************************************************************************//**
  Create a path to the start tile of a parameter.
****************************************************************************/
static struct pf_path *
pf_path_new_to_start_tile(const struct pf_parameter *param)
{
  struct pf_path *path = fc_malloc(sizeof(*path));
  struct pf_position *pos = fc_malloc(sizeof(*pos));

  path->length = 1;
  pf_position_fill_start_tile(pos, param);
  path->positions = pos;
  return path;
}

/************************************************************************//**
  After use, a path must be destroyed. Note this function accept NULL as
  argument.
****************************************************************************/
void pf_path_destroy(struct pf_path *path)
{
  if (NULL != path) {
    free(path->positions);
    free(path);
  }
}

/************************************************************************//**
  Concatenate two paths together. The additional segment 'src_path'
  should start where the initial segment 'dest_path' stops. The
  overlapping position is removed.

  If 'dest_path' == NULL, we just copy the src_path and nothing else.
****************************************************************************/
struct pf_path *pf_path_concat(struct pf_path *dest_path,
                               const struct pf_path *src_path)
{
  int dest_end;

  fc_assert_ret_val(src_path != NULL, NULL);

  if (dest_path == NULL) {
    /* Just copy path. */
    dest_path = fc_malloc(sizeof(*dest_path));
    dest_path->length = src_path->length;
    dest_path->positions = fc_malloc(sizeof(*dest_path->positions)
                                     * dest_path->length);
    memcpy(dest_path->positions, src_path->positions,
           sizeof(*dest_path->positions) * dest_path->length);
    return dest_path;
  }

  dest_end = dest_path->length - 1;
  fc_assert(dest_path->positions[dest_end].tile
            == src_path->positions[0].tile);
  fc_assert(dest_path->positions[dest_end].moves_left
            == src_path->positions[0].moves_left);
  fc_assert(dest_path->positions[dest_end].fuel_left
            == src_path->positions[0].fuel_left);

  if (src_path->length == 1) {
    return dest_path;
  }

  dest_path->length = dest_end + src_path->length;
  dest_path->positions = fc_realloc(dest_path->positions,
                                    sizeof(*dest_path->positions)
                                    * dest_path->length);
  /* Be careful to include the first position of src_path, it contains
   * the direction (it is undefined in the last position of dest_path) */
  memcpy(dest_path->positions + dest_end, src_path->positions,
         sizeof(*dest_path->positions) * src_path->length);

  return dest_path;
}

/************************************************************************//**
  Remove the part of a path leading up to a given tile.
  If given tile is on the path more than once then the first occurrence
  will be the one used.
  If tile is not on the path at all, returns FALSE and path is not changed
  at all.
****************************************************************************/
bool pf_path_advance(struct pf_path *path, struct tile *ptile)
{
  int i;
  struct pf_position *new_positions;

  for (i = 0; path->positions[i].tile != ptile; i++) {
    if (i >= path->length) {
      return FALSE;
    }
  }
  fc_assert_ret_val(i < path->length, FALSE);
  path->length -= i;
  new_positions = fc_malloc(sizeof(*path->positions) * path->length);
  memcpy(new_positions, path->positions + i,
         path->length * sizeof(*path->positions));
  free(path->positions);
  path->positions = new_positions;

  return TRUE;
}

/************************************************************************//**
  Remove the part of a path following a given tile.
  If given tile is on the path more than once then the last occurrence
  will be the one used.
  If tile is not on the path at all, returns FALSE and path is not changed
  at all.
****************************************************************************/
bool pf_path_backtrack(struct pf_path *path, struct tile *ptile)
{
  int i;
  struct pf_position *new_positions;

  fc_assert_ret_val(path->length > 0, FALSE);

  for (i = path->length - 1; path->positions[i].tile != ptile; i--) {
    if (i <= 0) {
      return FALSE;
    }
  }

  fc_assert_ret_val(i >= 0, FALSE);

  path->length = i + 1;
  new_positions = fc_malloc(sizeof(*path->positions) * path->length);
  memcpy(new_positions, path->positions,
         path->length * sizeof(*path->positions));
  free(path->positions);
  path->positions = new_positions;

  return TRUE;
}

/************************************************************************//**
  Get the last position of the path.
****************************************************************************/
const struct pf_position *pf_path_last_position(const struct pf_path *path)
{
  return path->positions + (path->length - 1);
}

/************************************************************************//**
  Debug a path. This function shouldn't be called directly, see
  pf_path_print() defined in "path_finding.h".
****************************************************************************/
void pf_path_print_real(const struct pf_path *path, enum log_level level,
                        const char *file, const char *function, int line)
{
  struct pf_position *pos;
  int i;

  if (path) {
    do_log(file, function, line, TRUE, level,
           "PF: path (at %p) consists of %d positions:",
           (void *) path, path->length);
  } else {
    do_log(file, function, line, TRUE, level, "PF: path is NULL");
    return;
  }

  for (i = 0, pos = path->positions; i < path->length; i++, pos++) {
    do_log(file, function, line, FALSE, level,
           "PF:   %2d/%2d: (%2d,%2d) dir=%-2s cost=%2d (%2d, %d) EC=%d",
           i + 1, path->length, TILE_XY(pos->tile),
           dir_get_name(pos->dir_to_next_pos), pos->total_MC,
           pos->turn, pos->moves_left, pos->total_EC);
  }
}


/* ===================== pf_reverse_map functions ======================== */

/* The path-finding reverse maps are used check the move costs that the
 * units needs to reach the start tile. It stores a pf_map for every unit
 * type. */

static genhash_val_t pf_pos_hash_val(const struct pf_parameter *parameter);
static bool pf_pos_hash_cmp(const struct pf_parameter *parameter1,
                            const struct pf_parameter *parameter2);
static void pf_reverse_map_destroy_pos(struct pf_position *pos);
static void pf_reverse_map_destroy_param(struct pf_parameter *param);

#define SPECHASH_TAG pf_pos
#define SPECHASH_IKEY_TYPE struct pf_parameter *
#define SPECHASH_IDATA_TYPE struct pf_position *
#define SPECHASH_IKEY_VAL pf_pos_hash_val
#define SPECHASH_IKEY_COMP pf_pos_hash_cmp
#define SPECHASH_IKEY_FREE pf_reverse_map_destroy_param
#define SPECHASH_IDATA_FREE pf_reverse_map_destroy_pos
#include "spechash.h"

/* The reverse map structure. */
struct pf_reverse_map {
  struct tile *target_tile;     /* Where we want to go. */
  int max_turns;                /* The maximum of turns. */
  struct pf_parameter template; /* Keep a parameter ready for usage. */
  struct pf_pos_hash *hash;     /* A hash where pf_position are stored. */
};

/* Here goes all unit type flags which affect the move rules handled by
 * the reverse map. */
static const enum unit_type_flag_id signifiant_flags[] = {
  UTYF_IGTER, UTYF_CIVILIAN, UTYF_COAST_STRICT
};
static const size_t signifiant_flags_num = ARRAY_SIZE(signifiant_flags);

/************************************************************************//**
  Hash function for pf_parameter key.
****************************************************************************/
static genhash_val_t pf_pos_hash_val(const struct pf_parameter *parameter)
{
  genhash_val_t result = 0;
  size_t b, i;

  for (i = 0, b = sizeof(result) * 8 - 1; i < signifiant_flags_num;
       i++, b--) {
    if (utype_has_flag(parameter->utype, signifiant_flags[i])) {
      result |= (1u << b);
    }
  }

  result += (uclass_number(utype_class(parameter->utype))
             + (parameter->move_rate << 5)
             + (tile_index(parameter->start_tile) << 11));
  if (!parameter->omniscience) {
    result += parameter->utype->unknown_move_cost << 23;
  }

  return result;
}

/************************************************************************//**
  Comparison function for pf_parameter hash key.
****************************************************************************/
static bool pf_pos_hash_cmp(const struct pf_parameter *parameter1,
                            const struct pf_parameter *parameter2)
{
  size_t i;

  if (parameter1->start_tile != parameter2->start_tile
      || parameter1->move_rate != parameter2->move_rate) {
    return FALSE;
  }

  if (parameter1->utype == parameter2->utype) {
    /* Short test. */
    return TRUE;
  }

  if (utype_class(parameter1->utype) != utype_class(parameter2->utype)) {
    return FALSE;
  }

  if (!parameter1->omniscience) {
#ifdef PF_DEBUG
    fc_assert(parameter2->omniscience == FALSE);
#endif
    if (parameter1->utype->unknown_move_cost
        != parameter2->utype->unknown_move_cost) {
      return FALSE;
    }
  }

  for (i = 0; i < signifiant_flags_num; i++) {
    if (utype_has_flag(parameter1->utype, signifiant_flags[i])
        != utype_has_flag(parameter2->utype, signifiant_flags[i])) {
      return FALSE;
    }
  }

  return TRUE;
}

/************************************************************************//**
  Destroy the position if not NULL.
****************************************************************************/
static void pf_reverse_map_destroy_pos(struct pf_position *pos)
{
  free(pos);
}

/************************************************************************//**
  Destroy the parameter.
****************************************************************************/
static void pf_reverse_map_destroy_param(struct pf_parameter *param)
{
  free(param);
}

/************************************************************************//**
  'pf_reverse_map' constructor. If 'max_turns' is positive, then it won't
  try to iterate the maps beyond this number of turns.
****************************************************************************/
struct pf_reverse_map *pf_reverse_map_new(const struct player *pplayer,
                                          struct tile *target_tile,
                                          int max_turns, bool omniscient,
                                          const struct civ_map *map)
{
  struct pf_reverse_map *pfrm = fc_malloc(sizeof(struct pf_reverse_map));
  struct pf_parameter *param = &pfrm->template;

  pfrm->target_tile = target_tile;
  pfrm->max_turns = max_turns;

  /* Initialize the parameter. */
  pft_fill_reverse_parameter(param, target_tile);
  param->owner = pplayer;
  param->omniscience = omniscient;
  param->map = map;

  /* Initialize the map hash. */
  pfrm->hash = pf_pos_hash_new();

  return pfrm;
}

/************************************************************************//**
  'pf_reverse_map' constructor for city. If 'max_turns' is positive, then
  it won't try to iterate the maps beyond this number of turns.
****************************************************************************/
struct pf_reverse_map *pf_reverse_map_new_for_city(const struct city *pcity,
                                                   const struct player *attacker,
                                                   int max_turns, bool omniscient,
                                                   const struct civ_map *map)
{
  return pf_reverse_map_new(attacker, city_tile(pcity), max_turns, omniscient,
                            map);
}

/************************************************************************//**
  'pf_reverse_map' destructor.
****************************************************************************/
void pf_reverse_map_destroy(struct pf_reverse_map *pfrm)
{
  fc_assert_ret(NULL != pfrm);

  pf_pos_hash_destroy(pfrm->hash);
  free(pfrm);
}

/************************************************************************//**
  Returns the map for the unit type. Creates it if needed. Returns NULL if
  'target_tile' is unreachable.
****************************************************************************/
static const struct pf_position *
pf_reverse_map_pos(struct pf_reverse_map *pfrm,
                   const struct pf_parameter *param)
{
  struct pf_position *pos;
  struct pf_map *pfm;
  struct pf_parameter *copy;
  struct tile *target_tile;
  const struct pf_normal_node *lattice;
  int max_cost;

  /* Check if we already processed something similar. */
  if (pf_pos_hash_lookup(pfrm->hash, param, &pos)) {
    return pos;
  }

  /* We didn't. Build map and iterate. */
  pfm = pf_normal_map_new(param);
  lattice = PF_NORMAL_MAP(pfm)->lattice;
  target_tile = pfrm->target_tile;
  if (pfrm->max_turns >= 0) {
    max_cost = param->move_rate * (pfrm->max_turns + 1);
    do {
      if (lattice[tile_index(pfm->tile)].cost >= max_cost) {
        break;
      } else if (pfm->tile == target_tile) {
        /* Found our position. Insert in hash, destroy map, and return. */
        pos = fc_malloc(sizeof(*pos));
        pf_normal_map_fill_position(PF_NORMAL_MAP(pfm), target_tile, pos);
        copy = fc_malloc(sizeof(*copy));
        *copy = *param;
        pf_pos_hash_insert(pfrm->hash, copy, pos);
        pf_map_destroy(pfm);
        return pos;
      }
    } while (pfm->iterate(pfm));
  } else {
    /* No limit for iteration. */
    do {
      if (pfm->tile == target_tile) {
        /* Found our position. Insert in hash, destroy map, and return. */
        pos = fc_malloc(sizeof(*pos));
        pf_normal_map_fill_position(PF_NORMAL_MAP(pfm), target_tile, pos);
        copy = fc_malloc(sizeof(*copy));
        *copy = *param;
        pf_pos_hash_insert(pfrm->hash, copy, pos);
        pf_map_destroy(pfm);
        return pos;
      }
    } while (pfm->iterate(pfm));
  }
  pf_map_destroy(pfm);

  /* Position not found. Let's insert NULL as position to avoid to iterate
   * the map again. */
  copy = fc_malloc(sizeof(*copy));
  *copy = *param;
  pf_pos_hash_insert(pfrm->hash, copy, NULL);
  return NULL;
}

/************************************************************************//**
  Returns the position for the unit. Creates it if needed. Returns NULL if
  'target_tile' is unreachable.
****************************************************************************/
static inline const struct pf_position *
pf_reverse_map_unit_pos(struct pf_reverse_map *pfrm,
                        const struct unit *punit)
{
  struct pf_parameter *param = &pfrm->template;

  /* Fill parameter. */
  param->start_tile = unit_tile(punit);
  param->move_rate = unit_move_rate(punit);
  /* Do not consider punit->moves_left, because this value is usually
   * not restored when calling this function. Let's assume the unit will
   * have its whole move rate. */
  param->moves_left_initially = param->move_rate;
  param->utype = unit_type_get(punit);
  return pf_reverse_map_pos(pfrm, param);
}

/************************************************************************//**
  Returns the position for the unit type. Creates it if needed. Returns NULL
  if 'target_tile' is unreachable.
****************************************************************************/
static inline const struct pf_position *
pf_reverse_map_utype_pos(struct pf_reverse_map *pfrm,
                         const struct unit_type *punittype,
                         struct tile *ptile)
{
  struct pf_parameter *param = &pfrm->template;
  const struct player *pplayer = param->owner;
  int veteran_level = get_unittype_bonus(pplayer, ptile, punittype,
                                         EFT_VETERAN_BUILD);

  if (veteran_level >= utype_veteran_levels(punittype)) {
    veteran_level = utype_veteran_levels(punittype) - 1;
  }

  /* Fill parameter. */
  param->start_tile = ptile;
  param->move_rate = utype_move_rate(punittype, ptile, pplayer,
                                     veteran_level, punittype->hp);
  param->moves_left_initially = param->move_rate;
  param->utype = punittype;
  return pf_reverse_map_pos(pfrm, param);
}

/************************************************************************//**
  Get the move costs that a unit type needs to reach the start tile. Returns
  PF_IMPOSSIBLE_MC if the tile is unreachable.
****************************************************************************/
int pf_reverse_map_utype_move_cost(struct pf_reverse_map *pfrm,
                                   const struct unit_type *punittype,
                                   struct tile *ptile)
{
  const struct pf_position *pos = pf_reverse_map_utype_pos(pfrm, punittype,
                                                           ptile);

  return (pos != NULL ? pos->total_MC : PF_IMPOSSIBLE_MC);
}

/************************************************************************//**
  Get the move costs that a unit needs to reach the start tile. Returns
  PF_IMPOSSIBLE_MC if the tile is unreachable.
****************************************************************************/
int pf_reverse_map_unit_move_cost(struct pf_reverse_map *pfrm,
                                  const struct unit *punit)
{
  const struct pf_position *pos = pf_reverse_map_unit_pos(pfrm, punit);

  return (pos != NULL ? pos->total_MC : PF_IMPOSSIBLE_MC);
}

/************************************************************************//**
  Fill the position. Return TRUE if the tile is reachable.
****************************************************************************/
bool pf_reverse_map_utype_position(struct pf_reverse_map *pfrm,
                                   const struct unit_type *punittype,
                                   struct tile *ptile,
                                   struct pf_position *pos)
{
  const struct pf_position *mypos = pf_reverse_map_utype_pos(pfrm, punittype,
                                                             ptile);

  if (mypos != NULL) {
    *pos = *mypos;
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  Fill the position. Return TRUE if the tile is reachable.
****************************************************************************/
bool pf_reverse_map_unit_position(struct pf_reverse_map *pfrm,
                                  const struct unit *punit,
                                  struct pf_position *pos)
{
  const struct pf_position *mypos = pf_reverse_map_unit_pos(pfrm, punit);

  if (mypos != NULL) {
    *pos = *mypos;
    return TRUE;
  } else {
    return FALSE;
  }
}
