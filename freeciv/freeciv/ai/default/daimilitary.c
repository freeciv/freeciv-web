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

#include <string.h>

/* utility */
#include "log.h"

/* common */
#include "combat.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "research.h"
#include "specialist.h"
#include "unitlist.h"

/* common/aicore */
#include "pf_tools.h"

/* server */
#include "citytools.h"
#include "cityturn.h"
#include "srv_log.h"
#include "srv_main.h"

/* server/advisors */
#include "advbuilding.h"
#include "advchoice.h"
#include "advdata.h"
#include "advtools.h"
#include "autosettlers.h"
#include "infracache.h" /* adv_city */

/* ai */
#include "difficulty.h"
#include "handicaps.h"

/* ai/default */
#include "aiair.h"
#include "aicity.h"
#include "aidata.h"
#include "aidiplomat.h"
#include "aiferry.h"
#include "aihand.h"
#include "aihunt.h"
#include "ailog.h"
#include "aiparatrooper.h"
#include "aiplayer.h"
#include "aitech.h"
#include "aitools.h"
#include "aiunit.h"
#include "daieffects.h"

#include "daimilitary.h"

static unsigned int assess_danger(struct ai_type *ait, struct city *pcity,
                                  const struct civ_map *dmap,
                                  player_unit_list_getter ul_cb);

/**********************************************************************//**
  Choose the best unit the city can build to defend against attacker v.
**************************************************************************/
struct unit_type *dai_choose_defender_versus(struct city *pcity,
                                             struct unit *attacker)
{
  struct unit_type *bestunit = NULL;
  double best = 0;
  int best_cost = FC_INFINITY;
  struct player *pplayer = city_owner(pcity);

  simple_ai_unit_type_iterate(punittype) {
    if (can_city_build_unit_now(pcity, punittype)) {
      int fpatt, fpdef, defense, attack;
      double want, loss, cost = utype_build_shield_cost(pcity, punittype);
      struct unit *defender;
      int veteran = get_unittype_bonus(city_owner(pcity), pcity->tile, punittype,
                                       EFT_VETERAN_BUILD);

      defender = unit_virtual_create(pplayer, pcity, punittype, veteran);
      defense = get_total_defense_power(attacker, defender);
      attack = get_total_attack_power(attacker, defender);
      get_modified_firepower(attacker, defender, &fpatt, &fpdef);

      /* Greg's algorithm. loss is the average number of health lost by
       * defender. If loss > attacker's hp then we should win the fight,
       * which is always a good thing, since we avoid shield loss. */
      loss = (double) defense * punittype->hp * fpdef / (attack * fpatt);
      want = (loss + MAX(0, loss - attacker->hp)) / cost;

#ifdef NEVER
      CITY_LOG(LOG_DEBUG, pcity, "desire for %s against %s(%d,%d) is %.2f",
               unit_name_orig(punittype),
               unit_name_orig(unit_type_get(attacker)), 
               TILE_XY(attacker->tile), want);
#endif /* NEVER */

      if (want > best || (want == best && cost <= best_cost)) {
        best = want;
        bestunit = punittype;
        best_cost = cost;
      }
      unit_virtual_destroy(defender);
    }
  } simple_ai_unit_type_iterate_end;

  return bestunit;
}

/**********************************************************************//**
  This function should assign a value to choice and want, where want is a value
  between 1 and 100.

  If choice is A_UNSET, this advisor doesn't want any particular tech
  researched at the moment.
**************************************************************************/
void military_advisor_choose_tech(struct player *pplayer,
				  struct adv_choice *choice)
{
  /* This function hasn't been implemented yet. */
  adv_init_choice(choice);
}

/**********************************************************************//**
  Choose best attacker based on movement type. It chooses based on unit
  desirability without regard to cost, unless costs are equal. This is
  very wrong. FIXME, use amortize on time to build.
**************************************************************************/
static struct unit_type *dai_choose_attacker(struct ai_type *ait, struct city *pcity,
                                             enum terrain_class tc, bool allow_gold_upkeep)
{
  struct unit_type *bestid = NULL;
  int best = -1;
  int cur;
  struct player *pplayer = city_owner(pcity);

  simple_ai_unit_type_iterate(putype) {
    if (!allow_gold_upkeep && utype_upkeep_cost(putype, pplayer, O_GOLD) > 0) {
      continue;
    }

    cur = dai_unit_attack_desirability(ait, putype);
    if ((tc == TC_LAND && utype_class(putype)->adv.land_move != MOVE_NONE)
        || (tc == TC_OCEAN
            && utype_class(putype)->adv.sea_move != MOVE_NONE)) {
      if (can_city_build_unit_now(pcity, putype)
          && (cur > best
              || (cur == best
                  && utype_build_shield_cost(pcity, putype)
                    <= utype_build_shield_cost(pcity, bestid)))) {
        best = cur;
        bestid = putype;
      }
    }
  } simple_ai_unit_type_iterate_end;

  return bestid;
}

/**********************************************************************//**
  Choose best defender based on movement type. It chooses based on unit
  desirability without regard to cost, unless costs are equal. This is
  very wrong. FIXME, use amortize on time to build.

  We should only be passed with L_DEFEND_GOOD role for now, since this
  is the only role being considered worthy of bodyguarding in findjob.
**************************************************************************/
static struct unit_type *dai_choose_bodyguard(struct ai_type *ait,
                                              struct city *pcity,
                                              enum terrain_class tc,
                                              enum unit_role_id role,
                                              bool allow_gold_upkeep)
{
  struct unit_type *bestid = NULL;
  int best = 0;
  struct player *pplayer = city_owner(pcity);

  simple_ai_unit_type_iterate(putype) {
    /* Only consider units of given role, or any if invalid given */
    if (unit_role_id_is_valid(role)) {
      if (!utype_has_role(putype, role)) {
        continue;
      }
    }

    if (!allow_gold_upkeep && utype_upkeep_cost(putype, pplayer, O_GOLD) > 0) {
      continue;
    }

    /* Only consider units of same move type */
    if ((tc == TC_LAND && utype_class(putype)->adv.land_move == MOVE_NONE)
        || (tc == TC_OCEAN
            && utype_class(putype)->adv.sea_move == MOVE_NONE)) {
      continue;
    }

    /* Now find best */
    if (can_city_build_unit_now(pcity, putype)) {
      const int desire = dai_unit_defence_desirability(ait, putype);

      if (desire > best
	  || (desire == best && utype_build_shield_cost(pcity, putype) <=
	      utype_build_shield_cost(pcity, bestid))) {
        best = desire;
        bestid = putype;
      }
    }
  } simple_ai_unit_type_iterate_end;

  return bestid;
}

/**********************************************************************//**
  Helper for assess_defense_quadratic and assess_defense_unit.
**************************************************************************/
static int base_assess_defense_unit(struct city *pcity, struct unit *punit,
                                    bool igwall, bool quadratic,
                                    int wall_value)
{
  int defense;
  bool do_wall = FALSE;

  if (!is_military_unit(punit)) {
    return 0;
  }

  defense = get_fortified_defense_power(NULL, punit) * punit->hp;
  if (unit_has_type_flag(punit, UTYF_BADCITYDEFENDER)) {
    /* Attacker firepower doubled, defender firepower set to 1 */
    defense /= 2;
  } else {
    defense *= unit_type_get(punit)->firepower;
  }

  if (pcity) {
    /* FIXME: We check if city got defense effect against *some*
     * unit type. Sea unit danger might cause us to build defenses
     * against air units... */
    do_wall = (!igwall && city_got_defense_effect(pcity, NULL));
  }
  defense /= POWER_DIVIDER;

  if (quadratic) {
    defense *= defense;
  }

  if (do_wall) {
    defense *= wall_value;
    defense /= 10;
  }

  return defense;
}

/**********************************************************************//**
  Need positive feedback in m_a_c_b and bodyguard routines. -- Syela
**************************************************************************/
int assess_defense_quadratic(struct ai_type *ait, struct city *pcity)
{
  int defense = 0, walls = 0;
  /* This can be an arg if needed, but we don't need to change it now. */
  const bool igwall = FALSE;
  struct ai_city *city_data = def_ai_city_data(pcity, ait);

  /* wallvalue = 10, walls = 10,
   * wallvalue = 40, walls = 20,
   * wallvalue = 90, walls = 30 */

  while (walls * walls < city_data->wallvalue * 10) {
    walls++;
  }

  unit_list_iterate(pcity->tile->units, punit) {
    defense += base_assess_defense_unit(pcity, punit, igwall, FALSE,
                                        walls);
  } unit_list_iterate_end;

  if (defense > 1<<12) {
    CITY_LOG(LOG_VERBOSE, pcity, "Overflow danger in assess_defense_quadratic:"
             " %d", defense);
    if (defense > 1<<15) {
      defense = 1<<15; /* more defense than we know what to do with! */
    }
  }

  return defense * defense;
}

/**********************************************************************//**
  One unit only, mostly for findjob; handling boats correctly. 980803 -- Syela
**************************************************************************/
int assess_defense_unit(struct ai_type *ait, struct city *pcity,
                        struct unit *punit, bool igwall)
{
  return base_assess_defense_unit(pcity, punit, igwall, TRUE,
				  def_ai_city_data(pcity, ait)->wallvalue);
}

/**********************************************************************//**
  Most of the time we don't need/want positive feedback. -- Syela

  It's unclear whether this should treat settlers/caravans as defense. -- Syela
  TODO: It looks like this is never used while deciding if we should attack
  pcity, if we have pcity defended properly, so I think it should. --pasky
**************************************************************************/
static int assess_defense_backend(struct ai_type *ait, struct city *pcity,
                                  bool igwall)
{
  /* Estimate of our total city defensive might */
  int defense = 0;

  unit_list_iterate(pcity->tile->units, punit) {
    defense += assess_defense_unit(ait, pcity, punit, igwall);
  } unit_list_iterate_end;

  return defense;
}

/**********************************************************************//**
  Estimate defense strength of city
**************************************************************************/
int assess_defense(struct ai_type *ait, struct city *pcity)
{
  return assess_defense_backend(ait, pcity, FALSE);
}

/**********************************************************************//**
  Estimate defense strength of city without considering how buildings
  help defense
**************************************************************************/
static int assess_defense_igwall(struct ai_type *ait, struct city *pcity)
{
  return assess_defense_backend(ait, pcity, TRUE);
}

/**********************************************************************//**
  How dangerous and far a unit is for a city?
**************************************************************************/
static unsigned int assess_danger_unit(const struct city *pcity,
                                       struct pf_reverse_map *pcity_map,
                                       const struct unit *punit,
                                       int *move_time)
{
  struct pf_position pos;
  const struct unit_type *punittype = unit_type_get(punit);
  const struct tile *ptile = city_tile(pcity);
  const struct unit *ferry;
  unsigned int danger;
  int mod;

  *move_time = PF_IMPOSSIBLE_MC;

  if (utype_can_do_action(punittype, ACTION_PARADROP)
      && 0 < punittype->paratroopers_range) {
    *move_time = (real_map_distance(ptile, unit_tile(punit))
                  / punittype->paratroopers_range);
  }

  if (pf_reverse_map_unit_position(pcity_map, punit, &pos)
      && (PF_IMPOSSIBLE_MC == *move_time
          || *move_time > pos.turn)) {
    *move_time = pos.turn;
  }

  if (unit_transported(punit)
      && (ferry = unit_transport_get(punit))
      && pf_reverse_map_unit_position(pcity_map, ferry, &pos)) {
    if ((PF_IMPOSSIBLE_MC == *move_time
         || *move_time > pos.turn)) {
      *move_time = pos.turn;
      if (!can_attack_from_non_native(punittype)) {
        (*move_time)++;
      }
    }
  }

  if (PF_IMPOSSIBLE_MC == *move_time) {
    return 0;
  }
  if (!is_native_tile(punittype, ptile)
      && !can_attack_non_native(punittype)) {
    return 0;
  }
  if (!is_native_near_tile(&(wld.map), unit_class_get(punit), ptile)) {
    return 0;
  }

  danger = adv_unit_att_rating(punit);
  mod = 100 + get_unittype_bonus(city_owner(pcity), ptile,
                                 punittype, EFT_DEFEND_BONUS);
  return danger * 100 / MAX(mod, 1);
}

/**********************************************************************//**
  Call assess_danger() for all cities owned by pplayer.

  This is necessary to initialize some ai data before some ai calculations.
**************************************************************************/
void dai_assess_danger_player(struct ai_type *ait, struct player *pplayer,
                              const struct civ_map *dmap)
{
  /* Do nothing if game is not running */
  if (S_S_RUNNING == server_state()) {
    city_list_iterate(pplayer->cities, pcity) {
      (void) assess_danger(ait, pcity, dmap, NULL);
    } city_list_iterate_end;
  }
}

/**********************************************************************//**
  Set (overwrite) our want for a building. Syela tries to explain:

    My first attempt to allow ng_wa >= 200 led to stupidity in cities
    with no defenders and danger = 0 but danger > 0.  Capping ng_wa at
    100 + urgency led to a failure to buy walls as required.  Allowing
    want > 100 with !urgency led to the AI spending too much gold and
    falling behind on science.  I'm trying again, but this will require
    yet more tedious observation -- Syela

  The idea in this horrible function is that there is an enemy nearby
  that can whack us, so let's build something that can defend against
  him. If danger is urgent and overwhelming, danger is 200+, if it is
  only overwhelming, set it depending on danger. If it is underwhelming,
  set it to 100 plus urgency.

  This algorithm is very strange. But I created it by nesting up
  Syela's convoluted if ... else logic, and it seems to work. -- Per
**************************************************************************/
static void dai_reevaluate_building(struct city *pcity, adv_want *value, 
                                    unsigned int urgency, unsigned int danger, 
                                    int defense)
{
  if (*value == 0 || danger <= 0) {
    return;
  }

  *value = MAX(*value, 100 + MAX(0, urgency)); /* default */

  if (urgency > 0 && danger > defense * 2) {
    *value += 100;
  } else if (defense != 0 && danger > defense) {
    *value = MAX(danger * 100 / defense, *value);
  }
}

/**********************************************************************//**
  Create cached information about danger, urgency and grave danger to our
  cities.

  Danger is a weight on how much power enemy units nearby have, which is
  compared to our defence.

  Urgency is the number of hostile units that can attack us in three turns.

  Grave danger is number of units that can attack us next turn.

  FIXME: We do not consider a paratrooper's mr_req and mr_sub
  fields. Not a big deal, though.

  FIXME: Due to the nature of assess_distance, a city will only be
  afraid of a boat laden with enemies if it stands on the coast (i.e.
  is directly reachable by this boat).
**************************************************************************/
static unsigned int assess_danger(struct ai_type *ait, struct city *pcity,
                                  const struct civ_map *dmap,
                                  player_unit_list_getter ul_cb)
{
  struct player *pplayer = city_owner(pcity);
  struct tile *ptile = city_tile(pcity);
  struct ai_city *city_data = def_ai_city_data(pcity, ait);
  unsigned int danger_reduced[B_LAST]; /* How much such danger there is that
                                        * building would help against. */
  int i;
  int defender;
  unsigned int urgency = 0;
  int defense;
  int total_danger = 0;
  int defense_bonuses[U_LAST];
  bool defender_type_handled[U_LAST];
  int assess_turns;
  bool omnimap;

  TIMING_LOG(AIT_DANGER, TIMER_START);

  /* Initialize data. */
  memset(&danger_reduced, 0, sizeof(danger_reduced));
  if (has_handicap(pplayer, H_DANGER)) {
    /* Always thinks that city is in grave danger */
    city_data->grave_danger = 1;
  } else {
    city_data->grave_danger = 0;
  }
  city_data->diplomat_threat = FALSE;
  city_data->has_diplomat = FALSE;

  unit_type_iterate(utype) {
    defense_bonuses[utype_index(utype)] = 0;
    defender_type_handled[utype_index(utype)] = FALSE;
  } unit_type_iterate_end;

  unit_list_iterate(ptile->units, punit) {
    struct unit_type *def = unit_type_get(punit);

    if (unit_has_type_flag(punit, UTYF_DIPLOMAT)) {
      city_data->has_diplomat = TRUE;
    }
    if (!defender_type_handled[utype_index(def)]) {
      /* This is first defender of this type. Check defender type
       * specific bonuses. */

      /* Skip defenders that have no bonuses at all. Acceptable
       * side-effect is that we can't consider negative bonuses at
       * all ("No bonuses" should be better than "negative bonus") */
      if (def->cache.max_defense_mp > 0) {
        unit_type_iterate(utype) {
          int idx = utype_index(utype);

          if (def->cache.defense_mp_bonuses[idx] > defense_bonuses[idx]) {
            defense_bonuses[idx] = def->cache.defense_mp_bonuses[idx];
          }
        } unit_type_iterate_end;
      }

      defender_type_handled[utype_index(def)] = TRUE;
    }
  } unit_list_iterate_end;

  if (player_is_cpuhog(pplayer)) {
    assess_turns = 6;
  } else {
    assess_turns = 3;
#ifdef FREECIV_WEB
    assess_turns = has_handicap(pplayer, H_ASSESS_DANGER_LIMITED) ? 2 : 3;
#endif
  }

  omnimap = !has_handicap(pplayer, H_MAP);

  /* Check. */
  players_iterate(aplayer) {
    struct pf_reverse_map *pcity_map;
    struct unit_list *units;

    if (!adv_is_player_dangerous(pplayer, aplayer)) {
      continue;
    }
    /* Note that we still consider the units of players we are not (yet)
     * at war with. */

    pcity_map = pf_reverse_map_new_for_city(pcity, aplayer, assess_turns,
                                            omnimap, dmap);

    if (ul_cb != NULL) {
      units = ul_cb(aplayer);
    } else {
      units = aplayer->units;
    }
    unit_list_iterate(units, punit) {
      int move_time;
      unsigned int vulnerability;
      int defbonus;
      struct unit_type *utype = unit_type_get(punit);
      struct unit_type_ai *utai = utype_ai_data(utype, ait);

#ifdef FREECIV_WEB
      int unit_distance = real_map_distance(ptile, unit_tile(punit));
      if (unit_distance > ASSESS_DANGER_MAX_DISTANCE
          || (has_handicap(pplayer, H_ASSESS_DANGER_LIMITED)
              && unit_distance > AI_HANDICAP_DISTANCE_LIMIT)) {
        /* Too far away. */
        continue;
      }
#endif

      if (!utai->carries_occupiers
          && !utype_acts_hostile(utype)
          && (utype_has_flag(utype, UTYF_CIVILIAN)
              || (!utype_can_do_action(utype, ACTION_ATTACK)
                  && !utype_can_do_action(utype, ACTION_SUICIDE_ATTACK)
                  && !utype_can_take_over(utype)))) {
        /* Harmless unit. */
        continue;
      }

      vulnerability = assess_danger_unit(pcity, pcity_map,
                                         punit, &move_time);

      if (PF_IMPOSSIBLE_MC == move_time) {
        continue;
      }

      if ((0 < vulnerability && unit_can_take_over(punit))
          || utai->carries_occupiers) {
        if (3 >= move_time) {
          urgency++;
          if (1 >= move_time) {
            city_data->grave_danger++;
          }
        }
      }

      defbonus = defense_bonuses[utype_index(utype)];
      if (defbonus > 1) {
        defbonus = (defbonus + 1) / 2;
      }
      vulnerability /= (defbonus + 1);
      (void) dai_wants_defender_against(ait, pplayer, pcity, utype,
                                        vulnerability / MAX(move_time, 1));

      if (utype_acts_hostile(unit_type_get(punit)) && 2 >= move_time) {
        city_data->diplomat_threat = TRUE;
      }

      vulnerability *= vulnerability; /* positive feedback */
      if (1 < move_time) {
        vulnerability /= move_time;
      }

      if (unit_can_do_action(punit, ACTION_NUKE)) {
        defender = dai_find_source_building(pcity, EFT_NUKE_PROOF,
                                            unit_type_get(punit));
        if (defender != B_LAST) {
          danger_reduced[defender] += vulnerability / MAX(move_time, 1);
        }
      } else {
        defender = dai_find_source_building(pcity, EFT_DEFEND_BONUS,
                                            unit_type_get(punit));
        if (defender != B_LAST) {
          danger_reduced[defender] += vulnerability / MAX(move_time, 1);
        }
      }

      total_danger += vulnerability;
    } unit_list_iterate_end;

    pf_reverse_map_destroy(pcity_map);

  } players_iterate_end;

  if (total_danger) {
    city_data->wallvalue = 90;
  } else {
    /* No danger.
     * This is half of the wallvalue of what danger 1 would produce. */
    city_data->wallvalue = 5;
  }

  if (0 < city_data->grave_danger) {
    /* really, REALLY urgent to defend */
    urgency += 10 * city_data->grave_danger;
  }

  /* HACK: This needs changing if multiple improvements provide
   * this effect. */
  /* FIXME: Accept only buildings helping unit classes we actually use.
   *        Now we consider any land mover helper suitable. */
  defense = assess_defense_igwall(ait, pcity);

  for (i = 0; i < B_LAST; i++) {
    if (0 < danger_reduced[i]) {
      dai_reevaluate_building(pcity, &pcity->server.adv->building_want[i],
                              urgency, danger_reduced[i], defense);
    }
  }

  if (has_handicap(pplayer, H_DANGER) && 0 == total_danger) {
    /* Has to have some danger
     * Otherwise grave_danger will be ignored. */
    city_data->danger = 1;
  } else {
    city_data->danger = total_danger;
  }
  city_data->urgency = urgency;

  TIMING_LOG(AIT_DANGER, TIMER_STOP);

  return urgency;
}

/**********************************************************************//**
  How much we would want that unit to defend a city? (Do not use this
  function to find bodyguards for ships or air units.)
**************************************************************************/
int dai_unit_defence_desirability(struct ai_type *ait,
                                  const struct unit_type *punittype)
{
  int desire = punittype->hp;
  int attack = punittype->attack_strength;
  int defense = punittype->defense_strength;
  int maxbonus = 0;

  /* Sea and helicopters often have their firepower set to 1 when
   * defending. We can't have such units as defenders. */
  if (!utype_has_flag(punittype, UTYF_BADCITYDEFENDER)
      && !((struct unit_type_ai *)utype_ai_data(punittype, ait))->firepower1) {
    /* Sea units get 1 firepower in Pearl Harbour,
     * and helicopters very bad against fighters */
    desire *= punittype->firepower;
  }
  desire *= defense;
  desire += punittype->move_rate / SINGLE_MOVE;
  desire += attack;

  maxbonus = punittype->cache.max_defense_mp;
  if (maxbonus > 1) {
    maxbonus = (maxbonus + 1) / 2;
  }
  desire += desire * maxbonus; 
  if (utype_has_flag(punittype, UTYF_GAMELOSS)) {
    desire /= 10; /* but might actually be worth it */
  }

  return desire;
}

/**********************************************************************//**
  How much we would want that unit to attack with?
**************************************************************************/
int dai_unit_attack_desirability(struct ai_type *ait,
                                 const struct unit_type *punittype)
{
  int desire = punittype->hp;
  int attack = punittype->attack_strength;
  int defense = punittype->defense_strength;

  desire *= punittype->move_rate;
  desire *= punittype->firepower;
  desire *= attack;
  desire += defense;
  if (utype_has_flag(punittype, UTYF_IGTER)) {
    desire += desire / 2;
  }
  if (utype_has_flag(punittype, UTYF_GAMELOSS)) {
    desire /= 10; /* but might actually be worth it */
  }
  if (utype_has_flag(punittype, UTYF_CITYBUSTER)) {
    desire += desire / 2;
  }
  if (can_attack_from_non_native(punittype)) {
    desire += desire / 4;
  }
  if (punittype->adv.igwall) {
    desire += desire / 4;
  }
  return desire;
}

/**********************************************************************//**
  What would be the best defender for that city? Records the best defender
  type in choice. Also sets the technology want for the units we can't
  build yet.
**************************************************************************/
bool dai_process_defender_want(struct ai_type *ait, struct player *pplayer,
                               struct city *pcity, unsigned int danger,
                               struct adv_choice *choice)
{
  const struct research *presearch = research_get(pplayer);
  /* FIXME: We check if the city has *some* defensive structure,
   * but not whether the city has a defensive structure against
   * any specific attacker.  The actual danger may not be mitigated
   * by the defense selected... */
  bool walls = city_got_defense_effect(pcity, NULL);
  /* Technologies we would like to have. */
  int tech_desire[U_LAST];
  /* Our favourite unit. */
  int best = -1;
  struct unit_type *best_unit_type = NULL;
  int best_unit_cost = 1;
  struct ai_city *city_data = def_ai_city_data(pcity, ait);
  struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

  memset(tech_desire, 0, sizeof(tech_desire));

  simple_ai_unit_type_iterate(punittype) {
    int desire; /* How much we want the unit? */

    /* Only consider proper defenders - otherwise waste CPU and
     * bump tech want needlessly. */
    if (!utype_has_role(punittype, L_DEFEND_GOOD)
	&& !utype_has_role(punittype, L_DEFEND_OK)) {
      continue;
    }

    desire = dai_unit_defence_desirability(ait, punittype);

    if (!utype_has_role(punittype, L_DEFEND_GOOD)) {
      desire /= 2; /* not good, just ok */
    }

    if (utype_has_flag(punittype, UTYF_FIELDUNIT)) {
      /* Causes unhappiness even when in defense, so not a good
       * idea for a defender, unless it is _really_ good */
      desire /= 2;
    }      

    desire /= POWER_DIVIDER/2; /* Good enough, no rounding errors. */
    desire *= desire;

    if (can_city_build_unit_now(pcity, punittype)) {
      /* We can build the unit now... */

      int build_cost = utype_build_shield_cost(pcity, punittype);
      int limit_cost = pcity->shield_stock + 40;

      if (walls && !utype_has_flag(punittype, UTYF_BADCITYDEFENDER)) {
	desire *= city_data->wallvalue;
	/* TODO: More use of POWER_FACTOR ! */
	desire /= POWER_FACTOR;
      }

      if ((best_unit_cost > limit_cost
           && build_cost < best_unit_cost)
          || ((desire > best ||
               (desire == best && build_cost <= best_unit_cost))
              && (best_unit_type == NULL
                  /* In case all units are more expensive than limit_cost */
                  || limit_cost <= pcity->shield_stock + 40))) {
	best = desire;
	best_unit_type = punittype;
	best_unit_cost = build_cost;
      }
    } else if (can_city_build_unit_later(pcity, punittype)) {
      /* We first need to develop the tech required by the unit... */

      /* Cost (shield equivalent) of gaining these techs. */
      /* FIXME? Katvrr advises that this should be weighted more heavily in
       * big danger. */
      int tech_cost = research_goal_bulbs_required(presearch,
                          advance_number(punittype->require_advance)) / 4
                          / city_list_size(pplayer->cities);

      /* Contrary to the above, we don't care if walls are actually built 
       * - we're looking into the future now. */
      if (!utype_has_flag(punittype, UTYF_BADCITYDEFENDER)) {
	desire *= city_data->wallvalue;
	desire /= POWER_FACTOR;
      }

      /* Yes, there's some similarity with kill_desire(). */
      /* TODO: Explain what shield cost has to do with tech want. */
      tech_desire[utype_index(punittype)] =
        (desire * danger / (utype_build_shield_cost(pcity, punittype) + tech_cost));
    }
  } simple_ai_unit_type_iterate_end;

  if (best == -1) {
    CITY_LOG(LOG_DEBUG, pcity, "Ooops - we cannot build any defender!");
  }

  if (best_unit_type) {
    if (!walls && !utype_has_flag(best_unit_type, UTYF_BADCITYDEFENDER)) {
      best *= city_data->wallvalue;
      best /= POWER_FACTOR;
    }
  } else {
    best_unit_cost = 100; /* Building impossible is considered costly.
                           * This should increase want for tech providing
                           * first defender type. */
  }

  if (best <= 0) best = 1; /* Avoid division by zero below. */

  /* Update tech_want for appropriate techs for units we want to build. */
  simple_ai_unit_type_iterate(punittype) {
    if (tech_desire[utype_index(punittype)] > 0) {
      /* TODO: Document or fix the algorithm below. I have no idea why
       * it is written this way, and the results seem strange to me. - Per */
      int desire = tech_desire[utype_index(punittype)] * best_unit_cost / best;

      plr_data->tech_want[advance_index(punittype->require_advance)]
        += desire;
      TECH_LOG(ait, LOG_DEBUG, pplayer, punittype->require_advance,
               "+ %d for %s to defend %s",
               desire,
               utype_rule_name(punittype),
               city_name_get(pcity));
    }
  } simple_ai_unit_type_iterate_end;

  if (!best_unit_type) {
    return FALSE;
  }

  choice->value.utype = best_unit_type;
  choice->want = danger;
  choice->type = CT_DEFENDER;
  return TRUE;
}

/**********************************************************************//**
  This function decides, what unit would be best for erasing enemy. It is
  called, when we just want to kill something, we've found it but we don't
  have the unit for killing that built yet - here we'll choose the type of
  that unit.

  We will also set increase the technology want to get units which could
  perform the job better.

  I decided this funct wasn't confusing enough, so I made
  kill_something_with() send it some more variables for it to meddle with.
  -- Syela

  'ptile' is location of the target.
  best_choice is pre-filled with our current choice, we only
  consider units who can move in all the same terrains for best_choice.
**************************************************************************/
static void process_attacker_want(struct ai_type *ait,
                                  struct city *pcity,
                                  int value,
                                  struct unit_type *victim_unit_type,
                                  struct player *victim_player,
                                  int veteran, struct tile *ptile,
                                  struct adv_choice *best_choice,
                                  struct pf_map *ferry_map,
                                  struct unit *boat,
                                  struct unit_type *boattype)
{
  struct player *pplayer = city_owner(pcity);
  const struct research *presearch = research_get(pplayer);
  /* The enemy city.  acity == NULL means stray enemy unit */
  struct city *acity = tile_city(ptile);
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_position pos;
  struct unit_type *orig_utype = best_choice->value.utype;
  int victim_count = 1;
  int needferry = 0;
  bool unhap = dai_assess_military_unhappiness(pcity);
  struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

  /* Has to be initialized to make gcc happy */
  struct ai_city *acity_data = NULL;

  if (acity != NULL) {
    acity_data = def_ai_city_data(acity, ait);
  }

  if (utype_class(orig_utype)->adv.sea_move == MOVE_NONE
      && !boat && boattype) {
    /* cost of ferry */
    needferry = utype_build_shield_cost(pcity, boattype);
  }
  
  if (!is_stack_vulnerable(ptile)) {
    /* If it is a city, a fortress or an air base,
     * we may have to whack it many times */
    victim_count += unit_list_size(ptile->units);
  }

  simple_ai_unit_type_iterate(punittype) {
    Tech_type_id tech_req = advance_number(punittype->require_advance);
    int tech_dist = research_goal_unknown_techs(presearch, tech_req);

    if (dai_can_unit_type_follow_unit_type(punittype, orig_utype, ait)
        && is_native_near_tile(&(wld.map), utype_class(punittype), ptile)
        && (U_NOT_OBSOLETED == punittype->obsoleted_by
            || !can_city_build_unit_direct(pcity, punittype->obsoleted_by))
        && punittype->attack_strength > 0 /* or we'll get SIGFPE */) {
      /* Values to be computed */
      int desire;
      adv_want want;
      int move_time;
      int vuln;
      int veteran_level = get_target_bonus_effects(NULL,
                                                   pplayer, NULL, pcity,
                                                   NULL, city_tile(pcity),
                                                   NULL, punittype, NULL,
                                                   NULL, NULL,
                                                   EFT_VETERAN_BUILD);
      /* Cost (shield equivalent) of gaining these techs. */
      /* FIXME? Katvrr advises that this should be weighted more heavily in big
       * danger. */
      int tech_cost = research_goal_bulbs_required(presearch, tech_req) / 4
                      / city_list_size(pplayer->cities);
      int bcost_balanced = build_cost_balanced(punittype);
      /* See description of kill_desire() for info about this variables. */
      int bcost = utype_build_shield_cost(pcity, punittype);
      int attack = adv_unittype_att_rating(punittype, veteran_level,
                                           SINGLE_MOVE,
                                           punittype->hp);

      /* Take into account reinforcements strength */
      if (acity) {
        attack += acity_data->attack;
      }

      if (attack == 0) {
       /* Yes, it can happen that a military unit has attack = 1,
        * for example militia with HP = 1 (in civ1 ruleset). */
        continue;
      }

      attack *= attack;

      pft_fill_utype_parameter(&parameter, punittype, city_tile(pcity),
                               pplayer);
      parameter.omniscience = !has_handicap(pplayer, H_MAP);
      pfm = pf_map_new(&parameter);

      /* Set the move_time appropriatelly. */
      move_time = -1;
      if (NULL != ferry_map) {
        struct tile *dest_tile;

        if (find_beachhead(pplayer, ferry_map, ptile, punittype,
                           &dest_tile, NULL)
            && pf_map_position(ferry_map, dest_tile, &pos)) {
          move_time = pos.turn;
          dest_tile = pf_map_parameter(ferry_map)->start_tile;
          pf_map_tiles_iterate(pfm, atile, FALSE) {
            if (atile == dest_tile) {
              pf_map_iter_position(pfm, &pos);
              move_time += pos.turn;
              break;
            } else if (atile == ptile) {
              /* Reaching directly seems better. */
              pf_map_iter_position(pfm, &pos);
              move_time = pos.turn;
              break;
            }
          } pf_map_tiles_iterate_end;
        }
      }

      if (-1 == move_time) {
        if (pf_map_position(pfm, ptile, &pos)) {
          move_time = pos.turn;
        } else {
          pf_map_destroy(pfm);
          continue;
        }
      }
      pf_map_destroy(pfm);

      /* Estimate strength of the enemy. */

      if (victim_unit_type) {
        vuln = unittype_def_rating_squared(punittype, victim_unit_type,
                                           victim_player,
                                           ptile, FALSE, veteran);
      } else {
        vuln = 0;
      }

      /* Not bothering to s/!vuln/!pdef/ here for the time being. -- Syela
       * (this is noted elsewhere as terrible bug making warships yoyoing) 
       * as the warships will go to enemy cities hoping that the enemy builds 
       * something for them to kill*/
      if (vuln == 0
          && (utype_class(punittype)->adv.land_move == MOVE_NONE
              || 0 < utype_fuel(punittype))) {
        desire = 0;
        
      } else {
        if (acity
            && utype_can_take_over(punittype)
            && acity_data->invasion.attack > 0
            && acity_data->invasion.occupy == 0) {
          desire = acity_data->worth * 10;
        } else {
          desire = 0;
        }

        if (!acity) {
          desire = kill_desire(value, attack, bcost, vuln, victim_count);
        } else {
          int kd;
          int city_attack = acity_data->attack * acity_data->attack;

          /* See aiunit.c:find_something_to_kill() for comments. */
          kd = kill_desire(value, attack,
                           (bcost + acity_data->bcost), vuln,
                           victim_count);

          if (value * city_attack > acity_data->bcost * vuln) {
            kd -= kill_desire(value, city_attack,
                              acity_data->bcost, vuln,
                              victim_count);
          }

          desire = MAX(desire, kd);
        }
      }
      
      desire -= tech_cost * SHIELD_WEIGHTING;
      /* We can be possibly making some people of our homecity unhappy - then
       * we don't want to travel too far away to our victims. */
      /* TODO: Unify the 'unhap' dependency to some common function. */
      desire -= move_time * (unhap ? SHIELD_WEIGHTING + 2 * TRADE_WEIGHTING
                             : SHIELD_WEIGHTING);

      want = military_amortize(pplayer, pcity, desire, MAX(1, move_time),
                               bcost_balanced + needferry);
      
      if (want > 0) {
        if (tech_dist > 0) {
          /* This is a future unit, tell the scientist how much we need it */
          plr_data->tech_want[advance_index(punittype->require_advance)]
            += want;
          TECH_LOG(ait, LOG_DEBUG, pplayer, punittype->require_advance,
                   "+ " ADV_WANT_PRINTF " for %s vs %s(%d,%d)",
                   want,
                   utype_rule_name(punittype),
                   (acity ? city_name_get(acity) : utype_rule_name(victim_unit_type)),
                   TILE_XY(ptile));
        } else if (want > best_choice->want) {
          struct impr_type *impr_req = punittype->need_improvement;

          if (can_city_build_unit_now(pcity, punittype)) {
            /* This is a real unit and we really want it */

            CITY_LOG(LOG_DEBUG, pcity, "overriding %s(" ADV_WANT_PRINTF
                     ") with %s(" ADV_WANT_PRINTF ")"
                     " [attack=%d,value=%d,move_time=%d,vuln=%d,bcost=%d]",
                     utype_rule_name(best_choice->value.utype),
		     best_choice->want,
                     utype_rule_name(punittype),
                     want,
                     attack, value, move_time, vuln, bcost);

            best_choice->value.utype = punittype;
            best_choice->want = want;
            best_choice->type = CT_ATTACKER;
          } else if (NULL == impr_req) {
            CITY_LOG(LOG_DEBUG, pcity, "cannot build unit %s",
                     utype_rule_name(punittype));
          } else if (can_city_build_improvement_now(pcity, impr_req)) {
	    /* Building this unit requires a specific type of improvement.
	     * So we build this improvement instead.  This may not be the
	     * best behavior. */
            CITY_LOG(LOG_DEBUG, pcity, "building %s to build unit %s",
                     improvement_rule_name(impr_req),
                     utype_rule_name(punittype));
            best_choice->value.building = impr_req;
            best_choice->want = want;
            best_choice->type = CT_BUILDING;
          } else {
	    /* This should never happen? */
            CITY_LOG(LOG_DEBUG, pcity, "cannot build %s or unit %s",
                     improvement_rule_name(impr_req),
                     utype_rule_name(punittype));
	  }
        }
      }
    }
  } simple_ai_unit_type_iterate_end;
}

/**********************************************************************//**
  This function
  1. receives (in myunit) a first estimate of what we would like to build.
  2. finds a potential victim for it.
  3. calculates the relevant stats of the victim.
  4. finds the best attacker for this type of victim (in process_attacker_want)
  5. if we still want to attack, records the best attacker in choice.
  If the target is overseas, the function might suggest building a ferry
  to carry a land attack unit, instead of the land attack unit itself.
**************************************************************************/
static struct adv_choice *kill_something_with(struct ai_type *ait, struct player *pplayer,
                                              struct city *pcity, struct unit *myunit,
                                              struct adv_choice *choice)
{
  /* Our attack rating (with reinforcements) */
  int attack;
  /* Benefit from fighting the target */
  int benefit;
  /* Defender of the target city/tile */
  struct unit *pdef; 
  struct unit_type *def_type;
  struct player *def_owner;
  int def_vet; /* Is the defender veteran? */
  /* Target coordinates */
  struct tile *ptile;
  /* Our transport */
  struct unit *ferryboat;
  /* Our target */
  struct city *acity;
  /* Type of the boat (real or a future one) */
  struct unit_type *boattype;
  struct pf_map *ferry_map = NULL;
  int move_time;
  struct adv_choice *best_choice;
  struct ai_city *city_data = def_ai_city_data(pcity, ait);
  struct ai_city *acity_data;

  best_choice = adv_new_choice();
  best_choice->value.utype = unit_type_get(myunit);
  best_choice->type = CT_ATTACKER;
  adv_choice_set_use(best_choice, "attacker");

  fc_assert_ret_val(is_military_unit(myunit) && !utype_fuel(unit_type_get(myunit)), choice);

  if (city_data->danger != 0 && assess_defense(ait, pcity) == 0) {
    /* Defence comes first! */
    goto cleanup;
  }

  best_choice->want = find_something_to_kill(ait, pplayer, myunit, &ptile, NULL,
                                             &ferry_map, &ferryboat,
                                             &boattype, &move_time);
  if (NULL == ptile
      || ptile == unit_tile(myunit)
      || !can_unit_attack_tile(myunit, ptile)) {
    goto cleanup;
  }

  acity = tile_city(ptile);

  if (myunit->id != 0) {
    log_error("%s(): non-virtual unit!", __FUNCTION__);
    goto cleanup;
  }

  attack = adv_unit_att_rating(myunit);
  if (acity) {
    acity_data = def_ai_city_data(acity, ait);
    attack += acity_data->attack;
  }
  attack *= attack;

  if (NULL != acity) {
    /* Rating of enemy defender */
    int vulnerability;

    if (!POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, city_owner(acity))) {
      /* Not a valid target */
      goto cleanup;
    }

    def_type = dai_choose_defender_versus(acity, myunit);
    def_owner = city_owner(acity);
    if (1 < move_time && def_type) {
      def_vet = do_make_unit_veteran(acity, def_type);
      vulnerability = unittype_def_rating_squared(unit_type_get(myunit), def_type,
                                                  city_owner(acity), ptile,
                                                  FALSE, def_vet);
      benefit = utype_build_shield_cost_base(def_type);
    } else {
      vulnerability = 0;
      benefit = 0;
      def_vet = 0;
    }

    pdef = get_defender(myunit, ptile);
    if (pdef) {
      int m = unittype_def_rating_squared(unit_type_get(myunit), unit_type_get(pdef),
                                          city_owner(acity), ptile, FALSE,
                                          pdef->veteran);
      if (vulnerability < m) {
        vulnerability = m;
        benefit = unit_build_shield_cost_base(pdef);
        def_vet = pdef->veteran;
        def_type = unit_type_get(pdef);
        def_owner = unit_owner(pdef);
      }
    }
    if (unit_can_take_over(myunit) || acity_data->invasion.occupy > 0) {
      /* bonus for getting the city */
      benefit += acity_data->worth / 3;
    }

    /* end dealing with cities */
  } else {

    if (NULL != ferry_map) {
      pf_map_destroy(ferry_map);
      ferry_map = NULL;
    }

    pdef = get_defender(myunit, ptile);
    if (!pdef) {
      /* Nobody to attack! */
      goto cleanup;
    }

    benefit = unit_build_shield_cost_base(pdef);

    def_type = unit_type_get(pdef);
    def_vet = pdef->veteran;
    def_owner = unit_owner(pdef);
    /* end dealing with units */
  }

  if (NULL == ferry_map) {
    process_attacker_want(ait, pcity, benefit, def_type, def_owner,
                          def_vet, ptile,
                          best_choice, NULL, NULL, NULL);
  } else { 
    /* Attract a boat to our city or retain the one that's already here */
    fc_assert_ret_val(unit_class_get(myunit)->adv.sea_move != MOVE_FULL, choice);
    best_choice->need_boat = TRUE;
    process_attacker_want(ait, pcity, benefit, def_type, def_owner,
                          def_vet, ptile,
                          best_choice, ferry_map, ferryboat, boattype);
  }

  if (best_choice->want > choice->want) {
    /* We want attacker more than what we have selected before */
    adv_free_choice(choice);
    choice = best_choice;
    CITY_LOG(LOG_DEBUG, pcity, "kill_something_with()"
             " %s has chosen attacker, %s, want=" ADV_WANT_PRINTF,
             city_name_get(pcity),
             utype_rule_name(best_choice->value.utype),
             best_choice->want);

    if (NULL != ferry_map && !ferryboat) { /* need a new ferry */
      /* We might need a new boat even if there are boats free,
       * if they are blockaded or in inland seas*/
      fc_assert_ret_val(unit_class_get(myunit)->adv.sea_move != MOVE_FULL, choice);
      if (dai_choose_role_unit(ait, pplayer, pcity, choice, CT_ATTACKER,
			       L_FERRYBOAT, choice->want, TRUE)
	  && dai_is_ferry_type(choice->value.utype, ait)) {
#ifdef FREECIV_DEBUG
        struct ai_plr *ai = dai_plr_data_get(ait, pplayer, NULL);

        log_debug("kill_something_with() %s has chosen attacker ferry, "
                  "%s, want=" ADV_WANT_PRINTF ", %d of %d free",
                  city_name_get(pcity),
                  utype_rule_name(choice->value.utype),
                  choice->want,
                  ai->stats.available_boats, ai->stats.boats);
#endif /* FREECIV_DEBUG */

        adv_choice_set_use(choice, "attacker ferry");
      } /* else can not build ferries yet */
    }
  }

cleanup:
  if (best_choice != choice) {
    /* It was not taken to use.
     * This hackery needed since 'goto cleanup:' might skip
     * sensible points to do adv_free_choice(). */
    adv_free_choice(best_choice);
  }
  if (NULL != ferry_map) {
    pf_map_destroy(ferry_map);
  }

  return choice;
}

/**********************************************************************//**
  This function should assign a value to choice and want and type,
  where want is a value between 1 and 100.
  if want is 0 this advisor doesn't want anything
**************************************************************************/
static void dai_unit_consider_bodyguard(struct ai_type *ait,
                                        struct city *pcity,
                                        struct unit_type *punittype,
                                        struct adv_choice *choice)
{
  struct unit *virtualunit;
  struct player *pplayer = city_owner(pcity);
  struct unit *aunit = NULL;
  struct city *acity = NULL;

  virtualunit = unit_virtual_create(pplayer, pcity, punittype,
                                    do_make_unit_veteran(pcity, punittype));

  if (choice->want < 100) {
    const int want = look_for_charge(ait, pplayer, virtualunit, &aunit, &acity);

    if (want > choice->want) {
      choice->want = want;
      choice->value.utype = punittype;
      choice->type = CT_DEFENDER;
      adv_choice_set_use(choice, "bodyguard");
    }
  }
  unit_virtual_destroy(virtualunit);
}

/**********************************************************************//**
  Before building a military unit, AI builds a barracks/port/airport
  NB: It is assumed this function isn't called in an emergency
  situation, when we need a defender _now_.
 
  TODO: something more sophisticated, like estimating future demand
  for military units, considering Sun Tzu instead.
**************************************************************************/
static void adjust_ai_unit_choice(struct city *pcity, 
                                  struct adv_choice *choice)
{
  Impr_type_id id;

  /* Sanity */
  if (!is_unit_choice_type(choice->type)
      || utype_has_flag(choice->value.utype, UTYF_CIVILIAN)
      || do_make_unit_veteran(pcity, choice->value.utype)) {
    return;
  }

  /*  N.B.: have to check that we haven't already built the building --mck */
  if ((id = dai_find_source_building(pcity, EFT_VETERAN_BUILD,
                                     choice->value.utype)) != B_LAST
       && !city_has_building(pcity, improvement_by_number(id))) {
    choice->value.building = improvement_by_number(id);
    choice->type = CT_BUILDING;
    adv_choice_set_use(choice, "veterancy building");
  }
}

/**********************************************************************//**
  This function selects either a defender or an attacker to be built.
  It records its choice into adv_choice struct.
  If 'choice->want' is 0 this advisor doesn't want anything.
**************************************************************************/
struct adv_choice *military_advisor_choose_build(struct ai_type *ait,
                                                 struct player *pplayer,
                                                 struct city *pcity,
                                                 const struct civ_map *mamap,
                                                 player_unit_list_getter ul_cb)
{
  struct adv_data *ai = adv_data_get(pplayer, NULL);
  struct unit_type *punittype;
  unsigned int our_def, urgency;
  struct tile *ptile = pcity->tile;
  struct unit *virtualunit;
  struct ai_city *city_data = def_ai_city_data(pcity, ait);
  adv_want martial_value = 0;
  bool martial_need = FALSE;
  struct adv_choice *choice = adv_new_choice();
  bool allow_gold_upkeep;

  urgency = assess_danger(ait, pcity, mamap, ul_cb);
  /* Changing to quadratic to stop AI from building piles 
   * of small units -- Syela */
  /* It has to be AFTER assess_danger thanks to wallvalue. */
  our_def = assess_defense_quadratic(ait, pcity); 

  dai_choose_diplomat_defensive(ait, pplayer, pcity, choice, our_def);

  if (pcity->feel[CITIZEN_UNHAPPY][FEELING_NATIONALITY]
      + pcity->feel[CITIZEN_ANGRY][FEELING_NATIONALITY] > 0) {
    martial_need = TRUE;
  }

  if (!martial_need) {
    specialist_type_iterate(sp) {
      if (pcity->specialists[sp] > 0
          && get_specialist_output(pcity, sp, O_LUXURY) > 0) {
        martial_need = TRUE;
        break;
      }
    } specialist_type_iterate_end;
  }

  if (martial_need
      && unit_list_size(pcity->tile->units) < get_city_bonus(pcity, EFT_MARTIAL_LAW_MAX)) {
    martial_value = dai_content_effect_value(pplayer, pcity,
                                             get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH),
                                             1, FEELING_FINAL);
  }

  /* Otherwise no need to defend yet */
  if (city_data->danger != 0 || martial_value > 0) {
    struct impr_type *pimprove;
    int num_defenders = unit_list_size(ptile->units);
    int wall_id, danger;
    bool build_walls = TRUE;
    int qdanger = city_data->danger * city_data->danger;

    /* First determine the danger.  It is measured in percents of our 
     * defensive strength, capped at 200 + urgency */
    if (qdanger >= our_def) {
      if (urgency == 0) {
        /* don't waste money */
        danger = 100;
      } else if (our_def == 0) {
        danger = 200 + urgency;
      } else {
        danger = MIN(200, 100 * qdanger / our_def) + urgency;
      }
    } else { 
      danger = 100 * qdanger / our_def;
    }
    if (pcity->surplus[O_SHIELD] <= 0 && our_def != 0) {
      /* Won't be able to support anything */
      danger = 0;
    }

    CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d urgency=%d danger=%d num_def=%d "
             "our_def=%d", urgency, danger, num_defenders, our_def);

    if (our_def == 0) {
      /* Build defensive unit first! Walls will help nothing if there's
       * nobody behind them. */
      if (dai_process_defender_want(ait, pplayer, pcity, danger, choice)) {
        choice->want = 100 + danger;
        adv_choice_set_use(choice, "first defender");
        build_walls = FALSE;

        CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d wants first defender with " ADV_WANT_PRINTF,
                 choice->want);
      }
    }
    if (build_walls) {
      /* FIXME: 1. Does not consider what kind of future danger is likely, so
       * may build SAM batteries when enemy has only land units. */
      /* We will build walls if we can and want and (have "enough" defenders or
       * can just buy the walls straight away) */

      /* HACK: This needs changing if multiple improvements provide
       * this effect. */
      wall_id = dai_find_source_building(pcity, EFT_DEFEND_BONUS, NULL);
      pimprove = improvement_by_number(wall_id);

      if (wall_id != B_LAST
          && pcity->server.adv->building_want[wall_id] != 0 && our_def != 0 
          && can_city_build_improvement_now(pcity, pimprove)
          && (danger < 101 || num_defenders > 1
              || (city_data->grave_danger == 0 
                  && pplayer->economic.gold
                     > impr_buy_gold_cost(pcity, pimprove, pcity->shield_stock)))
          && ai_fuzzy(pplayer, TRUE)) {
        if (pcity->server.adv->building_want[wall_id] > 0) {
          /* NB: great wall is under domestic */
          choice->value.building = pimprove;
          /* building_want is hacked by assess_danger */
          choice->want = pcity->server.adv->building_want[wall_id];
          if (urgency == 0 && choice->want > 100) {
            choice->want = 100;
          }
          choice->type = CT_BUILDING;
          adv_choice_set_use(choice, "defense building");
          CITY_LOG(LOG_DEBUG, pcity,
                   "m_a_c_d wants defense building with " ADV_WANT_PRINTF,
                   choice->want);
        } else {
          build_walls = FALSE;
        }
      } else {
        build_walls = FALSE;
      }
      if ((danger > 0 && num_defenders <= urgency) || martial_value > 0) {
        struct adv_choice uchoice;

        adv_init_choice(&uchoice);

        /* Consider building defensive units */
        if (dai_process_defender_want(ait, pplayer, pcity, danger, &uchoice)) {
          /* Potential defender found */
          if (urgency == 0
              && uchoice.value.utype->defense_strength == 1) {
            /* FIXME: check other reqs (unit class?) */
            if (get_city_bonus(pcity, EFT_HP_REGEN) > 0) {
              /* unlikely */
              uchoice.want = MIN(49, danger);
            } else {
              uchoice.want = MIN(25, danger);
            }
          } else {
            uchoice.want = danger;
          }

          uchoice.want += martial_value;
          adv_choice_set_use(&uchoice, "defender");

          if (!build_walls || uchoice.want > choice->want) {
            adv_choice_copy(choice, &uchoice);
          }
          adv_deinit_choice(&uchoice);

          CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d wants %s with desire " ADV_WANT_PRINTF,
                   utype_rule_name(choice->value.utype),
                   choice->want);
        } else {
          CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d cannot select defender");
        }
      } else {
        CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d does not want defenders");
      }
    }
  } /* ok, don't need to defend */

  if (pcity->surplus[O_SHIELD] <= 0 
      || pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] > pcity->feel[CITIZEN_UNHAPPY][FEELING_EFFECT]
      || pcity->id == ai->wonder_city) {
    /* Things we consider below are not life-saving so we don't want to 
     * build them if our populace doesn't feel like it */
    return choice;
  }

  if (pplayer->economic.tax <= 50 || city_total_unit_gold_upkeep(pcity) <= 0) {
    /* Always allow one unit with real gold upkeep (after EFT_UNIT_UPKEEP_FREE_PER_CITY)
     * Allow more if economics is so strong that we have not increased taxes. */
    allow_gold_upkeep = TRUE;
  } else {
    allow_gold_upkeep = FALSE;
  }

  /* Consider making a land bodyguard */
  punittype = dai_choose_bodyguard(ait, pcity, TC_LAND, L_DEFEND_GOOD,
                                   allow_gold_upkeep);
  if (punittype) {
    dai_unit_consider_bodyguard(ait, pcity, punittype, choice);
  }

  /* If we are in severe danger, don't consider attackers. This is probably
     too general. In many cases we will want to buy attackers to counterattack.
     -- Per */
  if (choice->want - martial_value > 100 && city_data->grave_danger > 0) {
    CITY_LOG(LOGLEVEL_BUILD, pcity,
             "severe danger (want " ADV_WANT_PRINTF "), force defender",
             choice->want);
    return choice;
  }

  /* Consider making an offensive diplomat */
  dai_choose_diplomat_offensive(ait, pplayer, pcity, choice);

  /* Consider making a sea bodyguard */
  punittype = dai_choose_bodyguard(ait, pcity, TC_OCEAN, L_DEFEND_GOOD,
                                   allow_gold_upkeep);
  if (punittype) {
    dai_unit_consider_bodyguard(ait, pcity, punittype, choice);
  }

  /* Consider making an airplane */
  (void) dai_choose_attacker_air(ait, pplayer, pcity, choice, allow_gold_upkeep);

  /* Consider making a paratrooper */
  dai_choose_paratrooper(ait, pplayer, pcity, choice, allow_gold_upkeep);

  /* Check if we want a sailing attacker. Have to put sailing first
     before we mung the seamap */
  punittype = dai_choose_attacker(ait, pcity, TC_LAND, allow_gold_upkeep);
  if (punittype) {
    virtualunit = unit_virtual_create(pplayer, pcity, punittype,
                                      do_make_unit_veteran(pcity, punittype));
    choice = kill_something_with(ait, pplayer, pcity, virtualunit, choice);
    unit_virtual_destroy(virtualunit);
  }

  /* Consider a land attacker or a ferried land attacker
   * (in which case, we might want a ferry before an attacker)
   */
  punittype = dai_choose_attacker(ait, pcity, TC_LAND, allow_gold_upkeep);
  if (punittype) {
    virtualunit = unit_virtual_create(pplayer, pcity, punittype, 1);
    choice = kill_something_with(ait, pplayer, pcity, virtualunit, choice);
    unit_virtual_destroy(virtualunit);
  }

  /* Consider a hunter */
  dai_hunter_choice(ait, pplayer, pcity, choice, allow_gold_upkeep);

  /* Consider veteran level enhancing buildings before non-urgent units */
  adjust_ai_unit_choice(pcity, choice);

  if (choice->want <= 0) {
    CITY_LOG(LOGLEVEL_BUILD, pcity, "military advisor has no advice");
  } else {
    CITY_LOG(LOGLEVEL_BUILD, pcity,
             "military advisor choice: %s (want " ADV_WANT_PRINTF ")",
             dai_choice_rule_name(choice),
             choice->want);
  }

  return choice;
}
