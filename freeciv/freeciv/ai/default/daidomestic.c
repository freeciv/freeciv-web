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
#include <math.h> /* pow, ceil */

/* utility */
#include "log.h"
#include "mem.h"

/* common */
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "traderoutes.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"

/* common/aicore */
#include "pf_tools.h"

/* server */
#include "citytools.h"
#include "unittools.h"
#include "srv_log.h"

/* server/advisors */
#include "advbuilding.h"
#include "advchoice.h"
#include "advdata.h"
#include "autosettlers.h"
#include "infracache.h" /* adv_city */

/* ai */
#include "aitraits.h"
#include "handicaps.h"

/* ai/default */
#include "aicity.h"
#include "aidata.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aitech.h"
#include "aitools.h"
#include "aiunit.h"
#include "daimilitary.h"

#include "daidomestic.h"


/***********************************************************************//**
  Evaluate the need for units (like caravans) that aid wonder construction.
  If another city is building wonder and needs help but pplayer is not
  advanced enough to build caravans, the corresponding tech will be 
  stimulated.
****************************************************************************/
static void dai_choose_help_wonder(struct ai_type *ait,
                                   struct city *pcity,
                                   struct adv_choice *choice,
                                   struct adv_data *ai)
{
  struct player *pplayer = city_owner(pcity);
  Continent_id continent = tile_continent(pcity->tile);
  struct ai_city *city_data = def_ai_city_data(pcity, ait);
  /* Total count of caravans available or already being built
   * on this continent */
  int caravans = 0;
  /* The type of the caravan */
  struct unit_type *unit_type;
  struct city *wonder_city = game_city_by_number(ai->wonder_city);

  if (num_role_units(action_id_get_role(ACTION_HELP_WONDER)) == 0) {
    /* No such units available in the ruleset */
    return;
  }

  if (pcity == wonder_city 
      || wonder_city == NULL
      || city_data->distance_to_wonder_city <= 0
      || !city_production_gets_caravan_shields(&wonder_city->production)
      /* TODO: Should helping to build a unit be considered when legal? */
      || VUT_UTYPE == wonder_city->production.kind
      /* TODO: Should helping to build an improvement be considered when
       * legal? */
      || !is_wonder(wonder_city->production.value.building)) {
    /* A distance of zero indicates we are very far away, possibly
     * on another continent. */
    return;
  }

  /* Count existing caravans */
  unit_list_iterate(pplayer->units, punit) {
    if (unit_can_do_action(punit, ACTION_HELP_WONDER)
        && tile_continent(unit_tile(punit)) == continent)
      caravans++;
  } unit_list_iterate_end;

  /* Count caravans being built */
  city_list_iterate(pplayer->cities, acity) {
    if (VUT_UTYPE == acity->production.kind
        && utype_can_do_action(acity->production.value.utype,
                               ACTION_HELP_WONDER)
        && tile_continent(acity->tile) == continent) {
      caravans++;
    }
  } city_list_iterate_end;

  unit_type = best_role_unit(pcity, action_id_get_role(ACTION_HELP_WONDER));

  if (!unit_type) {
    /* We cannot build such units yet
     * but we will consider it to stimulate science */
    unit_type = get_role_unit(action_id_get_role(ACTION_HELP_WONDER), 0);
  }

  fc_assert_msg(utype_can_do_action(unit_type, ACTION_HELP_WONDER),
                "Non existence of wonder helper unit not caught");

  /* Check if wonder needs a little help. */
  if (build_points_left(wonder_city)
      > utype_build_shield_cost_base(unit_type) * caravans) {
    struct impr_type *wonder = wonder_city->production.value.building;
    adv_want want = wonder_city->server.adv->building_want[improvement_index(wonder)];
    int dist = city_data->distance_to_wonder_city /
               unit_type->move_rate;

    fc_assert_ret(VUT_IMPROVEMENT == wonder_city->production.kind);

    want /= MAX(dist, 1);
    CITY_LOG(LOG_DEBUG, pcity, "want %s to help wonder in %s with " ADV_WANT_PRINTF, 
             utype_rule_name(unit_type),
             city_name_get(wonder_city),
             want);
    if (want > choice->want) {
      /* This sets our tech want in cases where we cannot actually build
       * the unit. */
      unit_type = dai_wants_role_unit(ait, pplayer, pcity,
                                      action_id_get_role(ACTION_HELP_WONDER),
                                      want);
      if (unit_type != NULL) {
        choice->want = want;
        choice->type = CT_CIVILIAN;
        choice->value.utype = unit_type;
      } else {
        CITY_LOG(LOG_DEBUG, pcity,
                 "would but could not build ACTION_HELP_WONDER unit, "
                 "bumped reqs");
      }
    }
  }
}

/***********************************************************************//**
  Evaluate the need for units (like caravans) that create trade routes.
  If pplayer is not advanced enough to build caravans, the corresponding
  tech will be stimulated.
***************************************************************************/
static void dai_choose_trade_route(struct ai_type *ait, struct city *pcity,
                                   struct adv_choice *choice,
                                   struct adv_data *ai)
{
  struct player *pplayer = city_owner(pcity);
  struct unit_type *unit_type;
  adv_want want;
  int income, bonus;
  int trade_routes;
  int caravan_units;
  int unassigned_caravans;
  int max_routes;
  Continent_id continent = tile_continent(pcity->tile);
  bool dest_city_found = FALSE;
  bool dest_city_nat_different_cont = FALSE;
  bool dest_city_nat_same_cont = FALSE;
  bool dest_city_in_different_cont = FALSE;
  bool dest_city_in_same_cont = FALSE;
  bool prefer_different_cont;
  int pct = 0;
  int trader_trait;
  bool need_boat = FALSE;
  int trade_action;

  if (city_list_size(pplayer->cities) < 5) {
    /* Consider trade routes only if enough destination cities.
     * This is just a quick check. We make more detailed check below. */
    return;
  }

  if (num_role_units(action_id_get_role(ACTION_TRADE_ROUTE)) == 0
      && num_role_units(action_id_get_role(ACTION_MARKETPLACE)) == 0) {
    /* No such units available in the ruleset */
    return;
  }

  if (trade_route_type_trade_pct(TRT_NATIONAL_IC) >
      trade_route_type_trade_pct(TRT_NATIONAL)) {
    prefer_different_cont = TRUE;
  } else {
    prefer_different_cont = FALSE;
  }

  /* Look for proper destination city for trade. */
  if (trade_route_type_trade_pct(TRT_NATIONAL) > 0
      || trade_route_type_trade_pct(TRT_NATIONAL_IC) > 0) {
    /* National traderoutes have value */
    city_list_iterate(pplayer->cities, acity) {
      if (can_cities_trade(pcity, acity)) {
        dest_city_found = TRUE;
        if (tile_continent(acity->tile) != continent) {
          dest_city_nat_different_cont = TRUE;
          if (prefer_different_cont) {
            break;
          }
        } else {
          dest_city_nat_same_cont = TRUE;
          if (!prefer_different_cont) {
            break;
          }
        }
      }
    } city_list_iterate_end;
  }

  /* FIXME: This check should consider more about relative
   * income from different traderoute types. This works just
   * with more typical ruleset setups. */
  if (prefer_different_cont && !dest_city_nat_different_cont) {
    if (trade_route_type_trade_pct(TRT_IN_IC) >
        trade_route_type_trade_pct(TRT_IN)) {
      prefer_different_cont = TRUE;
    } else {
      prefer_different_cont = FALSE;
    }

    players_iterate(aplayer) {
      if (aplayer == pplayer || !aplayer->is_alive) {
	continue;
      }
      if (pplayers_allied(pplayer, aplayer)) {
        city_list_iterate(aplayer->cities, acity) {
          if (can_cities_trade(pcity, acity)) {
            dest_city_found = TRUE;
            if (tile_continent(acity->tile) != continent) {
              dest_city_in_different_cont = TRUE;
              if (prefer_different_cont) {
                break;
              }
            } else {
              dest_city_in_same_cont = TRUE;
              if (!prefer_different_cont) {
                break;
              }
            }
          }
        } city_list_iterate_end;
      }
      if ((dest_city_in_different_cont && prefer_different_cont)
          || (dest_city_in_same_cont && !prefer_different_cont)) {
        break;
      }
    } players_iterate_end;
  }

  if (!dest_city_found) {
    /* No proper destination city. */
    return;
  }

  unit_type = best_role_unit(pcity,
                             action_id_get_role(ACTION_TRADE_ROUTE));

  if (!unit_type) {
    /* Can't establish trade route yet. What about entering a marketplace? */
    /* TODO: Should a future unit capable of establishing trade routes be
     * prioritized above a present unit capable of entering a market place?
     * In that case this should be below the check for a future unit
     * capable of establishing a trade route. */
    unit_type = best_role_unit(pcity,
                               action_id_get_role(ACTION_MARKETPLACE));
  }

  if (!unit_type) {
    /* We cannot build such units yet
     * but we will consider it to stimulate science */
    unit_type = get_role_unit(action_id_get_role(ACTION_TRADE_ROUTE), 0);
  }

  if (!unit_type) {
    /* We'll never be able to establish a trade route. Consider a unit that
     * can enter the marketplace in stead to stimulate science. */
    unit_type = get_role_unit(action_id_get_role(ACTION_MARKETPLACE), 0);
  }

  fc_assert_msg(unit_type,
                "Non existance of trade unit not caught");

  trade_routes = city_num_trade_routes(pcity);
  /* Count also caravans enroute to establish traderoutes */
  caravan_units = 0;
  unit_list_iterate(pcity->units_supported, punit) {
    if (unit_can_do_action(punit, ACTION_TRADE_ROUTE)) {
      caravan_units++;
    }
  } unit_list_iterate_end;

  max_routes = max_trade_routes(pcity);
  unassigned_caravans = caravan_units - (max_routes - trade_routes);
  trade_routes += caravan_units;

  /* We consider only initial benefit from establishing trade route.
   * We may actually get only initial benefit if both cities already
   * have four trade routes, if there already is route between them
   * or if the Establish Trade Route action is illegal. */

  /* The calculations of get_caravan_enter_city_trade_bonus() have to be
   * duplicated here because the city traded with is imaginary. */

  /* We assume that we are creating trade route to city with 75% of
   * pcitys trade 10 squares away. */
  income = (10 + 10) * (1.75 * pcity->surplus[O_TRADE]) / 24;

  /* A ruleset may use the Trade_Revenue_Bonus effect to reduce the one
   * time bonus if no trade route is established. Make sure it gets the
   * correct action. */
  trade_action = utype_can_do_action(unit_type, ACTION_TRADE_ROUTE) ?
        ACTION_TRADE_ROUTE : ACTION_MARKETPLACE;
  bonus = get_target_bonus_effects(NULL,
                                   pplayer, NULL,
                                   pcity, NULL,
                                   city_tile(pcity),
                                   NULL, NULL,
                                   NULL, NULL,
                                   action_by_number(trade_action),
                                   EFT_TRADE_REVENUE_BONUS);

  /* Be mercy full to players with small amounts. Round up. */
  income = ceil((float)income * pow(2.0, (double)bonus / 1000.0));

  if (dest_city_nat_same_cont) {
    pct = trade_route_type_trade_pct(TRT_NATIONAL);
  }
  if (dest_city_in_same_cont) {
    int typepct = trade_route_type_trade_pct(TRT_IN);

    pct = MAX(pct, typepct);
  }
  if (dest_city_nat_different_cont) {
    int typepct = trade_route_type_trade_pct(TRT_NATIONAL_IC);

    if (typepct > pct) {
      pct = typepct;
      need_boat = TRUE;
    }
  }
  if (dest_city_in_different_cont) {
    int typepct = trade_route_type_trade_pct(TRT_IN_IC);

    if (typepct > pct) {
      pct = typepct;
      need_boat = TRUE;
    }
  }

  income = pct * income / 100;

  want = income * ai->gold_priority + income * ai->science_priority;

  /* We get this income only once after all.
   * This value is adjusted for most interesting gameplay.
   * For optimal performance AI should build more caravans, but
   * we want it to build less valued buildings too. */
  trader_trait = ai_trait_get_value(TRAIT_TRADER, pplayer);
  want /= (130 * TRAIT_DEFAULT_VALUE / trader_trait);

  /* Increase want for trade routes if our economy is very weak.
   * We may have enough gold, but only because we have already set
   * tax rate to 100%. So don't check gold directly, but tax rate.
   * This method helps us out of deadlocks of completely stalled
   * scientific progress.
   */
  if (pplayer->economic.science < 50 && trade_routes < max_routes
      && utype_can_do_action(unit_type, ACTION_TRADE_ROUTE)) {
    want *=
      (6 - pplayer->economic.science / 10) * (6 - pplayer->economic.science / 10);
  }

  if (trade_routes == 0 && max_routes > 0
      && utype_can_do_action(unit_type, ACTION_TRADE_ROUTE)) {
    /* If we have no trade routes at all, we are certainly creating a new one. */
    want += trader_trait;
  } else if (trade_routes < max_routes
             && utype_can_do_action(unit_type, ACTION_TRADE_ROUTE)) {
    /* Possibly creating a new trade route */
    want += trader_trait / 4;
  }

  want -= utype_build_shield_cost(pcity, unit_type) * SHIELD_WEIGHTING / 150;

  /* Don't pile too many of them */
  if (unassigned_caravans * 10 > want && want > 0.0) {
    want = 0.1;
  } else {
    want -= unassigned_caravans * 10; /* Don't pile too many of them */
  }

  CITY_LOG(LOG_DEBUG, pcity,
           "want for trade route unit is " ADV_WANT_PRINTF " (expected initial income %d)",
           want,
           income);

  if (want > choice->want) {
    /* This sets our tech want in cases where we cannot actually build
     * the unit. */
    unit_type = dai_wants_role_unit(ait, pplayer, pcity,
                                    action_id_get_role(ACTION_TRADE_ROUTE),
                                    want);

    if (unit_type == NULL) {
      unit_type = dai_wants_role_unit(ait, pplayer, pcity,
                                      action_id_get_role(ACTION_MARKETPLACE),
                                      want);
    }

    if (unit_type != NULL) {
      choice->want = want;
      choice->type = CT_CIVILIAN;
      choice->value.utype = unit_type;
      choice->need_boat = need_boat;
    } else {
      CITY_LOG(LOG_DEBUG, pcity,
               "would but could not build trade route unit, bumped reqs");
    }
  }
}

/***********************************************************************//**
  This function should fill the supplied choice structure.

  If want is 0, this advisor doesn't want anything.
***************************************************************************/
struct adv_choice *domestic_advisor_choose_build(struct ai_type *ait, struct player *pplayer,
                                                 struct city *pcity)
{
  struct adv_data *adv = adv_data_get(pplayer, NULL);
  /* Unit type with certain role */
  struct unit_type *worker_type;
  struct unit_type *founder_type;
  adv_want worker_want, founder_want;
  struct ai_city *city_data = def_ai_city_data(pcity, ait);
  struct adv_choice *choice = adv_new_choice();

  /* Find out desire for workers (terrain improvers) */
  worker_type = city_data->worker_type;

  /* The worker want is calculated in aicity.c called from
   * dai_manage_cities.  The expand value is the % that the AI should
   * value expansion (basically to handicap easier difficulty levels)
   * and is set when the difficulty level is changed (difficulty.c). */
  worker_want = city_data->worker_want * pplayer->ai_common.expand / 100;

  if (adv->wonder_city == pcity->id) {
    if (worker_type == NULL || worker_type->pop_cost > 0) {
      worker_want /= 5;
    } else {
      worker_want /= 2;
    }
  }

  if (worker_type != NULL
      && pcity->surplus[O_FOOD] > utype_upkeep_cost(worker_type,
                                                    pplayer, O_FOOD)) {
    if (worker_want > 0) {
      CITY_LOG(LOG_DEBUG, pcity, "desires terrain improvers with passion " ADV_WANT_PRINTF, 
               worker_want);
      dai_choose_role_unit(ait, pplayer, pcity, choice, CT_CIVILIAN,
                           UTYF_SETTLERS, worker_want, FALSE);
      adv_choice_set_use(choice, "worker");
    }
    /* Terrain improvers don't use boats (yet) */

  } else if (worker_type == NULL && worker_want > 0) {
    /* Can't build workers. Lets stimulate science */
    dai_wants_role_unit(ait, pplayer, pcity, UTYF_SETTLERS, worker_want);
  }

  /* Find out desire for city founders */
  /* Basically, copied from above and adjusted. -- jjm */
  if (!game.scenario.prevent_new_cities) {
    founder_type = best_role_unit(pcity,
                                  action_id_get_role(ACTION_FOUND_CITY));

    /* founder_want calculated in aisettlers.c */
    founder_want = city_data->founder_want;

    if (adv->wonder_city == pcity->id) {
      if (founder_type == NULL || founder_type->pop_cost > 0) {
        founder_want /= 5;
      } else {
        founder_want /= 2;
      }
    }

    if (adv->max_num_cities <= city_list_size(pplayer->cities)) {
      founder_want /= 100;
    }

    /* Adjust founder want by traits */
    founder_want *= (double)ai_trait_get_value(TRAIT_EXPANSIONIST, pplayer)
      / TRAIT_DEFAULT_VALUE;

    if (founder_type
        && pcity->surplus[O_FOOD] >= utype_upkeep_cost(founder_type,
                                                       pplayer, O_FOOD)) {

      if (founder_want > choice->want) {
        CITY_LOG(LOG_DEBUG, pcity, "desires founders with passion " ADV_WANT_PRINTF,
                 founder_want);
        dai_choose_role_unit(ait, pplayer, pcity, choice, CT_CIVILIAN,
                             action_id_get_role(ACTION_FOUND_CITY),
                             founder_want,
                             city_data->founder_boat);
        adv_choice_set_use(choice, "founder");

      } else if (founder_want < -choice->want) {
        /* We need boats to colonize! */
        /* We might need boats even if there are boats free,
         * if they are blockaded or in inland seas. */
        struct ai_plr *ai = dai_plr_data_get(ait, pplayer, NULL);

        CITY_LOG(LOG_DEBUG, pcity, "desires founders with passion " ADV_WANT_PRINTF
                 " and asks for a new boat (%d of %d free)",
                 -founder_want, ai->stats.available_boats, ai->stats.boats);

        /* First fill choice with founder information */
        choice->want = 0 - founder_want;
        choice->type = CT_CIVILIAN;
        choice->value.utype = founder_type; /* default */
        choice->need_boat = TRUE;

        /* Then try to overwrite it with ferryboat information
         * If no ferryboat is found, above founder choice stays. */
        dai_choose_role_unit(ait, pplayer, pcity, choice, CT_CIVILIAN,
                             L_FERRYBOAT, -founder_want, TRUE);
        adv_choice_set_use(choice, "founder's boat");
      }
    } else if (!founder_type
               && (founder_want > choice->want || founder_want < -choice->want)) {
      /* Can't build founders. Lets stimulate science */
      dai_wants_role_unit(ait, pplayer, pcity,
                          action_id_get_role(ACTION_FOUND_CITY),
                          founder_want);
    }
  }

  {
    struct adv_choice *cur;

    /* Consider building caravan-type units to aid wonder construction */
    cur = adv_new_choice();
    adv_choice_set_use(cur, "wonder");
    dai_choose_help_wonder(ait, pcity, cur, adv);
    choice = adv_better_choice_free(choice, cur);

    /* Consider city improvements */
    cur = adv_new_choice();
    adv_choice_set_use(cur, "improvement");
    building_advisor_choose(pcity, cur);
    choice = adv_better_choice_free(choice, cur);

    /* Consider building caravan-type units for trade route */
    cur = adv_new_choice();
    adv_choice_set_use(cur, "trade route");
    dai_choose_trade_route(ait, pcity, cur, adv);
    choice = adv_better_choice_free(choice, cur);
  }

  if (choice->want >= 200) {
    /* If we don't do following, we buy caravans in city X when we should be
     * saving money to buy defenses for city Y. -- Syela */
    choice->want = 199;
  }

  return choice;
}

/***********************************************************************//**
  Calculate walking distances to wonder city from nearby cities.
***************************************************************************/
void dai_wonder_city_distance(struct ai_type *ait, struct player *pplayer, 
                              struct adv_data *adv)
{
  struct pf_map *pfm;
  struct pf_parameter parameter;
  struct unit_type *punittype;
  struct unit *ghost;
  int maxrange;
  struct city *wonder_city = game_city_by_number(adv->wonder_city);

  city_list_iterate(pplayer->cities, acity) {
    /* Mark unavailable */
    def_ai_city_data(acity, ait)->distance_to_wonder_city = 0;
  } city_list_iterate_end;

  if (wonder_city == NULL) {
    return;
  }

  punittype = best_role_unit_for_player(pplayer,
      action_id_get_role(ACTION_HELP_WONDER));

  if (!punittype) {
    return;
  }

  fc_assert_msg(utype_can_do_action(punittype, ACTION_HELP_WONDER),
                "Non existence of wonder helper unit not caught");

  ghost = unit_virtual_create(pplayer, wonder_city, punittype, 0);
  maxrange = unit_move_rate(ghost) * 7;

  pft_fill_unit_parameter(&parameter, ghost);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  pfm = pf_map_new(&parameter);

  pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
    struct city *acity = tile_city(ptile);

    if (move_cost > maxrange) {
      break;
    }
    if (!acity) {
      continue;
    }
    if (city_owner(acity) == pplayer) {
      def_ai_city_data(acity, ait)->distance_to_wonder_city = move_cost;
    }
  } pf_map_move_costs_iterate_end;

  pf_map_destroy(pfm);
  unit_virtual_destroy(ghost);
}
