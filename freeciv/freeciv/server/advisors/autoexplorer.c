/***********************************************************************
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
#include <fc_config.h>
#endif

#include <math.h> /* log */

/* utility */
#include "bitvector.h"
#include "log.h"

/* common */
#include "ai.h"
#include "map.h"
#include "movement.h"
#include "player.h"
#include "unit.h"

/* common/aicore */
#include "path_finding.h"
#include "pf_tools.h"

/* server */
#include "maphand.h"
#include "srv_log.h"

/* server/advisors */
#include "advgoto.h"

/* ai */
#include "handicaps.h"

#include "autoexplorer.h"


/**********************************************************************//**
  Determine if a tile is likely to be native, given information that
  the player actually has. Return the % certainty that it's native
  (100 = certain, 50 = no idea, 0 = certainly not).
**************************************************************************/
static int likely_native(struct tile *ptile,
                         struct player *pplayer,
                         struct unit_class *pclass)
{
  int native = 0;
  int foreign = 0;
  
  /* We do not check H_MAP here, it should be done by map_is_known() */
  if (map_is_known(ptile, pplayer)) {
    /* we've seen the tile already. */
    return (is_native_tile_to_class(pclass, ptile) ? 100 : 0);
  }

  /* The central tile is likely to be the same as the
   * nearby tiles. */
  adjc_dir_iterate(&(wld.map), ptile, ptile1, dir) {
    if (map_is_known(ptile1, pplayer)) {
      if (is_native_tile_to_class(pclass, ptile)) {
        native++;
      } else {
        foreign++;
      }
    }
  } adjc_dir_iterate_end;

  return 50 + (50 / wld.map.num_valid_dirs * (native - foreign));
}

/**********************************************************************//**
  Returns TRUE if a unit owned by the given player can safely "explore" the
  given tile. This mainly takes care that military units do not try to
  move into another player's territory in violation of a treaty.
**************************************************************************/
static bool player_may_explore(const struct tile *ptile,
                               const struct player *pplayer,
                               const struct unit_type *punittype)
{
  /* Don't allow military units to cross borders. */
  if (!utype_has_flag(punittype, UTYF_CIVILIAN)
      && !player_can_invade_tile(pplayer, ptile)) {
    return FALSE;
  }

  /* Can't visit tiles with non-allied units. */
  if (is_non_allied_unit_tile(ptile, pplayer)) {
    return FALSE;
  }

  /* Non-allied cities are taboo even if no units are inside. */
  if (tile_city(ptile)
      && !pplayers_allied(city_owner(tile_city(ptile)), pplayer)) {
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  TB function used by explorer_goto().
**************************************************************************/
static enum tile_behavior explorer_tb(const struct tile *ptile,
                                      enum known_type k,
                                      const struct pf_parameter *param)
{
  if (!player_may_explore(ptile, param->owner, param->utype)) {
    return TB_IGNORE;
  }
  return TB_NORMAL;
}

/**********************************************************************//**
  Constrained goto using player_may_explore().
**************************************************************************/
static bool explorer_goto(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct adv_risk_cost risk_cost;
  bool alive = TRUE;
  struct pf_map *pfm;
  struct pf_path *path;
  struct player *pplayer = unit_owner(punit);

  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  parameter.get_TB = explorer_tb;
  adv_avoid_risks(&parameter, &risk_cost, punit, NORMAL_STACKING_FEARFULNESS);

  /* Show the destination in the client */
  punit->goto_tile = ptile;

  UNIT_LOG(LOG_DEBUG, punit, "explorer_goto to %d,%d", TILE_XY(ptile));

  pfm = pf_map_new(&parameter);
  path = pf_map_path(pfm, ptile);

  if (path != NULL) {
    alive = adv_follow_path(punit, path, ptile);
    pf_path_destroy(path);
  }

  pf_map_destroy(pfm);

  return alive;
}

/**********************************************************************//**
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
 * #define MAX_NEW_TILES          (1 + 4 * (unit_type_get(punit)->vision_range))
 * The previous line would be ideal, but we'd like these to be constants
 * for efficiency, so pretend vision_range == 1 */
#define MAX_NEW_TILES          5

/* The number of tiles that the unit can see. =(1 + 2r)^2
 * #define VISION_TILES           (1 + 2 * unit_type_get(punit)->vision_range)*\
 *                                (1 + 2 * unit_type_get(punit)->vision_range)
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
  int radius_sq = unit_type_get(punit)->vision_radius_sq;
  int desirable = 0;
  int unknown = 0;

  /* First do some checks that would make a tile completely non-desirable.
   * If we're a barbarian and the tile has a hut, don't go there. */
  if (is_barbarian(pplayer) && tile_has_cause_extra(ptile, EC_HUT)) {
    return 0;
  }

  /* Do no try to cross borders and break a treaty, etc. */
  if (!player_may_explore(ptile, punit->owner, unit_type_get(punit))) {
    return 0;
  }

  circle_iterate(&(wld.map), ptile, radius_sq, ptile1) {
    int native = likely_native(ptile1, pplayer, unit_class_get(punit));

    if (!map_is_known(ptile1, pplayer)) {
      unknown++;

      /* FIXME: we should add OWN_CITY_SCORE to desirable if the tile 
       * can be harvested by a city of ours. Just calculating this each
       * time becomes rather expensive. Jason Short suggests:
       * It should be easy to generate this information once, for
       * the entire world.  It can be used by everyone and only 
       * sometimes needs to be recalculated (actually all changes 
       * only require local recalculation, but that could be unstable). */

      desirable += (native * SAME_TER_SCORE + (100 - native) * DIFF_TER_SCORE);
    } else {
      if (is_tiles_adjacent(ptile, ptile1)) {
	/* we don't value staying offshore from land,
	 * only adjacent. Otherwise destroyers do the wrong thing. */
	desirable += (native * KNOWN_SAME_TER_SCORE
                      + (100 - native) * KNOWN_DIFF_TER_SCORE);
      }
    }
  } circle_iterate_end;

  if (unknown <= 0) {
    /* We make sure we'll uncover at least one unexplored tile. */
    desirable = 0;
  }

  if ((!is_ai(pplayer) || !has_handicap(pplayer, H_HUTS))
      && map_is_known(ptile, pplayer)
      && tile_has_cause_extra(ptile, EC_HUT)) {
    /* we want to explore huts whenever we can,
     * even if doing so will not uncover any tiles. */
    desirable += HUT_SCORE;
  }

  return desirable;
}

/**********************************************************************//**
  Handle eXplore mode of a unit (explorers are always in eXplore mode 
  for AI) - explores unknown territory, finds huts.

  MR_OK: there is more territory to be explored.
  MR_DEATH: unit died.
  MR_PAUSE: unit cannot explore further now.
  Other results: unit cannot explore further.
**************************************************************************/
enum unit_move_result manage_auto_explorer(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  /* Loop prevention */
  const struct tile *init_tile = unit_tile(punit);

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

  if (!is_human(pplayer) && unit_has_type_flag(punit, UTYF_GAMELOSS)) {
    UNIT_LOG(LOG_DEBUG, punit, "exploration too dangerous!");
    return MR_BAD_ACTIVITY; /* too dangerous */
  }

  TIMING_LOG(AIT_EXPLORER, TIMER_START);

  pft_fill_unit_parameter(&parameter, punit);
  parameter.get_TB = no_fights_or_unknown;
  /* When exploring, even AI should pretend to not cheat. */
  parameter.omniscience = FALSE;

  pfm = pf_map_new(&parameter);
  pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
    int desirable;
    double log_desirable;

    /* Our callback should insure this. */
    fc_assert_action(map_is_known(ptile, pplayer), continue);

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
  } pf_map_move_costs_iterate_end;
  pf_map_destroy(pfm);

  TIMING_LOG(AIT_EXPLORER, TIMER_STOP);

  /* Go to the best tile found. */
  if (best_tile != NULL) {
    /* TODO: read the path off the map we made.  Then we can make a path 
     * which goes beside the unknown, with a good EC callback... */
    enum override_bool allow = NO_OVERRIDE;

    if (is_ai(pplayer)) {
      CALL_PLR_AI_FUNC(want_to_explore, pplayer, punit, best_tile, &allow);
    }
    if (allow == OVERRIDE_FALSE) {
      UNIT_LOG(LOG_DEBUG, punit, "not allowed to explore");
      return MR_NOT_ALLOWED;
    }
    if (!explorer_goto(punit, best_tile)) {
      /* Died?  Strange... */
      return MR_DEATH;
    }
    UNIT_LOG(LOG_DEBUG, punit, "exploration GOTO succeeded");
    if (punit->moves_left > 0) {
      /* We can still move on... */
      if (!same_pos(init_tile, unit_tile(punit))) {
        /* At least we moved (and maybe even got to where we wanted).  
         * Let's do more exploring. 
         * (Checking only whether our position changed is unsafe: can allow
         * yoyoing on a RR) */
	UNIT_LOG(LOG_DEBUG, punit, "recursively exploring...");
	return manage_auto_explorer(punit);          
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
