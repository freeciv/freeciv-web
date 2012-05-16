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

#include "city.h"
#include "distribute.h"
#include "game.h"
#include "government.h"
#include "log.h"
#include "map.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "unit.h"
#include "timing.h"

#include "cm.h"

#include "citytools.h"
#include "cityturn.h"
#include "plrhand.h"
#include "settlers.h" /* amortize */
#include "spacerace.h"
#include "unithand.h"

#include "advmilitary.h"
#include "advspace.h"
#include "aicity.h"
#include "aidata.h"
#include "ailog.h"
#include "aitech.h"
#include "aitools.h"
#include "aiunit.h"

#include "aihand.h"

/****************************************************************************
  A man builds a city
  With banks and cathedrals
  A man melts the sand so he can 
  See the world outside
  A man makes a car 
  And builds a road to run them on
  A man dreams of leaving
  but he always stays behind
  And these are the days when our work has come assunder
  And these are the days when we look for something other
  /U2 Lemon.
******************************************************************************/

#define LOGLEVEL_TAX LOG_DEBUG

/* When setting rates, we accept negative balance if we have a lot of
 * gold reserves. This is how long time gold reserves should last */
#define AI_GOLD_RESERVE_MIN_TURNS 35

/**************************************************************************
 handle spaceship related stuff
**************************************************************************/
static void ai_manage_spaceship(struct player *pplayer)
{
  if (game.info.spacerace) {
    if (pplayer->spaceship.state == SSHIP_STARTED) {
      ai_spaceship_autoplace(pplayer, &pplayer->spaceship);
      /* if we have built the best possible spaceship  -- AJS 19990610 */
      if ((pplayer->spaceship.structurals == NUM_SS_STRUCTURALS) &&
        (pplayer->spaceship.components == NUM_SS_COMPONENTS) &&
        (pplayer->spaceship.modules == NUM_SS_MODULES))
        handle_spaceship_launch(pplayer);
    }
  }
}

/**************************************************************************
  Set tax/science/luxury rates.

  TODO: Add general support for luxuries: select the luxury rate at which 
  all cities are content and the trade output (minus what is consumed by 
  luxuries) is maximal.  For this we need some more information from the 
  city management code.

  TODO: Audit the use of pplayer->ai.maxbuycost in the code elsewhere,
  then add support for it here.
**************************************************************************/
static void ai_manage_taxes(struct player *pplayer) 
{
  int maxrate = (ai_handicap(pplayer, H_RATES) 
                 ? get_player_bonus(pplayer, EFT_MAX_RATES) : 100);
  bool celebrate = TRUE;
  int can_celebrate = 0, total_cities = 0;
  int trade = 0; /* total amount of trade generated */
  int expenses = 0; /* total amount of gold upkeep */
  int min_reserve = ai_gold_reserve(pplayer);
  bool refill_coffers = pplayer->economic.gold < min_reserve;

  if (!game.info.changable_tax) {
    return; /* This ruleset does not support changing tax rates. */
  }

  if (government_of_player(pplayer) == game.government_during_revolution) {
    return; /* This government does not support changing tax rates. */
  }

  /* Find total trade surplus and gold expenses */
  city_list_iterate(pplayer->cities, pcity) {
    trade += pcity->surplus[O_TRADE];
    expenses += pcity->usage[O_GOLD];
  } city_list_iterate_end;

  /* Find minimum tax rate which gives us a positive balance. We assume
   * that we want science most and luxuries least here, and reverse or 
   * modify this assumption later. on */

  /* First set tax to the minimal available number */
  pplayer->economic.science = maxrate; /* Assume we want science here */
  pplayer->economic.tax = MAX(0, 100 - maxrate * 2); /* If maxrate < 50% */
  pplayer->economic.luxury = (100 - pplayer->economic.science
                             - pplayer->economic.tax); /* Spillover */

  /* Now find the minimum tax with positive balance
   * Negative balance is acceptable if we have a lot of gold. */
  while(pplayer->economic.tax < maxrate
        && (pplayer->economic.science > 0
            || pplayer->economic.luxury > 0)) {
    int rates[3], result[3];
    const int SCIENCE = 0, TAX = 1, LUXURY = 2;

    /* Assume our entire civilization is one big city, and 
     * distribute total income accordingly. This is a 
     * simplification that speeds up the code significantly. */
    rates[SCIENCE] = pplayer->economic.science;
    rates[LUXURY] = pplayer->economic.luxury;
    rates[TAX] = 100 - rates[SCIENCE] - rates[LUXURY];
    distribute(trade, 3, rates, result);

    if (expenses - result[TAX] > 0
        && (refill_coffers
            || expenses - result[TAX] >
               pplayer->economic.gold / AI_GOLD_RESERVE_MIN_TURNS)) {
      /* Clearly negative balance. Unacceptable */
      pplayer->economic.tax += 10;
      if (pplayer->economic.luxury > 0) {
        pplayer->economic.luxury -= 10;
      } else {
        pplayer->economic.science -= 10;
      }
    } else {
      /* Ok, got positive balance
       * Or just slightly negative, if we can afford that for a while */
      if (refill_coffers) {
        /* Need to refill coffers, increase tax a bit */
        pplayer->economic.tax += 10;
        if (pplayer->economic.luxury > 0) {
          pplayer->economic.luxury -= 10;
        } else {
          pplayer->economic.science -= 10;
        }
      }
      /* Done! Break the while loop */
      break;
    }
  }

  /* Should we celebrate? */
  /* TODO: In the future, we should check if we should 
   * celebrate for other reasons than growth. Currently 
   * this is ignored. Maybe we need ruleset AI hints. */
  /* TODO: Allow celebrate individual cities? No modpacks use this yet. */
  if (get_player_bonus(pplayer, EFT_RAPTURE_GROW) > 0
      && !ai_handicap(pplayer, H_AWAY)) {
    int luxrate = pplayer->economic.luxury;
    int scirate = pplayer->economic.science;
    struct cm_parameter cmp;
    struct cm_result cmr;

    while (pplayer->economic.luxury < maxrate
           && pplayer->economic.science > 0) {
      pplayer->economic.luxury += 10;
      pplayer->economic.science -= 10;
    }

    cm_init_parameter(&cmp);
    cmp.require_happy = TRUE;    /* note this one */
    cmp.allow_disorder = FALSE;
    cmp.allow_specialists = TRUE;
    cmp.factor[O_FOOD] = 20;
    cmp.minimal_surplus[O_GOLD] = -FC_INFINITY;

    city_list_iterate(pplayer->cities, pcity) {
      cm_clear_cache(pcity);
      cm_query_result(pcity, &cmp, &cmr); /* burn some CPU */

      total_cities++;

      if (cmr.found_a_valid
          && pcity->surplus[O_FOOD] > 0
          && pcity->size >= game.info.celebratesize
	  && city_can_grow_to(pcity, pcity->size + 1)) {
        pcity->ai->celebrate = TRUE;
        can_celebrate++;
      } else {
        pcity->ai->celebrate = FALSE;
      }
    } city_list_iterate_end;
    /* If more than half our cities can celebrate, go for it! */
    celebrate = (can_celebrate * 2 > total_cities);
    if (celebrate) {
      freelog(LOGLEVEL_TAX, "*** %s CELEBRATES! ***", player_name(pplayer));
      city_list_iterate(pplayer->cities, pcity) {
        if (pcity->ai->celebrate == TRUE) {
          freelog(LOGLEVEL_TAX, "setting %s to celebrate", city_name(pcity));
          cm_query_result(pcity, &cmp, &cmr);
          if (cmr.found_a_valid) {
            apply_cmresult_to_city(pcity, &cmr);
            city_refresh_from_main_map(pcity, TRUE);
            if (!city_happy(pcity)) {
              CITY_LOG(LOG_ERROR, pcity, "is NOT happy when it should be!");
            }
          }
        }
      } city_list_iterate_end;
    } else {
      pplayer->economic.luxury = luxrate;
      pplayer->economic.science = scirate;
      city_list_iterate(pplayer->cities, pcity) {
        /* KLUDGE: Must refresh to restore the original values which
         * were clobbered in cm_query_result(), after the tax rates
         * were changed. */
        city_refresh_from_main_map(pcity, TRUE);
      } city_list_iterate_end;
    }
  }

  if (!celebrate && pplayer->economic.luxury < maxrate) {
    /* TODO: Add general luxury code here. */
  }

  /* Ok, we now have the desired tax and luxury rates. Do we really want
   * science? If not, swap it with tax if it is bigger. */
  if ((ai_wants_no_science(pplayer) || ai_on_war_footing(pplayer))
      && pplayer->economic.science > pplayer->economic.tax) {
    int science = pplayer->economic.science;
    /* Swap science and tax */
    pplayer->economic.science = pplayer->economic.tax;
    pplayer->economic.tax = science;
  }

  assert(pplayer->economic.tax + pplayer->economic.luxury 
         + pplayer->economic.science == 100);
  freelog(LOGLEVEL_TAX, "%s rates: Sci=%d Lux=%d Tax=%d trade=%d expenses=%d"
          " celeb=(%d/%d)", player_name(pplayer), pplayer->economic.science,
          pplayer->economic.luxury, pplayer->economic.tax, trade, expenses,
          can_celebrate, total_cities);
  send_player_info(pplayer, pplayer);
}

/**************************************************************************
  Find best government to aim for.
  We do it by setting our government to all possible values and calculating
  our GDP (total ai_eval_calc_city) under this government.  If the very
  best of the governments is not available to us (it is not yet discovered),
  we record it in the goal.gov structure with the aim of wanting the
  necessary tech more.  The best of the available governments is recorded 
  in goal.revolution.  We record the want of each government, and only
  recalculate this data every ai->govt_reeval_turns turns.

  Note: Call this _before_ doing taxes!
**************************************************************************/
void ai_best_government(struct player *pplayer)
{
  struct ai_data *ai = ai_data_get(pplayer);
  int best_val = 0;
  int bonus = 0; /* in percentage */
  struct government *current_gov = government_of_player(pplayer);

  ai->goal.govt.gov = current_gov;
  ai->goal.govt.val = 0;
  ai->goal.govt.req = A_UNSET;
  ai->goal.revolution = current_gov;

  if (ai_handicap(pplayer, H_AWAY) || !pplayer->is_alive) {
    return;
  }

  if (ai->govt_reeval == 0) {
    government_iterate(gov) {
      int val = 0;
      int dist;

      if (gov == game.government_during_revolution) {
        continue; /* pointless */
      }
      if (gov->ai.better
          && can_change_to_government(pplayer, gov->ai.better)) {
        continue; /* we have better governments available */
      }
      pplayer->government = gov;
      /* Ideally we should change tax rates here, but since
       * this is a rather big CPU operation, we'd rather not. */
      check_player_max_rates(pplayer);
      city_list_iterate(pplayer->cities, acity) {
        auto_arrange_workers(acity);
      } city_list_iterate_end;
      city_list_iterate(pplayer->cities, pcity) {
        val += ai_eval_calc_city(pcity, ai);
      } city_list_iterate_end;

      /* Bonuses for non-economic abilities. We increase val by
       * a very small amount here to choose govt in cases where
       * we have no cities yet. */
      bonus += get_player_bonus(pplayer, EFT_VETERAN_BUILD) ? 3 : 0;
      bonus -= get_player_bonus(pplayer, EFT_REVOLUTION_WHEN_UNHAPPY) ? 3 : 0;
      bonus += get_player_bonus(pplayer, EFT_NO_INCITE) ? 4 : 0;
      bonus += get_player_bonus(pplayer, EFT_UNBRIBABLE_UNITS) ? 2 : 0;
      bonus += get_player_bonus(pplayer, EFT_INSPIRE_PARTISANS) ? 3 : 0;
      bonus += get_player_bonus(pplayer, EFT_RAPTURE_GROW) ? 2 : 0;
      bonus += get_player_bonus(pplayer, EFT_FANATICS) ? 3 : 0;
      bonus += get_player_bonus(pplayer, EFT_OUTPUT_INC_TILE) * 8;

      val += (val * bonus) / 100;

      /* FIXME: handle reqs other than technologies. */
      dist = 0;
      requirement_vector_iterate(&gov->reqs, preq) {
	if (VUT_ADVANCE == preq->source.kind) {
	  dist += MAX(1, num_unknown_techs_for_goal(pplayer,
						    advance_number(preq->source.value.advance)));
	}
      } requirement_vector_iterate_end;
      val = amortize(val, dist);
      ai->government_want[government_index(gov)] = val; /* Save want */
    } government_iterate_end;
    /* Now reset our gov to it's real state. */
    pplayer->government = current_gov;
    city_list_iterate(pplayer->cities, acity) {
      auto_arrange_workers(acity);
    } city_list_iterate_end;
    ai->govt_reeval = CLIP(5, city_list_size(pplayer->cities), 20);
  }
  ai->govt_reeval--;

  /* Figure out which government is the best for us this turn. */
  government_iterate(gov) {
    int gi = government_index(gov);
    if (ai->government_want[gi] > best_val 
        && can_change_to_government(pplayer, gov)) {
      best_val = ai->government_want[gi];
      ai->goal.revolution = gov;
    }
    if (ai->government_want[gi] > ai->goal.govt.val) {
      ai->goal.govt.gov = gov;
      ai->goal.govt.val = ai->government_want[gi];

      /* FIXME: handle reqs other than technologies. */
      ai->goal.govt.req = A_NONE;
      requirement_vector_iterate(&gov->reqs, preq) {
	if (VUT_ADVANCE == preq->source.kind) {
	  ai->goal.govt.req = advance_number(preq->source.value.advance);
	  break;
	}
      } requirement_vector_iterate_end;
    }
  } government_iterate_end;
  /* Goodness of the ideal gov is calculated relative to the goodness of the
   * best of the available ones. */
  ai->goal.govt.val -= best_val;
}

/**************************************************************************
  Change the government form, if it can and there is a good reason.
**************************************************************************/
static void ai_manage_government(struct player *pplayer)
{
  struct ai_data *ai = ai_data_get(pplayer);

  if (!pplayer->is_alive || ai_handicap(pplayer, H_AWAY)) {
    return;
  }

  if (ai->goal.revolution != government_of_player(pplayer)) {
    ai_government_change(pplayer, ai->goal.revolution); /* change */
  }

  /* Crank up tech want */
  if (ai->goal.govt.req == A_UNSET
      || player_invention_state(pplayer, ai->goal.govt.req) == TECH_KNOWN) {
    return; /* already got it! */
  } else if (ai->goal.govt.val > 0) {
    /* We have few cities in the beginning, compensate for this to ensure
     * that we are sufficiently forward-looking. */
    int want = MAX(ai->goal.govt.val, 100);
    struct nation_type *pnation = nation_of_player(pplayer);

    if (government_of_player(pplayer) == pnation->init_government) {
      /* Default government is the crappy one we start in (like Despotism).
       * We want something better pretty soon! */
      want += 25 * game.info.turn;
    }
    pplayer->ai_data.tech_want[ai->goal.govt.req] += want;
    TECH_LOG(LOG_DEBUG, pplayer, advance_by_number(ai->goal.govt.req), 
             "ai_manage_government() + %d for %s",
             want,
             government_rule_name(ai->goal.govt.gov));
  }
}

/**************************************************************************
  Activities to be done by AI _before_ human turn.  Here we just move the
  units intelligently.
**************************************************************************/
void ai_do_first_activities(struct player *pplayer)
{
  TIMING_LOG(AIT_ALL, TIMER_START);
  assess_danger_player(pplayer);
  /* TODO: Make assess_danger save information on what is threatening
   * us and make ai_mange_units and Co act upon this information, trying
   * to eliminate the source of danger */

  TIMING_LOG(AIT_UNITS, TIMER_START);
  ai_manage_units(pplayer); 
  TIMING_LOG(AIT_UNITS, TIMER_STOP);
  /* STOP.  Everything else is at end of turn. */

  TIMING_LOG(AIT_ALL, TIMER_STOP);
}

/**************************************************************************
  Activities to be done by AI _after_ human turn.  Here we respond to 
  dangers created by human and AI opposition by ordering defenders in 
  cities and setting taxes accordingly.  We also do other duties.  

  We do _not_ move units here, otherwise humans complain that AI moves 
  twice.
**************************************************************************/
void ai_do_last_activities(struct player *pplayer)
{
  TIMING_LOG(AIT_ALL, TIMER_START);

  ai_manage_government(pplayer);
  TIMING_LOG(AIT_TAXES, TIMER_START);
  ai_manage_taxes(pplayer); 
  TIMING_LOG(AIT_TAXES, TIMER_STOP);
  TIMING_LOG(AIT_CITIES, TIMER_START);
  ai_manage_cities(pplayer);
  TIMING_LOG(AIT_CITIES, TIMER_STOP);
  TIMING_LOG(AIT_TECH, TIMER_START);
  ai_manage_tech(pplayer); 
  TIMING_LOG(AIT_TECH, TIMER_STOP);
  ai_manage_spaceship(pplayer);

  TIMING_LOG(AIT_ALL, TIMER_STOP);
}
