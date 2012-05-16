/********************************************************************** 
 Freeciv - Copyright (C) 2002 - The Freeciv Project
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
#include <stdio.h> /* for sprintf */
#include <stdlib.h>
#include <string.h>

#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "shared.h"
#include "specialist.h"
#include "support.h"
#include "timing.h"

#include "cm.h"

/*
 * Terms used
 * ==========
 *
 * Stats: food, shields, trade, luxury, science, and gold.
 * Production: amount of stats you get directly from farming tiles or
 *      having specialists.
 * Surplus: amount of stats you get, taking buildings, taxes, disorder, and
 *      any other effects into account.
 *
 * Happy State: disorder (unhappy), content (!unhappy && !happy) and
 * happy (happy)
 *
 * Tile type: usually, many tiles produce the same f/s/t.  So we bin the
 *      tiles into tile types.  Specialists are also a 'tile type.'
 *
 * Unlike the original CM code, which used a dynamic programming approach,
 * this code uses a branch-and-bound approach.  The DP approach allowed
 * cacheing, but it was hard to guarantee the correctness of the cache, so
 * it was usually tossed and recomputed.
 *
 * The B&B approach also allows a very simple greedy search, whereas the DP
 * approach required a lot of pre-computing.  And, it appears to be very
 * slightly faster.  It evaluates about half as many solutions, but each
 * candidate solution is more expensive due to the lack of cacheing.
 *
 * We use highly specific knowledge about how the city computes its stats
 * in two places:
 * - setting the min_production array.  Ideally the city should tell us.
 * - computing the weighting for tiles.  Ditto.
 */


/****************************************************************************
 defines, structs, globals, forward declarations
*****************************************************************************/

#ifdef DEBUG
#define GATHER_TIME_STATS
#define CM_DEBUG
#endif

/* whether to print every query, or just at cm_free ; matters only
   if GATHER_TIME_STATS is on */
#define PRINT_TIME_STATS_EVERY_QUERY

#define LOG_TIME_STATS                                  LOG_DEBUG
#define LOG_CM_STATE                                    LOG_DEBUG
#define LOG_LATTICE                                     LOG_DEBUG
#define LOG_REACHED_LEAF                                LOG_DEBUG
#define LOG_BETTER_LEAF                                 LOG_DEBUG
#define LOG_PRUNE_BRANCH                                LOG_DEBUG

#ifdef GATHER_TIME_STATS
static struct {
  struct one_perf {
    struct timer *wall_timer;
    int query_count;
    int apply_count;
    const char *name;
  } greedy, opt;

  struct one_perf *current;
} performance;

static void print_performance(struct one_perf *counts);
#endif

/* Fitness of a solution.  */
struct cm_fitness {
  int weighted; /* weighted sum */
  bool sufficient; /* false => doesn't meet constraints */
};


/*
 * We have a cyclic structure here, so we need to forward-declare the
 * structs
 */
struct cm_tile_type;
struct cm_tile;

/*
 * A tile.  Has a pointer to the type, and the x/y coords.
 * Used mostly just for converting to cm_result.
 */
struct cm_tile {
  const struct cm_tile_type *type;
  int x, y; /* valid only if !is_specialist */
};

/* define the tile_vector as array<cm_tile> */
#define SPECVEC_TAG tile
#define SPECVEC_TYPE struct cm_tile
#include "specvec.h"

/* define the tile_type_vector as array <cm_tile_type*> */
#define SPECVEC_TAG tile_type
#define SPECVEC_TYPE struct cm_tile_type *
#include "specvec.h"
#define tile_type_vector_iterate(vector, var) { \
    struct cm_tile_type *var; \
    TYPED_VECTOR_ITERATE(struct cm_tile_type*, vector, var##p) { \
      var = *var##p; \
      {
#define tile_type_vector_iterate_end }} VECTOR_ITERATE_END; }

static inline void tile_type_vector_add(struct tile_type_vector *tthis,
    struct cm_tile_type *toadd) {
  tile_type_vector_append(tthis, &toadd);
}


/*
 * A tile type.
 * Holds the production (a hill produces 1/0/0);
 * Holds a list of which tiles match this (all the hills and tundra);
 * Holds a list of other types that are strictly better
 * (grassland 2/0/0, plains 1/1/0, but not gold mine 0/1/6).
 * Holds a list of other types that are strictly worse
 * (the grassland and plains hold a pointer to the hill).
 *
 * Specialists are special types; is_specialist is set, and the tile
 * vector is empty.  We can never run out of specialists.
       */
struct cm_tile_type {
  int production[O_LAST];
  double estimated_fitness; /* weighted sum of production */
  bool is_specialist;
  Specialist_type_id spec; /* valid only if is_specialist */
  struct tile_vector tiles;  /* valid only if !is_specialist */
  struct tile_type_vector better_types;
  struct tile_type_vector worse_types;
  int lattice_index; /* index in state->lattice */
  int lattice_depth; /* depth = sum(#tiles) over all better types */
};


/*
 * A partial solution.
 * Has the count of workers assigned to each lattice position, and
 * a count of idle workers yet unassigned.
 */
struct partial_solution {
  /* indices for these two match the lattice indices */
  int *worker_counts;   /* number of workers on each type */
  int *prereqs_filled;  /* number of better types filled up */

  int production[O_LAST]; /* raw production, cached for the heuristic */
  int idle;             /* number of idle workers */
};

/*
 * State of the search.
 * This holds all the information needed to do the search, all in one
 * struct, in order to clean up the function calls.
 */
struct cm_state {
  /* input from the caller */
  struct cm_parameter parameter;
  /*mutable*/ struct city *pcity;

  /* the tile lattice */
  struct tile_type_vector lattice;
  struct tile_type_vector lattice_by_prod[O_LAST];

  /* the best known solution, and its fitness */
  struct partial_solution best;
  struct cm_fitness best_value;

  /* hard constraints on production: any solution with less production than
   * this fails to satisfy the constraints, so we can stop investigating
   * this branch.  A solution with more production than this may still
   * fail (for being unhappy, for instance). */
  int min_production[O_LAST];

  /* the current solution we're examining. */
  struct partial_solution current;

  /*
   * Where we are in the search.  When we add a worker to the current
   * partial solution, we also push the tile type index on the stack.
 */
  struct {
    int *stack;
    int size;
  } choice;
};


/* return #fields + specialist types */
static int num_types(const struct cm_state *state);


/* debugging functions */
#ifdef CM_DEBUG
static void print_tile_type(int loglevel, const struct cm_tile_type *ptype,
    const char *prefix);
static void print_lattice(int loglevel,
    const struct tile_type_vector *lattice);
static void print_partial_solution(int loglevel,
    const struct partial_solution *soln,
    const struct cm_state *state);
#else
#define print_tile_type(loglevel, ptype, prefix)
#define print_lattice(loglevel, lattice)
#define print_partial_solution(loglevel, soln, state)
#endif


/****************************************************************************
  Initialize the CM data at the start of each game.  Note the citymap
  indices will not have been initialized yet (cm_init_citymap is called
  when they are).
****************************************************************************/
void cm_init(void)
{
  /* In the B&B algorithm there's not really anything to initialize. */
#ifdef GATHER_TIME_STATS
  memset(&performance, 0, sizeof(performance));

  performance.greedy.wall_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
  performance.greedy.name = "greedy";

  performance.opt.wall_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
  performance.opt.name = "opt";
#endif
}

/****************************************************************************
  Initialize the CM citymap data.  This function is called when the
  city map indices are generated (basically when the topology is set,
  shortly after the start of the game).
****************************************************************************/
void cm_init_citymap(void)
{
  /* In the B&B algorithm there's not really anything to initialize. */
}

/****************************************************************************
  Clear the cache for a city.
****************************************************************************/
void cm_clear_cache(struct city *pcity)
{
  /* The B&B algorithm doesn't have city caches so there's nothing to do. */
}

/****************************************************************************
  Called at the end of a game to free any CM data.
****************************************************************************/
void cm_free(void)
{
#ifdef GATHER_TIME_STATS
  print_performance(&performance.greedy);
  print_performance(&performance.opt);

  free_timer(performance.greedy.wall_timer);
  free_timer(performance.opt.wall_timer);
  memset(&performance, 0, sizeof(performance));
#endif
}


/***************************************************************************
  Functions of tile-types.
 ***************************************************************************/

/****************************************************************************
  Set all production to zero and initialize the vectors for this tile type.
****************************************************************************/
static void tile_type_init(struct cm_tile_type *type)
{
  memset(type, 0, sizeof(*type));
  tile_vector_init(&type->tiles);
  tile_type_vector_init(&type->better_types);
  tile_type_vector_init(&type->worse_types);
}

/****************************************************************************
  Duplicate a tile type, except for the vectors - the vectors of the new tile
  type will be empty.
****************************************************************************/
static struct cm_tile_type *tile_type_dup(const struct cm_tile_type *oldtype)
{
  struct cm_tile_type *newtype = fc_malloc(sizeof(*newtype));

  memcpy(newtype, oldtype, sizeof(*oldtype));
  tile_vector_init(&newtype->tiles);
  tile_type_vector_init(&newtype->better_types);
  tile_type_vector_init(&newtype->worse_types);
  return newtype;
}

/****************************************************************************
  Free all the storage in the tile type (but don't free the type itself).
****************************************************************************/
static void tile_type_destroy(struct cm_tile_type *type)
{
  /* The call to vector_free() will magically free all the tiles in the
   * vector. */
  tile_vector_free(&type->tiles);
  tile_type_vector_free(&type->better_types);
  tile_type_vector_free(&type->worse_types);
}

/****************************************************************************
  Destroy and free all types in the vector, and the vector itself.  This
  will free all memory associated with the vector.
****************************************************************************/
static void tile_type_vector_free_all(struct tile_type_vector *vec)
{
  tile_type_vector_iterate(vec, type) {
    /* Destroy all data in the type, and free the type itself. */
    tile_type_destroy(type);
    free(type);
  } tile_type_vector_iterate_end;

  tile_type_vector_free(vec);
}

/****************************************************************************
  Return TRUE iff all categories of the two types are equal.  This means
  all production outputs are equal and the is_specialist fields are also
  equal.
****************************************************************************/
static bool tile_type_equal(const struct cm_tile_type *a,
			    const struct cm_tile_type *b)
{
  output_type_iterate(stat) {
    if (a->production[stat] != b->production[stat])  {
      return FALSE;
    }
  } output_type_iterate_end;

  if (a->is_specialist != b->is_specialist) {
    return FALSE;
  }

  return TRUE;
}

/****************************************************************************
  Return TRUE if tile a is better or equal to tile b in all categories.

  Specialists are considered better than workers (all else being equal)
  since we have an unlimited number of them.
****************************************************************************/
static bool tile_type_better(const struct cm_tile_type *a,
			     const struct cm_tile_type *b)
{
  output_type_iterate(stat) {
    if (a->production[stat] < b->production[stat])  {
      return FALSE;
    }
  } output_type_iterate_end;

  if (a->is_specialist && !b->is_specialist) {
    /* If A is a specialist and B isn't, and all of A's production is at
     * least as good as B's, then A is better because it doesn't tie up
     * the map tile. */
    return TRUE;
  } else if (!a->is_specialist && b->is_specialist) {
    /* Vice versa. */
    return FALSE;
  }

  return TRUE;
}

/****************************************************************************
  If there is a tile type in the vector that is equivalent to the given
  type, return its index.  If not, return -1.

  Equivalence is defined in tile_type_equal().
****************************************************************************/
static int tile_type_vector_find_equivalent(
				const struct tile_type_vector *vec,
				const struct cm_tile_type *ptype)
{
  int i;

  for (i = 0; i < vec->size; i++) {
    if (tile_type_equal(vec->p[i], ptype)) {
      return i;
    }
  }

  return -1;
}

/****************************************************************************
  Return the number of tiles of this type that can be worked.  For
  is_specialist types this will always be infinite but for other types of
  tiles it is limited by what's available in the citymap.
****************************************************************************/
static int tile_type_num_tiles(const struct cm_tile_type *type)
{
  if(type->is_specialist) {
    return FC_INFINITY;
  } else {
    return tile_vector_size(&type->tiles);
  }
}

/****************************************************************************
  Return the number of tile types that are better than this type.

  Note this isn't the same as the number of *tiles* that are better.  There
  may be more than one tile of each type (see tile_type_num_tiles).
****************************************************************************/
static int tile_type_num_prereqs(const struct cm_tile_type *ptype)
{
  return ptype->better_types.size;
}

/****************************************************************************
  Retrieve a tile type by index.  For a given state there are a certain
  number of tile types, which may be iterated over using this function
  as a lookup.
****************************************************************************/
static const struct cm_tile_type *tile_type_get(const struct cm_state *state,
						int type)
{
  /* Sanity check the index. */
  assert(type >= 0);
  assert(type < state->lattice.size);
  return state->lattice.p[type];
}

/****************************************************************************
  Retrieve a tile of a particular type by index.  For a given tile type
  there are a certain number of tiles (1 or more), which may be iterated
  over using this function for index.  Don't call this for is_specialist
  types.  See also tile_type_num_tiles().
****************************************************************************/
static const struct cm_tile *tile_get(const struct cm_tile_type *ptype, int j)
{
  assert(!ptype->is_specialist);
  assert(j >= 0);
  assert(j < ptype->tiles.size);
  return &ptype->tiles.p[j];
}


/**************************************************************************
  Functions on the cm_fitness struct.
**************************************************************************/

/****************************************************************************
  Return TRUE iff fitness A is strictly better than fitness B.
****************************************************************************/
static bool fitness_better(struct cm_fitness a, struct cm_fitness b)
{
  if (a.sufficient != b.sufficient) {
    return a.sufficient;
  }
  return a.weighted > b.weighted;
}

/****************************************************************************
  Return a fitness struct that is the worst possible result we can
  represent.
****************************************************************************/
static struct cm_fitness worst_fitness(void)
{
  struct cm_fitness f;

  f.sufficient = FALSE;
  f.weighted = -FC_INFINITY;
  return f;
}

/****************************************************************************
  Compute the fitness of the given surplus (and disorder/happy status)
  according to the weights and minimums given in the parameter.
****************************************************************************/
static struct cm_fitness compute_fitness(const int surplus[],
					 bool disorder, bool happy,
					const struct cm_parameter *parameter)
{
  struct cm_fitness fitness;

  fitness.sufficient = TRUE;
  fitness.weighted = 0;

  output_type_iterate(stat) {
    fitness.weighted += surplus[stat] * parameter->factor[stat];
    if (surplus[stat] < parameter->minimal_surplus[stat]) {
      fitness.sufficient = FALSE;
    }
  } output_type_iterate_end;

  if (happy) {
    fitness.weighted += parameter->happy_factor;
  } else if (parameter->require_happy) {
    fitness.sufficient = FALSE;
  }

  if (disorder && !parameter->allow_disorder) {
    fitness.sufficient = FALSE;
  }

  return fitness;
}

/***************************************************************************
  Handle struct partial_solution.
  - perform a deep copy
  - convert to city
  - convert to cm_result
 ***************************************************************************/

/****************************************************************************
  Allocate and initialize an empty solution.
****************************************************************************/
static void init_partial_solution(struct partial_solution *into,
                                  int ntypes, int idle)
{
  into->worker_counts = fc_calloc(ntypes, sizeof(*into->worker_counts));
  into->prereqs_filled = fc_calloc(ntypes, sizeof(*into->prereqs_filled));
  memset(into->production, 0, sizeof(into->production));
  into->idle = idle;
}

/****************************************************************************
  Free all storage associated with the solution.  This is basically the
  opposite of init_partial_solution.
****************************************************************************/
static void destroy_partial_solution(struct partial_solution *into)
{
  free(into->worker_counts);
  free(into->prereqs_filled);
}

/****************************************************************************
  Copy the source solution into the destination one (the destination
  solution must already be allocated).
****************************************************************************/
static void copy_partial_solution(struct partial_solution *dst,
				  const struct partial_solution *src,
				  const struct cm_state *state)
{
  memcpy(dst->worker_counts, src->worker_counts,
	 sizeof(*dst->worker_counts) * num_types(state));
  memcpy(dst->prereqs_filled, src->prereqs_filled,
	 sizeof(*dst->prereqs_filled) * num_types(state));
  memcpy(dst->production, src->production, sizeof(dst->production));
  dst->idle = src->idle;
}


/**************************************************************************
  Evaluating a completed solution.
**************************************************************************/

/****************************************************************************
  Apply the solution to the city_map[].

  Note: this function writes directly into the city_map[], unlike most
  other code that uses accessor functions.  The city_map[] is merely a
  scratch pad, and not applied here to the main map tiles.
****************************************************************************/
static void apply_solution(struct cm_state *state,
                           const struct partial_solution *soln)
{
  int i;
  int citizens = 0;
  struct city *pcity = state->pcity;

#ifdef GATHER_TIME_STATS
  performance.current->apply_count++;
#endif

  assert(soln->idle == 0);

  /* Clear all specialists, and remove all workers from fields (except
   * the city center). */
  memset(&pcity->specialists, 0, sizeof(pcity->specialists));

  city_map_iterate(x, y) {
    if (is_free_worked_cxy(x, y)) {
      continue;
    }

    if (C_TILE_WORKER == pcity->city_map[x][y]) {
      pcity->city_map[x][y] = C_TILE_EMPTY;
    }
  } city_map_iterate_end;

  /* Now for each tile type, find the right number of such tiles and set them
   * as worked.  For specialists we just increase the number of specialists
   * of that type. */
  for (i = 0 ; i < num_types(state); i++) {
    int nworkers = soln->worker_counts[i];
    const struct cm_tile_type *type;

    if (nworkers == 0) {
      /* No citizens of this type. */
      continue;
    }
    citizens += nworkers;

    type = tile_type_get(state, i);

    if (type->is_specialist) {
      /* Just increase the number of specialists. */
      pcity->specialists[type->spec] += nworkers;
    } else {
      int j;

      /* Place citizen workers onto the citymap tiles. */
      for (j = 0; j < nworkers; j++) {
        const struct cm_tile *tile = tile_get(type, j);

        pcity->city_map[tile->x][tile->y] = C_TILE_WORKER;
      }
    }
  }

  /* Finally we must refresh the city to reset all the precomputed fields. */
  city_refresh_from_main_map(pcity, FALSE); /* from city_map[] instead */
  assert(citizens == pcity->size);
}

/****************************************************************************
  Convert the city's surplus numbers into an array.  Get the happy/disorder
  values, too.  This fills in the surplus array and disorder and happy 
  values based on the city's data.
****************************************************************************/
static void get_city_surplus(const struct city *pcity,
			     int surplus[],
			     bool *disorder, bool *happy)
{
  output_type_iterate(o) {
    surplus[o] = pcity->surplus[o];
  } output_type_iterate_end;

  *disorder = city_unhappy(pcity);
  *happy = city_happy(pcity);
}

/****************************************************************************
  Compute the fitness of the solution.  This is a fairly expensive operation.
****************************************************************************/
static struct cm_fitness evaluate_solution(struct cm_state *state,
    const struct partial_solution *soln)
{
  struct city *pcity = state->pcity;
  struct city backup;
  int surplus[O_LAST];
  bool disorder, happy;

  /* make a backup, apply and evaluate the solution, and restore.  This costs
   * one "apply". */
  memcpy(&backup, pcity, sizeof(backup));
  apply_solution(state, soln);
  get_city_surplus(pcity, surplus, &disorder, &happy);
  memcpy(pcity, &backup, sizeof(backup));

  return compute_fitness(surplus, disorder, happy, &state->parameter);
}

/****************************************************************************
  Convert the solution into a cm_result.  This is a fairly expensive
  operation.
****************************************************************************/
static void convert_solution_to_result(struct cm_state *state,
				       const struct partial_solution *soln,
				       struct cm_result *result)
{
  struct city backup;
  struct cm_fitness fitness;

  if (soln->idle != 0) {
    /* If there are unplaced citizens it's not a real solution, so the
     * result is invalid. */
    result->found_a_valid = FALSE;
    return;
  }

  /* make a backup, apply and evaluate the solution, and restore */
  memcpy(&backup, state->pcity, sizeof(backup));
  apply_solution(state, soln);
  cm_result_from_main_map(result, state->pcity, FALSE);
  memcpy(state->pcity, &backup, sizeof(backup));

  /* result->found_a_valid should be only true if it matches the
     parameter ; figure out if it does */
  fitness = compute_fitness(result->surplus, result->disorder,
			    result->happy, &state->parameter);
  result->found_a_valid = fitness.sufficient;
}

/***************************************************************************
  Compare functions to allow sorting lattice vectors.
 ***************************************************************************/

/****************************************************************************
  All the sorting in this code needs to respect the partial order
  in the lattice: if a is a parent of b, then a must sort before b.
  This routine enforces that relatively cheaply (without looking through
  the worse_types vectors of a and b), but requires that lattice_depth
  has already been computed.
****************************************************************************/
static int compare_tile_type_by_lattice_order(const struct cm_tile_type *a,
					      const struct cm_tile_type *b)
{
  if (a == b) {
    return 0;
  }

  /* Least depth first */
  if (a->lattice_depth != b->lattice_depth) {
    return a->lattice_depth - b->lattice_depth;
  }

  /* With equal depth, break ties arbitrarily, more production first. */
  output_type_iterate(stat) {
    if (a->production[stat] != b->production[stat]) {
      return b->production[stat] - a->production[stat];
    }
  } output_type_iterate_end;

  /* If we get here, then we have two copies of identical types, an error */
  assert(0);
  return 0;
}

/****************************************************************************
  Sort by fitness.  Since fitness is monotone in the production,
  if a has higher fitness than b, then a cannot be a child of b, so
  this respects the partial order -- unless a and b have equal fitness.
  In that case, use compare_tile_type_by_lattice_order.
****************************************************************************/
static int compare_tile_type_by_fitness(const void *va, const void *vb)
{
  struct cm_tile_type * const *a = va;
  struct cm_tile_type * const *b = vb;
  double diff;

  if (*a == *b) {
    return 0;
  }

  /* To avoid double->int roundoff problems, we call a result non-zero only
   * if it's larger than 0.5. */
  diff = (*b)->estimated_fitness - (*a)->estimated_fitness;
  if (diff > 0.5) {
    return 1; /* return value is int; don't round down! */
  }
  if (diff < -0.5) {
    return -1;
  }

  return compare_tile_type_by_lattice_order(*a, *b);
}

static Output_type_id compare_key;

/****************************************************************************
  Compare by the production of type compare_key.
  If a produces more food than b, then a cannot be a child of b, so
  this respects the partial order -- unless a and b produce equal food.
  In that case, use compare_tile_type_by_lattice_order.
****************************************************************************/
static int compare_tile_type_by_stat(const void *va, const void *vb)
{
  struct cm_tile_type * const *a = va;
  struct cm_tile_type * const *b = vb;

  if (*a == *b) {
    return 0;
  }

  /* most production of what we care about goes first */
  if ((*a)->production[compare_key] != (*b)->production[compare_key]) {
    /* b-a so we sort big numbers first */
    return (*b)->production[compare_key] - (*a)->production[compare_key];
  }

  return compare_tile_type_by_lattice_order(*a, *b);
}

/***************************************************************************
  Compute the tile-type lattice.
 ***************************************************************************/

/****************************************************************************
  Compute the production of tile [x,y] and stuff it into the tile type.
  Doesn't touch the other fields.
****************************************************************************/
static void compute_tile_production(const struct city *pcity,
				    const struct tile *ptile,
				    struct cm_tile_type *out)
{
  bool is_celebrating = base_city_celebrating(pcity);

  output_type_iterate(o) {
    out->production[o] = city_tile_output(pcity, ptile, is_celebrating, o);
  } output_type_iterate_end;
}

/****************************************************************************
  Add the tile [x,y], with production indicated by type, to
  the tile-type lattice.  'newtype' can be on the stack.
  x/y are ignored if the type is a specialist.
  If the type is new, it is linked in and the lattice_index set.
  The lattice_depth is not set.
****************************************************************************/
static void tile_type_lattice_add(struct tile_type_vector *lattice,
				  const struct cm_tile_type *newtype,
				  int x, int y)
{
  struct cm_tile_type *type;
  int i;

  i = tile_type_vector_find_equivalent(lattice, newtype);
  if(i >= 0) {
    /* We already have this type of tile; use it. */
    type = lattice->p[i];
  } else {
    /* This is a new tile type; add it to the lattice. */
    type = tile_type_dup(newtype);

    /* link up to the types we dominate, and those that dominate us */
    tile_type_vector_iterate(lattice, other) {
      if (tile_type_better(other, type)) {
        tile_type_vector_add(&type->better_types, other);
        tile_type_vector_add(&other->worse_types, type);
      } else if (tile_type_better(type, other)) {
        tile_type_vector_add(&other->better_types, type);
        tile_type_vector_add(&type->worse_types, other);
      }
    } tile_type_vector_iterate_end;

    /* insert into the list */
    type->lattice_index = lattice->size;
    tile_type_vector_add(lattice, type);
  }

  /* Finally, add the tile to the tile type. */
  if (!type->is_specialist) {
    struct cm_tile tile;

    tile.type = type;
    tile.x = x;
    tile.y = y;
    tile_vector_append(&type->tiles, &tile);
  }
}

/*
 * Add the specialist types to the lattice.
 */

/****************************************************************************
  Create lattice nodes for each type of specialist.  This adds a new
  tile_type for each specialist type.
****************************************************************************/
static void init_specialist_lattice_nodes(struct tile_type_vector *lattice,
					  const struct city *pcity)
{
  struct cm_tile_type type;

  tile_type_init(&type);
  type.is_specialist = TRUE;

  /* for each specialist type, create a tile_type that has as production
   * the bonus for the specialist (if the city is allowed to use it) */
  specialist_type_iterate(i) {
    if (city_can_use_specialist(pcity, i)) {
      type.spec = i;
      output_type_iterate(output) {
	type.production[output] = get_specialist_output(pcity, i, output);
      } output_type_iterate_end;

      tile_type_lattice_add(lattice, &type, 0, 0);
    }
  } specialist_type_iterate_end;
}

/****************************************************************************
  Topologically sort the lattice.
  Sets the lattice_depth field.
  Important assumption in this code: we've computed the transitive
  closure of the lattice. That is, better_types includes all types that
  are better.
****************************************************************************/
static void top_sort_lattice(struct tile_type_vector *lattice)
{
  int i;
  bool marked[lattice->size];
  bool will_mark[lattice->size];
  struct tile_type_vector vectors[2];
  struct tile_type_vector *current, *next;

  memset(marked, 0, sizeof(marked));
  memset(will_mark, 0, sizeof(will_mark));

  tile_type_vector_init(&vectors[0]);
  tile_type_vector_init(&vectors[1]);
  current = &vectors[0];
  next = &vectors[1];

  /* fill up 'next' */
  tile_type_vector_iterate(lattice, ptype) {
    if (tile_type_num_prereqs(ptype) == 0) {
      tile_type_vector_add(next, ptype);
    }
  } tile_type_vector_iterate_end;

  /* while we have nodes to process: mark the nodes whose prereqs have
   * all been visited.  Then, store all the new nodes on the frontier. */
  while (next->size != 0) {
    /* what was the next frontier is now the current frontier */
    struct tile_type_vector *vtmp = current;

    current = next;
    next = vtmp;
    next->size = 0; /* clear out the contents */

    /* look over the current frontier and process everyone */
    tile_type_vector_iterate(current, ptype) {
      /* see if all prereqs were marked.  If so, decide to mark this guy,
         and put all the descendents on 'next'.  */
      bool can_mark = TRUE;
      int sumdepth = 0;

      if (will_mark[ptype->lattice_index]) {
        continue; /* we've already been processed */
      }
      tile_type_vector_iterate(&ptype->better_types, better) {
        if (!marked[better->lattice_index]) {
          can_mark = FALSE;
          break;
        } else {
          sumdepth += tile_type_num_tiles(better);
          if (sumdepth >= FC_INFINITY) {
            /* if this is the case, then something better could
               always be used, and the same holds for our children */
            sumdepth = FC_INFINITY;
            can_mark = TRUE;
            break;
          }
        }
      } tile_type_vector_iterate_end;
      if (can_mark) {
        /* mark and put successors on the next frontier */
        will_mark[ptype->lattice_index] = TRUE;
        tile_type_vector_iterate(&ptype->worse_types, worse) {
          tile_type_vector_add(next, worse);
        } tile_type_vector_iterate_end;

        /* this is what we spent all this time computing. */
        ptype->lattice_depth = sumdepth;
      }
    } tile_type_vector_iterate_end;

    /* now, actually mark everyone and get set for next loop */
    for (i = 0; i < lattice->size; i++) {
      marked[i] = marked[i] || will_mark[i];
      will_mark[i] = FALSE;
    }
  }

  tile_type_vector_free(&vectors[0]);
  tile_type_vector_free(&vectors[1]);
}

/****************************************************************************
  Remove unreachable lattice nodes to speed up processing later.
  This doesn't reduce the number of evaluations we do, it just
  reduces the number of times we loop over and reject tile types
  we can never use.

  A node is unreachable if there are fewer available workers
  than are needed to fill up all predecessors.  A node at depth
  two needs three workers to be reachable, for example (two to fill
  the predecessors, and one for the tile).  We remove a node if
  its depth is equal to the city size, or larger.

  We could clean up the tile arrays in each type (if we have two workers,
  we can use only the first tile of a depth 1 tile type), but that
  wouldn't save us anything later.
****************************************************************************/
static void clean_lattice(struct tile_type_vector *lattice,
			  const struct city *pcity)
{
  int i, j; /* i is the index we read, j is the index we write */
  struct tile_type_vector tofree;
  bool forced_loop = FALSE;

  /* We collect the types we want to remove and free them in one fell 
     swoop at the end, in order to avoid memory errors.  */
  tile_type_vector_init(&tofree);

  /* forced_loop is workaround for what seems like gcc optimization
   * bug.
   * This applies to -O2 optimization on some distributions. */
  if (lattice->size > 0) {
    forced_loop = TRUE;
  }
  for (i = 0, j = 0; i < lattice->size || forced_loop; i++) {
    struct cm_tile_type *ptype = lattice->p[i];

    forced_loop = FALSE;

    if (ptype->lattice_depth >= pcity->size) {
      tile_type_vector_add(&tofree, ptype);
    } else {
      /* Remove links to children that are being removed. */

      int ci, cj; /* 'c' for 'child'; read from ci, write to cj */

      lattice->p[j] = ptype;
      lattice->p[j]->lattice_index = j;
      j++;

      for (ci = 0, cj = 0; ci < ptype->worse_types.size; ci++) {
        const struct cm_tile_type *ptype2 = ptype->worse_types.p[ci];

        if (ptype2->lattice_depth < pcity->size) {
          ptype->worse_types.p[cj] = ptype->worse_types.p[ci];
          cj++;
        }
      }
      ptype->worse_types.size = cj;
    }
  }
  lattice->size = j;

  tile_type_vector_free_all(&tofree);
}

/****************************************************************************
  Determine the estimated_fitness fields, and sort by that.
  estimate_fitness is later, in a section of code that isolates
  much of the domain-specific knowledge.
****************************************************************************/
static double estimate_fitness(const struct cm_state *state,
			       const int production[]);

static void sort_lattice_by_fitness(const struct cm_state *state,
				    struct tile_type_vector *lattice)
{
  int i;

  /* compute fitness */
  tile_type_vector_iterate(lattice, ptype) {
    ptype->estimated_fitness = estimate_fitness(state, ptype->production);
  } tile_type_vector_iterate_end;

  /* sort by it */
  qsort(lattice->p, lattice->size, sizeof(*lattice->p),
	compare_tile_type_by_fitness);

  /* fix the lattice indices */
  for (i = 0; i < lattice->size; i++) {
    lattice->p[i]->lattice_index = i;
  }

  freelog(LOG_LATTICE, "sorted lattice:");
  print_lattice(LOG_LATTICE, lattice);
}

/****************************************************************************
  Create the lattice and update related city_map[].

  Note: this function writes directly into the city_map[], unlike most
  other code that uses accessor functions.  The city_map[] is merely a
  scratch pad, and not assumed to accurately reflect main map tiles.
****************************************************************************/
static void init_tile_lattice(struct city *pcity,
			      struct tile_type_vector *lattice)
{
  struct cm_tile_type type;
  struct tile *pcenter = city_tile(pcity);

  /* add all the fields into the lattice */
  tile_type_init(&type); /* init just once */

  city_tile_iterate_cxy(pcenter, ptile, x, y) {
    if (is_free_worked(pcity, ptile)) {
      pcity->city_map[x][y] = C_TILE_WORKER;
      continue;
    } else if (tile_worked(ptile) == pcity) {
      /* is currently worked, but actually may be unavailable */
      pcity->city_map[x][y] = C_TILE_WORKER;
    } else if (city_can_work_tile(pcity, ptile)) {
      pcity->city_map[x][y] = C_TILE_EMPTY;
    } else {
      pcity->city_map[x][y] = C_TILE_UNAVAILABLE;
      continue;
    }

    compute_tile_production(pcity, ptile, &type); /* clobbers type */
    tile_type_lattice_add(lattice, &type, x, y); /* copy type if needed */
  } city_tile_iterate_cxy_end;

  /* Add all the specialists into the lattice.  */
  init_specialist_lattice_nodes(lattice, pcity);

  /* Set the lattice_depth fields, and clean up unreachable nodes. */
  top_sort_lattice(lattice);
  clean_lattice(lattice, pcity);

  /* All done now. */
  print_lattice(LOG_LATTICE, lattice);
}


/****************************************************************************

               Handling the choice stack for the bb algorithm.

****************************************************************************/


/****************************************************************************
  Return TRUE iff the stack is empty.
****************************************************************************/
static bool choice_stack_empty(struct cm_state *state)
{
  return state->choice.size == 0;
}

/****************************************************************************
  Return the last choice in the stack.
****************************************************************************/
static int last_choice(struct cm_state *state)
{
  assert(!choice_stack_empty(state));
  return state->choice.stack[state->choice.size - 1];
}

/****************************************************************************
  Return the number of different tile types.  There is one tile type for
  each type specialist, plus one for each distinct (different amounts of
  production) citymap tile.
****************************************************************************/
static int num_types(const struct cm_state *state)
{
  return tile_type_vector_size(&state->lattice);
}

/****************************************************************************
  Update the solution to assign 'number' more workers on to tiles of the
  given type.  'number' may be negative, in which case we're removing
  workers.
  We do lots of sanity checking, since many bugs can get caught here.
****************************************************************************/
static void add_workers(struct partial_solution *soln,
			int itype, int number,
			const struct cm_state *state)
{
  const struct cm_tile_type *ptype = tile_type_get(state, itype);
  int newcount;
  int old_worker_count = soln->worker_counts[itype];

  if (number == 0) {
    return;
  }

  /* update the number of idle workers */
  newcount = soln->idle - number;
  assert(newcount >= 0);
  assert(newcount <= state->pcity->size);
  soln->idle = newcount;

  /* update the worker counts */
  newcount = soln->worker_counts[itype] + number;
  assert(newcount >= 0);
  assert(newcount <= tile_type_num_tiles(ptype));
  soln->worker_counts[itype] = newcount;

  /* update prereqs array: if we are no longer full but we were,
   * we need to decrement the count, and vice-versa. */
  if (old_worker_count == tile_type_num_tiles(ptype)) {
    assert(number < 0);
    tile_type_vector_iterate(&ptype->worse_types, other) {
      soln->prereqs_filled[other->lattice_index]--;
      assert(soln->prereqs_filled[other->lattice_index] >= 0);
    } tile_type_vector_iterate_end;
  } else if (soln->worker_counts[itype] == tile_type_num_tiles(ptype)) {
    assert(number > 0);
    tile_type_vector_iterate(&ptype->worse_types, other) {
      soln->prereqs_filled[other->lattice_index]++;
      assert(soln->prereqs_filled[other->lattice_index]
          <= tile_type_num_prereqs(other));
    } tile_type_vector_iterate_end;
  }

  /* update production */
  output_type_iterate(stat) {
    newcount = soln->production[stat] + number * ptype->production[stat];
    assert(newcount >= 0);
    soln->production[stat] = newcount;
  } output_type_iterate_end;
}

/****************************************************************************
  Add just one worker to the solution.
****************************************************************************/
static void add_worker(struct partial_solution *soln,
		       int itype, const struct cm_state *state)
{
  add_workers(soln, itype, 1, state);
}

/****************************************************************************
  Remove just one worker from the solution.
****************************************************************************/
static void remove_worker(struct partial_solution *soln,
			  int itype, const struct cm_state *state)
{
  add_workers(soln, itype, -1, state);
}

/****************************************************************************
  Remove a worker from the current solution, and pop once off the
  choice stack.
****************************************************************************/
static void pop_choice(struct cm_state *state)
{
  assert(!choice_stack_empty(state));
  remove_worker(&state->current, last_choice(state), state);
  state->choice.size--;
}

/****************************************************************************
  True if all tiles better than this type have been used.
****************************************************************************/
static bool prereqs_filled(const struct partial_solution *soln, int type,
			   const struct cm_state *state)
{
  const struct cm_tile_type *ptype = tile_type_get(state, type);
  int prereqs = tile_type_num_prereqs(ptype);

  return soln->prereqs_filled[type] == prereqs;
}

/****************************************************************************
  Return the next choice to make after oldchoice.
  A choice can be made if:
  - we haven't used all the tiles
  - we've used up all the better tiles
  - using that choice, we have a hope of doing better than the best
    solution so far.
  If oldchoice == -1 then we return the first possible choice.
****************************************************************************/
static bool choice_is_promising(struct cm_state *state, int newchoice);

static int next_choice(struct cm_state *state, int oldchoice)
{
  int newchoice;

  for (newchoice = oldchoice + 1;
       newchoice < num_types(state); newchoice++) {
    const struct cm_tile_type *ptype = tile_type_get(state, newchoice);

    if(!ptype->is_specialist && (state->current.worker_counts[newchoice]
				 == tile_vector_size(&ptype->tiles))) {
      /* we've already used all these tiles */
      continue;
    }
    if (!prereqs_filled(&state->current, newchoice, state)) {
      /* we could use a strictly better tile instead */
      continue;
    }
    if (!choice_is_promising(state, newchoice)) {
      /* heuristic says we can't beat the best going this way */
      freelog(LOG_PRUNE_BRANCH, "--- pruning branch ---");
      print_partial_solution(LOG_PRUNE_BRANCH, &state->current, state);
      print_tile_type(LOG_PRUNE_BRANCH, tile_type_get(state, newchoice),
          " + worker on ");
      freelog(LOG_PRUNE_BRANCH, "--- branch pruned ---");
      continue;
    }
    break;
  }

  /* returns num_types if no next choice was available. */
  return newchoice;
}


/****************************************************************************
  Pick a sibling choice to the last choice.  This works down the branch to
  see if a choice that actually looks worse may actually be better.
****************************************************************************/
static bool take_sibling_choice(struct cm_state *state)
{
  int oldchoice = last_choice(state);
  int newchoice;

  /* need to remove first, to run the heuristic */
  remove_worker(&state->current, oldchoice, state);
  newchoice = next_choice(state, oldchoice);

  if (newchoice == num_types(state)) {
    /* add back in so the caller can then remove it again. */
    add_worker(&state->current, oldchoice, state);
    return FALSE;
  } else {
    add_worker(&state->current, newchoice, state);
    state->choice.stack[state->choice.size-1] = newchoice;
    /* choice.size is unchanged */
    return TRUE;
  }
}

/****************************************************************************
  Go down from the current branch, if we can.
  Thanks to the fact that the lattice is sorted by depth, we can keep the
  choice stack sorted -- that is, we can start our next choice as
  last_choice-1.  This keeps us from trying out all permutations of the
  same combination.
****************************************************************************/
static bool take_child_choice(struct cm_state *state)
{
  int oldchoice, newchoice;

  if (state->current.idle == 0) {
    return FALSE;
  }

  if (state->choice.size == 0) {
    oldchoice = 0;
  } else {
    oldchoice = last_choice(state);
  }

  /* oldchoice-1 because we can use oldchoice again */
  newchoice = next_choice(state, oldchoice - 1);

  /* did we fail? */
  if (newchoice == num_types(state)) {
    return FALSE;
  }

  /* now push the new choice on the choice stack */
  add_worker(&state->current, newchoice, state);
  state->choice.stack[state->choice.size] = newchoice;
  state->choice.size++;
  return TRUE;
}


/****************************************************************************
  Complete the solution by choosing tiles in order off the given
  tile lattice.
****************************************************************************/
static void complete_solution(struct partial_solution *soln,
			      const struct cm_state *state,
			      const struct tile_type_vector *lattice)
{
  int last_choice = -1;
  int i;

  if (soln->idle == 0) {
    return;
  }

  /* find the last worker type added (-1 if none) */
  for (i = 0; i < num_types(state); i++) {
    if (soln->worker_counts[i] != 0) {
      last_choice = i;
    }
  }

  /* While there are idle workers, pick up the next-best kind of tile,
   * and assign the workers to the tiles.
   * Respect lexicographic order and prerequisites.  */
  tile_type_vector_iterate(lattice, ptype) {
    int used = soln->worker_counts[ptype->lattice_index];
    int total = tile_type_num_tiles(ptype);
    int touse;

    if (ptype->lattice_index < last_choice) {
      /* lex-order: we can't use ptype (some other branch
         will check this combination, or already did) */
	continue;
    }
    if (!prereqs_filled(soln, ptype->lattice_index, state)) {
      /* don't bother using this tile before all better tiles are used */
      continue;
    }

    touse = total - used;
    if (touse > soln->idle) {
      touse = soln->idle;
    }
    add_workers(soln, ptype->lattice_index, touse, state);

    if (soln->idle == 0) {
      /* nothing left to do here */
      return;
    }
  } tile_type_vector_iterate_end;
}

/****************************************************************************
  The heuristic:
  A partial solution cannot produce more food than the amount of food it
  currently generates plus then placing all its workers on the best food
  tiles.  Similarly with all the other stats.
  If we take the max along each production, and it's not better than the
  best in at least one stat, the partial solution isn't worth anything.

  This function computes the max-stats produced by a partial solution.
****************************************************************************/
static void compute_max_stats_heuristic(const struct cm_state *state,
					const struct partial_solution *soln,
					int production[],
					int check_choice)
{
  struct partial_solution solnplus; /* will be soln, plus some tiles */

  /* Production is whatever the solution produces, plus the
     most possible of each kind of production the idle workers could
     produce */

  if (soln->idle == 1) {
    /* Then the total solution is soln + this new worker.  So we know the
       production exactly, and can shortcut the later code. */
    const struct cm_tile_type *ptype = tile_type_get(state, check_choice);

    memcpy(production, soln->production, sizeof(soln->production));
    output_type_iterate(stat) {
      production[stat] += ptype->production[stat];
    } output_type_iterate_end;
    return;
  }

  /* initialize solnplus here, after the shortcut check */
  init_partial_solution(&solnplus, num_types(state), state->pcity->size);

  output_type_iterate(stat) {
    /* compute the solution that has soln, then the check_choice,
       then complete it with the best available tiles for the stat. */
    copy_partial_solution(&solnplus, soln, state);
    add_worker(&solnplus, check_choice, state);
    complete_solution(&solnplus, state, &state->lattice_by_prod[stat]);

    production[stat] = solnplus.production[stat];
  } output_type_iterate_end;

  destroy_partial_solution(&solnplus);
}

/****************************************************************************
  A choice is unpromising if isn't better than the best in at least
  one way.
  A choice is also unpromising if any of the stats is less than the
  absolute minimum (in practice, this matters a lot more).
****************************************************************************/
static bool choice_is_promising(struct cm_state *state, int newchoice)
{
  int production[O_LAST];
  bool beats_best = FALSE;

  compute_max_stats_heuristic(state, &state->current, production, newchoice);

  output_type_iterate(stat) {
    if (production[stat] < state->min_production[stat]) {
      freelog(LOG_PRUNE_BRANCH, "--- pruning: insufficient %s (%d < %d)",
	      get_output_name(stat), production[stat],
	      state->min_production[stat]);
      return FALSE;
    }
    if (production[stat] > state->best.production[stat]) {
      beats_best = TRUE;
      /* may still fail to meet min at another production type, so
       * don't short-circuit */
    }
  } output_type_iterate_end;
  if (!beats_best) {
    freelog(LOG_PRUNE_BRANCH, "--- pruning: best is better in all ways");
  }
  return beats_best;
}

/****************************************************************************
  These two functions are very specific for the default civ2-like ruleset.
  These require the parameter to have been set.
  FIXME -- this should be more general.
****************************************************************************/
static void init_min_production(struct cm_state *state)
{
  struct city *pcity = state->pcity;
  struct tile *pcenter = city_tile(pcity);
  bool is_celebrating = base_city_celebrating(pcity);

  memset(state->min_production, 0, sizeof(state->min_production));
  output_type_iterate(o) {
    /* Calculate minimum output.  This assumes no waste.  Pruning
     * is done mainly based on minimum production, so we want to find the
     * absolute highest minimum possible.  However if the minimum we find
     * here is incorrectly too high, then the algorithm will fail. */
    int min;

    if (o != O_SHIELD && o != O_FOOD) {
      /* min-production calculations are only possible for food and
       * shields.  The others depend on complex trade calculations
       * that cannot be accounted for since the bonus from trade
       * routes depends on the amount of trade in an unpredictable way. */
      continue;
    }

    /* 1.  Calculate the minimum final production that is needed.
     * 2.  Divide by the bonus (rounding down) to get the minimum citizen
     *     production that is needed.
     * 3.  Subtract off any "free" production (trade routes, tithes, and
     *     city-center). */
    min = pcity->usage[o] + state->parameter.minimal_surplus[o];
    if (pcity->bonus[o] > 0) {
      /* FIXME: how does this work if the bonus is 0?  Then no production
       * is possible and we can probably short-cut everything. */
      min = min * 100 / pcity->bonus[o];
    }

    city_tile_iterate(pcenter, ptile) {
      if (is_free_worked(pcity, ptile)) {
	min -= city_tile_output(pcity, ptile, is_celebrating, o);
      }
    } city_tile_iterate_end;

    state->min_production[o] = MAX(min, 0);
  } output_type_iterate_end;

  /* We could get a minimum on luxury if we knew how many luxuries were
   * needed to make us content. */
}

/****************************************************************************
  Estimate the fitness of a tile.  Tiles are put into the lattice in
  fitness order, so that we start off choosing better tiles.
  The estimate MUST be monotone in the inputs; if it isn't, then
  the BB algorithm will fail.

  The only fields of the state used are the city and parameter.
****************************************************************************/
static double estimate_fitness(const struct cm_state *state,
			       const int production[]) {
  const struct city *pcity = state->pcity;
  const struct player *pplayer = city_owner(pcity);
  double estimates[O_LAST];
  double sum = 0;

  output_type_iterate(stat) {
    estimates[stat] = production[stat];
  } output_type_iterate_end;

  /* sci/lux/gold get benefit from the tax rates (in percentage) */
  estimates[O_SCIENCE]
    += pplayer->economic.science * estimates[O_TRADE] / 100.0;
  estimates[O_LUXURY]
    += pplayer->economic.luxury * estimates[O_TRADE] / 100.0;
  estimates[O_GOLD]
    += pplayer->economic.tax * estimates[O_TRADE] / 100.0;

  /* now add in the bonuses from building effects (in percentage) */
  output_type_iterate(stat) {
    estimates[stat] *= pcity->bonus[stat] / 100.0;
  } output_type_iterate_end;

  /* finally, sum it all up, weighted by the parameter, but give additional
   * weight to luxuries to take account of disorder/happy constraints */
  output_type_iterate(stat) {
    sum += estimates[stat] * state->parameter.factor[stat];
  } output_type_iterate_end;
  sum += estimates[O_LUXURY];
  return sum;
}



/****************************************************************************
  The high-level algorithm is:

  for each idle worker,
      non-deterministically choose a position for the idle worker to use

  To implement this, we keep a stack of the choices we've made.
  When we want the next node in the tree, we see if there are any idle
  workers left.  We push an idle worker, and make it take the first field
  in the lattice.  If there are no idle workers left, then we pop out
  until we can make another choice.
****************************************************************************/
static bool bb_next(struct cm_state *state)
{
  /* if no idle workers, then look at our solution. */
  if (state->current.idle == 0) {
    struct cm_fitness value = evaluate_solution(state, &state->current);

    print_partial_solution(LOG_REACHED_LEAF, &state->current, state);
    if (fitness_better(value, state->best_value)) {
      freelog(LOG_BETTER_LEAF, "-> replaces previous best");
      copy_partial_solution(&state->best, &state->current, state);
      state->best_value = value;
    }
  }

  /* try to move to a child branch, if we can.  If not (including if we're
     at a leaf), then move to a sibling. */
  if (!take_child_choice(state)) {
    /* keep trying to move to a sibling branch, or popping out a level if
       we're stuck (fully examined the current branch) */
    while ((!choice_stack_empty(state)) && !take_sibling_choice(state)) {
      pop_choice(state);
    }

    /* if we popped out all the way, we're done */
    if (choice_stack_empty(state)) {
      return TRUE;
    }
  }

  /* if we didn't detect that we were done, we aren't */
  return FALSE;
}

/****************************************************************************
  Initialize the state for the branch-and-bound algorithm.
****************************************************************************/
static struct cm_state *cm_init_state(struct city *pcity)
{
  int numtypes;
  struct cm_state *state = fc_malloc(sizeof(*state));

  freelog(LOG_CM_STATE, "creating cm_state for %s (size %d)",
	  city_name(pcity), pcity->size);

  /* copy the arguments */
  state->pcity = pcity;

  /* create the lattice */
  tile_type_vector_init(&state->lattice);
  init_tile_lattice(pcity, &state->lattice);
  numtypes = tile_type_vector_size(&state->lattice);

  /* For the heuristic, make sorted copies of the lattice */
  output_type_iterate(stat) {
    tile_type_vector_init(&state->lattice_by_prod[stat]);
    tile_type_vector_copy(&state->lattice_by_prod[stat], &state->lattice);
    compare_key = stat;
    qsort(state->lattice_by_prod[stat].p, state->lattice_by_prod[stat].size,
	  sizeof(*state->lattice_by_prod[stat].p),
	  compare_tile_type_by_stat);
  } output_type_iterate_end;

  /* We have no best solution yet, so its value is the worst possible. */
  init_partial_solution(&state->best, numtypes, pcity->size);
  state->best_value = worst_fitness();

  /* Initialize the current solution and choice stack to empty */
  init_partial_solution(&state->current, numtypes, pcity->size);
  state->choice.stack = fc_malloc(pcity->size
				  * sizeof(*state->choice.stack));
  state->choice.size = 0;

  return state;
}

/****************************************************************************
  Set the parameter for the state.  This is the first step in actually
  solving anything.
****************************************************************************/
static void begin_search(struct cm_state *state,
			 const struct cm_parameter *parameter)
{
#ifdef GATHER_TIME_STATS
  start_timer(performance.current->wall_timer);
  performance.current->query_count++;
#endif

  /* copy the parameter and sort the main lattice by it */
  cm_copy_parameter(&state->parameter, parameter);
  sort_lattice_by_fitness(state, &state->lattice);
  init_min_production(state);

  /* clear out the old solution */
  state->best_value = worst_fitness();
  destroy_partial_solution(&state->current);
  init_partial_solution(&state->current, num_types(state),
			state->pcity->size);
  state->choice.size = 0;
}


/****************************************************************************
  Clean up after a search.
  Currently, does nothing except stop the timer and output.
****************************************************************************/
static void end_search(struct cm_state *state)
{
#ifdef GATHER_TIME_STATS
  stop_timer(performance.current->wall_timer);

#ifdef PRINT_TIME_STATS_EVERY_QUERY
  print_performance(performance.current);
#endif

  performance.current = NULL;
#endif
}

/****************************************************************************
  Release all the memory allocated by the state.
****************************************************************************/
static void cm_free_state(struct cm_state *state)
{
  tile_type_vector_free_all(&state->lattice);
  output_type_iterate(stat) {
    tile_type_vector_free(&state->lattice_by_prod[stat]);
  } output_type_iterate_end;
  destroy_partial_solution(&state->best);
  destroy_partial_solution(&state->current);
  free(state->choice.stack);
  free(state);
}


/****************************************************************************
  Run B&B until we find the best solution.
****************************************************************************/
static void cm_find_best_solution(struct cm_state *state,
                                  const struct cm_parameter *const parameter,
                                  struct cm_result *result)
{
#ifdef GATHER_TIME_STATS
  performance.current = &performance.opt;
#endif

  begin_search(state, parameter);

  /* search until we find a feasible solution */
  while (!bb_next(state)) {
    /* nothing */
  }

  /* convert to the caller's format */
  convert_solution_to_result(state, &state->best, result);

  end_search(state);
}

/***************************************************************************
  Wrapper that actually runs the branch & bound, and returns the best
  solution.
 ***************************************************************************/
void cm_query_result(struct city *pcity,
		     const struct cm_parameter *param,
		     struct cm_result *result)
{
  struct cm_state *state = cm_init_state(pcity);

  /* Refresh the city.  Otherwise the CM can give wrong results or just be
   * slower than necessary.  Note that cities are often passed in in an
   * unrefreshed state (which should probably be fixed). */
  city_refresh_from_main_map(pcity, TRUE);

  cm_find_best_solution(state, param, result);
  cm_free_state(state);
}

/**************************************************************************
  Returns true if the two cm_parameters are equal.
**************************************************************************/
bool cm_are_parameter_equal(const struct cm_parameter *const p1,
			    const struct cm_parameter *const p2)
{
  output_type_iterate(i) {
    if (p1->minimal_surplus[i] != p2->minimal_surplus[i]) {
      return FALSE;
    }
    if (p1->factor[i] != p2->factor[i]) {
      return FALSE;
    }
  } output_type_iterate_end;
  if (p1->require_happy != p2->require_happy) {
    return FALSE;
  }
  if (p1->allow_disorder != p2->allow_disorder) {
    return FALSE;
  }
  if (p1->allow_specialists != p2->allow_specialists) {
    return FALSE;
  }
  if (p1->happy_factor != p2->happy_factor) {
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  Copy the parameter from the source to the destination field.
**************************************************************************/
void cm_copy_parameter(struct cm_parameter *dest,
		       const struct cm_parameter *const src)
{
  memcpy(dest, src, sizeof(struct cm_parameter));
}

/**************************************************************************
  Initialize the parameter to sane default values.
**************************************************************************/
void cm_init_parameter(struct cm_parameter *dest)
{
  output_type_iterate(stat) {
    dest->minimal_surplus[stat] = 0;
    dest->factor[stat] = 1;
  } output_type_iterate_end;

  dest->happy_factor = 1;
  dest->require_happy = FALSE;
  dest->allow_disorder = FALSE;
  dest->allow_specialists = TRUE;
}

/***************************************************************************
  Initialize the parameter to sane default values that will always produce
  a result.
***************************************************************************/
void cm_init_emergency_parameter(struct cm_parameter *dest)
{
  output_type_iterate(stat) {
    dest->minimal_surplus[stat] = -FC_INFINITY;
    dest->factor[stat] = 1;
  } output_type_iterate_end;

  dest->happy_factor = 1;
  dest->require_happy = FALSE;
  dest->allow_disorder = TRUE;
  dest->allow_specialists = TRUE;
}

/****************************************************************************
  Count the total number of workers in the result.
****************************************************************************/
int cm_result_workers(const struct cm_result *result)
{
  int count = 0;

  city_map_iterate(x, y) {
    if (is_free_worked_cxy(x, y)) {
      continue;
    }

    if (result->worker_positions_used[x][y]) {
      count++;
    }
  } city_map_iterate_end;

  return count;
}

/****************************************************************************
  Count the total number of specialists in the result.
****************************************************************************/
int cm_result_specialists(const struct cm_result *result)
{
  int count = 0;

  specialist_type_iterate(spec) {
    count += result->specialists[spec];
  } specialist_type_iterate_end;

  return count;
}

/****************************************************************************
  Count the total number of citizens in the result.
****************************************************************************/
int cm_result_citizens(const struct cm_result *result)
{
  return cm_result_workers(result) + cm_result_specialists(result);
}

/****************************************************************************
  Copy the city's current setup into the cm result structure.
****************************************************************************/
void cm_result_from_main_map(struct cm_result *result,
                             const struct city *pcity, bool main_map)
{
  struct tile *pcenter = city_tile(pcity);

  memset(result->worker_positions_used, 0,
	 sizeof(result->worker_positions_used));

  city_tile_iterate_cxy(pcenter, ptile, x, y) {
    if (main_map) {
      struct city *pwork = tile_worked(ptile);

      result->worker_positions_used[x][y] =
        (NULL != pwork && pwork == pcity);
    } else {
      result->worker_positions_used[x][y] =
        (C_TILE_WORKER == pcity->city_map[x][y]);
    }
  } city_tile_iterate_cxy_end;

  /* copy the specialist counts */
  specialist_type_iterate(spec) {
    result->specialists[spec] = pcity->specialists[spec];
  } specialist_type_iterate_end;

  /* find the surplus production numbers */
  get_city_surplus(pcity, result->surplus,
      &result->disorder, &result->happy);

  /* this is a valid result, in a sense */
  result->found_a_valid = TRUE;
}

/****************************************************************************
  Debugging routines.
****************************************************************************/
#ifdef CM_DEBUG
static void snprint_production(char *buffer, size_t bufsz,
			       const int production[])
{
  my_snprintf(buffer, bufsz, "[%d %d %d %d %d %d]",
	      production[O_FOOD], production[O_SHIELD],
	      production[O_TRADE], production[O_GOLD],
	      production[O_LUXURY], production[O_SCIENCE]);
}

/****************************************************************************
  Print debugging data about a particular tile type.
****************************************************************************/
static void print_tile_type(int loglevel, const struct cm_tile_type *ptype,
			    const char *prefix)
{
  char prodstr[256];

  snprint_production(prodstr, sizeof(prodstr), ptype->production);
  freelog(loglevel, "%s%s fitness %g depth %d, idx %d; %d tiles", prefix,
	  prodstr, ptype->estimated_fitness, ptype->lattice_depth,
	  ptype->lattice_index, tile_type_num_tiles(ptype));
}

/****************************************************************************
  Print debugging data about a whole B&B lattice.
****************************************************************************/
static void print_lattice(int loglevel,
			  const struct tile_type_vector *lattice)
{
  freelog(loglevel, "lattice has %u terrain types", (unsigned)lattice->size);
  tile_type_vector_iterate(lattice, ptype) {
    print_tile_type(loglevel, ptype, "  ");
  } tile_type_vector_iterate_end;
}

/****************************************************************************
  Print debugging data about a partial CM solution.
****************************************************************************/
static void print_partial_solution(int loglevel,
				   const struct partial_solution *soln,
				   const struct cm_state *state)
{
  int i;
  int last_type = 0;
  char buf[256];

  if(soln->idle != 0) {
    freelog(loglevel, "** partial solution has %d idle workers", soln->idle);
  } else {
    freelog(loglevel, "** completed solution:");
  }

  snprint_production(buf, sizeof(buf), soln->production);
  freelog(loglevel, "production: %s", buf);

  freelog(loglevel, "tiles used:");
  for (i = 0; i < num_types(state); i++) {
    if (soln->worker_counts[i] != 0) {
      my_snprintf(buf, sizeof(buf), "  %d tiles of type ",
		  soln->worker_counts[i]);
      print_tile_type(loglevel, tile_type_get(state, i), buf);
    }
  }

  for (i = 0; i < num_types(state); i++) {
    if (soln->worker_counts[i] != 0) {
      last_type = i;
    }
  }

  freelog(loglevel, "tiles available:");
  for (i = last_type; i < num_types(state); i++) {
    const struct cm_tile_type *ptype = tile_type_get(state, i);

    if (soln->prereqs_filled[i] == tile_type_num_prereqs(ptype)
	&& soln->worker_counts[i] < tile_type_num_tiles(ptype)) {
      print_tile_type(loglevel, tile_type_get(state, i), "  ");
    }
  }
}

#endif /* CM_DEBUG */

#ifdef GATHER_TIME_STATS
/****************************************************************************
  Print debugging performance data.
****************************************************************************/
static void print_performance(struct one_perf *counts)
{
  double s, ms;
  double q;
  int queries, applies;

  s = read_timer_seconds(counts->wall_timer);
  ms = 1000.0 * s;

  queries = counts->query_count;
  q = queries;

  applies = counts->apply_count;

  freelog(LOG_TIME_STATS,
      "CM-%s: overall=%fs queries=%d %fms / query, %d applies",
      counts->name, s, queries, ms / q, applies);
}
#endif

/****************************************************************************
  Print debugging information about one city.
****************************************************************************/
void cm_print_city(const struct city *pcity)
{
  struct tile *pcenter = city_tile(pcity);

  freelog(LOG_TEST, "cm_print_city(city %d=\"%s\")",
          pcity->id,
          city_name(pcity));
  freelog(LOG_TEST,
	  "  size=%d, specialists=%s",
	  pcity->size, specialists_string(pcity->specialists));

  freelog(LOG_TEST, "  workers at:");
  city_tile_iterate_cxy(pcenter, ptile, x, y) {
    struct city *pwork = tile_worked(ptile);

    if (NULL != pwork && pwork == pcity) {
      freelog(LOG_TEST, "    {%2d,%2d} (%4d,%4d)", x, y, TILE_XY(ptile));
    }

    if (C_TILE_WORKER == pcity->city_map[x][y]) {
      freelog(LOG_TEST, "    {%2d,%2d}", x, y);
    }
  } city_tile_iterate_cxy_end;

  freelog(LOG_TEST, "  food    = %3d (%+3d)",
          pcity->prod[O_FOOD], pcity->surplus[O_FOOD]);
  freelog(LOG_TEST, "  shield  = %3d (%+3d)",
          pcity->prod[O_SHIELD], pcity->surplus[O_SHIELD]);
  freelog(LOG_TEST, "  trade   = %3d", pcity->surplus[O_TRADE]);

  freelog(LOG_TEST, "  gold    = %3d (%+3d)", pcity->prod[O_GOLD],
	  pcity->surplus[O_GOLD]);
  freelog(LOG_TEST, "  luxury  = %3d", pcity->prod[O_LUXURY]);
  freelog(LOG_TEST, "  science = %3d", pcity->prod[O_SCIENCE]);
}


/****************************************************************************
  Print debugging information about a full CM result.
****************************************************************************/
void cm_print_result(const struct cm_result *result)
{
  int y;
  int workers = cm_result_workers(result);

  freelog(LOG_TEST, "cm_print_result(result=%p)", (void *)result);
  freelog(LOG_TEST, "  found_a_valid=%d disorder=%d happy=%d",
      result->found_a_valid, result->disorder, result->happy);

  freelog(LOG_TEST, "  workers at:");
  city_map_iterate(x, y) {
    if (result->worker_positions_used[x][y]) {
      freelog(LOG_TEST, "    {%2d,%2d}", x, y);
    }
  } city_map_iterate_end;

  for (y = 0; y < CITY_MAP_SIZE; y++) {
    char line[CITY_MAP_SIZE + 1];
    int x;

    line[CITY_MAP_SIZE] = 0;

    for (x = 0; x < CITY_MAP_SIZE; x++) {
      if (!is_valid_city_coords(x, y)) {
        line[x] = '-';
      } else if (is_free_worked_cxy(x, y)) {
        line[x] = '+';
      } else if (result->worker_positions_used[x][y]) {
        line[x] = 'w';
      } else {
        line[x] = '.';
      }
    }
    freelog(LOG_TEST, "  %s", line);
  }
  freelog(LOG_TEST, "  (workers/specialists) %d/%s",
          workers,
          specialists_string(result->specialists));

  output_type_iterate(i) {
    freelog(LOG_TEST, "  %10s surplus=%d",
        get_output_name(i), result->surplus[i]);
  } output_type_iterate_end;
}
