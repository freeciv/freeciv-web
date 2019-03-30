/***********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
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
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "player.h"
#include "spaceship.h"
#include "tech.h"
#include "unitlist.h"
#include "victory.h"

#include "aisupport.h"

/*******************************************************************//**
  Find who is leading the space race. Returns NULL if nobody is.
***********************************************************************/
struct player *player_leading_spacerace(void)
{
  struct player *best = NULL;
  int best_arrival = FC_INFINITY;
  enum spaceship_state best_state = SSHIP_NONE;

  if (!victory_enabled(VC_SPACERACE)) {
    return NULL;
  }

  players_iterate_alive(pplayer) {
    struct player_spaceship *ship = &pplayer->spaceship;
    int arrival = (int) ship->travel_time + ship->launch_year;

    if (is_barbarian(pplayer) || ship->state == SSHIP_NONE) {
      continue;
    }

    if (ship->state != SSHIP_LAUNCHED
        && ship->state > best_state) {
      best_state = ship->state;
      best = pplayer;
    } else if (ship->state == SSHIP_LAUNCHED
               && arrival < best_arrival) {
      best_state = ship->state;
      best_arrival = arrival;
      best = pplayer;
    }
  } players_iterate_alive_end;

  return best;
}

/*******************************************************************//**
  Calculate average distances to other players. We calculate the 
  average distance from all of our cities to the closest enemy city.
***********************************************************************/
int player_distance_to_player(struct player *pplayer, struct player *target)
{
  int cities = 0;
  int dists = 0;

  if (pplayer == target
      || !target->is_alive
      || !pplayer->is_alive
      || city_list_size(pplayer->cities) == 0
      || city_list_size(target->cities) == 0) {
    return 1;
  }

  /* For all our cities, find the closest distance to an enemy city. */
  city_list_iterate(pplayer->cities, pcity) {
    int min_dist = FC_INFINITY;

    city_list_iterate(target->cities, c2) {
      int dist = real_map_distance(c2->tile, pcity->tile);

      if (min_dist > dist) {
        min_dist = dist;
      }
    } city_list_iterate_end;
    dists += min_dist;
    cities++;
  } city_list_iterate_end;

  return MAX(dists / cities, 1);
}

/*******************************************************************//**
  Rough calculation of the worth of pcity in gold.
***********************************************************************/
int city_gold_worth(struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  int worth = 0, i;
  struct unit_type *u = NULL;

  if (!game.scenario.prevent_new_cities) {
    u = best_role_unit_for_player(city_owner(pcity),
                                  action_id_get_role(ACTION_FOUND_CITY));
  }

  if (u != NULL) {
    worth += utype_buy_gold_cost(pcity, u, 0); /* cost of settler */
  }
  for (i = 1; i < city_size_get(pcity); i++) {
    worth += city_granary_size(i); /* cost of growing city */
  }
  output_type_iterate(o) {
    worth += pcity->prod[o] * 10;
  } output_type_iterate_end;
  unit_list_iterate(pcity->units_supported, punit) {
    if (same_pos(unit_tile(punit), pcity->tile)) {
      struct unit_type *punittype = unit_type_get(punit)->obsoleted_by;

      if (punittype && can_city_build_unit_direct(pcity, punittype)) {
        worth += unit_disband_shields(punit); /* obsolete, candidate for disbanding */
      } else {
        worth += unit_build_shield_cost(pcity, punit); /* good stuff */
      }
    }
  } unit_list_iterate_end;
  city_built_iterate(pcity, pimprove) {
    if (improvement_obsolete(pplayer, pimprove, pcity)) {
      worth += impr_sell_gold(pimprove); /* obsolete, candidate for selling */
    } else if (!is_wonder(pimprove)) {
      /* Buy cost, with nonzero shield amount */
      worth += impr_build_shield_cost(pcity, pimprove) * 2;
    } else {
      worth += impr_build_shield_cost(pcity, pimprove) * 4;
    }
  } city_built_iterate_end;
  if (city_unhappy(pcity)) {
    worth *= 0.75;
  }
  return worth;
}
