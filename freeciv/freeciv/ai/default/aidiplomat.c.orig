/***********************************************************************
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
#include <fc_config.h>
#endif

/* utility */
#include "bitvector.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "timing.h"

/* common */
#include "actions.h"
#include "city.h"
#include "combat.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "unit.h"
#include "unitlist.h"

/* aicore */
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
#include "advdata.h"
#include "advgoto.h"
#include "autosettlers.h"

/* ai */
#include "handicaps.h"

/* ai/default */
#include "aicity.h"
#include "aidata.h"
#include "aiguard.h"
#include "aihand.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aitools.h"
#include "aiunit.h"
#include "daimilitary.h"

#include "aidiplomat.h"


#define LOG_DIPLOMAT LOG_DEBUG
#define LOG_DIPLOMAT_BUILD LOG_DEBUG

/* 3000 is a just a large number, but not hillariously large as the
 * previously used one. This is important for diplomacy. - Per */
#define DIPLO_DEFENSE_WANT 3000

static bool is_city_surrounded_by_our_spies(struct player *pplayer, 
                                            struct city *pcity);

static void find_city_to_diplomat(struct player *pplayer, struct unit *punit,
                                  struct city **ctarget, int *move_dist,
                                  struct pf_map *pfm);

/**************************************************************************//**
  Number of improvements that can be sabotaged in pcity.
******************************************************************************/
static int count_sabotagable_improvements(struct city *pcity)
{
  int count = 0;

  city_built_iterate(pcity, pimprove) {
    if (pimprove->sabotage > 0) {
      count++;
    }
  } city_built_iterate_end;

  return count;
}

/**************************************************************************//**
  Number of techs that we don't have and the enemy (tplayer) does.
******************************************************************************/
static int count_stealable_techs(struct player *pplayer, struct player *tplayer)
{
  struct research *presearch = research_get(pplayer);
  struct research *tresearch = research_get(tplayer);
  int count = 0;

  advance_index_iterate(A_FIRST, idx) {
    if (research_invention_state(presearch, idx) != TECH_KNOWN
        && research_invention_state(tresearch, idx) == TECH_KNOWN) {
      count++;
    }
  } advance_index_iterate_end;

  return count;
}

/**************************************************************************//**
  Calculates our need for diplomats as defensive units. May replace
  values in choice. The values 16000 and 3000 used below are totally
  arbitrary but seem to work.
******************************************************************************/
void dai_choose_diplomat_defensive(struct ai_type *ait,
                                   struct player *pplayer,
                                   struct city *pcity,
                                   struct adv_choice *choice, int def)
{
  /* Build a diplomat if our city is threatened by enemy diplomats, and
     we have other defensive troops, and we don't already have a diplomat
     to protect us. If we see an enemy diplomat and we don't have diplomat
     tech... race it! */
  struct ai_city *city_data = def_ai_city_data(pcity, ait);

  if (def != 0 && city_data->diplomat_threat
      && !city_data->has_diplomat) {
    struct unit_type *ut = best_role_unit(pcity, UTYF_DIPLOMAT);

    if (ut) {
       log_base(LOG_DIPLOMAT_BUILD, 
                "A defensive diplomat will be built in city %s.",
                city_name_get(pcity));
       choice->want = 16000; /* diplomat more important than soldiers */
       city_data->urgency = 1;
       choice->type = CT_DEFENDER;
       choice->value.utype = ut;
       choice->need_boat = FALSE;
       adv_choice_set_use(choice, "defensive diplomat");
    } else if (num_role_units(UTYF_DIPLOMAT) > 0) {
      /* We don't know diplomats yet... */
      log_base(LOG_DIPLOMAT_BUILD,
               "A defensive diplomat is wanted badly in city %s.",
               city_name_get(pcity));
      ut = get_role_unit(UTYF_DIPLOMAT, 0);
      if (ut) {
        struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

        plr_data->tech_want[advance_index(ut->require_advance)]
          += DIPLO_DEFENSE_WANT;
        TECH_LOG(ait, LOG_DEBUG, pplayer, ut->require_advance,
                 "ai_choose_diplomat_defensive() + %d for %s",
                 DIPLO_DEFENSE_WANT,
                 utype_rule_name(ut));
      }
    }
  }
}

/**************************************************************************//**
  Calculates our need for diplomats as offensive units. May replace
  values in choice.
******************************************************************************/
void dai_choose_diplomat_offensive(struct ai_type *ait,
                                   struct player *pplayer,
                                   struct city *pcity,
                                   struct adv_choice *choice)
{
  struct unit_type *ut = best_role_unit(pcity, UTYF_DIPLOMAT);
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);
  int expenses;

  dai_calc_data(pplayer, NULL, &expenses, NULL);

  if (!ut) {
    /* We don't know diplomats yet! */
    return;
  }

  if (has_handicap(pplayer, H_DIPLOMAT)) {
    /* Diplomats are too tough on newbies */
    return;
  }

  /* Do we have a good reason for building diplomats? */
  {
    const struct research *presearch = research_get(pplayer);
    struct pf_map *pfm;
    struct pf_parameter parameter;
    struct city *acity;
    int want, loss, p_success, p_failure, time_to_dest;
    int gain_incite = 0, gain_theft = 0, gain = 1;
    int incite_cost;
    struct unit *punit = unit_virtual_create(pplayer, pcity, ut,
                                             do_make_unit_veteran(pcity, ut));

    pft_fill_unit_parameter(&parameter, punit);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    pfm = pf_map_new(&parameter);

    find_city_to_diplomat(pplayer, punit, &acity, &time_to_dest, pfm);

    pf_map_destroy(pfm);
    unit_virtual_destroy(punit);

    if (acity == NULL
	|| BV_ISSET(ai->stats.diplomat_reservations, acity->id)) {
      /* Found no target or city already considered */
      return;
    }
    incite_cost = city_incite_cost(pplayer, acity);
    if (POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, city_owner(acity))
        && (is_action_possible_on_city(ACTION_SPY_INCITE_CITY,
                                       pplayer, acity)
            || is_action_possible_on_city(ACTION_SPY_INCITE_CITY_ESC,
                                          pplayer, acity))
        && (incite_cost < INCITE_IMPOSSIBLE_COST)
        && (incite_cost < pplayer->economic.gold - expenses)) {
      /* incite gain (FIXME: we should count wonders too but need to
         cache that somehow to avoid CPU hog -- Per) */
      gain_incite = acity->prod[O_FOOD] * FOOD_WEIGHTING
                    + acity->prod[O_SHIELD] * SHIELD_WEIGHTING
                    + (acity->prod[O_LUXURY]
                       + acity->prod[O_GOLD]
                       + acity->prod[O_SCIENCE]) * TRADE_WEIGHTING;
      gain_incite *= SHIELD_WEIGHTING; /* WAG cost to take city otherwise */
      gain_incite -= incite_cost * TRADE_WEIGHTING;
    }
    if ((research_get(city_owner(acity))->techs_researched
         > presearch->techs_researched)
        && (is_action_possible_on_city(ACTION_SPY_TARGETED_STEAL_TECH,
                                       pplayer, acity)
            || is_action_possible_on_city(ACTION_SPY_TARGETED_STEAL_TECH_ESC,
                                          pplayer, acity)
            || is_action_possible_on_city(ACTION_SPY_STEAL_TECH,
                                          pplayer, acity)
            || is_action_possible_on_city(ACTION_SPY_STEAL_TECH_ESC,
                                          pplayer, acity))
	&& !pplayers_allied(pplayer, city_owner(acity))) {
      /* tech theft gain */
      /* FIXME: this value is right only when
       * 'game.info.game.info.tech_cost_style' is set to
       * TECH_COST_CIV1CIV2. */
      gain_theft =
          (research_total_bulbs_required(presearch, presearch->researching,
                                         FALSE) * TRADE_WEIGHTING);
    }
    gain = MAX(gain_incite, gain_theft);
    loss = utype_build_shield_cost(pcity, ut) * SHIELD_WEIGHTING;

    /* Probability to succeed, assuming no defending diplomat */
    p_success = game.server.diplchance;
    /* Probability to lose our unit */
    p_failure = (utype_can_do_action(ut, ACTION_SPY_TARGETED_STEAL_TECH_ESC)
                 || utype_can_do_action(ut, ACTION_SPY_STEAL_TECH_ESC) ?
                   100 - p_success :
                   100);

    /* Get the time to dest in turns (minimum 1 turn) */
    time_to_dest = (time_to_dest + ut->move_rate - 1) / ut->move_rate;
    /* Discourage long treks */
    time_to_dest *= ((time_to_dest + 1) / 2);

    /* Almost kill_desire */
    want = (p_success * gain - p_failure * loss) / 100
           - SHIELD_WEIGHTING * time_to_dest;
    if (want <= 0) {
      return;
    }

    want = military_amortize(pplayer, pcity, want, time_to_dest, 
                             utype_build_shield_cost(pcity, ut));

    if (!player_has_embassy(pplayer, city_owner(acity))
        && want < 99
        && (is_action_possible_on_city(ACTION_ESTABLISH_EMBASSY,
                                       pplayer, acity)
            || is_action_possible_on_city(ACTION_ESTABLISH_EMBASSY_STAY,
                                          pplayer, acity))) {
        log_base(LOG_DIPLOMAT_BUILD,
                 "A diplomat desired in %s to establish an embassy with %s "
                 "in %s",
                 city_name_get(pcity),
                 player_name(city_owner(acity)),
                 city_name_get(acity));
        want = 99;
    }
    if (want > choice->want) {
      log_base(LOG_DIPLOMAT_BUILD,
               "%s, %s: %s is desired with want %d to spy in %s (incite "
               "want %d cost %d gold %d, tech theft want %d, ttd %d)",
               player_name(pplayer),
               city_name_get(pcity),
               utype_rule_name(ut),
               want,
               city_name_get(acity),
               gain_incite,
               incite_cost,
               pplayer->economic.gold - expenses,
               gain_theft,
               time_to_dest);
      choice->want = want;
      choice->type = CT_CIVILIAN; /* so we don't build barracks for it */
      choice->value.utype = ut;
      choice->need_boat = FALSE;
      adv_choice_set_use(choice, "offensive diplomat");
      BV_SET(ai->stats.diplomat_reservations, acity->id);
    }
  }
}

/**************************************************************************//**
  Pick a tech for actor_player to steal from target_player.

  TODO: Make a smarter choice than picking the first stealable tech found.
******************************************************************************/
static Tech_type_id
choose_tech_to_steal(const struct player *actor_player,
                     const struct player *target_player)
{
  const struct research *actor_research = research_get(actor_player);
  const struct research *target_research = research_get(target_player);

  if (actor_research != target_research) {
    if (can_see_techs_of_target(actor_player, target_player)) {
      advance_iterate(A_FIRST, padvance) {
        Tech_type_id i = advance_number(padvance);

        if (research_invention_state(target_research, i) == TECH_KNOWN
            && research_invention_gettable(actor_research, i,
                                           game.info.tech_steal_allow_holes)
            && (research_invention_state(actor_research, i) == TECH_UNKNOWN
                || (research_invention_state(actor_research, i)
                    == TECH_PREREQS_KNOWN))) {

          return i;
        }
      } advance_iterate_end;
    }
  }

  /* Unable to find a target. */
  return A_UNSET;
}

/**************************************************************************//**
  Check if something is on our receiving end for some nasty diplomat
  business! Note that punit may die or be moved during this function. We
  must be adjacent to target city.

  We try to make embassy first, and abort if we already have one and target
  is allied. Then we steal, incite, sabotage or poison the city, in that
  order of priority.
******************************************************************************/
static void dai_diplomat_city(struct ai_type *ait, struct unit *punit,
                              struct city *ctarget)
{
  struct player *pplayer = unit_owner(punit);
  struct player *tplayer = city_owner(ctarget);
  int count_impr = count_sabotagable_improvements(ctarget);
  int count_tech = count_stealable_techs(pplayer, tplayer);
  int incite_cost, expenses;

  fc_assert_ret(is_ai(pplayer));

  if (punit->moves_left == 0) {
    UNIT_LOG(LOG_ERROR, punit, "no moves left in ai_diplomat_city()!");
  }

  unit_activity_handling(punit, ACTIVITY_IDLE);

#define T(my_act, my_val)                                                  \
  if (action_prob_possible(action_prob_vs_city(punit, my_act, ctarget))) { \
    log_base(LOG_DIPLOMAT, "%s %s[%d] does " #my_act " at %s",             \
             nation_rule_name(nation_of_unit(punit)),                      \
             unit_rule_name(punit), punit->id, city_name_get(ctarget));    \
    unit_do_action(pplayer, punit->id,                                     \
                   ctarget->id, my_val, "", my_act);                       \
    return;                                                                \
  }

  T(ACTION_ESTABLISH_EMBASSY, 0);
  T(ACTION_ESTABLISH_EMBASSY_STAY, 0);

  if (pplayers_allied(pplayer, tplayer)) {
    return; /* Don't do the rest to allies */
  }

  if (count_tech > 0 
      && (diplomats_unignored_tech_stealings(punit, ctarget) == 0
          || (action_prob_possible(action_prob_vs_city(punit,
                  ACTION_SPY_TARGETED_STEAL_TECH_ESC, ctarget))
              || action_prob_possible(action_prob_vs_city(punit,
                     ACTION_SPY_STEAL_TECH_ESC, ctarget))))) {
    Tech_type_id tgt_tech;

    /* Picking a random tech has better odds. */
    T(ACTION_SPY_STEAL_TECH_ESC, 0);
    T(ACTION_SPY_STEAL_TECH, 0);

    /* Not able to steal a random tech. This means worse odds. */
    tgt_tech = choose_tech_to_steal(pplayer, tplayer);
    if (tgt_tech != A_UNSET) {
      /* A tech target can be identified. */
      T(ACTION_SPY_TARGETED_STEAL_TECH_ESC, tgt_tech);
      T(ACTION_SPY_TARGETED_STEAL_TECH, tgt_tech);
    }
  } else {
    UNIT_LOG(LOG_DIPLOMAT, punit, "We have already stolen from %s!",
             city_name_get(ctarget));
  }

  incite_cost = city_incite_cost(pplayer, ctarget);
  dai_calc_data(pplayer, NULL, &expenses, NULL);

  if (incite_cost <= pplayer->economic.gold - 2 * expenses) {
    T(ACTION_SPY_INCITE_CITY_ESC, 0);
    T(ACTION_SPY_INCITE_CITY, 0);
  } else {
    UNIT_LOG(LOG_DIPLOMAT, punit, "%s too expensive!",
             city_name_get(ctarget));
  }

  if (!pplayers_at_war(pplayer, tplayer)) {
    return; /* The rest are casus belli */
  }

  if (count_impr > 0) {
    T(ACTION_SPY_SABOTAGE_CITY_ESC, 0);
    T(ACTION_SPY_SABOTAGE_CITY, 0);
  }

  /* Sabotage a specific city improvement. This has worse odds than
   * sabotaging a random city improvement. */
  if (count_impr > 0) {
    /* TODO: consider target improvements in stead of always going after
     * the current production. */
    int tgt_impr = -1;

    T(ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC, tgt_impr + 1);
    T(ACTION_SPY_TARGETED_SABOTAGE_CITY, tgt_impr + 1);
  }

  T(ACTION_SPY_STEAL_GOLD_ESC, 0);
  T(ACTION_SPY_STEAL_GOLD, 0);

  T(ACTION_STEAL_MAPS_ESC, 0);
  T(ACTION_STEAL_MAPS, 0);

  /* last resort */
  T(ACTION_SPY_POISON_ESC, 0);
  T(ACTION_SPY_POISON, 0);

   /* absolutely last resort */
  T(ACTION_SPY_NUKE_ESC, 0);
  T(ACTION_SPY_NUKE, 0);
#undef T

  /* This can happen for a number of odd and esoteric reasons  */
  UNIT_LOG(LOG_DIPLOMAT, punit,
           "decides to stand idle outside enemy city %s!",
           city_name_get(ctarget));
  dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
}

/**************************************************************************//**
  Check if we have a diplomat / spy near a given city. This is used to prevent
  a stack of such units next to a foreign city.
******************************************************************************/
static bool is_city_surrounded_by_our_spies(struct player *pplayer,
                                            struct city *enemy_city)
{
  adjc_iterate(&(wld.map), city_tile(enemy_city), ptile) {
    if (has_handicap(pplayer, H_FOG)
        && !map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
      /* We cannot see danger at (ptile) => assume there is none. */
      continue;
    }
    unit_list_iterate(ptile->units, punit) {
      if (unit_owner(punit) == pplayer
          && (utype_can_do_action(unit_type_get(punit),
                                  ACTION_SPY_INVESTIGATE_CITY)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_INV_CITY_SPEND)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_POISON)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_POISON_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_STEAL_GOLD)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_STEAL_GOLD_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_SABOTAGE_CITY)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_SABOTAGE_CITY_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_TARGETED_SABOTAGE_CITY)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_STEAL_TECH)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_STEAL_TECH_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_TARGETED_STEAL_TECH)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_TARGETED_STEAL_TECH_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_INCITE_CITY)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_INCITE_CITY_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_BRIBE_UNIT)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_SABOTAGE_UNIT_ESC)
              || utype_can_do_action(unit_type_get(punit),
                                     ACTION_SPY_SABOTAGE_UNIT))) {
        return TRUE;
      }
    } unit_list_iterate_end;
  } adjc_iterate_end;

  return FALSE;
}

/**************************************************************************//**
  Returns (in ctarget) the closest city to send diplomats against, or NULL
  if none available on this continent.  punit can be virtual.
******************************************************************************/
static void find_city_to_diplomat(struct player *pplayer, struct unit *punit,
                                  struct city **ctarget, int *move_dist,
                                  struct pf_map *pfm)
{
  bool has_embassy;
  int incite_cost = 0; /* incite cost */
  int expenses;
  bool dipldef; /* whether target is protected by diplomats */

  fc_assert_ret(punit != NULL);
  *ctarget = NULL;
  *move_dist = -1;
  dai_calc_data(pplayer, NULL, &expenses, NULL);

  pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
    struct city *acity;
    struct player *aplayer;
    bool can_incite;
    bool can_steal;

    acity = tile_city(ptile);

    if (!acity) {
      continue;
    }
    aplayer = city_owner(acity);

    has_embassy = player_has_embassy(pplayer, aplayer);

    if (aplayer == pplayer || is_barbarian(aplayer)
        || (pplayers_allied(pplayer, aplayer) && has_embassy)) {
      continue; 
    }

    incite_cost = city_incite_cost(pplayer, acity);
    can_incite = (incite_cost < INCITE_IMPOSSIBLE_COST)
        && (is_action_possible_on_city(ACTION_SPY_INCITE_CITY,
                                       pplayer, acity)
            || is_action_possible_on_city(ACTION_SPY_INCITE_CITY_ESC,
                                          pplayer, acity));

    can_steal = is_action_possible_on_city(ACTION_SPY_STEAL_TECH,
                                           pplayer, acity)
        || is_action_possible_on_city(ACTION_SPY_STEAL_TECH_ESC,
                                      pplayer, acity)
        || is_action_possible_on_city(ACTION_SPY_TARGETED_STEAL_TECH,
                                      pplayer, acity)
        || is_action_possible_on_city(ACTION_SPY_TARGETED_STEAL_TECH_ESC,
                                      pplayer, acity);

    dipldef = (count_diplomats_on_tile(acity->tile) > 0);
    /* Three actions to consider:
     * 1. establishing embassy OR
     * 2. stealing techs OR
     * 3. inciting revolt */
    if ((!has_embassy
         && (is_action_possible_on_city(ACTION_ESTABLISH_EMBASSY,
                                        pplayer, acity)
             || is_action_possible_on_city(ACTION_ESTABLISH_EMBASSY_STAY,
                                           pplayer, acity)))
        || (diplomats_unignored_tech_stealings(punit, acity) == 0
            && !dipldef && can_steal
            && (research_get(pplayer)->techs_researched
                < research_get(aplayer)->techs_researched))
        || (incite_cost < (pplayer->economic.gold - expenses)
            && can_incite && !dipldef)) {
      if (!is_city_surrounded_by_our_spies(pplayer, acity)) {
        /* We have the closest enemy city on the continent */
        *ctarget = acity;
        *move_dist = move_cost;
        break;
      }
    }
  } pf_map_move_costs_iterate_end;
}

/**************************************************************************//**
  Go to nearest/most threatened city (can be the current city too).
******************************************************************************/
static struct city *dai_diplomat_defend(struct ai_type *ait,
                                        struct player *pplayer,
                                        struct unit *punit,
                                        const struct unit_type *utype,
                                        struct pf_map *pfm)
{
  int best_dist = 30; /* any city closer than this is better than none */
  int best_urgency = 0;
  struct city *ctarget = NULL;
  struct city *pcity = tile_city(unit_tile(punit));

  if (pcity 
      && count_diplomats_on_tile(pcity->tile) == 1
      && def_ai_city_data(pcity, ait)->urgency > 0) {
    /* Danger and we are only diplomat present - stay. */
    return pcity;
  }

  pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
    struct city *acity;
    struct player *aplayer;
    int dipls, urgency;
    struct ai_city *city_data;

    acity = tile_city(ptile);
    if (!acity) {
      continue;
    }
    aplayer = city_owner(acity);
    if (aplayer != pplayer) {
      continue;
    }

    city_data = def_ai_city_data(acity, ait);
    urgency = city_data->urgency;
    dipls = (count_diplomats_on_tile(ptile)
             - (same_pos(ptile, unit_tile(punit)) ? 1 : 0));
    if (dipls == 0 && city_data->diplomat_threat) {
      /* We are _really_ needed there */
      urgency = (urgency + 1) * 5;
    } else if (dipls > 0) {
      /* We are probably not needed there... */
      urgency /= 3;
    }

    /* This formula may not be optimal, but it works. */
    if (move_cost > best_dist) {
      /* punish city for being so far away */
      urgency /= (float) (move_cost / best_dist);
    }

    if (urgency > best_urgency) {
      /* Found something worthy of our presence */
      ctarget = acity;
      best_urgency = urgency;
      /* squelch divide-by-zero */
      best_dist = MAX(move_cost, 1);
    }
  } pf_map_move_costs_iterate_end;

  return ctarget;
}

/**************************************************************************//**
  Find units that we can reach, and bribe them. Returns TRUE if survived
  the ordeal, FALSE if not or we expended all our movement.
  Will try to bribe a ship on the coast as well as land stuff.
******************************************************************************/
static bool dai_diplomat_bribe_nearby(struct ai_type *ait,
                                      struct player *pplayer, 
                                      struct unit *punit, struct pf_map *pfm)
{
  int gold_avail, expenses;

  dai_calc_data(pplayer, NULL, &expenses, NULL);
  gold_avail = pplayer->economic.gold - expenses;

  pf_map_positions_iterate(pfm, pos, FALSE) {
    struct tile *ptile = pos.tile;
    bool threat = FALSE;
    int newval, bestval = 0, cost;
    struct unit *pvictim = is_other_players_unit_tile(ptile, pplayer);
    int sanity = punit->id;
    struct unit_type *ptype;

    if (pos.total_MC > punit->moves_left) {
      /* Didn't find anything within range. */
      break;
    }

    if (!pvictim
        || !POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, unit_owner(pvictim))
        || unit_list_size(ptile->units) > 1
        || tile_city(ptile)
        || !is_action_enabled_unit_on_unit(ACTION_SPY_BRIBE_UNIT,
                                           punit, pvictim)) {
      continue;
    }

    /* Calculate if enemy is a threat */
    /* First find best defender on our tile */
    unit_list_iterate(ptile->units, aunit) {
      struct unit_type *atype = unit_type_get(aunit);

      newval = DEFENSE_POWER(atype);
      if (bestval < newval) {
        bestval = newval;
      }
    } unit_list_iterate_end;
    /* Compare with victim's attack power */
    ptype = unit_type_get(pvictim);
    newval = ATTACK_POWER(ptype);
    if (newval > bestval
        && unit_move_rate(pvictim) > pos.total_MC) {
      /* Enemy can probably kill us */
      threat = TRUE;
    } else {
      /* Enemy cannot reach us or probably not kill us */
      threat = FALSE;
    }

    if (has_handicap(pplayer, H_NOBRIBE_WF)) {
      /* Don't bribe settlers! */
      if (unit_has_type_flag(pvictim, UTYF_SETTLERS)
          || unit_can_do_action(pvictim, ACTION_FOUND_CITY)) {
        continue;
      }
    }

    /* Should we make the expense? */
    cost = unit_bribe_cost(pvictim, pplayer);
    if (!threat) {
      /* Don't empty our treasure without good reason! */
      gold_avail = pplayer->economic.gold - dai_gold_reserve(pplayer);
    }
    if (cost > gold_avail) {
      /* Can't afford */
      continue;
    }

    /* Found someone! */
    {
      struct tile *bribee_tile;
      struct pf_path *path;

      bribee_tile = mapstep(&(wld.map), pos.tile, DIR_REVERSE(pos.dir_to_here));
      path = pf_map_path(pfm, bribee_tile);
      if (!path || !adv_unit_execute_path(punit, path) 
          || punit->moves_left <= 0) {
        pf_path_destroy(path);
        return FALSE;
      }
      pf_path_destroy(path);
    }

    if (action_prob_possible(action_prob_vs_unit(punit,
                                 ACTION_SPY_BRIBE_UNIT,
                                 pvictim))) {
      unit_do_action(pplayer, punit->id,
                     pvictim->id, -1, "",
                     ACTION_SPY_BRIBE_UNIT);
      /* autoattack might kill us as we move in */
      if (game_unit_by_number(sanity) && punit->moves_left > 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else if (action_prob_possible(action_prob_vs_unit(punit,
                                        ACTION_SPY_SABOTAGE_UNIT_ESC,
                                        pvictim))
               && threat) {
      /* don't stand around waiting for the final blow */
      unit_do_action(pplayer, punit->id,
                     pvictim->id, -1, "",
                     ACTION_SPY_SABOTAGE_UNIT_ESC);
      /* autoattack might kill us as we move in */
      if (game_unit_by_number(sanity) && punit->moves_left > 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else {
      /* usually because we ended move early due to another unit */
      UNIT_LOG(LOG_DIPLOMAT, punit, "could not bribe target (%d, %d), "
               " %d moves left", TILE_XY(pos.tile), punit->moves_left);
      return FALSE;
    }
  } pf_map_positions_iterate_end;

  return (punit->moves_left > 0);
}

/**************************************************************************//**
  If we are the only diplomat in a threatened city, defend against enemy
  actions. The passive defense is set by game.diplchance.  The active
  defense is to bribe units which end their move nearby. Our next trick is
  to look for enemy cities on our continent and do our diplomat things.

  FIXME: It is important to establish contact with all civilizations, so
  we should send diplomats by boat eventually. I just don't know how that
  part of the code works, yet - Per
******************************************************************************/
void dai_manage_diplomat(struct ai_type *ait, struct player *pplayer,
                         struct unit *punit)
{
  struct city *pcity, *ctarget = NULL;
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_position pos;
  struct unit_ai *unit_data;

  CHECK_UNIT(punit);

  /* Generate map */
  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  parameter.get_zoc = NULL; /* kludge */
  parameter.get_TB = no_intermediate_fights;
  pfm = pf_map_new(&parameter);

  pcity = tile_city(unit_tile(punit));

  /* Look for someone to bribe */
  if (!dai_diplomat_bribe_nearby(ait, pplayer, punit, pfm)) {
    /* Died or ran out of moves */
    pf_map_destroy(pfm);
    return;
  }

  /* If we are the only diplomat in a threatened city, then stay to defend */
  pcity = tile_city(unit_tile(punit)); /* we may have moved */
  if (pcity) {
    struct ai_city *city_data = def_ai_city_data(pcity, ait);

    if (count_diplomats_on_tile(unit_tile(punit)) == 1
        && (city_data->diplomat_threat
            || city_data->urgency > 0)) {
      UNIT_LOG(LOG_DIPLOMAT, punit, "stays to protect %s (urg %d)", 
               city_name_get(pcity), city_data->urgency);
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL); /* abort mission */
      def_ai_unit_data(punit, ait)->done = TRUE;
      pf_map_destroy(pfm);
      return;
    }
  }

  unit_data = def_ai_unit_data(punit, ait);

  /* Check if existing target still makes sense */
  if (unit_data->task == AIUNIT_ATTACK
      || unit_data->task == AIUNIT_DEFEND_HOME) {
    bool failure = FALSE;

    ctarget = tile_city(punit->goto_tile);
    if (pf_map_position(pfm, punit->goto_tile, &pos)
        && ctarget) {
      if (same_pos(ctarget->tile, unit_tile(punit))) {
        failure = TRUE;
      } else if (pplayers_allied(pplayer, city_owner(ctarget))
          && unit_data->task == AIUNIT_ATTACK
          && player_has_embassy(pplayer, city_owner(ctarget))) {
        /* We probably incited this city with another diplomat */
        failure = TRUE;
      } else if (!pplayers_allied(pplayer, city_owner(ctarget))
                 && unit_data->task == AIUNIT_DEFEND_HOME) {
        /* We probably lost the city */
        failure = TRUE;
      }
    } else {
      /* City vanished! */
      failure = TRUE;
    }
    if (failure) {
      UNIT_LOG(LOG_DIPLOMAT, punit, "mission aborted");
      dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
    }
  }

  /* We may need a new map now. Both because we cannot get paths from an 
   * old map, and we need paths to move, and because fctd below requires
   * a new map for its iterator. */
  if (!same_pos(parameter.start_tile, unit_tile(punit))
      || unit_data->task == AIUNIT_NONE) {
    pf_map_destroy(pfm);
    pft_fill_unit_parameter(&parameter, punit);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    parameter.get_zoc = NULL; /* kludge */
    parameter.get_TB = no_intermediate_fights;
    pfm = pf_map_new(&parameter);
  }

  /* If we are not busy, acquire a target. */
  if (unit_data->task == AIUNIT_NONE) {
    enum ai_unit_task task;
    int move_dist; /* dummy */

    find_city_to_diplomat(pplayer, punit, &ctarget, &move_dist, pfm);

    if (ctarget) {
      task = AIUNIT_ATTACK;
      aiguard_request_guard(ait, punit);
      UNIT_LOG(LOG_DIPLOMAT, punit, "going on attack");
    } else if ((ctarget = dai_diplomat_defend(ait, pplayer, punit,
                                              unit_type_get(punit), pfm))
               != NULL) {
      task = AIUNIT_DEFEND_HOME;
      UNIT_LOG(LOG_DIPLOMAT, punit, "going to defend %s",
               city_name_get(ctarget));
    } else if ((ctarget = find_closest_city(unit_tile(punit), NULL, pplayer,
                                            TRUE, FALSE, FALSE, TRUE, FALSE,
                                            NULL))
               != NULL) {
      /* This should only happen if the entire continent was suddenly
       * conquered. So we head for closest coastal city and wait for someone
       * to code ferrying for diplomats, or hostile attacks from the sea. */
      task = AIUNIT_DEFEND_HOME;
      UNIT_LOG(LOG_DIPLOMAT, punit, "going idle");
    } else {
      UNIT_LOG(LOG_DIPLOMAT, punit, "could not find a job");
      def_ai_unit_data(punit, ait)->done = TRUE;
      pf_map_destroy(pfm);
      return;
    }

    dai_unit_new_task(ait, punit, task, ctarget->tile);
    fc_assert(punit->moves_left > 0 && ctarget 
              && unit_data->task != AIUNIT_NONE);
  }

  CHECK_UNIT(punit);
  if (ctarget == NULL) {
    UNIT_LOG(LOG_ERROR, punit, "ctarget not set (task == %d)",
             unit_data->task);
    pf_map_destroy(pfm);
    return;
  }

  /* GOTO unless we want to stay */
  if (!same_pos(unit_tile(punit), ctarget->tile)) {
    struct pf_path *path;

    path = pf_map_path(pfm, punit->goto_tile);
    if (path && adv_unit_execute_path(punit, path) && punit->moves_left > 0) {
      /* Check if we can do something with our destination now. */
      if (unit_data->task == AIUNIT_ATTACK) {
        int dist  = real_map_distance(unit_tile(punit), punit->goto_tile);

        UNIT_LOG(LOG_DIPLOMAT, punit, "attack, dist %d to %s",
                 dist, ctarget ? city_name_get(ctarget) : "(none)");
        if (dist <= 1) {
          /* Do our stuff */
          dai_unit_new_task(ait, punit, AIUNIT_NONE, NULL);
          dai_diplomat_city(ait, punit, ctarget);
        }
      }
    }
    pf_path_destroy(path);
  } else {
    def_ai_unit_data(punit, ait)->done = TRUE;
  }
  pf_map_destroy(pfm);
}
