/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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

#include "log.h"
#include "movement.h"
#include "player.h"
#include "unit.h"

#include "path_finding.h"
#include "pf_tools.h"

#include "maphand.h"

#include "ailog.h"
#include "aitools.h"

#include "aiexplorer.h"


/**************************************************************************
  Determine if a tile is likely to be water, given information that
  the player actually has. Return the % certainty that it's water
  (100 = certain, 50 = no idea, 0 = certainly not).
**************************************************************************/
static int likely_ocean(struct tile *ptile, struct player *pplayer)
{
  int ocean = 0;
  int land = 0;
  
  /* We do not check H_MAP here, it should be done by map_is_known() */
  if (map_is_known(ptile, pplayer)) {
    /* we've seen the tile already. */
    return (is_ocean_tile(ptile) ? 100 : 0);
  }

  /* The central tile is likely to be the same as the
   * nearby tiles. */
  adjc_dir_iterate(ptile, ptile1, dir) {
    if (map_is_known(ptile1, pplayer)) {
      if (is_ocean_tile(ptile1)) {
        ocean++;
      } else {
        land++;
      }
    }
  } adjc_dir_iterate_end;

  return 50 + (50 / map.num_valid_dirs * (ocean - land));
}

/**************************************************************************
  Returns TRUE if a unit owned by the given player can safely "explore" the
  given tile. This mainly takes care that military units do not try to
  move into another player's territory in violation of a treaty.
**************************************************************************/
static bool ai_may_explore(const struct tile *ptile,
                           const struct player *pplayer,
                           const bv_flags unit_flags)
{
  /* Don't allow military units to cross borders. */
  if (!BV_ISSET(unit_flags, F_CIVILIAN)
      && players_non_invade(tile_owner(ptile), pplayer)) {
    return FALSE;
  }

  /* Can't visit tiles with non-allied units. */
  if (is_non_allied_unit_tile(ptile, pplayer)) {
    return FALSE;
  }

  /* Non-allied cities are taboo even if no units are inside. */
  if (tile_city(ptile) && !pplayers_allied(tile_owner(ptile), pplayer)) {
    return FALSE;
  }

  return TRUE;
}

/***************************************************************************
  TB function used by ai_explorer_goto().
***************************************************************************/
static enum tile_behavior ai_explorer_tb(const struct tile *ptile,
                                         enum known_type k,
                                         const struct pf_parameter *param)
{
  if (!ai_may_explore(ptile, param->owner, param->unit_flags)) {
    return TB_IGNORE;
  }
  return TB_NORMAL;
}

/***************************************************************************
  Constrained goto using ai_may_explore().
***************************************************************************/
static bool ai_explorer_goto(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct ai_risk_cost risk_cost;

  ai_fill_unit_param(&parameter, &risk_cost, punit, ptile);
  parameter.get_TB = ai_explorer_tb;

  UNIT_LOG(LOG_DEBUG, punit, "ai_explorer_goto to %d,%d",
           ptile->x, ptile->y);
  return ai_unit_goto_constrained(punit, ptile, &parameter);
}

/**************************************************************************
Return a value indicating how desirable it is to explore the given tile.
In general, we want to discover unknown terrain of the opposite kind to
our natural terrain, i.e. pedestrians like ocean and boats like land.
Even if terrain is known, but of opposite kind, we still want it
-- so that we follow the shoreline.
We also would like discovering tiles which can be harvested by our cities -- 
because that improves citizen placement. We do not currently do this, see
comment below.
**************************************************************************/
#define SAME_TER_SCORE         21
#define DIFF_TER_SCORE         81
#define KNOWN_SAME_TER_SCORE   0
#define KNOWN_DIFF_TER_SCORE   51

/* The maximum number of tiles that the unit might uncover in a move. 
 * #define MAX_NEW_TILES          (1 + 4 * (unit_type(punit)->vision_range))
 * The previous line would be ideal, but we'd like these to be constants
 * for efficiency, so pretend vision_range == 1 */
#define MAX_NEW_TILES          5

/* The number of tiles that the unit can see. =(1 + 2r)^2
 * #define VISION_TILES           (1 + 2 * unit_type(punit)->vision_range)*\
 *                                (1 + 2 * unit_type(punit)->vision_range)
 * As above, set vision_range == 1 */
#define VISION_TILES           9

/* The desirability of the best tile possible without cities or huts. 
 * TER_SCORE is given per 1% of certainty about the terrain, so
 * muliply by 100 to compensate. */
#define BEST_NORMAL_TILE       \
  (100 * MAX_NEW_TILES * DIFF_TER_SCORE +\
   100 * (VISION_TILES - MAX_NEW_TILES) * KNOWN_DIFF_TER_SCORE)

/* We value exploring around our cities just slightly more than exploring
 * tiles fully surrounded by different terrain. */
#define OWN_CITY_SCORE         (BEST_NORMAL_TILE + 1)

/* And we value exploring huts even more than our own cities. */
#define HUT_SCORE              (OWN_CITY_SCORE + 1) 

#define BEST_POSSIBLE_SCORE    (HUT_SCORE + BEST_NORMAL_TILE)

static int explorer_desirable(struct tile *ptile, struct player *pplayer, 
                              struct unit *punit)
{
  int land_score, ocean_score, known_land_score, known_ocean_score;
  int radius_sq = unit_type(punit)->vision_radius_sq;
  int desirable = 0;
  int unknown = 0;

  /* First do some checks that would make a tile completely non-desirable.
   * If we're a barbarian and the tile has a hut, don't go there. */
  if (is_barbarian(pplayer) && tile_has_special(ptile, S_HUT)) {
    return 0;
  }

  /* Do no try to cross borders and break a treaty, etc. */
  if (!ai_may_explore(ptile, punit->owner, unit_type(punit)->flags)) {
    return 0;
  }

  /* What value we assign to the number of land and water tiles
   * depends on if we're a land or water unit. */
  if (is_ground_unit(punit)) {
    land_score = SAME_TER_SCORE;
    ocean_score = DIFF_TER_SCORE;
    known_land_score = KNOWN_SAME_TER_SCORE;
    known_ocean_score = KNOWN_DIFF_TER_SCORE;
  } else {
    land_score = DIFF_TER_SCORE;
    ocean_score = SAME_TER_SCORE;
    known_land_score = KNOWN_DIFF_TER_SCORE;
    known_ocean_score = KNOWN_SAME_TER_SCORE;
  }

  circle_iterate(ptile, radius_sq, ptile1) {
    int ocean = likely_ocean(ptile1, pplayer);

    if (!map_is_known(ptile1, pplayer)) {
      unknown++;

      /* FIXME: we should add OWN_CITY_SCORE to desirable if the tile 
       * can be harvested by a city of ours. Just calculating this each
       * time becomes rather expensive. Jason Short suggests:
       * It should be easy to generate this information once, for
       * the entire world.  It can be used by everyone and only 
       * sometimes needs to be recalculated (actually all changes 
       * only require local recalculation, but that could be unstable). */

      desirable += (ocean * ocean_score + (100 - ocean) * land_score);
    } else {
      if(is_tiles_adjacent(ptile, ptile1)) {
	/* we don't value staying offshore from land,
	 * only adjacent. Otherwise destroyers do the wrong thing. */
	desirable += (ocean * known_ocean_score 
                      + (100 - ocean) * known_land_score);
      }
    }
  } circle_iterate_end;

  if (unknown <= 0) {
    /* We make sure we'll uncover at least one unexplored tile. */
    desirable = 0;
  }

  if ((!pplayer->ai_data.control || !ai_handicap(pplayer, H_HUTS))
      && map_is_known(ptile, pplayer)
      && tile_has_special(ptile, S_HUT)) {
    /* we want to explore huts whenever we can,
     * even if doing so will not uncover any tiles. */
    desirable += HUT_SCORE;
  }

  return desirable;
}

/**************************************************************************
  Handle eXplore mode of a unit (explorers are always in eXplore mode 
  for AI) - explores unknown territory, finds huts.

  MR_OK: there is more territory to be explored.
  MR_DEATH: unit died.
  MR_PAUSE: unit cannot explore further now.
  Other results: unit cannot explore further.
**************************************************************************/
enum unit_move_result ai_manage_explorer(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  /* Loop prevention */
  int init_moves = punit->moves_left;

  /* The log of the want of the most desirable tile, 
   * given nearby water, cities, etc. */
  double log_most_desirable = -FC_INFINITY;

  /* The maximum distance we are willing to search. It decreases depending
   * on the want of already discovered tagets.  It is defined as the distance
   * at which a tile with BEST_POSSIBLE_SCORE would have to be found in
   * order to be better than the current most_desirable tile. */
  int max_dist = FC_INFINITY;

  /* Coordinates of most desirable tile. Initialized to make 
   * compiler happy. Also MC to the best tile. */
  struct tile *best_tile = NULL;
  int best_MC = FC_INFINITY;

  /* Path-finding stuff */
  struct pf_map *pfm;
  struct pf_parameter parameter;

#define DIST_FACTOR   0.6

  double logDF = log(DIST_FACTOR);
  double logBPS = log(BEST_POSSIBLE_SCORE);

  UNIT_LOG(LOG_DEBUG, punit, "auto-exploring.");

  if (pplayer->ai_data.control && unit_has_type_flag(punit, F_GAMELOSS)) {
    UNIT_LOG(LOG_DEBUG, punit, "exploration too dangerous!");
    return MR_BAD_ACTIVITY; /* too dangerous */
  }

  TIMING_LOG(AIT_EXPLORER, TIMER_START);

  pft_fill_unit_parameter(&parameter, punit);
  parameter.get_TB = no_fights_or_unknown;
  /* When exploring, even AI should pretend to not cheat. */
  parameter.omniscience = FALSE;

  pfm = pf_map_new(&parameter);
  pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
    int desirable;
    double log_desirable;

    /* Our callback should insure this. */
    assert(map_is_known(ptile, pplayer));

    desirable = explorer_desirable(ptile, pplayer, punit);

    if (desirable <= 0) { 
      /* Totally non-desirable tile. No need to continue. */
      continue;
    }

    /* take the natural log */
    log_desirable = log(desirable);

    /* Ok, the way we calculate goodness is taking the base tile 
     * desirability amortized by the time it takes to get there:
     *
     *     goodness = desirability * DIST_FACTOR^total_MC
     *
     * TODO: JDS notes that we should really make our exponential
     *       term dimensionless by dividing by move_rate.
     * 
     * We want to truncate our search, so we calculate a maximum distance
     * that we would move to find the tile with the most possible desirability
     * (BEST_POSSIBLE_SCORE) that gives us the same goodness as the current
     * tile position we're looking at. Therefore we have:
     *
     *   desirability * DIST_FACTOR^total_MC = 
     *               BEST_POSSIBLE_SCORE * DIST_FACTOR^(max distance)      (1)
     *
     * and then solve for max_dist. We only want to change max_dist when
     * we find a tile that has better goodness than we've found so far; hence
     * the conditional below. It looks cryptic, but all it is is testing which
     * of two goodnesses is bigger after taking the natural log of both sides.
     */
    if (log_desirable + move_cost * logDF 
	> log_most_desirable + best_MC * logDF) {

      log_most_desirable = log_desirable;
      best_tile = ptile;
      best_MC = move_cost;

      /* take the natural log and solve equation (1) above.  We round
       * max_dist down (is this correct?). */
      max_dist = best_MC + (log_most_desirable - logBPS)/logDF;
    }

    /* let's not go further than this */
    if (move_cost > max_dist) {
      break;
    }
  } pf_map_iterate_move_costs_end;
  pf_map_destroy(pfm);

  TIMING_LOG(AIT_EXPLORER, TIMER_STOP);

  /* Go to the best tile found. */
  if (best_tile != NULL) {
    /* TODO: read the path off the map we made.  Then we can make a path 
     * which goes beside the unknown, with a good EC callback... */
    if (!ai_explorer_goto(punit, best_tile)) {
      /* Died?  Strange... */
      return MR_DEATH;
    }
    UNIT_LOG(LOG_DEBUG, punit, "exploration GOTO succeeded");
    if (punit->moves_left > 0) {
      /* We can still move on... */
      if (punit->moves_left < init_moves) {
	/* At least we moved (and maybe even got to where we wanted).  
         * Let's do more exploring. 
         * (Checking only whether our position changed is unsafe: can allow
         * yoyoing on a RR) */
	UNIT_LOG(LOG_DEBUG, punit, "recursively exploring...");
	return ai_manage_explorer(punit);          
      } else {
	UNIT_LOG(LOG_DEBUG, punit, "done exploring (all finished)...");
	return MR_PAUSE;
      }
    }
    UNIT_LOG(LOG_DEBUG, punit, "done exploring (but more go go)...");
    return MR_OK;
  } else {
    /* Didn't find anything. */
    UNIT_LOG(LOG_DEBUG, punit, "failed to explore more");
    return MR_BAD_MAP_POSITION;
  }
#undef DIST_FACTOR
}

#undef SAME_TER_SCORE
#undef DIFF_TER_SCORE
#undef KNOWN_SAME_TER_SCORE
#undef KNOWN_DIFF_TER_SCORE
#undef OWN_CITY_SCORE
#undef HUT_SCORE
