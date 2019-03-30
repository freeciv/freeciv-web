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

/* utility */
#include "distribute.h"
#include "log.h"
#include "shared.h"
#include "timing.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "unit.h"
#include "victory.h"

/* common/aicore */
#include "cm.h"

/* server */
#include "citytools.h"
#include "cityturn.h"
#include "plrhand.h"
#include "sernet.h"
#include "spacerace.h"
#include "srv_log.h"
#include "unithand.h"

/* server/advisors */
#include "advdata.h"
#include "advspace.h"
#include "advtools.h"

/* ai */
#include "handicaps.h"

/* ai/default */
#include "aicity.h"
#include "aidata.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aitech.h"
#include "aitools.h"
#include "aiunit.h"
#include "daidiplomacy.h"
#include "daimilitary.h"

#include "aihand.h"

/*****************************************************************************
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
*****************************************************************************/

#define LOGLEVEL_TAX LOG_DEBUG

/* When setting rates, we accept negative balance if we have a lot of
 * gold/bulb reserves. This is how long time gold/bulb reserves should last.
 * For city grow due to rapture turns_for_rapture is added. */
#define AI_GOLD_RESERVE_MIN_TURNS 10
#define AI_BULBS_RESERVE_MIN_TURNS 10

/* This factor is used for the delta between the estimated and real income to
 * get 'real' rates for the AI player. */
#define PCT_DELTA_TAX 50
#define PCT_DELTA_SCI 10

/*************************************************************************//**
  Handle spaceship related stuff
*****************************************************************************/
static void dai_manage_spaceship(struct player *pplayer)
{
  if (victory_enabled(VC_SPACERACE)) {
    if (pplayer->spaceship.state == SSHIP_STARTED) {
      adv_spaceship_autoplace(pplayer, &pplayer->spaceship);

      /* if we have built the best possible spaceship  -- AJS 19990610 */
      if ((pplayer->spaceship.structurals == NUM_SS_STRUCTURALS)
          && (pplayer->spaceship.components == NUM_SS_COMPONENTS)
          && (pplayer->spaceship.modules == NUM_SS_MODULES))
        handle_spaceship_launch(pplayer);
    }
  }
}

/*************************************************************************//**
  Returns the total amount of trade generated (trade) and total amount of
  gold needed as upkeep (expenses).
*****************************************************************************/
void dai_calc_data(const struct player *pplayer, int *trade, int *expenses,
                   int *income)
{
  if (NULL != trade) {
    *trade = 0;
  }
  if (NULL != expenses) {
    *expenses = 0;
  }
  if (NULL != income) {
    *income = 0;
  }

  /* Find total trade surplus and gold expenses */
  city_list_iterate(pplayer->cities, pcity) {
    if (NULL != trade) {
      *trade += pcity->surplus[O_TRADE];
    }

    if (NULL != expenses) {
      *expenses += pcity->usage[O_GOLD];
    }

    if (NULL != income) {
      *income += pcity->surplus[O_GOLD];
    }
  } city_list_iterate_end;

  switch (game.info.gold_upkeep_style) {
  case GOLD_UPKEEP_CITY:
    break;
  case GOLD_UPKEEP_MIXED:
  case GOLD_UPKEEP_NATION:
    /* Account for units with gold upkeep paid for by the nation. */
    unit_list_iterate(pplayer->units, punit) {
      *expenses += punit->upkeep[O_GOLD];
    } unit_list_iterate_end;
    break;
  }
}

/*****************************************************************************
  Additional data needed for the AI rates calculation
*****************************************************************************/
enum {
  AI_RATE_SCI = 0,
  AI_RATE_TAX,
  AI_RATE_LUX,
  AI_RATE_COUNT
};

enum celebration {
  AI_CELEBRATION_UNCHECKED,
  AI_CELEBRATION_NO,
  AI_CELEBRATION_YES,
};

#define RATE_NOT_SET -1
#define RATE_VALID(_rate)                                                    \
  (_rate != RATE_NOT_SET)
#define RATE_REMAINS(_rates)                                                 \
  MAX(0, 100 - _rates[AI_RATE_SCI] - _rates[AI_RATE_TAX]                     \
         - _rates[AI_RATE_LUX])

/*************************************************************************//**
  Set tax/science/luxury rates.

  TODO: Add general support for luxuries: select the luxury rate at which 
  all cities are content and the trade output (minus what is consumed by 
  luxuries) is maximal.  For this we need some more information from the 
  city management code.

  TODO: Audit the use of pplayer->ai.maxbuycost in the code elsewhere,
  then add support for it here.

  This function first determins the minimum tax rate (rate_tax_min) and a tax
  rate for a balanced treasury (rate_tax_balance). Similarily, the science
  rates are determiend (rate_sci_min and rate_sci_balance). Considering the
  minimum rates for tax and science the chance for celebrations is checked. If
  celebration is possible for more than half of the cities, the needed luxury
  rate is saved as rate_lux_min_celebrate. For celebration some reserves are
  defined (see turns_for_rapture).

  At the end the results are compared and the rates are selected as:

  1. Go for celebration.
  2. Use a balanced science and treasury.
  3. go for a balaned treasury / science.
  4. Try to select the best (risk of tech loss or have to sell something).

  At the end the remaining is divided on science/gold/luxury depending on the
  AI settings.
*****************************************************************************/
static void dai_manage_taxes(struct ai_type *ait, struct player *pplayer)
{
  int maxrate = (has_handicap(pplayer, H_RATES)
                 ? get_player_bonus(pplayer, EFT_MAX_RATES) : 100);
  struct research *research = research_get(pplayer);
  enum celebration celebrate = AI_CELEBRATION_UNCHECKED;
  struct adv_data *ai = adv_data_get(pplayer, NULL);
  int can_celebrate = 0, total_cities = 0;
  int trade = 0;         /* total amount of trade generated */
  int expenses = 0;      /* total amount of gold upkeep */
  int income = 0;        /* total amount of gold income */
  int turns_for_rapture; /* additional reserve needed for rapture */
  int rates_save[AI_RATE_COUNT], rates[AI_RATE_COUNT], result[AI_RATE_COUNT];
  int rate_tax_min = RATE_NOT_SET;
  int rate_tax_balance = RATE_NOT_SET;
  int rate_sci_min = RATE_NOT_SET;
  int rate_sci_balance = RATE_NOT_SET;
  int rate_lux_min_celebrate = maxrate;
  int delta_tax = 0, delta_sci = 0;

#ifdef DEBUG_TIMERS
  struct timer *taxtimer= NULL;
#endif

  struct cm_parameter cmp;

  if (!game.info.changable_tax) {
    return; /* This ruleset does not support changing tax rates. */
  }

  maxrate = CLIP(34, maxrate, 100);

  if (government_of_player(pplayer) == game.government_during_revolution) {
    return; /* This government does not support changing tax rates. */
  }

#ifdef DEBUG_TIMERS
  taxtimer = timer_new(TIMER_CPU, TIMER_DEBUG);
  timer_start(taxtimer);
#endif

  /* City parameters needed for celebrations. */
  cm_init_parameter(&cmp);
  cmp.require_happy = TRUE;    /* note this one */
  cmp.allow_disorder = FALSE;
  cmp.allow_specialists = TRUE;
  cmp.factor[O_FOOD] = 20;
  cmp.minimal_surplus[O_GOLD] = -FC_INFINITY;

  /* Define reserve (gold/bulbs) needed for rapture (celebration). */
  turns_for_rapture = ai->celebrate ? 0 : (game.info.rapturedelay + 1) * 3;

  log_base(LOGLEVEL_TAX, "%s [max] rate=%d", player_name(pplayer), maxrate);

  /* Save AI rates. */
  rates_save[AI_RATE_SCI] = pplayer->economic.science;
  rates_save[AI_RATE_TAX] = pplayer->economic.tax;
  rates_save[AI_RATE_LUX] = pplayer->economic.luxury;

  log_base(LOGLEVEL_TAX, "%s [old] (Sci/Lux/Tax)=%d/%d/%d",
           player_name(pplayer), rates_save[AI_RATE_SCI],
           rates_save[AI_RATE_LUX], rates_save[AI_RATE_TAX]);

  /* Get some data for the AI player. */
  dai_calc_data(pplayer, &trade, &expenses, &income);

  /* Get the estimates for tax with the current rates. */
  distribute(trade, AI_RATE_COUNT, rates_save, result);

  /* The delta between the estimate and the real value. */
  delta_tax = (result[AI_RATE_TAX] - expenses) - income;

  log_base(LOGLEVEL_TAX, "%s [tax] estimated=%d real=%d (delta=%d)",
           player_name(pplayer), result[AI_RATE_TAX] - expenses, income,
           delta_tax);

  /* Find minimum tax rate which gives us a positive balance for gold and
   * science. We assume that we want science most and luxuries least here,
   * and reverse or modify this assumption later.
   * Furthermore, We assume our entire civilization is one big city, and
   * distribute total income accordingly. This is a simplification that speeds
   * up the code significantly. */

  /* === Gold === */

  /* First set tax (gold) to the minimal available number */
  rates[AI_RATE_SCI] = maxrate; /* Assume we want science here */
  rates[AI_RATE_TAX] = MAX(0, 100 - maxrate * 2); /* If maxrate < 50% */
  rates[AI_RATE_LUX] = (100 - rates[AI_RATE_SCI] - rates[AI_RATE_TAX]);

  /* Now find the minimum tax with positive gold balance
   * Negative balance is acceptable if we have a lot of gold. */

  log_base(LOGLEVEL_TAX, "%s [tax] trade=%d gold=%d expenses=%d",
           player_name(pplayer), trade, pplayer->economic.gold, expenses);

  while (rates[AI_RATE_TAX] <= maxrate
         && rates[AI_RATE_SCI] >= 0
         && rates[AI_RATE_LUX] >= 0) {
    bool refill_coffers = pplayer->economic.gold < dai_gold_reserve(pplayer);
    int balance_tax, balance_tax_min;

    distribute(trade, AI_RATE_COUNT, rates, result);

    /* Consider the delta between the result and the real value from the
     * last turn to get better estimates. */
    balance_tax = (result[AI_RATE_TAX] - expenses)
                  - delta_tax * PCT_DELTA_TAX / 100;
    balance_tax_min = -(pplayer->economic.gold
                        / (AI_GOLD_RESERVE_MIN_TURNS + turns_for_rapture));

    log_base(LOGLEVEL_TAX, "%s [tax] Tax=%d income=%d",
             player_name(pplayer), rates[AI_RATE_TAX], balance_tax);

    if (!RATE_VALID(rate_tax_min) && balance_tax > balance_tax_min) {
      /* Just slightly negative; we can afford that for a while */
      rate_tax_min = rates[AI_RATE_TAX];
    }

    if (expenses == 0 || balance_tax > 0) {
      /* Ok, got no expenses or positive balance */
      if (refill_coffers && rates[AI_RATE_TAX] < maxrate) {
        /* Need to refill coffers, increase tax a bit */
        rates[AI_RATE_TAX] += 10;
        if (rates[AI_RATE_LUX] > 0) {
          rates[AI_RATE_LUX] -= 10;
        } else {
          rates[AI_RATE_SCI] -= 10;
        }

        log_base(LOGLEVEL_TAX, "%s [tax] Tax=%d to refill coffers",
                 player_name(pplayer), rates[AI_RATE_TAX]);
      }

      /* This is the minimum rate for a balanced treasury (gold). */
      rate_tax_balance = rates[AI_RATE_TAX];

      /* Done! Break the while loop */
      break;
    }

    /* Negative balance. Unacceptable - increase tax. */
    rates[AI_RATE_TAX] += 10;
    if (rates[AI_RATE_LUX] > 0) {
      rates[AI_RATE_LUX] -= 10;
    } else {
      rates[AI_RATE_SCI] -= 10;
    }
  }

  /* If no minimum value was found for the tax use the maximum tax rate. */
  rate_tax_min = RATE_VALID(rate_tax_min) ? rate_tax_min : maxrate;

  log_base(LOGLEVEL_TAX, "%s [tax] min=%d balanced=%d", player_name(pplayer),
           rate_tax_min, rate_tax_balance);

  /* === Science === */

  if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
    /* Tech upkeep activated. */
    int tech_upkeep = player_tech_upkeep(pplayer);
    int bulbs_researched = research->bulbs_researched;

    /* The delta between the estimate and the real value. */
    delta_sci = (result[AI_RATE_SCI] - tech_upkeep)
                - pplayer->server.bulbs_last_turn;
    log_base(LOGLEVEL_TAX, "%s [sci] estimated=%d real=%d (delta=%d)",
             player_name(pplayer),
             result[AI_RATE_SCI] - tech_upkeep,
             pplayer->server.bulbs_last_turn, delta_sci);

    log_base(LOGLEVEL_TAX, "%s [sci] trade=%d bulbs=%d upkeep=%d",
             player_name(pplayer), trade, research->bulbs_researched,
             tech_upkeep);

    rates[AI_RATE_TAX] = maxrate; /* Assume we want gold here */
    rates[AI_RATE_SCI] = MAX(0, 100 - maxrate * 2);
    rates[AI_RATE_LUX] = (100 - rates[AI_RATE_SCI] - rates[AI_RATE_TAX]);

    /* Now find the minimum science tax with positive bulbs balance
     * Negative balance is acceptable if we have a lots of bulbs. */
    while (rates[AI_RATE_SCI] <= maxrate
           && rates[AI_RATE_TAX] >= 0
           && rates[AI_RATE_LUX] >= 0) {
      int balance_sci, balance_sci_min;

      distribute(trade, AI_RATE_COUNT, rates, result);

      /* Consider the delta between the result and the real value from the
       * last turn. */
      balance_sci = (result[AI_RATE_SCI] - tech_upkeep)
                    - delta_sci * PCT_DELTA_SCI / 100;
      balance_sci_min = -(bulbs_researched
                          / (AI_BULBS_RESERVE_MIN_TURNS + turns_for_rapture));
      log_base(LOGLEVEL_TAX, "%s [sci] Sci=%d research=%d",
               player_name(pplayer), rates[AI_RATE_SCI], balance_sci);

      if (!RATE_VALID(rate_sci_min) && balance_sci > balance_sci_min) {
        /* Just slightly negative, if we can afford that for a while */
        rate_sci_min = rates[AI_RATE_SCI];
      }

      if (tech_upkeep == 0 || balance_sci > 0) {
        /* Ok, got no expenses or positive balance */
        rate_sci_balance = rates[AI_RATE_SCI];

        /* Done! Break the while loop */
        break;
      }

      /* Negative balance. Unacceptable - increase science.*/
      rates[AI_RATE_SCI] += 10;
      if (rates[AI_RATE_LUX] > 0) {
        rates[AI_RATE_LUX] -= 10;
      } else {
        rates[AI_RATE_TAX] -= 10;
      }
    }

    /* If no minimum value was found for the science use the maximum science
     * rate. */
    rate_sci_min = RATE_VALID(rate_sci_min) ? rate_sci_min : maxrate;

    log_base(LOGLEVEL_TAX, "%s [sci] min=%d balanced=%d",
             player_name(pplayer), rate_sci_min, rate_sci_balance);
  } else {
    /* No tech upkeep - minimum science value is 0. */
    rate_sci_min = 0;
  }

  /* === Luxury === */

  /* Should (and can) we celebrate? */
  /* TODO: In the future, we should check if we should 
   * celebrate for other reasons than growth. Currently 
   * this is ignored. Maybe we need ruleset AI hints. */
  /* TODO: Allow celebrate individual cities? No modpacks use this yet. */
  if (get_player_bonus(pplayer, EFT_RAPTURE_GROW) > 0
      && !has_handicap(pplayer, H_AWAY)
      && 100 > rate_tax_min + rate_sci_min) {
    celebrate = AI_CELEBRATION_NO;

    /* Set the minimum tax for a positive science and gold balance and use the
     * remaining trade goods for luxuries (max. luxuries). */
    rates[AI_RATE_SCI] = rate_sci_min;
    rates[AI_RATE_TAX] = rate_tax_min;
    rates[AI_RATE_LUX] = 0;

    rates[AI_RATE_LUX] = MIN(maxrate, rates[AI_RATE_LUX]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_TAX] = MIN(maxrate, rates[AI_RATE_TAX]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_SCI] = MIN(maxrate, rates[AI_RATE_SCI]
                                      + RATE_REMAINS(rates));

    /* Temporary set the new rates. */
    pplayer->economic.luxury = rates[AI_RATE_LUX];
    pplayer->economic.tax = rates[AI_RATE_TAX];
    pplayer->economic.science = rates[AI_RATE_SCI];

    /* Check if we celebrate - the city state must be restored at the end! */
    city_list_iterate(pplayer->cities, pcity) {
      struct cm_result *cmr = cm_result_new(pcity);
      struct ai_city *city_data = def_ai_city_data(pcity, ait);

      cm_clear_cache(pcity);
      cm_query_result(pcity, &cmp, cmr, FALSE); /* burn some CPU */

      total_cities++;

      if (cmr->found_a_valid
          && pcity->surplus[O_FOOD] > 0
          && city_size_get(pcity) >= game.info.celebratesize
          && city_can_grow_to(pcity, city_size_get(pcity) + 1)) {
        city_data->celebrate = TRUE;
        can_celebrate++;
      } else {
        city_data->celebrate = FALSE;
      }
      cm_result_destroy(cmr);
    } city_list_iterate_end;

    /* If more than half our cities can celebrate, go for it! */
    if (can_celebrate * 2 > total_cities) {
      celebrate = AI_CELEBRATION_YES;
      rate_lux_min_celebrate = pplayer->economic.luxury;
      log_base(LOGLEVEL_TAX, "%s [lux] celebration possible (Sci/Lux/Tax)>="
                             "%d/%d/%d (%d of %d cities)",
               player_name(pplayer), rate_sci_min, rate_lux_min_celebrate,
               rate_tax_min, can_celebrate, total_cities);
    } else {
      log_base(LOGLEVEL_TAX, "%s [lux] no celebration: only %d of %d cities",
               player_name(pplayer), can_celebrate, total_cities);
    }
  }

  if (celebrate != AI_CELEBRATION_YES) {
    /* TODO: Calculate a minimum luxury tax rate.
     *       Add general luxury code here. */
  }

  /* === Set the rates. === */

  /* Reset rates values. */
  rates[AI_RATE_SCI] = 0;
  rates[AI_RATE_TAX] = 0;
  rates[AI_RATE_LUX] = 0;

  /* Now decide that to do ... */
  if (celebrate == AI_CELEBRATION_YES
      && rate_tax_min + rate_sci_min + rate_lux_min_celebrate <= 100) {
    /* Celebration! */
    rates[AI_RATE_SCI] = rate_sci_min;
    rates[AI_RATE_TAX] = rate_tax_min;
    rates[AI_RATE_LUX] = rate_lux_min_celebrate;

    log_base(LOGLEVEL_TAX, "%s [res] celebration! (Sci/Lux/Tax)>=%d/%d/%d",
             player_name(pplayer), rates[AI_RATE_SCI], rates[AI_RATE_LUX],
             rates[AI_RATE_TAX]);
  } else {
    /* No celebration */
    celebrate = (celebrate == AI_CELEBRATION_YES) ? AI_CELEBRATION_NO
                                                  : celebrate;

    if (RATE_VALID(rate_tax_balance)) {
      if (RATE_VALID(rate_sci_balance)) {
        if (100 >= rate_tax_balance + rate_sci_balance) {
          /* A balanced treasury and research. */
          rates[AI_RATE_SCI] = rate_sci_balance;
          rates[AI_RATE_TAX] = rate_tax_balance;

          log_base(LOGLEVEL_TAX, "%s [res] balanced! (Sci/Lux/Tax)>=%d/%d/%d",
                   player_name(pplayer), rates[AI_RATE_SCI],
                   rates[AI_RATE_LUX], rates[AI_RATE_TAX]);
        } else {
          /* Try to keep all tech and get as much gold as possible. */
          rates[AI_RATE_SCI] = rate_sci_balance;
          rates[AI_RATE_TAX] = MIN(maxrate, RATE_REMAINS(rates));

          log_base(LOGLEVEL_TAX, "%s [res] balanced sci! tax? "
                                 "(Sci/Lux/Tax)>=%d/%d/%d",
                   player_name(pplayer), rates[AI_RATE_SCI],
                   rates[AI_RATE_LUX], rates[AI_RATE_TAX]);
        }
      } else {
        /* A balanced tax and as much science as possible. */
        rates[AI_RATE_TAX] = rate_tax_balance;
        rates[AI_RATE_SCI] = MIN(maxrate, RATE_REMAINS(rates));

        log_base(LOGLEVEL_TAX, "%s [res] balanced tax! sci? "
                               "(Sci/Lux/Tax)>=%d/%d/%d",
                 player_name(pplayer), rates[AI_RATE_SCI],
                 rates[AI_RATE_LUX], rates[AI_RATE_TAX]);
      }
    } else if (RATE_VALID(rate_sci_balance)) {
      /* Try to keep all techs and get as much gold as possible. */
      rates[AI_RATE_SCI] = rate_sci_balance;
      rates[AI_RATE_TAX] = MIN(maxrate, RATE_REMAINS(rates));

      log_base(LOGLEVEL_TAX, "%s [res] balanced sci! tax? "
                             "(Sci/Lux/Tax)>=%d/%d/%d",
               player_name(pplayer), rates[AI_RATE_SCI],
               rates[AI_RATE_LUX], rates[AI_RATE_TAX]);
    } else {
      /* We need more trade to get a positive gold and science balance. */
      if (!adv_wants_science(pplayer) || dai_on_war_footing(ait, pplayer)) {
        /* Go for gold (improvements and units) and risk the loss of a
         * tech. */
        rates[AI_RATE_TAX] = maxrate;
        rates[AI_RATE_SCI] = MIN(maxrate, RATE_REMAINS(rates));

        log_base(LOGLEVEL_TAX, "%s [res] risk of tech loss! (Sci/Lux/Tax)>="
                               "%d/%d/%d", player_name(pplayer),
                 rates[AI_RATE_SCI], rates[AI_RATE_LUX],
                 rates[AI_RATE_TAX]);
      } else {
        /* Go for science and risk the loss of improvements or units. */
        rates[AI_RATE_SCI] = MAX(maxrate, rate_sci_min);
        rates[AI_RATE_TAX] = MIN(maxrate, RATE_REMAINS(rates));

        log_base(LOGLEVEL_TAX, "%s [res] risk of empty treasury! "
                               "(Sci/Lux/Tax)>=%d/%d/%d",
                 player_name(pplayer), rates[AI_RATE_SCI],
                 rates[AI_RATE_LUX], rates[AI_RATE_TAX]);
      };
    }
  }

  /* Put the remaining to tax or science. */
  if (!adv_wants_science(pplayer)) {
    rates[AI_RATE_TAX] = MIN(maxrate, rates[AI_RATE_TAX]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_LUX] = MIN(maxrate, rates[AI_RATE_LUX]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_SCI] = MIN(maxrate, rates[AI_RATE_SCI]
                                      + RATE_REMAINS(rates));
  } else if (dai_on_war_footing(ait, pplayer)) {
    rates[AI_RATE_TAX] = MIN(maxrate, rates[AI_RATE_TAX]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_SCI] = MIN(maxrate, rates[AI_RATE_SCI]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_LUX] = MIN(maxrate, rates[AI_RATE_LUX]
                                      + RATE_REMAINS(rates));
  } else {
    rates[AI_RATE_SCI] = MIN(maxrate, rates[AI_RATE_SCI]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_TAX] = MIN(maxrate, rates[AI_RATE_TAX]
                                      + RATE_REMAINS(rates));
    rates[AI_RATE_LUX] = MIN(maxrate, rates[AI_RATE_LUX]
                                      + RATE_REMAINS(rates));
  }

  /* Check and set the calculated rates. */
  fc_assert_ret(0 <= rates[AI_RATE_SCI] && rates[AI_RATE_SCI] <= maxrate);
  fc_assert_ret(0 <= rates[AI_RATE_TAX] && rates[AI_RATE_TAX] <= maxrate);
  fc_assert_ret(0 <= rates[AI_RATE_LUX] && rates[AI_RATE_LUX] <= maxrate);
  fc_assert_ret(rates[AI_RATE_SCI] + rates[AI_RATE_TAX] + rates[AI_RATE_LUX]
                == 100);

  log_base(LOGLEVEL_TAX, "%s [new] (Sci/Lux/Tax)=%d/%d/%d",
           player_name(pplayer), rates[AI_RATE_SCI], rates[AI_RATE_LUX],
           rates[AI_RATE_TAX]);

  pplayer->economic.science = rates[AI_RATE_SCI];
  pplayer->economic.tax = rates[AI_RATE_TAX];
  pplayer->economic.luxury = rates[AI_RATE_LUX];

  /* === Cleanup === */

  /* Cancel all celebrations from the last turn. */
  ai->celebrate = FALSE;

  /* Now do celebrate or reset the city states if needed. */
  if (celebrate == AI_CELEBRATION_YES) {
    log_base(LOGLEVEL_TAX, "*** %s CELEBRATES! ***", player_name(pplayer));

    /* We do celebrate! */
    ai->celebrate = TRUE;

    city_list_iterate(pplayer->cities, pcity) {
      struct cm_result *cmr = cm_result_new(pcity);

      if (def_ai_city_data(pcity, ait)->celebrate == TRUE) {
        log_base(LOGLEVEL_TAX, "setting %s to celebrate", city_name_get(pcity));
        cm_query_result(pcity, &cmp, cmr, FALSE);
        if (cmr->found_a_valid) {
          apply_cmresult_to_city(pcity, cmr);
          city_refresh_from_main_map(pcity, NULL);
          if (!city_happy(pcity)) {
            CITY_LOG(LOG_ERROR, pcity, "is NOT happy when it should be!");
          }
        } else {
          CITY_LOG(LOG_ERROR, pcity, "has NO valid state!");
        }
      }
      cm_result_destroy(cmr);
    } city_list_iterate_end;
  } else if (celebrate == AI_CELEBRATION_NO) {
    city_list_iterate(pplayer->cities, pcity) {
      /* KLUDGE: Must refresh to restore the original values which
       * were clobbered in cm_query_result(), after the tax rates
       * were changed. */
      city_refresh_from_main_map(pcity, NULL);
    } city_list_iterate_end;
  }

  send_player_info_c(pplayer, pplayer->connections);

#ifdef DEBUG_TIMERS
  timer_stop(taxtimer);
  log_base(LOGLEVEL_TAX, "Tax calculation for %s (player %d) in %.3f "
                         "seconds.", player_name(pplayer),
           player_index(pplayer), timer_read_seconds(taxtimer));
  timer_destroy(taxtimer);
#endif /* DEBUG_TIMERS */
}
#undef RATE_NOT_SET
#undef RATE_VALID
#undef RATE_REMAINS

/*************************************************************************//**
  Change the government form, if it can and there is a good reason.
*****************************************************************************/
static void dai_manage_government(struct ai_type *ait, struct player *pplayer)
{
  struct adv_data *adv = adv_data_get(pplayer, NULL);

  if (!pplayer->is_alive || has_handicap(pplayer, H_AWAY)) {
    return;
  }

  if (adv->goal.revolution != government_of_player(pplayer)) {
    dai_government_change(pplayer, adv->goal.revolution); /* change */
  }

  /* Crank up tech want */
  if (adv->goal.govt.req == A_UNSET
      || research_invention_state(research_get(pplayer),
                                  adv->goal.govt.req) == TECH_KNOWN) {
    return; /* already got it! */
  } else if (adv->goal.govt.val > 0) {
    /* We have few cities in the beginning, compensate for this to ensure
     * that we are sufficiently forward-looking. */
    int want = MAX(adv->goal.govt.val, 100);
    struct nation_type *pnation = nation_of_player(pplayer);
    struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

    if (government_of_player(pplayer) == init_government_of_nation(pnation)) {
      /* Default government is the crappy one we start in (like Despotism).
       * We want something better pretty soon! */
      want += 25 * game.info.turn;
    }
    plr_data->tech_want[adv->goal.govt.req] += want;
    TECH_LOG(ait, LOG_DEBUG, pplayer, advance_by_number(adv->goal.govt.req), 
             "dai_manage_government() + %d for %s",
             want,
             government_rule_name(adv->goal.govt.gov));
  }
}

/*************************************************************************//**
  Activities to be done by AI _before_ human turn.  Here we just move the
  units intelligently.
*****************************************************************************/
void dai_do_first_activities(struct ai_type *ait, struct player *pplayer)
{
  TIMING_LOG(AIT_ALL, TIMER_START);
  dai_assess_danger_player(ait, pplayer, &(wld.map));
  /* TODO: Make assess_danger save information on what is threatening
   * us and make dai_manage_units and Co act upon this information, trying
   * to eliminate the source of danger */

  TIMING_LOG(AIT_UNITS, TIMER_START);
  dai_manage_units(ait, pplayer);
  TIMING_LOG(AIT_UNITS, TIMER_STOP);
  /* STOP.  Everything else is at end of turn. */

  TIMING_LOG(AIT_ALL, TIMER_STOP);

  flush_packets(); /* AIs can be such spammers... */
}

/*************************************************************************//**
  Activities to be done by AI _after_ human turn.  Here we respond to
  dangers created by human and AI opposition by ordering defenders in
  cities and setting taxes accordingly.  We also do other duties.

  We do _not_ move units here, otherwise humans complain that AI moves
  twice.
*****************************************************************************/
void dai_do_last_activities(struct ai_type *ait, struct player *pplayer)
{
  TIMING_LOG(AIT_ALL, TIMER_START);
  dai_clear_tech_wants(ait, pplayer);

  dai_manage_government(ait, pplayer);
  dai_adjust_policies(ait, pplayer);
  TIMING_LOG(AIT_TAXES, TIMER_START);
  dai_manage_taxes(ait, pplayer);
  TIMING_LOG(AIT_TAXES, TIMER_STOP);
  TIMING_LOG(AIT_CITIES, TIMER_START);
  dai_manage_cities(ait, pplayer);
  TIMING_LOG(AIT_CITIES, TIMER_STOP);
  TIMING_LOG(AIT_TECH, TIMER_START);
  dai_manage_tech(ait, pplayer); 
  TIMING_LOG(AIT_TECH, TIMER_STOP);
  dai_manage_spaceship(pplayer);

  TIMING_LOG(AIT_ALL, TIMER_STOP);
}
