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
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "combat.h"
#include "effects.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"

/* aicore */
#include "pf_tools.h"

/* server */
#include "cityhand.h"
#include "citytools.h"
#include "cityturn.h"
#include "gotohand.h"
#include "notify.h"
#include "plrhand.h"
#include "settlers.h"
#include "srv_main.h"
#include "unithand.h"
#include "unittools.h"

/* ai */
#include "advdomestic.h"
#include "advmilitary.h"
#include "aidata.h"
#include "aihand.h"
#include "ailog.h"
#include "aitools.h"
#include "aiunit.h"

#include "aicity.h"

#define SPECVEC_TAG tech
#define SPECVEC_TYPE struct advance *
#include "specvec.h"

#define SPECVEC_TAG impr
#define SPECVEC_TYPE struct impr_type *
#include "specvec.h"

#define LOG_BUY LOG_DEBUG
#define LOG_EMERGENCY LOG_VERBOSE
#define LOG_WANT LOG_VERBOSE

/* Iterate over cities within a certain range around a given city
 * (city_here) that exist within a given city list. */
#define city_range_iterate(city_here, list, range, city)		\
{									\
  city_list_iterate(list, city) {					\
    if (range == REQ_RANGE_PLAYER					\
     || ((range == REQ_RANGE_CITY || range == REQ_RANGE_LOCAL)		\
      && city == city_here)						\
     || (range == REQ_RANGE_CONTINENT					\
      && tile_continent(city->tile) ==					\
	 tile_continent(city_here->tile))) {

#define city_range_iterate_end						\
    }									\
  } city_list_iterate_end;						\
}

#define CITY_EMERGENCY(pcity)						\
 (pcity->surplus[O_SHIELD] < 0 || city_unhappy(pcity)			\
  || pcity->food_stock + pcity->surplus[O_FOOD] < 0)

static void ai_sell_obsolete_buildings(struct city *pcity);
static void resolve_city_emergency(struct player *pplayer, struct city *pcity);

/**************************************************************************
  Return the number of "luxury specialists".  This is the number of
  specialists who provide at least HAPPY_COST luxury, being the number of
  luxuries needed to make one citizen content or happy.

  The AI assumes that for any specialist that provides HAPPY_COST luxury, 
  if we can get that luxury from some other source it allows the specialist 
  to become a worker.  The benefits from an extra worker are weighed against
  the losses from acquiring the two extra luxury.

  This is a very bad model if the abilities of specialists are changed.
  But as long as the civ2 model of specialists is used it will continue
  to work okay.
**************************************************************************/
static int get_entertainers(const struct city *pcity)
{
  int providers = 0;

  specialist_type_iterate(i) {
    if (get_specialist_output(pcity, i, O_LUXURY) >= game.info.happy_cost) {
      providers += pcity->specialists[i];
    }
  } specialist_type_iterate_end;

  return providers;
}

/**************************************************************************
  This calculates the usefulness of pcity to us. Note that you can pass
  another player's ai_data structure here for evaluation by different
  priorities.
**************************************************************************/
int ai_eval_calc_city(struct city *pcity, struct ai_data *ai)
{
  int i = (pcity->surplus[O_FOOD] * ai->food_priority
           + pcity->surplus[O_SHIELD] * ai->shield_priority
           + pcity->prod[O_LUXURY] * ai->luxury_priority
           + pcity->prod[O_GOLD] * ai->gold_priority
           + pcity->prod[O_SCIENCE] * ai->science_priority
           + pcity->feel[CITIZEN_HAPPY][FEELING_FINAL] * ai->happy_priority
           - pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] * ai->unhappy_priority
           - pcity->feel[CITIZEN_ANGRY][FEELING_FINAL] * ai->angry_priority
           - pcity->pollution * ai->pollution_priority);

  if (pcity->surplus[O_FOOD] < 0 || pcity->surplus[O_SHIELD] < 0) {
    /* The city is unmaintainable, it can't be good */
    i = MIN(i, 0);
  }

  return i;
}

/**************************************************************************
  Calculates city want from some input values.  Set pimprove to NULL when
  nothing in the city has changed, and you just want to know the
  base want of a city.
**************************************************************************/
static inline int city_want(struct player *pplayer, struct city *acity, 
                            struct ai_data *ai, struct impr_type *pimprove)
{
  int want = 0, prod[O_LAST], bonus[O_LAST], waste[O_LAST], i;

  memset(prod, 0, O_LAST * sizeof(*prod));
  if (NULL != pimprove
   && ai->impr_calc[improvement_index(pimprove)] == AI_IMPR_CALCULATE_FULL) {
    struct tile *acenter = city_tile(acity);
    bool celebrating = base_city_celebrating(acity);

    /* The below calculation mostly duplicates get_worked_tile_output(). 
     * We do this only for buildings that we know may change tile
     * outputs. */
    city_tile_iterate(acenter, ptile) {
      if (tile_worked(ptile) == acity) {
        output_type_iterate(o) {
          prod[o] += city_tile_output(acity, ptile, celebrating, o);
        } output_type_iterate_end;
      }
    } city_tile_iterate_end;

    add_specialist_output(acity, prod);
  } else {
    assert(sizeof(*prod) == sizeof(*acity->citizen_base));
    memcpy(prod, acity->citizen_base, O_LAST * sizeof(*prod));
  }

  for (i = 0; i < NUM_TRADEROUTES; i++) {
    prod[O_TRADE] += acity->trade_value[i];
  }
  prod[O_GOLD] += get_city_tithes_bonus(acity);
  output_type_iterate(o) {
    bonus[o] = get_final_city_output_bonus(acity, o);
    waste[o] = city_waste(acity, o, prod[o] * bonus[o] / 100);
  } output_type_iterate_end;
  add_tax_income(pplayer,
		 prod[O_TRADE] * bonus[O_TRADE] / 100 - waste[O_TRADE],
		 prod);
  output_type_iterate(o) {
    prod[o] = prod[o] * bonus[o] / 100 - waste[o];
  } output_type_iterate_end;

  city_built_iterate(acity, pimprove) {
    prod[O_GOLD] -= city_improvement_upkeep(acity, pimprove);
  } city_built_iterate_end;
  /* Unit upkeep isn't handled here.  Unless we do a full city_refresh it
   * won't be changed anyway. */

  want += prod[O_FOOD] * ai->food_priority;
  if (prod[O_SHIELD] != 0) {
    want += prod[O_SHIELD] * ai->shield_priority;
    want -= city_pollution(acity, prod[O_SHIELD]) * ai->pollution_priority;
  }
  want += prod[O_LUXURY] * ai->luxury_priority;
  want += prod[O_SCIENCE] * ai->science_priority;
  want += prod[O_GOLD] * ai->gold_priority;

  return want;
}

/**************************************************************************
  Calculates want for some buildings by actually adding the building and
  measuring the effect.
**************************************************************************/
static int base_want(struct player *pplayer, struct city *pcity, 
                     struct impr_type *pimprove)
{
  struct ai_data *ai = ai_data_get(pplayer);
  int final_want = 0;
  int wonder_player_id = WONDER_NOT_OWNED;
  int wonder_city_id = WONDER_NOT_BUILT;

  if (ai->impr_calc[improvement_index(pimprove)] == AI_IMPR_ESTIMATE) {
    return 0; /* Nothing to calculate here. */
  }

  if (!can_city_build_improvement_now(pcity, pimprove)
      || (is_small_wonder(pimprove)
          && find_city_from_small_wonder(pplayer, pimprove))) {
    return 0;
  }

  if (is_wonder(pimprove)) {
    if (is_great_wonder(pimprove)) {
      wonder_player_id =
          game.info.great_wonder_owners[improvement_index(pimprove)];
    }
    wonder_city_id = pplayer->wonders[improvement_index(pimprove)];
  }
  /* Add the improvement */
  city_add_improvement(pcity, pimprove);

  /* Stir, then compare notes */
  city_range_iterate(pcity, pplayer->cities, ai->impr_range[improvement_index(pimprove)], acity) {
    final_want += city_want(pplayer, acity, ai, pimprove) - acity->ai->worth;
  } city_range_iterate_end;

  /* Restore */
  city_remove_improvement(pcity, pimprove);
  if (is_wonder(pimprove)) {
    if (is_great_wonder(pimprove)) {
      game.info.great_wonder_owners[improvement_index(pimprove)] =
          wonder_player_id;
    }

    pplayer->wonders[improvement_index(pimprove)] = wonder_city_id;
  }

  return final_want;
}

/************************************************************************** 
  Increase want for a technology because of the value of that technology
  in providing an improvement effect.

  The input building_want gives the desire for the improvement;
  the value may be negative for technologies that produce undesirable
  effects.

  This function must convert from units of 'building want' to 'tech want'.
  We put this conversion in a function because the 'want' scales are
  unclear and kludged. Consequently, this conversion might require tweaking.
**************************************************************************/
static void want_tech_for_improvement_effect(struct player *pplayer,
					     const struct city *pcity,
					     const struct impr_type *pimprove,
					     const struct advance *tech,
					     int building_want)
{
  /* The conversion factor was determined by experiment,
   * and might need adjustment.
   */
  const int tech_want = building_want * pcity->ai->building_wait * 14 / 8;
#if 0
  /* This logging is relatively expensive,
   * so activate it only while necessary. */
  TECH_LOG(LOG_DEBUG, pplayer, tech,
    "wanted by %s for %s: %d -> %d",
    city_name(pcity), improvement_rule_name(pimprove),
    building_want, tech_want);
#endif
  if (tech) {
    pplayer->ai_data.tech_want[advance_index(tech)] += tech_want;
  }
}

/************************************************************************** 
  How desirable particular effect making people content is for a
  particular city?
**************************************************************************/
static int content_effect_value(const struct player *pplayer,
                                const struct city *pcity,
                                int amount,
                                int num_cities,
                                int happiness_step)
{
  int v = 0;

  if (get_city_bonus(pcity, EFT_NO_UNHAPPY) <= 0) {
    int i;
    int max_converted = pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL];

    /* See if some step of happiness calculation gets capped */
    for (i = happiness_step; i < FEELING_FINAL; i++) {
      max_converted = MIN(max_converted, pcity->feel[CITIZEN_UNHAPPY][i]);
    }

    v = MIN(amount, max_converted + get_entertainers(pcity)) * 35;
  }

  if (num_cities > 1) {
    int factor = 2;

    /* Try to build wonders to offset empire size unhappiness */
    if (city_list_size(pplayer->cities) 
        > get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE)) {
      if (get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE) > 0) {
        factor += city_list_size(pplayer->cities) 
          / MAX(get_player_bonus(pplayer, EFT_EMPIRE_SIZE_STEP), 1);
      }
      factor += 2;
    }
    v += factor * num_cities * amount;
  }

  return v;
}

/**************************************************************************
  Unit class affected by this effect.
**************************************************************************/
static struct unit_class *affected_unit_class(const struct effect *peffect)
{
  requirement_list_iterate(peffect->reqs, preq) {
    if (preq->source.kind == VUT_UCLASS) {
      return preq->source.value.uclass;
    }
  } requirement_list_iterate_end;

  return NULL;
}

/**************************************************************************
  Number of AI stats units affected by effect
**************************************************************************/
static int num_affected_units(const struct effect *peffect,
                              const struct ai_data *ai)
{
  struct unit_class *uclass;
  enum unit_move_type move;

  uclass = affected_unit_class(peffect);
  if (uclass) {
    move = uclass_move_type(uclass);
    switch (move) {
     case LAND_MOVING:
         return ai->stats.units.land;
     case SEA_MOVING:
       return ai->stats.units.sea;
     case BOTH_MOVING:
       return ai->stats.units.amphibious;
     case MOVETYPE_LAST:
       break;
    }
  }
  return ai->stats.units.land + ai->stats.units.sea
         + ai->stats.units.amphibious;
}

/**************************************************************************
  How desirable is a particular effect for a particular city?
  Expressed as an adjustment of the base value (v)
  given the number of cities in range (c).
**************************************************************************/
static int improvement_effect_value(struct player *pplayer,
				    struct government *gov,
				    const struct ai_data *ai,
				    const struct city *pcity,
				    const bool capital, 
				    const struct impr_type *pimprove,
				    const struct effect *peffect,
				    const int c,
				    const int nplayers,
				    int v)
{
  int amount = peffect->value;
  struct unit_class *uclass;
  enum unit_move_type move = MOVETYPE_LAST;
  int num;

  switch (peffect->type) {
  /* These (Wonder) effects have already been evaluated in base_want() */
  case EFT_CAPITAL_CITY:
  case EFT_UPKEEP_FREE:
  case EFT_POLLU_POP_PCT:
  case EFT_POLLU_PROD_PCT:
  case EFT_OUTPUT_BONUS:
  case EFT_OUTPUT_BONUS_2:
  case EFT_OUTPUT_ADD_TILE:
  case EFT_OUTPUT_INC_TILE:
  case EFT_OUTPUT_PER_TILE:
  case EFT_OUTPUT_WASTE:
  case EFT_OUTPUT_WASTE_BY_DISTANCE:
  case EFT_OUTPUT_WASTE_PCT:
  case EFT_SPECIALIST_OUTPUT:
    break;

  case EFT_CITY_VISION_RADIUS_SQ:
  case EFT_UNIT_VISION_RADIUS_SQ:
    /* Wild guess.  "Amount" is the number of tiles (on average) that
     * will be revealed by the effect.  Note that with an omniscient
     * AI this effect is actually not useful at all. */
    v += c * amount;
    break;

  case EFT_TURN_YEARS:
  case EFT_SLOW_DOWN_TIMELINE:
    /* AI doesn't care about these. */
    break;

    /* WAG evaluated effects */
  case EFT_INCITE_COST_PCT:
    v += c * amount / 100;
    break;
  case EFT_MAKE_HAPPY:
    v += (get_entertainers(pcity) + pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]) * 5 * amount;
    if (city_list_size(pplayer->cities)
	> get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE)) {
      v += c * amount; /* offset large empire size */
    }
    v += c * amount;
    break;
  case EFT_UNIT_RECOVER:
    /* TODO */
    break;
  case EFT_NO_UNHAPPY:
    v += (get_entertainers(pcity) + pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]) * 30;
    break;
  case EFT_FORCE_CONTENT:
    v += content_effect_value(pplayer, pcity, amount, c, FEELING_FINAL);
    break;
  case EFT_MAKE_CONTENT:
    v += content_effect_value(pplayer, pcity, amount, c, FEELING_EFFECT);
    break;
  case EFT_MAKE_CONTENT_MIL_PER:
    if (get_city_bonus(pcity, EFT_NO_UNHAPPY) <= 0) {
      v += MIN(pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] + get_entertainers(pcity),
	       amount) * 25;
      v += MIN(amount, 5) * c;
    }
    break;
  case EFT_MAKE_CONTENT_MIL:
    if (get_city_bonus(pcity, EFT_NO_UNHAPPY) <= 0) {
      v += pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] * amount
        * MAX(unit_list_size(pcity->units_supported), 0) * 2;
      v += c * MAX(amount + 2, 1);
    }
    break;
  case EFT_TECH_PARASITE:
    {
      int turns;
      int bulbs;
      int value;
	  
      if (nplayers <= amount) {
	break;
      }
          
      turns = 9999;
      bulbs = 0;
      players_iterate(aplayer) {
	int potential = aplayer->bulbs_last_turn
	  + city_list_size(aplayer->cities) + 1;
	if (valid_advance(pimprove->obsolete_by)) {
	  turns = MIN(turns, 
		      total_bulbs_required_for_goal(aplayer, advance_number(pimprove->obsolete_by))
		      / (potential + 1));
	}
	if (players_on_same_team(aplayer, pplayer)) {
	  continue;
	}
	bulbs += potential;
      } players_iterate_end;
  
      /* For some number of turns we will be receiving bulbs for free
       * Bulbs should be amortized properly for each turn.
       * We use formula for the sum of geometric series:
       */
      value = bulbs * (1.0 - pow(1.0 - (1.0 / MORT), turns)) * MORT;
	  
      value = value  * (100 - game.info.freecost)	  
	* (nplayers - amount) / (nplayers * amount * 100);
	  
      /* WAG */
      value /= 3;

      CITY_LOG(LOG_DEBUG, pcity,
	       "%s parasite effect: bulbs %d, turns %d, value %d", 
	       improvement_rule_name(pimprove),
	       bulbs,
	       turns,
	       value);
	
      v += value;
      break;
    }
  case EFT_GROWTH_FOOD:
    v += c * 4 + (amount / 7) * pcity->surplus[O_FOOD];
    break;
  case EFT_HEALTH_PCT:
    /* Is plague possible */
    if (game.info.illness_on && pcity->size > MAX(1, game.info.illness_min_size)) {
      v += c * 5 + (amount / 5) * pcity->illness;
    }
    break;
  case EFT_AIRLIFT:
    /* FIXME: We need some smart algorithm here. The below is 
     * totally braindead. */
    v += c + MIN(ai->stats.units.land, 13);
    break;
  case EFT_ANY_GOVERNMENT:
    if (!can_change_to_government(pplayer, ai->goal.govt.gov)) {
      v += MIN(MIN(ai->goal.govt.val, 65),
	       num_unknown_techs_for_goal(pplayer, ai->goal.govt.req) * 10);
    }
    break;
  case EFT_ENABLE_NUKE:
    /* Treat nuke as a Cruise Missile upgrade */
    v += 20 + ai->stats.units.missiles * 5;
    break;
  case EFT_ENABLE_SPACE:
    if (game.info.spacerace) {
      v += 5;
      if (ai->diplomacy.production_leader == pplayer) {
	v += 100;
      }
    }
    break;
  case EFT_GIVE_IMM_TECH:
    if (!ai_wants_no_science(pplayer)) {
      v += amount * (game.info.sciencebox + 1);
    }
    break;
  case EFT_HAVE_EMBASSIES:
    v += 5 * nplayers;
    break;
  case EFT_REVEAL_CITIES:
  case EFT_NO_ANARCHY:
    break;  /* Useless for AI */
  case EFT_NUKE_PROOF:
    if (ai->threats.nuclear) {
      v += pcity->size * unit_list_size(pcity->tile->units) * (capital + 1)
	* amount / 100;
    }
    break;
  case EFT_REVEAL_MAP:
    if (!ai->explore.land_done || !ai->explore.sea_done) {
      v += 10;
    }
    break;
  case EFT_SIZE_UNLIMIT:
    /* Note we look up the SIZE_UNLIMIT again right below.  This could
     * be avoided... */
    if (get_city_bonus(pcity, EFT_SIZE_UNLIMIT) == 0) {
      amount = 20; /* really big city */
    }
    /* there not being a break here is deliberate, mind you */
  case EFT_SIZE_ADJ:
    if (get_city_bonus(pcity, EFT_SIZE_UNLIMIT) == 0) {
      const int aqueduct_size = get_city_bonus(pcity, EFT_SIZE_ADJ);

      if (!city_can_grow_to(pcity, pcity->size + 1)) {
	v += pcity->surplus[O_FOOD] * ai->food_priority * amount;
	if (pcity->size == aqueduct_size) {
	  v += 30 * pcity->surplus[O_FOOD];
	}
      }
      v += c * amount * 4 / aqueduct_size;
    }
    break;
  case EFT_SS_STRUCTURAL:
  case EFT_SS_COMPONENT:
  case EFT_SS_MODULE:
    if (game.info.spacerace
	/* If someone has started building spaceship already or
	 * we have chance to win a spacerace */
	&& (ai->diplomacy.spacerace_leader
	    || ai->diplomacy.production_leader == pplayer)) {
      v += 95;
    }
    break;
  case EFT_SPY_RESISTANT:
    /* Uhm, problem: City Wall has -50% here!! */
    break;
  case EFT_MOVE_BONUS:
    num = num_affected_units(peffect, ai);
    v += (8 * v * amount + num);
    break;
  case EFT_UNIT_NO_LOSE_POP:
    v += unit_list_size(pcity->tile->units) * 2;
    break;
  case EFT_HP_REGEN:
    num = num_affected_units(peffect, ai);
    v += (5 * c + num);
    break;
  case EFT_VETERAN_COMBAT:
    num = num_affected_units(peffect, ai);
    v += (2 * c + num);
    break;
  case EFT_VETERAN_BUILD:
    /* FIXME: check other reqs (e.g., unitflag) */
    num = num_affected_units(peffect, ai);
    v += (3 * c + num);
    break;
  case EFT_UPGRADE_UNIT:
    if (amount == 1) {
      v += ai->stats.units.upgradeable * 2;
    } else if (amount == 2) {
      v += ai->stats.units.upgradeable * 3;
    } else {
      v += ai->stats.units.upgradeable * 4;
    }
    break;
  case EFT_DEFEND_BONUS:
    if (ai_handicap(pplayer, H_DEFENSIVE)) {
      v += amount / 10; /* make AI slow */
    }
    uclass = affected_unit_class(peffect);
    if (uclass) {
      move = uclass_move_type(uclass);
    }

    if (uclass == NULL || move == SEA_MOVING) {
      /* Helps against sea units */
      if (is_ocean_tile(pcity->tile)) {
        v += ai->threats.ocean[-tile_continent(pcity->tile)]
          ? amount/5 : amount/20;
      } else {
        adjc_iterate(pcity->tile, tile2) {
          if (is_ocean_tile(tile2)) {
            if (ai->threats.ocean[-tile_continent(tile2)]) {
              v += amount/5;
              break;
            }
          }
        } adjc_iterate_end;
      }
    }
    v += (amount/20 + ai->threats.invasions - 1) * c; /* for wonder */
    if (capital || uclass == NULL || move != SEA_MOVING) {
      if (ai->threats.continent[tile_continent(pcity->tile)]
          || capital
          || (ai->threats.invasions
              && is_water_adjacent_to_tile(pcity->tile))) {
        if (ai->threats.continent[tile_continent(pcity->tile)]) {
          v += amount;
        } else {
          v += amount / (!ai->threats.igwall ? (15 - capital * 5) : 15);
        }
      }
    }
    break;
  case EFT_NO_INCITE:
    if (get_city_bonus(pcity, EFT_NO_INCITE) <= 0) {
      v += MAX((game.info.diplchance * 2 
		- game.info.incite_total_factor) / 2
	       - game.info.incite_improvement_factor * 5
	       - game.info.incite_unit_factor * 5, 0);
    }
    break;
  case EFT_GAIN_AI_LOVE:
    players_iterate(aplayer) {
      if (aplayer->ai_data.control) {
	if (ai_handicap(pplayer, H_DEFENSIVE)) {
	  v += amount / 10;
	} else {
	  v += amount / 20;
	}
      }
    } players_iterate_end;
    break;
  case EFT_UPGRADE_PRICE_PCT:
    /* This is based on average base upgrade price of 50. */
    v -= ai->stats.units.upgradeable * amount / 2;
    break;
  /* Currently not supported for building AI - wait for modpack users */
  case EFT_CITY_UNHAPPY_SIZE:
  case EFT_UNHAPPY_FACTOR:
  case EFT_UPKEEP_FACTOR:
  case EFT_UNIT_UPKEEP_FREE_PER_CITY:
  case EFT_CIVIL_WAR_CHANCE:
  case EFT_EMPIRE_SIZE_BASE:
  case EFT_EMPIRE_SIZE_STEP:
  case EFT_MAX_RATES:
  case EFT_MARTIAL_LAW_EACH:
  case EFT_MARTIAL_LAW_MAX:
  case EFT_RAPTURE_GROW:
  case EFT_UNBRIBABLE_UNITS:
  case EFT_REVOLUTION_WHEN_UNHAPPY:
  case EFT_HAS_SENATE:
  case EFT_INSPIRE_PARTISANS:
  case EFT_HAPPINESS_TO_GOLD:
  case EFT_FANATICS:
  case EFT_NO_DIPLOMACY:
  case EFT_OUTPUT_PENALTY_TILE:
  case EFT_OUTPUT_INC_TILE_CELEBRATE:
  case EFT_TRADE_REVENUE_BONUS:
  case EFT_TILE_WORKABLE:
    break;
    /* This has no effect for AI */
  case EFT_VISIBLE_WALLS:
  case EFT_SHIELD2GOLD_FACTOR:
    break;
  case EFT_TECH_COST_FACTOR:
    v -= amount * 50;
    break;
  case EFT_MIGRATION_PCT:
    /* consider all foreign cities within the set distance */
    iterate_outward(city_tile(pcity), game.info.mgr_distance + 1, ptile) {
      struct city *acity = tile_city(ptile);

      if (!acity || acity == pcity || city_owner(acity) == pplayer) {
        /* no city, the city in the center or own city */
        continue;
      }

      v += amount; /* AI wants migration into its cities! */
    } iterate_outward_end;
    break;
  case EFT_LAST:
    freelog(LOG_ERROR, "Bad effect type.");
    break;
  }

  return v;
}

/************************************************************************** 
  Increase the degree to which we want to meet a set of requirements,
  because they will enable construction of an improvement
  with desirable effects.

  v is the desire for the improvement.

  Returns whether all the requirements are met.
**************************************************************************/
static bool adjust_wants_for_reqs(struct player *pplayer,
                                  struct city *pcity, 
                                  struct impr_type *pimprove,
                                  const int v)
{
  bool all_met = TRUE;
  int n_needed_techs = 0;
  int n_needed_improvements = 0;
  struct tech_vector needed_techs;
  struct impr_vector needed_improvements;

  tech_vector_init(&needed_techs);
  impr_vector_init(&needed_improvements);

  requirement_vector_iterate(&pimprove->reqs, preq) {
    const bool active = is_req_active(pplayer, pcity, pimprove,
                                      pcity->tile, NULL, NULL, NULL, preq,
                                      RPT_POSSIBLE);

    if (VUT_ADVANCE == preq->source.kind && !active) {
      /* Found a missing technology requirement for this improvement. */
      tech_vector_append(&needed_techs, &preq->source.value.advance);
    } else if (VUT_IMPROVEMENT == preq->source.kind && !active) {
      /* Found a missing improvement requirement for this improvement.
       * For example, in the default ruleset a city must have a Library
       * before it can have a University. */
      impr_vector_append(&needed_improvements, &preq->source.value.building);
    }
    all_met = all_met && active;
  } requirement_vector_iterate_end;

  /* If v is negative, the improvement is not worth building,
   * but there is no need to punish research of the technologies
   * that would make it available.
   */
  n_needed_techs = tech_vector_size(&needed_techs);
  if (0 < v && 0 < n_needed_techs) {
    /* Because we want this improvements,
     * we want the techs that will make it possible */
    const int dv = v / n_needed_techs;
    int t;

    for (t = 0; t < n_needed_techs; t++) {
      want_tech_for_improvement_effect(pplayer, pcity, pimprove,
                                       *tech_vector_get(&needed_techs, t), dv);
    }
  }

  /* If v is negative, the improvement is not worth building,
   * but there is no need to punish building the improvements
   * that would make it available.
   */
  n_needed_improvements = impr_vector_size(&needed_improvements);
  if (0 < v && 0 < n_needed_improvements) {
    /* Because we want this improvement,
     * we want the improvements that will make it possible */
    const int dv = v / (n_needed_improvements * 4); /* WAG */
    int i;

    for (i = 0; i < n_needed_improvements; i++) {
      struct impr_type *needed_impr = *impr_vector_get(&needed_improvements, i);
      /* TODO: increase the want for the needed_impr,
       * if we can build it now */
      /* Recurse */
      (void)adjust_wants_for_reqs(pplayer, pcity, needed_impr, dv);
    }
  }

  /* TODO: use a similar method to increase wants for governments
   * that will make this improvement possible? */

  tech_vector_free(&needed_techs);
  impr_vector_free(&needed_improvements);

  return all_met;
}

/************************************************************************** 
  Calculate effects of possible improvements and extra effects of existing
  improvements. Consequently adjust the desirability of those improvements
  or the technologies that would make them possible.

  This function may (indeed, should) be called even for improvements that a city
  already has, or can not (yet) build. For existing improvements,
  it will discourage research of technologies that would make the improvement
  obsolete or reduce its effectiveness, and encourages technologies that would
  improve its effectiveness. For improvements that the city can not yet build
  it will encourage research of the techs and building of the improvements
  that will make the improvement possible.

  A complexity is that there are two sets of requirements to consider:
  the requirements for the building itself, and the requirements for
  the effects for the building.

  A few base variables:
    c - number of cities we have in current range
    u - units we have of currently affected type
    v - the want for the improvement we are considering

  This function contains a whole lot of WAGs. We ignore cond_* for now,
  thinking that one day we may fulfil the cond_s anyway. In general, we
  first add bonus for city improvements, then for wonders.

  IDEA: Calculate per-continent aggregates of various data, and use this
  for wonders below for better wonder placements.
**************************************************************************/
static void adjust_improvement_wants_by_effects(struct player *pplayer,
                                                struct city *pcity, 
                                                struct impr_type *pimprove,
                                                const bool already)
{
  int v = 0;
  int cities[REQ_RANGE_LAST];
  int nplayers = normal_player_count();
  struct ai_data *ai = ai_data_get(pplayer);
  bool capital = is_capital(pcity);
  bool can_build = TRUE;
  struct government *gov = government_of_player(pplayer);
  struct universal source = {
    .kind = VUT_IMPROVEMENT,
    .value = {.building = pimprove}
  };
  const bool is_coinage = improvement_has_flag(pimprove, IF_GOLD);

  /* Remove team members from the equation */
  players_iterate(aplayer) {
    if (aplayer->team
        && aplayer->team == pplayer->team
        && aplayer != pplayer) {
      nplayers--;
    }
  } players_iterate_end;

  if (is_coinage) {
    /* Since coinage contains some entirely spurious ruleset values,
     * we need to hard-code a sensible want.
     * We must otherwise handle the special IF_GOLD improvement
     * like the others, so the AI will research techs that make it available,
     * for rulesets that do not provide it from the start.
     */
    v += TRADE_WEIGHTING;
  } else {
    /* Base want is calculated above using a more direct approach. */
    v += base_want(pplayer, pcity, pimprove);
    if (v != 0) {
      CITY_LOG(LOG_DEBUG, pcity, "%s base_want is %d (range=%d)", 
               improvement_rule_name(pimprove),
               v,
               ai->impr_range[improvement_index(pimprove)]);
    }
  }

  if (!is_coinage) {
    /* Adjust by building cost */
    /* FIXME: ought to reduce by upkeep cost and amortise by building cost */
    v -= (impr_build_shield_cost(pimprove)
         / (pcity->surplus[O_SHIELD] * 10 + 1));
  }

  /* Find number of cities per range.  */
  cities[REQ_RANGE_PLAYER] = city_list_size(pplayer->cities);
  cities[REQ_RANGE_WORLD] = cities[REQ_RANGE_PLAYER]; /* kludge. */

  cities[REQ_RANGE_CONTINENT] = ai->stats.cities[tile_continent(pcity->tile)];

  cities[REQ_RANGE_CITY] = 1;
  cities[REQ_RANGE_LOCAL] = 0;

  effect_list_iterate(get_req_source_effects(&source), peffect) {
    struct requirement *mypreq = NULL;
    bool active = TRUE;
    int n_needed_techs = 0;
    struct tech_vector needed_techs;

    if (is_effect_disabled(pplayer, pcity, pimprove,
			   NULL, NULL, NULL, NULL,
			   peffect, RPT_CERTAIN)) {
      /* We believe that effect if disabled only if there is no change that it
       * is not. This should lead AI using wider spectrum of improvements.
       *
       * TODO: Select between RPT_POSSIBLE and RPT_CERTAIN dynamically
       * depending how much AI can take risks. */
      continue;
    }

    tech_vector_init(&needed_techs);

    requirement_list_iterate(peffect->reqs, preq) {
      /* Check if all the requirements for the currently evaluated effect
       * are met, except for having the building that we are evaluating. */
      if (VUT_IMPROVEMENT == preq->source.kind
	  && preq->source.value.building == pimprove) {
	mypreq = preq;
        continue;
      }
      if (!is_req_active(pplayer, pcity, pimprove, NULL, NULL, NULL, NULL,
			 preq, RPT_POSSIBLE)) {
	active = FALSE;
	if (VUT_ADVANCE == preq->source.kind) {
	  /* This missing requirement is a missing tech requirement.
	   * This will be for some additional effect
	   * (For example, in the default ruleset, Mysticism increases
	   * the effect of Temples). */
          tech_vector_append(&needed_techs, &preq->source.value.advance);
	}
      }
    } requirement_list_iterate_end;

    n_needed_techs = tech_vector_size(&needed_techs);
    if (active || n_needed_techs) {
      const int v1 = improvement_effect_value(pplayer, gov, ai,
					      pcity, capital, 
					      pimprove, peffect,
					      cities[mypreq->range],
					      nplayers, v);
      /* v1 could be negative (the effect could be undesirable),
       * although it is usually positive.
       * For example, in the default ruleset, Communism decreases the
       * effectiveness of a Cathedral. */

      if (active) {
	v = v1;
      } else { /* n_needed_techs */
	/* We might want the technology that will enable this
	 * (additional) effect.
	 * The better the effect, the more we want the technology.
	 * Use (v1 - v) rather than v1 in case there are multiple
	 * effects that have technology requirements and to eliminate the
	 * 'base' effect of the building.
         * We are more interested in (additional) effects that enhance
	 * buildings we already have.
	 */
        const int a = already? 5: 4; /* WAG */
        const int dv = (v1 - v) * a / (4 * n_needed_techs);
	int t;
	for (t = 0; t < n_needed_techs; t++) {
	  want_tech_for_improvement_effect(pplayer, pcity, pimprove,
                                           *tech_vector_get(&needed_techs, t),
                                           dv);
	}
      }
    }

    tech_vector_free(&needed_techs);
  } effect_list_iterate_end;

  if (already && valid_advance(pimprove->obsolete_by)) {
    /* Discourage research of the technology that would make this building
     * obsolete. The bigger the desire for this building, the more
     * we want to discourage the technology. */
    want_tech_for_improvement_effect(pplayer, pcity, pimprove,
				     pimprove->obsolete_by, -v);
  } else if (!already) {
    /* Increase the want for technologies that will enable
     * construction of this improvement, if necessary.
     */
    const bool all_met = adjust_wants_for_reqs(pplayer, pcity, pimprove, v);
    can_build = can_build && all_met;
  }

  if (is_coinage && can_build) {
    /* Could have a negative want for coinage,
     * if we have some stock in a building already. */
    pcity->ai->building_want[improvement_index(pimprove)] += v;
  } else if (!already && can_build) {
    /* Convert the base 'want' into a building want
     * by applying various adjustments */

    /* Would it mean losing shields? */
    if ((VUT_UTYPE == pcity->production.kind 
        || (is_wonder(pcity->production.value.building)
          && !is_wonder(pimprove))
        || (!is_wonder(pcity->production.value.building)
          && is_wonder(pimprove)))
     && pcity->turn_last_built != game.info.turn) {
      v -= (pcity->shield_stock / 2) * (SHIELD_WEIGHTING / 2);
    }

    /* Reduce want if building gets obsoleted soon */
    if (valid_advance(pimprove->obsolete_by)) {
      v -= v / MAX(1, num_unknown_techs_for_goal(pplayer, advance_number(pimprove->obsolete_by)));
    }

    /* Are we wonder city? Try to avoid building non-wonders very much. */
    if (pcity->id == ai->wonder_city && !is_wonder(pimprove)) {
      v /= 5;
    }

    /* Set */
    pcity->ai->building_want[improvement_index(pimprove)] += v;
  }
  /* Else we either have the improvement already,
   * or we can not build it (yet) */
}

/**************************************************************************
  Calculate walking distance to nearest friendly cities from every city.

  The hidden assumption here is that a F_HELP_WONDER unit is like any
  other unit that will use this data.

  pcity->ai.downtown is set to the number of cities within 4 turns of
  the best help wonder unit we can currently produce.
**************************************************************************/
static void calculate_city_clusters(struct player *pplayer)
{
  struct unit_type *punittype;
  struct unit *ghost;
  int range;

  city_list_iterate(pplayer->cities, pcity) {
    pcity->ai->downtown = 0;
  } city_list_iterate_end;

  if (num_role_units(F_HELP_WONDER) == 0) {
    return; /* ruleset has no help wonder unit */
  }

  punittype = best_role_unit_for_player(pplayer, F_HELP_WONDER);
  if (!punittype) {
    punittype = get_role_unit(F_HELP_WONDER, 0); /* simulate future unit */
  }
  ghost = create_unit_virtual(pplayer, NULL, punittype, 0);
  range = unit_move_rate(ghost) * 4;

  city_list_iterate(pplayer->cities, pcity) {
    struct pf_parameter parameter;
    struct pf_map *pfm;

    ghost->tile = pcity->tile;
    pft_fill_unit_parameter(&parameter, ghost);
    pfm = pf_map_new(&parameter);

    pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
      struct city *acity = tile_city(ptile);

      if (move_cost > range) {
        break;
      }
      if (!acity) {
        continue;
      }
      if (city_owner(acity) == pplayer) {
        pcity->ai->downtown++;
      }
    } pf_map_iterate_move_costs_end;

    pf_map_destroy(pfm);
  } city_list_iterate_end;

  destroy_unit_virtual(ghost);
}

/**************************************************************************
  Calculate walking distances to wonder city from nearby cities.
**************************************************************************/
static void calculate_wonder_helpers(struct player *pplayer, 
                                     struct ai_data *ai)
{
  struct pf_map *pfm;
  struct pf_parameter parameter;
  struct unit_type *punittype;
  struct unit *ghost;
  int maxrange;
  struct city *wonder_city = game_find_city_by_number(ai->wonder_city);

  city_list_iterate(pplayer->cities, acity) {
    acity->ai->distance_to_wonder_city = 0; /* unavailable */
  } city_list_iterate_end;

  if (wonder_city == NULL) {
    return;
  }
  if (city_owner(wonder_city) != pplayer) {
    ai->wonder_city = 0;
    return;
  }

  punittype = best_role_unit_for_player(pplayer, F_HELP_WONDER);
  if (!punittype) {
    return;
  }
  ghost = create_unit_virtual(pplayer, wonder_city, punittype, 0);
  maxrange = unit_move_rate(ghost) * 7;

  pft_fill_unit_parameter(&parameter, ghost);
  pfm = pf_map_new(&parameter);

  pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
    struct city *acity = tile_city(ptile);

    if (move_cost > maxrange) {
      break;
    }
    if (!acity) {
      continue;
    }
    if (city_owner(acity) == pplayer) {
      acity->ai->distance_to_wonder_city = move_cost;
    }
  } pf_map_iterate_move_costs_end;

  pf_map_destroy(pfm);
  destroy_unit_virtual(ghost);
}

/************************************************************************** 
  Whether the AI should calculate the building wants for this city
  this turn, ahead of schedule.

  Always recalculate if the city just finished building,
  so we can make a sensible choice for the next thing to build.
  Perhaps the improvement we were building has become obsolete,
  or a different player built the Great Wonder we were building.
**************************************************************************/
static bool should_force_recalc(struct city *pcity)
{
  return city_built_last_turn(pcity)
      || (VUT_IMPROVEMENT == pcity->production.kind
       && !improvement_has_flag(pcity->production.value.building, IF_GOLD)
       && !can_city_build_improvement_later(pcity, pcity->production.value.building));
}

/************************************************************************** 
  Calculate how much an AI player should want to build particular
  improvements, because of the effects of those improvements, and
  increase the want for technologies that will enable buildings with
  desirable effects.
**************************************************************************/
static void adjust_wants_by_effects(struct player *pplayer,
                                    struct city *wonder_city)
{
  /* Clear old building wants.
   * Do this separately from the iteration over improvement types
   * because each iteration could actually update more than one improvement,
   * if improvements have improvements as requirements.
   */
  city_list_iterate(pplayer->cities, pcity) {
    if (!pplayer->ai_data.control) {
      /* For a human player, any building is worth building until discarded */
      improvement_iterate(pimprove) {
        pcity->ai->building_want[improvement_index(pimprove)] = 1;
      } improvement_iterate_end;
    } else if (pcity->ai->building_turn <= game.info.turn) {
      /* Do a scheduled recalculation this turn */
      improvement_iterate(pimprove) {
        pcity->ai->building_want[improvement_index(pimprove)] = 0;
      } improvement_iterate_end;
    } else if (should_force_recalc(pcity)) {
      /* Do an emergency recalculation this turn. */
      pcity->ai->building_wait = pcity->ai->building_turn - game.info.turn;
      pcity->ai->building_turn = game.info.turn;

      improvement_iterate(pimprove) {
        pcity->ai->building_want[improvement_index(pimprove)] = 0;
      } improvement_iterate_end;
    }
  } city_list_iterate_end;

  improvement_iterate(pimprove) {
    const bool is_coinage = improvement_has_flag(pimprove, IF_GOLD);

    /* Handle coinage specially because you can never complete coinage */
    if (is_coinage
     || can_player_build_improvement_later(pplayer, pimprove)) {
      city_list_iterate(pplayer->cities, pcity) {
        if (pcity != wonder_city && is_wonder(pimprove)) {
          /* Only wonder city should build wonders! */
          pcity->ai->building_want[improvement_index(pimprove)] = 0;
        } else if ((!is_coinage
                    && !can_city_build_improvement_later(pcity, pimprove))
                   || is_building_replaced(pcity, pimprove, RPT_CERTAIN)) {
          /* Don't consider impossible or redundant buildings */
          pcity->ai->building_want[improvement_index(pimprove)] = 0;
        } else if (pplayer->ai_data.control
                   && pcity->ai->building_turn <= game.info.turn) {
          /* Building wants vary relatively slowly, so not worthwhile
           * recalculating them every turn.
           * We DO want to calculate (tech) wants because of buildings
           * we already have. */
          const bool already = city_has_building(pcity, pimprove);

          adjust_improvement_wants_by_effects(pplayer, pcity, 
                                              pimprove, already);

          assert(!(already
                   && 0 < pcity->ai->building_want[improvement_index(pimprove)]));
        } else if (city_has_building(pcity, pimprove)) {
          /* Never want to build something we already have. */
          pcity->ai->building_want[improvement_index(pimprove)] = 0;
        }
        /* else wait until a later turn */
      } city_list_iterate_end;
    } else {
      /* An impossible improvement */
      city_list_iterate(pplayer->cities, pcity) {
        pcity->ai->building_want[improvement_index(pimprove)] = 0;
      } city_list_iterate_end;
    }
  } improvement_iterate_end;

#ifdef DEBUG
  /* This logging is relatively expensive, so activate only if necessary */
  city_list_iterate(pplayer->cities, pcity) {
    improvement_iterate(pimprove) {
      if (pcity->ai->building_want[improvement_index(pimprove)] != 0) {
        CITY_LOG(LOG_DEBUG, pcity, "want to build %s with %d", 
                 improvement_rule_name(pimprove),
                 pcity->ai->building_want[improvement_index(pimprove)]);
      }
    } improvement_iterate_end;
  } city_list_iterate_end;
#endif
}

/************************************************************************** 
  Prime pcity->ai.building_want[]
**************************************************************************/
void ai_manage_buildings(struct player *pplayer)
/* TODO:  RECALC_SPEED should be configurable to ai difficulty. -kauf  */
#define RECALC_SPEED 5
{
  struct ai_data *ai = ai_data_get(pplayer);
  struct city *wonder_city = game_find_city_by_number(ai->wonder_city);

  if (wonder_city && city_owner(wonder_city) != pplayer) {
    /* We lost it to the enemy! */
    ai->wonder_city = 0;
    wonder_city = NULL;
  }

  /* Preliminary analysis - find our Wonder City. Also check if it
   * is sane to continue building the wonder in it. If either does
   * not check out, make a Wonder City. */
  if (NULL == wonder_city
   || 0 <= wonder_city->surplus[O_SHIELD] /* FIXME: need higher surplus? */
   || VUT_UTYPE == wonder_city->production.kind /* changed to defender? */
   || !is_wonder(wonder_city->production.value.building) /* redundant test? */
   || !can_city_build_improvement_now(wonder_city, 
                                      wonder_city->production.value.building)
   || improvement_obsolete(pplayer,
                           wonder_city->production.value.building)
   || is_building_replaced(wonder_city, 
                           wonder_city->production.value.building,
                           RPT_CERTAIN)
      ) {
    /* Find a new wonder city! */
    int best_candidate_value = 0;
    struct city *best_candidate = NULL;
    /* Whether ruleset has a help wonder unit type */
    bool has_help = (num_role_units(F_HELP_WONDER) > 0);

    calculate_city_clusters(pplayer);

    city_list_iterate(pplayer->cities, pcity) {
      int value = pcity->surplus[O_SHIELD];
      Continent_id place = tile_continent(pcity->tile);

      if (pcity->ai->grave_danger > 0) {
        continue;
      }
      if (is_ocean_near_tile(pcity->tile)) {
        value /= 2;
      }
      /* Downtown is the number of cities within a certain pf range.
       * These may be able to help with caravans. Also look at the whole
       * continent. */
      if (first_role_unit_for_player(pplayer, F_HELP_WONDER)) {
        value += pcity->ai->downtown;
        value += ai->stats.cities[place] / 8;
      }
      if (ai->threats.continent[place] > 0) {
        /* We have threatening neighbours: -25% */
        value -= value / 4;
      }
      /* Require that there is at least some neighbors for wonder helpers,
       * if ruleset supports it. */
      if (value > best_candidate_value
          && (!has_help || ai->stats.cities[place] > 5)
          && (!has_help || pcity->ai->downtown > 3)) {
        best_candidate = pcity;
        best_candidate_value = value;
      }
    } city_list_iterate_end;
    if (best_candidate) {
      CITY_LOG(LOG_DEBUG, best_candidate, "chosen as wonder-city!");
      ai->wonder_city = best_candidate->id;
      wonder_city = best_candidate;
    }
  }
  calculate_wonder_helpers(pplayer, ai);

  /* First find current worth of cities and cache this. */
  city_list_iterate(pplayer->cities, acity) {
    acity->ai->worth = city_want(pplayer, acity, ai, NULL);
  } city_list_iterate_end;

  adjust_wants_by_effects(pplayer, wonder_city);

  /* Reset recalc counter */
  city_list_iterate(pplayer->cities, pcity) {
    if (pcity->ai->building_turn <= game.info.turn) {
      /* This will spread recalcs out so that no one turn end is 
       * much longer than others */
      pcity->ai->building_wait = myrand(RECALC_SPEED) + RECALC_SPEED;
      pcity->ai->building_turn = game.info.turn + pcity->ai->building_wait;
    }
  } city_list_iterate_end;
}

/**************************************************************************
  Choose a build for the barbarian player.

  TODO: Move this into advmilitary.c
  TODO: It will be called for each city but doesn't depend on the city,
  maybe cache it?  Although barbarians don't normally have many cities, 
  so can be a bigger bother to cache it.
**************************************************************************/
static void ai_barbarian_choose_build(struct player *pplayer, 
                                      struct city *pcity,
				      struct ai_choice *choice)
{
  struct unit_type *bestunit = NULL;
  int i, bestattack = 0;

  /* Choose the best unit among the basic ones */
  for(i = 0; i < num_role_units(L_BARBARIAN_BUILD); i++) {
    struct unit_type *iunit = get_role_unit(L_BARBARIAN_BUILD, i);

    if (iunit->attack_strength > bestattack
        && can_city_build_unit_now(pcity, iunit)) {
      bestunit = iunit;
      bestattack = iunit->attack_strength;
    }
  }

  /* Choose among those made available through other civ's research */
  for(i = 0; i < num_role_units(L_BARBARIAN_BUILD_TECH); i++) {
    struct unit_type *iunit = get_role_unit(L_BARBARIAN_BUILD_TECH, i);

    if (iunit->attack_strength > bestattack
        && can_city_build_unit_now(pcity, iunit)) {
      bestunit = iunit;
      bestattack = iunit->attack_strength;
    }
  }

  /* If found anything, put it into the choice */
  if (bestunit) {
    choice->value.utype = bestunit;
    /* FIXME: 101 is the "overriding military emergency" indicator */
    choice->want   = 101;
    choice->type   = CT_ATTACKER;
  } else {
    freelog(LOG_WANT, "Barbarians don't know what to build!");
  }
}

/************************************************************************** 
  Chooses what the city will build.  Is called after the military advisor
  put it's choice into pcity->ai.choice and "settler advisor" put settler
  want into pcity->founder_*.

  Note that AI cheats -- it suffers no penalty for switching from unit to 
  improvement, etc.
**************************************************************************/
static void ai_city_choose_build(struct player *pplayer, struct city *pcity)
{
  struct ai_choice newchoice;
  struct ai_data *ai = ai_data_get(pplayer);

  init_choice(&newchoice);

  if (ai_handicap(pplayer, H_AWAY)
      && city_built_last_turn(pcity)
      && pcity->ai->urgency == 0) {
    /* Don't change existing productions unless we have to. */
    return;
  }

  if( is_barbarian(pplayer) ) {
    ai_barbarian_choose_build(pplayer, pcity, &(pcity->ai->choice));
  } else {
    /* FIXME: 101 is the "overriding military emergency" indicator */
    if ((pcity->ai->choice.want <= 100 || pcity->ai->urgency == 0)
        && !(ai_on_war_footing(pplayer) && pcity->ai->choice.want > 0
             && pcity->id != ai->wonder_city)) {
      domestic_advisor_choose_build(pplayer, pcity, &newchoice);
      copy_if_better_choice(&newchoice, &(pcity->ai->choice));
    }
  }

  /* Fallbacks */
  if (pcity->ai->choice.want == 0) {
    /* Fallbacks do happen with techlevel 0, which is now default. -- Per */
    CITY_LOG(LOG_WANT, pcity, "Falling back - didn't want to build soldiers,"
	     " workers, caravans, settlers, or buildings!");
    pcity->ai->choice.want = 1;
    if (best_role_unit(pcity, F_TRADE_ROUTE)) {
      pcity->ai->choice.value.utype = best_role_unit(pcity, F_TRADE_ROUTE);
      pcity->ai->choice.type = CT_CIVILIAN;
    } else if (best_role_unit(pcity, F_SETTLERS)) {
      pcity->ai->choice.value.utype = best_role_unit(pcity, F_SETTLERS);
      pcity->ai->choice.type = CT_CIVILIAN;
    } else {
      CITY_LOG(LOG_ERROR, pcity, "Cannot even build a fallback "
	       "(caravan/coinage/settlers). Fix the ruleset!");
      pcity->ai->choice.want = 0;
    }
  }

  if (pcity->ai->choice.want != 0) {
    ASSERT_CHOICE(pcity->ai->choice);

    CITY_LOG(LOG_DEBUG, pcity, "wants %s with desire %d.",
	     ai_choice_rule_name(&pcity->ai->choice),
	     pcity->ai->choice.want);
    
    /* FIXME: parallel to citytools change_build_target() */
    if (VUT_IMPROVEMENT == pcity->production.kind
     && is_great_wonder(pcity->production.value.building)
     && (CT_BUILDING != pcity->ai->choice.type
      || pcity->ai->choice.value.building != pcity->production.value.building)) {
      notify_player(NULL, pcity->tile, E_WONDER_STOPPED,
		    _("The %s have stopped building The %s in %s."),
		    nation_plural_for_player(pplayer),
		    city_production_name_translation(pcity),
		    city_name(pcity));
    }
    if (CT_BUILDING == pcity->ai->choice.type
      && is_great_wonder(pcity->ai->choice.value.building)
      && (VUT_IMPROVEMENT != pcity->production.kind
       || pcity->production.value.building != pcity->ai->choice.value.building)) {
      notify_player(NULL, pcity->tile, E_WONDER_STARTED,
		    _("The %s have started building The %s in %s."),
		    nation_plural_for_player(city_owner(pcity)),
		    city_improvement_name_translation(pcity,
                      pcity->ai->choice.value.building),
		    city_name(pcity));
    }

    switch (pcity->ai->choice.type) {
    case CT_CIVILIAN:
    case CT_ATTACKER:
    case CT_DEFENDER:
      pcity->production.kind = VUT_UTYPE;
      pcity->production.value.utype = pcity->ai->choice.value.utype;
      break;
    case CT_BUILDING:
      pcity->production.kind = VUT_IMPROVEMENT;
      pcity->production.value.building = pcity->ai->choice.value.building;
      break;
    case CT_NONE:
      pcity->production.kind = VUT_NONE;
      break;
    case CT_LAST:
      pcity->production.kind = VUT_LAST;
      break;
    };
  }
}

/************************************************************************** 
...
**************************************************************************/
static void try_to_sell_stuff(struct player *pplayer, struct city *pcity)
{
  improvement_iterate(pimprove) {
    if (can_city_sell_building(pcity, pimprove)
	&& !building_has_effect(pimprove, EFT_DEFEND_BONUS)) {
/* selling walls to buy defenders is counterproductive -- Syela */
      really_handle_city_sell(pplayer, pcity, pimprove);
      break;
    }
  } improvement_iterate_end;
}

/************************************************************************** 
  Increase maxbuycost.  This variable indicates (via ai_gold_reserve) to 
  the tax selection code how much money do we need for buying stuff.
**************************************************************************/
static void increase_maxbuycost(struct player *pplayer, int new_value)
{
  pplayer->ai_data.maxbuycost = MAX(pplayer->ai_data.maxbuycost, new_value);
}

/************************************************************************** 
  Try to upgrade a city's units. limit is the last amount of gold we can
  end up with after the upgrade. military is if we want to upgrade non-
  military or military units.
**************************************************************************/
static void ai_upgrade_units(struct city *pcity, int limit, bool military)
{
  struct player *pplayer = city_owner(pcity);

  unit_list_iterate(pcity->tile->units, punit) {
    if (pcity->owner == punit->owner) {
      /* Only upgrade units you own, not allied ones */
      struct unit_type *punittype = can_upgrade_unittype(pplayer, unit_type(punit));

      if (military && !IS_ATTACKER(punit)) {
        /* Only upgrade military units this round */
        continue;
      } else if (!military && IS_ATTACKER(punit)) {
        /* Only civilians or tranports this round */
        continue;
      }
      if (punittype) {
        int cost = unit_upgrade_price(pplayer, unit_type(punit), punittype);
        int real_limit = limit;

        /* Triremes are DANGEROUS!! We'll do anything to upgrade 'em. */
        if (unit_has_type_flag(punit, F_TRIREME)) {
          real_limit = pplayer->ai_data.est_upkeep;
        }
        if (pplayer->economic.gold - cost > real_limit) {
          CITY_LOG(LOG_BUY, pcity, "Upgraded %s to %s for %d (%s)",
                   unit_rule_name(punit),
                   utype_rule_name(punittype),
                   cost,
                   military ? "military" : "civilian");
          handle_unit_upgrade(city_owner(pcity), punit->id);
        } else {
          increase_maxbuycost(pplayer, cost);
        }
      }
    }
  } unit_list_iterate_end;
}

/************************************************************************** 
  Buy and upgrade stuff!
**************************************************************************/
static void ai_spend_gold(struct player *pplayer)
{
  struct ai_choice bestchoice;
  int cached_limit = ai_gold_reserve(pplayer);
  bool war_footing = ai_on_war_footing(pplayer);

  /* Disband explorers that are at home but don't serve a purpose. 
   * FIXME: This is a hack, and should be removed once we
   * learn how to ferry explorers to new land. */
  city_list_iterate(pplayer->cities, pcity) {
    struct tile *ptile = pcity->tile;
    unit_list_iterate_safe(ptile->units, punit) {
      if (unit_has_type_role(punit, L_EXPLORER)
          && pcity->id == punit->homecity
          && pcity->ai->urgency == 0) {
        CITY_LOG(LOG_BUY, pcity, "disbanding %s to increase production",
                 unit_rule_name(punit));
	handle_unit_disband(pplayer,punit->id);
      }
    } unit_list_iterate_safe_end;
  } city_list_iterate_end;
  
  do {
    bool expensive; /* don't buy when it costs x2 unless we must */
    int buycost;
    int limit = cached_limit; /* cached_limit is our gold reserve */
    struct city *pcity = NULL;

    /* Find highest wanted item on the buy list */
    init_choice(&bestchoice);
    city_list_iterate(pplayer->cities, acity) {
      if (acity->ai->choice.want > bestchoice.want && ai_fuzzy(pplayer, TRUE)) {
        bestchoice.value = acity->ai->choice.value;
        bestchoice.want = acity->ai->choice.want;
        bestchoice.type = acity->ai->choice.type;
        pcity = acity;
      }
    } city_list_iterate_end;

    /* We found nothing, so we're done */
    if (bestchoice.want == 0) {
      break;
    }

    /* Not dealing with this city a second time */
    pcity->ai->choice.want = 0;

    ASSERT_CHOICE(bestchoice);

    /* Try upgrade units at danger location (high want is usually danger) */
    if (pcity->ai->urgency > 1) {
      if (bestchoice.type == CT_BUILDING
       && is_wonder(bestchoice.value.building)) {
        CITY_LOG(LOG_BUY, pcity, "Wonder being built in dangerous position!");
      } else {
        /* If we have urgent want, spend more */
        int upgrade_limit = limit;

        if (pcity->ai->urgency > 1) {
          upgrade_limit = pplayer->ai_data.est_upkeep;
        }
        /* Upgrade only military units now */
        ai_upgrade_units(pcity, upgrade_limit, TRUE);
      }
    }

    if (pcity->anarchy != 0 && bestchoice.type != CT_BUILDING) {
      continue; /* Nothing we can do */
    }

    /* Cost to complete production */
    buycost = city_production_buy_gold_cost(pcity);

    if (buycost <= 0) {
      continue; /* Already completed */
    }

    if (is_unit_choice_type(bestchoice.type)
        && utype_has_flag(bestchoice.value.utype, F_CITIES)) {
      if (get_city_bonus(pcity, EFT_GROWTH_FOOD) == 0
          && pcity->size == 1
          && city_granary_size(pcity->size)
             > pcity->food_stock + pcity->surplus[O_FOOD]) {
        /* Don't buy settlers in size 1 cities unless we grow next turn */
        continue;
      } else if (city_list_size(pplayer->cities) > 6) {
        /* Don't waste precious money buying settlers late game
         * since this raises taxes, and we want science. Adjust this
         * again when our tax algorithm is smarter. */
        continue;
      } else if (war_footing) {
        continue;
      }
    } else {
      /* We are not a settler. Therefore we increase the cash need we
       * balance our buy desire with to keep cash at hand for emergencies 
       * and for upgrades */
      limit *= 2;
    }

    /* It costs x2 to buy something with no shields contributed */
    expensive = (pcity->shield_stock == 0)
                || (pplayer->economic.gold - buycost < limit);

    if (bestchoice.type == CT_ATTACKER
	&& buycost 
           > utype_build_shield_cost(bestchoice.value.utype) * 2
        && !war_footing) {
       /* Too expensive for an offensive unit */
       continue;
    }

    /* FIXME: Here Syela wanted some code to check if
     * pcity was doomed, and we should therefore attempt
     * to sell everything in it of non-military value */

    if (pplayer->economic.gold - pplayer->ai_data.est_upkeep >= buycost
        && (!expensive 
            || (pcity->ai->grave_danger != 0 && assess_defense(pcity) == 0)
            || (bestchoice.want > 200 && pcity->ai->urgency > 1))) {
      /* Buy stuff */
      CITY_LOG(LOG_BUY, pcity, "Crash buy of %s for %d (want %d)",
               ai_choice_rule_name(&bestchoice),
               buycost,
               bestchoice.want);
      really_handle_city_buy(pplayer, pcity);
    } else if (pcity->ai->grave_danger != 0 
               && bestchoice.type == CT_DEFENDER
               && assess_defense(pcity) == 0) {
      /* We have no gold but MUST have a defender */
      CITY_LOG(LOG_BUY, pcity, "must have %s but can't afford it (%d < %d)!",
               ai_choice_rule_name(&bestchoice),
               pplayer->economic.gold,
               buycost);
      try_to_sell_stuff(pplayer, pcity);
      if (pplayer->economic.gold - pplayer->ai_data.est_upkeep >= buycost) {
        CITY_LOG(LOG_BUY, pcity, "now we can afford it (sold something)");
        really_handle_city_buy(pplayer, pcity);
      }
      increase_maxbuycost(pplayer, buycost);
    }
  } while (TRUE);

  if (!war_footing) {
    /* Civilian upgrades now */
    city_list_iterate(pplayer->cities, pcity) {
      ai_upgrade_units(pcity, cached_limit, FALSE);
    } city_list_iterate_end;
  }

  freelog(LOG_BUY, "%s wants to keep %d in reserve (tax factor %d)", 
          player_name(pplayer), cached_limit, pplayer->ai_data.maxbuycost);
}

/**************************************************************************
  One of the top level AI functions.  It does (by calling other functions):
  worker allocations,
  build choices,
  extra gold spending.
**************************************************************************/
void ai_manage_cities(struct player *pplayer)
{
  pplayer->ai_data.maxbuycost = 0;

  TIMING_LOG(AIT_EMERGENCY, TIMER_START);
  city_list_iterate(pplayer->cities, pcity) {
    if (CITY_EMERGENCY(pcity)) {
      auto_arrange_workers(pcity); /* this usually helps */
    }
    if (CITY_EMERGENCY(pcity)) {
      /* Fix critical shortages or unhappiness */
      resolve_city_emergency(pplayer, pcity);
    }
    ai_sell_obsolete_buildings(pcity);
    sync_cities();
  } city_list_iterate_end;
  TIMING_LOG(AIT_EMERGENCY, TIMER_STOP);

  TIMING_LOG(AIT_BUILDINGS, TIMER_START);
  ai_manage_buildings(pplayer);
  TIMING_LOG(AIT_BUILDINGS, TIMER_STOP);

  /* Initialize the infrastructure cache, which is used shortly. */
  initialize_infrastructure_cache(pplayer);
  city_list_iterate(pplayer->cities, pcity) {
    /* Note that this function mungs the seamap, but we don't care */
    TIMING_LOG(AIT_CITY_MILITARY, TIMER_START);
    military_advisor_choose_build(pplayer, pcity, &pcity->ai->choice);
    TIMING_LOG(AIT_CITY_MILITARY, TIMER_STOP);
    if (ai_on_war_footing(pplayer) && pcity->ai->choice.want > 0) {
      continue; /* Go, soldiers! */
    }
    /* Will record its findings in pcity->settler_want */ 
    TIMING_LOG(AIT_CITY_TERRAIN, TIMER_START);
    contemplate_terrain_improvements(pcity);
    TIMING_LOG(AIT_CITY_TERRAIN, TIMER_STOP);

    TIMING_LOG(AIT_CITY_SETTLERS, TIMER_START);
    if (pcity->ai->founder_turn <= game.info.turn) {
      /* Will record its findings in pcity->founder_want */ 
      contemplate_new_city(pcity);
      /* Avoid recalculating all the time.. */
      pcity->ai->founder_turn = 
        game.info.turn + myrand(RECALC_SPEED) + RECALC_SPEED;
    } else if (pcity->debug) {
      /* recalculate every turn */
      contemplate_new_city(pcity);
    }
    TIMING_LOG(AIT_CITY_SETTLERS, TIMER_STOP);
    ASSERT_CHOICE(pcity->ai->choice);
  } city_list_iterate_end;

  city_list_iterate(pplayer->cities, pcity) {
    ai_city_choose_build(pplayer, pcity);
  } city_list_iterate_end;

  ai_spend_gold(pplayer);
}

/**************************************************************************
... 
**************************************************************************/
static bool building_unwanted(struct player *plr, struct impr_type *pimprove)
{
#if 0 /* This check will become more complicated now. */ 
  return (ai_wants_no_science(plr)
          && building_has_effect(pimprove, EFT_SCIENCE_BONUS));
#endif
  return FALSE;
}

/**************************************************************************
  Sell an obsolete building if there are any in the city.
**************************************************************************/
static void ai_sell_obsolete_buildings(struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);

  city_built_iterate(pcity, pimprove) {
    if (can_city_sell_building(pcity, pimprove) 
       && !building_has_effect(pimprove, EFT_DEFEND_BONUS)
	      /* selling city walls is really, really dumb -- Syela */
       && (is_building_replaced(pcity, pimprove, RPT_CERTAIN)
	   || building_unwanted(pplayer, pimprove))) {
      do_sell_building(pplayer, pcity, pimprove);
      notify_player(pplayer, pcity->tile, E_IMP_SOLD,
		       _("%s is selling %s (not needed) for %d."), 
		       city_name(pcity),
		       improvement_name_translation(pimprove), 
		       impr_sell_gold(pimprove));
      return; /* max 1 building each turn */
    }
  } city_built_iterate_end;
}

/**************************************************************************
  This function tries desperately to save a city from going under by
  revolt or starvation of food or resources. We do this by taking
  over resources held by nearby cities and disbanding units.

  TODO: Try to move units into friendly cities to reduce unhappiness
  instead of disbanding. Also rather starve city than keep it in
  revolt, as long as we don't lose settlers.

  TODO: Make function that tries to save units by moving them into
  cities that can upkeep them and change homecity rather than just
  disband. This means we'll have to move this function to beginning
  of AI turn sequence (before moving units).

  "I don't care how slow this is; it will very rarely be used." -- Syela

  Syela is wrong. It happens quite too often, mostly due to unhappiness.
  Also, most of the time we are unable to resolve the situation. 
**************************************************************************/
static void resolve_city_emergency(struct player *pplayer, struct city *pcity)
{
  struct tile *pcenter = city_tile(pcity);

  freelog(LOG_EMERGENCY,
          "Emergency in %s (%s, angry%d, unhap%d food%d, prod%d)",
          city_name(pcity),
          city_unhappy(pcity) ? "unhappy" : "content",
          pcity->feel[CITIZEN_ANGRY][FEELING_FINAL],
          pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL],
          pcity->surplus[O_FOOD],
          pcity->surplus[O_SHIELD]);

  city_tile_iterate(pcenter, atile) {
    struct city *acity = tile_worked(atile);

    if (acity && acity != pcity && city_owner(acity) == city_owner(pcity))  {
      int ax, ay;
      bool is_valid = city_base_to_city_map(&ax, &ay, acity, atile);

      freelog(LOG_EMERGENCY, "%s taking over %s square in (%d, %d)",
              city_name(pcity),
              city_name(acity),
              TILE_XY(atile));

      if (!is_valid) {
        assert(is_valid);
        continue;
      }

      if (is_free_worked(acity, atile)) {
	/* Can't remove a worker here. */
        continue;
      }

      city_map_update_empty(acity, atile, ax, ay);
      acity->specialists[DEFAULT_SPECIALIST]++;
      city_freeze_workers_queue(acity);
    }
  } city_tile_iterate_end;

  auto_arrange_workers(pcity);

  if (!CITY_EMERGENCY(pcity)) {
    freelog(LOG_EMERGENCY, "Emergency in %s resolved",
            city_name(pcity));
    goto cleanup;
  }

  unit_list_iterate_safe(pcity->units_supported, punit) {
    if (city_unhappy(pcity)
        && (utype_happy_cost(unit_type(punit), pplayer) > 0
            && (unit_being_aggressive(punit) || is_field_unit(punit)))
        && punit->ai.passenger == 0) {
      UNIT_LOG(LOG_EMERGENCY, punit, "is causing unrest, disbanded");
      handle_unit_disband(pplayer, punit->id);
      city_refresh(pcity);
    }
  } unit_list_iterate_safe_end;

  if (CITY_EMERGENCY(pcity)) {
    freelog(LOG_EMERGENCY, "Emergency in %s remains unresolved", 
            city_name(pcity));
  } else {
    freelog(LOG_EMERGENCY, 
            "Emergency in %s resolved by disbanding unit(s)",
            city_name(pcity));
  }

  cleanup:
  city_thaw_workers_queue();
  sync_cities();
}

/**************************************************************************
  Initialize city for use with default AI.
**************************************************************************/
void ai_init_city(struct city *pcity)
{
  pcity->ai = fc_calloc(1, sizeof(*pcity->ai));

  pcity->ai->building_wait = BUILDING_WAIT_MINIMUM;
  pcity->ai->trade_want = 1; /* we always want some TRADE_WEIGHTING */
}

/**************************************************************************
  Free city from use with default AI.
**************************************************************************/
void ai_close_city(struct city *pcity)
{
  if (pcity->ai) {
    free(pcity->ai);
    pcity->ai = NULL;
  }
}
