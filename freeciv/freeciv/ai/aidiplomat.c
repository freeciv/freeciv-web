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

#include "city.h"
#include "combat.h"
#include "game.h"
#include "government.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "movement.h"
#include "packets.h"
#include "path_finding.h"
#include "pf_tools.h"
#include "player.h"
#include "shared.h"
#include "timing.h"
#include "unit.h"
#include "unitlist.h"

#include "barbarian.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplomats.h"
#include "maphand.h"
#include "settlers.h"
#include "unithand.h"
#include "unittools.h"

#include "advmilitary.h"
#include "aicity.h"
#include "aidata.h"
#include "aiguard.h"
#include "aihand.h"
#include "ailog.h"
#include "aitools.h"
#include "aiunit.h"

#include "aidiplomat.h"

#define LOG_DIPLOMAT LOG_DEBUG
#define LOG_DIPLOMAT_BUILD LOG_DEBUG

/* 3000 is a just a large number, but not hillariously large as the
 * previously used one. This is important for diplomacy. - Per */
#define DIPLO_DEFENSE_WANT 3000

static void find_city_to_diplomat(struct player *pplayer, struct unit *punit,
                                  struct city **ctarget, int *move_dist,
                                  struct pf_map *pfm);

/******************************************************************************
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

/******************************************************************************
  Number of techs that we don't have and the enemy (tplayer) does.
******************************************************************************/
static int count_stealable_techs(struct player *pplayer, struct player *tplayer)
{
  int count = 0;

  advance_index_iterate(A_FIRST, index) {
    if ((player_invention_state(pplayer, index) != TECH_KNOWN)
        && (player_invention_state(tplayer, index) == TECH_KNOWN)) {
      count++;
    }
  } advance_index_iterate_end;

  return count;
}

/**********************************************************************
  Calculates our need for diplomats as defensive units. May replace
  values in choice. The values 16000 and 3000 used below are totally
  arbitrary but seem to work.
***********************************************************************/
void ai_choose_diplomat_defensive(struct player *pplayer,
                                  struct city *pcity,
                                  struct ai_choice *choice, int def)
{
  /* Build a diplomat if our city is threatened by enemy diplomats, and
     we have other defensive troops, and we don't already have a diplomat
     to protect us. If we see an enemy diplomat and we don't have diplomat
     tech... race it! */
  if (def != 0 && pcity->ai->diplomat_threat && !pcity->ai->has_diplomat) {
    struct unit_type *ut = best_role_unit(pcity, F_DIPLOMAT);

    if (ut) {
       freelog(LOG_DIPLOMAT_BUILD, 
               "A defensive diplomat will be built in city %s.",
               city_name(pcity));
       choice->want = 16000; /* diplomat more important than soldiers */
       pcity->ai->urgency = 1;
       choice->type = CT_DEFENDER;
       choice->value.utype = ut;
       choice->need_boat = FALSE;
    } else if (num_role_units(F_DIPLOMAT) > 0) {
      /* We don't know diplomats yet... */
      freelog(LOG_DIPLOMAT_BUILD,
              "A defensive diplomat is wanted badly in city %s.",
              city_name(pcity));
      ut = get_role_unit(F_DIPLOMAT, 0);
      if (ut) {
        pplayer->ai_data.tech_want[advance_index(ut->require_advance)]
          += DIPLO_DEFENSE_WANT;
        TECH_LOG(LOG_DEBUG, pplayer, ut->require_advance,
                 "ai_choose_diplomat_defensive() + %d for %s",
                 DIPLO_DEFENSE_WANT,
                 utype_rule_name(ut));
      }
    }
  }
}

/**********************************************************************
  Calculates our need for diplomats as offensive units. May replace
  values in choice.
***********************************************************************/
void ai_choose_diplomat_offensive(struct player *pplayer,
                                  struct city *pcity,
                                  struct ai_choice *choice)
{
  struct unit_type *ut = best_role_unit(pcity, F_DIPLOMAT);
  struct ai_data *ai = ai_data_get(pplayer);

  if (!ut) {
    /* We don't know diplomats yet! */
    return;
  }

  if (ai_handicap(pplayer, H_DIPLOMAT)) {
    /* Diplomats are too tough on newbies */
    return;
  }

  /* Do we have a good reason for building diplomats? */
  {
    struct pf_map *pfm;
    struct pf_parameter parameter;
    struct city *acity;
    int want, loss, p_success, p_failure, time_to_dest;
    int gain_incite = 0, gain_theft = 0, gain = 1;
    int incite_cost;
    struct unit *punit = create_unit_virtual(pplayer, pcity, ut,
                                             do_make_unit_veteran(pcity, ut));

    pft_fill_unit_parameter(&parameter, punit);
    pfm = pf_map_new(&parameter);

    find_city_to_diplomat(pplayer, punit, &acity, &time_to_dest, pfm);

    pf_map_destroy(pfm);
    destroy_unit_virtual(punit);

    if (acity == NULL
	|| BV_ISSET(ai->stats.diplomat_reservations, acity->id)) {
      /* Found no target or city already considered */
      return;
    }
    incite_cost = city_incite_cost(pplayer, acity);
    if (HOSTILE_PLAYER(pplayer, ai, city_owner(acity))
        && (incite_cost < INCITE_IMPOSSIBLE_COST)
        && (incite_cost < pplayer->economic.gold - pplayer->ai_data.est_upkeep)) {
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
    if ((get_player_research(city_owner(acity))->techs_researched
	 < get_player_research(pplayer)->techs_researched)
	&& !pplayers_allied(pplayer, city_owner(acity))) {
      /* tech theft gain */
      gain_theft = total_bulbs_required(pplayer) * TRADE_WEIGHTING;
    }
    gain = MAX(gain_incite, gain_theft);
    loss = utype_build_shield_cost(ut) * SHIELD_WEIGHTING;

    /* Probability to succeed, assuming no defending diplomat */
    p_success = game.info.diplchance;
    /* Probability to lose our unit */
    p_failure = (utype_has_flag(ut, F_SPY) ? 100 - p_success : 100);

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
                             utype_build_shield_cost(ut));

    if (!player_has_embassy(pplayer, city_owner(acity))
        && want < 99) {
        freelog(LOG_DIPLOMAT_BUILD,
                "A diplomat desired in %s to establish an embassy with %s "
                "in %s",
                city_name(pcity),
                player_name(city_owner(acity)),
                city_name(acity));
        want = 99;
    }
    if (want > choice->want) {
      freelog(LOG_DIPLOMAT_BUILD,
              "%s, %s: %s is desired with want %d to spy in %s (incite "
              "want %d cost %d gold %d, tech theft want %d, ttd %d)",
              player_name(pplayer),
              city_name(pcity),
              utype_rule_name(ut),
              want,
              city_name(acity),
              gain_incite,
              incite_cost,
              pplayer->economic.gold - pplayer->ai_data.est_upkeep,
              gain_theft,
              time_to_dest);
      choice->want = want;
      choice->type = CT_CIVILIAN; /* so we don't build barracks for it */
      choice->value.utype = ut;
      choice->need_boat = FALSE;
      BV_SET(ai->stats.diplomat_reservations, acity->id);
    }
  }
}

/**************************************************************************
  Check if something is on our receiving end for some nasty diplomat
  business! Note that punit may die or be moved during this function. We
  must be adjacent to target city.

  We try to make embassy first, and abort if we already have one and target
  is allied. Then we steal, incite, sabotage or poison the city, in that
  order of priority.
**************************************************************************/
static void ai_diplomat_city(struct unit *punit, struct city *ctarget)
{
  struct player *pplayer = unit_owner(punit);
  struct player *tplayer = city_owner(ctarget);
  int count_impr = count_sabotagable_improvements(ctarget);
  int count_tech = count_stealable_techs(pplayer, tplayer);
  int gold_avail = pplayer->economic.gold - 2 * pplayer->ai_data.est_upkeep;
  int incite_cost;

  assert(pplayer->ai_data.control);

  if (punit->moves_left == 0) {
    UNIT_LOG(LOG_ERROR, punit, "no moves left in ai_diplomat_city()!");
  }

  unit_activity_handling(punit, ACTIVITY_IDLE);

#define T(my_act,my_val)                                            \
  if (diplomat_can_do_action(punit, my_act, ctarget->tile)) {       \
    freelog(LOG_DIPLOMAT, "%s %s[%d] does " #my_act " at %s",       \
            nation_rule_name(nation_of_unit(punit)),                \
            unit_rule_name(punit), punit->id, city_name(ctarget));  \
    handle_unit_diplomat_action(pplayer, punit->id,                 \
                                ctarget->id, my_val, my_act);       \
    return;                                                         \
  }

  T(DIPLOMAT_EMBASSY,0);

  if (pplayers_allied(pplayer, tplayer)) {
    return; /* Don't do the rest to allies */
  }

  if (count_tech > 0 
      && (ctarget->steal == 0 || unit_has_type_flag(punit, F_SPY))) {
    T(DIPLOMAT_STEAL,0);
  } else {
    UNIT_LOG(LOG_DIPLOMAT, punit, "We have already stolen from %s!",
             city_name(ctarget));
  }

  incite_cost = city_incite_cost(pplayer, ctarget);
  if (incite_cost <= gold_avail) {
    T(DIPLOMAT_INCITE,0);
  } else {
    UNIT_LOG(LOG_DIPLOMAT, punit, "%s too expensive!",
             city_name(ctarget));
  }

  if (!pplayers_at_war(pplayer, tplayer)) {
    return; /* The rest are casus belli */
  }
  if (count_impr > 0) T(DIPLOMAT_SABOTAGE, B_LAST+1);
  T(SPY_POISON, 0); /* absolutely last resort */
#undef T

  /* This can happen for a number of odd and esoteric reasons  */
  UNIT_LOG(LOG_DIPLOMAT, punit,
           "decides to stand idle outside enemy city %s!",
           city_name(ctarget));
  ai_unit_new_role(punit, AIUNIT_NONE, NULL);
}

/**************************************************************************
  Returns (in ctarget) the closest city to send diplomats against, or NULL 
  if none available on this continent.  punit can be virtual.
**************************************************************************/
static void find_city_to_diplomat(struct player *pplayer, struct unit *punit,
                                  struct city **ctarget, int *move_dist,
                                  struct pf_map *pfm)
{
  bool has_embassy;
  int incite_cost = 0; /* incite cost */
  bool dipldef; /* whether target is protected by diplomats */

  assert(punit != NULL);
  *ctarget = NULL;
  *move_dist = -1;

  pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
    struct city *acity;
    struct player *aplayer;
    bool can_incite;

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
    can_incite = (incite_cost < INCITE_IMPOSSIBLE_COST);

    dipldef = (count_diplomats_on_tile(acity->tile) > 0);
    /* Three actions to consider:
     * 1. establishing embassy OR
     * 2. stealing techs OR
     * 3. inciting revolt */
    if (!has_embassy
        || (acity->steal == 0
	    && (get_player_research(pplayer)->techs_researched
		< get_player_research(aplayer)->techs_researched)
	    && !dipldef)
        || (incite_cost < (pplayer->economic.gold - pplayer->ai_data.est_upkeep)
            && can_incite && !dipldef)) {
      /* We have the closest enemy city on the continent */
      *ctarget = acity;
      *move_dist = move_cost;
      break;
    }
  } pf_map_iterate_move_costs_end;
}

/**************************************************************************
  Go to nearest/most threatened city (can be the current city too).
**************************************************************************/
static struct city *ai_diplomat_defend(struct player *pplayer,
                                       struct unit *punit,
                                       const struct unit_type *utype,
				       struct pf_map *pfm)
{
  int best_dist = 30; /* any city closer than this is better than none */
  int best_urgency = 0;
  struct city *ctarget = NULL;
  struct city *pcity = tile_city(punit->tile);

  if (pcity 
      && count_diplomats_on_tile(pcity->tile) == 1
      && pcity->ai->urgency > 0) {
    /* Danger and we are only diplomat present - stay. */
    return pcity;
  }

  pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
    struct city *acity;
    struct player *aplayer;
    int dipls, urgency;

    acity = tile_city(ptile);
    if (!acity) {
      continue;
    }
    aplayer = city_owner(acity);
    if (aplayer != pplayer) {
      continue;
    }

    urgency = acity->ai->urgency;
    dipls = (count_diplomats_on_tile(ptile)
             - (same_pos(ptile, punit->tile) ? 1 : 0));
    if (dipls == 0 && acity->ai->diplomat_threat) {
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
  } pf_map_iterate_move_costs_end;

  return ctarget;
}

/**************************************************************************
  Find units that we can reach, and bribe them. Returns TRUE if survived
  the ordeal, FALSE if not or we expended all our movement.
  Will try to bribe a ship on the coast as well as land stuff.
**************************************************************************/
static bool ai_diplomat_bribe_nearby(struct player *pplayer, 
                                     struct unit *punit, struct pf_map *pfm)
{
  int gold_avail = pplayer->economic.gold - pplayer->ai_data.est_upkeep;
  struct ai_data *ai = ai_data_get(pplayer);

  pf_map_iterate_positions(pfm, pos, FALSE) {
    struct tile *ptile = pos.tile;
    bool threat = FALSE;
    int newval, bestval = 0, cost;
    struct unit *pvictim = unit_list_get(ptile->units, 0);
    int sanity = punit->id;

    if (pos.total_MC > punit->moves_left) {
      /* Didn't find anything within range. */
      break;
    }

    if (!pvictim
        || !HOSTILE_PLAYER(pplayer, ai, unit_owner(pvictim))
        || unit_list_size(ptile->units) > 1
        || (tile_city(ptile)
         && get_city_bonus(tile_city(ptile), EFT_NO_INCITE) > 0)
        || get_player_bonus(unit_owner(pvictim), EFT_NO_INCITE) > 0) {
      continue;
    }

    /* Calculate if enemy is a threat */
    /* First find best defender on our tile */
    unit_list_iterate(ptile->units, aunit) {
      newval = DEFENCE_POWER(aunit);
      if (bestval < newval) {
        bestval = newval;
      }
    } unit_list_iterate_end;
    /* Compare with victim's attack power */
    newval = ATTACK_POWER(pvictim);
    if (newval > bestval
        && unit_move_rate(pvictim) > pos.total_MC) {
      /* Enemy can probably kill us */
      threat = TRUE;
    } else {
      /* Enemy cannot reach us or probably not kill us */
      threat = FALSE;
    }
    /* Don't bribe settlers! */
    if (unit_has_type_flag(pvictim, F_SETTLERS)
        || unit_has_type_flag(pvictim, F_CITIES)) {
      continue;
    }
    /* Should we make the expense? */
    cost = unit_bribe_cost(pvictim);
    if (!threat) {
      /* Don't empty our treasure without good reason! */
      gold_avail = pplayer->economic.gold - ai_gold_reserve(pplayer);
    }
    if (cost > gold_avail) {
      /* Can't afford */
      continue;
    }

    /* Found someone! */
    {
      struct tile *ptile;
      struct pf_path *path;

      ptile = mapstep(pos.tile, DIR_REVERSE(pos.dir_to_here));
      path = pf_map_get_path(pfm, ptile);
      if (!path || !ai_unit_execute_path(punit, path) 
          || punit->moves_left <= 0) {
        pf_path_destroy(path);
        return FALSE;
      }
      pf_path_destroy(path);
    }

    if (diplomat_can_do_action(punit, DIPLOMAT_BRIBE, pos.tile)) {
      handle_unit_diplomat_action(pplayer, punit->id,
				  unit_list_get(ptile->units, 0)->id, -1,
				  DIPLOMAT_BRIBE);
      /* autoattack might kill us as we move in */
      if (game_find_unit_by_number(sanity) && punit->moves_left > 0) {
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
  } pf_map_iterate_positions_end;

  return (punit->moves_left > 0);
}

/**************************************************************************
  If we are the only diplomat in a threatened city, defend against enemy 
  actions. The passive defense is set by game.diplchance.  The active 
  defense is to bribe units which end their move nearby. Our next trick is 
  to look for enemy cities on our continent and do our diplomat things.

  FIXME: It is important to establish contact with all civilizations, so
  we should send diplomats by boat eventually. I just don't know how that
  part of the code works, yet - Per
**************************************************************************/
void ai_manage_diplomat(struct player *pplayer, struct unit *punit)
{
  struct city *pcity, *ctarget = NULL;
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_position pos;

  CHECK_UNIT(punit);

  /* Generate map */
  pft_fill_unit_parameter(&parameter, punit);
  parameter.get_zoc = NULL; /* kludge */
  pfm = pf_map_new(&parameter);

  pcity = tile_city(punit->tile);

  /* Look for someone to bribe */
  if (!ai_diplomat_bribe_nearby(pplayer, punit, pfm)) {
    /* Died or ran out of moves */
    pf_map_destroy(pfm);
    return;
  }

  /* If we are the only diplomat in a threatened city, then stay to defend */
  pcity = tile_city(punit->tile); /* we may have moved */
  if (pcity && count_diplomats_on_tile(punit->tile) == 1
      && (pcity->ai->diplomat_threat || pcity->ai->urgency > 0)) {
    UNIT_LOG(LOG_DIPLOMAT, punit, "stays to protect %s (urg %d)", 
             city_name(pcity), pcity->ai->urgency);
    ai_unit_new_role(punit, AIUNIT_NONE, NULL); /* abort mission */
    punit->ai.done = TRUE;
    pf_map_destroy(pfm);
    return;
  }

  /* Check if existing target still makes sense */
  if (punit->ai.ai_role == AIUNIT_ATTACK
      || punit->ai.ai_role == AIUNIT_DEFEND_HOME) {
    bool failure = FALSE;

    ctarget = tile_city(punit->goto_tile);
    if (pf_map_get_position(pfm, punit->goto_tile, &pos)
        && ctarget) {
      if (same_pos(ctarget->tile, punit->tile)) {
        failure = TRUE;
      } else if (pplayers_allied(pplayer, city_owner(ctarget))
          && punit->ai.ai_role == AIUNIT_ATTACK
          && player_has_embassy(pplayer, city_owner(ctarget))) {
        /* We probably incited this city with another diplomat */
        failure = TRUE;
      } else if (!pplayers_allied(pplayer, city_owner(ctarget))
                 && punit->ai.ai_role == AIUNIT_DEFEND_HOME) {
        /* We probably lost the city */
        failure = TRUE;
      }
    } else {
      /* City vanished! */
      failure = TRUE;
    }
    if (failure) {
      UNIT_LOG(LOG_DIPLOMAT, punit, "mission aborted");
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    }
  }

  /* We may need a new map now. Both because we cannot get paths from an 
   * old map, and we need paths to move, and because fctd below requires
   * a new map for its iterator. */
  if (!same_pos(parameter.start_tile, punit->tile)
      || punit->ai.ai_role == AIUNIT_NONE) {
    pf_map_destroy(pfm);
    pft_fill_unit_parameter(&parameter, punit);
    parameter.get_zoc = NULL; /* kludge */
    pfm = pf_map_new(&parameter);
  }

  /* If we are not busy, acquire a target. */
  if (punit->ai.ai_role == AIUNIT_NONE) {
    enum ai_unit_task task;
    int move_dist; /* dummy */

    find_city_to_diplomat(pplayer, punit, &ctarget, &move_dist, pfm);

    if (ctarget) {
      task = AIUNIT_ATTACK;
      aiguard_request_guard(punit);
      UNIT_LOG(LOG_DIPLOMAT, punit, "going on attack");
    } else if ((ctarget = ai_diplomat_defend(pplayer, punit,
                                             unit_type(punit), pfm)) != NULL) {
      task = AIUNIT_DEFEND_HOME;
      UNIT_LOG(LOG_DIPLOMAT, punit, "going to defend %s",
               city_name(ctarget));
    } else if ((ctarget = find_closest_owned_city(pplayer, punit->tile, 
						  TRUE, NULL)) != NULL) {
      /* This should only happen if the entire continent was suddenly
       * conquered. So we head for closest coastal city and wait for someone
       * to code ferrying for diplomats, or hostile attacks from the sea. */
      task = AIUNIT_DEFEND_HOME;
      UNIT_LOG(LOG_DIPLOMAT, punit, "going idle");
    } else {
      UNIT_LOG(LOG_DIPLOMAT, punit, "could not find a job");
      punit->ai.done = TRUE;
      pf_map_destroy(pfm);
      return;
    }

    ai_unit_new_role(punit, task, ctarget->tile);
    assert(punit->moves_left > 0 && ctarget 
           && punit->ai.ai_role != AIUNIT_NONE);
  }

  CHECK_UNIT(punit);
  if (ctarget == NULL) {
    UNIT_LOG(LOG_ERROR, punit, "ctarget not set (role==%d)",
	     punit->ai.ai_role);
    pf_map_destroy(pfm);
    return;
  }

  /* GOTO unless we want to stay */
  if (!same_pos(punit->tile, ctarget->tile)) {
    struct pf_path *path;

    path = pf_map_get_path(pfm, punit->goto_tile);
    if (path && ai_unit_execute_path(punit, path) && punit->moves_left > 0) {
      /* Check if we can do something with our destination now. */
      if (punit->ai.ai_role == AIUNIT_ATTACK) {
        int dist  = real_map_distance(punit->tile, punit->goto_tile);
        UNIT_LOG(LOG_DIPLOMAT, punit, "attack, dist %d to %s",
                 dist, ctarget ? city_name(ctarget) : "(none)");
        if (dist == 1) {
          /* Do our stuff */
          ai_unit_new_role(punit, AIUNIT_NONE, NULL);
          ai_diplomat_city(punit, ctarget);
        }
      }
    }
    pf_path_destroy(path);
  } else {
    punit->ai.done = TRUE;
  }
  pf_map_destroy(pfm);
}
