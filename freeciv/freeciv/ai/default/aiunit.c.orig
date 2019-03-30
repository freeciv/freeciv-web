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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <math.h>

/* utility */
#include "bitvector.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "timing.h"

/* common */
#include "city.h"
#include "combat.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "specialist.h"
#include "traderoutes.h"
#include "unit.h"
#include "unitlist.h"

/* common/aicore */
#include "caravan.h"
#include "pf_tools.h"

/* server */
#include "barbarian.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplomats.h"
#include "maphand.h"
#include "srv_log.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advbuilding.h"
#include "advgoto.h"
#include "advtools.h"
#include "autoexplorer.h"
#include "autosettlers.h"

/* ai */
#include "difficulty.h"
#include "handicaps.h"

/* ai/default */
#include "aiair.h"
#include "aicity.h"
#include "aidata.h"
#include "aidiplomat.h"
#include "aiferry.h"
#include "aiguard.h"
#include "aihand.h"
#include "aihunt.h"
#include "ailog.h"
#include "aiparatrooper.h"
#include "aiplayer.h"
#include "aitools.h"
#include "daimilitary.h"

#include "aiunit.h"


#define LOGLEVEL_RECOVERY LOG_DEBUG
#define LOG_CARAVAN       LOG_DEBUG
#define LOG_CARAVAN2      LOG_DEBUG
#define LOG_CARAVAN3      LOG_DEBUG

static bool dai_find_boat_for_unit(struct ai_type *ait, struct unit *punit);
static bool dai_caravan_can_trade_cities_diff_cont(struct player *pplayer, 
                                                   struct unit *punit);
static void dai_manage_caravan(struct ai_type *ait, struct player *pplayer,
                               struct unit *punit);
static void dai_manage_barbarian_leader(struct ai_type *ait,
                                        struct player *pplayer,
                                        struct unit *leader);

static void dai_military_findjob(struct ai_type *ait,
                                 struct player *pplayer,struct unit *punit);
static void dai_military_defend(struct ai_type *ait, struct player *pplayer,
                                struct unit *punit);
static void dai_military_attack(struct ai_type *ait, struct player *pplayer,
                                struct unit *punit);

static bool unit_role_defender(const struct unit_type *punittype);
static int unit_def_rating_squared(const struct unit *punit,
                                   const struct unit *pdef);

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

/**********************************************************************//**
  Returns the city with the most need of an airlift.

  To be considerd, a city must have an air field. All cities with an
  urgent need for units are serviced before cities in danger.

  Return value may be NULL, this means no servicable city found.

  parameter pplayer may not be NULL.
**************************************************************************/
static struct city *find_neediest_airlift_city(struct ai_type *ait,
                                               const struct player *pplayer)
{
  struct city *neediest_city = NULL;
  int most_danger = 0;
  int most_urgent = 0;

  city_list_iterate(pplayer->cities, pcity) {
    struct ai_city *city_data = def_ai_city_data(pcity, ait);

    if (pcity->airlift) {
      if (city_data->urgency > most_urgent) {
        most_urgent = city_data->urgency;
        neediest_city = pcity;
      } else if (0 == most_urgent /* urgency trumps danger */
                 && city_data->danger > most_danger) {
        most_danger = city_data->danger;
        neediest_city = pcity;
      }
    }
  } city_list_iterate_end;

  return neediest_city;
}

/**********************************************************************//**
  Move defenders around with airports. Since this expends all our
  movement, a valid question is - why don't we do this on turn end?
  That's because we want to avoid emergency actions to protect the city
  during the turn if that isn't necessary.
**************************************************************************/
static void dai_airlift(struct ai_type *ait, struct player *pplayer)
{
  struct city *most_needed;
  int comparison;
  struct unit *transported;

  do {
    most_needed = find_neediest_airlift_city(ait, pplayer);
    comparison = 0;
    transported = NULL;

    if (!most_needed) {
      return;
    }

    unit_list_iterate(pplayer->units, punit) {
      struct tile *ptile = (unit_tile(punit));
      struct city *pcity = tile_city(ptile);

      if (pcity) {
        struct ai_city *city_data = def_ai_city_data(pcity, ait);
        struct unit_ai *unit_data = def_ai_unit_data(punit, ait);
        struct unit_type *ptype = unit_type_get(punit);

        if (city_data->urgency == 0
            && city_data->danger - DEFENSE_POWER(ptype) < comparison
            && unit_can_airlift_to(punit, most_needed)
            && DEFENSE_POWER(ptype) > 2
            && (unit_data->task == AIUNIT_NONE
                || unit_data->task == AIUNIT_DEFEND_HOME)
            && IS_ATTACKER(ptype)) {
          comparison = city_data->danger;
          transported = punit;
        }
      }
    } unit_list_iterate_end;
    if (!transported) {
      return;
    }
    UNIT_LOG(LOG_DEBUG, transported, "airlifted to defend %s",
             city_name_get(most_needed));
    unit_do_action(pplayer, transported->id, most_needed->id,
                   0, "", ACTION_AIRLIFT);
  } while (TRUE);
}

/**********************************************************************//**
  This is a much simplified form of assess_defense (see daimilitary.c),
  but which doesn't use pcity->server.ai.wallvalue and just returns a boolean
  value.  This is for use with "foreign" cities, especially non-ai
  cities, where ai.wallvalue may be out of date or uninitialized --dwp
**************************************************************************/
static bool has_defense(struct city *pcity)
{
  struct tile *ptile = city_tile(pcity);

  unit_list_iterate(ptile->units, punit) {
    if (is_military_unit(punit) && base_get_defense_power(punit) != 0
        && punit->hp != 0) {
      struct unit_class *pclass = unit_class_get(punit);

      if (pclass->non_native_def_pct > 0
          || is_native_tile_to_class(pclass, ptile)) {
        return TRUE;
      }
    }
  }
  unit_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  In the words of Syela: "Using funky fprime variable instead of f in
  the denom, so that def=1 units are penalized correctly."

  Translation (GB): build_cost_balanced is used in the denominator of
  the want equation (see, e.g.  find_something_to_kill) instead of
  just build_cost to make AI build more balanced units (with def > 1).
**************************************************************************/
int build_cost_balanced(const struct unit_type *punittype)
{
  return 2 * utype_build_shield_cost_base(punittype) * punittype->attack_strength /
      (punittype->attack_strength + punittype->defense_strength);
}

/**********************************************************************//**
  Attack rating of this particular unit right now.
**************************************************************************/
static int unit_att_rating_now(const struct unit *punit)
{
  return adv_unittype_att_rating(unit_type_get(punit), punit->veteran,
                                 punit->moves_left, punit->hp);
}

/**********************************************************************//**
  Square of the adv_unit_att_rating() function - used in actual computations.
**************************************************************************/
static int unit_att_rating_squared(const struct unit *punit)
{
  int v = adv_unit_att_rating(punit);

  return v * v;
}

/**********************************************************************//**
  Defence rating of this particular unit against this attacker.
**************************************************************************/
static int unit_def_rating(const struct unit *attacker,
                           const struct unit *defender)
{
  struct unit_type *def_type = unit_type_get(defender);
  
  return (get_total_defense_power(attacker, defender)
          * (attacker->id != 0 ? defender->hp : def_type->hp)
          * def_type->firepower / POWER_DIVIDER);
}

/**********************************************************************//**
  Square of the previous function - used in actual computations.
**************************************************************************/
static int unit_def_rating_squared(const struct unit *attacker,
                                   const struct unit *defender)
{
  int v = unit_def_rating(attacker, defender);

  return v * v;
}

/**********************************************************************//**
  Defence rating of def_type unit against att_type unit, squared.
  See get_virtual_defense_power for the arguments att_type, def_type,
  x, y, fortified and veteran.
**************************************************************************/
int unittype_def_rating_squared(const struct unit_type *att_type,
                                const struct unit_type *def_type,
                                const struct player *def_player,
                                struct tile *ptile, bool fortified,
                                int veteran)
{
  int v = get_virtual_defense_power(att_type, def_type, def_player, ptile,
                                    fortified, veteran)
    * def_type->hp * def_type->firepower / POWER_DIVIDER;

  return v * v;
}

/**********************************************************************//**
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

/**********************************************************************//**
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

/**********************************************************************//**
  Calculates the value and cost of nearby allied units to see if we can
  expect any help in our attack. Base function.
**************************************************************************/
static void reinforcements_cost_and_value(struct unit *punit,
                                          struct tile *ptile0,
                                          int *value, int *cost)
{
  *cost = 0;
  *value = 0;
  square_iterate(&(wld.map), ptile0, 1, ptile) {
    unit_list_iterate(ptile->units, aunit) {
      if (aunit != punit
	  && pplayers_allied(unit_owner(punit), unit_owner(aunit))) {
        int val = adv_unit_att_rating(aunit);

        if (val != 0) {
          *value += val;
          *cost += unit_build_shield_cost_base(aunit);
        }
      }
    } unit_list_iterate_end;
  } square_iterate_end;
}

/**********************************************************************//**
  Is there another unit which really should be doing this attack? Checks
  all adjacent tiles and the tile we stand on for such units.
**************************************************************************/
static bool is_my_turn(struct unit *punit, struct unit *pdef)
{
  int val = unit_att_rating_now(punit), cur, d;

  CHECK_UNIT(punit);

  square_iterate(&(wld.map), unit_tile(pdef), 1, ptile) {
    unit_list_iterate(ptile->units, aunit) {
      if (aunit == punit || unit_owner(aunit) != unit_owner(punit)) {
        continue;
      }
      if (unit_attack_units_at_tile_result(aunit, unit_tile(pdef)) != ATT_OK
          || unit_attack_unit_at_tile_result(aunit, pdef, unit_tile(pdef)) != ATT_OK) {
        continue;
      }
      d = get_virtual_defense_power(unit_type_get(aunit), unit_type_get(pdef),
                                    unit_owner(pdef), unit_tile(pdef),
                                    FALSE, 0);
      if (d == 0) {
        return TRUE;            /* Thanks, Markus -- Syela */
      }
      cur = unit_att_rating_now(aunit) *
          get_virtual_defense_power(unit_type_get(punit), unit_type_get(pdef),
                                    unit_owner(pdef), unit_tile(pdef),
                                    FALSE, 0) / d;
      if (cur > val && ai_fuzzy(unit_owner(punit), TRUE)) {
        return FALSE;
      }
    }
    unit_list_iterate_end;
  }
  square_iterate_end;
  return TRUE;
}

/**********************************************************************//**
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
static int dai_rampage_want(struct unit *punit, struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);
  struct unit *pdef;

  CHECK_UNIT(punit);

  if (can_unit_attack_tile(punit, ptile)
      && (pdef = get_defender(punit, ptile))) {
    /* See description of kill_desire() about these variables. */
    int attack = unit_att_rating_now(punit);
    int benefit = stack_cost(punit, pdef);
    int loss = unit_build_shield_cost_base(punit);

    attack *= attack;

    /* If the victim is in the city/fortress, we correct the benefit
     * with our health because there could be reprisal attacks.  We
     * shouldn't send already injured units to useless suicide.
     * Note that we do not specially encourage attacks against
     * cities: rampage is a hit-n-run operation. */
    if (!is_stack_vulnerable(ptile) 
        && unit_list_size(ptile->units) > 1) {
      benefit = (benefit * punit->hp) / unit_type_get(punit)->hp;
    }

    /* If we have non-zero attack rating... */
    if (attack > 0 && is_my_turn(punit, pdef)) {
      double chance = unit_win_chance(punit, pdef);
      int desire = avg_benefit(benefit, loss, chance);

      /* No need to amortize, our operation takes one turn. */
      UNIT_LOG(LOG_DEBUG, punit, "Rampage: Desire %d to kill %s(%d,%d)",
               desire,
               unit_rule_name(pdef),
               TILE_XY(unit_tile(pdef)));

      return MAX(0, desire);
    }
  } else if (0 == unit_list_size(ptile->units)) {
    /* No defender. */
    struct city *pcity = tile_city(ptile);

    /* ...and free foreign city waiting for us. Who would resist! */
    if (NULL != pcity
        && pplayers_at_war(pplayer, city_owner(pcity))
        && unit_can_take_over(punit)) {
      return -RAMPAGE_FREE_CITY_OR_BETTER;
    }

    /* ...or tiny pleasant hut here! */
    if (tile_has_cause_extra(ptile, EC_HUT) && !is_barbarian(pplayer)
        && is_native_tile(unit_type_get(punit), ptile)
        && unit_class_get(punit)->hut_behavior == HUT_NORMAL) {
      return -RAMPAGE_HUT_OR_BETTER;
    }
  }

  return 0;
}

/**********************************************************************//**
  Look for worthy targets within a one-turn horizon.
**************************************************************************/
static struct pf_path *find_rampage_target(struct unit *punit,
                                           int thresh_adj, int thresh_move)
{
  struct pf_map *tgt_map;
  struct pf_path *path = NULL;
  struct pf_parameter parameter;
  /* Coordinates of the best target (initialize to silence compiler) */
  struct tile *ptile = unit_tile(punit);
  /* Want of the best target */
  int max_want = 0;
  struct player *pplayer = unit_owner(punit);
 
  pft_fill_unit_attack_param(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  /* When trying to find rampage targets we ignore risks such as
   * enemy units because we are looking for trouble!
   * Hence no call ai_avoid_risks()
   */

  tgt_map = pf_map_new(&parameter);
  pf_map_move_costs_iterate(tgt_map, iter_tile, move_cost, FALSE) {
    int want;
    bool move_needed;
    int thresh;
 
    if (move_cost > punit->moves_left) {
      /* This is too far */
      break;
    }

    if (has_handicap(pplayer, H_TARGETS) 
        && !map_is_known_and_seen(iter_tile, pplayer, V_MAIN)) {
      /* The target is under fog of war */
      continue;
    }
    
    want = dai_rampage_want(punit, iter_tile);

    /* Negative want means move needed even though the tiles are adjacent */
    move_needed = (!is_tiles_adjacent(unit_tile(punit), iter_tile)
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
  } pf_map_move_costs_iterate_end;

  if (max_want > 0) {
    /* We found something */
    path = pf_map_path(tgt_map, ptile);
    fc_assert(path != NULL);
  }

  pf_map_destroy(tgt_map);
  
  return path;
}

/**********************************************************************//**
  Find and kill anything reachable within this turn and worth more than
  the relevant of the given thresholds until we have run out of juicy
  targets or movement.  The first threshold is for attacking which will
  leave us where we stand (attacking adjacent units), the second is for
  attacking distant (but within reach) targets.

  For example, if unit is a bodyguard on duty, it should call
    dai_military_rampage(punit, 100, RAMPAGE_FREE_CITY_OR_BETTER)
  meaning "we will move _only_ to pick up a free city but we are happy to
  attack adjacent squares as long as they are worthy of it".

  Returns TRUE if survived the rampage session.
**************************************************************************/
bool dai_military_rampage(struct unit *punit, int thresh_adj, 
                          int thresh_move)
{
  int count = punit->moves_left + 1; /* break any infinite loops */
  struct pf_path *path = NULL;

  TIMING_LOG(AIT_RAMPAGE, TIMER_START);  
  CHECK_UNIT(punit);

  fc_assert_ret_val(thresh_adj <= thresh_move, TRUE);
  /* This teaches the AI about the dangers inherent in occupychance. */
  thresh_adj += ((thresh_move - thresh_adj) * game.server.occupychance / 100);

  while (count-- > 0 && punit->moves_left > 0
         && (path = find_rampage_target(punit, thresh_adj, thresh_move))) {
    if (!adv_unit_execute_path(punit, path)) {
      /* Died */
      count = -1;
    }
    pf_path_destroy(path);
    path = NULL;
  }

  fc_assert(NULL == path);

  TIMING_LOG(AIT_RAMPAGE, TIMER_STOP);
  return (count >= 0);
}

/**********************************************************************//**
  If we are not covering our charge's ass, go do it now. Also check if we
  can kick some ass, which is always nice.
**************************************************************************/
static void dai_military_bodyguard(struct ai_type *ait, struct player *pplayer,
                                   struct unit *punit)
{
  struct unit *aunit = aiguard_charge_unit(ait, punit);
  struct city *acity = aiguard_charge_city(ait, punit);
  struct tile *ptile;

  CHECK_UNIT(punit);
  CHECK_GUARD(ait, punit);

  if (aunit && unit_owner(aunit) == unit_owner(punit)) {
    /* protect a unit */
    if (aunit->goto_tile != NULL) {
      /* Our charge is going somewhere: maybe we should meet them there */
      /* FIXME: This probably isn't the best algorithm for this. */
      int me2them = real_map_distance(unit_tile(punit), unit_tile(aunit));
      int me2goal = real_map_distance(unit_tile(punit), aunit->goto_tile);
      int them2goal = real_map_distance(unit_tile(aunit), aunit->goto_tile);

      if (me2goal < me2them
          || (me2goal/unit_move_rate(punit) < them2goal/unit_move_rate(aunit)
              && me2goal/unit_move_rate(punit) < me2them/unit_move_rate(punit)
              && unit_move_rate(punit) > unit_move_rate(aunit))) {
        ptile = aunit->goto_tile;
      } else {
        ptile = unit_tile(aunit);
      }
    } else {
      ptile = unit_tile(aunit);
    }
  } else if (acity && city_owner(acity) == unit_owner(punit)) {
    /* protect a city */
    ptile = acity->tile;
  } else {
    /* should be impossible */
    BODYGUARD_LOG(ait, LOG_DEBUG, punit, "we lost our charge");
    dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    return;
  }

  if (same_pos(unit_tile(punit), ptile)) {
    BODYGUARD_LOG(ait, LOG_DEBUG, punit, "at RV");
  } else {
    if (goto_is_sane(punit, ptile)) {
      BODYGUARD_LOG(ait, LOG_DEBUG, punit, "meeting charge");
      if (!dai_gothere(ait, pplayer, punit, ptile)) {
        /* We died */
        return;
      }
    } else {
      BODYGUARD_LOG(ait, LOG_DEBUG, punit, "can not meet charge");
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    }
  }
  /* We might have stopped because of an enemy nearby.
   * Perhaps we can kill it.*/
  if (dai_military_rampage(punit, BODYGUARD_RAMPAGE_THRESHOLD,
                           RAMPAGE_FREE_CITY_OR_BETTER)
      && same_pos(unit_tile(punit), ptile)) {
    def_ai_unit_data(punit, ait)->done = TRUE; /* Stay with charge */
  }
}

/**********************************************************************//**
  Does the unit with the id given have the flag L_DEFEND_GOOD?
**************************************************************************/
static bool unit_role_defender(const struct unit_type *punittype)
{
  return (utype_has_role(punittype, L_DEFEND_GOOD));
}

/**********************************************************************//**
  See if we can find something to defend. Called both by wannabe bodyguards
  and building want estimation code. Returns desirability for using this
  unit as a bodyguard or for defending a city.

  We do not consider units with higher movement than us, or units that are
  native to terrains or extras not native to us, as potential charges. Nor
  do we attempt to bodyguard units with higher defence than us, or military
  units with lower attack than us that are not transports.
**************************************************************************/
int look_for_charge(struct ai_type *ait, struct player *pplayer,
                    struct unit *punit,
                    struct unit **aunit, struct city **acity)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct city *pcity;
  struct ai_city *data, *best_data = NULL;
  const int toughness = adv_unit_def_rating_basic_squared(punit);
  int def, best_def = -1;
  /* Arbitrary: 3 turns. */
  const int max_move_cost = 3 * unit_move_rate(punit);

  *aunit = NULL;
  *acity = NULL;

  if (0 == toughness) {
    /* useless */
    return 0;
  }

  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  pfm = pf_map_new(&parameter);

  pf_map_move_costs_iterate(pfm, ptile, move_cost, TRUE) {
    if (move_cost > max_move_cost) {
      /* Consider too far. */
      break;
    }

    pcity = tile_city(ptile);

    /* Consider unit bodyguard. */
    unit_list_iterate(ptile->units, buddy) {
      struct unit_type *ptype = unit_type_get(punit);
      struct unit_type *buddy_type = unit_type_get(buddy);

      /* TODO: allied unit bodyguard? */
      if (!dai_can_unit_type_follow_unit_type(ptype, buddy_type, ait)
          || unit_owner(buddy) != pplayer
          || !aiguard_wanted(ait, buddy)
          || unit_move_rate(buddy) > unit_move_rate(punit)
          || DEFENSE_POWER(buddy_type) >= DEFENSE_POWER(ptype)
          || (is_military_unit(buddy)
              && 0 == get_transporter_capacity(buddy)
              && ATTACK_POWER(buddy_type) <= ATTACK_POWER(ptype))) {

        continue;
      }

      def = (toughness - adv_unit_def_rating_basic_squared(buddy));
      if (0 >= def) {
        continue;
      }

      if (0 == get_transporter_capacity(buddy)) {
        /* Reduce want based on move cost. We can't do this for
         * transports since they move around all the time, leading
         * to hillarious flip-flops. */
        def >>= move_cost / (2 * unit_move_rate(punit));
      }
      if (def > best_def) {
        *aunit = buddy;
        *acity = NULL;
        best_def = def;
      }
    } unit_list_iterate_end;

    /* City bodyguard. TODO: allied city bodyguard? */
    if (ai_fuzzy(pplayer, TRUE)
        && NULL != pcity
        && city_owner(pcity) == pplayer
        && (data = def_ai_city_data(pcity, ait))
        && 0 < data->urgency) {
      if (NULL != best_data
          && (0 < best_data->grave_danger
              || best_data->urgency > data->urgency
              || ((best_data->danger > data->danger
                   || AIUNIT_DEFEND_HOME == def_ai_unit_data(punit, ait)->task)
                  && 0 == data->grave_danger))) {
        /* Chances are we'll be between cities when we are needed the most!
         * Resuming pf_map_move_costs_iterate()... */
        continue;
      }
      def = (data->danger - assess_defense_quadratic(ait, pcity));
      if (def <= 0) {
        continue;
      }
      /* Reduce want based on move cost. */
      def >>= move_cost / (2 * unit_move_rate(punit));
      if (def > best_def && ai_fuzzy(pplayer, TRUE)) {
       *acity = pcity;
       *aunit = NULL;
       best_def = def;
       best_data = data;
     }
    }
  } pf_map_move_costs_iterate_end;

  pf_map_destroy(pfm);

  UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "%s(), best_def=%d, type=%s (%d, %d)",
           __FUNCTION__, best_def * 100 / toughness,
           (NULL != *acity ? city_name_get(*acity)
            : (NULL != *aunit ? unit_rule_name(*aunit) : "")),
           (NULL != *acity ? index_to_map_pos_x(tile_index(city_tile(*acity)))
            : (NULL != *aunit ?
               index_to_map_pos_x(tile_index(unit_tile(*aunit))) : -1)),
           (NULL != *acity ? index_to_map_pos_y(tile_index(city_tile(*acity)))
            : (NULL != *aunit ?
               index_to_map_pos_y(tile_index(unit_tile(*aunit))) : -1)));

  return ((best_def * 100) / toughness);
}

/**********************************************************************//**
  See if the follower can follow the followee
**************************************************************************/
bool dai_can_unit_type_follow_unit_type(struct unit_type *follower,
                                        struct unit_type *followee,
                                        struct ai_type *ait)
{
  struct unit_type_ai *utai = utype_ai_data(follower, ait);

  unit_type_list_iterate(utai->potential_charges, pcharge) {
    if (pcharge == followee) {
      return TRUE;
    }
  } unit_type_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  See if we have a specific job for the unit.
**************************************************************************/
static void dai_military_findjob(struct ai_type *ait,
                                 struct player *pplayer, struct unit *punit)
{
  struct unit_type *punittype = unit_type_get(punit);
  struct unit_ai *unit_data;

  CHECK_UNIT(punit);

  /* keep barbarians aggresive and primitive */
  if (is_barbarian(pplayer)) {
    if (can_unit_do_activity(punit, ACTIVITY_PILLAGE)
	&& is_land_barbarian(pplayer)) {
      /* land barbarians pillage */
      unit_activity_handling(punit, ACTIVITY_PILLAGE);
    }
    dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    return;
  }

  unit_data = def_ai_unit_data(punit, ait);

  /* If I am a bodyguard, check whether I can do my job. */
  if (unit_data->task == AIUNIT_ESCORT
      || unit_data->task == AIUNIT_DEFEND_HOME) {
    aiguard_update_charge(ait, punit);
  }
  if (aiguard_has_charge(ait, punit)
      && unit_data->task == AIUNIT_ESCORT) {
    struct unit *aunit = aiguard_charge_unit(ait, punit);
    struct city *acity = aiguard_charge_city(ait, punit);
    struct ai_city *city_data = NULL;

    if (acity != NULL) {
      city_data = def_ai_city_data(acity, ait);
    }

    /* Check if the city we are on our way to rescue is still in danger,
     * or the unit we should protect is still alive... */
    if ((aunit && (aiguard_has_guard(ait, aunit) || aiguard_wanted(ait, aunit))
         && adv_unit_def_rating_basic(punit) > adv_unit_def_rating_basic(aunit)) 
        || (acity && city_owner(acity) == unit_owner(punit)
            && city_data->urgency != 0 
            && city_data->danger > assess_defense_quadratic(ait, acity))) {
      return; /* Yep! */
    } else {
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL); /* Nope! */
    }
  }

  /* Is the unit badly damaged? */
  if ((unit_data->task == AIUNIT_RECOVER
       && punit->hp < punittype->hp)
      || punit->hp < punittype->hp * 0.25) { /* WAG */
    UNIT_LOG(LOGLEVEL_RECOVERY, punit, "set to hp recovery");
    dai_unit_new_task(ait, punit, AIUNIT_RECOVER, NULL);
    return;
  }

  TIMING_LOG(AIT_BODYGUARD, TIMER_START);
  if (unit_role_defender(unit_type_get(punit))) {
    /* This is a defending unit that doesn't need to stay put.
     * It needs to defend something, but not necessarily where it's at.
     * Therefore, it will consider becoming a bodyguard. -- Syela */
    struct city *acity;
    struct unit *aunit;

    look_for_charge(ait, pplayer, punit, &aunit, &acity);
    if (acity) {
      dai_unit_new_task(ait, punit, AIUNIT_ESCORT, acity->tile);
      aiguard_assign_guard_city(ait, acity, punit);
      BODYGUARD_LOG(ait, LOG_DEBUG, punit, "going to defend city");
    } else if (aunit) {
      dai_unit_new_task(ait, punit, AIUNIT_ESCORT, unit_tile(aunit));
      aiguard_assign_guard_unit(ait, aunit, punit);
      BODYGUARD_LOG(ait, LOG_DEBUG, punit, "going to defend unit");
    }
  }
  TIMING_LOG(AIT_BODYGUARD, TIMER_STOP);
}

/**********************************************************************//**
  Send a unit to the city it should defend. If we already have a city
  it should defend, use the punit->server.ai->charge field to denote this.
  Otherwise, it will stay put in the city it is in, or find a city
  to reside in, or travel all the way home.

  TODO: Add make homecity.
  TODO: Add better selection of city to defend.
**************************************************************************/
static void dai_military_defend(struct ai_type *ait, struct player *pplayer,
                                struct unit *punit)
{
  struct city *pcity = aiguard_charge_city(ait, punit);

  CHECK_UNIT(punit);

  if (!pcity || city_owner(pcity) != pplayer) {
    pcity = tile_city(unit_tile(punit));
    /* Do not stay defending an allied city forever */
    aiguard_clear_charge(ait, punit);
  }

  if (!pcity) {
    /* Try to find a place to rest. Sitting duck out in the wilderness
     * is generally a bad idea, since we protect no cities that way, and
     * it looks silly. */
    pcity = find_closest_city(unit_tile(punit), NULL, pplayer,
                              FALSE, FALSE, FALSE, TRUE, FALSE,
                              unit_class_get(punit));
  }

  if (!pcity) {
    pcity = game_city_by_number(punit->homecity);
  }

  if (dai_military_rampage(punit, BODYGUARD_RAMPAGE_THRESHOLD * 5,
                           RAMPAGE_FREE_CITY_OR_BETTER)) {
    /* ... we survived */
    if (pcity) {
      UNIT_LOG(LOG_DEBUG, punit, "go to defend %s", city_name_get(pcity));
      if (same_pos(unit_tile(punit), pcity->tile)) {
        UNIT_LOG(LOG_DEBUG, punit, "go defend successful");
        def_ai_unit_data(punit, ait)->done = TRUE;
      } else {
        (void) dai_gothere(ait, pplayer, punit, pcity->tile);
      }
    } else {
      UNIT_LOG(LOG_VERBOSE, punit, "defending nothing...?");
    }
  }
}

/**********************************************************************//**
  Mark invasion possibilities of punit in the surrounding cities. The
  given radius limites the area which is searched for cities. The
  center of the area is either the unit itself (dest == FALSE) or the
  destination of the current goto (dest == TRUE). The invasion threat
  is marked in pcity->server.ai.invasion by setting the "which" bit (to
  tell attack which can only kill units from occupy possibility).

  If dest == TRUE then a valid goto is presumed.
**************************************************************************/
static void invasion_funct(struct ai_type *ait, struct unit *punit,
                           bool dest, int radius, int which)
{
  struct tile *ptile;
  struct player *pplayer = unit_owner(punit);

  CHECK_UNIT(punit);

  if (dest) {
    ptile = punit->goto_tile;
  } else {
    ptile = unit_tile(punit);
  }

  square_iterate(&(wld.map), ptile, radius, tile1) {
    struct city *pcity = tile_city(tile1);

    if (pcity
        && POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, city_owner(pcity))
	&& (dest || !has_defense(pcity))) {
      int attacks;
      struct ai_city *city_data = def_ai_city_data(pcity, ait);

      if (unit_has_type_flag(punit, UTYF_ONEATTACK)) {
        attacks = 1;
      } else {
        attacks = unit_type_get(punit)->move_rate;
      }
      city_data->invasion.attack += attacks;
      if (which == INVASION_OCCUPY) {
        city_data->invasion.occupy++;
      }
    }
  } square_iterate_end;
}

/**********************************************************************//**
  Returns TRUE if a beachhead as been found to reach 'dest_tile'.
**************************************************************************/
bool find_beachhead(const struct player *pplayer, struct pf_map *ferry_map,
                    struct tile *dest_tile,
                    const struct unit_type *cargo_type,
                    struct tile **ferry_dest, struct tile **beachhead_tile)
{
  if (NULL == tile_city(dest_tile)
      || can_attack_from_non_native(cargo_type)) {
    /* Unit can directly go to 'dest_tile'. */
    struct tile *best_tile = NULL;
    int best_cost = PF_IMPOSSIBLE_MC, cost;

    if (NULL != beachhead_tile) {
      *beachhead_tile = dest_tile;
    }

    adjc_iterate(&(wld.map), dest_tile, ptile) {
      cost = pf_map_move_cost(ferry_map, ptile);
      if (cost != PF_IMPOSSIBLE_MC
          && (NULL == best_tile || cost < best_cost)) {
        best_tile = ptile;
        best_cost = cost;
      }
    } adjc_iterate_end;

    if (NULL != ferry_dest) {
      *ferry_dest = best_tile;
    }

    return (PF_IMPOSSIBLE_MC != best_cost);
  } else {
    /* We need to find a beach around 'dest_tile'. */
    struct tile *best_tile = NULL, *best_beach = NULL;
    struct tile_list *checked_tiles = tile_list_new();
    int best_cost = PF_IMPOSSIBLE_MC, cost;

    tile_list_append(checked_tiles, dest_tile);
    adjc_iterate(&(wld.map), dest_tile, beach) {
      if (is_native_tile(cargo_type, beach)) {
        /* Can land there. */
        adjc_iterate(&(wld.map), beach, ptile) {
          if (!tile_list_search(checked_tiles, ptile)
              && !is_non_allied_unit_tile(ptile, pplayer)) {
            tile_list_append(checked_tiles, ptile);
            cost = pf_map_move_cost(ferry_map, ptile);
            if (cost != PF_IMPOSSIBLE_MC
                && (NULL == best_tile || cost < best_cost)) {
              best_beach = beach;
              best_tile = ptile;
              best_cost = cost;
            }
          }
        } adjc_iterate_end;
      }
    } adjc_iterate_end;

    tile_list_destroy(checked_tiles);

    if (NULL != beachhead_tile) {
      *beachhead_tile = best_beach;
    }
    if (NULL != ferry_dest) {
      *ferry_dest = best_tile;
    }
    return (PF_IMPOSSIBLE_MC != best_cost);
  }
}

/**********************************************************************//**
  Find something to kill! This function is called for units to find targets
  to destroy and for cities that want to know if they should build offensive
  units. Target location returned in 'dest_tile', want as function return
  value.

  punit->id == 0 means that the unit is virtual (considered to be built).
**************************************************************************/
int find_something_to_kill(struct ai_type *ait, struct player *pplayer,
                           struct unit *punit,
                           struct tile **pdest_tile, struct pf_path **ppath,
                           struct pf_map **pferrymap,
                           struct unit **pferryboat,
                           struct unit_type **pboattype, int *pmove_time)
{
  const int attack_value = adv_unit_att_rating(punit);    /* basic attack. */
  struct pf_parameter parameter;
  struct pf_map *punit_map, *ferry_map;
  struct pf_position pos;
  struct unit_class *punit_class = unit_class_get(punit);
  struct unit_type *punit_type = unit_type_get(punit);
  struct tile *punit_tile = unit_tile(punit);
  /* Type of our boat (a future one if ferryboat == NULL). */
  struct unit_type *boattype = NULL;
  struct unit *ferryboat = NULL;
  struct city *pcity;
  struct ai_city *acity_data;
  int bcost, bcost_bal; /* Build cost of the attacker (+adjustments). */
  bool handicap = has_handicap(pplayer, H_TARGETS);
  bool unhap = FALSE;   /* Do we make unhappy citizen. */
  bool harbor = FALSE;  /* Do we have access to sea? */
  bool go_by_boat;      /* Whether we need a boat or not. */
  int vulnerability;    /* Enemy defence rating. */
  int benefit;          /* Benefit from killing the target. */
  struct unit *pdefender;       /* Enemy city defender. */
  int move_time;        /* Turns needed to target. */
  int reserves;
  int max_move_cost = max_move_cost = 10 * unit_move_rate(punit);
  int attack;           /* Our total attack value with reinforcements. */
  int victim_count;     /* Number of enemies there. */
  int needferry;        /* Cost of building a ferry boat. */
  /* This is a kluge, because if we don't set x and y with !punit->id,
   * p_a_w isn't called, and we end up not wanting ironclads and therefore
   * never learning steam engine, even though ironclads would be very
   * useful. -- Syela */
  int bk = 0; 
  int want;             /* Want (amortized) of the operaton. */
  int best = 0;         /* Best of all wants. */
  struct tile *goto_dest_tile = NULL;

  /* Very preliminary checks. */
  *pdest_tile = punit_tile;
  if (NULL != pferrymap) {
    *pferrymap = NULL;
  }
  if (NULL != pferryboat) {
    *pferryboat = NULL;
  }
  if (NULL != pboattype) {
    *pboattype = NULL;
  }
  if (NULL != pmove_time) {
    *pmove_time = 0;
  }
  if (NULL != ppath) {
    *ppath = NULL;
  }

  if (0 == attack_value) {
    /* A very poor attacker... probably low on HP. */
    return 0;
  }

  TIMING_LOG(AIT_FSTK, TIMER_START);


  /*** Part 1: Calculate targets ***/

  /* This horrible piece of code attempts to calculate the attractiveness of
   * enemy cities as targets for our units, by checking how many units are
   * going towards it or are near it already. */

  /* Reset enemy cities data. */
  players_iterate(aplayer) {
    /* See comment below in next usage of POTENTIALLY_HOSTILE_PLAYER. */
    if ((punit->id == 0 && !POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, aplayer))
        || (punit->id != 0 && !pplayers_at_war(pplayer, aplayer))) {
      continue;
    }
    city_list_iterate(aplayer->cities, acity) {
      struct ai_city *city_data = def_ai_city_data(acity, ait);

      reinforcements_cost_and_value(punit, acity->tile,
                                    &city_data->attack,
                                    &city_data->bcost);
      city_data->invasion.attack = 0;
      city_data->invasion.occupy = 0;
    } city_list_iterate_end;
  } players_iterate_end;

  /* Second, calculate in units on their way there, and mark targets for
   * invasion */
  unit_list_iterate(pplayer->units, aunit) {
    struct unit_type *atype;

    if (aunit == punit) {
      continue;
    }

    atype = unit_type_get(aunit);

    /* dealing with invasion stuff */
    if (IS_ATTACKER(atype)) {
      if (aunit->activity == ACTIVITY_GOTO) {
        invasion_funct(ait, aunit, TRUE, 0,
                       (unit_can_take_over(aunit)
                        ? INVASION_OCCUPY : INVASION_ATTACK));
        if ((pcity = tile_city(aunit->goto_tile))) {
          struct ai_city *city_data = def_ai_city_data(pcity, ait);

          city_data->attack += adv_unit_att_rating(aunit);
          city_data->bcost += unit_build_shield_cost_base(aunit);
        } 
      }
      invasion_funct(ait, aunit, FALSE, unit_move_rate(aunit) / SINGLE_MOVE,
                     (unit_can_take_over(aunit)
                      ? INVASION_OCCUPY : INVASION_ATTACK));
    } else if (def_ai_unit_data(aunit, ait)->passenger != 0
               && !same_pos(unit_tile(aunit), unit_tile(punit))) {
      /* It's a transport with reinforcements */
      if (aunit->activity == ACTIVITY_GOTO) {
        invasion_funct(ait, aunit, TRUE, 1, INVASION_OCCUPY);
      }
      invasion_funct(ait, aunit, FALSE, 2, INVASION_OCCUPY);
    }
  } unit_list_iterate_end;
  /* end horrible initialization subroutine */


  /*** Part 2: Now pick one target ***/

  /* We first iterate through all cities, then all units, looking
   * for a viable target. We also try to gang up on the target
   * to avoid spreading out attacks too widely to be inefficient.
   */

  pcity = tile_city(punit_tile);
  if (NULL != pcity && (0 == punit->id || pcity->id == punit->homecity)) {
    /* I would have thought unhappiness should be taken into account
     * irrespectfully the city in which it will surface... -- GB */
    unhap = dai_assess_military_unhappiness(pcity);
  }

  bcost = unit_build_shield_cost_base(punit);
  bcost_bal = build_cost_balanced(punit_type);

  pft_fill_unit_attack_param(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  punit_map = pf_map_new(&parameter);

  if (MOVE_NONE == punit_class->adv.sea_move) {
    /* We need boat to move over sea. */
    ferryboat = unit_transport_get(punit);

    /* First check if we can use the boat we are currently loaded to. */
    if (ferryboat == NULL || !is_boat_free(ait, ferryboat, punit, 0)) {
      /* No, we cannot control current boat */
      ferryboat = NULL;
    }

    if (NULL == ferryboat) {
      /* Try to find new boat */
      ferryboat = player_unit_by_number(pplayer,
                                        aiferry_find_boat(ait, punit, 1, NULL));
    }

    if (0 == punit->id && is_terrain_class_near_tile(punit_tile, TC_OCEAN)) {
      harbor = TRUE;
    }
  }

  if (NULL != ferryboat) {
    boattype = unit_type_get(ferryboat);
    pft_fill_unit_overlap_param(&parameter, ferryboat);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    ferry_map = pf_map_new(&parameter);
  } else {
    boattype = best_role_unit_for_player(pplayer, L_FERRYBOAT);
    if (NULL == boattype) {
      /* We pretend that we can have the simplest boat to stimulate tech. */
      boattype = get_role_unit(L_FERRYBOAT, 0);
    }
    if (NULL != boattype && harbor) {
      /* Let's simulate a boat at 'punit' position. */
      pft_fill_utype_overlap_param(&parameter, boattype, punit_tile,
                                   pplayer);
      parameter.omniscience = !has_handicap(pplayer, H_MAP);
      ferry_map = pf_map_new(&parameter);
    } else {
      ferry_map = NULL;
    }
  }

  players_iterate(aplayer) {
    /* For the virtual unit case, which is when we are called to evaluate
     * which units to build, we want to calculate in danger and which
     * players we want to make war with in the future. We do _not_ want
     * to do this when actually making attacks. */
    if ((punit->id == 0 && !POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, aplayer))
        || (punit->id != 0 && !pplayers_at_war(pplayer, aplayer))) {
      continue; /* Not an enemy. */
    }

    city_list_iterate(aplayer->cities, acity) {
      struct tile *atile = city_tile(acity);

      if (!is_native_tile(punit_type, atile)
          && !can_attack_non_native(punit_type)) {
        /* Can't attack this city. It is on land. */
        continue;
      }

      if (handicap && !map_is_known(atile, pplayer)) {
        /* Can't see it */
        continue;
      }

      if (pf_map_position(punit_map, atile, &pos)) {
        go_by_boat = FALSE;
        move_time = pos.turn;
      } else if (NULL == ferry_map) {
        continue;       /* Impossible to handle. */
      } else {
        struct tile *dest, *beach;

        if (!find_beachhead(pplayer, ferry_map, atile, punit_type,
                            &dest, &beach)) {
          continue;   /* Impossible to go by boat. */
        }
        if (!pf_map_position(ferry_map, dest, &pos)) {
          fc_assert(pf_map_position(ferry_map, dest, &pos));
          continue;
        }
        move_time = pos.turn;   /* Sailing time. */
        if (dest != beach) {
          move_time++;          /* Land time. */
        }
        if (NULL != ferryboat && unit_tile(ferryboat) != punit_tile) {
          if (pf_map_position(punit_map, unit_tile(ferryboat), &pos)) {
            move_time += pos.turn;      /* Time to reach the boat. */
          } else {
            continue;   /* Cannot reach the boat. */
          }
        }
        go_by_boat = TRUE;
      }

      if (can_unit_attack_tile(punit, city_tile(acity))
          && (pdefender = get_defender(punit, city_tile(acity)))) {
        vulnerability = unit_def_rating_squared(punit, pdefender);
        benefit = unit_build_shield_cost_base(pdefender);
      } else {
        pdefender = NULL;
        vulnerability = 0;
        benefit = 0;
      }

      if (1 < move_time) {
        struct unit_type *def_type = dai_choose_defender_versus(acity, punit);

        if (def_type) {
          int v = unittype_def_rating_squared(punit_type, def_type, aplayer,
                                              atile, FALSE,
                                              do_make_unit_veteran(acity,
                                                                   def_type));
          if (v > vulnerability) {
            /* They can build a better defender! */
            vulnerability = v;
            benefit = utype_build_shield_cost(acity, def_type);
          }
        }
      }

      acity_data = def_ai_city_data(acity, ait);

      reserves = (acity_data->invasion.attack
                  - unit_list_size(acity->tile->units));

      if (0 < reserves && (unit_can_take_over(punit)
                           || 0 < acity_data->invasion.occupy)) {
        /* There are units able to occupy the city after all defenders
         * are killed! */
        benefit += acity_data->worth * reserves / 5;
      }

      attack = attack_value + acity_data->attack;
      attack *= attack;
      /* Avoiding handling upkeep aggregation this way -- Syela */

      /* AI was not sending enough reinforcements to totally wipe out a city
       * and conquer it in one turn.
       * This variable enables total carnage. -- Syela */
      victim_count = unit_list_size(atile->units) + 1;

      if (!unit_can_take_over(punit) && NULL == pdefender) {
        /* Nothing there to bash and we can't occupy!
         * Not having this check caused warships yoyoing */
        want = 0;
      } else if (10 < move_time) {
        /* Too far! */
        want = 0;
      } else if (unit_can_take_over(punit)
                 && 0 < acity_data->invasion.attack
                 && 0 == acity_data->invasion.occupy) {
        /* Units able to occupy really needed there! */
        want = bcost * SHIELD_WEIGHTING;
      } else {
        want = kill_desire(benefit, attack, bcost + acity_data->bcost,
                           vulnerability, victim_count);
      }
      want -= move_time * (unhap ? SHIELD_WEIGHTING + 2 * TRADE_WEIGHTING 
                           : SHIELD_WEIGHTING);
      /* Build_cost of ferry. */
      needferry = (go_by_boat && NULL == ferryboat
                   ? utype_build_shield_cost(acity, boattype) : 0);
      /* FIXME: add time to build the ferry? */
      want = military_amortize(pplayer, game_city_by_number(punit->homecity),
                               want, MAX(1, move_time),
                               bcost_bal + needferry);

      /* BEGIN STEAM-ENGINES-ARE-OUR-FRIENDS KLUGE. */
      if (0 >= want && 0 == punit->id && 0 == best) {
        int bk_e = military_amortize(pplayer,
                                     game_city_by_number(punit->homecity),
                                     benefit * SHIELD_WEIGHTING,
                                     MAX(1, move_time),
                                     bcost_bal + needferry);

        if (bk_e > bk) {
          *pdest_tile = atile;
          if (NULL != pferrymap) {
            *pferrymap = go_by_boat ? ferry_map : NULL;
          }
          if (NULL != pferryboat) {
            *pferryboat = go_by_boat ? ferryboat : NULL;
          }
          if (NULL != pboattype) {
            *pboattype = go_by_boat ? boattype : NULL;
          }
          if (NULL != pmove_time) {
            *pmove_time = move_time;
          }
          goto_dest_tile = (go_by_boat && NULL != ferryboat
                            ? unit_tile(ferryboat) : atile);
          bk = bk_e;
        }
      }
      /* END STEAM-ENGINES KLUGE */

      if (0 != punit->id
          && NULL != ferryboat
          && punit_class->adv.sea_move == MOVE_NONE) {
        UNIT_LOG(LOG_DEBUG, punit,
                 "%s(): with boat %s@(%d, %d) -> %s@(%d, %d)"
                 " (go_by_boat=%d, move_time=%d, want=%d, best=%d)",
                 __FUNCTION__, unit_rule_name(ferryboat),
                 TILE_XY(unit_tile(ferryboat)), city_name_get(acity),
                 TILE_XY(atile), go_by_boat, move_time, want, best);
      }

      if (want > best && ai_fuzzy(pplayer, TRUE)) {
        /* Yes, we like this target */
        best = want;
        *pdest_tile = atile;
        if (NULL != pferrymap) {
          *pferrymap = go_by_boat ? ferry_map : NULL;
        }
        if (NULL != pferryboat) {
          *pferryboat = go_by_boat ? ferryboat : NULL;
        }
        if (NULL != pboattype) {
          *pboattype = go_by_boat ? boattype : NULL;
        }
        if (NULL != pmove_time) {
          *pmove_time = move_time;
        }
        goto_dest_tile = (go_by_boat && NULL != ferryboat
                          ? unit_tile(ferryboat) : atile);
      }
    } city_list_iterate_end;

    attack = unit_att_rating_squared(punit);
    /* I'm not sure the following code is good but it seems to be adequate.
     * I am deliberately not adding ferryboat code to the unit_list_iterate.
     * -- Syela */
    unit_list_iterate(aplayer->units, aunit) {
      struct tile *atile = unit_tile(aunit);

      if (NULL != tile_city(atile)) {
        /* already dealt with it. */
        continue;
      }

      if (handicap && !map_is_known(atile, pplayer)) {
        /* Can't see the target. */
        continue;
      }

      if ((unit_has_type_flag(aunit, UTYF_CIVILIAN)
           || (utype_may_act_at_all(unit_type_get(aunit))
               && !utype_acts_hostile(unit_type_get(aunit))))
          && 0 == punit->id) {
        /* We will not build units just to chase caravans and
         * ambassadors. */
        continue;
      }

      /* We have to assume the attack is diplomatically ok.
       * We cannot use can_player_attack_tile, because we might not
       * be at war with aplayer yet */
      if (!can_unit_attack_tile(punit, atile)
          || aunit != get_defender(punit, atile)) {
        /* We cannot attack it, or it is not the main defender. */
        continue;
      }

      if (!pf_map_position(punit_map, atile, &pos)) {
        /* Cannot reach it. */
        continue;
      }

      vulnerability = unit_def_rating_squared(punit, aunit);
      benefit = unit_build_shield_cost_base(aunit);

      move_time = pos.turn;
      if (10 < move_time) {
        /* Too far. */
        want = 0;
      } else {
        want = kill_desire(benefit, attack, bcost, vulnerability, 1);
          /* Take into account maintainance of the unit. */
          /* FIXME: Depends on the government. */
        want -= move_time * SHIELD_WEIGHTING;
        /* Take into account unhappiness
         * (costs 2 luxuries to compensate). */
        want -= (unhap ? 2 * move_time * TRADE_WEIGHTING : 0);
      }
      want = military_amortize(pplayer, game_city_by_number(punit->homecity),
                               want, MAX(1, move_time), bcost_bal);
      if (want > best && ai_fuzzy(pplayer, TRUE)) {
        best = want;
        *pdest_tile = atile;
        if (NULL != pferrymap) {
          *pferrymap = NULL;
        }
        if (NULL != pferryboat) {
          *pferryboat = NULL;
        }
        if (NULL != pboattype) {
          *pboattype = NULL;
        }
        if (NULL != pmove_time) {
          *pmove_time = move_time;
        }
        goto_dest_tile = atile;
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  if (NULL != ppath) {
    *ppath = (NULL != goto_dest_tile && goto_dest_tile != punit_tile
              ? pf_map_path(punit_map, goto_dest_tile) : NULL);
  }

  pf_map_destroy(punit_map);
  if (NULL != ferry_map
      && (NULL == pferrymap || *pferrymap != ferry_map)) {
    pf_map_destroy(ferry_map);
  }

  TIMING_LOG(AIT_FSTK, TIMER_STOP);

  return best;
}

/**********************************************************************//**
  Find safe city to recover in. An allied player's city is just as good as
  one of our own, since both replenish our hitpoints and reduce unhappiness.

  TODO: Actually check how safe the city is. This is a difficult decision
  not easily taken, since we also want to protect unsafe cities, at least
  most of the time.
**************************************************************************/
struct city *find_nearest_safe_city(struct unit *punit)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct player *pplayer = unit_owner(punit);
  struct city *pcity, *best_city = NULL;
  int best = FC_INFINITY, cur;

  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  pfm = pf_map_new(&parameter);

  pf_map_move_costs_iterate(pfm, ptile, move_cost, TRUE) {
    if (move_cost > best) {
      /* We already found a better city. No need to continue. */
      break;
    }

    pcity = tile_city(ptile);
    if (NULL == pcity || !pplayers_allied(pplayer, city_owner(pcity))) {
      continue;
    }

    /* Score based on the move cost. */
    cur = move_cost;

    /* Note the unit owner may be different from the city owner. */
    if (0 == get_unittype_bonus(unit_owner(punit), ptile, unit_type_get(punit),
                               EFT_HP_REGEN)) {
      /* If we cannot regen fast our hit points here, let's make some
       * penalty. */
      cur *= 3;
    }

    if (cur < best) {
      best_city = pcity;
      best = cur;
    }
  } pf_map_move_costs_iterate_end;

  pf_map_destroy(pfm);
  return best_city;
}

/**********************************************************************//**
  Go berserk, assuming there are no targets nearby.
  TODO: Is it not possible to remove this special casing for barbarians?
  FIXME: enum unit_move_result
**************************************************************************/
static void dai_military_attack_barbarian(struct ai_type *ait,
                                          struct player *pplayer,
                                          struct unit *punit)
{
  struct city *pc;
  bool only_continent = TRUE;

  if (unit_transported(punit)) {
    /* If we are in transport, we can go to any continent.
     * Actually, we are not currently in a continent where to stay. */
    only_continent = FALSE;
  }

  if ((pc = find_closest_city(unit_tile(punit), NULL, pplayer, FALSE,
                              only_continent, FALSE, FALSE, TRUE, NULL))) {
    if (can_unit_exist_at_tile(&(wld.map), punit, unit_tile(punit))) {
      UNIT_LOG(LOG_DEBUG, punit, "Barbarian heading to conquer %s",
               city_name_get(pc));
      (void) dai_gothere(ait, pplayer, punit, pc->tile);
    } else {
      struct unit *ferry = NULL;

      if (unit_transported(punit)) {
        ferry = unit_transport_get(punit);

        /* We already are in a boat so it needs no
         * free capacity */
        if (!is_boat_free(ait, ferry, punit, 0)) {
          /* We cannot control our ferry. */
          ferry = NULL;
        }
      } else {
        /* We are not in a boat yet. Search for one. */
        unit_list_iterate(unit_tile(punit)->units, aunit) {
          if (is_boat_free(ait, aunit, punit, 1)
              && unit_transport_load(punit, aunit, FALSE)) {
            ferry = aunit;
            break;
          }
        } unit_list_iterate_end;
      }

      if (ferry) {
        UNIT_LOG(LOG_DEBUG, punit, "Barbarian sailing to conquer %s",
                 city_name_get(pc));
	(void) aiferry_goto_amphibious(ait, ferry, punit, pc->tile);
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

/**********************************************************************//**
  This does the attack until we have used up all our movement, unless we
  should safeguard a city.  First we rampage nearby, then we go
  looking for trouble elsewhere. If there is nothing to kill, sailing units 
  go home, others explore while barbs go berserk.
**************************************************************************/
static void dai_military_attack(struct ai_type *ait, struct player *pplayer,
                                struct unit *punit)
{
  struct tile *dest_tile;
  int id = punit->id;
  int ct = 10;
  struct city *pcity = NULL;

  CHECK_UNIT(punit);

  /* Barbarians pillage, and might keep on doing that so they sometimes
   * even finish it. */
  if (punit->activity == ACTIVITY_PILLAGE && is_barbarian(pplayer)
      && fc_rand(2) == 1) {
    return;
  }

  /* First find easy nearby enemies, anything better than pillage goes.
   * NB: We do not need to repeat dai_military_rampage, it is repeats itself
   * until it runs out of targets. */
  /* FIXME: 1. dai_military_rampage never checks if it should defend newly
   * conquered cities.
   * FIXME: 2. would be more convenient if it returned FALSE if we run out 
   * of moves too.*/
  if (!dai_military_rampage(punit, RAMPAGE_ANYTHING, RAMPAGE_ANYTHING)) {
    return; /* we died */
  }
  
  if (punit->moves_left <= 0) {
    return;
  }

  /* Main attack loop */
  do {
    struct tile *start_tile = unit_tile(punit);
    struct pf_path *path;
    struct unit *ferryboat;

    /* Then find enemies the hard way */
    find_something_to_kill(ait, pplayer, punit, &dest_tile, &path,
                           NULL, &ferryboat, NULL, NULL);
    if (!same_pos(unit_tile(punit), dest_tile)) {
      if (!is_tiles_adjacent(unit_tile(punit), dest_tile)
          || !can_unit_attack_tile(punit, dest_tile)) {

        /* Adjacent and can't attack usually means we are not marines
         * and on a ferry. This fixes the problem (usually). */
        UNIT_LOG(LOG_DEBUG, punit, "mil att gothere -> (%d, %d)",
                 TILE_XY(dest_tile));

        /* Set ACTIVITY_GOTO more permanently than just inside
         * adv_follow_path(). This way other units will know we're
         * on our way even if we don't reach target yet. */
        punit->goto_tile = dest_tile;
        unit_activity_handling(punit, ACTIVITY_GOTO);
        if (NULL != path && !adv_follow_path(punit, path, dest_tile)) {
          /* Died. */
          pf_path_destroy(path);
          return;
        }
        if (NULL != ferryboat) {
          /* Need a boat. */
          aiferry_gobyboat(ait, pplayer, punit, dest_tile, FALSE);
          pf_path_destroy(path);
          return;
        }
        if (0 >= punit->moves_left) {
          /* No moves left. */
          pf_path_destroy(path);
          return;
        }

        /* Either we're adjacent or we sitting on the tile. We might be
         * sitting on the tile if the enemy that _was_ sitting there 
         * attacked us and died _and_ we had enough movement to get there */
        if (same_pos(unit_tile(punit), dest_tile)) {
          UNIT_LOG(LOG_DEBUG, punit, "mil att made it -> (%d, %d)",
                   TILE_XY(dest_tile));
          pf_path_destroy(path);
          break;
        }
      }

      if (is_tiles_adjacent(unit_tile(punit), dest_tile)) {
        /* Close combat. fstk sometimes want us to attack an adjacent
         * enemy that rampage wouldn't */
        UNIT_LOG(LOG_DEBUG, punit, "mil att bash -> (%d, %d)",
                 TILE_XY(dest_tile));
        if (!dai_unit_attack(ait, punit, dest_tile)) {
          /* Died */
          pf_path_destroy(path);
          return;
        }
      } else if (!same_pos(start_tile, unit_tile(punit))) {
        /* Got stuck. Possibly because of adjacency to an
         * enemy unit. Perhaps we are in luck and are now next to a
         * tempting target? Let's find out... */
        (void) dai_military_rampage(punit,
                                    RAMPAGE_ANYTHING, RAMPAGE_ANYTHING);
        pf_path_destroy(path);
        return;
      }

    } else {
      /* FIXME: This happens a bit too often! */
      UNIT_LOG(LOG_DEBUG, punit, "fstk didn't find us a worthy target!");
      /* No worthy enemies found, so abort loop */
      ct = 0;
    }
    pf_path_destroy(path);

    ct--; /* infinite loops from railroads must be stopped */
  } while (punit->moves_left > 0 && ct > 0);

  /* Cleanup phase */
  if (punit->moves_left == 0) {
    return;
  }
  pcity = find_nearest_safe_city(punit);
  if (pcity != NULL
      && (dai_is_ferry(punit, ait)
          || punit->hp < unit_type_get(punit)->hp * 0.50)) { /* WAG */
    /* Go somewhere safe */
    UNIT_LOG(LOG_DEBUG, punit, "heading to nearest safe house.");
    (void) dai_unit_goto(ait, punit, pcity->tile);
  } else if (!is_barbarian(pplayer)) {
    /* Nothing else to do, so try exploring. */
    switch (manage_auto_explorer(punit)) {
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
    dai_military_attack_barbarian(ait, pplayer, punit);
  }
  if ((punit = game_unit_by_number(id)) && punit->moves_left > 0) {
    UNIT_LOG(LOG_DEBUG, punit, "attack: giving up unit to defense");
    dai_military_defend(ait, pplayer, punit);
  }
}

/**********************************************************************//**
  Request a boat for a unit to transport it to another continent.
  Return whether is alive or not
**************************************************************************/
static bool dai_find_boat_for_unit(struct ai_type *ait, struct unit *punit)
{
  bool alive = TRUE;
  int ferryboat = 0;
  struct pf_path *path_to_ferry = NULL;

  UNIT_LOG(LOG_CARAVAN, punit, "requesting a boat!");
  ferryboat = aiferry_find_boat(ait, punit, 1, &path_to_ferry);
  /* going to meet the boat */
  if ((ferryboat <= 0)) {
    UNIT_LOG(LOG_CARAVAN, punit, 
             "in find_boat_for_unit cannot find any boats.");
    /* if we are undefended on the country side go to a city */
    struct city *current_city = tile_city(unit_tile(punit));
    if (current_city == NULL) {
      struct city *city_near = find_nearest_safe_city(punit);
      if (city_near != NULL) {
        alive = dai_unit_goto(ait, punit, city_near->tile);
      }
    }
  } else {
    if (path_to_ferry != NULL) {
      if (!adv_unit_execute_path(punit, path_to_ferry)) { 
        /* Died. */
        pf_path_destroy(path_to_ferry);
        path_to_ferry = NULL;
        alive = FALSE;
      } else {
        pf_path_destroy(path_to_ferry);
        path_to_ferry = NULL;
        alive = TRUE;
      }
    }
  }
  return alive;
}

/**********************************************************************//**
  Send the caravan to the specified city, or make it help the wonder /
  trade, if it's already there.  After this call, the unit may no longer
  exist (it might have been used up, or may have died travelling).
  It uses the ferry system to trade among continents.
**************************************************************************/
static void dai_caravan_goto(struct ai_type *ait, struct player *pplayer,
                             struct unit *punit,
                             const struct city *dest_city,
                             bool help_wonder,
                             bool required_boat, bool request_boat)
{
  bool alive = TRUE;
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  fc_assert_ret(NULL != dest_city);

  /* if we're not there yet, and we can move, move... */
  if (!same_pos(dest_city->tile, unit_tile(punit))
      && punit->moves_left != 0) {
    log_base(LOG_CARAVAN, "%s %s[%d](%d,%d) task %s going to %s in %s %s",
             nation_rule_name(nation_of_unit(punit)),
             unit_rule_name(punit), punit->id, TILE_XY(unit_tile(punit)),
             dai_unit_task_rule_name(unit_data->task),
             help_wonder ? "help a wonder" : "trade", city_name_get(dest_city),
             required_boat ? "with a boat" : "");
    if (required_boat) {
      /* to trade with boat */
      if (request_boat) {
        /* Try to find new boat */
        alive = dai_find_boat_for_unit(ait, punit);
      } else {
        /* if we are not being transported then ask for a boat again */
        alive = TRUE;
        if (!unit_transported(punit) &&
            (tile_continent(unit_tile(punit))
             != tile_continent(dest_city->tile))) {
          alive = dai_find_boat_for_unit(ait, punit);
        }
      }
      if (alive)  {
	alive = dai_gothere(ait, pplayer, punit, dest_city->tile);
      }
    } else {
      /* to trade without boat */
      alive = dai_unit_goto(ait, punit, dest_city->tile);
    }
  }

  /* if moving didn't kill us, and we got to the destination, handle it. */
  if (alive && real_map_distance(dest_city->tile, unit_tile(punit)) <= 1) {
    /* release the boat! */
    if (unit_transported(punit)) {
      aiferry_clear_boat(ait, punit);
    }
    if (help_wonder && is_action_enabled_unit_on_city(ACTION_HELP_WONDER,
                                                      punit, dest_city)) {
        /*
         * We really don't want to just drop all caravans in immediately.
         * Instead, we want to aggregate enough caravans to build instantly.
         * -AJS, 990704
         */
      log_base(LOG_CARAVAN, "%s %s[%d](%d,%d) helps build wonder in %s",
               nation_rule_name(nation_of_unit(punit)),
               unit_rule_name(punit),
               punit->id,
               TILE_XY(unit_tile(punit)),
               city_name_get(dest_city));
      unit_do_action(pplayer, punit->id, dest_city->id,
                     0, "", ACTION_HELP_WONDER);
    } else if (is_action_enabled_unit_on_city(ACTION_TRADE_ROUTE,
                                              punit, dest_city)) {
      log_base(LOG_CARAVAN, "%s %s[%d](%d,%d) creates trade route in %s",
               nation_rule_name(nation_of_unit(punit)),
               unit_rule_name(punit),
               punit->id,
               TILE_XY(unit_tile(punit)),
               city_name_get(dest_city));
      unit_do_action(pplayer, punit->id, dest_city->id,
                     0, "", ACTION_TRADE_ROUTE);
    } else if (is_action_enabled_unit_on_city(ACTION_MARKETPLACE,
                                              punit, dest_city)) {
      /* Get the one time bonus. */
      log_base(LOG_CARAVAN, "%s %s[%d](%d,%d) enters marketplace of %s",
               nation_rule_name(nation_of_unit(punit)),
               unit_rule_name(punit),
               punit->id,
               TILE_XY(unit_tile(punit)),
               city_name_get(dest_city));
      unit_do_action(pplayer, punit->id, dest_city->id,
                     0, "", ACTION_MARKETPLACE);
    } else {
      enum log_level level = LOG_NORMAL;

      if (help_wonder) {
        /* A Caravan ordered to help build wonder may arrive after
         * enough shields to build the wonder is produced. */
        level = LOG_VERBOSE;
      }

      log_base(level,
               "%s %s[%d](%d,%d) unable to trade with %s",
               nation_rule_name(nation_of_unit(punit)),
               unit_rule_name(punit),
               punit->id,
               TILE_XY(unit_tile(punit)),
               city_name_get(dest_city));
    }
  }
}

/**********************************************************************//**
  For debugging, print out information about every city we come to when
  optimizing the caravan.
**************************************************************************/
static void caravan_optimize_callback(const struct caravan_result *result,
                                      void *data)
{
  const struct unit *caravan = data;

  log_base(LOG_CARAVAN3, "%s %s[%d](%d,%d) %s: %s %s worth %g",
           nation_rule_name(nation_of_unit(caravan)),
           unit_rule_name(caravan),
           caravan->id,
           TILE_XY(unit_tile(caravan)),
           city_name_get(result->src),
           result->help_wonder ? "wonder in" : "trade to",
           city_name_get(result->dest),
           result->value);
}

/**********************************************************************//**
  Evaluate if a unit is tired of waiting for a boat at home continent
**************************************************************************/
static bool dai_is_unit_tired_waiting_boat(struct ai_type *ait,
                                           struct unit *punit)
{
  struct tile *src = NULL, *dest = NULL, *src_home_city = NULL;
  struct city *phome_city = NULL;
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);
  
  if ((unit_data->task != AIUNIT_NONE)) {
    src = unit_tile(punit);
    phome_city = game_city_by_number(punit->homecity);
    if (phome_city != NULL) {
      src_home_city = city_tile(phome_city);
    }
    dest = punit->goto_tile;

    if (src == NULL || dest == NULL) {
      return FALSE;
    }
    /* if we're not at home continent */
    if (tile_continent(src) != tile_continent(src_home_city)) {
      return FALSE;
    }

    if (!goto_is_sane(punit, dest)) {
      if (unit_transported(punit)) {
        /* if we're being transported */
        return FALSE;
      }
      if ((punit->server.birth_turn + 15 < game.info.turn)) {
        /* we are tired of waiting */
        int ferrys = aiferry_avail_boats(ait, punit->owner);

        if (ferrys <= 0) {
          /* there are no ferrys available... give up */
          return TRUE;
        } else {
          if (punit->server.birth_turn + 20 < game.info.turn) {
            /* we are feed up! */
            return TRUE;
          }
        }
      }
    }
  }

  return FALSE;
}

/**********************************************************************//**
  Check if a caravan can make a trade route to a city on a different
  continent.
**************************************************************************/
static bool dai_caravan_can_trade_cities_diff_cont(struct player *pplayer,
                                                   struct unit *punit) {
  struct city *pcity = game_city_by_number(punit->homecity);
  Continent_id continent;

  fc_assert(pcity != NULL);

  continent = tile_continent(pcity->tile);

  /* Look for proper destination city at different continent. */
  city_list_iterate(pplayer->cities, acity) {
    if (can_cities_trade(pcity, acity)) {
      if (tile_continent(acity->tile) != continent) {
        return TRUE;
      }
    }
  } city_list_iterate_end;

  players_iterate(aplayer) {
    if (aplayer == pplayer || !aplayer->is_alive) {
      continue;
    }
    if (pplayers_allied(pplayer, aplayer)) {      
      city_list_iterate(aplayer->cities, acity) {
        if (can_cities_trade(pcity, acity)) {
          if (tile_continent(acity->tile) != continent) {
            return TRUE;
          }
        }
      } city_list_iterate_end;
    } players_iterate_end;
  }

  return FALSE;
}

/**********************************************************************//**
  Try to move caravan to suitable city and to make it caravan's homecity.
  Returns FALSE iff caravan dies.
**************************************************************************/
static bool search_homecity_for_caravan(struct ai_type *ait, struct unit *punit)
{
  struct city *nearest = NULL;
  int min_dist = FC_INFINITY;
  struct tile *current_loc = unit_tile(punit);
  Continent_id continent = tile_continent(current_loc);
  bool alive = TRUE;

  city_list_iterate(punit->owner->cities, pcity) {
    struct tile *ctile = city_tile(pcity);

    if (tile_continent(ctile) == continent) {
      int this_dist = map_distance(current_loc, ctile);

      if (this_dist < min_dist) {
        min_dist = this_dist;
        nearest = pcity;
      }
    }
  } city_list_iterate_end;

  if (nearest != NULL) {
    alive = dai_unit_goto(ait, punit, nearest->tile);
    if (alive && same_pos(unit_tile(punit), nearest->tile)) {
      dai_unit_make_homecity(punit, nearest);
    }
  }

  return alive;
}

/**********************************************************************//**
  Use caravans for building wonders, or send caravans to establish
  trade with a city, owned by yourself or an ally.

  We use ferries for trade across the sea.
**************************************************************************/
static void dai_manage_caravan(struct ai_type *ait, struct player *pplayer,
                               struct unit *punit)
{
  struct caravan_parameter parameter;
  struct caravan_result result;
  const struct city *homecity;
  const struct city *dest = NULL;
  struct unit_ai *unit_data;
  bool help_wonder = FALSE;
  bool required_boat = FALSE;
  bool request_boat = FALSE;
  bool tired_of_waiting_boat = FALSE;

  CHECK_UNIT(punit);

  if (!unit_can_do_action(punit, ACTION_TRADE_ROUTE)
      && !unit_can_do_action(punit, ACTION_MARKETPLACE)
      && !unit_can_do_action(punit, ACTION_HELP_WONDER)) {
    /* we only want units that can establish trade, enter marketplace or
     * help build wonders */
    return;
  }

  unit_data = def_ai_unit_data(punit, ait);

  log_base(LOG_CARAVAN2, "%s %s[%d](%d,%d) task %s to (%d,%d)",
           nation_rule_name(nation_of_unit(punit)),
           unit_rule_name(punit), punit->id, TILE_XY(unit_tile(punit)),
           dai_unit_task_rule_name(unit_data->task), 
           TILE_XY(punit->goto_tile));

  homecity = game_city_by_number(punit->homecity);
  if (homecity == NULL && unit_data->task == AIUNIT_TRADE) {
    if (!search_homecity_for_caravan(ait, punit)) {
      return;
    }
    homecity = game_city_by_number(punit->homecity);
    if (homecity == NULL) {
      return;
    }
  }

  if ((unit_data->task == AIUNIT_TRADE
       || unit_data->task == AIUNIT_WONDER)) {
    /* we are moving to our destination */
    /* we check to see if our current goal is feasible */
    struct city *city_dest = tile_city(punit->goto_tile);

    if ((city_dest == NULL) 
        || !pplayers_allied(unit_owner(punit), city_dest->owner)
        || (unit_data->task == AIUNIT_TRADE
            && !(can_cities_trade(homecity, city_dest)
                 && can_establish_trade_route(homecity, city_dest)))
        || (unit_data->task == AIUNIT_WONDER
            /* Helping the (new) production is illegal. */
            && !city_production_gets_caravan_shields(&city_dest->production))
        || (unit_data->task == AIUNIT_TRADE
            && real_map_distance(city_dest->tile, unit_tile(punit)) <= 1
            && !(is_action_enabled_unit_on_city(ACTION_TRADE_ROUTE,
                                                punit, city_dest)
                 || is_action_enabled_unit_on_city(ACTION_MARKETPLACE,
                                                   punit, city_dest)))
        || (unit_data->task == AIUNIT_WONDER
            && real_map_distance(city_dest->tile, unit_tile(punit)) <= 1
            && !is_action_enabled_unit_on_city(ACTION_HELP_WONDER,
                                               punit, city_dest))) {
      /* destination invalid! */
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
      log_base(LOG_CARAVAN2, "%s %s[%d](%d,%d) destination invalid!",
               nation_rule_name(nation_of_unit(punit)),
               unit_rule_name(punit), punit->id, TILE_XY(unit_tile(punit)));
    } else {
      /* destination valid, are we tired of waiting for a boat? */
      if (dai_is_unit_tired_waiting_boat(ait, punit)) {
        aiferry_clear_boat(ait, punit);
        dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
        log_base(LOG_CARAVAN2, "%s %s[%d](%d,%d) unit tired of waiting!",
                 nation_rule_name(nation_of_unit(punit)),
                 unit_rule_name(punit), punit->id, TILE_XY(unit_tile(punit)));
        tired_of_waiting_boat = TRUE;
      } else {
        dest = city_dest;
        help_wonder = (unit_data->task == AIUNIT_WONDER) ? TRUE : FALSE;
        required_boat = (tile_continent(unit_tile(punit)) == 
                         tile_continent(dest->tile)) ? FALSE : TRUE;
        request_boat = FALSE;
      }
    }
  }

  if (unit_data->task == AIUNIT_NONE) {
    if (homecity == NULL) {
      /* FIXME: We shouldn't bother in getting homecity for
       * caravan that will then be used for wonder building. */
      if (!search_homecity_for_caravan(ait, punit)) {
        return;
      }
      homecity = game_city_by_number(punit->homecity);
      if (homecity == NULL) {
        return;
      }
    }

    caravan_parameter_init_from_unit(&parameter, punit);
    /* Make more trade with allies than other peaceful nations
     * by considering only allies 50% of the time.
     * (the other 50% allies are still considered, but so are other nations) */
    if (fc_rand(2)) {
      /* Be optimistic about development of relations with no-contact and
       * cease-fire nations. */
      parameter.allow_foreign_trade = FTL_NONWAR;
    } else {
      parameter.allow_foreign_trade = FTL_ALLIED;
    }

    if (log_do_output_for_level(LOG_CARAVAN2)) {
      parameter.callback = caravan_optimize_callback;
      parameter.callback_data = punit;
    }
    if (dai_caravan_can_trade_cities_diff_cont(pplayer, punit)) {
      parameter.ignore_transit_time = TRUE;
    }
    if (tired_of_waiting_boat) {
      parameter.allow_foreign_trade = FTL_NATIONAL_ONLY;
      parameter.ignore_transit_time = FALSE;
    }
    caravan_find_best_destination(punit, &parameter, &result, !has_handicap(pplayer, H_MAP));
    if (result.dest != NULL) {
      /* we did find a new destination for the unit */
      dest = result.dest;
      help_wonder = result.help_wonder;
      required_boat = (tile_continent(unit_tile(punit)) ==
                       tile_continent(dest->tile)) ? FALSE : TRUE;
      request_boat = required_boat;
      dai_unit_new_task(ait, punit,
                        (help_wonder) ? AIUNIT_WONDER : AIUNIT_TRADE,
                        dest->tile);
    } else {
      dest = NULL;
    }
  }

  if (dest != NULL) {
    dai_caravan_goto(ait, pplayer, punit, dest, help_wonder, 
                     required_boat, request_boat);
    return; /* that may have clobbered the unit */
  } else {
    /* We have nowhere to go! */
     log_base(LOG_CARAVAN2, "%s %s[%d](%d,%d), nothing to do!",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit), punit->id,
              TILE_XY(unit_tile(punit)));
     /* Should we become a defensive unit? */
  }
}

/**********************************************************************//**
  This function goes wait a unit in a city for the hitpoints to recover.
  If something is attacking our city, kill it yeahhh!!!.
**************************************************************************/
static void dai_manage_hitpoint_recovery(struct ai_type *ait,
                                         struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(unit_tile(punit));
  struct city *safe = NULL;
  struct unit_type *punittype = unit_type_get(punit);

  CHECK_UNIT(punit);

  if (pcity) {
    /* rest in city until the hitpoints are recovered, but attempt
       to protect city from attack (and be opportunistic too)*/
    if (dai_military_rampage(punit, RAMPAGE_ANYTHING, 
                             RAMPAGE_FREE_CITY_OR_BETTER)) {
      UNIT_LOG(LOGLEVEL_RECOVERY, punit, "recovering hit points.");
    } else {
      return; /* we died heroically defending our city */
    }
  } else {
    /* goto to nearest city to recover hit points */
    /* just before, check to see if we can occupy an undefended enemy city */
    if (!dai_military_rampage(punit, RAMPAGE_FREE_CITY_OR_BETTER, 
                              RAMPAGE_FREE_CITY_OR_BETTER)) { 
      return; /* oops, we died */
    }

    /* find city to stay and go there */
    safe = find_nearest_safe_city(punit);
    if (safe) {
      UNIT_LOG(LOGLEVEL_RECOVERY, punit, "going to %s to recover",
               city_name_get(safe));
      if (!dai_unit_goto(ait, punit, safe->tile)) {
        log_base(LOGLEVEL_RECOVERY, "died trying to hide and recover");
        return;
      }
    } else {
      /* oops */
      UNIT_LOG(LOGLEVEL_RECOVERY, punit, "didn't find a city to recover in!");
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
      dai_military_attack(ait, pplayer, punit);
      return;
    }
  }

  /* is the unit still damaged? if true recover hit points, if not idle */
  if (punit->hp == punittype->hp) {
    /* we are ready to go out and kick ass again */
    UNIT_LOG(LOGLEVEL_RECOVERY, punit, "ready to kick ass again!");
    dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);  
    return;
  } else {
    def_ai_unit_data(punit, ait)->done = TRUE; /* sit tight */
  }
}

/**********************************************************************//**
  Decide what to do with a military unit. It will be managed once only.
  It is up to the caller to try again if it has moves left.
**************************************************************************/
void dai_manage_military(struct ai_type *ait, struct player *pplayer,
                         struct unit *punit)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);
  int id = punit->id;

  CHECK_UNIT(punit);

  /* "Escorting" aircraft should not do anything. They are activated
   * by their transport or charge.  We do _NOT_ set them to 'done'
   * since they may need be activated once our charge moves. */
  if (unit_data->task == AIUNIT_ESCORT
      && utype_fuel(unit_type_get(punit))) {
    return;
  }

  if ((punit->activity == ACTIVITY_SENTRY
       || punit->activity == ACTIVITY_FORTIFIED)
      && has_handicap(pplayer, H_AWAY)) {
    /* Don't move sentried or fortified units controlled by a player
     * in away mode. */
    unit_data->done = TRUE;
    return;
  }

  /* Since military units re-evaluate their actions every turn,
     we must make sure that previously reserved ferry is freed. */
  aiferry_clear_boat(ait, punit);

  TIMING_LOG(AIT_HUNTER, TIMER_START);
  /* Try hunting with this unit */
  if (dai_hunter_qualify(pplayer, punit)) {
    int result, sanity = punit->id;

    UNIT_LOG(LOGLEVEL_HUNT, punit, "is qualified as hunter");
    result = dai_hunter_manage(ait, pplayer, punit);
    if (NULL == game_unit_by_number(sanity)) {
      TIMING_LOG(AIT_HUNTER, TIMER_STOP);
      return; /* died */
    }
    if (result == -1) {
      (void) dai_hunter_manage(ait, pplayer, punit); /* More carnage */
      TIMING_LOG(AIT_HUNTER, TIMER_STOP);
      return;
    } else if (result >= 1) {
      TIMING_LOG(AIT_HUNTER, TIMER_STOP);
      return; /* Done moving */
    } else if (unit_data->task == AIUNIT_HUNTER) {
      /* This should be very rare */
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    }
  } else if (unit_data->task == AIUNIT_HUNTER) {
    dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
  }
  TIMING_LOG(AIT_HUNTER, TIMER_STOP);

  /* Do we have a specific job for this unit? If not, we default
   * to attack. */
  dai_military_findjob(ait, pplayer, punit);

  switch (unit_data->task) {
  case AIUNIT_AUTO_SETTLER:
  case AIUNIT_BUILD_CITY:
    fc_assert(FALSE); /* This is not the place for this role */
    break;
  case AIUNIT_DEFEND_HOME:
    TIMING_LOG(AIT_DEFENDERS, TIMER_START);
    dai_military_defend(ait, pplayer, punit);
    TIMING_LOG(AIT_DEFENDERS, TIMER_STOP);
    break;
  case AIUNIT_ATTACK:
  case AIUNIT_NONE:
    TIMING_LOG(AIT_ATTACK, TIMER_START);
    dai_military_attack(ait, pplayer, punit);
    TIMING_LOG(AIT_ATTACK, TIMER_STOP);
    break;
  case AIUNIT_ESCORT: 
    TIMING_LOG(AIT_BODYGUARD, TIMER_START);
    dai_military_bodyguard(ait, pplayer, punit);
    TIMING_LOG(AIT_BODYGUARD, TIMER_STOP);
    break;
  case AIUNIT_EXPLORE:
    switch (manage_auto_explorer(punit)) {
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
    def_ai_unit_data(punit, ait)->done = (punit->moves_left <= 0);
    break;
  case AIUNIT_RECOVER:
    TIMING_LOG(AIT_RECOVER, TIMER_START);
    dai_manage_hitpoint_recovery(ait, punit);
    TIMING_LOG(AIT_RECOVER, TIMER_STOP);
    break;
  case AIUNIT_HUNTER:
    fc_assert(FALSE); /* dealt with above */
    break;
  default:
    fc_assert(FALSE);
  }

  /* If we are still alive, either sentry or fortify. */
  if ((punit = game_unit_by_number(id))) {
    unit_data = def_ai_unit_data(punit, ait);
    struct city *pcity = tile_city(unit_tile(punit));

    if (unit_list_find(unit_tile(punit)->units,
                       unit_data->ferryboat)) {
      unit_activity_handling(punit, ACTIVITY_SENTRY);
    } else if (pcity || punit->activity == ACTIVITY_IDLE) {
      /* We do not need to fortify in cities - we fortify and sentry
       * according to home defense setup, for easy debugging. */
      if (!pcity || unit_data->task == AIUNIT_DEFEND_HOME) {
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

/**********************************************************************//**
  Manages settlers.
**************************************************************************/
static void dai_manage_settler(struct ai_type *ait, struct player *pplayer,
                               struct unit *punit)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  punit->ai_controlled = TRUE;
  unit_data->done = TRUE; /* we will manage this unit later... ugh */
  /* if BUILD_CITY must remain BUILD_CITY, otherwise turn into autosettler */
  if (unit_data->task == AIUNIT_NONE) {
    adv_unit_new_task(punit, AUT_AUTO_SETTLER, NULL);
  }
  return;
}

/**********************************************************************//**
  manage one unit
  Careful: punit may have been destroyed upon return from this routine!

  Gregor: This function is a very limited approach because if a unit has
  several flags the first one in order of appearance in this function
  will be used.
**************************************************************************/
void dai_manage_unit(struct ai_type *ait, struct player *pplayer,
                     struct unit *punit)
{
  struct unit_ai *unit_data;
  struct unit *bodyguard = aiguard_guard_of(ait, punit);
  bool is_ferry = FALSE;

  CHECK_UNIT(punit);

  /* Don't manage the unit if it is under human orders. */
  if (unit_has_orders(punit)) {
    unit_data = def_ai_unit_data(punit, ait);

    UNIT_LOG(LOG_VERBOSE, punit, "is under human orders, aborting AI control.");
    dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    unit_data->done = TRUE;
    return;
  }

  /* Check if we have lost our bodyguard. If we never had one, all
   * fine. If we had one and lost it, ask for a new one. */
  if (!bodyguard && aiguard_has_guard(ait, punit)) {
    UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "lost bodyguard, asking for new");
    aiguard_request_guard(ait, punit);
  }

  unit_data = def_ai_unit_data(punit, ait);

  if (punit->moves_left <= 0) {
    /* Can do nothing */
    unit_data->done = TRUE;
    return;
  }

  is_ferry = dai_is_ferry(punit, ait);

  if (unit_has_type_flag(punit, UTYF_DIPLOMAT)) {
    TIMING_LOG(AIT_DIPLOMAT, TIMER_START);
    dai_manage_diplomat(ait, pplayer, punit);
    TIMING_LOG(AIT_DIPLOMAT, TIMER_STOP);
    return;
  } else if (unit_has_type_flag(punit, UTYF_SETTLERS)
             || unit_is_cityfounder(punit)) {
    dai_manage_settler(ait, pplayer, punit);
    return;
  } else if (unit_can_do_action(punit, ACTION_TRADE_ROUTE)
             || unit_can_do_action(punit, ACTION_MARKETPLACE)
             || unit_can_do_action(punit, ACTION_HELP_WONDER)) {
    TIMING_LOG(AIT_CARAVAN, TIMER_START);
    dai_manage_caravan(ait, pplayer, punit);
    TIMING_LOG(AIT_CARAVAN, TIMER_STOP);
    return;
  } else if (unit_has_type_role(punit, L_BARBARIAN_LEADER)) {
    dai_manage_barbarian_leader(ait, pplayer, punit);
    return;
  } else if (unit_can_do_action(punit, ACTION_PARADROP)) {
    dai_manage_paratrooper(ait, pplayer, punit);
    return;
  } else if (is_ferry && unit_data->task != AIUNIT_HUNTER) {
    TIMING_LOG(AIT_FERRY, TIMER_START);
    dai_manage_ferryboat(ait, pplayer, punit);
    TIMING_LOG(AIT_FERRY, TIMER_STOP);
    return;
  } else if (utype_fuel(unit_type_get(punit))
             && unit_data->task != AIUNIT_ESCORT) {
    TIMING_LOG(AIT_AIRUNIT, TIMER_START);
    dai_manage_airunit(ait, pplayer, punit);
    TIMING_LOG(AIT_AIRUNIT, TIMER_STOP);
    return;
  } else if (is_losing_hp(punit)) {
    /* This unit is losing hitpoints over time */

    /* TODO: We can try using air-unit code for helicopters, just
     * pretend they have fuel = HP / 3 or something. */
    unit_data->done = TRUE; /* we did our best, which was ... 
                                             nothing */
    return;
  } else if (is_military_unit(punit)) {
    TIMING_LOG(AIT_MILITARY, TIMER_START);
    UNIT_LOG(LOG_DEBUG, punit, "recruit unit for the military");
    dai_manage_military(ait, pplayer, punit); 
    TIMING_LOG(AIT_MILITARY, TIMER_STOP);
    return;
  } else {
    /* what else could this be? -- Syela */
    switch (manage_auto_explorer(punit)) {
    case MR_DEATH:
      /* don't use punit! */
      break;
    case MR_OK:
      UNIT_LOG(LOG_DEBUG, punit, "now exploring");
      break;
    default:
      UNIT_LOG(LOG_DEBUG, punit, "fell through all unit tasks, defending");
      dai_unit_new_task(ait, punit, AIUNIT_DEFEND_HOME, NULL);
      dai_military_defend(ait, pplayer, punit);
      break;
    };
    return;
  }
}

/**********************************************************************//**
  Master city defense function.  We try to pick up the best available
  defenders, and not disrupt existing roles.

  TODO: Make homecity, respect homecity.
**************************************************************************/
static void dai_set_defenders(struct ai_type *ait, struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    /* The idea here is that we should never keep more than two
     * units in permanent defense. */
    int total_defense = 0;
    int total_attack = def_ai_city_data(pcity, ait)->danger;
    bool emergency = FALSE;
    int count = 0;
    int mart_max = get_city_bonus(pcity, EFT_MARTIAL_LAW_MAX);
    int mart_each = get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH);
    int martless_unhappy = pcity->feel[CITIZEN_UNHAPPY][FEELING_NATIONALITY]
      + pcity->feel[CITIZEN_ANGRY][FEELING_NATIONALITY];
    int entertainers = 0;
    bool enough = FALSE;

    specialist_type_iterate(sp) {
      if (get_specialist_output(pcity, sp, O_LUXURY) > 0) {
        entertainers += pcity->specialists[sp];
      }
    } specialist_type_iterate_end;

    martless_unhappy += entertainers; /* We want to use martial law instead
                                       * of entertainers. */

    while (!enough
           && (total_defense <= total_attack
               || (count < mart_max && mart_each > 0
                   && martless_unhappy > mart_each * count))) {
      int best_want = 0;
      struct unit *best = NULL;
      bool defense_needed = total_defense <= total_attack; /* Defense or martial */

      unit_list_iterate(pcity->tile->units, punit) {
        struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

        if ((unit_data->task == AIUNIT_NONE || emergency)
            && unit_data->task != AIUNIT_DEFEND_HOME
            && unit_owner(punit) == pplayer) {
          int want = assess_defense_unit(ait, pcity, punit, FALSE);

          if (want > best_want) {
            best_want = want;
            best = punit;
          }
        }
      } unit_list_iterate_end;
      
      if (best == NULL) {
        if (defense_needed) {
          /* Ooops - try to grab any unit as defender! */
          if (emergency) {
            CITY_LOG(LOG_DEBUG, pcity, "Not defended properly");
            break;
          }
          emergency = TRUE;
        } else {
          break;
        }
      } else {
        struct unit_type *btype = unit_type_get(best);

        if ((martless_unhappy < mart_each * count
             || count >= mart_max || mart_each <= 0)
            && ((count >= 2
                 && btype->attack_strength > btype->defense_strength)
                || (count >= 4
                    && btype->attack_strength == btype->defense_strength))) {
          /* In this case attack would be better defense than fortifying
           * to city. */
          enough = TRUE;
        } else {
          int loglevel = pcity->server.debug ? LOG_AI_TEST : LOG_DEBUG;

          total_defense += best_want;
          UNIT_LOG(loglevel, best, "Defending city");
          dai_unit_new_task(ait, best, AIUNIT_DEFEND_HOME, pcity->tile);
          count++;
        }
      }
    }
    CITY_LOG(LOG_DEBUG, pcity, "Evaluating defense: %d defense, %d incoming"
             ", %d defenders (out of %d)", total_defense, total_attack, count,
             unit_list_size(pcity->tile->units));
  } city_list_iterate_end;
}

/**********************************************************************//**
  Master manage unit function.

  A manage function should set the unit to 'done' when it should no
  longer be touched by this code, and its role should be reset to IDLE
  when its role has accomplished its mission or the manage function
  fails to have or no longer has any use for the unit.
**************************************************************************/
void dai_manage_units(struct ai_type *ait, struct player *pplayer) 
{
  TIMING_LOG(AIT_AIRLIFT, TIMER_START);
  dai_airlift(ait, pplayer);
  TIMING_LOG(AIT_AIRLIFT, TIMER_STOP);

  /* Clear previous orders, if desirable, here. */
  unit_list_iterate(pplayer->units, punit) {
    struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

    unit_data->done = FALSE;
    if (unit_data->task == AIUNIT_DEFEND_HOME) {
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    }
  } unit_list_iterate_end;

  /* Find and set city defenders first - figure out which units are
   * allowed to leave home. */
  dai_set_defenders(ait, pplayer);

  unit_list_iterate_safe(pplayer->units, punit) {
    if ((!unit_transported(punit) || unit_owner(unit_transport_get(punit)) != pplayer)
         && !def_ai_unit_data(punit, ait)->done) {
      /* Though it is usually the passenger who drives the transport,
       * the transporter is responsible for managing its passengers. */
      dai_manage_unit(ait, pplayer, punit);
    }
  } unit_list_iterate_safe_end;
}

/**********************************************************************//**
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

/**********************************************************************//**
  Barbarian leader tries to stack with other barbarian units, and if it's
  not possible it runs away. When on coast, it may disappear with 33%
  chance.
**************************************************************************/
static void dai_manage_barbarian_leader(struct ai_type *ait,
                                        struct player *pplayer,
                                        struct unit *leader)
{
  struct tile *leader_tile = unit_tile(leader), *safest_tile;
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_reverse_map *pfrm;
  struct unit *worst_danger;
  int move_cost, best_move_cost;
  int body_guards;
  bool alive = TRUE;

  CHECK_UNIT(leader);

  if (0 == leader->moves_left
      || (can_unit_survive_at_tile(&(wld.map), leader, leader_tile)
          && 1 < unit_list_size(leader_tile->units))) {
    unit_activity_handling(leader, ACTIVITY_SENTRY);
    return;
  }

  if (is_boss_of_boat(ait, leader)) {
    /* We are in charge. Of course, since we are the leader...
     * But maybe somebody more militaristic should lead our ship to battle! */

    /* First release boat from leaders lead */
    aiferry_clear_boat(ait, leader);

    unit_list_iterate(leader_tile->units, warrior) {
      if (!unit_has_type_role(warrior, L_BARBARIAN_LEADER)
          && get_transporter_capacity(warrior) == 0
          && warrior->moves_left > 0) {
        /* This seems like a good warrior to lead us in to conquest! */
        dai_manage_unit(ait, pplayer, warrior);

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

  /* Check the total number of units able to protect our leader. */
  body_guards = 0;
  unit_list_iterate(pplayer->units, punit) {
    if (!unit_has_type_role(punit, L_BARBARIAN_LEADER)
        && goto_is_sane(punit, leader_tile)) {
      body_guards++;
    }
  } unit_list_iterate_end;

  if (0 < body_guards) {
    pft_fill_unit_parameter(&parameter, leader);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    pfm = pf_map_new(&parameter);

    /* Find the closest body guard. FIXME: maybe choose the strongest too? */
    pf_map_tiles_iterate(pfm, ptile, FALSE) {
      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) == pplayer
            && !unit_has_type_role(punit, L_BARBARIAN_LEADER)
            && goto_is_sane(punit, leader_tile)) {
          struct pf_path *path = pf_map_path(pfm, ptile);

          adv_follow_path(leader, path, ptile);
          pf_path_destroy(path);
          pf_map_destroy(pfm);
          return;
        }
      } unit_list_iterate_end;
    } pf_map_tiles_iterate_end;

    pf_map_destroy(pfm);
  }

  UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader needs to flee");

  /* Check for units we could fear. */
  pfrm = pf_reverse_map_new(pplayer, leader_tile, 3,
                            !has_handicap(pplayer, H_MAP), &(wld.map));
  worst_danger = NULL;
  best_move_cost = FC_INFINITY;

  players_iterate(other_player) {
    if (other_player == pplayer) {
      continue;
    }

    unit_list_iterate(other_player->units, punit) {
      move_cost = pf_reverse_map_unit_move_cost(pfrm, punit);
      if (PF_IMPOSSIBLE_MC != move_cost && move_cost < best_move_cost) {
        best_move_cost = move_cost;
        worst_danger = punit;
      }
    } unit_list_iterate_end;
  } players_iterate_end;

  pf_reverse_map_destroy(pfrm);

  if (NULL == worst_danger) {
    unit_activity_handling(leader, ACTIVITY_IDLE);
    UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader: no close enemy.");
    return;
  }

  pft_fill_unit_parameter(&parameter, worst_danger);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  pfm = pf_map_new(&parameter);
  best_move_cost = pf_map_move_cost(pfm, leader_tile);

  /* Try to escape. */
  do {
    safest_tile = leader_tile;

    UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader: moves left: %d.",
             leader->moves_left);

    adjc_iterate(&(wld.map), leader_tile, near_tile) {
      if (adv_could_unit_move_to_tile(leader, near_tile) != 1) {
        continue;
      }

      move_cost = pf_map_move_cost(pfm, near_tile);
      if (PF_IMPOSSIBLE_MC != move_cost
          && move_cost > best_move_cost) {
        UNIT_LOG(LOG_DEBUG, leader,
                 "Barbarian leader: safest is (%d, %d), safeness %d",
                 TILE_XY(near_tile), best_move_cost);
        best_move_cost = move_cost;
        safest_tile = near_tile;
      }
    } adjc_iterate_end;

    UNIT_LOG(LOG_DEBUG, leader, "Barbarian leader: fleeing to (%d, %d).",
             TILE_XY(safest_tile));
    if (same_pos(unit_tile(leader), safest_tile)) {
      UNIT_LOG(LOG_DEBUG, leader, 
               "Barbarian leader: reached the safest position.");
      unit_activity_handling(leader, ACTIVITY_IDLE);
      pf_map_destroy(pfm);
      return;
    }

    alive = dai_unit_goto(ait, leader, safest_tile);
    if (alive) {
      if (same_pos(unit_tile(leader), leader_tile)) {
        /* Didn't move. No point to retry. */
        pf_map_destroy(pfm);
        return;
      }
      leader_tile = unit_tile(leader);
    }
  } while (alive && 0 < leader->moves_left);

  pf_map_destroy(pfm);
}

/**********************************************************************//**
  Are there dangerous enemies at or adjacent to the tile 'ptile'?

  Always override advisor danger detection since we are omniscient and
  advisor is not.
**************************************************************************/
void dai_consider_tile_dangerous(struct ai_type *ait, struct tile *ptile,
                                 struct unit *punit,
				 enum override_bool *result)
{
  int a = 0, d, db;
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(ptile);
  int extras_bonus = 0;

  if (is_human(pplayer)) {
    /* Use advisors code for humans. */
    return;
  }

  if (pcity && pplayers_allied(city_owner(pcity), unit_owner(punit))
      && !is_non_allied_unit_tile(ptile, pplayer)) {
    /* We will be safe in a friendly city */
    *result = OVERRIDE_FALSE;
    return;
  }

  /* Calculate how well we can defend at (x,y) */
  db = 10 + tile_terrain(ptile)->defense_bonus / 10;
  extras_bonus += tile_extras_defense_bonus(ptile, unit_type_get(punit));

  db += (db * extras_bonus) / 100;
  d = adv_unit_def_rating_basic_squared(punit) * db;

  adjc_iterate(&(wld.map), ptile, ptile1) {
    if (has_handicap(pplayer, H_FOG)
        && !map_is_known_and_seen(ptile1, unit_owner(punit), V_MAIN)) {
      /* We cannot see danger at (ptile1) => assume there is none */
      continue;
    }
    unit_list_iterate(ptile1->units, enemy) {
      if (pplayers_at_war(unit_owner(enemy), unit_owner(punit)) 
          && unit_attack_unit_at_tile_result(enemy, punit, ptile) == ATT_OK
          && unit_attack_units_at_tile_result(enemy, ptile) == ATT_OK) {
        a += adv_unit_att_rating(enemy);
        if ((a * a * 10) >= d) {
          /* The enemies combined strength is too big! */
          *result = OVERRIDE_TRUE;
          return;
        }
      }
    } unit_list_iterate_end;
  } adjc_iterate_end;

  *result = OVERRIDE_FALSE;
}

/**********************************************************************//**
  Updates the global array simple_ai_types.
**************************************************************************/
static void update_simple_ai_types(void)
{
  int i = 0;

  unit_type_iterate(punittype) {
    struct unit_class *pclass = utype_class(punittype);

    if (A_NEVER != punittype->require_advance
        && !utype_has_flag(punittype, UTYF_CIVILIAN)
        && !utype_can_do_action(punittype, ACTION_SUICIDE_ATTACK)
        && !(pclass->adv.land_move == MOVE_NONE
             && !can_attack_non_native(punittype))
        && !utype_fuel(punittype)
        && punittype->transport_capacity < 8) {
      simple_ai_types[i] = punittype;
      i++;
    }
  } unit_type_iterate_end;

  simple_ai_types[i] = NULL;
}

/**********************************************************************//**
  Initialise the unit data from the ruleset for the AI.
**************************************************************************/
void dai_units_ruleset_init(struct ai_type *ait)
{
  /* TODO: remove the simple_ai_types cache or merge it with a general ai
   *       cache; see the comment to struct unit_type *simple_ai_types at
   *       the beginning of this file. */
  update_simple_ai_types();

  unit_type_iterate(ptype) {
    struct unit_type_ai *utai = fc_malloc(sizeof(*utai));

    utai->firepower1 = FALSE;
    utai->ferry = FALSE;
    utai->missile_platform = FALSE;
    utai->carries_occupiers = FALSE;
    utai->potential_charges = unit_type_list_new();

    utype_set_ai_data(ptype, ait, utai);
  } unit_type_iterate_end;

  unit_type_iterate(punittype) {
    struct unit_class *pclass = utype_class(punittype);

    /* Confirm firepower */
    combat_bonus_list_iterate(punittype->bonuses, pbonus) {
      if (pbonus->type == CBONUS_FIREPOWER1) {
        unit_type_iterate(penemy) {
          if (utype_has_flag(penemy, pbonus->flag)) {
            struct unit_type_ai *utai = utype_ai_data(penemy, ait);

            utai->firepower1 = TRUE;
          }
        } unit_type_iterate_end;
      }
    } combat_bonus_list_iterate_end;

    /* Consider potential cargo */
    if (punittype->transport_capacity > 0) {
      struct unit_type_ai *utai = utype_ai_data(punittype, ait);

      unit_type_iterate(pctype) {
        struct unit_class *pcargo = utype_class(pctype);

        if (can_unit_type_transport(punittype, pcargo)) {
          if (utype_can_do_action(pctype, ACTION_SUICIDE_ATTACK)) {
            utai->missile_platform = TRUE;
          } else if (pclass->adv.sea_move != MOVE_NONE
              && pcargo->adv.land_move != MOVE_NONE) {
            if (pcargo->adv.sea_move != MOVE_FULL) {
              utai->ferry = TRUE;
            } else {
              if (0 != utype_fuel(pctype)) {
                utai->ferry = TRUE;
              }
            }
          }

          if (uclass_has_flag(pcargo, UCF_CAN_OCCUPY_CITY)) {
            utai->carries_occupiers = TRUE;
          }
        }
      } unit_type_iterate_end;
    }

    /* Consider potential charges */
    unit_type_iterate(pcharge) {
      bool can_move_like_charge = FALSE;

      if (0 < utype_fuel(punittype)
          && (0 == utype_fuel(pcharge)
              || utype_fuel(pcharge) > utype_fuel(punittype))) {
        continue;
      }

      unit_class_list_iterate(pclass->cache.subset_movers, chgcls) {
        if (chgcls == utype_class(pcharge)) {
          can_move_like_charge = TRUE;
        }
      } unit_class_list_iterate_end;

      if (can_move_like_charge) {
        struct unit_type_ai *utai = utype_ai_data(punittype, ait);
        unit_type_list_append(utai->potential_charges, pcharge);
      }

    } unit_type_iterate_end;
  } unit_type_iterate_end;
}

/**********************************************************************//**
  Close AI unit type data
**************************************************************************/
void dai_units_ruleset_close(struct ai_type *ait)
{
  unit_type_iterate(ptype) {
    struct unit_type_ai *utai = utype_ai_data(ptype, ait);

    if (utai == NULL) {
      continue;
    }
    utype_set_ai_data(ptype, ait, NULL);

    unit_type_list_destroy(utai->potential_charges);
    free(utai);
  } unit_type_iterate_end;
}

/**********************************************************************//**
  Initialize unit for use with default AI.
**************************************************************************/
void dai_unit_init(struct ai_type *ait, struct unit *punit)
{
  /* Make sure that contents of unit_ai structure are correctly initialized,
   * if you ever allocate it by some other mean than fc_calloc() */
  struct unit_ai *unit_data = fc_calloc(1, sizeof(struct unit_ai));

  unit_data->done = FALSE;
  unit_data->cur_pos = NULL;
  unit_data->prev_pos = NULL;
  unit_data->target = 0;
  BV_CLR_ALL(unit_data->hunted);
  unit_data->ferryboat = 0;
  unit_data->passenger = 0;
  unit_data->bodyguard = 0;
  unit_data->charge = 0;

  unit_set_ai_data(punit, ait, unit_data);
}

/**********************************************************************//**
  Free unit from use with default AI.
**************************************************************************/
void dai_unit_turn_end(struct ai_type *ait, struct unit *punit)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  fc_assert_ret(unit_data != NULL);

  BV_CLR_ALL(unit_data->hunted);
}

/**********************************************************************//**
  Free unit from use with default AI.
**************************************************************************/
void dai_unit_close(struct ai_type *ait, struct unit *punit)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  fc_assert_ret(unit_data != NULL);

  aiguard_clear_charge(ait, punit);
  aiguard_clear_guard(ait, punit);

  if (unit_data != NULL) {
    unit_set_ai_data(punit, ait, NULL);
    FC_FREE(unit_data);
  }
}

/**********************************************************************//**
  Save AI data of a unit.
**************************************************************************/
void dai_unit_save(struct ai_type *ait, const char *aitstr,
                   struct section_file *file,
                   const struct unit *punit, const char *unitstr)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  secfile_insert_int(file, unit_data->passenger, "%s.%spassenger",
                     unitstr, aitstr);
  secfile_insert_int(file, unit_data->ferryboat, "%s.%sferryboat",
                     unitstr, aitstr);
  secfile_insert_int(file, unit_data->charge, "%s.%scharge",
                     unitstr, aitstr);
  secfile_insert_int(file, unit_data->bodyguard, "%s.%sbodyguard",
                     unitstr, aitstr);
}

/**********************************************************************//**
  Load AI data of a unit.
**************************************************************************/
void dai_unit_load(struct ai_type *ait, const char *aitstr,
                   const struct section_file *file,
                   struct unit *punit, const char *unitstr)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  unit_data->passenger
    = secfile_lookup_int_default(file, 0, "%s.%spassenger",
                                 unitstr, aitstr);
  unit_data->ferryboat
    = secfile_lookup_int_default(file, 0, "%s.%sferryboat",
                                 unitstr, aitstr);
  unit_data->charge
    = secfile_lookup_int_default(file, 0, "%s.%scharge",
                                 unitstr, aitstr);
  unit_data->bodyguard
    = secfile_lookup_int_default(file, 0, "%s.%sbodyguard",
                                 unitstr, aitstr);
}

struct role_unit_cb_data
{
  enum terrain_class tc;
  struct city *build_city;
};

/**********************************************************************//**
  Filter callback for role unit iteration
**************************************************************************/
static bool role_unit_cb(struct unit_type *ptype, void *data)
{
  struct role_unit_cb_data *cb_data = (struct role_unit_cb_data *)data;
  struct unit_class *pclass = utype_class(ptype);

  if ((cb_data->tc == TC_LAND && pclass->adv.land_move == MOVE_NONE)
      || (cb_data->tc == TC_OCEAN && pclass->adv.sea_move == MOVE_NONE)) {
    return FALSE;
  }

  if (cb_data->build_city == NULL
      || can_city_build_unit_now(cb_data->build_city, ptype)) {
    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Get unit type player can build, suitable to role, with given move type.
**************************************************************************/
struct unit_type *dai_role_utype_for_terrain_class(struct city *pcity, int role,
                                                   enum terrain_class tc)
{
  struct role_unit_cb_data cb_data = { .build_city = pcity, .tc = tc };

  return role_units_iterate_backwards(role, role_unit_cb, &cb_data);
}

/**********************************************************************//**
  Returns whether 'attacker' can attack 'defender' immediately.
**************************************************************************/
bool dai_unit_can_strike_my_unit(const struct unit *attacker,
                                 const struct unit *defender)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  const struct tile *ptarget = unit_tile(defender);
  int max_move_cost = attacker->moves_left;
  bool able_to_strike = FALSE;

  pft_fill_unit_parameter(&parameter, attacker);
  parameter.omniscience = !has_handicap(unit_owner(defender), H_MAP);
  pfm = pf_map_new(&parameter);

  pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
    if (move_cost > max_move_cost) {
      break;
    }

    if (ptile == ptarget) {
      able_to_strike = TRUE;
      break;
    }
  } pf_map_move_costs_iterate_end;

  pf_map_destroy(pfm);

  return able_to_strike;
}

/**********************************************************************//**
  Switch to autoexploring.
**************************************************************************/
void dai_switch_to_explore(struct ai_type *ait, struct unit *punit,
                           struct tile *target, enum override_bool *allow)
{
  struct unit_ai *udata = def_ai_unit_data(punit, ait);

  if (udata->task != AIUNIT_NONE && udata->task != AIUNIT_EXPLORE) {
    *allow = OVERRIDE_FALSE;

    return;
  }
}
