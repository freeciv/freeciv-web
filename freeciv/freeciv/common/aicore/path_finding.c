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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "pqueue.h"

/* common */
#include "game.h"
#include "map.h"
#include "movement.h"

#include "path_finding.h"

/* For explanations on how to use this module, see path_finding.h */

#define INITIAL_QUEUE_SIZE 100

#ifdef DEBUG
#define PF_DEBUG
#endif

/* Since speed is quite important to us and alloccation of large arrays is
 * slow, we try to pack info in the smallest types possible */
typedef short mapindex_t;
typedef unsigned char utiny_t;

/* ===================== Internal structures ===================== */

#ifdef PF_DEBUG
/* The mode we use the pf_map. Used for cast converion checks. */
enum pf_mode {
  PF_NORMAL = 1,		/* Usual goto */
  PF_DANGER,			/* Goto with dangerous positions */
  PF_FUEL			/* Goto with fueled units */
};
#endif /* PF_DEBUG */

enum pf_node_status {
  NS_UNINIT = 0,		/* memory is calloced, hence zero 
				 * means uninitialised */
  NS_INIT,                      /* node initialized, but no path found yet */
  NS_NEW,			/* the optimal route isn't found yet */
  NS_WAITING,			/* the optimal route is found,
				 * considering waiting */
  NS_PROCESSED			/* the optimal route is found */
};

enum pf_zoc_type {
  ZOC_NO,			/* No ZoC */
  ZOC_ALLIED,			/* Allied ZoC */
  ZOC_MINE			/* My ZoC */
};

/* Abstract base class for pf_normal_map,
 * pf_danger_map, and pf_fuel_map. */
struct pf_map {
#ifdef PF_DEBUG
  /* The mode of the map, for conversion checking. */
  enum pf_mode mode;
#endif /* PF_DEBUG */

  /* "Virtual" function table. See the comment header
   * for each pf_<function name> for details. */
  void (*destroy)(struct pf_map *pfm); /* Destructor. */
  int (*get_move_cost)(struct pf_map *pfm, struct tile *ptile);
  struct pf_path *(*get_path)(struct pf_map *pfm, struct tile *ptile);
  bool (*get_position)(struct pf_map *pfm, struct tile *ptile,
                       struct pf_position *pos);
  bool (*iterate)(struct pf_map *pfm);

  /* Private data. */
  struct tile *tile;          /* The current position. */
  struct pf_parameter params; /* Initial parameters. */
};

/* Down-cast macro. */
#define PF_MAP(pfm) ((struct pf_map *) (pfm))


/* ==================== Common functions ====================== */

/****************************************************************************
  Return the number of "moves" started with.

  This is different from the moves_left_initially because of fuel.  For units
  with fuel > 1 all moves on the same tank of fuel are considered to be one
  turn.  Thus the rest of the PF code doesn't actually know that the unit
  has fuel, it just thinks it has that many more MP.
****************************************************************************/
static int get_moves_left_initially(const struct pf_parameter *param)
{
  return (param->moves_left_initially
          + (param->fuel_left_initially - 1) * param->move_rate);
}

/*************************************************************************
  Return the "move rate".

  This is different from the parameter's move_rate because of fuel. For
  units with fuel > 1 all moves on the same tank of fuel are considered
  to be one turn. Thus the rest of the PF code doesn't actually know that
  the unit has fuel, it just thinks it has that many more MP.
*************************************************************************/
static int get_move_rate(const struct pf_parameter *param)
{
  return param->move_rate * param->fuel;
}

/********************************************************************
  Number of turns required to reach node.
  See comment in pf_map_new() about the usage of
  get_moves_left_initially().
********************************************************************/
static int get_turn(const struct pf_parameter *param, int cost)
{
  if (get_move_rate(param) <= 0) {
    /* This unit cannot move by itself. */
    return FC_INFINITY;
  }

  /* Negative cost can happen when a unit initially has more MP than its
   * move-rate (due to wonders transfer etc).  Although this may be a bug, 
   * we'd better be ready.
   *
   * Note that cost==0 corresponds to the current turn with full MP. */
  return (cost < 0 ? 0 : cost / get_move_rate(param));
}

/********************************************************************
  Moves left after node is reached
  See comment in required() about the usage of
  get_moves_left_initially().
********************************************************************/
static int get_moves_left(const struct pf_parameter *param, int cost)
{
  int move_rate = get_move_rate(param);

  if (move_rate <= 0) {
    /* This unit never have moves left. */
    return 0;
  }

  /* Cost may be negative; see get_turn(). */
  return (cost < 0 ? move_rate - cost
          : (move_rate - (cost % move_rate)));
}

/***************************************************************************
  Obtain cost-of-path from pure cost and extra cost
***************************************************************************/
static int get_total_CC(const struct pf_parameter *param, int cost,
                        int extra)
{
  return PF_TURN_FACTOR * cost + extra * get_move_rate(param);
}

/***************************************************************************
  Take a position previously filled out (as by fill_position) and
  "finalize" it by reversing all fuel multipliers.

  See get_moves_left_initially and get_move_rate.
***************************************************************************/
static void finalize_position(const struct pf_parameter *param,
                              struct pf_position *pos)
{
  int move_rate;

  if (param->turn_mode == TM_BEST_TIME
      || param->turn_mode == TM_WORST_TIME) {
    pos->turn *= param->fuel;
    move_rate = get_move_rate(param);
    if (move_rate > 0) {
      pos->turn += ((get_move_rate(param) - pos->moves_left)
                    / param->move_rate);

      /* We add 1 because a fuel of 1 means "no" fuel left; e.g. fuel
       * ranges from [1,ut->fuel] not from [0,ut->fuel) as one may think. */
      pos->fuel_left = pos->moves_left / param->move_rate + 1;

      pos->moves_left %= param->move_rate;
    } else {
      /* This unit cannot move by itself. */
      pos->turn = same_pos(pos->tile, param->start_tile) ? 0 : FC_INFINITY;
      pos->fuel_left = 0;
    }
  }
}



/* ============ Specific pf_normal_* mode structures ============= */

/* Some comments on implementation:
 * 1. cost (aka total_MC) is sum of MCs altered to fit to the turn_mode
 *    see adjust_cost
 * 2. dir_to_here is for backtracking along the tree of shortest paths
 * 3. node_known_type, behavior, zoc_number and extra_tile are all cached
 *    values.
 * It is possible to shove them into a separate array which is allocated
 * only if a corresponding option in the parameter is set.  A less drastic
 * measure would be to pack the first three into one byte.  All of there are
 * time-saving measures and should be tested once we get an established
 * user-base. */
struct pf_normal_node {
  int cost;			/* total_MC */
  int extra_cost;		/* total_EC */
  utiny_t dir_to_here;		/* direction from which we came */

  /* Cached values */
  utiny_t status;		/* (enum pf_node_status really) */
  int extra_tile;		/* EC */
  utiny_t node_known_type;
  utiny_t behavior;
  utiny_t zoc_number;		/* (enum pf_zoc_type really) */
  bool can_invade;
};

/* Derived structure of struct pf_map. */
struct pf_normal_map {
  struct pf_map base_map;   /* Base structure, must be the first! */

  struct pqueue *queue;     /* Queue of nodes we have reached but not
                             * processed yet (NS_NEW), sorted by their
                             * total_CC. */
  struct pf_normal_node *lattice; /* Lattice of nodes. */
};

/* Up-cast macro. */
#ifdef PF_DEBUG
static inline struct pf_normal_map *
pf_normal_map_check(struct pf_map *pfm, const char *file, int line)
{
  if (!pfm || pfm->mode != PF_NORMAL) {
    real_die(file, line, "Wrong pf_map to pf_normal_map conversion");
  }
  return (struct pf_normal_map *) pfm;
}
#define PF_NORMAL_MAP(pfm) pf_normal_map_check(pfm, __FILE__, __LINE__)
#else
#define PF_NORMAL_MAP(pfm) ((struct pf_normal_map *) (pfm))
#endif /* PF_DEBUG */

/* =============  Specific pf_normal_* mode functions =============== */

/******************************************************************
  Calculates cached values of the target node: 
  node_known_type and zoc
******************************************************************/
static void pf_normal_node_init(struct pf_normal_map *pfnm,
                                struct pf_normal_node *node,
                                struct tile *ptile)
{
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pfnm));

  /* Establish the "known" status of node */
  if (params->omniscience) {
    node->node_known_type = TILE_KNOWN_SEEN;
  } else {
    node->node_known_type = tile_get_known(ptile, params->owner);
  }

  /* Establish the tile behavior */
  if (params->get_TB) {
    node->behavior = params->get_TB(ptile, node->node_known_type, params);
  } else {
    /* The default */
    node->behavior = TB_NORMAL;
  }

  if (params->get_zoc) {
    struct city *pcity = tile_city(ptile);
    struct terrain *pterrain = tile_terrain(ptile);
    bool my_zoc = (NULL != pcity || pterrain == T_UNKNOWN
                   || terrain_has_flag(pterrain, TER_OCEANIC)
                   || params->get_zoc(params->owner, ptile));
    /* ZoC rules cannot prevent us from moving into/attacking an occupied 
     * tile.  Other rules can, but we don't care about them here. */ 
    bool occupied = (unit_list_size(ptile->units) > 0 || NULL != pcity);

    /* ZOC_MINE means can move unrestricted from/into it,
     * ZOC_ALLIED means can move unrestricted into it,
     * but not necessarily from it */
    node->zoc_number = (my_zoc ? ZOC_MINE
                        : (occupied ? ZOC_ALLIED : ZOC_NO));
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->zoc_number = 0;
#endif
  }

  /* Evaluate the extra cost of the destination */
  if (params->get_EC) {
    node->extra_tile = params->get_EC(ptile, node->node_known_type, params);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so  should be already set to 0. */
    node->extra_tile = 0;
#endif
  }

  if (params->can_invade_tile) {
    node->can_invade = params->can_invade_tile(params->owner, ptile);
  } else {
    node->can_invade = TRUE;
  }

  node->status = NS_INIT;
}

/****************************************************************************
  Fill in the position which must be discovered already. A helper
  for *_get_position functions.  This also "finalizes" the position.
****************************************************************************/
static void pf_normal_map_fill_position(const struct pf_normal_map *pfnm,
                                        struct tile *ptile,
                                        struct pf_position *pos)
{
  mapindex_t index = tile_index(ptile);
  struct pf_normal_node *node = &pfnm->lattice[index];
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pfnm));

#ifdef PF_DEBUG
  if (node->status != NS_PROCESSED
      && !same_pos(ptile, PF_MAP(pfnm)->tile)) {
    die("pf_normal_map_fill_position to an unreached destination");
    return;
  }
#endif /* PF_DEBUG */

  pos->tile = ptile;
  pos->total_EC = node->extra_cost;
  pos->total_MC = (node->cost - get_move_rate(params)
                   + get_moves_left_initially(params));
  if (params->turn_mode == TM_BEST_TIME
      || params->turn_mode == TM_WORST_TIME) {
    pos->turn = get_turn(params, node->cost);
    pos->moves_left = get_moves_left(params, node->cost);
  } else if (params->turn_mode == TM_NONE
             || params->turn_mode == TM_CAPPED) {
    pos->turn = -1;
    pos->moves_left = -1;
    pos->fuel_left = -1;
  } else {
    die("unknown TC");
  }

  pos->dir_to_here = node->dir_to_here;
  /* This field does not apply */
  pos->dir_to_next_pos = -1;

  finalize_position(params, pos);
}

/*******************************************************************
  Read off the path to the node dest_tile, which must already be
  discovered.  A helper for *_get_path functions.
*******************************************************************/
static struct pf_path *
pf_normal_map_construct_path(const struct pf_normal_map *pfnm,
                             struct tile *dest_tile)
{
  struct pf_normal_node *node = &pfnm->lattice[tile_index(dest_tile)];
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pfnm));
  enum direction8 dir_next = -1;
  struct pf_path *path;
  struct tile *ptile;
  int i;

#ifdef PF_DEBUG
  if (node->status != NS_PROCESSED
      && !same_pos(dest_tile, PF_MAP(pfnm)->tile)) {
    die("construct_path to an unreached destination");
    return NULL;
  }
#endif /* PF_DEBUG */

  ptile = dest_tile;
  path = fc_malloc(sizeof(*path));

  /* 1: Count the number of steps to get here.
   * To do it, backtrack until we hit the starting point */
  for (i = 0; ; i++) {
    if (same_pos(ptile, params->start_tile)) {
      /* Ah-ha, reached the starting point! */
      break;
    }

    ptile = mapstep(ptile, DIR_REVERSE(node->dir_to_here));
    node = &pfnm->lattice[tile_index(ptile)];
  }

  /* 2: Allocate the memory */
  path->length = i + 1;
  path->positions = fc_malloc((i + 1) * sizeof(*(path->positions)));

  /* 3: Backtrack again and fill the positions this time */
  ptile = dest_tile;
  node = &pfnm->lattice[tile_index(ptile)];

  for (; i >= 0; i--) {
    pf_normal_map_fill_position(pfnm, ptile, &path->positions[i]);
    /* fill_position doesn't set direction */
    path->positions[i].dir_to_next_pos = dir_next;

    dir_next = node->dir_to_here;

    if (i > 0) {
      /* Step further back, if we haven't finished yet */
      ptile = mapstep(ptile, DIR_REVERSE(dir_next));
      node = &pfnm->lattice[tile_index(ptile)];
    }
  }

  return path;
}

/*********************************************************************
  Adjust MC to reflect the turn mode and the move_rate.
*********************************************************************/
static int pf_normal_map_adjust_cost(const struct pf_normal_map *pfnm,
                                     int cost)
{
  const struct pf_parameter *params;
  const struct pf_normal_node *node;
  int moves_left;

  assert(cost >= 0);

  params = pf_map_get_parameter(PF_MAP(pfnm));
  node = &pfnm->lattice[tile_index(PF_MAP(pfnm)->tile)];

  switch (params->turn_mode) {
  case TM_NONE:
    break;
  case TM_CAPPED:
    cost = MIN(cost, get_move_rate(params));
    break;
  case TM_WORST_TIME:
    cost = MIN(cost, get_move_rate(params));
    moves_left = get_moves_left(params, node->cost);
    if (cost > moves_left) {
      cost += moves_left;
    }
    break;
  case TM_BEST_TIME:
    moves_left = get_moves_left(params, node->cost);
    if (cost > moves_left) {
      cost = moves_left;
    }
    break;
  default:
    die("unknown TM");
    break;
  }
  return cost;
}

/**************************************************************************
  Bare-bones PF iterator.  All Freeciv rules logic is hidden in get_costs
  callback (compare to pf_normal_map_iterate function).
  Plan: 1. Process previous position
        2. Get new nearest position and return it
**************************************************************************/
static bool pf_jumbo_map_iterate(struct pf_map *pfm)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);
  struct tile *tile = pfm->tile;
  mapindex_t index = tile_index(tile);
  struct pf_normal_node *node = &pfnm->lattice[index];
  const struct pf_parameter *params = pf_map_get_parameter(pfm);

  node->status = NS_PROCESSED;

  /* Processing Stage */
  /* The previous position is contained in {x,y} fields of map */

  adjc_dir_iterate(tile, tile1, dir) {
    mapindex_t index1 = tile_index(tile1);
    struct pf_normal_node *node1 = &pfnm->lattice[index1];
    int priority;

    if (node1->status == NS_PROCESSED) {
      /* This gives 15% speedup */
      continue;
    }

    if (node1->status <= NS_INIT) {
      node1->cost = PF_IMPOSSIBLE_MC;
    }

    /* User-supplied callback get_costs takes care of everything (ZOC, 
     * known, costs etc).  See explanations in path_finding.h */
    priority = params->get_costs(tile, dir, tile1, node->cost,
                                 node->extra_cost, &node1->cost,
                                 &node1->extra_cost, params);
    if (priority >= 0) {
      /* We found a better route to xy1, record it 
       * (the costs are recorded already) */
      node1->status = NS_NEW;
      node1->dir_to_here = dir;
      pq_insert(pfnm->queue, index1, -priority);
    }
  } adjc_dir_iterate_end;

  /* Get the next nearest node */
  for (;;) {
    bool removed = pq_remove(pfnm->queue, &index);

    if (!removed) {
      return FALSE;
    }
    if (pfnm->lattice[index].status == NS_NEW) {
      break;
    }
    /* If the node has already been processed, get the next one. */
  }

  pfm->tile = index_to_tile(index);

  return TRUE;
}

/*****************************************************************
  Primary method for iterative path-finding.
  Plan: 1. Process previous position
        2. Get new nearest position and return it
*****************************************************************/
static bool pf_normal_map_iterate(struct pf_map *pfm)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);
  struct tile *tile = pfm->tile;
  mapindex_t index = tile_index(tile);
  struct pf_normal_node *node = &pfnm->lattice[index];
  const struct pf_parameter *params = pf_map_get_parameter(pfm);
  int cost_of_path;

  node->status = NS_PROCESSED;

  /* There is no exit from DONT_LEAVE tiles! */
  if (node->behavior != TB_DONT_LEAVE) {

    /* Processing Stage */
    /* The previous position is contained in {x,y} fields of map */

    adjc_dir_iterate(tile, tile1, dir) {
      mapindex_t index1 = tile_index(tile1);
      struct pf_normal_node *node1 = &pfnm->lattice[index1];
      int cost;
      int extra = 0;

      if (node1->status == NS_PROCESSED) {
        /* This gives 15% speedup */
        continue;
      }

      if (node1->status == NS_UNINIT) {
        pf_normal_node_init(pfnm, node1, tile1);
      }

      /* Can we enter this tile at all? */
      if (!node1->can_invade || node1->behavior == TB_IGNORE) {
        continue;
      }

      /* Is the move ZOC-ok? */
      if (params->get_zoc
          && !(node->zoc_number == ZOC_MINE
               || node1->zoc_number != ZOC_NO)) {
        continue;
      }

      /* Evaluate the cost of the move */
      if (node1->node_known_type == TILE_UNKNOWN) {
        cost = params->unknown_MC;
      } else {
        cost = params->get_MC(tile, dir, tile1, params);
      }
      if (cost == PF_IMPOSSIBLE_MC) {
        continue;
      }
      cost = pf_normal_map_adjust_cost(pfnm, cost);
      if (cost == PF_IMPOSSIBLE_MC) {
        continue;
      }

      /* Total cost at tile1.  Cost may be negative; see get_turn(). */
      cost += node->cost;

      /* Evaluate the extra cost if it's relevant */
      if (params->get_EC) {
        extra = node->extra_cost;
        /* Add the cached value */
        extra += node1->extra_tile;
      }

      /* Update costs and add to queue, if we found a better route to xy1. */
      cost_of_path = get_total_CC(params, cost, extra);

      if (node1->status == NS_INIT
          || cost_of_path < get_total_CC(params, node1->cost,
                                         node1->extra_cost)) {
        node1->status = NS_NEW;
        node1->extra_cost = extra;
        node1->cost = cost;
        node1->dir_to_here = dir;
        pq_insert(pfnm->queue, index1, -cost_of_path);
      }
    } adjc_dir_iterate_end;
  }

  /* Get the next nearest node */
  for (;;) {
    bool removed = pq_remove(pfnm->queue, &index);

    if (!removed) {
      return FALSE;
    }
    if (pfnm->lattice[index].status == NS_NEW) {
      /* Discard if this node has already been processed */
      break;
    }
  }

  pfm->tile = index_to_tile(index);

  return TRUE;
}

/************************************************************************
  Iterate the map until ptile is reached.
************************************************************************/
static bool pf_normal_map_iterate_until(struct pf_normal_map *pfnm,
					struct tile *ptile)
{
  struct pf_map *pfm = PF_MAP(pfnm);
  const struct pf_normal_node *node = &pfnm->lattice[tile_index(ptile)];

  while (node->status != NS_PROCESSED
	 && !same_pos(ptile, pfm->tile)) {
    if (!pf_map_iterate(pfm)) {
      return FALSE;
    }
  }

  return TRUE;
}

/************************************************************************
  Return the move cost at ptile.  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
  This function returns PF_IMPOSSIBLE_MC on unreachable positions.
************************************************************************/
static int pf_normal_map_get_move_cost(struct pf_map *pfm,
				       struct tile *ptile)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  if (pf_normal_map_iterate_until(pfnm, ptile)) {
    return (pfnm->lattice[tile_index(ptile)].cost
	    - get_move_rate(pf_map_get_parameter(pfm))
	    + get_moves_left_initially(pf_map_get_parameter(pfm)));
  } else {
    return PF_IMPOSSIBLE_MC;
  }
}

/************************************************************************
  Return the path to ptile.  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
************************************************************************/
static struct pf_path *pf_normal_map_get_path(struct pf_map *pfm,
                                              struct tile *ptile)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  if (pf_normal_map_iterate_until(pfnm, ptile)) {
    return pf_normal_map_construct_path(pfnm, ptile);
  } else {
    return NULL;
  }
}

/*******************************************************************
  Get info about position at ptile and put it in pos.  If ptile
  has not been reached yet, iterate the map until we reach it.
  Should _always_ check the return value, forthe position might be
  unreachable.
*******************************************************************/
static bool pf_normal_map_get_position(struct pf_map *pfm,
                                       struct tile *ptile,
                                       struct pf_position *pos)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  if (pf_normal_map_iterate_until(pfnm, ptile)) {
    pf_normal_map_fill_position(pfnm, ptile, pos);
    return TRUE;
  } else {
    return FALSE;
  }
}

/*********************************************************************
  After usage the map must be destroyed.
*********************************************************************/
static void pf_normal_map_destroy(struct pf_map *pfm)
{
  struct pf_normal_map *pfnm = PF_NORMAL_MAP(pfm);

  free(pfnm->lattice);
  pq_destroy(pfnm->queue);
  free(pfnm);
}

/***************************************************************************
  Create a pf_normal_map. NB: The "constructor" returns a pf_map.
***************************************************************************/
static struct pf_map *pf_normal_map_new(const struct pf_parameter *parameter)
{
  struct pf_normal_map *pfnm;
  struct pf_map *base_map;
  struct pf_parameter *params;
  struct pf_normal_node *node;

  pfnm = fc_calloc(1, sizeof(struct pf_normal_map));
  base_map = &pfnm->base_map;
  params = &base_map->params;
#ifdef PF_DEBUG
  /* Set the mode, used for cast check */
  base_map->mode = PF_NORMAL;
#endif /* PF_DEBUG */

  /* Alloc the map */
  pfnm->lattice = fc_calloc(MAP_INDEX_SIZE, sizeof(struct pf_normal_node));
  pfnm->queue = pq_create(INITIAL_QUEUE_SIZE);

  /* MC callback must be set */
  assert(parameter->get_MC != NULL);

  /* Copy parameters */
  *params = *parameter;

  /* Initialize virtual function table. */
  base_map->destroy = pf_normal_map_destroy;
  base_map->get_move_cost = pf_normal_map_get_move_cost;
  base_map->get_path = pf_normal_map_get_path;
  base_map->get_position = pf_normal_map_get_position;
  if (params->get_costs) {
    base_map->iterate = pf_jumbo_map_iterate;
  } else {
    base_map->iterate = pf_normal_map_iterate;
  }

  /* Initialise starting coordinates */
  base_map->tile = params->start_tile;

  /* Initialise starting node */
  node = &pfnm->lattice[tile_index(params->start_tile)];
  pf_normal_node_init(pfnm, node, params->start_tile);
  /* This makes calculations of turn/moves_left more convenient, but we 
   * need to subtract this value before we return cost to the user.  Note
   * that cost may be negative if moves_left_initially > move_rate
   * (see get_turn()). */
  node->cost = get_move_rate(params) - get_moves_left_initially(params);
  node->extra_cost = 0;
  node->dir_to_here = -1;

  return PF_MAP(pfnm);
}



/* ============ Specific pf_danger_* mode structures ============= */

/* Some comments on implementation:
 * 1. cost (aka total_MC) is sum of MCs altered to fit to the turn_mode
 * see adjust_cost
 * 2. dir_to_here is for backtracking along the tree of shortest paths
 * 3. node_known_type, behavior, zoc_number, extra_tile and waited
 * are all cached values.
 * It is possible to shove them into a separate array which is allocated
 * only if a corresponding option in the parameter is set.  A less drastic
 * measure would be to pack the first three into one byte.  All of there
 * are time-saving measures and should be tested once we get an established
 * user-base. */
struct pf_danger_node {
  int cost;			/* total_MC */
  int extra_cost;		/* total_EC */
  utiny_t dir_to_here;		/* direction from which we came */

  /* Cached values */
  utiny_t status;		/* (enum pf_node_status really) */
  int extra_tile;		/* EC */
  utiny_t node_known_type;
  utiny_t behavior;
  utiny_t zoc_number;		/* (enum pf_zoc_type really) */
  bool can_invade;
  bool is_dangerous;
  bool waited;			/* TRUE if waited to get here */

  struct pf_danger_pos {
    enum direction8 dir;
    int cost;
    int extra_cost;
  } *danger_segment;  	        /* Segment leading across the danger area
				 * back to the nearest safe node:
				 * need to remeber costs and stuff */
};

/* Derived structure of struct pf_map. */
struct pf_danger_map {
  struct pf_map base_map;       /* Base structure, must be the first! */

  struct pqueue *queue;         /* Queue of nodes we have reached but not
                                 * processed yet (NS_NEW), sorted by their
                                 * total_CC */
  struct pqueue *danger_queue;  /* Dangerous positions go there */
  struct pf_danger_node *lattice; /* Lattice of nodes */
};

/* Up-cast macro. */
#ifdef PF_DEBUG
static inline struct pf_danger_map *
pf_danger_map_check(struct pf_map *pfm, const char *file, int line)
{
  if (!pfm || pfm->mode != PF_DANGER) {
    real_die(file, line, "Wrong pf_map to pf_danger_map conversion");
  }
  return (struct pf_danger_map *) pfm;
}
#define PF_DANGER_MAP(pfm) pf_danger_map_check(pfm, __FILE__, __LINE__)
#else
#define PF_DANGER_MAP(pfm) ((struct pf_danger_map *) (pfm))
#endif /* PF_DEBUG */

/* ============  Specific pf_danger_* mode functions ============== */

/**********************************************************************
  Calculates cached values of the target node: node_known_type and zoc.
**********************************************************************/
static void pf_danger_node_init(struct pf_danger_map *pfdm,
                                struct pf_danger_node *node,
                                struct tile *ptile)
{
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pfdm));

  /* Establish the "known" status of node */
  if (params->omniscience) {
    node->node_known_type = TILE_KNOWN_SEEN;
  } else {
    node->node_known_type = tile_get_known(ptile, params->owner);
  }

  /* Establish the tile behavior */
  if (params->get_TB) {
    node->behavior = params->get_TB(ptile, node->node_known_type, params);
  } else {
    /* The default */
    node->behavior = TB_NORMAL;
  }

  if (params->get_zoc) {
    struct city *pcity = tile_city(ptile);
    struct terrain *pterrain = tile_terrain(ptile);
    bool my_zoc = (NULL != pcity || pterrain == T_UNKNOWN
                   || terrain_has_flag(pterrain, TER_OCEANIC)
                   || params->get_zoc(params->owner, ptile));
    /* ZoC rules cannot prevent us from moving into/attacking an occupied
     * tile.  Other rules can, but we don't care about them here. */
    bool occupied = (unit_list_size(ptile->units) > 0 || NULL != pcity);

    /* ZOC_MINE means can move unrestricted from/into it,
     * ZOC_ALLIED means can move unrestricted into it,
     * but not necessarily from it */
    node->zoc_number = (my_zoc ? ZOC_MINE
                        : (occupied ? ZOC_ALLIED : ZOC_NO));
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->zoc_number = 0;
#endif
  }

  /* Evaluate the extra cost of the destination */
  if (params->get_EC) {
    node->extra_tile = params->get_EC(ptile, node->node_known_type, params);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->extra_tile = 0;
#endif
  }

  if (params->can_invade_tile) {
    node->can_invade = params->can_invade_tile(params->owner, ptile);
  } else {
    node->can_invade = TRUE;
  }

  node->is_dangerous =
    params->is_pos_dangerous(ptile, node->node_known_type, params);

  /* waited is set to zero by fc_calloc. */

  node->status = NS_INIT;
}

/****************************************************************************
  Fill in the position which must be discovered already. A helper 
  for *_get_position functions.  This also "finalizes" the position.
****************************************************************************/
static void pf_danger_map_fill_position(const struct pf_danger_map *pfdm,
                                        struct tile *ptile,
                                        struct pf_position *pos)
{
  mapindex_t index = tile_index(ptile);
  struct pf_danger_node *node = &pfdm->lattice[index];
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pfdm));

#ifdef PF_DEBUG
  if (node->status != NS_PROCESSED
      && !same_pos(ptile, PF_MAP(pfdm)->tile)) {
    die("pf_normal_map_fill_position to an unreached destination");
    return;
  }
#endif /* PF_DEBUG */

  pos->tile = ptile;
  pos->total_EC = node->extra_cost;
  pos->total_MC = (node->cost - get_move_rate(params)
                   + get_moves_left_initially(params));
  if (params->turn_mode == TM_BEST_TIME
      || params->turn_mode == TM_WORST_TIME) {
    pos->turn = get_turn(params, node->cost);
    pos->moves_left = get_moves_left(params, node->cost);
  } else if (params->turn_mode == TM_NONE
             || params->turn_mode == TM_CAPPED) {
    pos->turn = -1;
    pos->moves_left = -1;
    pos->fuel_left = -1;
  } else {
    die("unknown TC");
  }

  pos->dir_to_here = node->dir_to_here;
  /* This field does not apply */
  pos->dir_to_next_pos = -1;

  finalize_position(params, pos);
}

/*******************************************************************
  Read off the path to the node ptile, but with danger
  NB: will only find paths to safe tiles!
*******************************************************************/
static struct pf_path *
pf_danger_map_construct_path(const struct pf_danger_map *pfdm,
                             struct tile *ptile)
{
  struct pf_path *path = fc_malloc(sizeof(*path));
  enum direction8 dir_next = -1;
  struct pf_danger_pos *danger_seg = NULL;
  bool waited = FALSE;
  struct pf_danger_node *node = &pfdm->lattice[tile_index(ptile)];
  int length = 1;
  struct tile *iter_tile = ptile;
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pfdm));
  struct pf_position *pos;
  int i;

  if (params->turn_mode != TM_BEST_TIME
      && params->turn_mode != TM_WORST_TIME) {
    die("illegal TM in path-finding with danger");
    return NULL;
  }

  /* First iterate to find path length */
  while (!same_pos(iter_tile, params->start_tile)) {

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
       * if it's NULL, we are not on one so danger_seg should be NULL */
      danger_seg = node->danger_segment;
    } else {
      /* We are in a danger segment */
      dir_next = danger_seg->dir;
      danger_seg++;
    }

    /* Step backward */
    iter_tile = mapstep(iter_tile, DIR_REVERSE(dir_next));
    node = &pfdm->lattice[tile_index(iter_tile)];
  }

  /* Allocate memory for path */
  path->positions = fc_malloc(length * sizeof(struct pf_position));
  path->length = length;

  /* Reset variables for main iteration */
  iter_tile = ptile;
  node = &pfdm->lattice[tile_index(ptile)];
  danger_seg = NULL;
  waited = FALSE;

  for (i = length - 1; i >= 0; i--) {
    bool old_waited = FALSE;

    /* 1: Deal with waiting */
    if (!node->is_dangerous) {
      if (waited) {
        /* Waited at _this_ tile, need to record it twice in the
         * path. Here we record our state _after_ waiting (e.g.
         * full move points). */
        pos = &path->positions[i];
        pos->tile = iter_tile;
        pos->total_EC = node->extra_cost;
        pos->turn = get_turn(params, node->cost) + 1;
        pos->moves_left = get_move_rate(params);
        pos->total_MC = ((path->positions[i].turn - 1) * params->move_rate
                         + params->moves_left_initially);
        pos->dir_to_next_pos = dir_next;
        finalize_position(params, pos);
        /* Set old_waited so that we record -1 as a direction at
         * the step we were going to wait. */
        old_waited = TRUE;
        i--;
      }
      /* Update "waited" (d_node->waited means "waited to get here"). */
      waited = node->waited;
    }

    /* 2: Fill the current position */
    pos = &path->positions[i];
    pos->tile = iter_tile;
    if (!node->is_dangerous || !danger_seg) {
      pos->total_MC = node->cost;
      pos->total_EC = node->extra_cost;
    } else {
      /* When on dangerous tiles, must have a valid danger segment. */
      assert(danger_seg != NULL);
      pos->total_MC = danger_seg->cost;
      pos->total_EC = danger_seg->extra_cost;
    }
    pos->turn = get_turn(params, pos->total_MC);
    pos->moves_left = get_moves_left(params, pos->total_MC);
    pos->total_MC -= (get_move_rate(params)
                      - get_moves_left_initially(params));
    pos->dir_to_next_pos = (old_waited ? -1 : dir_next);
    finalize_position(params, pos);

    /* 3: Check if we finished */
    if (i == 0) {
      /* We should be back at the start now! */
      assert(same_pos(iter_tile, params->start_tile));
      return path;
    }

    /* 4: Calculate the next direction */
    if (!node->is_dangerous || !danger_seg) {
      /* We are in the normal node and dir_to_here field is valid. */
      dir_next = node->dir_to_here;
      /* d_node->danger_segment is the indicator of what lies ahead.
       * If it's non-NULL, we are entering a danger segment,
       * If it's NULL, we are not on one so danger_seg should be NULL. */
      danger_seg = node->danger_segment;
    } else {
      /* We are in a danger segment */
      dir_next = danger_seg->dir;
      danger_seg++;
    }

    /* 5: Step further back */
    iter_tile = mapstep(iter_tile, DIR_REVERSE(dir_next));
    node = &pfdm->lattice[tile_index(iter_tile)];
  }

  die("pf_danger_map_construct_path: cannot get to the starting point!");
  return NULL;
}

/***********************************************************************
  Creating path segment going back from d_node1 to a safe tile.
***********************************************************************/
static void pf_danger_map_create_segment(struct pf_danger_map *pfdm,
                                         struct pf_danger_node *node1)
{
  int i;
  struct tile *ptile = PF_MAP(pfdm)->tile;
  struct pf_danger_node *node = &pfdm->lattice[tile_index(ptile)];
  struct pf_danger_pos *pos;
  int length = 0;

  /* Allocating memory */
  if (node1->danger_segment) {
    freelog(LOG_ERROR, "Possible memory leak in "
            "pf_danger_map_create_segment().");
  }

  /* First iteration for determining segment length */
  while (node->is_dangerous && is_valid_dir(node->dir_to_here)) {
    length++;
    ptile = mapstep(ptile, DIR_REVERSE(node->dir_to_here));
    node = &pfdm->lattice[tile_index(ptile)];
  }

  /* Allocate memory for segment */
  node1->danger_segment = fc_malloc(length * sizeof(struct pf_danger_pos));

  /* Reset tile and node pointers for main iteration */
  ptile = PF_MAP(pfdm)->tile;
  node = &pfdm->lattice[tile_index(ptile)];

  /* Now fill the positions */
  for (i = 0, pos = node1->danger_segment; i < length; i++, pos++) {
    /* Record the direction */
    pos->dir = node->dir_to_here;
    pos->cost = node->cost;
    pos->extra_cost = node->extra_cost;
    if (i == length - 1) {
      /* The last dangerous node contains "waiting" info */
      node1->waited = node->waited;
    }

    /* Step further down the tree */
    ptile = mapstep(ptile, DIR_REVERSE(node->dir_to_here));
    node = &pfdm->lattice[tile_index(ptile)];
  }

  /* Make sure we reached a safe node or the start point */
  assert(!node->is_dangerous || !is_valid_dir(node->dir_to_here));
}

/**********************************************************************
  Adjust cost taking into account possibility of making the move.
**********************************************************************/
static int pf_danger_map_adjust_cost(const struct pf_parameter *params,
                                     int cost, bool to_danger,
                                     int moves_left)
{
  if (cost == PF_IMPOSSIBLE_MC) {
    return PF_IMPOSSIBLE_MC;
  }

  cost = MIN(cost, get_move_rate(params));

  if (params->turn_mode == TM_BEST_TIME) {
    if (to_danger && cost >= moves_left) {
      /* We would have to end the turn on a dangerous tile! */
      return PF_IMPOSSIBLE_MC;
    }
  } else {
    /* Default is TM_WORST_TIME.  
     * It should be specified explicitly though! */
    if (cost > moves_left
        || (to_danger && cost == moves_left)) {
      /* This move is impossible (at least without waiting) 
       * or we would end our turn on a dangerous tile */
      return PF_IMPOSSIBLE_MC;
    }
  }

  return cost;
}

/*****************************************************************************
  This function returns the fills the cost needed for a position, to get full
  moves at the next turn. This would be called only when the status is
  NS_WAITING.
*****************************************************************************/
static int
pf_danger_map_fill_cost_for_full_moves(const struct pf_parameter *param,
                                       int cost)
{
  int moves_left = get_moves_left(param, cost);

  if (moves_left < get_move_rate(param)) {
    return cost + moves_left;
  } else {
    return cost;
  }
}

/*****************************************************************************
  Primary method for iterative path-finding in presence of danger
  Notes: 
  1. Whenever the path-finding stumbles upon a dangerous 
  location, it goes into a sub-Dijkstra which processes _only_ 
  dangerous locations, by means of a separate queue.  When this
  sub-Dijkstra reaches a safe location, it records the segment of
  the path going across the dangerous terrain.  Hence danger_segment is an
  extended (and reversed) version of the dir_to_here field.  It can be 
  re-recorded multiple times as we find shorter and shorter routes.
  2. Waiting is realised by inserting the (safe) tile back into 
  the queue with a lower priority P.  This tile might pop back 
  sooner than P, because there might be several copies of it in 
  the queue already.  But that does not seem to present any 
  problems.
  3. For some purposes, NS_WAITING is just another flavour of NS_PROCESSED,
  since the path to a NS_WAITING tile has already been found.
  4. The code is arranged so that if the turn-mode is TM_WORST_TIME, a 
  cavalry with non-full MP will get to a safe mountain tile only after 
  waiting.  This waiting, although realised through NS_WAITING, is 
  different from waiting before going into the danger area, so it will not 
  be marked as "waiting" on the resulting paths.
  5. This algorithm cannot guarantee the best safe segments across 
  dangerous region.  However it will find a safe segment if there 
  is one.  To gurantee the best (in terms of total_CC) safe segments 
  across danger, supply get_EC which returns small extra on 
  dangerous tiles.
*****************************************************************************/
static bool pf_danger_map_iterate(struct pf_map *pfm)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);
  struct tile *tile = pfm->tile;
  mapindex_t index = tile_index(tile);
  struct pf_danger_node *node = &pfdm->lattice[index];
  const struct pf_parameter *params = pf_map_get_parameter(pfm);

  /* There is no exit from DONT_LEAVE tiles! */
  if (node->behavior != TB_DONT_LEAVE) {
    /* Cost at tile but taking into account waiting */
    int loc_cost;
    if (node->status != NS_WAITING) {
      loc_cost = node->cost;
    } else {
      loc_cost = pf_danger_map_fill_cost_for_full_moves(params,
                                                        node->cost);
    }

    /* The previous position is contained in {x,y} fields of map */
    adjc_dir_iterate(tile, tile1, dir) {
      mapindex_t index1 = tile_index(tile1);
      struct pf_danger_node *node1 = &pfdm->lattice[index1];
      int cost;
      int extra = 0;

      /* Dangerous tiles can be updated even after being processed */
      if ((node1->status == NS_PROCESSED || node1->status == NS_WAITING)
          && !node1->is_dangerous) {
        continue;
      }

      /* Initialise target tile if necessary */
      if (node1->status == NS_UNINIT) {
        pf_danger_node_init(pfdm, node1, tile1);
      }

      /* Can we enter this tile at all? */
      if (!node1->can_invade || node1->behavior == TB_IGNORE) {
        continue;
      }

      /* Is the move ZOC-ok? */
      if (params->get_zoc && !(node->zoc_number == ZOC_MINE
                               || node1->zoc_number != ZOC_NO)) {
        continue;
      }

      /* Evaluate the cost of the move */
      if (node1->node_known_type == TILE_UNKNOWN) {
        cost = params->unknown_MC;
      } else {
        cost = params->get_MC(tile, dir, tile1, params);
      }
      if (cost == PF_IMPOSSIBLE_MC) {
        continue;
      }
      cost = pf_danger_map_adjust_cost(params, cost, node1->is_dangerous,
                                       get_moves_left(params, loc_cost));

      if (cost == PF_IMPOSSIBLE_MC) {
        /* This move is deemed impossible */
        continue;
      }

      /* Total cost at xy1 */
      cost += loc_cost;

      /* Evaluate the extra cost of the destination, if it's relevant */
      if (params->get_EC) {
        extra = node1->extra_tile + node->extra_cost;
      }

      /* Update costs and add to queue, if this is a better route to tile1 */
      if (!node1->is_dangerous) {
        int cost_of_path = get_total_CC(params, cost, extra);

        if (node1->status == NS_INIT
            || (cost_of_path< get_total_CC(params, node1->cost,
                                           node1->extra_cost))) {
          node1->extra_cost = extra;
          node1->cost = cost;
          node1->dir_to_here = dir;
          /* Clear the previously recorded path back */
          if (node1->danger_segment) {
            free(node1->danger_segment);
            node1->danger_segment = NULL;
          }
          if (node->is_dangerous) {
            /* Transition from red to blue, need to record the path back */
            pf_danger_map_create_segment(pfdm, node1);
          } else {
            /* We don't consider waiting to get to a safe tile as 
             * "real" waiting */
            node1->waited = FALSE;
          }
          node1->status = NS_NEW;
          pq_insert(pfdm->queue, index1, -cost_of_path);
        }
      } else {
        /* The procedure is slightly different for dangerous nodes */
        /* We will update costs if:
         * 1: we are here for the first time
         * 2: we can possibly go further across dangerous area or
         * 3: we can have lower extra and will not
         *    overwrite anything useful */
        if (node1->status == NS_INIT
            || (get_moves_left(params, cost)
                > get_moves_left(params, node1->cost))
            || ((get_total_CC(params, cost, extra)
                 < get_total_CC(params, node1->cost, node1->extra_cost))
                && node1->status == NS_PROCESSED)) {
          node1->extra_cost = extra;
          node1->cost = cost;
          node1->dir_to_here = dir;
          node1->status = NS_NEW;
          node1->waited = (node->status == NS_WAITING);
          /* Extra costs of all nodes in danger_queue are equal! */
          pq_insert(pfdm->danger_queue, index1, -cost);
        }
      }
    } adjc_dir_iterate_end;
  }

  if (!node->is_dangerous && node->status != NS_WAITING
      && get_moves_left(params, node->cost) < get_move_rate(params)) {
    int fc, cc;
    /* Consider waiting at this node. 
     * To do it, put it back into queue. */
    fc = pf_danger_map_fill_cost_for_full_moves(params, node->cost);
    cc = get_total_CC(params, fc, node->extra_cost);
    node->status = NS_WAITING;
    pq_insert(pfdm->queue, index, -cc);
  } else {
    node->status = NS_PROCESSED;
  }

  /* Get the next nearest node */

  /* First try to get it from danger_queue */
  if (!pq_remove(pfdm->danger_queue, &index)) {
    /* No dangerous nodes to process, go for a safe one */
    do {
      if (!pq_remove(pfdm->queue, &index)) {
        return FALSE;
      }
    } while (pfdm->lattice[index].status == NS_PROCESSED);
  }

  tile = index_to_tile(index);
  pfm->tile = tile;
  node = &pfdm->lattice[index];

  assert(node->status > NS_INIT);

  if (node->status == NS_WAITING) {
    /* We've already returned this node once, skip it */
    freelog(LOG_DEBUG, "Considering waiting at (%d, %d)", TILE_XY(tile));
    return pf_map_iterate(pfm);
  } else if (node->is_dangerous) {
    /* We don't return dangerous tiles */
    freelog(LOG_DEBUG, "Reached dangerous tile (%d, %d)", TILE_XY(tile));
    return pf_map_iterate(pfm);
  }

  /* Just return it */
  return TRUE;
}

/************************************************************************
  Iterate the map until ptile is reached.
************************************************************************/
static bool pf_danger_map_iterate_until(struct pf_danger_map *pfdm,
					struct tile *ptile)
{
  struct pf_map *pfm = PF_MAP(pfdm);
  const struct pf_danger_node *node = &pfdm->lattice[tile_index(ptile)];

  while (node->status != NS_PROCESSED
	 && node->status != NS_WAITING
	 && !same_pos(ptile, pfm->tile)) {
    if (!pf_map_iterate(pfm)) {
      return FALSE;
    }
  }

  return TRUE;
}

/*************************************************************************
  Return the move cost at ptile.  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
  This function returns PF_IMPOSSIBLE_MC on unreachable positions.
*************************************************************************/
static int pf_danger_map_get_move_cost(struct pf_map *pfm,
				       struct tile *ptile)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);

  if (pf_danger_map_iterate_until(pfdm, ptile)) {
    return (pfdm->lattice[tile_index(ptile)].cost
	    - get_move_rate(pf_map_get_parameter(pfm))
	    + get_moves_left_initially(pf_map_get_parameter(pfm)));
  } else {
    return PF_IMPOSSIBLE_MC;
  }
}

/*************************************************************************
  Return the path to ptile.  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
*************************************************************************/
static struct pf_path *pf_danger_map_get_path(struct pf_map *pfm,
                                              struct tile *ptile)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);
  struct pf_danger_node *node = &pfdm->lattice[tile_index(ptile)];

  if (node->status != NS_UNINIT
      && node->is_dangerous
      && !same_pos(ptile, pfm->params.start_tile)) {
    /* "Best" path to a dangerous tile is undefined */
    /* TODO: return the "safest" path */
    return NULL;
  }

  if (pf_danger_map_iterate_until(pfdm, ptile)) {
    return pf_danger_map_construct_path(pfdm, ptile);
  } else {
    return NULL;
  }
}

/***************************************************************************
  Get info about position at ptile and put it in pos.  If ptile has not been
  reached yet, iterate the map until we reach it. Should _always_ check the
  return value, for the position might be unreachable.
***************************************************************************/
static bool pf_danger_map_get_position(struct pf_map *pfm,
                                       struct tile *ptile,
                                       struct pf_position *pos)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);

  if (pf_danger_map_iterate_until(pfdm, ptile)) {
    pf_danger_map_fill_position(pfdm, ptile, pos);
    return TRUE;
  } else {
    return FALSE;
  }
}

/*********************************************************************
  After usage the map must be destroyed.
*********************************************************************/
static void pf_danger_map_destroy(struct pf_map *pfm)
{
  struct pf_danger_map *pfdm = PF_DANGER_MAP(pfm);
  struct pf_danger_node *node;
  mapindex_t i;

  /* Need to clean up the dangling danger_sements */
  for (i = 0, node = pfdm->lattice; i < MAP_INDEX_SIZE; i++, node++) {
    if (node->danger_segment) {
      free(node->danger_segment);
    }
  }
  free(pfdm->lattice);
  pq_destroy(pfdm->queue);
  pq_destroy(pfdm->danger_queue);
  free(pfdm);
}

/**************************************************************************
  Create a pf_danger_map. NB: The "constructor" returns a pf_map.
**************************************************************************/
static struct pf_map *pf_danger_map_new(const struct pf_parameter *parameter)
{
  struct pf_danger_map *pfdm;
  struct pf_map *base_map;
  struct pf_parameter *params;
  struct pf_danger_node *node;

  pfdm = fc_calloc(1, sizeof(*pfdm));
  base_map = &pfdm->base_map;
  params = &base_map->params;
#ifdef PF_DEBUG
  /* Set the mode, used for cast check */
  base_map->mode = PF_DANGER;
#endif /* PF_DEBUG */

  /* Alloc the map */
  pfdm->lattice = fc_calloc(MAP_INDEX_SIZE, sizeof(struct pf_danger_node));
  pfdm->queue = pq_create(INITIAL_QUEUE_SIZE);
  pfdm->danger_queue = pq_create(INITIAL_QUEUE_SIZE);

  /* MC callback must be set */
  assert(parameter->get_MC != NULL);

  /* is_pos_dangerous callback must be set */
  assert(parameter->is_pos_dangerous != NULL);

  /* Copy parameters */
  *params = *parameter;

  /* Initialize virtual function table. */
  base_map->destroy = pf_danger_map_destroy;
  base_map->get_move_cost = pf_danger_map_get_move_cost;
  base_map->get_path = pf_danger_map_get_path;
  base_map->get_position = pf_danger_map_get_position;
  base_map->iterate = pf_danger_map_iterate;

  /* Initialise starting coordinates */
  base_map->tile = params->start_tile;

  /* Initialise starting node */
  node = &pfdm->lattice[tile_index(params->start_tile)];
  pf_danger_node_init(pfdm, node, params->start_tile);
  /* This makes calculations of turn/moves_left more convenient, but we
   * need to subtract this value before we return cost to the user. Note
   * that cost may be negative if moves_left_initially > move_rate
   * (see get_turn()). */
  node->cost = get_move_rate(params) - get_moves_left_initially(params);
  node->extra_cost = 0;
  node->dir_to_here = -1;

  return PF_MAP(pfdm);
}



/* ================ Specific pf_fuel_* mode structures ================= */

/* Some comments on implementation:
 * 1. cost (aka total_MC) is sum of MCs altered to fit to the turn_mode
 *    see adjust_cost.
 * 2. dir_to_here is for backtracking along the tree of shortest paths.
 * 3. node_known_type, behavior, zoc_number, extra_tile, is_enemy_tile,
 *    moves_left_req and waited are all cached values.
 * It is possible to shove them into a separate array which is allocated
 * only if a corresponding option in the parameter is set. A less drastic
 * measure would be to pack the first three into one byte. All of there are
 * time-saving measures and should be tested once we get an established
 * user-base. */
struct pf_fuel_node {
  int cost;			/* total_MC */
  int extra_cost;		/* total_EC */
  int moves_left;		/* Moves left at this position */
  utiny_t dir_to_here;		/* direction from which we came */

  /* Cached values */
  utiny_t status;		/* (enum pf_node_status really) */
  int extra_tile;		/* EC */
  utiny_t node_known_type;
  utiny_t behavior;
  utiny_t zoc_number;		/* (enum pf_zoc_type really) */
  bool can_invade;
  bool is_enemy_tile;
  int moves_left_req;		/* The minimum required moves left */
  bool waited;			/* TRUE if waited to get here */

  struct pf_fuel_pos {
    enum direction8 dir;
    int cost;
    int extra_cost;
    int moves_left;
  } *fuel_segment;		/* Segment leading across the danger area
				 * back to the nearest safe node:
				 * need to remeber costs and stuff */
};

/* Derived structure of struct pf_map. */
struct pf_fuel_map {
  struct pf_map base_map;       /* Base structure, must be the first! */

  struct pqueue *queue;         /* Queue of nodes we have reached but not
                                 * processed yet (NS_NEW), sorted by their
                                 * total_CC */
  struct pqueue *out_of_fuel_queue; /* Dangerous positions go there */
  struct pf_fuel_node *lattice; /* Lattice of nodes */
};

/* Up-cast macro. */
#ifdef PF_DEBUG
static inline struct pf_fuel_map *
pf_fuel_map_check(struct pf_map *pfm, const char *file, int line)
{
  if (!pfm || pfm->mode != PF_FUEL) {
    real_die(file, line, "Wrong pf_map to pf_fuel_map conversion");
  }
  return (struct pf_fuel_map *) pfm;
}
#define PF_FUEL_MAP(pfm) pf_fuel_map_check(pfm, __FILE__, __LINE__)
#else
#define PF_FUEL_MAP(pfm) ((struct pf_fuel_map *) (pfm))
#endif /* PF_DEBUG */

/* ==============  Specific pf_fuel_* mode functions =============== */

/**********************************************************************
  Calculates cached values of the target node: node_known_type and zoc
**********************************************************************/
static void pf_fuel_node_init(struct pf_fuel_map *pffm,
                              struct pf_fuel_node *node,
                              struct tile *ptile)
{
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pffm));

  /* Establish the "known" status of node */
  if (params->omniscience) {
    node->node_known_type = TILE_KNOWN_SEEN;
  } else {
    node->node_known_type = tile_get_known(ptile, params->owner);
  }

  /* Establish the tile behavior */
  if (params->get_TB) {
    node->behavior = params->get_TB(ptile, node->node_known_type, params);
  } else {
    /* The default */
    node->behavior = TB_NORMAL;
  }

  if (params->get_zoc) {
    struct city *pcity = tile_city(ptile);
    struct terrain *pterrain = tile_terrain(ptile);
    bool my_zoc = (NULL != pcity || pterrain == T_UNKNOWN
                   || terrain_has_flag(pterrain, TER_OCEANIC)
                   || params->get_zoc(params->owner, ptile));
    /* ZoC rules cannot prevent us from moving into/attacking an occupied
     * tile.  Other rules can, but we don't care about them here. */
    bool occupied = (unit_list_size(ptile->units) > 0 || NULL != pcity);

    /* ZOC_MINE means can move unrestricted from/into it,
     * ZOC_ALLIED means can move unrestricted into it,
     * but not necessarily from it */
    node->zoc_number = (my_zoc ? ZOC_MINE
                        : (occupied ? ZOC_ALLIED : ZOC_NO));
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->zoc_number = 0;
#endif
  }

  /* Evaluate the extra cost of the destination */
  if (params->get_EC) {
    node->extra_tile = params->get_EC(ptile, node->node_known_type,
                                      params);
#ifdef ZERO_VARIABLES_FOR_SEARCHING
  } else {
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->extra_tile = 0;
#endif
  }

  if (params->can_invade_tile) {
    node->can_invade = params->can_invade_tile(params->owner, ptile);
  } else {
    node->can_invade = TRUE;
  }

  if (is_enemy_unit_tile(ptile, params->owner)
      || (is_enemy_city_tile(ptile, params->owner))) {
    node->is_enemy_tile = TRUE;
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->moves_left_req = 0; /* Attack is always possible theorically. */
#endif
  } else {
#ifdef ZERO_VARIABLES_FOR_SEARCHING
    /* Nodes are allocated by fc_calloc(), so should be already set to 0. */
    node->is_enemy_tile = FALSE;
#endif
    node->moves_left_req =
      params->get_moves_left_req(ptile, node->node_known_type, params);
  }

  /* waited is set to zero by fc_calloc. */

  node->status = NS_INIT;
}

/****************************************************************************
  Finalize the fuel position.
****************************************************************************/
static void pf_fuel_base_finalize_position(const struct pf_parameter *param,
                                           struct pf_position *pos,
                                           int cost, int moves_left)
{
  int move_rate = get_move_rate(param);

  if (move_rate > 0) {
    /* Cost may be negative; see get_turn(). */
    pos->turn = cost < 0 ? 0 : cost / move_rate;
    pos->fuel_left = (moves_left - 1) / move_rate + 1;
    pos->moves_left = moves_left % move_rate;
  } else {
    /* This unit cannot move by itself. */
    pos->turn = same_pos(pos->tile, param->start_tile) ? 0 : FC_INFINITY;
    pos->fuel_left = 0;
    pos->moves_left = 0;
  }
}

/****************************************************************************
  Finalize the fuel position.
****************************************************************************/
static void pf_fuel_finalize_position(struct pf_position *pos,
                                      const struct pf_parameter *params,
                                      const struct pf_fuel_node *node,
                                      const struct pf_fuel_pos *head)
{
  if (head) {
    pf_fuel_base_finalize_position(params, pos,
                                   head->cost, head->moves_left);
  } else {
    pf_fuel_base_finalize_position(params, pos,
                                   node->cost, node->moves_left);
  }
}

/****************************************************************************
  Fill in the position which must be discovered already. A helper
  for *_get_position functions.  This also "finalizes" the position.
****************************************************************************/
static void pf_fuel_map_fill_position(const struct pf_fuel_map *pffm,
                                      struct tile *ptile,
                                      struct pf_position *pos)
{
  mapindex_t index = tile_index(ptile);
  struct pf_fuel_node *node = &pffm->lattice[index];
  struct pf_fuel_pos *head = node->fuel_segment;
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pffm));

#ifdef PF_DEBUG
  if ((node->status != NS_PROCESSED
       && !same_pos(ptile, PF_MAP(pffm)->tile)) || !head) {
    die("pf_normal_map_fill_position to an unreached destination");
    return;
  }
#endif /* PF_DEBUG */

  pos->tile = ptile;
  pos->total_EC = head->extra_cost;
  pos->total_MC = (head->cost - get_move_rate(params)
                   + get_moves_left_initially(params));
  if (params->turn_mode == TM_BEST_TIME
      || params->turn_mode == TM_WORST_TIME) {
    pf_fuel_finalize_position(pos, params, node, head);
  } else if (params->turn_mode == TM_NONE
             || params->turn_mode == TM_CAPPED) {
    pos->turn = -1;
    pos->moves_left = -1;
    pos->fuel_left = -1;
  } else {
    die("unknown TC");
  }

  pos->dir_to_here = head->dir;
  /* This field does not apply */
  pos->dir_to_next_pos = -1;
}

/*******************************************************************
  Read off the path to the node (x, y), but with danger
  NB: will only find paths to safe tiles!
*******************************************************************/
static struct pf_path *
pf_fuel_map_construct_path(const struct pf_fuel_map *pffm,
                           struct tile *ptile)
{
  struct pf_path *path = fc_malloc(sizeof(*path));
  enum direction8 dir_next = -1;
  struct pf_fuel_pos *segment = NULL;
  struct pf_fuel_node *node = &pffm->lattice[tile_index(ptile)];
  bool waited = node->waited;
  int length = 1;
  struct tile *iter_tile = ptile;
  const struct pf_parameter *params = pf_map_get_parameter(PF_MAP(pffm));
  struct pf_position *pos;
  int i;

  if (params->turn_mode != TM_BEST_TIME
      && params->turn_mode != TM_WORST_TIME) {
    die("illegal TM in path-finding with danger");
    return NULL;
  }

  /* First iterate to find path length */
  /* Note: the start point could be reached in the middle of a segment. */
  while (!same_pos(iter_tile, params->start_tile)
         || (segment && is_valid_dir(segment->dir))) {

    if (node->moves_left_req == 0) {
      if (waited) {
        length += 2;
      } else {
        length++;
      }
      waited = node->waited;
    } else {
      length++;
    }

    if (node->moves_left_req == 0 || !segment) {
      segment = node->fuel_segment;
    }

    if (segment) {
      /* We are in a danger segment */
      dir_next = segment->dir;
      segment++;
    } else {
      /* Classical node */
      dir_next = node->dir_to_here;
    }

    /* Step backward */
    iter_tile = mapstep(iter_tile, DIR_REVERSE(dir_next));
    node = &pffm->lattice[tile_index(iter_tile)];
  }
  if (node->moves_left_req == 0 && waited) {
    /* We wait at the start point */
    length++;
  }

  /* Allocate memory for path */
  path->positions = fc_malloc(length * sizeof(struct pf_position));
  path->length = length;

  /* Reset variables for main iteration */
  iter_tile = ptile;
  node = &pffm->lattice[tile_index(ptile)];
  segment = NULL;
  waited = node->waited;
  dir_next = -1;

  for (i = length - 1; i >= 0; i--) {
    bool old_waited = FALSE;

    if (node->moves_left_req == 0 || !segment) {
      segment = node->fuel_segment;
    }

    /* 1: Deal with waiting */
    if (node->moves_left_req == 0) {
      if (waited) {
        /* Waited at _this_ tile, need to record it twice in the
         * path. Here we record our state _after_ waiting (e.g.
         * full move points). */
        pos = &path->positions[i];
        pos->tile = iter_tile;
        pos->total_EC = segment ? segment->extra_cost : node->extra_cost;
        pf_fuel_finalize_position(pos, params, node, segment);
        pos->total_MC = ((pos->turn - 1) * params->move_rate
                         + params->moves_left_initially);
        pos->dir_to_next_pos = dir_next;
        /* Set old_waited so that we record -1 as a direction at
         * the step we were going to wait. */
        old_waited = TRUE;
        i--;
      }
      /* Update "waited" (node->waited means "waited to get here") */
      waited = node->waited;
    }

    /* 2: Fill the current position */
    pos = &path->positions[i];
    pos->tile = iter_tile;
    if (segment) {
      pos->total_MC = segment->cost;
      pos->total_EC = segment->extra_cost;
    } else {
      pos->total_MC = node->cost;
      pos->total_EC = node->extra_cost;
    }
    pf_fuel_finalize_position(pos, params, node, segment);
    pos->total_MC -= (get_move_rate(params)
                      - get_moves_left_initially(params));
    pos->dir_to_next_pos = (old_waited ? -1 : dir_next);

    /* 3: Check if we finished */
    if (i == 0) {
      /* We should be back at the start now! */
      assert(same_pos(iter_tile, params->start_tile));
      return path;
    }

    /* 4: Calculate the next direction */
    if (segment) {
      /* We are in a danger segment */
      dir_next = segment->dir;
      segment++;
    } else {
      /* Classical node */
      dir_next = node->dir_to_here;
    }

    /* 5: Step further back */
    iter_tile = mapstep(iter_tile, DIR_REVERSE(dir_next));
    node = &pffm->lattice[tile_index(iter_tile)];
  }

  die("pf_fuel_map_construct_path: cannot get to the starting point!");
  return NULL;
}

/***********************************************************************
  Creating path segment going back from d_node1 to a safe tile,
  or the start tile by default (!is_valid_dir(node->dir_to_here)).
***********************************************************************/
static void pf_fuel_map_create_segment(struct pf_fuel_map *pffm,
                                       struct tile *tile1,
                                       struct pf_fuel_node *node1)
{
  struct tile *ptile = tile1;
  struct pf_fuel_node *node = node1;
  struct pf_fuel_pos *pos;
  int length = 1;
  int i;

  /* Clear the previously recorded path back */
  if (node1->fuel_segment) {
    free(node1->fuel_segment);
  }

  /* First iteration for determining segment length */
  do {
    length++;
    ptile = mapstep(ptile, DIR_REVERSE(node->dir_to_here));
    node = &pffm->lattice[tile_index(ptile)];
  } while (node->moves_left_req != 0 && is_valid_dir(node->dir_to_here));

  /* Allocate memory for segment */
  node1->fuel_segment = fc_malloc(length * sizeof(struct pf_fuel_pos));

  /* Reset tile and node pointers for main iteration */
  ptile = tile1;
  node = node1;

  /* Now fill the positions */
  for (i = 0, pos = node1->fuel_segment; i < length; i++, pos++) {
    /* Record the direction */
    pos->dir = node->dir_to_here;
    pos->cost = node->cost;
    pos->extra_cost = node->extra_cost;
    pos->moves_left = node->moves_left;
    if (i == length - 2) {
      /* The node before the last contains "waiting" info */
      node1->waited = node->waited;
    } else if (i == length - 1) {
      break;
    }

    /* Step further down the tree */
    ptile = mapstep(ptile, DIR_REVERSE(node->dir_to_here));
    node = &pffm->lattice[tile_index(ptile)];
  }

  /* Make sure we reached a safe node, or the start tile */
  assert(node->moves_left_req == 0 || !is_valid_dir(node->dir_to_here));
}

/***************************************************************************
  This function returns whether a unit with or without fuel can attack.

  moves_left: moves left before the attack.
  moves_left_req: required moves left to hold on the tile after attacking.
***************************************************************************/
static bool pf_fuel_map_attack_is_possible(const struct pf_parameter *param,
                                           int moves_left,
                                           int moves_left_req)
{
  if (BV_ISSET(param->unit_flags, F_ONEATTACK)) {
    if (param->fuel == 1) {
      /* Case missile */
      return TRUE;
    } else {
      /* Case Bombers */
      if (moves_left <= param->move_rate) {
        /* We are in the last turn of fuel, don't attack */
        return FALSE;
      } else {
        return TRUE;
      }
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

/*****************************************************************************
  This function returns the fill cost needed for a position, to get full
  moves at the next turn. This would be called only when the status is
  NS_WAITING.
*****************************************************************************/
static int
pf_fuel_map_fill_cost_for_full_moves(const struct pf_parameter *param,
                                     int cost, int moves_left)
{
  return cost + moves_left % param->move_rate;
}

/***************************************************************************
  Primary method for iterative path-finding in presence of danger

  Notes:
  1. Whenever the path-finding stumbles upon a dangerous location, it goes
  into a sub-Dijkstra which processes _only_ dangerous locations, by means
  of a separate queue.  When this sub-Dijkstra reaches a safe location, it
  records the segment of the path going across the dangerous terrain.  Hence
  segment is an extended (and reversed) version of the dir_to_here field.
  It can be re-recorded multiple times as we find shorter and shorter
  routes.
  
  2. Waiting is realised by inserting the (safe) tile back into the queue
  with a lower priority P.  This tile might pop back sooner than P, because
  there might be several copies of it in the queue already.  But that does
  not seem to present any problems.

  3. For some purposes, NS_WAITING is just another flavour of NS_PROCESSED,
  since the path to a NS_WAITING tile has already been found.

  4. The code is arranged so that if the turn-mode is TM_WORST_TIME, a
  cavalry with non-full MP will get to a safe mountain tile only after
  waiting.  This waiting, although realised through NS_WAITING, is different
  from waiting before going into the danger area, so it will not be marked
  as "waiting" on the resulting paths.

  5. This algorithm cannot guarantee the best safe segments across dangerous
  region.  However it will find a safe segment if there is one.  To gurantee
  the best (in terms of total_CC) safe segments across danger, supply get_EC
  which returns small extra on dangerous tiles.
***************************************************************************/
static bool pf_fuel_map_iterate(struct pf_map *pfm)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);
  struct tile *tile = pfm->tile;
  mapindex_t index = tile_index(tile);
  struct pf_fuel_node *node = &pffm->lattice[index];
  const struct pf_parameter *params = pf_map_get_parameter(pfm);

  /* There is no exit from DONT_LEAVE tiles! */
  if (node->behavior != TB_DONT_LEAVE) {
    int loc_cost, loc_moves_left;

    if (node->status != NS_WAITING) {
      loc_cost = node->cost;
      loc_moves_left = node->moves_left;
    } else {
      /* Cost and moves left at tile but taking into account waiting */
      loc_cost = pf_fuel_map_fill_cost_for_full_moves(params, node->cost,
                                                      node->moves_left);
      loc_moves_left = get_move_rate(params);
    }

    /* The previous position is contained in {x,y} fields of map */
    adjc_dir_iterate(tile, tile1, dir) {
      mapindex_t index1 = tile_index(tile1);
      struct pf_fuel_node *node1 = &pffm->lattice[index1];
      int cost, extra = 0;
      int moves_left, mlr = 0;
      int cost_of_path, old_cost_of_path;
      struct tile *prev_tile;
      struct pf_fuel_pos *pos;

      /* Non-full fuel tiles can be updated even after being processed */
      if ((node1->status == NS_PROCESSED  || node1->status == NS_WAITING)
          && node1->moves_left_req == 0) {
        continue;
      }

      /* Initialise target tile if necessary */
      if (node1->status == NS_UNINIT) {
        pf_fuel_node_init(pffm, node1, tile1);
      }

      /* Cannot use this unreachable tile */
      if (node1->moves_left_req == PF_IMPOSSIBLE_MC) {
        continue;
      }

      /* Can we enter this tile at all? */
      if (!node1->can_invade || node1->behavior == TB_IGNORE) {
        continue;
      }

      /* Is the move ZOC-ok? */
      if (params->get_zoc && !(node->zoc_number == ZOC_MINE
                               || node1->zoc_number != ZOC_NO)) {
        continue;
      }

      /* Evaluate the cost of the move */
      if (node1->node_known_type == TILE_UNKNOWN) {
        cost = params->unknown_MC;
      } else {
        cost = params->get_MC(tile, dir, tile1, params);
      }
      if (cost == PF_IMPOSSIBLE_MC) {
        continue;
      }

      moves_left = loc_moves_left - cost;
      if (moves_left < node1->moves_left_req) {
        /* We don't have enough moves left */
        continue;
      }

      if (node1->is_enemy_tile
          && !pf_fuel_map_attack_is_possible(params, loc_moves_left,
                                             node->moves_left_req)) {
        /* We wouldn't have enough moves left after attacking */
        continue;
      }

      /* Total cost at xy1 */
      cost += loc_cost;

      /* Evaluate the extra cost of the destination, if it's relevant */
      if (params->get_EC) {
        extra = node1->extra_tile + node->extra_cost;
      }

      /*
       * Update costs and add to queue, if this is a better route to xy1.
       *
       * case safe tiles or reached directly without waiting...
       */
      pos = node1->fuel_segment;
      cost_of_path = get_total_CC(params, cost, extra);
      if (node1->status == NS_INIT) {
        /* Not calculated yet */
        old_cost_of_path = 0;
      } else if (pos) {
        /*
         * We have a path to this tile. The default values could have been
         * overwritten if we had more moves left to deal with waiting. Then,
         * we have to get back the value of this node to calculate the cost.
         */
        old_cost_of_path = get_total_CC(params, pos->cost, pos->extra_cost);
      } else {
        /* Default cost */
        old_cost_of_path = get_total_CC(params, node1->cost,
                                        node1->extra_cost);
      }

      /* We would prefer to be the nearest possible of a refuel point,
       * especially in the attack case. */
      if (cost_of_path == old_cost_of_path) {
        prev_tile = mapstep(tile1, DIR_REVERSE(pos ? pos->dir
                                               : node1->dir_to_here));
        mlr = pffm->lattice[tile_index(prev_tile)].moves_left_req;
      } else {
        prev_tile = NULL;
      }

      if (node1->status == NS_INIT || cost_of_path < old_cost_of_path
          || (prev_tile && mlr > node->moves_left_req)) {
        node1->extra_cost = extra;
        node1->cost = cost;
        node1->moves_left = moves_left;
        node1->dir_to_here = dir;
        /* Always record the segment, including when it is not dangerous. */
        pf_fuel_map_create_segment(pffm, tile1, node1);
        if (node->status == NS_WAITING) {
          /* It is the first part of a fuel segment */
          node1->waited = TRUE;
        }
        node1->status = NS_NEW;
        if (node1->moves_left_req == 0) {
          pq_insert(pffm->queue, index1, -cost_of_path);
        } else {
          /* Extra costs of all nodes in out_of_fuel_queue are equal! */
          pq_insert(pffm->out_of_fuel_queue, index1, -cost);
        }
        continue;
      }

      if (node1->moves_left_req == 0) {
        /* Waiting to cross over a refuel is senseless. */
        continue;
      }

      if (pos) {
        /*
         * We had a path to this tile. Here, we have to use back the
         * default values because we could have more moves left if we
         * waited somewhere.
         */
        old_cost_of_path = get_total_CC(params, node1->cost,
                                        node1->extra_cost);
      }

      if (moves_left > node1->moves_left
          || cost_of_path < old_cost_of_path) {
        /* The procedure is slightly different for out of fuel nodes */
        /* We will update costs if:
         * 1: we are here for the first time
         * 2: we can possibly go further across dangerous area or
         * 3: we can have lower extra and will not overwrite anything
         *    useful */
        node1->extra_cost = extra;
        node1->cost = cost;
        node1->moves_left = moves_left;
        node1->dir_to_here = dir;
        node1->status = NS_NEW;
        node1->waited = (node->status == NS_WAITING);
        /* Extra costs of all nodes in out_of_fuel_queue are equal! */
        pq_insert(pffm->out_of_fuel_queue, index1, -cost);
      }
    } adjc_dir_iterate_end;
  }

  if (node->moves_left_req == 0
      && !node->is_enemy_tile
      && node->status != NS_WAITING
      && node->moves_left < get_move_rate(params)) {
    int fc, cc;
    /* Consider waiting at this node.
     * To do it, put it back into queue. */
    node->status = NS_WAITING;
    fc = pf_fuel_map_fill_cost_for_full_moves(params, node->cost,
                                              node->moves_left);
    cc = get_total_CC(params, fc, node->extra_cost);
    pq_insert(pffm->queue, index, -cc);
  } else {
    node->status = NS_PROCESSED;
  }

  /* Get the next nearest node */

  /* First try to get it from out_of_fuel_queue */
  if (!pq_remove(pffm->out_of_fuel_queue, &index)) {
    /* No dangerous nodes to process, go for a safe one */
    do {
      if (!pq_remove(pffm->queue, &index)) {
        return FALSE;
      }
    } while (pffm->lattice[index].status == NS_PROCESSED);
  }

  tile = index_to_tile(index);
  pfm->tile = tile;
  node = &pffm->lattice[index];

  assert(node->status > NS_INIT);

  if (node->status == NS_WAITING) {
    /* We've already returned this node once, skip it */
    freelog(LOG_DEBUG, "Considering waiting at (%d, %d)", TILE_XY(tile));
    return pf_map_iterate(pfm);
  } else if (!node->fuel_segment) {
    /* We don't return dangerous tiles */
    freelog(LOG_DEBUG, "Reached dangerous tile (%d, %d)", TILE_XY(tile));
    return pf_map_iterate(pfm);
  } else {
    /* Just return it */
    return TRUE;
  }
}

/************************************************************************
  Iterate the map until ptile is reached.
************************************************************************/
static bool pf_fuel_map_iterate_until(struct pf_fuel_map *pffm,
				      struct tile *ptile)
{
  struct pf_map *pfm = PF_MAP(pffm);
  const struct pf_fuel_node *node = &pffm->lattice[tile_index(ptile)];

  while (node->status != NS_PROCESSED
	 && node->status != NS_WAITING
	 && !same_pos(ptile, pfm->tile)) {
    if (!pf_map_iterate(pfm)) {
      return FALSE;
    }
  }

  return TRUE;
}

/*************************************************************************
  Return the move cost at ptile.  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
  This function returns PF_IMPOSSIBLE_MC on unreachable positions.
*************************************************************************/
static int pf_fuel_map_get_move_cost(struct pf_map *pfm, struct tile *ptile)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);

  if (pf_fuel_map_iterate_until(pffm, ptile)) {
    const struct pf_fuel_node *node = &pffm->lattice[tile_index(ptile)];

    return ((node->fuel_segment ? node->fuel_segment->cost : node->cost)
	    - get_move_rate(pf_map_get_parameter(pfm))
	    + get_moves_left_initially(pf_map_get_parameter(pfm)));
  } else {
    return PF_IMPOSSIBLE_MC;
  }
}

/************************************************************************
  Return the path to ptile.  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
************************************************************************/
static struct pf_path *pf_fuel_map_get_path(struct pf_map *pfm,
                                            struct tile *ptile)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);
  const struct pf_fuel_node *node = &pffm->lattice[tile_index(ptile)];

  if (node->status != NS_UNINIT
      && node->moves_left_req == PF_IMPOSSIBLE_MC
      && !same_pos(ptile, pfm->params.start_tile)) {
    /* Cannot reach it in any case. */
    return NULL;
  }

  if (pf_fuel_map_iterate_until(pffm, ptile)
      && (node->fuel_segment
          || same_pos(ptile, pfm->params.start_tile))) {
    return pf_fuel_map_construct_path(pffm, ptile);
  } else {
    return NULL;
  }
}

/*******************************************************************
  Get info about position at ptile and put it in pos.  If ptile
  has not been reached yet, iterate the map until we reach it.
  Should _always_ check the return value, forthe position might be
  unreachable.
*******************************************************************/
static bool pf_fuel_map_get_position(struct pf_map *pfm,
                                     struct tile *ptile,
                                     struct pf_position *pos)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);

  if (pf_fuel_map_iterate_until(pffm, ptile)) {
    pf_fuel_map_fill_position(pffm, ptile, pos);
    return TRUE;
  } else {
    return FALSE;
  }
}

/*********************************************************************
  After usage the map must be destroyed.
*********************************************************************/
static void pf_fuel_map_destroy(struct pf_map *pfm)
{
  struct pf_fuel_map *pffm = PF_FUEL_MAP(pfm);
  struct pf_fuel_node *node;
  mapindex_t i;

  /* Need to clean up the dangling fuel segments */
  for (i = 0, node = pffm->lattice; i < MAP_INDEX_SIZE; i++, node++) {
    if (node->fuel_segment) {
      free(node->fuel_segment);
    }
  }
  free(pffm->lattice);
  pq_destroy(pffm->queue);
  pq_destroy(pffm->out_of_fuel_queue);
  free(pffm);
}

/******************************************************************
  Create a pf_fuel_map. NB: The "constructor" returns a pf_map.
******************************************************************/
static struct pf_map *pf_fuel_map_new(const struct pf_parameter *parameter)
{
  struct pf_fuel_map *pffm;
  struct pf_map *base_map;
  struct pf_parameter *params;
  struct pf_fuel_node *node;

  pffm = fc_calloc(1, sizeof(*pffm));
  base_map = &pffm->base_map;
  params = &base_map->params;
#ifdef PF_DEBUG
  /* Set the mode, used for cast check */
  base_map->mode = PF_FUEL;
#endif /* PF_DEBUG */

  /* Alloc the map */
  pffm->lattice = fc_calloc(MAP_INDEX_SIZE, sizeof(struct pf_fuel_node));
  pffm->queue = pq_create(INITIAL_QUEUE_SIZE);
  pffm->out_of_fuel_queue = pq_create(INITIAL_QUEUE_SIZE);

  /* MC callback must be set */
  assert(parameter->get_MC != NULL);

  /* get_moves_left_req callback must be set */
  assert(parameter->get_moves_left_req != NULL);

  /* Copy parameters */
  *params = *parameter;

  /* Initialize virtual function table. */
  base_map->destroy = pf_fuel_map_destroy;
  base_map->get_move_cost = pf_fuel_map_get_move_cost;
  base_map->get_path = pf_fuel_map_get_path;
  base_map->get_position = pf_fuel_map_get_position;
  base_map->iterate = pf_fuel_map_iterate;

  /* Initialise starting coordinates */
  base_map->tile = params->start_tile;

  /* Initialise starting node */
  node = &pffm->lattice[tile_index(params->start_tile)];
  pf_fuel_node_init(pffm, node, params->start_tile);
  /* This makes calculations of turn/moves_left more convenient, but we
   * need to subtract this value before we return cost to the user.  Note
   * that cost may be negative if moves_left_initially > move_rate
   * (see get_turn()). */
  node->moves_left = get_moves_left_initially(params);
  node->cost = get_move_rate(params) - node->moves_left;
  node->extra_cost = 0;
  node->dir_to_here = -1;

  return PF_MAP(pffm);
}



/* ====================== pf_map public functions ======================= */

/***************************************************************************
  Factory function to create a new map according to the parameter.
  Does not do any iterations.
***************************************************************************/
struct pf_map *pf_map_new(const struct pf_parameter *parameter)
{
  if (parameter->is_pos_dangerous) {
    if (parameter->get_moves_left_req) {
      freelog(LOG_ERROR, "path finding code cannot deal with dangers "
              "and fuel together.");
    }
    if (parameter->get_costs) {
      freelog(LOG_ERROR, "jumbo callbacks for danger maps are not yet "
              "implemented.");
    }
    return pf_danger_map_new(parameter);
  } else if (parameter->get_moves_left_req) {
    if (parameter->get_costs) {
      freelog(LOG_ERROR, "jumbo callbacks for fuel maps are not yet "
              "implemented.");
    }
    return pf_fuel_map_new(parameter);
  }

  return pf_normal_map_new(parameter);
}

/*********************************************************************
  After usage the map must be destroyed.
*********************************************************************/
void pf_map_destroy(struct pf_map *pfm)
{
  pfm->destroy(pfm);
}

/************************************************************************
  Get the path to ptile, put it in "path".  If ptile has not been reached
  yet, iterate the map until we reach it or run out of map.
************************************************************************/
struct pf_path *pf_map_get_path(struct pf_map *pfm, struct tile *ptile)
{
  return pfm->get_path(pfm, ptile);
}

/*******************************************************************
  Get info about position at ptile and put it in pos.  If ptile
  has not been reached yet, iterate the map until we reach it.
  Should _always_ check the return value, forthe position might be
  unreachable.
*******************************************************************/
bool pf_map_get_position(struct pf_map *pfm, struct tile *ptile,
			 struct pf_position *pos)
{
  return pfm->get_position(pfm, ptile, pos);
}

/*****************************************************************
  Primary method for iterative path-finding.
  Plan: 1. Process previous position.
        2. Get new nearest position and return it.
*****************************************************************/
bool pf_map_iterate(struct pf_map *pfm)
{
  if (get_move_rate(pf_map_get_parameter(pfm)) <= 0) {
    /* This unit cannot move by itself. */
    return FALSE;
  } else {
    return pfm->iterate(pfm);
  }
}

/*******************************************************************
  Return the current position.
*******************************************************************/
struct tile *pf_map_iterator_get_tile(struct pf_map *pfm)
{
  return pfm->tile;
}

/*******************************************************************
  Return the move cost at the current position.
*******************************************************************/
int pf_map_iterator_get_move_cost(struct pf_map *pfm)
{
  return pfm->get_move_cost(pfm, pfm->tile);
}

/*******************************************************************
  Read all info about the current position into pos.
*******************************************************************/
void pf_map_iterator_get_position(struct pf_map *pfm,
				  struct pf_position *pos)
{
  assert(pfm->get_position(pfm, pfm->tile, pos));
}

/************************************************************************
  Get the path to our current position.
************************************************************************/
struct pf_path *pf_map_iterator_get_path(struct pf_map *pfm)
{
  return pfm->get_path(pfm, pfm->tile);
}

/************************************************************************
  Return current pf_parameter for given pf_map.
************************************************************************/
const struct pf_parameter *pf_map_get_parameter(const struct pf_map *pfm)
{
  return &pfm->params;
}


/* ===================== pf_path public functions ====================== */

/************************************************************************
  Printing a path.
************************************************************************/
void pf_path_print(const struct pf_path *path, int log_level)
{
  struct pf_position *pos;
  int i;

  if (path) {
    freelog(log_level, "PF: path (at %p) consists of %d positions:",
            (void *)path, path->length);
  } else {
    freelog(log_level, "PF: path is NULL");
    return;
  }

  for (i = 0, pos = path->positions; i < path->length; i++, pos++) {
    freelog(log_level,
            "PF:   %2d/%2d: (%2d,%2d) dir=%-2s cost=%2d (%2d, %d) EC=%d",
            i + 1, path->length, TILE_XY(pos->tile),
            dir_get_name(pos->dir_to_next_pos), pos->total_MC,
            pos->turn, pos->moves_left, pos->total_EC);
  }
}

/************************************************************************
  After use, a path must be destroyed.
************************************************************************/
void pf_path_destroy(struct pf_path *path)
{
  if (path) {
    free(path->positions);
    free(path);
  }
}

/******************************************************************
  Get the last position of "path".
******************************************************************/
const struct pf_position *pf_path_get_last_position(const struct pf_path *path)
{
  return &path->positions[path->length - 1];
}



/* ====================== pf_city map functions ======================= */

/* We will iterate this map at max to this cost. */
#define MAX_COST 255

struct pf_city_map {
  struct pf_parameter param;    /* Keep a parameter ready for usage. */
  struct pf_map **maps;         /* A vector of pf_map for every unit_type. */
};

/************************************************************************
  This function estime the cost for unit moves to reach the city.
  N.B.: The costs are calculated in the invert order because we want
  to know how many costs needs the units to REACH the city, and not
  to leave it.
************************************************************************/
static int pf_city_map_get_costs(const struct tile *to_tile,
                                 enum direction8 dir,
                                 const struct tile *from_tile,
                                 int to_cost, int to_extra,
                                 int *from_cost, int *from_extra,
                                 const struct pf_parameter *param)
{
  int cost;

  if (!param->omniscience
      && TILE_UNKNOWN == tile_get_known(to_tile, param->owner)) {
    cost = SINGLE_MOVE;
  } else if (!is_native_tile_to_class(param->uclass, to_tile)
             && !tile_city(to_tile)) {
    return -1;  /* Impossible move. */
  } else if (uclass_has_flag(param->uclass, UCF_TERRAIN_SPEED)) {
    if (BV_ISSET(param->unit_flags, F_IGTER)) {
      cost = MIN(map_move_cost(from_tile, to_tile), SINGLE_MOVE);
    } else {
      cost = map_move_cost(from_tile, to_tile);
    }
  } else {
    cost = SINGLE_MOVE;
  }

  if (to_cost + cost > MAX_COST) {
    return -1;  /* We reached the maximum we wanted. */
  } else if (*from_cost == PF_IMPOSSIBLE_MC     /* Uninitialized yet. */
             || to_cost + cost < *from_cost) {
    *from_cost = to_cost + cost;
    /* N.B.: We don't deal with from_extra. */
  }

  /* Let's calculate some priority. */
  return MAX(3 * SINGLE_MOVE - cost, 0);
}

/************************************************************************
  Constructor: create a new city map. The pf_city_map is used to
  check the move costs that the units needs to reach it.
************************************************************************/
struct pf_city_map *pf_city_map_new(const struct city *pcity)
{
  struct pf_city_map *pfcm = fc_malloc(sizeof(struct pf_city_map));

  /* Initialize the parameter. */
  memset(&pfcm->param, 0, sizeof(pfcm->param));
  pfcm->param.get_costs = pf_city_map_get_costs;
  pfcm->param.start_tile = city_tile(pcity);
  pfcm->param.owner = city_owner(pcity);
  pfcm->param.omniscience = !ai_handicap(city_owner(pcity), H_MAP);

  /* Initialize the map vector. */
  pfcm->maps = fc_calloc(utype_count(), sizeof(struct pf_map *));

  return pfcm;
}

/************************************************************************
  Destructor: free the new city map.
************************************************************************/
void pf_city_map_destroy(struct pf_city_map *pfcm)
{
  size_t i;

  assert(NULL != pfcm);

  for (i = 0; i < utype_count(); i++) {
    if (pfcm->maps[i]) {
      pf_map_destroy(pfcm->maps[i]);
    }
  }
  free(pfcm->maps);
  free(pfcm);
}

/************************************************************************
  Get the move costs that unit needs to reach the city.
************************************************************************/
int pf_city_map_get_move_cost(struct pf_city_map *pfcm,
                              const struct unit_type *punittype,
                              struct tile *ptile)
{
  Unit_type_id index = utype_index(punittype);
  struct pf_map *pfm = pfcm->maps[index];

  if (!pfm) {
    /* Not created yet. */
    pfcm->param.uclass = utype_class(punittype);
    pfcm->param.unit_flags = punittype->flags;
    pfm =  pf_map_new(&pfcm->param);
    pfcm->maps[index] = pfm;
  }

  return pfm->get_move_cost(pfm, ptile);
}
