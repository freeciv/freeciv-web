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
#include "rand.h"
#include "log.h"

/* common */
#include "base.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"

#include "combat.h"

/*******************************************************************//**
  Checks if player is restricted diplomatically from attacking the tile.
  Returns FALSE if
  1) the tile is empty or
  2) the tile contains a non-enemy city or
  3) the tile contains a non-enemy unit
***********************************************************************/
static bool can_player_attack_tile(const struct player *pplayer,
			    const struct tile *ptile)
{
  struct city *pcity = tile_city(ptile);
  
  /* 1. Is there anyone there at all? */
  if (!pcity && unit_list_size((ptile->units)) == 0) {
    return FALSE;
  }

  /* 2. If there is a city there, can we attack it? */
  if (pcity && !pplayers_at_war(city_owner(pcity), pplayer)) {
    return FALSE;
  }

  /* 3. Are we allowed to attack _all_ units there? */
  unit_list_iterate(ptile->units, aunit) {
    if (!pplayers_at_war(unit_owner(aunit), pplayer)) {
      /* Enemy hiding behind a human/diplomatic shield */
      return FALSE;
    }
  } unit_list_iterate_end;

  return TRUE;
}

/*******************************************************************//**
  Can unit attack other
***********************************************************************/
static bool is_unit_reachable_by_unit(const struct unit *defender,
                                      const struct unit *attacker)
{
  struct unit_class *dclass = unit_class_get(defender);
  struct unit_type *atype = unit_type_get(attacker);

  return BV_ISSET(atype->targets, uclass_index(dclass));
}

/*******************************************************************//**
  Can unit attack other at given location
***********************************************************************/
bool is_unit_reachable_at(const struct unit *defender,
                          const struct unit *attacker,
                          const struct tile *location)
{
  if (NULL != tile_city(location)) {
    return TRUE;
  }

  if (is_unit_reachable_by_unit(defender, attacker)) {
    return TRUE;
  }

  if (tile_has_native_base(location, unit_type_get(defender))) {
    return TRUE;
  }

  return FALSE;
}

/*******************************************************************//**
  Checks if a unit can physically attack pdefender at the tile
  (assuming it is adjacent and at war).

  Unit can NOT attack if:
  1) its unit type is unable to perform any attack action.
  2) it is not a fighter and defender is a flying unit (except city/airbase).
  3) it is a ground unit without marine ability and it attacks from ocean.
  4) it is a ground unit and it attacks a target on an ocean square or
     it is a sailing unit without shore bombardment capability and it
     attempts to attack land.

  Does NOT check:
  1) Moves left
  2) Adjacency
  3) Diplomatic status
***********************************************************************/
enum unit_attack_result unit_attack_unit_at_tile_result(const struct unit *punit,
                                                        const struct unit *pdefender,
                                                        const struct tile *dest_tile)
{
  /* 1. Can we attack _anything_ ? */
  if (!(utype_can_do_action(unit_type_get(punit), ACTION_ATTACK)
        || utype_can_do_action(unit_type_get(punit), ACTION_SUICIDE_ATTACK)
        /* Needed because ACTION_NUKE uses this when evaluating its hard
         * requirements. */
        || utype_can_do_action(unit_type_get(punit), ACTION_NUKE))) {
    return ATT_NON_ATTACK;
  }

  /* 2. Only fighters can attack planes, except in city or airbase attacks */
  if (!is_unit_reachable_at(pdefender, punit, dest_tile)) {
    return ATT_UNREACHABLE;
  }

  /* 3. Can't attack with ground unit from ocean, except for marines */
  if (!is_native_tile(unit_type_get(punit), unit_tile(punit))
      && !utype_can_do_act_when_ustate(unit_type_get(punit), ACTION_ATTACK,
                                       USP_NATIVE_TILE, FALSE)
      && !utype_can_do_act_when_ustate(unit_type_get(punit),
                                       ACTION_SUICIDE_ATTACK,
                                       USP_NATIVE_TILE, FALSE)) {
    return ATT_NONNATIVE_SRC;
  }

  /* 4. Most units can not attack non-native terrain.
   *    Most ships can attack land tiles (shore bombardment) */
  if (!is_native_tile(unit_type_get(punit), dest_tile)
      && !can_attack_non_native(unit_type_get(punit))) {
    return ATT_NONNATIVE_DST;
  }

  return ATT_OK;
}

/*******************************************************************//**
  When unreachable_protects setting is TRUE:
  To attack a stack, unit must be able to attack every unit there (not
  including transported units).
************************************************************************/
static enum unit_attack_result unit_attack_all_at_tile_result(const struct unit *punit,
                                                              const struct tile *ptile)
{
  unit_list_iterate(ptile->units, aunit) {
    /* HACK: we don't count transported units here.  This prevents some
     * bugs like a submarine carrying a cruise missile being invulnerable
     * to other sea units.  However from a gameplay perspective it's a hack,
     * since players can load and unload their units manually to protect
     * their transporters. */
    if (!unit_transported(aunit)) {
        enum unit_attack_result result;

        result = unit_attack_unit_at_tile_result(punit, aunit, ptile);
        if (result != ATT_OK) {
          return result;
        }
      }
  } unit_list_iterate_end;

  return ATT_OK;
}

/*******************************************************************//**
  When unreachable_protects setting is FALSE:
  To attack a stack, unit must be able to attack some unit there (not
  including transported units).
************************************************************************/
static enum unit_attack_result unit_attack_any_at_tile_result(const struct unit *punit,
                                                              const struct tile *ptile)
{
  enum unit_attack_result result = ATT_OK;

  unit_list_iterate(ptile->units, aunit) {
    /* HACK: we don't count transported units here.  This prevents some
     * bugs like a cargoplane carrying a land unit being vulnerable. */
    if (!unit_transported(aunit)) {
      result = unit_attack_unit_at_tile_result(punit, aunit, ptile);
      if (result == ATT_OK) {
        return result;
      }
    }
  } unit_list_iterate_end;

  /* That's result from check against last unit on tile, not first.
   * Shouldn't matter. */
  return result;
}

/*******************************************************************//**
  Check if unit can attack unit stack at tile.
***********************************************************************/
enum unit_attack_result unit_attack_units_at_tile_result(const struct unit *punit,
                                                         const struct tile *ptile)
{
  if (game.info.unreachable_protects) {
    return unit_attack_all_at_tile_result(punit, ptile);
  } else {
    return unit_attack_any_at_tile_result(punit, ptile);
  }
}

/*******************************************************************//**
  Is unit (1) diplomatically allowed to attack and (2) physically able
  to do so?
***********************************************************************/
bool can_unit_attack_tile(const struct unit *punit,
                          const struct tile *dest_tile)
{
  return (can_player_attack_tile(unit_owner(punit), dest_tile)
          && unit_attack_units_at_tile_result(punit, dest_tile) == ATT_OK);
}

/*******************************************************************//**
Returns the chance of the attacker winning, a number between 0 and 1.
If you want the chance that the defender wins just use 1-chance(...)

NOTE: this number can be _very_ small, fx in a battle between an
ironclad and a battleship the ironclad has less than 1/100000 chance of
winning.

The algoritm calculates the probability of each possible number of HP's
the attacker has left. Maybe that info should be preserved for use in
the AI.
***********************************************************************/
double win_chance(int as, int ahp, int afp, int ds, int dhp, int dfp)
{
  /* number of rounds a unit can fight without dying */
  int att_N_lose = (ahp + dfp - 1) / dfp;
  int def_N_lose = (dhp + afp - 1) / afp;
  /* Probability of losing one round */
  double att_P_lose1 = (as + ds == 0) ? 0.5 : (double) ds / (as + ds);
  double def_P_lose1 = 1 - att_P_lose1;

  /*
    This calculates 

    binomial_coeff(def_N_lose-1 + lr, lr)
      * def_P_lose1^(def_N_lose-1)
      * att_P_lose1^(lr)
      * def_P_lose1

    for each possible number of rounds lost (rl) by the winning unit.
    rl is of course less than the number of rounds the winning unit
    should lose to lose all it's hit points.
    The probabilities are then summed.

    To see this is correct consider the set of series for all valid fights.
    These series are the type (win, lose, lose...). The possible lenghts are
    def_N_lose to def_N_lose+att_N_lose-1. A series is not valid unless it
    contains def_N_lose wins, and one of the wins must be the last one, or
    the series would be equivalent the a shorter series (the attacker would
    have won one or more fights previously).
    So since the last fight is a win we disregard it while calculating. Now
    a series contains def_N_lose-1 wins. So for each possible lenght of a
    series we find the probability of every valid series and then sum.
    For a specific lenght (a "lr") every series have the probability
      def_P_lose1^(def_N_lose-1) * att_P_lose1^(lr)
    and then getting from that to the real series requires a win, ie factor
    def_N_lose. The number of series with lenght (def_N_lose-1 + lr) and
    "lr" lost fights is
      binomial_coeff(def_N_lose-1 + lr, lr)
    And by multiplying we get the formula on the top of this code block.
    Adding the cumulative probability for each valid lenght then gives the
    total probability.

    We clearly have all valid series this way. To see that we have counted
    none twice note that would require a series with a smaller series inbedded.
    But since the smaller series already included def_N_lose wins, and the
    larger series ends with a win, it would have too many wins and therefore
    cannot exist.

    In practice each binomial coefficient for a series lenght can be calculated
    from the previous. In the coefficient (n, k) n is increased and k is
    unchanged.
    The "* def_P_lose1" is multiplied on the sum afterwards.

    (lots of talk for so little code)
  */

  double binom_save = pow(def_P_lose1, (double)(def_N_lose - 1));
  double accum_prob = binom_save; /* lr = 0 */

  int lr; /* the number of Lost Rounds by the attacker */
  for (lr = 1; lr < att_N_lose; lr++) {
    /* update the coefficient */
    int n = lr + def_N_lose - 1;
    binom_save *= n;
    binom_save /= lr;
    binom_save *= att_P_lose1;
    /* use it for this lr */
    accum_prob += binom_save;
  }
  /* Every element of the sum needs a factor for the very last fight round */
  accum_prob *= def_P_lose1;

  return accum_prob;
}

/*******************************************************************//**
A unit's effective firepower depend on the situation.
***********************************************************************/
void get_modified_firepower(const struct unit *attacker,
			    const struct unit *defender,
			    int *att_fp, int *def_fp)
{
  struct city *pcity = tile_city(unit_tile(defender));

  *att_fp = unit_type_get(attacker)->firepower;
  *def_fp = unit_type_get(defender)->firepower;

  /* Check CityBuster flag */
  if (unit_has_type_flag(attacker, UTYF_CITYBUSTER) && pcity) {
    *att_fp *= 2;
  }

  /*
   * UTYF_BADWALLATTACKER sets the firepower of the attacking unit to 1 if
   * an EFT_DEFEND_BONUS applies (such as a land unit attacking a city with
   * city walls).
   */
  if (unit_has_type_flag(attacker, UTYF_BADWALLATTACKER)
      && get_unittype_bonus(unit_owner(defender), unit_tile(defender),
                            unit_type_get(attacker), EFT_DEFEND_BONUS) > 0) {
    *att_fp = 1;
  }

  /* pearl harbour - defender's firepower is reduced to one, 
   *                 attacker's is multiplied by two         */
  if (unit_has_type_flag(defender, UTYF_BADCITYDEFENDER)
      && tile_city(unit_tile(defender))) {
    *att_fp *= 2;
    *def_fp = 1;
  }
  
  /* 
   * When attacked by fighters, helicopters have their firepower
   * reduced to 1.
   */
  if (combat_bonus_against(unit_type_get(attacker)->bonuses,
                           unit_type_get(defender),
                           CBONUS_FIREPOWER1)) {
    *def_fp = 1;
  }

  /* In land bombardment both units have their firepower reduced to 1 */
  if (!is_native_tile(unit_type_get(attacker), unit_tile(defender))
      && !can_exist_at_tile(&(wld.map),
                            unit_type_get(defender), unit_tile(attacker))) {
    *att_fp = 1;
    *def_fp = 1;
  }
}

/*******************************************************************//**
Returns a double in the range [0;1] indicating the attackers chance of
winning. The calculation takes all factors into account.
***********************************************************************/
double unit_win_chance(const struct unit *attacker,
		       const struct unit *defender)
{
  int def_power = get_total_defense_power(attacker, defender);
  int att_power = get_total_attack_power(attacker, defender);

  double chance;

  int def_fp, att_fp;
  get_modified_firepower(attacker, defender, &att_fp, &def_fp);

  chance = win_chance(att_power, attacker->hp, att_fp,
		      def_power, defender->hp, def_fp);

  return chance;
}

/*******************************************************************//**
  Try defending against nuclear attack; if successful, return a city which 
  had enough luck and EFT_NUKE_PROOF.
  If the attack was successful return NULL.
***********************************************************************/
struct city *sdi_try_defend(const struct player *owner,
                            const struct tile *ptile)
{
  square_iterate(&(wld.map), ptile, 2, ptile1) {
    struct city *pcity = tile_city(ptile1);

    if (pcity
        && fc_rand(100) < get_target_bonus_effects(NULL,
                                                   city_owner(pcity), owner,
                                                   pcity, NULL, ptile,
                                                   NULL, NULL,
                                                   NULL, NULL, NULL,
                                                   EFT_NUKE_PROOF)) {
      return pcity;
    }
  } square_iterate_end;

  return NULL;
}

/*******************************************************************//**
  Convenience wrapper for base_get_attack_power.
***********************************************************************/
int get_attack_power(const struct unit *punit)
{
  return base_get_attack_power(unit_type_get(punit), punit->veteran,
                               punit->moves_left);
}

/*******************************************************************//**
  Returns the attack power, modified by moves left, and veteran
  status.
***********************************************************************/
int base_get_attack_power(const struct unit_type *punittype,
                          int veteran, int moves_left)
{
  int power;
  const struct veteran_level *vlevel;

  fc_assert_ret_val(punittype != NULL, 0);

  vlevel = utype_veteran_level(punittype, veteran);
  fc_assert_ret_val(vlevel != NULL, 0);

  power = punittype->attack_strength * POWER_FACTOR
          * vlevel->power_fact / 100;

  if (game.info.tired_attack && moves_left < SINGLE_MOVE) {
    power = (power * moves_left) / SINGLE_MOVE;
  }

  return power;
}

/*******************************************************************//**
  Returns the defense power, modified by veteran status.
***********************************************************************/
int base_get_defense_power(const struct unit *punit)
{
  const struct veteran_level *vlevel;

  fc_assert_ret_val(punit != NULL, 0);

  vlevel = utype_veteran_level(unit_type_get(punit), punit->veteran);
  fc_assert_ret_val(vlevel != NULL, 0);

  return unit_type_get(punit)->defense_strength * POWER_FACTOR
         * vlevel->power_fact / 100;
}

/*******************************************************************//**
  Returns the defense power, modified by terrain and veteran status.
  Note that rivers as special road types are not handled here as
  terrain property.
***********************************************************************/
static int get_defense_power(const struct unit *punit)
{
  int db, power = base_get_defense_power(punit);
  struct tile *ptile = unit_tile(punit);
  struct unit_class *pclass = unit_class_get(punit);

  if (uclass_has_flag(pclass, UCF_TERRAIN_DEFENSE)) {
    db = 100 + tile_terrain(ptile)->defense_bonus;
    power = (power * db) / 100;
  }

  if (!is_native_tile_to_class(pclass, ptile)) {
    power = power * pclass->non_native_def_pct / 100;
  }

  return power;
}

/*******************************************************************//**
  Return the modified attack power of a unit.
***********************************************************************/
int get_total_attack_power(const struct unit *attacker,
                           const struct unit *defender)
{
  int mod;
  int attackpower = get_attack_power(attacker);

  mod = 100 + get_unittype_bonus(unit_owner(attacker), unit_tile(defender),
                                 unit_type_get(attacker), EFT_ATTACK_BONUS);

  return attackpower * mod / 100;
}

/*******************************************************************//**
 Return an increased defensepower. Effects which increase the
 defensepower are:
  - unit type effects (horse vs pikemen for example)
  - defender in a fortress
  - fortified defender

May be called with a non-existing att_type to avoid any unit type
effects.
***********************************************************************/
static int defense_multiplication(const struct unit_type *att_type,
                                  const struct unit_type *def_type,
                                  const struct player *def_player,
                                  const struct tile *ptile,
                                  int defensepower, bool fortified)
{
  struct city *pcity = tile_city(ptile);
  int mod;

  fc_assert_ret_val(NULL != def_type, 0);

  if (NULL != att_type) {
    int defense_divider;
    int defense_multiplier = 1 + def_type->cache.defense_mp_bonuses[utype_index(att_type)];

    defensepower *= defense_multiplier;

    /* This applies even if pcity is NULL. */
    mod = 100 + get_unittype_bonus(def_player, ptile,
                                   att_type, EFT_DEFEND_BONUS);
    defensepower = MAX(0, defensepower * mod / 100);

    defense_divider = 1 + combat_bonus_against(att_type->bonuses, def_type,
                                               CBONUS_DEFENSE_DIVIDER);
    defensepower /= defense_divider;
  }

  defensepower +=
    defensepower * tile_extras_defense_bonus(ptile, def_type) / 100;

  if ((pcity || fortified)
      && uclass_has_flag(utype_class(def_type), UCF_CAN_FORTIFY)
      && !utype_has_flag(def_type, UTYF_CANT_FORTIFY)) {
    defensepower = (defensepower * 3) / 2;
  }

  return defensepower;
}

/*******************************************************************//**
 May be called with a non-existing att_type to avoid any effects which
 depend on the attacker.
***********************************************************************/
int get_virtual_defense_power(const struct unit_type *att_type,
			      const struct unit_type *def_type,
			      const struct player *def_player,
			      const struct tile *ptile,
			      bool fortified, int veteran)
{
  int defensepower = def_type->defense_strength;
  int db;
  const struct veteran_level *vlevel;
  struct unit_class *defclass;

  fc_assert_ret_val(def_type != NULL, 0);

  if (!can_exist_at_tile(&(wld.map), def_type, ptile)) {
    /* Ground units on ship doesn't defend. */
    return 0;
  }

  vlevel = utype_veteran_level(def_type, veteran);
  fc_assert_ret_val(vlevel != NULL, 0);

  defclass = utype_class(def_type);

  db = POWER_FACTOR;
  if (uclass_has_flag(defclass, UCF_TERRAIN_DEFENSE)) {
    db += tile_terrain(ptile)->defense_bonus / (100 / POWER_FACTOR);
  }
  defensepower *= db;
  defensepower *= vlevel->power_fact / 100;
  if (!is_native_tile_to_class(defclass, ptile)) {
    defensepower = defensepower * defclass->non_native_def_pct / 100;
  }

  return defense_multiplication(att_type, def_type, def_player,
                                ptile, defensepower,
                                fortified);
}

/*******************************************************************//**
 return the modified defense power of a unit.
 An veteran aegis cruiser in a mountain city with SAM and SDI defense 
 being attacked by a missile gets defense 288.
***********************************************************************/
int get_total_defense_power(const struct unit *attacker,
			    const struct unit *defender)
{
  return defense_multiplication(unit_type_get(attacker),
                                unit_type_get(defender),
                                unit_owner(defender), unit_tile(defender),
                                get_defense_power(defender),
                                defender->activity == ACTIVITY_FORTIFIED);
}

/*******************************************************************//**
  Return total defense power of the unit if it fortifies, if possible,
  where it is. attacker might be NULL to skip calculating attacker specific
  bonuses.
***********************************************************************/
int get_fortified_defense_power(const struct unit *attacker,
                                const struct unit *defender)
{
  struct unit_type *att_type = NULL;

  if (attacker != NULL) {
    att_type = unit_type_get(attacker);
  }

  return defense_multiplication(att_type, unit_type_get(defender),
                                unit_owner(defender), unit_tile(defender),
                                get_defense_power(defender),
                                TRUE);
}

/*******************************************************************//**
A number indicating the defense strength.
Unlike the one got from win chance this doesn't potentially get insanely
small if the units are unevenly matched, unlike win_chance.
***********************************************************************/
static int get_defense_rating(const struct unit *attacker,
			      const struct unit *defender)
{
  int afp, dfp;

  int rating = get_total_defense_power(attacker, defender);
  get_modified_firepower(attacker, defender, &afp, &dfp);

  /* How many rounds the defender will last */
  rating *= (defender->hp + afp-1)/afp;

  rating *= dfp;

  return rating;
}

/*******************************************************************//**
  Finds the best defender on the tile, given an attacker.  The diplomatic
  relationship of attacker and defender is ignored; the caller should check
  this.
***********************************************************************/
struct unit *get_defender(const struct unit *attacker,
			  const struct tile *ptile)
{
  struct unit *bestdef = NULL;
  int bestvalue = -99, best_cost = 0, rating_of_best = 0;

  /* Simply call win_chance with all the possible defenders in turn, and
   * take the best one.  It currently uses build cost as a tiebreaker in
   * case 2 units are identical, but this is crude as build cost does not
   * neccesarily have anything to do with the value of a unit.  This function
   * could be improved to take the value of the unit into account.  It would
   * also be nice if the function was a bit more fuzzy about prioritizing,
   * making it able to fx choose a 1a/9d unit over a 10a/10d unit. It should
   * also be able to spare units without full hp's to some extent, as these
   * could be more valuable later. */
  unit_list_iterate(ptile->units, defender) {
    /* We used to skip over allied units, but the logic for that is
     * complicated and is now handled elsewhere. */
    if (unit_can_defend_here(&(wld.map), defender)
        && unit_attack_unit_at_tile_result(attacker, defender, ptile) == ATT_OK) {
      bool change = FALSE;
      int build_cost = unit_build_shield_cost_base(defender);
      int defense_rating = get_defense_rating(attacker, defender);
      /* This will make units roughly evenly good defenders look alike. */
      int unit_def 
        = (int) (100000 * (1 - unit_win_chance(attacker, defender)));

      fc_assert_action(0 <= unit_def, continue);

      if (unit_has_type_flag(defender, UTYF_GAMELOSS)
          && !is_stack_vulnerable(unit_tile(defender))) {
        unit_def = -1; /* then always use leader as last defender. */
        /* FIXME: multiple gameloss units with varying defense value
         * not handled. */
      }

      if (unit_def > bestvalue) {
	change = TRUE;
      } else if (unit_def == bestvalue) {
	if (build_cost < best_cost) {
	  change = TRUE;
	} else if (build_cost == best_cost) {
	  if (rating_of_best < defense_rating) {	
	    change = TRUE;
	  }
	}
      }

      if (change) {
	bestvalue = unit_def;
	bestdef = defender;
	best_cost = build_cost;
	rating_of_best = defense_rating;
      }
    }
  } unit_list_iterate_end;

  return bestdef;
}

/*******************************************************************//**
  Get unit at (x, y) that wants to kill defender.

  Works like get_defender; see comment there.
  This function is mostly used by the AI.
***********************************************************************/
struct unit *get_attacker(const struct unit *defender,
                          const struct tile *ptile)
{
  struct unit *bestatt = 0;
  int bestvalue = -1, unit_a, best_cost = 0;

  unit_list_iterate(ptile->units, attacker) {
    int build_cost = unit_build_shield_cost_base(attacker);

    if (pplayers_allied(unit_owner(defender), unit_owner(attacker))) {
      return NULL;
    }
    unit_a = (int) (100000 * (unit_win_chance(attacker, defender)));
    if (unit_a > bestvalue
        || (unit_a == bestvalue && build_cost < best_cost)) {
      bestvalue = unit_a;
      bestatt = attacker;
      best_cost = build_cost;
    }
  } unit_list_iterate_end;

  return bestatt;
}

/*******************************************************************//**
  Is it a city/fortress/air base or will the whole stack die in an attack
***********************************************************************/
bool is_stack_vulnerable(const struct tile *ptile)
{
  return (game.info.killstack
          && !tile_has_extra_flag(ptile, EF_NO_STACK_DEATH)
          && NULL == tile_city(ptile));
}

/*******************************************************************//**
  Get bonus value against given unit type from bonus list.

  Consider using cached values instead of calling this recalculation
  directly.
***********************************************************************/
int combat_bonus_against(const struct combat_bonus_list *list,
                         const struct unit_type *enemy,
                         enum combat_bonus_type type)
{
  int value = 0;

  combat_bonus_list_iterate(list, pbonus) {
    if (pbonus->type == type && utype_has_flag(enemy, pbonus->flag)) {
      value += pbonus->value;
    }
  } combat_bonus_list_iterate_end;

  return value;
}
