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
#include "rand.h"

/* common */
#include "ai.h"
#include "city.h"
#include "effects.h"
#include "game.h"
#include "movement.h"
#include "player.h"
#include "specialist.h"

/* common/aicore */
#include "path_finding.h"
#include "pf_tools.h"

/* server */
#include "citytools.h"
#include "plrhand.h"
#include "srv_log.h"

/* server/advisors */
#include "advdata.h"
#include "advtools.h"
#include "infracache.h" /* adv_city */

/* ai */
#include "handicaps.h"

#include "advbuilding.h"

/**********************************************************************//**
  Calculate walking distance to nearest friendly cities from every city.

  The hidden assumption here is that a ACTION_HELP_WONDER unit is like any
  other unit that will use this data.

  pcity->server.adv->downtown is set to the number of cities within 4 turns of
  the best help wonder unit we can currently produce.
**************************************************************************/
static void calculate_city_clusters(struct player *pplayer)
{
  struct unit_type *punittype;
  struct unit *ghost;
  int range;

  city_list_iterate(pplayer->cities, pcity) {
    pcity->server.adv->downtown = 0;
  } city_list_iterate_end;

  if (num_role_units(action_id_get_role(ACTION_HELP_WONDER)) == 0) {
    return; /* ruleset has no help wonder unit */
  }

  punittype = best_role_unit_for_player(pplayer,
      action_id_get_role(ACTION_HELP_WONDER));

  if (!punittype) {
    /* simulate future unit */
    punittype = get_role_unit(action_id_get_role(ACTION_HELP_WONDER), 0);
  }

  fc_assert_msg(utype_can_do_action(punittype, ACTION_HELP_WONDER),
                "Non existence of wonder helper unit not caught");

  ghost = unit_virtual_create(pplayer, NULL, punittype, 0);
  range = unit_move_rate(ghost) * 4;

  city_list_iterate(pplayer->cities, pcity) {
    struct pf_parameter parameter;
    struct pf_map *pfm;
    struct adv_city *city_data = pcity->server.adv;

    unit_tile_set(ghost, pcity->tile);
    pft_fill_unit_parameter(&parameter, ghost);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    pfm = pf_map_new(&parameter);

    pf_map_move_costs_iterate(pfm, ptile, move_cost, FALSE) {
      struct city *acity = tile_city(ptile);

      if (move_cost > range) {
        break;
      }
      if (!acity) {
        continue;
      }
      if (city_owner(acity) == pplayer) {
        city_data->downtown++;
      }
    } pf_map_move_costs_iterate_end;

    pf_map_destroy(pfm);
  } city_list_iterate_end;

  unit_virtual_destroy(ghost);
}

/**********************************************************************//**
  Set building wants for human player 
**************************************************************************/
static void ba_human_wants(struct player *pplayer, struct city *wonder_city)
{
  /* Clear old building wants.
   * Do this separately from the iteration over improvement types
   * because each iteration could actually update more than one improvement,
   * if improvements have improvements as requirements.
   */
  city_list_iterate(pplayer->cities, pcity) {
    /* For a human player, any building is worth building until discarded */
    improvement_iterate(pimprove) {
      pcity->server.adv->building_want[improvement_index(pimprove)] = 1;
    } improvement_iterate_end;
  } city_list_iterate_end;

  improvement_iterate(pimprove) {
    const bool is_coinage = improvement_has_flag(pimprove, IF_GOLD);

    /* Handle coinage specially because you can never complete coinage */
    if (is_coinage
     || can_player_build_improvement_later(pplayer, pimprove)) {
      city_list_iterate(pplayer->cities, pcity) {
        if (pcity != wonder_city && is_wonder(pimprove)) {
          /* Only wonder city should build wonders! */
          pcity->server.adv->building_want[improvement_index(pimprove)] = 0;
        } else if (!is_coinage
                    && (!can_city_build_improvement_later(pcity, pimprove)
                        || (!is_improvement_productive(pcity, pimprove)))) {
          /* Don't consider impossible or unproductive buildings */
          pcity->server.adv->building_want[improvement_index(pimprove)] = 0;
        } else if (city_has_building(pcity, pimprove)) {
          /* Never want to build something we already have. */
          pcity->server.adv->building_want[improvement_index(pimprove)] = 0;
        }
        /* else wait until a later turn */
      } city_list_iterate_end;
    } else {
      /* An impossible improvement */
      city_list_iterate(pplayer->cities, pcity) {
        pcity->server.adv->building_want[improvement_index(pimprove)] = 0;
      } city_list_iterate_end;
    }
  } improvement_iterate_end;

#ifdef FREECIV_DEBUG
  /* This logging is relatively expensive, so activate only if necessary */
  city_list_iterate(pplayer->cities, pcity) {
    improvement_iterate(pimprove) {
      if (pcity->server.adv->building_want[improvement_index(pimprove)] != 0) {
        CITY_LOG(LOG_DEBUG, pcity, "want to build %s with " ADV_WANT_PRINTF, 
                 improvement_rule_name(pimprove),
                 pcity->server.adv->building_want[improvement_index(pimprove)]);
      }
    } improvement_iterate_end;
  } city_list_iterate_end;
#endif /* FREECIV_DEBUG */
}

/**********************************************************************//**
  Prime pcity->server.adv.building_want[]
**************************************************************************/
void building_advisor(struct player *pplayer)
{
  struct adv_data *adv = adv_data_get(pplayer, NULL);
  struct city *wonder_city = game_city_by_number(adv->wonder_city);

  CALL_FUNC_EACH_AI(build_adv_init, pplayer);

  if (wonder_city && city_owner(wonder_city) != pplayer) {
    /* We lost it to the enemy! */
    adv->wonder_city = 0;
    wonder_city = NULL;
  }

  /* Preliminary analysis - find our Wonder City. Also check if it
   * is sane to continue building the wonder in it. If either does
   * not check out, make a Wonder City. */
  if (NULL == wonder_city
   || 0 >= wonder_city->surplus[O_SHIELD]
   || VUT_UTYPE == wonder_city->production.kind /* changed to defender? */
   || !is_wonder(wonder_city->production.value.building)
   || !can_city_build_improvement_now(wonder_city, 
                                      wonder_city->production.value.building)
   || !is_improvement_productive(wonder_city,
                                 wonder_city->production.value.building)) {
    /* Find a new wonder city! */
    int best_candidate_value = 0;
    struct city *best_candidate = NULL;
    /* Whether ruleset has a help wonder unit type */
    bool has_help =
        (num_role_units(action_id_get_role(ACTION_HELP_WONDER)) > 0);

    calculate_city_clusters(pplayer);

    city_list_iterate(pplayer->cities, pcity) {
      int value = pcity->surplus[O_SHIELD];
      Continent_id place = tile_continent(pcity->tile);
      struct adv_city *city_data = pcity->server.adv;

      if (is_ai(pplayer)) {
        bool result = TRUE;

        /* AI has opportunity to say that this city cannot be
         * wonder city */
        CALL_PLR_AI_FUNC(consider_wonder_city, pplayer, pcity, &result);
        if (!result) {
          continue;
        }
      }

      if (is_terrain_class_near_tile(pcity->tile, TC_OCEAN)) {
        value /= 2;
      }
      /* Downtown is the number of cities within a certain pf range.
       * These may be able to help with caravans. Also look at the whole
       * continent. */
      if (first_role_unit_for_player(pplayer,
              action_id_get_role(ACTION_HELP_WONDER))) {
        value += city_data->downtown;

        if (place >= 0) {
          value += adv->stats.cities[place] / 8;
        }
      }
      if (place >= 0 && adv->threats.continent[place] > 0) {
        /* We have threatening neighbours: -25% */
        value -= value / 4;
      }
      /* Require that there is at least some neighbors for wonder helpers,
       * if ruleset supports it. */
      if (value > best_candidate_value
          && (!has_help || (place >= 0 && adv->stats.cities[place] > 5))
          && (!has_help || city_data->downtown > 3)) {
        best_candidate = pcity;
        best_candidate_value = value;
      }
    } city_list_iterate_end;
    if (best_candidate) {
      CITY_LOG(LOG_DEBUG, best_candidate, "chosen as wonder-city!");
      adv->wonder_city = best_candidate->id;
      wonder_city = best_candidate;
    }
  }

  if (is_ai(pplayer)) {
    CALL_PLR_AI_FUNC(build_adv_prepare, pplayer, pplayer, adv);
    CALL_PLR_AI_FUNC(build_adv_adjust_want, pplayer, pplayer, wonder_city);
  } else {
    ba_human_wants(pplayer, wonder_city);
  }
}

/**********************************************************************//**
  Choose improvement we like most and put it into adv_choice.
**************************************************************************/
void building_advisor_choose(struct city *pcity, struct adv_choice *choice)
{
  struct player *plr = city_owner(pcity);
  struct impr_type *chosen = NULL;
  int want = 0;

  improvement_iterate(pimprove) {
    if (is_wonder(pimprove)) {
      continue; /* Humans should not be advised to build wonders or palace */
    }
    if (pcity->server.adv->building_want[improvement_index(pimprove)] > want
          && can_city_build_improvement_now(pcity, pimprove)) {
      want = pcity->server.adv->building_want[improvement_index(pimprove)];
      chosen = pimprove;
    }
  } improvement_iterate_end;

  choice->want = want;
  choice->value.building = chosen;

  if (chosen) {
    choice->type = CT_BUILDING;

    CITY_LOG(LOG_DEBUG, pcity, "wants most to build %s at %d",
             improvement_rule_name(chosen),
             want);
  } else {
    choice->type = CT_NONE;
  }
  choice->need_boat = FALSE;

  /* Allow ai to override */
  CALL_PLR_AI_FUNC(choose_building, plr, pcity, choice);
}

/**********************************************************************//**
  Setup improvement building
**************************************************************************/
void advisor_choose_build(struct player *pplayer, struct city *pcity)
{
  struct adv_choice choice;

  building_advisor_choose(pcity, &choice);

  if (valid_improvement(choice.value.building)) {
    struct universal target = {
      .kind = VUT_IMPROVEMENT,
      .value = {.building = choice.value.building}
    };

    change_build_target(pplayer, pcity, &target, E_IMP_AUTO);
    return;
  }

  /* Build the first thing we can think of (except moving small wonder). */
  improvement_iterate(pimprove) {
    if (can_city_build_improvement_now(pcity, pimprove)
        && pimprove->genus != IG_SMALL_WONDER) {
      struct universal target = {
        .kind = VUT_IMPROVEMENT,
        .value = {.building = pimprove}
      };

      change_build_target(pplayer, pcity, &target, E_IMP_AUTO);
      return;
    }
  } improvement_iterate_end;
}
