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
#include "log.h"

/* common */
#include "game.h"
#include "traderoutes.h"

/* aicore */
#include "path_finding.h"
#include "pf_tools.h"

#include "caravan.h"

/************************************************************************//**
  Create a valid parameter with default values.
****************************************************************************/
void caravan_parameter_init_default(struct caravan_parameter *parameter)
{
  parameter->horizon = FC_INFINITY;
  parameter->discount = 0.95;
  parameter->consider_windfall = TRUE;
  parameter->consider_trade = TRUE;
  parameter->consider_wonders = TRUE; /* see also init_from_unit */
  parameter->account_for_broken_routes = TRUE;
  parameter->allow_foreign_trade = FTL_NATIONAL_ONLY;
  parameter->ignore_transit_time = FALSE;
  parameter->convert_trade = FALSE;
  parameter->callback = NULL;
}

/************************************************************************//**
  Create a valid parameter with default values based on the caravan.
****************************************************************************/
void caravan_parameter_init_from_unit(struct caravan_parameter *parameter,
                                      const struct unit *caravan)
{
  caravan_parameter_init_default(parameter);
  if (!unit_can_do_action(caravan, ACTION_TRADE_ROUTE)) {
    parameter->consider_trade = FALSE;
  }
  if (!unit_can_do_action(caravan, ACTION_MARKETPLACE)
      && !unit_can_do_action(caravan, ACTION_TRADE_ROUTE)) {
    parameter->consider_windfall = FALSE;
  }
  if (!unit_can_do_action(caravan, ACTION_HELP_WONDER)) {
    parameter->consider_wonders = FALSE;
  }
}

/************************************************************************//**
  Check for legality.
****************************************************************************/
bool caravan_parameter_is_legal(const struct caravan_parameter *parameter)
{
  /* a discount > 1.0 means money later is worth more than money now,
     which is ridiculous. */
  if (parameter->discount > 1.0) {
    return FALSE;
  }

  /* a negative discount doesn't converge */
  if (parameter->discount < 0.0) {
    return FALSE;
  }

  /* infinite horizon with no discount gives infinite reward. */
  if (parameter->horizon == FC_INFINITY && parameter->discount == 1.0) {
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  For debugging, print out the parameter.
****************************************************************************/
void caravan_parameter_log_real(const struct caravan_parameter *parameter,
                                enum log_level level, const char *file,
                                const char *function, int line)
{
  const char *foreign = "<illegal>";

  switch (parameter->allow_foreign_trade) {
  case FTL_NATIONAL_ONLY:
    foreign = "no";
    break;
  case FTL_ALLIED:
    foreign = "allied";
    break;
  case FTL_PEACEFUL:
    foreign = "peaceful";
    break;
  case FTL_NONWAR:
    foreign = "anything but enemies";
    break;
  }

  do_log(file, function, line, FALSE, level,
         "parameter {\n"
         "  horizon   = %d\n"
         "  discount  = %g\n"
         "  objective = <%s,%s,%s>\n"
         "  account-broken = %s\n"
         "  allow-foreign  = %s\n"
         "  ignore-transit = %s\n"
         "  convert-trade  = %s\n"
         "}\n",
         parameter->horizon,
         parameter->discount,
         parameter->consider_windfall ? "windfall" : "-",
         parameter->consider_trade ? "trade" : "-",
         parameter->consider_wonders ? "wonders" : "-",
         parameter->account_for_broken_routes ? "yes" : "no",
         foreign,
         parameter->ignore_transit_time ? "yes" : "no",
         parameter->convert_trade ? "yes" : "no");
}


/************************************************************************//**
  Initialize the result to be worth zero and go from nowhere to nowhere.
****************************************************************************/
void caravan_result_init_zero(struct caravan_result *result)
{
  result->src = result->dest = NULL;
  result->arrival_time = 0;
  result->value = 0;
  result->help_wonder = FALSE;
  result->required_boat = FALSE;
}

/************************************************************************//**
  Initialize the result to go from src to dest with the given amount
  of time.  This is useful for calling get_discounted_reward and the such.
****************************************************************************/
static void caravan_result_init(struct caravan_result *result,
                                const struct city *src,
                                const struct city *dest,
                                int arrival_time)
{
  result->src = src;
  result->dest = dest;
  result->arrival_time = arrival_time;

  result->value = 0;
  result->help_wonder = FALSE;
  if ((src != NULL) && (dest != NULL)) {
    if (tile_continent(src->tile) != tile_continent(dest->tile)) {
      result->required_boat = TRUE;
    } else {
      result->required_boat = FALSE;
    }
  } else {
    result->required_boat = FALSE;
  }
}

/************************************************************************//**
  Compare the two results for sorting.
****************************************************************************/
int caravan_result_compare(const struct caravan_result *a,
                           const struct caravan_result *b)
{
  if (a->value > b->value) {
    return 1;
  } else if (a->value < b->value) {
    return -1;
  } else {
    /* faster time is better, so reverse-sort on time. */
    return b->arrival_time - a->arrival_time ;
  }
}

/************************************************************************//**
  We use the path finding in several places.
  This provides a single implementation of that.  It is critical that
  this function be re-entrant since we call it recursively.

  The callback should return TRUE if it wants to stop searching,
  FALSE otherwise.
****************************************************************************/
typedef bool (*search_callback) (void *data, const struct city *pcity,
                                 int arrival_turn, int arrival_moves_left);

static void caravan_search_from(const struct unit *caravan,
                                const struct caravan_parameter *param,
                                struct tile *start_tile,
                                int turns_before, int moves_left_before,
                                bool omniscient,
                                search_callback callback,
                                void *callback_data) {
  struct pf_map *pfm;
  struct pf_parameter pfparam;
  int end_time;

  end_time = param->horizon - turns_before;

  /* Initialize the pf run. */
  pft_fill_unit_parameter(&pfparam, (struct unit *) caravan);
  pfparam.start_tile = start_tile;
  pfparam.moves_left_initially = moves_left_before;
  pfparam.omniscience = omniscient;
  pfm = pf_map_new(&pfparam);

  /* For every tile in distance order:
     quit if we've exceeded the maximum number of turns
     otherwise, run the callback if we're on a city.
     Do-while loop rather than while loop to make sure to process the
     start tile.
   */
  pf_map_positions_iterate(pfm, pos, TRUE) {
    struct city *pcity;

    if (pos.turn > end_time) {
      break;
    }

    pcity = tile_city(pos.tile);
    if (pcity && callback(callback_data, pcity, turns_before + pos.turn,
                          pos.moves_left)) {
      break;
    }
  } pf_map_positions_iterate_end;

  pf_map_destroy(pfm);
}

/************************************************************************//**
  When the caravan arrives, compute the benefit from the immediate windfall,
  taking into account the parameter's objective.
****************************************************************************/
static double windfall_benefit(const struct unit *caravan,
                               const struct city *src,
                               const struct city *dest,
                               const struct caravan_parameter *param)
{
  if (!param->consider_windfall || !can_cities_trade(src, dest)) {
    return 0;
  } else {
    bool can_establish = (unit_can_do_action(caravan, ACTION_TRADE_ROUTE)
                          && can_establish_trade_route(src, dest));
    int bonus = get_caravan_enter_city_trade_bonus(src, dest, NULL,
                                                   can_establish);

    /* bonus goes to both sci and gold. */
    bonus *= 2;

    return bonus;
  }
}

/****************************************************************************
  Compute the change in the per-turn trade.
****************************************************************************/

/************************************************************************//**
  How much does the city benefit from the new trade route?
  How much does the former partner lose?
****************************************************************************/
static int one_city_trade_benefit(const struct city *pcity,
                                  const struct player *pplayer,
                                  bool countloser, int newtrade)
{
  int losttrade = 0;

  /* if the city is owned by someone else, we don't benefit from the
     new trade (but we might still lose from a broken trade route) */
  if (city_owner(pcity) != pplayer) {
    newtrade = 0;
  }

  if (city_num_trade_routes(pcity) < max_trade_routes(pcity)) {
    /* if the city can handle this route, we don't break any old routes */
    losttrade = 0;
  } else {
    struct trade_route_list *would_remove = (countloser ? trade_route_list_new() : NULL);
    int oldtrade = city_trade_removable(pcity, would_remove);

    /* if we own the city, the trade benefit is only by how much
       better we are than the old trade route */
    if (city_owner(pcity) == pplayer) {
      newtrade -= oldtrade;
    }

    /* if the cities that lost a trade route is one of ours, and if we
       care about accounting for the lost trade, count it. */
    if (countloser) {
      trade_route_list_iterate(would_remove, plost) {
        struct city *losercity = game_city_by_number(plost->partner);

        if (city_owner(losercity) == pplayer) {
          trade_routes_iterate(losercity, pback) {
            if (pback->partner == pcity->id) {
              losttrade += pback->value;
            }
          } trade_routes_iterate_end;
        }
      } trade_route_list_iterate_end;
      trade_route_list_destroy(would_remove);
    }
  }

  /* find the increase or decrease in trade we benefit from */
  return newtrade - losttrade;
}

/************************************************************************//**
  Compute one_trade_benefit for both cities and do some other logic.
  This yields the total benefit in terms of trade per turn of establishing
  a route from src to dest.
****************************************************************************/
static double trade_benefit(const struct player *caravan_owner,
                            const struct city *src,
                            const struct city *dest,
                            const struct caravan_parameter *param)
{
  /* do we care about trade at all? */
  if (!param->consider_trade) {
    return 0;
  }

  /* first, see if a new route is made. */
  if (!can_cities_trade(src, dest) || !can_establish_trade_route(src, dest)) {
    return 0;
  }
  if (max_trade_routes(src) <= 0 || max_trade_routes(dest) <= 0) {
    /* Can't create new traderoutes even by replacing old ones if
     * there's no slots at all. */
    return 0;
  }

  if (!param->convert_trade) {
    bool countloser = param->account_for_broken_routes;
    int newtrade = trade_base_between_cities(src, dest);

    return one_city_trade_benefit(src, caravan_owner, countloser, newtrade)
         + one_city_trade_benefit(dest, caravan_owner, countloser, newtrade);
  } else {
    /* Always fails. */
    fc_assert_msg(FALSE == param->convert_trade,
                  "Unimplemented functionality: "
                  "using CM to calculate trade.");
    return 0;
  }
}

/************************************************************************//**
  Check the benefit of helping build the wonder in dest.
  This is based on how much the caravan would help if it arrived
  after turns_delay turns during which the city managed the same
  production it currently gets (i.e. no other caravans, no
  population growth or terrain improvement, ...)
****************************************************************************/
static double wonder_benefit(const struct unit *caravan, int arrival_time,
                             const struct city *dest,
                             const struct caravan_parameter *param)
{
  int costwithout, costwith;
  int shields_at_arrival;

  if (!param->consider_wonders
      /* TODO: Should helping an ally to build be considered when legal? */
      || unit_owner(caravan) != city_owner(dest)
      || !city_production_gets_caravan_shields(&dest->production)
      /* TODO: Should helping to build a unit be considered when legal? */
      || VUT_UTYPE == dest->production.kind
      /* TODO: Should helping to build an improvement be considered when
       * legal? */
      || !is_wonder(dest->production.value.building)) {
    return 0;
  }

  shields_at_arrival = dest->shield_stock
    + arrival_time * dest->surplus[O_SHIELD];

  costwithout = impr_buy_gold_cost(dest, dest->production.value.building,
      shields_at_arrival);
  costwith = impr_buy_gold_cost(dest, dest->production.value.building,
      shields_at_arrival + unit_build_shield_cost_base(caravan));

  fc_assert_ret_val(costwithout >= costwith, -1.0);
  return costwithout - costwith;
}

/************************************************************************//**
  Discount a value by the given discount rate.
  The payment occurs as a lump sum in 'term' turns.
****************************************************************************/
static double presentvalue(double payment, int term, double rate)
{
  return payment * pow(rate, term);
}

/************************************************************************//**
  Compute the net present value of an perpetuity given the discount rate.
  A perpetuity is an annual payment for an infinite number of turns.
****************************************************************************/
static double perpetuity(double payment, double rate)
{
  return payment / (1.0 - rate);
}

/************************************************************************//**
  Compute the net present value of an annuity given the discount rate.
  An annuity is an annual payment for a fixed term (number of turns).
****************************************************************************/
static double annuity(double payment, int term, double rate)
{
  return perpetuity(payment, rate) * (1.0 - 1.0 / pow(rate, term));
}

/************************************************************************//**
  Are the two players allowed to trade by the parameter settings?
****************************************************************************/
static bool does_foreign_trade_param_allow(const struct caravan_parameter *param,
                                           struct player *src, struct player *dest)
{
  switch (param->allow_foreign_trade) {
    case FTL_NATIONAL_ONLY:
      return (src == dest);
      break;
    case FTL_ALLIED:
      return pplayers_allied(src, dest);
      break;
    case FTL_PEACEFUL:
      return pplayers_in_peace(src, dest);
      break;
    case FTL_NONWAR:
      return !pplayers_at_war(src, dest);
  }

  fc_assert(FALSE);
  return FALSE;
}

/************************************************************************//**
  Compute the discounted reward from the trade route that is indicated
  by the src, dest, and arrival_time fields of the result:  Fills in
  the value and help_wonder fields.
  Assumes the owner of src is the owner of the caravan.
****************************************************************************/
static bool get_discounted_reward(const struct unit *caravan,
                                  const struct caravan_parameter *parameter,
                                  struct caravan_result *result)
{
  double trade;
  double windfall;
  double wonder;
  const struct city *src = result->src;
  const struct city *dest = result->dest;
  int arrival_time = result->arrival_time;
  double discount = parameter->discount;
  struct player *pplayer_src = city_owner(src);
  struct player *pplayer_dest = city_owner(dest);
  bool consider_wonder;
  bool consider_trade;
  bool consider_windfall;

  /* if no foreign trade is allowed, just quit. */
  if (!does_foreign_trade_param_allow(parameter, pplayer_src, pplayer_dest)) {
    caravan_result_init_zero(result);
    return FALSE;
  }

  consider_wonder = parameter->consider_wonders
    && action_prob_possible(
        action_speculate_unit_on_city(ACTION_HELP_WONDER,
                                      caravan, src, city_tile(dest),
                                      TRUE, dest));
  consider_trade = parameter->consider_trade
    && action_prob_possible(
        action_speculate_unit_on_city(ACTION_TRADE_ROUTE,
                                      caravan, src, city_tile(dest),
                                      TRUE, dest));
  consider_windfall = parameter->consider_windfall
    && action_prob_possible(
        action_speculate_unit_on_city(ACTION_MARKETPLACE,
                                      caravan, src, city_tile(dest),
                                      TRUE, dest));

  if (!consider_wonder && !consider_trade && !consider_windfall) {
    /* No caravan action is possible against this target. */
    caravan_result_init_zero(result);

    return FALSE;
  }

  trade = trade_benefit(pplayer_src, src, dest, parameter);
  windfall = windfall_benefit(caravan, src, dest, parameter);
  if (consider_wonder) {
    wonder = wonder_benefit(caravan, arrival_time, dest, parameter);
    /* we want to aid for wonder building */
    wonder *= 2;

    wonder = presentvalue(wonder, arrival_time, discount);
  } else {
    wonder = -1.0;
  }

  if (consider_trade) {
    if (parameter->horizon == FC_INFINITY) {
      trade = perpetuity(trade, discount);
    } else {
      trade = annuity(trade, parameter->horizon - arrival_time, discount);
    }
    trade = presentvalue(trade, arrival_time, discount);
  } else {
    trade = 0.0;
  }

  if (consider_windfall) {
    windfall = presentvalue(windfall, arrival_time, discount);
  } else {
    windfall = 0.0;
  }

  if (consider_trade && trade + windfall >= wonder) {
    result->value = trade + windfall;
    result->help_wonder = FALSE;
  } else if (consider_wonder) {
    result->value = wonder;
    result->help_wonder = TRUE;
  } else {
    caravan_result_init_zero(result);
    return FALSE;
  }

  if (parameter->callback) {
    parameter->callback(result, parameter->callback_data);
  }

  return TRUE;
}

/****************************************************************************
  Functions to compute the benefit of moving the caravan to dest.
****************************************************************************/

/************************************************************************//**
  Ignoring the transit time, return the value of moving the caravan to
  dest.
****************************************************************************/
static void caravan_evaluate_notransit(const struct unit *caravan,
                                       const struct city *dest,
                                       const struct caravan_parameter *param,
                                       struct caravan_result *result)
{
  const struct city *src = game_city_by_number(caravan->homecity);

  caravan_result_init(result, src, dest, 0);
  get_discounted_reward(caravan, param, result);
}

/************************************************************************//**
  Structure and callback for the caravan_search invocation in
  caravan_evaluate_withtransit.
****************************************************************************/
struct cewt_data {
  const struct unit *caravan;
  struct caravan_result *result;
  const struct caravan_parameter *param;
};

static bool cewt_callback(void *vdata, const struct city *dest,
                          int arrival_time, int moves_left)
{
  struct cewt_data *data = vdata;

  fc_assert_ret_val(data->result, FALSE);

  if (dest == data->result->dest) {
    data->result->arrival_time = arrival_time;
    get_discounted_reward(data->caravan, data->param, data->result);
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  Using the caravan_search function to take transit time into account,
  evaluate the benefit of sending the caravan to dest.
****************************************************************************/
static void caravan_evaluate_withtransit(const struct unit *caravan,
                                         const struct city *dest,
                                         const struct caravan_parameter *param,
                                         struct caravan_result *result,
                                         bool omniscient)
{
  struct cewt_data data;

  data.caravan = caravan;
  data.param = param;
  caravan_result_init(result, game_city_by_number(caravan->homecity),
                      dest, 0);
  caravan_search_from(caravan, param, unit_tile(caravan), 0,
                      caravan->moves_left, omniscient, cewt_callback, &data);
}

/************************************************************************//**
  Evaluate the value of sending the caravan to dest.
****************************************************************************/
void caravan_evaluate(const struct unit *caravan,
                      const struct city *dest,
                      const struct caravan_parameter *param,
                      struct caravan_result *result, bool omniscient)
{
  if (param->ignore_transit_time) {
    caravan_evaluate_notransit(caravan, dest, param, result);
  } else {
    caravan_evaluate_withtransit(caravan, dest, param, result, omniscient);
  }
}


/************************************************************************//**
  Find the best destination for the caravan, ignoring transit time.
****************************************************************************/
static void caravan_find_best_destination_notransit(const struct unit *caravan,
                                         const struct caravan_parameter *param,
                                                    struct caravan_result *best)
{
  struct caravan_result current;
  struct city *pcity = game_city_by_number(caravan->homecity);
  struct player *src_owner = city_owner(pcity);

  caravan_result_init(best, pcity, NULL, 0);
  current = *best;

  players_iterate(dest_owner) {
    if (does_foreign_trade_param_allow(param, src_owner, dest_owner)) {
      city_list_iterate(dest_owner->cities, dest) {
        caravan_result_init(&current, pcity, dest, 0);
        get_discounted_reward(caravan, param, &current);

        if (caravan_result_compare(&current, best) > 0) {
          *best = current;
        }
      } city_list_iterate_end;
    }
  } players_iterate_end;
}

/************************************************************************//**
  Callback and struct for caravan_search invocation in
  caravan_find_best_destination_withtransit.
****************************************************************************/
struct cfbdw_data {
  const struct caravan_parameter *param;
  const struct unit *caravan;
  struct caravan_result *best;
};

static bool cfbdw_callback(void *vdata, const struct city *dest,
                           int arrival_time, int moves_left)
{
  struct cfbdw_data *data = vdata;
  struct caravan_result current;

  caravan_result_init(&current, data->best->src, dest, arrival_time);

  get_discounted_reward(data->caravan, data->param, &current);
  if (caravan_result_compare(&current, data->best) > 0) {
    *data->best = current;
  }

  return FALSE; /* don't stop. */
}

/************************************************************************//**
  Using caravan_search, find the best destination.
****************************************************************************/
static void caravan_find_best_destination_withtransit(
    const struct unit *caravan,
    const struct caravan_parameter *param,
    const struct city *src,
    int turns_before,
    int moves_left,
    bool omniscient,
    struct caravan_result *result) 
{
  struct tile *start_tile;
  struct cfbdw_data data;

  data.param = param;
  data.caravan = caravan;
  data.best = result;
  caravan_result_init(data.best, src, NULL, 0);

  if (src->id != caravan->homecity) {
    start_tile = src->tile;
  } else {
    start_tile = unit_tile(caravan);
  }

  caravan_search_from(caravan, param, start_tile, turns_before,
                      caravan->moves_left, omniscient, cfbdw_callback, &data);
}

/************************************************************************//**
  Find the best destination city for the caravan.
  Store it in *destout (if destout is non-null); return the value of the
  trade route.
****************************************************************************/
void caravan_find_best_destination(const struct unit *caravan,
                                   const struct caravan_parameter *parameter,
                                   struct caravan_result *result, bool omniscient)
{
  if (parameter->ignore_transit_time) {
    caravan_find_best_destination_notransit(caravan, parameter, result);
  } else {
    const struct city *src = game_city_by_number(caravan->homecity);

    fc_assert(src != NULL);

    caravan_find_best_destination_withtransit(caravan, parameter, src, 0,
                                              caravan->moves_left, omniscient, result);
  }
}

/************************************************************************//**
  Find the best pair-wise trade route, ignoring transit time.
****************************************************************************/
static void caravan_optimize_notransit(const struct unit *caravan,
                                       const struct caravan_parameter *param,
                                       struct caravan_result *best)
{
  struct player *pplayer = unit_owner(caravan);

  /* Iterate over all cities we own (since the caravan could change its
   * home city); iterate over all cities we know about (places the caravan
   * can go to); pick out the best trade route. */
  city_list_iterate(pplayer->cities, src) {
    players_iterate(dest_owner) {
      if (does_foreign_trade_param_allow(param, pplayer, dest_owner)) {
        city_list_iterate(dest_owner->cities, dest) {
          struct caravan_result current;

          caravan_result_init(&current, src, dest, 0);
          get_discounted_reward(caravan, param, &current);
          if (caravan_result_compare(&current, best) > 0) {
            *best = current;
          }
        } city_list_iterate_end;
      }
    } players_iterate_end;
  } city_list_iterate_end;
}

/****************************************************************************
  Struct for the caravan_search invocation in
  caravan_optimize_withtransit.
****************************************************************************/
struct cowt_data {
  const struct caravan_parameter *param;
  const struct unit *caravan;
  struct caravan_result *best;
  bool omniscient;
};

/************************************************************************//**
  Callback for the caravan_search invocation in
  caravan_optimize_withtransit.

  For every city we can reach, use caravan_find_best_destination as a
  subroutine.
****************************************************************************/
static bool cowt_callback(void *vdata, const struct city *pcity,
                          int arrival_time, int moves_left)
{
  struct cowt_data *data = vdata;
  const struct unit *caravan = data->caravan;
  struct caravan_result current;

  caravan_result_init(&current, game_city_by_number(caravan->homecity),
                      pcity, arrival_time);

  /* first, see what benefit we'd get from not changing home city */
  get_discounted_reward(caravan, data->param, &current);
  if (caravan_result_compare(&current, data->best) > 0) {
    *data->best = current;
  }

  /* next, try changing home city (if we're allowed to) */
  if (city_owner(pcity) == unit_owner(caravan)) {
    caravan_find_best_destination_withtransit(
                caravan, data->param, pcity, arrival_time, moves_left, data->omniscient,
                &current);
    if (caravan_result_compare(&current, data->best) > 0) {
      *data->best = current;
    }
  }

  return FALSE; /* don't stop searching */
}

/************************************************************************//**
  Find the best src/dest pair (including possibly changing home city), taking
  account of the trip time.
****************************************************************************/
static void caravan_optimize_withtransit(const struct unit *caravan,
                                         const struct caravan_parameter *param,
                                         struct caravan_result *result,
                                         bool omniscient)
{
  struct cowt_data data;

  data.param = param;
  data.caravan = caravan;
  data.best = result;
  data.omniscient = omniscient;
  caravan_result_init_zero(data.best);
  caravan_search_from(caravan, param, unit_tile(caravan), 0,
                      caravan->moves_left, omniscient, cowt_callback, &data);
}

/************************************************************************//**
  For every city the caravan can change home in, find the best destination.
  Return the best src/dest pair by reference (if non-null), and return
  the value of that trade route.
****************************************************************************/
void caravan_optimize_allpairs(const struct unit *caravan,
                               const struct caravan_parameter *param,
                               struct caravan_result *result, bool omniscient)
{
  if (param->ignore_transit_time) {
    caravan_optimize_notransit(caravan, param, result);
  } else {
    caravan_optimize_withtransit(caravan, param, result, omniscient);
  }
}
