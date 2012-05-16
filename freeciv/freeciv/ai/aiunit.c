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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "city.h"
#include "combat.h"
#include "game.h"
#include "government.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "movement.h"
#include "packets.h"
#include "pf_tools.h"
#include "player.h"
#include "rand.h"
#include "shared.h"
#include "timing.h"
#include "unit.h"
#include "unitlist.h"

#include "barbarian.h"
#include "caravan.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplomats.h"
#include "gotohand.h"
#include "maphand.h"
#include "settlers.h"
#include "unithand.h"
#include "unittools.h"

#include "advmilitary.h"
#include "aiair.h"
#include "aicity.h"
#include "aidata.h"
#include "aidiplomat.h"
#include "aiexplorer.h"
#include "aiferry.h"
#include "aiguard.h"
#include "aihand.h"
#include "aihunt.h"
#include "ailog.h"
#include "aiparatrooper.h"
#include "aitools.h"

#include "aiunit.h"

#define LOGLEVEL_RECOVERY LOG_DEBUG
#define LOG_CARAVAN       LOG_DEBUG
#define LOG_CARAVAN2      LOG_DEBUG

static void ai_manage_caravan(struct player *pplayer, struct unit *punit);
static void ai_manage_barbarian_leader(struct player *pplayer,
				       struct unit *leader);

static void ai_military_findjob(struct player *pplayer,struct unit *punit);
static void ai_military_defend(struct player *pplayer,struct unit *punit);
static void ai_military_attack(struct player *pplayer,struct unit *punit);

static int unit_move_turns(struct unit *punit, struct tile *ptile);
static bool unit_role_defender(const struct unit_type *punittype);
static int unit_def_rating_sq(struct unit *punit, struct unit *pdef);

/*
 * Cached values. Updated by update_simple_ai_types.
 *
 * This a hack to enable emulation of old loops previously hardwired
 * as 
 *    for (i = U_WARRIORS; i <= U_BATTLESHIP; i++)
 *
 * (Could probably just adjust the loops themselves fairly simply,
 * but this is safer for regression testing.)
 *
 * Not dealing with planes yet.
 *
 * Terminated by NULL.
 */
struct unit_type *simple_ai_types[U_LAST];

/**************************************************************************
  Move defenders around with airports. Since this expends all our 
  movement, a valid question is - why don't we do this on turn end?
  That's because we want to avoid emergency actions to protect the city
  during the turn if that isn't necessary.
**************************************************************************/
static void ai_airlift(struct player *pplayer)
{
  struct city *most_needed;
  int comparison;
  struct unit *transported;

  do {
    most_needed = NULL;
    comparison = 0;
    transported = NULL;

    city_list_iterate(pplayer->cities, pcity) {
      if (pcity->ai->urgency > comparison && pcity->airlift) {
        comparison = pcity->ai->urgency;
        most_needed = pcity;
      }
    } city_list_iterate_end;
    if (!most_needed) {
      comparison = 0;
      city_list_iterate(pplayer->cities, pcity) {
        if (pcity->ai->danger > comparison && pcity->airlift) {
          comparison = pcity->ai->danger;
          most_needed = pcity;
        }
      } city_list_iterate_end;
    }
    if (!most_needed) {
      return;
    }
    comparison = 0;
    unit_list_iterate(pplayer->units, punit) {
      struct tile *ptile = (punit->tile);
      struct city *pcity = tile_city(ptile);

      if (pcity
          && pcity->ai->urgency == 0
          && pcity->ai->danger - DEFENCE_POWER(punit) < comparison
          && unit_can_airlift_to(punit, most_needed)
          && DEFENCE_POWER(punit) > 2
          && (punit->ai.ai_role == AIUNIT_NONE
              || punit->ai.ai_role == AIUNIT_DEFEND_HOME)
          && IS_ATTACKER(punit)) {
        comparison = pcity->ai->danger;
        transported = punit;
      }
    } unit_list_iterate_end;
    if (!transported) {
      return;
    }
    UNIT_LOG(LOG_DEBUG, transported, "airlifted to defend %s",
             city_name(most_needed));
    do_airline(transported, most_needed);
  } while (TRUE);
}

/**************************************************************************
  Similar to is_my_zoc(), but with some changes:
  - destination (x0,y0) need not be adjacent?
  - don't care about some directions?
  
  Note this function only makes sense for ground units.

  Fix to bizarre did-not-find bug.  Thanks, Katvrr -- Syela
**************************************************************************/
static bool could_be_my_zoc(struct unit *myunit, struct tile *ptile)
{
  assert(is_ground_unit(myunit));
  
  if (same_pos(ptile, myunit->tile))
    return FALSE; /* can't be my zoc */
  if (is_tiles_adjacent(ptile, myunit->tile)
      && !is_non_allied_unit_tile(ptile, unit_owner(myunit)))
    return FALSE;

  adjc_iterate(ptile, atile) {
    if (!is_ocean_tile(atile)
	&& is_non_allied_unit_tile(atile, unit_owner(myunit))) {
      return FALSE;
    }
  } adjc_iterate_end;
  
  return TRUE;
}

/**************************************************************************
  returns:
    0 if can't move
    1 if zoc_ok
   -1 if zoc could be ok?

  see also unithand can_unit_move_to_tile_with_notify()
**************************************************************************/
int could_unit_move_to_tile(struct unit *punit, struct tile *dest_tile)
{
  enum unit_move_result reason =
      test_unit_move_to_tile(unit_type(punit), unit_owner(punit),
                             ACTIVITY_IDLE, punit->tile, 
                             dest_tile, unit_has_type_flag(punit, F_IGZOC));
  switch (reason) {
  case MR_OK:
    return 1;

  case MR_ZOC:
    if (could_be_my_zoc(punit, punit->tile)) {
      return -1;
    }
    break;

  default:
    break;
  };
  return 0;
}

/********************************************************************** 
  This is a much simplified form of assess_defense (see advmilitary.c),
  but which doesn't use pcity->ai.wallvalue and just returns a boolean
  value.  This is for use with "foreign" cities, especially non-ai
  cities, where ai.wallvalue may be out of date or uninitialized --dwp
***********************************************************************/
static bool has_defense(struct city *pcity)
{
  unit_list_iterate((pcity->tile)->units, punit) {
    if (is_military_unit(punit) && get_defense_power(punit) != 0 && punit->hp != 0) {
      return TRUE;
    }
  }
  unit_list_iterate_end;
  return FALSE;
}

/**********************************************************************
 Precondition: A warmap must already be generated for the punit.

 Returns the minimal amount of turns required to reach the given
 destination position. The actual turn at which the unit will
 reach the given point depends on the movement points it has remaining.

 For example: Unit has a move rate of 3, path cost is 5, unit has 2
 move points left.

 path1 costs: first tile = 3, second tile = 2

 turn 0: points = 2, unit has to wait
 turn 1: points = 3, unit can move, points = 0, has to wait
 turn 2: points = 3, unit can move, points = 1

 path2 costs: first tile = 2, second tile = 3

 turn 0: points = 2, unit can move, points = 0, has to wait
 turn 1: points = 3, unit can move, points = 0

 In spite of the path costs being the same, units that take a path of the
 same length will arrive at different times. This function also does not 
 take into account ZOC or lack of hp affecting movement.
 
 Note: even if a unit has only fractional move points left, there is
 still a possibility it could cross the tile.
**************************************************************************/
static int unit_move_turns(struct unit *punit, struct tile *ptile)
{
  int move_time;
  int move_rate = unit_move_rate(punit);
  struct unit_class *pclass = unit_class(punit);

  if (!uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    /* Unit does not care about terrain */
    move_time = real_map_distance(punit->tile, ptile) * SINGLE_MOVE / move_rate;
  } else {
    if (unit_has_type_flag(punit, F_IGTER)) {
      /* FIXME: IGTER units should have their move rates multiplied by 
       * igter_speedup. Note: actually, igter units should never have their 
       * move rates multiplied. The correct behaviour is to have every tile 
       * they cross cost 1/3 of a movement point. ---RK */
      move_rate *= 3;
    }

    if (uclass_move_type(pclass) == SEA_MOVING) {
      move_time = WARMAP_SEACOST(ptile) / move_rate;
    } else {
      move_time = WARMAP_COST(ptile) / move_rate;
    }
  }
  return move_time;
}
 
/*********************************************************************
  In the words of Syela: "Using funky fprime variable instead of f in
  the denom, so that def=1 units are penalized correctly."

  Translation (GB): build_cost_balanced is used in the denominator of
  the want equation (see, e.g.  find_something_to_kill) instead of
  just build_cost to make AI build more balanced units (with def > 1).
*********************************************************************/
int build_cost_balanced(const struct unit_type *punittype)
{
  return 2 * utype_build_shield_cost(punittype) * punittype->attack_strength /
      (punittype->attack_strength + punittype->defense_strength);
}

/**************************************************************************
  Attack rating of this kind of unit.
**************************************************************************/
int unittype_att_rating(const struct unit_type *punittype, int veteran,
                        int moves_left, int hp)
{
  return base_get_attack_power(punittype, veteran, moves_left) * hp
         * punittype->firepower / POWER_DIVIDER;
}

/********************************************************************** 
  Attack rating of this particular unit right now.
***********************************************************************/
static int unit_att_rating_now(struct unit *punit)
{
  return unittype_att_rating(unit_type(punit), punit->veteran,
                             punit->moves_left, punit->hp);
}

/********************************************************************** 
  Attack rating of this particular unit assuming that it has a
  complete move left.
***********************************************************************/
int unit_att_rating(struct unit *punit)
{
  return unittype_att_rating(unit_type(punit), punit->veteran,
                             SINGLE_MOVE, punit->hp);
}

/********************************************************************** 
  Square of the previous function - used in actual computations.
***********************************************************************/
static int unit_att_rating_sq(struct unit *punit)
{
  int v = unit_att_rating(punit);
  return v * v;
}

/********************************************************************** 
  Defence rating of this particular unit against this attacker.
***********************************************************************/
static int unit_def_rating(struct unit *attacker, struct unit *defender)
{
  return get_total_defense_power(attacker, defender) *
         (attacker->id != 0 ? defender->hp : unit_type(defender)->hp) *
         unit_type(defender)->firepower / POWER_DIVIDER;
}

/********************************************************************** 
  Square of the previous function - used in actual computations.
***********************************************************************/
static int unit_def_rating_sq(struct unit *attacker,
                              struct unit *defender)
{
  int v = unit_def_rating(attacker, defender);
  return v * v;
}

/********************************************************************** 
  Basic (i.e. not taking attacker specific corections into account) 
  defence rating of this particular unit.
***********************************************************************/
int unit_def_rating_basic(struct unit *punit)
{
  return base_get_defense_power(punit) * punit->hp *
    unit_type(punit)->firepower / POWER_DIVIDER;
}

/********************************************************************** 
  Square of the previous function - used in actual computations.
***********************************************************************/
int unit_def_rating_basic_sq(struct unit *punit)
{
  int v = unit_def_rating_basic(punit);
  return v * v;
}

/**************************************************************************
  Defence rating of def_type unit against att_type unit, squared.
  See get_virtual_defense_power for the arguments att_type, def_type,
  x, y, fortified and veteran.
**************************************************************************/
int unittype_def_rating_sq(const struct unit_type *att_type,
			   const struct unit_type *def_type,
			   const struct player *def_player,
                           struct tile *ptile, bool fortified, int veteran)
{
  int v = get_virtual_defense_power(att_type, def_type, def_player, ptile,
                                    fortified, veteran)
    * def_type->hp * def_type->firepower / POWER_DIVIDER;

  return v * v;
}

/**************************************************************************
Compute how much we want to kill certain victim we've chosen, counted in
SHIELDs.

FIXME?: The equation is not accurate as the other values can vary for other
victims on same tile (we take values from best defender) - however I believe
it's accurate just enough now and lost speed isn't worth that. --pasky

Benefit is something like 'attractiveness' of the victim, how nice it would be
to destroy it. Larger value, worse loss for enemy.

Attack is the total possible attack power we can throw on the victim. Note that
it usually comes squared.

Loss is the possible loss when we would lose the unit we are attacking with (in
SHIELDs).

Vuln is vulnerability of our unit when attacking the enemy. Note that it
usually comes squared as well.

Victim count is number of victims stacked in the target tile. Note that we
shouldn't treat cities as a stack (despite the code using this function) - the
scaling is probably different. (extremely dodgy usage of it -- GB)
**************************************************************************/
int kill_desire(int benefit, int attack, int loss, int vuln, int victim_count)
{
  int desire;

  /*         attractiveness     danger */ 
  desire = ((benefit * attack - loss * vuln) * victim_count * SHIELD_WEIGHTING
            / (attack + vuln * victim_count));

  return desire;
}

/**************************************************************************
  Compute how much we want to kill certain victim we've chosen, counted in
  SHIELDs.  See comment to kill_desire.

  chance -- the probability the action will succeed, 
  benefit -- the benefit (in shields) that we are getting in the case of 
             success
  loss -- the loss (in shields) that we suffer in the case of failure

  Essentially returns the probabilistic average win amount:
      benefit * chance - loss * (1 - chance)
**************************************************************************/
static int avg_benefit(int benefit, int loss, double chance)
{
  return (int)(((benefit + loss) * chance - loss) * SHIELD_WEIGHTING);
}

/**************************************************************************
  Calculates the value and cost of nearby allied units to see if we can
  expect any help in our attack. Base function.
**************************************************************************/
static void reinforcements_cost_and_value(struct unit *punit,
					  struct tile *ptile0,
                                          int *value, int *cost)
{
  *cost = 0;
  *value = 0;
  square_iterate(ptile0, 1, ptile) {
    unit_list_iterate(ptile->units, aunit) {
      if (aunit != punit
	  && pplayers_allied(unit_owner(punit), unit_owner(aunit))) {
        int val = unit_att_rating(aunit);

        if (val != 0) {
          *value += val;
          *cost += unit_build_shield_cost(aunit);
        }
      }
    } unit_list_iterate_end;
  } square_iterate_end;
}

/*************************************************************************
  Is there another unit which really should be doing this attack? Checks
  all adjacent tiles and the tile we stand on for such units.
**************************************************************************/
static bool is_my_turn(struct unit *punit, struct unit *pdef)
{
  int val = unit_att_rating_now(punit), cur, d;

  CHECK_UNIT(punit);

  square_iterate(pdef->tile, 1, ptile) {
    unit_list_iterate(ptile->units, aunit) {
      if (aunit == punit || unit_owner(aunit) != unit_owner(punit))
	continue;
      if (!can_unit_attack_all_at_tile(aunit, pdef->tile))
	continue;
      d = get_virtual_defense_power(unit_type(aunit), unit_type(pdef),
				    unit_owner(pdef), pdef->tile,
				    FALSE, 0);
      if (d == 0)
	return TRUE;		/* Thanks, Markus -- Syela */
      cur = unit_att_rating_now(aunit) *
	  get_virtual_defense_power(unit_type(punit), unit_type(pdef),
				    unit_owner(pdef), pdef->tile,
				    FALSE, 0) / d;
      if (cur > val && ai_fuzzy(unit_owner(punit), TRUE))
	return FALSE;
    }
    unit_list_iterate_end;
  }
  square_iterate_end;
  return TRUE;
}

/*************************************************************************
  This function appraises the location (x, y) for a quick hit-n-run 
  operation.  We do not take into account reinforcements: rampage is for
  loners.

  Returns value as follows:
    -RAMPAGE_FREE_CITY_OR_BETTER    
             means undefended enemy city
    -RAMPAGE_HUT_OR_BETTER    
             means hut
    RAMPAGE_ANYTHING ... RAMPAGE_HUT_OR_BETTER - 1  
             is value of enemy unit weaker than our unit
    0        means nothing found or error
  Here the minus indicates that you need to enter the target tile (as 
  opposed to attacking it, which leaves you where you are).
**************************************************************************/
static int ai_rampage_want(struct unit *punit, struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);
  struct unit *pdef = get_defender(punit, ptile);

  CHECK_UNIT(punit);
  
  if (pdef) {
    
    if (!can_unit_attack_tile(punit, ptile)) {
      return 0;
    }
    
    {
      /* See description of kill_desire() about these variables. */
      int attack = unit_att_rating_now(punit);
      int benefit = stack_cost(pdef);
      int loss = unit_build_shield_cost(punit);

      attack *= attack;
      
      /* If the victim is in the city/fortress, we correct the benefit
       * with our health because there could be reprisal attacks.  We
       * shouldn't send already injured units to useless suicide.
       * Note that we do not specially encourage attacks against
       * cities: rampage is a hit-n-run operation. */
      if (!is_stack_vulnerable(ptile) 
          && unit_list_size(ptile->units) > 1) {
        benefit = (benefit * punit->hp) / unit_type(punit)->hp;
      }
      
      /* If we have non-zero attack rating... */
      if (attack > 0 && is_my_turn(punit, pdef)) {
	double chance = unit_win_chance(punit, pdef);
	int desire = avg_benefit(benefit, loss, chance);

        /* No need to amortize, our operation takes one turn. */
	UNIT_LOG(LOG_DEBUG, punit, "Rampage: Desire %d to kill %s(%d,%d)",
		 desire,
		 unit_rule_name(pdef),
		 TILE_XY(pdef->tile));

        return MAX(0, desire);
      }
    }
    
  } else {
    struct city *pcity = tile_city(ptile);
    
    /* No defender... */
    
    /* ...and free foreign city waiting for us. Who would resist! */
    if (pcity && pplayers_at_war(pplayer, city_owner(pcity))
        && COULD_OCCUPY(punit)) {
      return -RAMPAGE_FREE_CITY_OR_BETTER;
    }
    
    /* ...or tiny pleasant hut here! */
    if (tile_has_special(ptile, S_HUT) && !is_barbarian(pplayer)
        && is_ground_unit(punit)) {
      return -RAMPAGE_HUT_OR_BETTER;
    }
  }
  
  return 0;
}

/*************************************************************************
  Look for worthy targets within a one-turn horizon.
*************************************************************************/
static struct pf_path *find_rampage_target(struct unit *punit,
                                           int thresh_adj, int thresh_move)
{
  struct pf_map *tgt_map;
  struct pf_path *path = NULL;
  struct pf_parameter parameter;
  /* Coordinates of the best target (initialize to silence compiler) */
  struct tile *ptile = punit->tile;
  /* Want of the best target */
  int max_want = 0;
  struct player *pplayer = unit_owner(punit);
 
  pft_fill_unit_attack_param(&parameter, punit);
  /* When trying to find rampage targets we ignore risks such as
   * enemy units because we are looking for trouble!
   * Hence no call ai_avoid_risks()
   */

  tgt_map = pf_map_new(&parameter);
  pf_map_iterate_move_costs(tgt_map, iter_tile, move_cost, FALSE) {
    int want;
    bool move_needed;
    int thresh;
 
    if (move_cost > punit->moves_left) {
      /* This is too far */
      break;
    }

    if (ai_handicap(pplayer, H_TARGETS) 
        && !map_is_known_and_seen(iter_tile, pplayer, V_MAIN)) {
      /* The target is under fog of war */
      continue;
    }
    
    want = ai_rampage_want(punit, iter_tile);

    /* Negative want means move needed even though the tiles are adjacent */
    move_needed = (!is_tiles_adjacent(punit->tile, iter_tile)
                   || want < 0);
    /* Select the relevant threshold */
    thresh = (move_needed ? thresh_move : thresh_adj);
    want = (want < 0 ? -want : want);

    if (want > max_want && want > thresh) {
      /* The new want exceeds both the previous maximum 
       * and the relevant threshold, so it's worth recording */
      max_want = want;
      ptile = iter_tile;
    }
  } pf_map_iterate_move_costs_end;

  if (max_want > 0) {
    /* We found something */
    path = pf_map_get_path(tgt_map, ptile);
    assert(path != NULL);
  }

  pf_map_destroy(tgt_map);
  
  return path;
}

/*************************************************************************
  Find and kill anything reachable within this turn and worth more than
  the relevant of the given thresholds until we have run out of juicy 
  targets or movement.  The first threshold is for attacking which will 
  leave us where we stand (attacking adjacent units), the second is for 
  attacking distant (but within reach) targets.

  For example, if unit is a bodyguard on duty, it should call
    ai_military_rampage(punit, 100, RAMPAGE_FREE_CITY_OR_BETTER)
  meaning "we will move _only_ to pick up a free city but we are happy to
  attack adjacent squares as long as they are worthy of it".

  Returns TRUE if survived the rampage session.
**************************************************************************/
bool ai_military_rampage(struct unit *punit, int thresh_adj, 
			 int thresh_move)
{
  int count = punit->moves_left + 1; /* break any infinite loops */
  struct pf_path *path = NULL;

  TIMING_LOG(AIT_RAMPAGE, TIMER_START);  
  CHECK_UNIT(punit);

  assert(thresh_adj <= thresh_move);
  /* This teaches the AI about the dangers inherent in occupychance. */
  thresh_adj += ((thresh_move - thresh_adj) * game.info.occupychance / 100);

  while (count-- > 0 && punit->moves_left > 0
         && (path = find_rampage_target(punit, thresh_adj, thresh_move))) {
    if (!ai_unit_execute_path(punit, path)) {
      /* Died */
      count = -1;
    }
    pf_path_destroy(path);
    path = NULL;
  }

  assert(!path);

  TIMING_LOG(AIT_RAMPAGE, TIMER_STOP);
  return (count >= 0);
}

/*************************************************************************
  If we are not covering our charge's ass, go do it now. Also check if we
  can kick some ass, which is always nice.
**************************************************************************/
static void ai_military_bodyguard(struct player *pplayer, struct unit *punit)
{
  struct unit *aunit = aiguard_charge_unit(punit);
  struct city *acity = aiguard_charge_city(punit);
  struct tile *ptile;

  CHECK_UNIT(punit);
  CHECK_GUARD(punit);

  if (aunit && unit_owner(aunit) == unit_owner(punit)) {
    /* protect a unit */
    /* FIXME: different behaviour for sailing units is silly;
     * should choose behaviour based on relative positions and
     * movement rates */
    if (is_sailing_unit(aunit)) {
      ptile = aunit->goto_tile;
    } else {
      ptile = aunit->tile;
    }
  } else if (acity && city_owner(acity) == unit_owner(punit)) {
    /* protect a city */
    ptile = acity->tile;
  } else {
    /* should be impossible */
    BODYGUARD_LOG(LOG_DEBUG, punit, "we lost our charge");
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    return;
  }

  if (same_pos(punit->tile, ptile)) {
    BODYGUARD_LOG(LOG_DEBUG, punit, "at RV");
  } else {
    if (goto_is_sane(punit, ptile, TRUE)) {
      BODYGUARD_LOG(LOG_DEBUG, punit, "meeting charge");
      if (!ai_gothere(pplayer, punit, ptile)) {
        /* We died */
        return;
      }
    } else {
      BODYGUARD_LOG(LOG_DEBUG, punit, "can not meet charge");
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    }
  }
  /* We might have stopped because of an enemy nearby.
   * Perhaps we can kill it.*/
  if (ai_military_rampage(punit, BODYGUARD_RAMPAGE_THRESHOLD,
                          RAMPAGE_FREE_CITY_OR_BETTER)
      && same_pos(punit->tile, ptile)) {
    punit->ai.done = TRUE; /* Stay with charge */
  }
}

/*************************************************************************
  Tries to find a land tile adjacent to water and to our target 
  (dest_tile).  Prefers tiles which are more defensible and/or
  where we will have moves left.
  FIXME: It checks if the ocean tile is in our Zone of Control?!
**************************************************************************/
static bool find_beachhead(struct unit *punit, struct tile *dest_tile,
			   struct tile **beachhead_tile)
{
  int ok, best = 0;

  CHECK_UNIT(punit);

  adjc_iterate(dest_tile, tile1) {
    struct terrain *pterrain = tile_terrain(tile1);

    ok = 0;
    if (WARMAP_SEACOST(tile1) <= 6 * THRESHOLD && !is_ocean(pterrain)) {
      /* accessible beachhead */
      adjc_iterate(tile1, tile2) {
	if (is_ocean_tile(tile2)
	    && is_my_zoc(unit_owner(punit), tile2)) {
	  ok++;
	  goto OK;

	  /* FIXME: The above used to be "break; out of adjc_iterate",
	  but this is incorrect since the current adjc_iterate() is
	  implemented as two nested loops.  */
	}
      } adjc_iterate_end;
    OK:

      if (ok > 0) {
	/* accessible beachhead with zoc-ok water tile nearby */
        ok = 10 + pterrain->defense_bonus / 10;
	if (tile_has_special(tile1, S_RIVER))
	  ok += (ok * terrain_control.river_defense_bonus) / 100;
        if (pterrain->movement_cost * SINGLE_MOVE <
            unit_move_rate(punit))
	  ok *= 8;
        ok += (6 * THRESHOLD - WARMAP_SEACOST(tile1));
        if (ok > best) {
	  best = ok;
	  *beachhead_tile = tile1;
	}
      }
    }
  } adjc_iterate_end;

  return (best > 0);
}

/*************************************************************************
  Does the unit with the id given have the flag L_DEFEND_GOOD?
**************************************************************************/
static bool unit_role_defender(const struct unit_type *punittype)
{
  if (utype_move_type(punittype) != LAND_MOVING) {
    return FALSE; /* temporary kluge */
  }
  return (utype_has_role(punittype, L_DEFEND_GOOD));
}

/*************************************************************************
  See if we can find something to defend. Called both by wannabe
  bodyguards and building want estimation code. Returns desirability
  for using this unit as a bodyguard or for defending a city.

  We do not consider units with higher movement than us, or units that
  have different move type than us, as potential charges. Nor do we 
  attempt to bodyguard units with higher defence than us, or military
  units with higher attack than us.

  Requires an initialized warmap!
**************************************************************************/
int look_for_charge(struct player *pplayer, struct unit *punit,
                    struct unit **aunit, struct city **acity)
{
  int dist, def, best = 0;
  int toughness = unit_def_rating_basic_sq(punit);

  if (toughness == 0) {
    /* useless */
    return 0; 
  }

  /* Unit bodyguard */
  unit_list_iterate(pplayer->units, buddy) {
    if (!aiguard_wanted(buddy)
        || !goto_is_sane(punit, buddy->tile, TRUE)
        || unit_move_rate(buddy) > unit_move_rate(punit)
        || DEFENCE_POWER(buddy) >= DEFENCE_POWER(punit)
        || (is_military_unit(buddy) && get_transporter_capacity(buddy) == 0
            && ATTACK_POWER(buddy) <= ATTACK_POWER(punit))
        || uclass_move_type(unit_class(buddy)) != uclass_move_type(unit_class(punit))) { 
      continue;
    }
    if (tile_city(punit->tile)
        && punit->ai.ai_role == AIUNIT_DEFEND_HOME) {
      /* FIXME: Not even if it is an allied city?
       * And why is this *inside* the loop ?*/
      continue; /* Do not run away from defense duty! */
    }
    dist = unit_move_turns(punit, buddy->tile);
    def = (toughness - unit_def_rating_basic_sq(buddy));
    if (def <= 0) {
      continue; /* This should hopefully never happen. */
    }
    if (get_transporter_capacity(buddy) == 0) {
      /* Reduce want based on distance. We can't do this for
       * transports since they move around all the time, leading
       * to hillarious flip-flops */
      def = def >> (dist/2);
    }
    if (def > best) { 
      *aunit = buddy; 
      best = def; 
    }
  } unit_list_iterate_end;

  /* City bodyguard */
  if (uclass_move_type(unit_class(punit)) == LAND_MOVING) {
    struct city *pcity = tile_city(punit->tile);

   city_list_iterate(pplayer->cities, mycity) {
    if (!goto_is_sane(punit, mycity->tile, TRUE)
        || mycity->ai->urgency == 0) {
      continue;
    }
    if (pcity
        && (pcity->ai->grave_danger > 0
            || pcity->ai->urgency > mycity->ai->urgency
            || ((pcity->ai->danger > mycity->ai->danger
                 || punit->ai.ai_role == AIUNIT_DEFEND_HOME)
                && mycity->ai->grave_danger == 0))) {
      /* Do not yoyo between cities in need of defense. Chances are
       * we'll be between cities when we are needed the most! */
      continue;
    }
    dist = unit_move_turns(punit, mycity->tile);
    def = (mycity->ai->danger - assess_defense_quadratic(mycity));
    if (def <= 0) {
      continue;
    }
    def = def >> dist;
    if (def > best && ai_fuzzy(pplayer, TRUE)) { 
      *acity = mycity; 
      best = def; 
    }
   } city_list_iterate_end;
  }

  UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "look_for_charge, best=%d, "
           "type=%s(%d,%d)",
           best * 100 / toughness,
           *acity
           ? city_name(*acity)
           : (*aunit
              ? unit_rule_name(*aunit)
              : ""), 
           *acity
           ? (*acity)->tile->x
           : (*aunit
	      ? (*aunit)->tile->x
	      : 0),
           *acity
           ? (*acity)->tile->y
           : (*aunit
              ? (*aunit)->tile->y
              : 0)
           );
  
  return ((best * 100) / toughness);
}

/********************************************************************** 
  See if we have a specific job for the unit.
***********************************************************************/
static void ai_military_findjob(struct player *pplayer,struct unit *punit)
{
  struct unit_type *punittype = unit_type(punit);

  CHECK_UNIT(punit);

  /* keep barbarians aggresive and primitive */
  if (is_barbarian(pplayer)) {
    if (can_unit_do_activity(punit, ACTIVITY_PILLAGE)
	&& is_land_barbarian(pplayer)) {
      /* land barbarians pillage */
      unit_activity_handling(punit, ACTIVITY_PILLAGE);
    }
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    return;
  }

  /* If I am a bodyguard, check whether I can do my job. */
  if (punit->ai.ai_role == AIUNIT_ESCORT
      || punit->ai.ai_role == AIUNIT_DEFEND_HOME) {
    aiguard_update_charge(punit);
  }
  if (aiguard_has_charge(punit)
      && punit->ai.ai_role == AIUNIT_ESCORT) {
    struct unit *aunit = aiguard_charge_unit(punit);
    struct city *acity = aiguard_charge_city(punit);

    /* Check if the city we are on our way to rescue is still in danger,
     * or the unit we should protect is still alive... */
    if ((aunit && (aiguard_has_guard(aunit) || aiguard_wanted(aunit))
         && unit_def_rating_basic(punit) > unit_def_rating_basic(aunit)) 
        || (acity && city_owner(acity) == unit_owner(punit)
            && acity->ai->urgency != 0 
            && acity->ai->danger > assess_defense_quadratic(acity))) {
      return; /* Yep! */
    } else {
      ai_unit_new_role(punit, AIUNIT_NONE, NULL); /* Nope! */
    }
  }

  /* Is the unit badly damaged? */
  if ((punit->ai.ai_role == AIUNIT_RECOVER
       && punit->hp < punittype->hp)
      || punit->hp < punittype->hp * 0.25) { /* WAG */
    UNIT_LOG(LOGLEVEL_RECOVERY, punit, "set to hp recovery");
    ai_unit_new_role(punit, AIUNIT_RECOVER, NULL);
    return;
  }

  TIMING_LOG(AIT_BODYGUARD, TIMER_START);
  if (unit_role_defender(unit_type(punit))) {
    /* 
     * This is a defending unit that doesn't need to stay put.
     * It needs to defend something, but not necessarily where it's at.
     * Therefore, it will consider becoming a bodyguard. -- Syela 
     */
    struct city *acity = NULL; 
    struct unit *aunit = NULL;
    int val;

    generate_warmap(tile_city(punit->tile), punit);

    val = look_for_charge(pplayer, punit, &aunit, &acity);
    if (acity) {
      ai_unit_new_role(punit, AIUNIT_ESCORT, acity->tile);
      aiguard_assign_guard_city(acity, punit);
      BODYGUARD_LOG(LOG_DEBUG, punit, "going to defend city");
    } else if (aunit) {
      ai_unit_new_role(punit, AIUNIT_ESCORT, aunit->tile);
      aiguard_assign_guard_unit(aunit, punit);
      BODYGUARD_LOG(LOG_DEBUG, punit, "going to defend unit");
    }
  }
  TIMING_LOG(AIT_BODYGUARD, TIMER_STOP);
}

/********************************************************************** 
  Send a unit to the city it should defend. If we already have a city
  it should defend, use the punit->ai.charge field to denote this.
  Otherwise, it will stay put in the city it is in, or find a city
  to reside in, or travel all the way home.

  TODO: Add make homecity.
  TODO: Add better selection of city to defend.
***********************************************************************/
static void ai_military_defend(struct player *pplayer,struct unit *punit)
{
  struct city *pcity = aiguard_charge_city(punit);

  CHECK_UNIT(punit);

  if (!pcity || city_owner(pcity) != pplayer) {
    pcity = tile_city(punit->tile);
    /* Do not stay defending an allied city forever */
    aiguard_clear_charge(punit);
  }

  if (!pcity) {
    /* Try to find a place to rest. Sitting duck out in the wilderness
     * is generally a bad idea, since we protect no cities that way, and
     * it looks silly. */
    pcity = find_closest_owned_city(pplayer, punit->tile, FALSE, NULL);
  }

  if (!pcity) {
    pcity = game_find_city_by_number(punit->homecity);
  }

  if (ai_military_rampage(punit, RAMPAGE_ANYTHING, RAMPAGE_ANYTHING)) {
    /* ... we survived */
    if (pcity) {
      UNIT_LOG(LOG_DEBUG, punit, "go to defend %s", city_name(pcity));
      if (same_pos(punit->tile, pcity->tile)) {
        UNIT_LOG(LOG_DEBUG, punit, "go defend successful");
        punit->ai.done = TRUE;
      } else {
        (void) ai_gothere(pplayer, punit, pcity->tile);
      }
    } else {
      UNIT_LOG(LOG_VERBOSE, punit, "defending nothing...?");
    }
  }
}

/***************************************************************************
 A rough estimate of time (measured in turns) to get to the enemy city, 
 taking into account ferry transfer.
 If boat == NULL, we will build a boat of type boattype right here, so
 we wouldn't have to walk to it.

 Requires ready warmap(s).  Assumes punit is ground or sailing.
***************************************************************************/
int turns_to_enemy_city(const struct unit_type *our_type, struct city *acity,
                        int speed, bool go_by_boat, 
                        struct unit *boat, const struct unit_type *boattype)
{
  struct unit_class *pclass = utype_class(our_type);

  if (pclass->ai.sea_move == MOVE_NONE && go_by_boat) {
    int boatspeed = boattype->move_rate;
    int move_time = (WARMAP_SEACOST(acity->tile)) / boatspeed;
      
    if (utype_has_flag(boattype, F_TRIREME) && move_time > 2) {
      /* Return something prohibitive */
      return 999;
    }
    if (boat) {
      /* Time to get to the boat */
      move_time += (WARMAP_COST(boat->tile) + speed - 1) / speed;
    }
      
    if (!utype_has_flag(our_type, F_MARINES)) {
      /* Time to get off the boat (Marines do it from the vessel) */
      move_time += 1;
    }
      
    return move_time;
  }

  if (pclass->ai.land_move == MOVE_NONE
      && pclass->ai.sea_move != MOVE_NONE) {
    /* We are a boat: time to sail */
    return (WARMAP_SEACOST(acity->tile) + speed - 1) / speed;
  }

  return (WARMAP_COST(acity->tile) + speed - 1) / speed;
}

/************************************************************************
 Rough estimate of time (in turns) to catch up with the enemy unit.  
 FIXME: Take enemy speed into account in a more sensible way

 Requires precomputed warmap.  Assumes punit is ground or sailing. 
************************************************************************/ 
int turns_to_enemy_unit(const struct unit_type *our_type,
			int speed, struct tile *ptile,
                        const struct unit_type *enemy_type)
{
  struct unit_class *pclass = utype_class(our_type);
  int dist;

  if (pclass->ai.land_move != MOVE_NONE) {
    dist = WARMAP_COST(ptile);
  } else {
    dist = WARMAP_SEACOST(ptile);
  }

  /* if dist <= move_rate, we hit the enemy right now */    
  if (dist > speed) { 
    /* Weird attempt to take into account enemy running away... */
    dist *= enemy_type->move_rate;
    if (utype_has_flag(enemy_type, F_IGTER)) {
      dist *= 3;
    }
  }
  
  return (dist + speed - 1) / speed;
}

/*************************************************************************
  Mark invasion possibilities of punit in the surrounding cities. The
  given radius limites the area which is searched for cities. The
  center of the area is either the unit itself (dest == FALSE) or the
  destination of the current goto (dest == TRUE). The invasion threat
  is marked in pcity->ai.invasion by setting the "which" bit (to
  tell attack which can only kill units from occupy possibility).

  If dest == TRUE then a valid goto is presumed.
**************************************************************************/
static void invasion_funct(struct unit *punit, bool dest, int radius,
			   int which)
{
  struct tile *ptile;
  struct player *pplayer = unit_owner(punit);
  struct ai_data *ai = ai_data_get(pplayer);

  CHECK_UNIT(punit);

  if (dest) {
    ptile = punit->goto_tile;
  } else {
    ptile = punit->tile;
  }

  square_iterate(ptile, radius, tile1) {
    struct city *pcity = tile_city(tile1);

    if (pcity
        && HOSTILE_PLAYER(pplayer, ai, city_owner(pcity))
	&& (dest || !has_defense(pcity))) {
      int attacks;

      if (unit_has_type_flag(punit, F_ONEATTACK)) {
        attacks = 1;
      } else {
        attacks = unit_type(punit)->move_rate;
      }
      pcity->ai->invasion.attack += attacks;
      if (which == INVASION_OCCUPY) {
        pcity->ai->invasion.occupy++;
      }
    }
  } square_iterate_end;
}

/*************************************************************************
  Find something to kill! This function is called for units to find 
  targets to destroy and for cities that want to know if they should
  build offensive units. Target location returned in (x, y), want as
  function return value.

  punit->id == 0 means that the unit is virtual (considered to be built).
**************************************************************************/
int find_something_to_kill(struct player *pplayer, struct unit *punit,
			   struct tile **dest_tile)
{
  struct ai_data *ai = ai_data_get(pplayer);
  /* basic attack */
  int attack_value = unit_att_rating(punit);
  /* Enemy defence rating */
  int vuln;
  /* Benefit from killing the target */
  int benefit;
  /* Number of enemies there */
  int victim_count;
  /* Want (amortized) of the operaton */
  int want;
  /* Best of all wants */
  int best = 0;
  /* Our total attack value with reinforcements */
  int attack;
  int move_time, move_rate;
  Continent_id con = tile_continent(punit->tile);
  struct unit *pdef;
  int maxd, needferry;
  /* Do we have access to sea? */
  bool harbor = FALSE;
  /* Build cost of the attacker (+adjustments) */
  int bcost, bcost_bal;
  bool handicap = ai_handicap(pplayer, H_TARGETS);
  struct unit *ferryboat = NULL;
  /* Type of our boat (a future one if ferryboat == NULL) */
  struct unit_type *boattype = NULL;
  bool unhap = FALSE;
  struct city *pcity;
  /* this is a kluge, because if we don't set x and y with !punit->id,
   * p_a_w isn't called, and we end up not wanting ironclads and therefore 
   * never learning steam engine, even though ironclads would be very 
   * useful. -- Syela */
  int bk = 0; 
  struct unit_class *pclass = unit_class(punit);

  /*** Very preliminary checks */
  *dest_tile = punit->tile;

  if (utype_fuel(unit_type(punit)) || is_losing_hp(punit)) {
    /* Don't know what to do with them! */
    /* This is not LOG_ERROR in stable branch, as calling
     * fstk is in many cases right thing to do when custom
     * rulesets are used - and callers correctly handle cases
     * where fstk failed to find target.
     * FIXME: handling of units different to those in default
     * ruleset should be improved. */
    UNIT_LOG(LOG_VERBOSE, punit, "find_something_to_kill() bad unit type");
    return 0;
  }

  if (attack_value == 0) {
    /* A very poor attacker...
     *  probably low on HP */
    return 0;
  }

  TIMING_LOG(AIT_FSTK, TIMER_START);

  /*** Part 1: Calculate targets ***/
  /* This horrible piece of code attempts to calculate the attractiveness of
   * enemy cities as targets for our units, by checking how many units are
   * going towards it or are near it already.
   */

  /* First calculate in nearby units */
  players_iterate(aplayer) {
    /* See comment below in next usage of HOSTILE_PLAYER. */
    if ((punit->id == 0 && !HOSTILE_PLAYER(pplayer, ai, aplayer))
        || (punit->id != 0 && !pplayers_at_war(pplayer, aplayer))) {
      continue;
    }
    city_list_iterate(aplayer->cities, acity) {
      reinforcements_cost_and_value(punit, acity->tile,
                                    &acity->ai->attack, &acity->ai->bcost);
      acity->ai->invasion.attack = 0;
      acity->ai->invasion.occupy = 0;
    } city_list_iterate_end;
  } players_iterate_end;

  /* Second, calculate in units on their way there, and mark targets for
   * invasion */
  unit_list_iterate(pplayer->units, aunit) {
    if (aunit == punit) {
      continue;
    }

    /* dealing with invasion stuff */
    if (IS_ATTACKER(aunit)) {
      if (aunit->activity == ACTIVITY_GOTO) {
        invasion_funct(aunit, TRUE, 0,
                       (COULD_OCCUPY(aunit) ? INVASION_OCCUPY : INVASION_ATTACK));
        if ((pcity = tile_city(aunit->goto_tile))) {
          pcity->ai->attack += unit_att_rating(aunit);
          pcity->ai->bcost += unit_build_shield_cost(aunit);
        } 
      }
      invasion_funct(aunit, FALSE, unit_move_rate(aunit) / SINGLE_MOVE,
                     (COULD_OCCUPY(aunit) ? INVASION_OCCUPY : INVASION_ATTACK));
    } else if (aunit->ai.passenger != 0 &&
               !same_pos(aunit->tile, punit->tile)) {
      /* It's a transport with reinforcements */
      if (aunit->activity == ACTIVITY_GOTO) {
        invasion_funct(aunit, TRUE, 1, INVASION_OCCUPY);
      }
      invasion_funct(aunit, FALSE, 2, INVASION_OCCUPY);
    }
  } unit_list_iterate_end;
  /* end horrible initialization subroutine */

  /*** Part 2: Now pick one target ***/
  /* We first iterate through all cities, then all units, looking
   * for a viable target. We also try to gang up on the target
   * to avoid spreading out attacks too widely to be inefficient.
   */

  pcity = tile_city(punit->tile);

  if (pcity && (punit->id == 0 || pcity->id == punit->homecity)) {
    /* I would have thought unhappiness should be taken into account 
     * irrespectfully the city in which it will surface...  GB */ 
    unhap = ai_assess_military_unhappiness(pcity);
  }

  move_rate = unit_move_rate(punit);
  if (unit_has_type_flag(punit, F_IGTER)) {
    move_rate *= 3;
  }

  maxd = MIN(6, move_rate) * THRESHOLD + 1;

  bcost = unit_build_shield_cost(punit);
  bcost_bal = build_cost_balanced(unit_type(punit));

  /* most flexible but costs milliseconds */
  generate_warmap(tile_city(*dest_tile), punit);

  if (pclass->ai.sea_move == MOVE_NONE) {
    /* We need boat to move over sea */

    /* First check if we can use the boat we are currently loaded to */
    if (punit->transported_by != -1) {
      ferryboat = player_find_unit_by_id(unit_owner(punit),
                                         punit->transported_by);

      /* We are already in, so don't ask for free capacity */
      if (ferryboat == NULL || !is_boat_free(ferryboat, punit, 0)) {
        /* No, we cannot control current boat */
        ferryboat = NULL;
      }
    }

    if (ferryboat == NULL) {
      /* Try to find new boat */
      int boatid = aiferry_find_boat(punit, 1, NULL);
      ferryboat = player_find_unit_by_id(pplayer, boatid);
    }
  }

  if (ferryboat) {
    boattype = unit_type(ferryboat);
    generate_warmap(tile_city(ferryboat->tile), ferryboat);
  } else {
    boattype = best_role_unit_for_player(pplayer, L_FERRYBOAT);
    if (boattype == NULL) {
      /* We pretend that we can have the simplest boat -- to stimulate tech */
      boattype = get_role_unit(L_FERRYBOAT, 0);
    }
  }

  if (pclass->ai.sea_move == MOVE_NONE
      && punit->id == 0 
      && is_ocean_near_tile(punit->tile)) {
    harbor = TRUE;
  }

  players_iterate(aplayer) {
    int reserves;

    /* For the virtual unit case, which is when we are called to evaluate
     * which units to build, we want to calculate in danger and which
     * players we want to make war with in the future. We do _not_ want
     * to do this when actually making attacks. */
    if ((punit->id == 0 && !HOSTILE_PLAYER(pplayer, ai, aplayer))
        || (punit->id != 0 && !pplayers_at_war(pplayer, aplayer))) {
      /* Not an enemy */
      continue;
    }

    city_list_iterate(aplayer->cities, acity) {
      bool go_by_boat = (pclass->ai.sea_move == MOVE_NONE
                         && !(goto_is_sane(punit, acity->tile, TRUE) 
                              && WARMAP_COST(acity->tile) < maxd));

      if (!is_native_tile(unit_type(punit), acity->tile)
          && !can_attack_non_native(unit_type(punit))) {
        /* Can't attack this city. It is on land. */
        continue;
      }

      if (handicap && !map_is_known(acity->tile, pplayer)) {
        /* Can't see it */
        continue;
      }

      if (go_by_boat 
          && (!(ferryboat || harbor)
              || WARMAP_SEACOST(acity->tile) > 6 * THRESHOLD)) {
        /* Too far or impossible to go by boat */
        continue;
      }
      
      if (pclass->ai.land_move == MOVE_NONE
          && WARMAP_SEACOST(acity->tile) >= maxd) {
        /* Too far to sail */
        continue;
      }
      
      if ((pdef = get_defender(punit, acity->tile))) {
        vuln = unit_def_rating_sq(punit, pdef);
        benefit = unit_build_shield_cost(pdef);
      } else { 
        vuln = 0; 
        benefit = 0; 
      }
      
      move_time = turns_to_enemy_city(unit_type(punit), acity, move_rate, 
                                      go_by_boat, ferryboat, boattype);

      if (move_time > 1) {
        struct unit_type *def_type = ai_choose_defender_versus(acity, punit);

        if (def_type) {
          int v = unittype_def_rating_sq(unit_type(punit), def_type,
                                         city_owner(acity), acity->tile, FALSE,
                                         do_make_unit_veteran(acity, def_type));
          if (v > vuln) {
            /* They can build a better defender! */ 
            vuln = v; 
            benefit = utype_build_shield_cost(def_type); 
          }
        }
      }

      reserves = acity->ai->invasion.attack -
        unit_list_size(acity->tile->units);

      if (reserves >= 0) {
        /* We have enough units to kill all the units in the city */
        if (reserves > 0
            && (COULD_OCCUPY(punit) || acity->ai->invasion.occupy > 0)) {
          /* There are units able to occupy the city after all defenders
           * are killed! */
          benefit += 60;
        }
      }

      attack = (attack_value + acity->ai->attack) 
        * (attack_value + acity->ai->attack);
      /* Avoiding handling upkeep aggregation this way -- Syela */
      
      /* AI was not sending enough reinforcements to totally wipe out a city
       * and conquer it in one turn.  
       * This variable enables total carnage. -- Syela */
      victim_count 
        = unit_list_size(acity->tile->units) + 1;

      if (!COULD_OCCUPY(punit) && !pdef) {
        /* Nothing there to bash and we can't occupy! 
         * Not having this check caused warships yoyoing */
        want = 0;
      } else if (move_time > THRESHOLD) {
        /* Too far! */
        want = 0;
      } else if (COULD_OCCUPY(punit)
                 && acity->ai->invasion.attack > 0
                 && acity->ai->invasion.occupy == 0) {
        /* Units able to occupy really needed there! */
        want = bcost * SHIELD_WEIGHTING;
      } else {
        int a_squared = acity->ai->attack * acity->ai->attack;
        
        want = kill_desire(benefit, attack, (bcost + acity->ai->bcost), 
                           vuln, victim_count);
        if (benefit * a_squared > acity->ai->bcost * vuln) {
          /* If there're enough units to do the job, we don't need this
           * one. */
          /* FIXME: The problem with ai.bcost is that bigger it is, less is
           * our desire to go help other units.  Now suppose we need five
           * cavalries to take over a city, we have four (which is not
           * enough), then we will be severely discouraged to build the
           * fifth one.  Where is logic in this??!?! --GB */
          want -= kill_desire(benefit, a_squared, acity->ai->bcost, 
                              vuln, victim_count);
        }
      }
      want -= move_time * (unhap ? SHIELD_WEIGHTING + 2 * TRADE_WEIGHTING 
                           : SHIELD_WEIGHTING);
      /* build_cost of ferry */
      needferry = (go_by_boat && !ferryboat
		   ? utype_build_shield_cost(boattype) : 0);
      /* FIXME: add time to build the ferry? */
      want = military_amortize(pplayer, game_find_city_by_number(punit->homecity),
                               want, MAX(1, move_time),
			       bcost_bal + needferry);

      /* BEGIN STEAM-ENGINES-ARE-OUR-FRIENDS KLUGE */
      if (want <= 0 && punit->id == 0 && best == 0) {
        int bk_e = military_amortize(pplayer,
				     game_find_city_by_number(punit->homecity),
                                     benefit * SHIELD_WEIGHTING, 
                                     MAX(1, move_time),
				     bcost_bal + needferry);

        if (bk_e > bk) {
	  *dest_tile = acity->tile;
          bk = bk_e;
        }
      }
      /* END STEAM-ENGINES KLUGE */
      
      if (punit->id != 0 && ferryboat && pclass->ai.sea_move == MOVE_NONE) {
        UNIT_LOG(LOG_DEBUG, punit, "in fstk with boat %s@(%d, %d) -> %s@(%d, %d)"
                 " (go_by_boat=%d, move_time=%d, want=%d, best=%d)",
                 unit_rule_name(ferryboat),
                 TILE_XY(ferryboat->tile),
                 city_name(acity),
                 TILE_XY(acity->tile), 
                 go_by_boat, move_time, want, best);
      }
      
      if (want > best && ai_fuzzy(pplayer, TRUE)) {
        /* Yes, we like this target */
        if (punit->id != 0 && pclass->ai.sea_move == MOVE_NONE
            && !unit_has_type_flag(punit, F_MARINES)
            && tile_continent(acity->tile) != con) {
          /* a non-virtual ground unit is trying to attack something on 
           * another continent.  Need a beachhead which is adjacent 
           * to the city and an available ocean tile */
	  struct tile *btile;

          if (find_beachhead(punit, acity->tile, &btile)) { 
            best = want;
	    *dest_tile = acity->tile;
            /* the ferryboat needs to target the beachhead, but the unit 
             * needs to target the city itself.  This is a little weird, 
             * but it's the best we can do. -- Syela */
          } /* else do nothing, since we have no beachhead */
        } else {
          best = want;
	  *dest_tile = acity->tile;
        } /* end need-beachhead else */
      }
    } city_list_iterate_end;

    attack = unit_att_rating_sq(punit);
    /* I'm not sure the following code is good but it seems to be adequate. 
     * I am deliberately not adding ferryboat code to the unit_list_iterate. 
     * -- Syela */
    unit_list_iterate(aplayer->units, aunit) {
      if (tile_city(aunit->tile)) {
        /* already dealt with it */
        continue;
      }

      if (handicap && !map_is_known(aunit->tile, pplayer)) {
        /* Can't see the target */
        continue;
      }

      if ((unit_has_type_flag(aunit, F_HELP_WONDER) || unit_has_type_flag(aunit, F_TRADE_ROUTE))
          && punit->id == 0) {
        /* We will not build units just to chase caravans */
        continue;
      }

      /* We have to assume the attack is diplomatically ok.
       * We cannot use can_player_attack_tile, because we might not
       * be at war with aplayer yet */
      if (!can_unit_attack_all_at_tile(punit, aunit->tile)
          || !(aunit == get_defender(punit, aunit->tile))) {
        /* We cannot attack it, or it is not the main defender. */
        continue;
      }

      if (pclass->ai.sea_move != MOVE_FULL
          && (tile_continent(aunit->tile) != con 
              || WARMAP_COST(aunit->tile) >= maxd)) {
        /* Maybe impossible or too far to walk */
        continue;
      }

      if (pclass->ai.land_move != MOVE_FULL
          && (!goto_is_sane(punit, aunit->tile, TRUE)
              || WARMAP_SEACOST(aunit->tile) >= maxd)) {
        /* Maybe impossible or too far to sail */
        continue;
      }

      vuln = unit_def_rating_sq(punit, aunit);
      benefit = unit_build_shield_cost(aunit);
 
      move_time = turns_to_enemy_unit(unit_type(punit), move_rate, 
                                      aunit->tile, unit_type(aunit));

      if (move_time > THRESHOLD) {
        /* Too far */
        want = 0;
      } else {
        want = kill_desire(benefit, attack, bcost, vuln, 1);
          /* Take into account maintainance of the unit */
          /* FIXME: Depends on the government */
        want -= move_time * SHIELD_WEIGHTING;
        /* Take into account unhappiness 
         * (costs 2 luxuries to compensate) */
        want -= (unhap ? 2 * move_time * TRADE_WEIGHTING : 0);
      }
      want = military_amortize(pplayer, game_find_city_by_number(punit->homecity),
                               want, MAX(1, move_time), bcost_bal);
      if (want > best && ai_fuzzy(pplayer, TRUE)) {
        best = want;
	*dest_tile = aunit->tile;
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  TIMING_LOG(AIT_FSTK, TIMER_STOP);

  return(best);
}

/**********************************************************************
  Find safe city to recover in. An allied player's city is just as 
  good as one of our own, since both replenish our hitpoints and
  reduce unhappiness.

  TODO: Actually check how safe the city is. This is a difficult
  decision not easily taken, since we also want to protect unsafe
  cities, at least most of the time.
***********************************************************************/
struct city *find_nearest_safe_city(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  struct city *acity = NULL;
  int best = 6 * THRESHOLD + 1, cur;
  bool ground = is_ground_unit(punit);

  CHECK_UNIT(punit);

  generate_warmap(tile_city(punit->tile), punit);
  players_iterate(aplayer) {
    if (pplayers_allied(pplayer,aplayer)) {
      city_list_iterate(aplayer->cities, pcity) {
        if (ground) {
          cur = WARMAP_COST(pcity->tile);
        } else {
          cur = WARMAP_SEACOST(pcity->tile);
        }
	/* Note the "player" here is the unit owner NOT the city owner. */
	if (get_unittype_bonus(unit_owner(punit), pcity->tile, unit_type(punit),
			       EFT_HP_REGEN) > 0) {
	  cur /= 3;
	}
        if (cur < best) {
          best = cur;
          acity = pcity;
        }
      } city_list_iterate_end;
    }
  } players_iterate_end;
  if (best > 6 * THRESHOLD) {
    return NULL;
  }
  return acity;
}

/*************************************************************************
  Go berserk, assuming there are no targets nearby.
  TODO: Is it not possible to remove this special casing for barbarians?
  FIXME: enum unit_move_result
**************************************************************************/
static void ai_military_attack_barbarian(struct player *pplayer,
					 struct unit *punit)
{
  struct city *pc;
  bool any_continent = FALSE;

  if (punit->transported_by != -1) {
    /* If we are in transport, we can go to any continent.
     * Actually, we are not currently in a continent where to stay. */
    any_continent = TRUE;
  }

  if ((pc = dist_nearest_city(pplayer, punit->tile, any_continent, TRUE))) {
    if (can_unit_exist_at_tile(punit, punit->tile)) {
      UNIT_LOG(LOG_DEBUG, punit, "Barbarian heading to conquer %s",
               city_name(pc));
      (void) ai_gothere(pplayer, punit, pc->tile);
    } else {
      struct unit *ferry = NULL;

      if (punit->transported_by != -1) {
        ferry = game_find_unit_by_number(punit->transported_by);

        /* We already are in a boat so it needs no
         * free capacity */
        if (!is_boat_free(ferry, punit, 0)) {
          /* We cannot control our ferry. */
          ferry = NULL;
        }
      } else {
        /* We are not in a boat yet. Search for one. */
        unit_list_iterate(punit->tile->units, aunit) {
          if (is_boat_free(aunit, punit, 1)) {
            ferry = aunit;
            punit->transported_by = ferry->id;
            break;
          }
        } unit_list_iterate_end;
      }

      if (ferry) {
	UNIT_LOG(LOG_DEBUG, punit, "Barbarian sailing to conquer %s",
		 city_name(pc));
	(void)aiferry_goto_amphibious(ferry, punit, pc->tile);
      } else {
        /* This is not an error. Somebody else might be in charge
         * of the ferry. */
        UNIT_LOG(LOG_DEBUG, punit, "unable to find barbarian ferry");
      }
    }
  } else {
    UNIT_LOG(LOG_DEBUG, punit, "Barbarian find no target city");
  }
}

/*************************************************************************
  This does the attack until we have used up all our movement, unless we
  should safeguard a city.  First we rampage nearby, then we go
  looking for trouble elsewhere. If there is nothing to kill, sailing units 
  go home, others explore while barbs go berserk.
**************************************************************************/
static void ai_military_attack(struct player *pplayer, struct unit *punit)
{
  struct tile *dest_tile;
  int id = punit->id;
  int ct = 10;
  struct city *pcity = NULL;
  struct tile *start_tile = punit->tile;

  CHECK_UNIT(punit);

  /* First find easy nearby enemies, anything better than pillage goes.
   * NB: We do not need to repeat ai_military_rampage, it is repeats itself
   * until it runs out of targets. */
  /* FIXME: 1. ai_military_rampage never checks if it should defend newly
   * conquered cities.
   * FIXME: 2. would be more convenient if it returned FALSE if we run out 
   * of moves too.*/
  if (!ai_military_rampage(punit, RAMPAGE_ANYTHING, RAMPAGE_ANYTHING)) {
    return; /* we died */
  }
  
  if (punit->moves_left <= 0) {
    return;
  }

  /* Main attack loop */
  do {
    /* Then find enemies the hard way */
    find_something_to_kill(pplayer, punit, &dest_tile);
    if (!same_pos(punit->tile, dest_tile)) {

      if (!is_tiles_adjacent(punit->tile, dest_tile)
          || !can_unit_attack_tile(punit, dest_tile)) {
        /* Adjacent and can't attack usually means we are not marines
         * and on a ferry. This fixes the problem (usually). */
        UNIT_LOG(LOG_DEBUG, punit, "mil att gothere -> (%d,%d)", 
                 dest_tile->x, dest_tile->y);
        if (!ai_gothere(pplayer, punit, dest_tile)) {
          /* Died or got stuck */
	  if (game_find_unit_by_number(id)
	      && punit->moves_left && punit->tile != start_tile) {
	    /* Got stuck. Possibly because of adjacency to an
	     * enemy unit. Perhaps we are in luck and are now next to a
	     * tempting target? Let's find out... */
	    (void) ai_military_rampage(punit,
				       RAMPAGE_ANYTHING, RAMPAGE_ANYTHING);
	  }
          return;
        }
        if (punit->moves_left <= 0) {
          return;
        }
        /* Either we're adjacent or we sitting on the tile. We might be
         * sitting on the tile if the enemy that _was_ sitting there 
         * attacked us and died _and_ we had enough movement to get there */
        if (same_pos(punit->tile, dest_tile)) {
          UNIT_LOG(LOG_DEBUG, punit, "mil att made it -> (%d,%d)",
                 dest_tile->x, dest_tile->y);
          break;
        }
      }
      
      /* Close combat. fstk sometimes want us to attack an adjacent
       * enemy that rampage wouldn't */
      UNIT_LOG(LOG_DEBUG, punit, "mil att bash -> %d, %d",
	       dest_tile->x, dest_tile->y);
      if (!ai_unit_attack(punit, dest_tile)) {
        /* Died */
          return;
      }

    } else {
      /* FIXME: This happens a bit too often! */
      UNIT_LOG(LOG_DEBUG, punit, "fstk didn't find us a worthy target!");
      /* No worthy enemies found, so abort loop */
      ct = 0;
    }

    ct--; /* infinite loops from railroads must be stopped */
  } while (punit->moves_left > 0 && ct > 0);

  /* Cleanup phase */
  if (punit->moves_left == 0) {
    return;
  }
  pcity = find_nearest_safe_city(punit);
  if (is_sailing_unit(punit) && pcity) {
    /* Sail somewhere */
    UNIT_LOG(LOG_DEBUG, punit, "sailing to nearest safe house.");
    (void) ai_unit_goto(punit, pcity->tile);
  } else if (!is_barbarian(pplayer)) {
    /* Nothing else to do, so try exploring. */
    switch (ai_manage_explorer(punit)) {
    case MR_DEATH:
      /* don't use punit! */
      return;
    case MR_OK:
      UNIT_LOG(LOG_DEBUG, punit, "nothing else to do, so exploring");
      break;
    default:
      UNIT_LOG(LOG_DEBUG, punit, "nothing to do - no more exploring either");
      break;
    };
  } else {
    /* You can still have some moves left here, but barbarians should
       not sit helplessly, but advance towards nearest known enemy city */
    UNIT_LOG(LOG_DEBUG, punit, "attack: barbarian");
    ai_military_attack_barbarian(pplayer, punit);
  }
  if ((punit = game_find_unit_by_number(id)) && punit->moves_left > 0) {
    UNIT_LOG(LOG_DEBUG, punit, "attack: giving up unit to defense");
    ai_military_defend(pplayer, punit);
  }
}


/*************************************************************************
  Send the caravan to the specified city, or make it help the wonder /
  trade, if it's already there.  After this call, the unit may no longer
  exist (it might have been used up, or may have died travelling).
**************************************************************************/
static void ai_caravan_goto(struct player *pplayer,
                            struct unit *punit,
                            const struct city *pcity,
                            bool help_wonder)
{
  bool alive = TRUE;
  assert(pcity);

  /* if we're not there yet, and we can move, move. */
  if (!same_pos(pcity->tile, punit->tile) && punit->moves_left != 0) {
    freelog(LOG_CARAVAN, "%s %s[%d](%d,%d) going to %s in %s",
            nation_rule_name(nation_of_unit(punit)),
            unit_rule_name(punit),
            punit->id,
            TILE_XY(punit->tile),
            help_wonder ? "help a wonder" : "trade", city_name(pcity));
    alive = ai_unit_goto(punit, pcity->tile); 
  }

  /* if moving didn't kill us, and we got to the destination, handle it. */
  if (alive && same_pos(pcity->tile, punit->tile)) {
    if (help_wonder) {
        /*
         * We really don't want to just drop all caravans in immediately.
         * Instead, we want to aggregate enough caravans to build instantly.
         * -AJS, 990704
         */
      freelog(LOG_CARAVAN, "%s %s[%d](%d,%d) helps build wonder in %s",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit),
              punit->id,
              TILE_XY(punit->tile),
              city_name(pcity));
	handle_unit_help_build_wonder(pplayer, punit->id);
    } else {
      freelog(LOG_CARAVAN, "%s %s[%d](%d,%d) creates trade route in %s",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit),
              punit->id,
              TILE_XY(punit->tile),
              city_name(pcity));
      handle_unit_establish_trade(pplayer, punit->id);
    }
  }
}

/*************************************************************************
  For debugging, print out information about every city we come to when
  optimizing the caravan.
 *************************************************************************/
static void caravan_optimize_callback(const struct caravan_result *result,
                                      void *data)
{
  const struct unit *caravan = data;

  freelog(LOG_CARAVAN2, "%s %s[%d](%d,%d) %s: %s %s worth %g",
	  nation_rule_name(nation_of_unit(caravan)),
          unit_rule_name(caravan),
          caravan->id,
          TILE_XY(caravan->tile),
	  city_name(result->src),
	  result->help_wonder ? "wonder in" : "trade to",
	  city_name(result->dest),
	  result->value);
}

/*************************************************************************
  Use caravans for building wonders, or send caravans to establish
  trade with a city on the same continent, owned by yourself or an
  ally.

  TODO list
  - use ferries.
**************************************************************************/
static void ai_manage_caravan(struct player *pplayer, struct unit *punit)
{
  struct caravan_parameter parameter;
  struct caravan_result result;

  CHECK_UNIT(punit);

  if (punit->ai.ai_role != AIUNIT_NONE) {
             return;
           }

  if (unit_has_type_flag(punit, F_TRADE_ROUTE) || unit_has_type_flag(punit, F_HELP_WONDER)) {
    caravan_parameter_init_from_unit(&parameter, punit);
    if (fc_log_level >= LOG_CARAVAN2) {
      parameter.callback = caravan_optimize_callback;
      parameter.callback_data = punit;
        }
    caravan_find_best_destination(punit, &parameter, &result);
      }

  if (result.dest != NULL) {
    ai_caravan_goto(pplayer, punit, result.dest, result.help_wonder);
    return; /* that may have clobbered the unit */
    }
  else {
    /*
     * We have nowhere to go!
     * Should we become a defensive unit?
     */
  }
}

/*************************************************************************
 This function goes wait a unit in a city for the hitpoints to recover. 
 If something is attacking our city, kill it yeahhh!!!.
**************************************************************************/
static void ai_manage_hitpoint_recovery(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(punit->tile);
  struct city *safe = NULL;
  struct unit_type *punittype = unit_type(punit);

  CHECK_UNIT(punit);

  if (pcity) {
    /* rest in city until the hitpoints are recovered, but attempt
       to protect city from attack (and be opportunistic too)*/
    if (ai_military_rampage(punit, RAMPAGE_ANYTHING, 
                            RAMPAGE_FREE_CITY_OR_BETTER)) {
      UNIT_LOG(LOGLEVEL_RECOVERY, punit, "recovering hit points.");
    } else {
      return; /* we died heroically defending our city */
    }
  } else {
    /* goto to nearest city to recover hit points */
    /* just before, check to see if we can occupy an undefended enemy city */
    if (!ai_military_rampage(punit, RAMPAGE_FREE_CITY_OR_BETTER, 
                             RAMPAGE_FREE_CITY_OR_BETTER)) { 
      return; /* oops, we died */
    }

    /* find city to stay and go there */
    safe = find_nearest_safe_city(punit);
    if (safe) {
      UNIT_LOG(LOGLEVEL_RECOVERY, punit, "going to %s to recover",
               city_name(safe));
      if (!ai_unit_goto(punit, safe->tile)) {
        freelog(LOGLEVEL_RECOVERY, "died trying to hide and recover");
        return;
      }
    } else {
      /* oops */
      UNIT_LOG(LOGLEVEL_RECOVERY, punit, "didn't find a city to recover in!");
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
      ai_military_attack(pplayer, punit);
      return;
    }
  }

  /* is the unit still damaged? if true recover hit points, if not idle */
  if (punit->hp == punittype->hp) {
    /* we are ready to go out and kick ass again */
    UNIT_LOG(LOGLEVEL_RECOVERY, punit, "ready to kick ass again!");
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);  
    return;
  } else {
    punit->ai.done = TRUE; /* sit tight */
  }
}

/**************************************************************************
  Decide what to do with a military unit. It will be managed once only.
  It is up to the caller to try again if it has moves left.
**************************************************************************/
void ai_manage_military(struct player *pplayer, struct unit *punit)
{
  int id = punit->id;

  CHECK_UNIT(punit);

  /* "Escorting" aircraft should not do anything. They are activated
   * by their transport or charge.  We do _NOT_ set them to 'done'
   * since they may need be activated once our charge moves. */
  if (punit->ai.ai_role == AIUNIT_ESCORT && utype_fuel(unit_type(punit))) {
    return;
  }

  if ((punit->activity == ACTIVITY_SENTRY
       || punit->activity == ACTIVITY_FORTIFIED)
      && ai_handicap(pplayer, H_AWAY)) {
    /* Don't move sentried or fortified units controlled by a player
     * in away mode. */
    punit->ai.done = TRUE;
    return;
  }

  /* Since military units re-evaluate their actions every turn,
     we must make sure that previously reserved ferry is freed. */
  aiferry_clear_boat(punit);

  TIMING_LOG(AIT_HUNTER, TIMER_START);
  /* Try hunting with this unit */
  if (ai_hunter_qualify(pplayer, punit)) {
    int result, sanity = punit->id;

    UNIT_LOG(LOGLEVEL_HUNT, punit, "is qualified as hunter");
    result = ai_hunter_manage(pplayer, punit);
    if (!game_find_unit_by_number(sanity)) {
      TIMING_LOG(AIT_HUNTER, TIMER_STOP);
      return; /* died */
    }
    if (result == -1) {
      (void) ai_hunter_manage(pplayer, punit); /* More carnage */
      TIMING_LOG(AIT_HUNTER, TIMER_STOP);
      return;
    } else if (result >= 1) {
      TIMING_LOG(AIT_HUNTER, TIMER_STOP);
      return; /* Done moving */
    } else if (punit->ai.ai_role == AIUNIT_HUNTER) {
      /* This should be very rare */
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    }
  } else if (punit->ai.ai_role == AIUNIT_HUNTER) {
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);
  }
  TIMING_LOG(AIT_HUNTER, TIMER_STOP);

  /* Do we have a specific job for this unit? If not, we default
   * to attack. */
  ai_military_findjob(pplayer, punit);

  switch (punit->ai.ai_role) {
  case AIUNIT_AUTO_SETTLER:
  case AIUNIT_BUILD_CITY:
    assert(FALSE); /* This is not the place for this role */
    break;
  case AIUNIT_DEFEND_HOME:
    TIMING_LOG(AIT_DEFENDERS, TIMER_START);
    ai_military_defend(pplayer, punit);
    TIMING_LOG(AIT_DEFENDERS, TIMER_STOP);
    break;
  case AIUNIT_ATTACK:
  case AIUNIT_NONE:
    TIMING_LOG(AIT_ATTACK, TIMER_START);
    ai_military_attack(pplayer, punit);
    TIMING_LOG(AIT_ATTACK, TIMER_STOP);
    break;
  case AIUNIT_ESCORT: 
    TIMING_LOG(AIT_BODYGUARD, TIMER_START);
    ai_military_bodyguard(pplayer, punit);
    TIMING_LOG(AIT_BODYGUARD, TIMER_STOP);
    break;
  case AIUNIT_EXPLORE:
    switch (ai_manage_explorer(punit)) {
    case MR_DEATH:
      /* don't use punit! */
      return;
    case MR_OK:
      UNIT_LOG(LOG_DEBUG, punit, "more exploring");
      break;
    default:
      UNIT_LOG(LOG_DEBUG, punit, "no more exploring either");
      break;
    };
    punit->ai.done = (punit->moves_left <= 0);
    break;
  case AIUNIT_RECOVER:
    TIMING_LOG(AIT_RECOVER, TIMER_START);
    ai_manage_hitpoint_recovery(punit);
    TIMING_LOG(AIT_RECOVER, TIMER_STOP);
    break;
  case AIUNIT_HUNTER:
    assert(FALSE); /* dealt with above */
    break;
  default:
    assert(FALSE);
  }

  /* If we are still alive, either sentry or fortify. */
  if ((punit = game_find_unit_by_number(id))) {
    struct city *pcity = tile_city(punit->tile);

    if (unit_list_find(punit->tile->units, punit->ai.ferryboat)) {
      unit_activity_handling(punit, ACTIVITY_SENTRY);
    } else if (pcity || punit->activity == ACTIVITY_IDLE) {
      /* We do not need to fortify in cities - we fortify and sentry
       * according to home defense setup, for easy debugging. */
      if (!pcity || punit->ai.ai_role == AIUNIT_DEFEND_HOME) {
        if (punit->activity == ACTIVITY_IDLE
            || punit->activity == ACTIVITY_SENTRY) {
          unit_activity_handling(punit, ACTIVITY_FORTIFYING);
        }
      } else {
        unit_activity_handling(punit, ACTIVITY_SENTRY);
      }
    }
  }
}

/**************************************************************************
  Barbarian units may disband spontaneously if their age is more than
  BARBARIAN_MIN_LIFESPAN, they are not in cities, and they are far from
  any enemy units. It is to remove barbarians that do not engage into any
  activity for a long time.
**************************************************************************/
static bool unit_can_be_retired(struct unit *punit)
{
  if (punit->birth_turn + BARBARIAN_MIN_LIFESPAN > game.info.turn) {
    return FALSE;
  }

  if (is_allied_city_tile
      ((punit->tile), unit_owner(punit))) return FALSE;

  /* check if there is enemy nearby */
  square_iterate(punit->tile, 3, ptile) {
    if (is_enemy_city_tile(ptile, unit_owner(punit)) ||
	is_enemy_unit_tile(ptile, unit_owner(punit)))
      return FALSE;
  }
  square_iterate_end;

  return TRUE;
}

/**************************************************************************
 manage one unit
 Careful: punit may have been destroyed upon return from this routine!

 Gregor: This function is a very limited approach because if a unit has
 several flags the first one in order of appearance in this function
 will be used.
**************************************************************************/
void ai_manage_unit(struct player *pplayer, struct unit *punit)
{
  struct unit *bodyguard = aiguard_guard_of(punit);
  bool is_ferry = FALSE;

  CHECK_UNIT(punit);

  /* Don't manage the unit if it is under human orders. */
  if (unit_has_orders(punit)) {
    UNIT_LOG(LOG_VERBOSE, punit, "is under human orders, aborting AI control.");
    punit->ai.ai_role = AIUNIT_NONE;
    punit->ai.done = TRUE;
    return;
  }

  /* retire useless barbarian units here, before calling the management
     function */
  if( is_barbarian(pplayer) ) {
    /* Todo: should be configurable */
    if (unit_can_be_retired(punit) && myrand(100) > 90) {
      wipe_unit(punit);
      return;
    }
  }

  /* Check if we have lost our bodyguard. If we never had one, all
   * fine. If we had one and lost it, ask for a new one. */
  if (!bodyguard && aiguard_has_guard(punit)) {
    UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "lost bodyguard, asking for new");
    aiguard_request_guard(punit);
  }  

  if (punit->moves_left <= 0) {
    /* Can do nothing */
    punit->ai.done = TRUE;
    return;
  }

  if (get_transporter_capacity(punit) > 0) {
    unit_class_iterate(pclass) {
      /* FIXME: BOTH_MOVING units need ferry only if they use fuel */
      if (can_unit_type_transport(unit_type(punit), pclass)
          && (pclass->move_type == LAND_MOVING
              || (pclass->move_type == BOTH_MOVING
                  && !uclass_has_flag(pclass, UCF_MISSILE)))) {
        is_ferry = TRUE;
        break;
      }
    } unit_class_iterate_end;
  }

  if ((unit_has_type_flag(punit, F_DIPLOMAT))
      || (unit_has_type_flag(punit, F_SPY))) {
    TIMING_LOG(AIT_DIPLOMAT, TIMER_START);
    ai_manage_diplomat(pplayer, punit);
    TIMING_LOG(AIT_DIPLOMAT, TIMER_STOP);
    return;
  } else if (unit_has_type_flag(punit, F_SETTLERS)
	     ||unit_has_type_flag(punit, F_CITIES)) {
    ai_manage_settler(pplayer, punit);
    return;
  } else if (unit_has_type_flag(punit, F_TRADE_ROUTE)
             || unit_has_type_flag(punit, F_HELP_WONDER)) {
    TIMING_LOG(AIT_CARAVAN, TIMER_START);
    ai_manage_caravan(pplayer, punit);
    TIMING_LOG(AIT_CARAVAN, TIMER_STOP);
    return;
  } else if (unit_has_type_role(punit, L_BARBARIAN_LEADER)) {
    ai_manage_barbarian_leader(pplayer, punit);
    return;
  } else if (unit_has_type_flag(punit, F_PARATROOPERS)) {
    ai_manage_paratrooper(pplayer, punit);
    return;
  } else if (is_ferry && punit->ai.ai_role != AIUNIT_HUNTER) {
    TIMING_LOG(AIT_FERRY, TIMER_START);
    ai_manage_ferryboat(pplayer, punit);
    TIMING_LOG(AIT_FERRY, TIMER_STOP);
    return;
  } else if (utype_fuel(unit_type(punit))
             && punit->ai.ai_role != AIUNIT_ESCORT) {
    TIMING_LOG(AIT_AIRUNIT, TIMER_START);
    ai_manage_airunit(pplayer, punit);
    TIMING_LOG(AIT_AIRUNIT, TIMER_STOP);
    return;
  } else if (is_losing_hp(punit)) {
    /* This unit is losing hitpoints over time */

    /* TODO: We can try using air-unit code for helicopters, just
     * pretend they have fuel = HP / 3 or something. */
    punit->ai.done = TRUE; /* we did our best, which was ... nothing */
    return;
  } else if (is_military_unit(punit)) {
    TIMING_LOG(AIT_MILITARY, TIMER_START);
    UNIT_LOG(LOG_DEBUG, punit, "recruit unit for the military");
    ai_manage_military(pplayer,punit); 
    TIMING_LOG(AIT_MILITARY, TIMER_STOP);
    return;
  } else {
    /* what else could this be? -- Syela */
    switch (ai_manage_explorer(punit)) {
    case MR_DEATH:
      /* don't use punit! */
      break;
    case MR_OK:
      UNIT_LOG(LOG_DEBUG, punit, "now exploring");
      break;
    default:
      UNIT_LOG(LOG_DEBUG, punit, "fell through all unit tasks, defending");
      ai_unit_new_role(punit, AIUNIT_DEFEND_HOME, NULL);
      ai_military_defend(pplayer, punit);
      break;
    };
    return;
  }
}

/**************************************************************************
  Master city defense function.  We try to pick up the best available
  defenders, and not disrupt existing roles.

  TODO: Make homecity, respect homecity.
**************************************************************************/
static void ai_set_defenders(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    /* The idea here is that we should never keep more than two
     * units in permanent defense. */
    int total_defense = 0;
    int total_attack = pcity->ai->danger;
    bool emergency = FALSE;
    int count = 0;

    while (total_defense < total_attack) {
      int best_want = 0;
      struct unit *best = NULL;

      unit_list_iterate(pcity->tile->units, punit) {
       if ((punit->ai.ai_role == AIUNIT_NONE || emergency)
           && punit->ai.ai_role != AIUNIT_DEFEND_HOME
           && unit_owner(punit) == pplayer) {
          int want = assess_defense_unit(pcity, punit, FALSE);

          if (want > best_want) {
            best_want = want;
            best = punit;
          }
        }
      } unit_list_iterate_end;
      if (best == NULL) {
        /* Ooops - try to grab any unit as defender! */
        if (emergency) {
          CITY_LOG(LOG_DEBUG, pcity, "Not defended properly");
          break;
        }
        emergency = TRUE;
      } else {
        int loglevel = pcity->debug ? LOG_TEST : LOG_DEBUG;

        total_defense += best_want;
        UNIT_LOG(loglevel, best, "Defending city");
        ai_unit_new_role(best, AIUNIT_DEFEND_HOME, pcity->tile);
        count++;
      }
    }
    CITY_LOG(LOG_DEBUG, pcity, "Evaluating defense: %d defense, %d incoming"
             ", %d defenders (out of %d)", total_defense, total_attack, count,
             unit_list_size(pcity->tile->units));
  } city_list_iterate_end;
}

/**************************************************************************
  Master manage unit function.

  A manage function should set the unit to 'done' when it should no
  longer be touched by this code, and its role should be reset to IDLE
  when its role has accomplished its mission or the manage function
  fails to have or no longer has any use for the unit.
**************************************************************************/
void ai_manage_units(struct player *pplayer) 
{
  TIMING_LOG(AIT_AIRLIFT, TIMER_START);
  ai_airlift(pplayer);
  TIMING_LOG(AIT_AIRLIFT, TIMER_STOP);

  /* Clear previous orders, if desirable, here. */
  unit_list_iterate(pplayer->units, punit) {
    punit->ai.done = FALSE;
    if (punit->ai.ai_role == AIUNIT_DEFEND_HOME) {
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    }
  } unit_list_iterate_end;

  /* Find and set city defenders first - figure out which units are
   * allowed to leave home. */
  ai_set_defenders(pplayer);

  unit_list_iterate_safe(pplayer->units, punit) {
    if (punit->transported_by <= 0 && !punit->ai.done) {
      /* Though it is usually the passenger who drives the transport,
       * the transporter is responsible for managing its passengers. */
      ai_manage_unit(pplayer, punit);
    }
  } unit_list_iterate_safe_end;
}

/**************************************************************************
 Whether unit_type test is on the "upgrade path" of unit_type base,
 even if we can't upgrade now.
**************************************************************************/
bool is_on_unit_upgrade_path(const struct unit_type *test,
			     const struct unit_type *base)
{
  /* This is the real function: */
  do {
    base = base->obsoleted_by;
    if (base == test) {
      return TRUE;
    }
  } while (base);
  return FALSE;
}

/*************************************************************************
Barbarian leader tries to stack with other barbarian units, and if it's
not possible it runs away. When on coast, it may disappear with 33% chance.
**************************************************************************/
static void ai_manage_barbarian_leader(struct player *pplayer,
				       struct unit *leader)
{
  Continent_id con = tile_continent(leader->tile);
  int safest = 0;
  struct tile *safest_tile = leader->tile;
  struct unit *closest_unit = NULL;
  int dist, mindist = 10000;

  CHECK_UNIT(leader);

  if (leader->moves_left == 0
      || (can_unit_survive_at_tile(leader, leader->tile)
          && unit_list_size(leader->tile->units) > 1) ) {
      unit_activity_handling(leader, ACTIVITY_SENTRY);
      return;
  }

  if (is_boss_of_boat(leader)) {
    /* We are in charge. Of course, since we are the leader...
     * But maybe somebody more militaristic should lead our ship to battle! */
  
    /* First release boat from leaders lead */
    aiferry_clear_boat(leader);
  
    unit_list_iterate(leader->tile->units, warrior) {
      if (!unit_has_type_role(warrior, L_BARBARIAN_LEADER)
          && get_transporter_capacity(warrior) == 0
          && warrior->moves_left > 0) {
        /* This seems like a good warrior to lead us in to conquest! */
        ai_manage_unit(pplayer, warrior);

        /* If we reached our destination, ferryboat already called
         * ai_manage_unit() for leader. So no need to continue here.
         * Leader might even be dead.
         * If this return is removed, surronding unit_list_iterate()
         * has to be replaced with unit_list_iterate_safe()*/
        return;
      }
    } unit_list_iterate_end;
  }

  /* If we are not in charge of the boat, continue as if we
   * were not in a boat - we may want to leave the ship now. */

  /* the following takes much CPU time and could be avoided */
  generate_warmap(tile_city(leader->tile), leader);

  /* duck under own units */
  unit_list_iterate(pplayer->units, aunit) {
    if (unit_has_type_role(aunit, L_BARBARIAN_LEADER)
	|| !is_ground_unit(aunit)
	|| tile_continent(aunit->tile) != con)
      continue;

    if (WARMAP_COST(aunit->tile) < mindist) {
      mindist = WARMAP_COST(aunit->tile);
      closest_unit = aunit;
    }
  } unit_list_iterate_end;

  if (closest_unit
      && !same_pos(closest_unit->tile, leader->tile)
      && (tile_continent(leader->tile)
          == tile_continent(closest_unit->tile))) {
    (void) ai_unit_goto(leader, closest_unit->tile);
    return; /* sticks better to own units with this -- jk */
  }

  UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader needs to flee");
  mindist = 1000000;
  closest_unit = NULL;

  players_iterate(other_player) {
    unit_list_iterate(other_player->units, aunit) {
      if (is_military_unit(aunit)
	  && is_ground_unit(aunit)
	  && tile_continent(aunit->tile) == con) {
	/* questionable assumption: aunit needs as many moves to reach us as we
	   need to reach it */
	dist = WARMAP_COST(aunit->tile) - unit_move_rate(aunit);
	if (dist < mindist) {
	  freelog(LOG_DEBUG, "Barbarian leader: closest enemy is %s(%d,%d) dist %d",
                  unit_rule_name(aunit),
                  aunit->tile->x,
		  aunit->tile->y,
		  dist);
	  mindist = dist;
	  closest_unit = aunit;
	}
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  /* Disappearance - 33% chance on coast, when older than barbarian life span */
  if (is_ocean_near_tile(leader->tile)
      && leader->birth_turn + BARBARIAN_MIN_LIFESPAN < game.info.turn) {
    if(myrand(3) == 0) {
      UNIT_LOG(LOG_DEBUG, leader, "barbarian leader disappearing...");
      wipe_unit(leader);
      return;
    }
  }

  if (!closest_unit) {
    unit_activity_handling(leader, ACTIVITY_IDLE);
    UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader: no enemy.");
    return;
  }

  generate_warmap(tile_city(closest_unit->tile), closest_unit);

  do {
    struct tile *last_tile;

    UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader: moves left: %d.",
             leader->moves_left);

    square_iterate(leader->tile, 1, near_tile) {
      if (WARMAP_COST(near_tile) > safest
	  && could_unit_move_to_tile(leader, near_tile) == 1) {
	safest = WARMAP_COST(near_tile);
	freelog(LOG_DEBUG,
		"Barbarian leader: safest is %d, %d, safeness %d",
		near_tile->x, near_tile->y, safest);
	safest_tile = near_tile;
      }
    } 
    square_iterate_end;

    UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader: fleeing to (%d,%d).", 
             safest_tile->x, safest_tile->y);
    if (same_pos(leader->tile, safest_tile)) {
      UNIT_LOG(LOG_DEBUG, leader, 
               "Barbarian leader: reached the safest position.");
      unit_activity_handling(leader, ACTIVITY_IDLE);
      return;
    }

    last_tile = leader->tile;
    (void) ai_unit_goto(leader, safest_tile);
    if (same_pos(leader->tile, last_tile)) {
      /* Deep inside the goto handling code, in 
	 server/unithand.c::handle_unite_move_request(), the server
	 may decide that a unit is better off not moving this turn,
	 because the unit doesn't have quite enough movement points
	 remaining.  Unfortunately for us, this favor that the server
	 code does may lead to an endless loop here in the barbarian
	 leader code:  the BL will try to flee to a new location, execute 
	 the goto, find that it's still at its present (unsafe) location,
	 and repeat.  To break this loop, we test for the condition
	 where the goto doesn't do anything, and break if this is
	 the case. */
      break;
    }
  } while (leader->moves_left > 0);
}

/*************************************************************************
  Updates the global array simple_ai_types.
**************************************************************************/
void update_simple_ai_types(void)
{
  int i = 0;

  unit_type_iterate(punittype) {
    if (A_NEVER != punittype->require_advance
	&& !utype_has_flag(punittype, F_CIVILIAN)
	&& !uclass_has_flag(utype_class(punittype), UCF_MISSILE)
	&& !utype_has_flag(punittype, F_NO_LAND_ATTACK)
        && !utype_fuel(punittype)
	&& punittype->transport_capacity < 8) {
      simple_ai_types[i] = punittype;
      i++;
    }
  } unit_type_iterate_end;

  simple_ai_types[i] = NULL;
}

/****************************************************************************
  Build cached values about unit classes for AI
****************************************************************************/
void unit_class_ai_init(void)
{
  bv_special special;
  bv_bases bases;

  BV_CLR_ALL(special); /* Can it move even without road */
  BV_CLR_ALL(bases);

  unit_class_iterate(pclass) {
    bool move_land_enabled  = FALSE; /* Can move at some land terrains */
    bool move_land_disabled = FALSE; /* Cannot move at some land terrains */
    bool move_sea_enabled   = FALSE; /* Can move at some ocean terrains */
    bool move_sea_disabled  = FALSE; /* Cannot move at some ocean terrains */

    terrain_type_iterate(pterrain) {
      if (is_native_to_class(pclass, pterrain, special, bases)) {
        /* Can move at terrain */
        if (is_ocean(pterrain)) {
          move_sea_enabled = TRUE;
        } else {
          move_land_enabled = TRUE;
        }
      } else {
        /* Cannot move at terrain */
        if (is_ocean(pterrain)) {
          move_sea_disabled = TRUE;
        } else {
          move_land_disabled = TRUE;
        }
      }
    } terrain_type_iterate_end;

    if (move_land_enabled && !move_land_disabled) {
      pclass->ai.land_move = MOVE_FULL;
    } else if (move_land_enabled && move_land_disabled) {
      pclass->ai.land_move = MOVE_PARTIAL;
    } else {
      assert(!move_land_enabled);
      pclass->ai.land_move = MOVE_NONE;
    }

    if (move_sea_enabled && !move_sea_disabled) {
      pclass->ai.sea_move = MOVE_FULL;
    } else if (move_sea_enabled && move_sea_disabled) {
      pclass->ai.sea_move = MOVE_PARTIAL;
    } else {
      assert(!move_sea_enabled);
      pclass->ai.sea_move = MOVE_NONE;
    }

  } unit_class_iterate_end;
}
