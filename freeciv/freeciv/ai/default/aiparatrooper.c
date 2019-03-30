/***********************************************************************
 Freeciv - Copyright (C) 2005 The Freeciv Team
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
#include "log.h"

/* common */
#include "city.h"
#include "game.h"
#include "player.h"
#include "research.h"
#include "unit.h"
#include "unitlist.h"

/* common/aicore */
#include "pf_tools.h"

/* server */
#include "citytools.h"
#include "maphand.h"
#include "srv_log.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advdata.h"

/* server/generator */
#include "mapgen_utils.h"

/* ai */
#include "handicaps.h"

/* ai/default */
#include "aicity.h"
#include "aidata.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aiunit.h"
#include "aitools.h"

#include "aiparatrooper.h"


#define LOGLEVEL_PARATROOPER LOG_DEBUG

/*************************************************************************//**
  Find best tile the paratrooper should jump to.
*****************************************************************************/
static struct tile *find_best_tile_to_paradrop_to(struct ai_type *ait,
                                                  struct unit *punit)
{
  int best = 0;
  int val;
  struct tile* best_tile = NULL;
  int range = unit_type_get(punit)->paratroopers_range;
  struct city* acity;
  struct player* pplayer = unit_owner(punit);

  /* First, we search for undefended cities in danger */
  square_iterate(&(wld.map), unit_tile(punit), range, ptile) {
    if (!map_is_known(ptile, pplayer)) {
      continue;
    }
  
    acity = tile_city(ptile);
    if (acity && city_owner(acity) == unit_owner(punit)
        && unit_list_size(ptile->units) == 0) {
      val = city_size_get(acity) * def_ai_city_data(acity, ait)->urgency;
      if (val > best) {
	best = val;
	best_tile = ptile;
      }
    }
  } square_iterate_end;
  
  if (best_tile != NULL) {
    acity = tile_city(best_tile);
    UNIT_LOG(LOGLEVEL_PARATROOPER, punit, 
             "Choose to jump in order to protect allied city %s (%d %d). "
             "Benefit: %d",
             city_name_get(acity), TILE_XY(best_tile), best);
    return best_tile;
  }

  /* Second, we search for undefended enemy cities */
  square_iterate(&(wld.map), unit_tile(punit), range, ptile) {
    acity = tile_city(ptile);
    if (acity && pplayers_at_war(unit_owner(punit), city_owner(acity)) &&
        (unit_list_size(ptile->units) == 0)) {
      if (!map_is_known_and_seen(ptile, pplayer, V_MAIN)
          && has_handicap(pplayer, H_FOG)) {
        continue;
      }
      /* Prefer big cities on other continents */
      val = city_size_get(acity)
            + (tile_continent(unit_tile(punit)) != tile_continent(ptile));
      if (val > best) {
        best = val;
	best_tile = ptile;
      }
    }
  } square_iterate_end;
  
  if (best_tile != NULL) {
    acity = tile_city(best_tile);
    UNIT_LOG(LOGLEVEL_PARATROOPER, punit, 
             "Choose to jump into enemy city %s (%d %d). Benefit: %d",
             city_name_get(acity), TILE_XY(best_tile), best);
    return best_tile;
  }

  /* Jump to kill adjacent units */
  square_iterate(&(wld.map), unit_tile(punit), range, ptile) {
    struct terrain *pterrain = tile_terrain(ptile);
    if (is_ocean(pterrain)) {
      continue;
    }
    if (!map_is_known(ptile, pplayer)) {
      continue;
    }
    acity = tile_city(ptile);
    if (acity && !pplayers_allied(city_owner(acity), pplayer)) {
      continue;
    }
    if (!acity && unit_list_size(ptile->units) > 0) {
      continue;
    }
    /* Iterate over adjacent tile to find good victim */
    adjc_iterate(&(wld.map), ptile, target) {
      if (unit_list_size(target->units) == 0
          || !can_unit_attack_tile(punit, target)
          || is_ocean_tile(target)
          || (has_handicap(pplayer, H_FOG)
              && !map_is_known_and_seen(target, pplayer, V_MAIN))) {
        continue;
      }
      val = 0;
      if (is_stack_vulnerable(target)) {
        unit_list_iterate(target->units, victim) {
          if ((!has_handicap(pplayer, H_FOG)
               || can_player_see_unit(pplayer, victim))
              && (unit_attack_unit_at_tile_result(punit, victim, target) == ATT_OK)) {
            val += victim->hp * 100;
          }
        } unit_list_iterate_end;
      } else {
        val += get_defender(punit, target)->hp * 100;
      }
      val *= unit_win_chance(punit, get_defender(punit, target));
      val += pterrain->defense_bonus / 10;
      val -= punit->hp * 100;
      
      if (val > best) {
        best = val;
	best_tile = ptile;
      }
    } adjc_iterate_end;
  } square_iterate_end;

  if (best_tile != NULL) {
    UNIT_LOG(LOG_DEBUG, punit, "Choose to jump at (%d, %d) to attack "
                               "adjacent units. Benefit: %d",
             TILE_XY(best_tile), best);
  }

  return best_tile;
}

/*************************************************************************//**
  This function does manage the paratrooper units of the AI.
*****************************************************************************/
void dai_manage_paratrooper(struct ai_type *ait, struct player *pplayer,
                            struct unit *punit)
{
  struct city *pcity = tile_city(unit_tile(punit));
  struct tile *ptile_dest = NULL;

  int sanity = punit->id;

  /* defend attacking (and be opportunistic too) */
  if (!dai_military_rampage(punit, RAMPAGE_ANYTHING,
                            RAMPAGE_FREE_CITY_OR_BETTER)) {
    /* dead */
    return;
  }

  /* check to recover hit points */
  if ((punit->hp < unit_type_get(punit)->hp / 2) && pcity) {
    UNIT_LOG(LOGLEVEL_PARATROOPER, punit, "Recovering hit points.");
    return;
  }

  /* nothing to do! */
  if (punit->moves_left == 0) {
    return;
  }
  
  if (pcity && unit_list_size(unit_tile(punit)->units) == 1) {
    UNIT_LOG(LOGLEVEL_PARATROOPER, punit, "Defending the city.");
    return;
  }

  if (can_unit_paradrop(punit)) {
    ptile_dest = find_best_tile_to_paradrop_to(ait, punit);

    if (ptile_dest) {
      if (unit_perform_action(unit_owner(punit),
                              punit->id, tile_index(ptile_dest), 0, "",
                              ACTION_PARADROP, ACT_REQ_PLAYER)) {
	/* successfull! */
        if (NULL == game_unit_by_number(sanity)) {
	  /* the unit did not survive the move */
	  return;
	}
	/* and we attack the target */
        (void) dai_military_rampage(punit, RAMPAGE_ANYTHING,
                                    RAMPAGE_ANYTHING);
      }
    }
  } else {
    /* we can't paradrop :-(  */
    struct city *acity = NULL;

    /* we are in a city, so don't try to find another */
    if (pcity) {
      UNIT_LOG(LOGLEVEL_PARATROOPER, punit,
	       "waiting in a city for next turn.");
      return;
    }

    /* find a city to go to recover and paradrop from */
    acity = find_nearest_safe_city(punit);

    if (acity) {
      UNIT_LOG(LOGLEVEL_PARATROOPER, punit, "Going to %s", city_name_get(acity));
      if (!dai_unit_goto(ait, punit, acity->tile)) {
        /* die or unsuccessfull move */
        return;
      }
    } else {
      UNIT_LOG(LOGLEVEL_PARATROOPER, punit,
	       "didn't find city to go and recover.");
      dai_manage_military(ait, pplayer, punit);
    }
  }
}

/*************************************************************************//**
  Evaluate value of the unit.
  Idea: one paratrooper can scare/protect all cities in his range
*****************************************************************************/
static int calculate_want_for_paratrooper(struct unit *punit,
				          struct tile *ptile_city)
{
  struct unit_type* u_type = unit_type_get(punit);
  int range = u_type->paratroopers_range;
  int profit = 0;
  struct player* pplayer = unit_owner(punit);
  int total, total_cities;

  profit += u_type->defense_strength
          + u_type->move_rate 
	  + u_type->attack_strength;
  
  square_iterate(&(wld.map), ptile_city, range, ptile) {
    int multiplier;
    struct city *pcity = tile_city(ptile);
    
    if (!pcity) {
      continue;
    }

    if (!map_is_known(ptile, pplayer)) {
      continue;
    }

    /* We prefer jumping to other continents. On the same continent we 
     * can fight traditionally.
     * FIXME: Handle ocean cities we can attack. */    
    if (!is_ocean_tile(ptile)
        && tile_continent(ptile_city) != tile_continent(ptile)) {
      if (get_continent_size(tile_continent(ptile)) < 3) {
        /* Tiny island are hard to conquer with traditional units */
        multiplier = 10;
      } else {
        multiplier = 5;
      }
    } else {
      multiplier = 1;
    }
    
    /* There are lots of units, the city will be safe against paratroopers. */
    if (unit_list_size(ptile->units) > 2) {
      continue;
    }
    
    /* Prefer long jumps.
     * If a city is near we can take/protect it with normal units */
    if (pplayers_allied(pplayer, city_owner(pcity))) {
      profit += city_size_get(pcity)
                * multiplier * real_map_distance(ptile_city, ptile) / 2;
    } else {

      profit += city_size_get(pcity) * multiplier
                * real_map_distance(ptile_city, ptile);
    }
  } square_iterate_end;

  total = adv_data_get(pplayer, NULL)->stats.units.paratroopers;
  total_cities = city_list_size(pplayer->cities);

  if (total > total_cities) {
    profit = profit * total_cities / total;
  }

  return profit;
}

/*************************************************************************//**
  Chooses to build a paratroopers if necessary
*****************************************************************************/
void dai_choose_paratrooper(struct ai_type *ait,
                            struct player *pplayer, struct city *pcity,
                            struct adv_choice *choice, bool allow_gold_upkeep)
{
  const struct research *presearch;
  int profit;
  Tech_type_id tech_req;
  Tech_type_id requirements[A_LAST];
  int num_requirements = 0;
  int i;
  struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

  /* military_advisor_choose_build does something idiotic,
   * this function should not be called if there is danger... */
  if (choice->want >= 100 && choice->type != CT_ATTACKER) {
    return;
  }

  unit_type_iterate(u_type) {
    struct unit *virtual_unit;

    if (!utype_can_do_action(u_type, ACTION_PARADROP)) {
      continue;
    }
    if (A_NEVER == u_type->require_advance) {
      continue;
    }

    if (!allow_gold_upkeep && utype_upkeep_cost(u_type, pplayer, O_GOLD) > 0) {
      continue;
    }

    /* Temporary hack because pathfinding can't handle Fighters. */
    if (!utype_can_do_action(u_type, ACTION_SUICIDE_ATTACK)
        && 1 == utype_fuel(u_type)) {
      continue;
    }

    /* assign tech for paratroopers */
    tech_req = advance_index(u_type->require_advance);
    if (tech_req != A_NONE && tech_req != A_UNSET) {
      for (i = 0; i < num_requirements; i++) {
        if (requirements[i] == tech_req) {
	  break;
	}
      }
      if (i == num_requirements) {
        requirements[i++] = tech_req;
      }
    }

    /* we only update choice struct if we can build it! */
    if (!can_city_build_unit_now(pcity, u_type)) {
      continue;
    }

    /* it's worth building that unit? */
    virtual_unit = unit_virtual_create(pplayer, pcity, u_type,
                                       do_make_unit_veteran(pcity, u_type));
    profit = calculate_want_for_paratrooper(virtual_unit, pcity->tile);
    unit_virtual_destroy(virtual_unit);

    /* update choice struct if it's worth */
    if (profit > choice->want) {
      /* Update choice */
      choice->want = profit;
      choice->value.utype = u_type;
      choice->type = CT_ATTACKER;
      choice->need_boat = FALSE;
      adv_choice_set_use(choice, "paratrooper");
      log_base(LOGLEVEL_PARATROOPER, "%s wants to build %s (want=%d)",
               city_name_get(pcity), utype_rule_name(u_type), profit);
    }
  } unit_type_iterate_end;

  /* we raise want if the required tech is not known */
  presearch = research_get(pplayer);
  for (i = 0; i < num_requirements; i++) {
    tech_req = requirements[i];
    plr_data->tech_want[tech_req] += 2;
    log_base(LOGLEVEL_PARATROOPER, "Raising tech want in city %s for %s "
             "stimulating %s with %d (" ADV_WANT_PRINTF ") and req",
             city_name_get(pcity),
             player_name(pplayer),
             advance_rule_name(advance_by_number(tech_req)),
             2,
             plr_data->tech_want[tech_req]);

    /* now, we raise want for prerequisites */
    advance_index_iterate(A_FIRST, k) {
      if (research_goal_tech_req(presearch, tech_req, k)) {
        plr_data->tech_want[k] += 1;
      }
    } advance_index_iterate_end;
  }
  return;
}
