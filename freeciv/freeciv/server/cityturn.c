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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> /* exp, sqrt */

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common/aicore */
#include "cm.h"

/* common */
#include "achievements.h"
#include "actiontools.h"
#include "borders.h"
#include "calendar.h"
#include "citizens.h"
#include "city.h"
#include "culture.h"
#include "events.h"
#include "disaster.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "player.h"
#include "research.h"
#include "road.h"
#include "server_settings.h"
#include "specialist.h"
#include "tech.h"
#include "traderoutes.h"
#include "unit.h"
#include "unitlist.h"

/* common/scriptcore */
#include "luascript_types.h"

/* server */
#include "citizenshand.h"
#include "citytools.h"
#include "cityturn.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "sanitycheck.h"
#include "spacerace.h"
#include "srv_log.h"
#include "srv_main.h"
#include "techtools.h"
#include "unittools.h"
#include "unithand.h"

/* server/advisors */
#include "advbuilding.h"
#include "advdata.h"
#include "autosettlers.h"

/* server/scripting */
#include "script_server.h"

/* Queue for pending city_refresh() */
static struct city_list *city_refresh_queue = NULL;

/* The game is currently considering to remove the listed units because of
 * missing gold upkeep. A unit ends up here if it has gold upkeep that
 * can't be payed. A random unit in the list will be removed until the
 * problem is solved. */
static struct unit_list *uk_rem_gold = NULL;

static void check_pollution(struct city *pcity);
static void city_populate(struct city *pcity, struct player *nationality);

static bool worklist_change_build_target(struct player *pplayer,
					 struct city *pcity);

static bool city_distribute_surplus_shields(struct player *pplayer,
					    struct city *pcity);
static bool city_build_building(struct player *pplayer, struct city *pcity);
static bool city_build_unit(struct player *pplayer, struct city *pcity);
static bool city_build_stuff(struct player *pplayer, struct city *pcity);
static struct impr_type *building_upgrades_to(struct city *pcity,
					      struct impr_type *pimprove);
static void upgrade_building_prod(struct city *pcity);
static struct unit_type *unit_upgrades_to(struct city *pcity,
					  struct unit_type *id);
static void upgrade_unit_prod(struct city *pcity);

/* Helper struct for associating a building to a city. */
struct cityimpr {
  struct city *pcity;
  struct impr_type *pimprove;
};

#define SPECLIST_TAG cityimpr
#define SPECLIST_TYPE struct cityimpr
#include "speclist.h"

#define cityimpr_list_iterate(cityimprlist, pcityimpr) \
    TYPED_LIST_ITERATE(struct cityimpr, cityimprlist, pcityimpr)
#define cityimpr_list_iterate_end  LIST_ITERATE_END

static bool sell_random_building(struct player *pplayer,
                                 struct cityimpr_list *imprs);
static struct unit *sell_random_unit(struct player *pplayer,
                                     struct unit_list *punitlist);

static citizens city_reduce_specialists(struct city *pcity, citizens change);
static citizens city_reduce_workers(struct city *pcity, citizens change);

static bool city_balance_treasury_buildings(struct city *pcity);
static bool city_balance_treasury_units(struct city *pcity);
static bool player_balance_treasury_units_and_buildings
            (struct player *pplayer);
static bool player_balance_treasury_units(struct player *pplayer);

static bool disband_city(struct city *pcity);

static void define_orig_production_values(struct city *pcity);
static void update_city_activity(struct city *pcity);
static void nullify_caravan_and_disband_plus(struct city *pcity);
static bool city_illness_check(const struct city * pcity);

static float city_migration_score(struct city *pcity);
static bool do_city_migration(struct city *pcity_from,
                              struct city *pcity_to);
static bool check_city_migrations_player(const struct player *pplayer);

/**********************************************************************//**
  Updates unit upkeeps and city internal cached data. Returns whether
  city radius has changed.
**************************************************************************/
bool city_refresh(struct city *pcity)
{
  bool retval;

  pcity->server.needs_refresh = FALSE;

  retval = city_map_update_radius_sq(pcity);
  city_units_upkeep(pcity); /* update unit upkeep */
  city_refresh_from_main_map(pcity, NULL);
  city_style_refresh(pcity);

  if (retval) {
    /* Force a sync of the city after the change. */
    send_city_info(city_owner(pcity), pcity);
  }

  return retval;
}

/**********************************************************************//**
  Called on government change or wonder completion or stuff like that
  -- Syela
**************************************************************************/
void city_refresh_for_player(struct player *pplayer)
{
  conn_list_do_buffer(pplayer->connections);
  city_list_iterate(pplayer->cities, pcity) {
    if (city_refresh(pcity)) {
      auto_arrange_workers(pcity);
    }
    send_city_info(pplayer, pcity);
  } city_list_iterate_end;
  conn_list_do_unbuffer(pplayer->connections);
}

/**********************************************************************//**
  Queue pending city_refresh() for later.
**************************************************************************/
void city_refresh_queue_add(struct city *pcity)
{
  if (NULL == city_refresh_queue) {
    city_refresh_queue = city_list_new();
  } else if (city_list_find_number(city_refresh_queue, pcity->id)) {
    return;
  }

  city_list_prepend(city_refresh_queue, pcity);
  pcity->server.needs_refresh = TRUE;
}

/**********************************************************************//**
  Refresh the listed cities.
  Called after significant changes to borders, and arranging workers.
**************************************************************************/
void city_refresh_queue_processing(void)
{
  if (NULL == city_refresh_queue) {
    return;
  }

  city_list_iterate(city_refresh_queue, pcity) {
    if (pcity->server.needs_refresh) {
      if (city_refresh(pcity)) {
        auto_arrange_workers(pcity);
      }
      send_city_info(city_owner(pcity), pcity);
    }
  } city_list_iterate_end;

  city_list_destroy(city_refresh_queue);
  city_refresh_queue = NULL;
}

/**********************************************************************//**
  Automatically sells obsolete buildings from city.
**************************************************************************/
void remove_obsolete_buildings_city(struct city *pcity, bool refresh)
{
  struct player *pplayer = city_owner(pcity);
  bool sold = FALSE;

  city_built_iterate(pcity, pimprove) {
    if (improvement_obsolete(pplayer, pimprove, pcity)
        && can_city_sell_building(pcity, pimprove)) {
      int sgold;

      do_sell_building(pplayer, pcity, pimprove, "obsolete");
      sgold = impr_sell_gold(pimprove);
      notify_player(pplayer, city_tile(pcity), E_IMP_SOLD, ftc_server,
                    PL_("%s is selling %s (obsolete) for %d.",
                        "%s is selling %s (obsolete) for %d.",
                        sgold),
                    city_link(pcity),
                    improvement_name_translation(pimprove),
                    sgold);
      sold = TRUE;
    }
  } city_built_iterate_end;

  if (sold && refresh) {
    if (city_refresh(pcity)) {
      auto_arrange_workers(pcity);
    }
    send_city_info(pplayer, pcity);
    send_player_info_c(pplayer, NULL); /* Send updated gold to all */
  }
}

/**********************************************************************//**
  Sell obsolete buildings from all cities of the player
**************************************************************************/
void remove_obsolete_buildings(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    remove_obsolete_buildings_city(pcity, FALSE);
  } city_list_iterate_end;
}

/**************************************************************************
  Rearrange workers according to a cm_result struct.  The caller must make
  sure that the result is valid.
**************************************************************************/
void apply_cmresult_to_city(struct city *pcity,
                            const struct cm_result *cmr)
{
  struct tile *pcenter = city_tile(pcity);

  /* Now apply results */
  city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity), pcenter,
                                     ptile, idx, x, y) {
    struct city *pwork = tile_worked(ptile);

    if (cmr->worker_positions[idx]) {
      if (NULL == pwork) {
        city_map_update_worker(pcity, ptile);
      } else {
        fc_assert(pwork == pcity);
      }
    } else {
      if (pwork == pcity) {
        city_map_update_empty(pcity, ptile);
      }
    }
  } city_tile_iterate_skip_free_worked_end;

  specialist_type_iterate(sp) {
    pcity->specialists[sp] = cmr->specialists[sp];
  } specialist_type_iterate_end;
}

/**********************************************************************//**
  Call sync_cities() to send the affected cities to the clients.
**************************************************************************/
void auto_arrange_workers(struct city *pcity)
{
  struct cm_parameter cmp;
  struct cm_result *cmr;

  /* See comment in freeze_workers(): we can't rearrange while
   * workers are frozen (i.e. multiple updates need to be done). */
  if (pcity->server.workers_frozen > 0) {
    pcity->server.needs_arrange = TRUE;
    return;
  }
  TIMING_LOG(AIT_CITIZEN_ARRANGE, TIMER_START);

  /* Freeze the workers and make sure all the tiles around the city
   * are up to date.  Then thaw, but hackishly make sure that thaw
   * doesn't call us recursively, which would waste time. */
  city_freeze_workers(pcity);
  pcity->server.needs_arrange = FALSE;

  city_map_update_all(pcity);

  pcity->server.needs_arrange = FALSE;
  city_thaw_workers(pcity);

  /* Now start actually rearranging. */
  city_refresh(pcity);

  sanity_check_city(pcity);
  cm_clear_cache(pcity);

  cm_init_parameter(&cmp);
  cmp.require_happy = FALSE;
  cmp.allow_disorder = FALSE;
  cmp.allow_specialists = TRUE;

  /* We used to look at pplayer->ai.xxx_priority to determine the values
   * to be used here.  However that doesn't work at all because those values
   * are on a different scale.  Later the ai may wish to adjust its
   * priorities - this should be done via a separate set of variables. */
  if (city_size_get(pcity) > 1) {
    if (city_size_get(pcity) <= game.info.notradesize) {
      cmp.factor[O_FOOD] = 15;
    } else {
      if (city_granary_size(city_size_get(pcity)) == pcity->food_stock) {
        /* We don't need more food if the granary is full. */
        cmp.factor[O_FOOD] = 0;
      } else {
        cmp.factor[O_FOOD] = 10;
      }
    }
  } else {
    /* Growing to size 2 is the highest priority. */
    cmp.factor[O_FOOD] = 20;
  }
  cmp.factor[O_SHIELD] = 5;
  cmp.factor[O_TRADE] = 0; /* Trade only provides gold/science. */
  cmp.factor[O_GOLD] = 2;
  cmp.factor[O_LUXURY] = 0; /* Luxury only influences happiness. */
  cmp.factor[O_SCIENCE] = 2;
  cmp.happy_factor = 0;

  if (city_granary_size(city_size_get(pcity)) == pcity->food_stock) {
    cmp.minimal_surplus[O_FOOD] = 0;
  } else {
    cmp.minimal_surplus[O_FOOD] = 1;
  }
  cmp.minimal_surplus[O_SHIELD] = 1;
  cmp.minimal_surplus[O_TRADE] = 0;
  cmp.minimal_surplus[O_GOLD] = -FC_INFINITY;
  cmp.minimal_surplus[O_LUXURY] = 0;
  cmp.minimal_surplus[O_SCIENCE] = 0;

  /* This must be after city_refresh() so that the result gets created for the right
   * city radius */
  cmr = cm_result_new(pcity);
  cm_query_result(pcity, &cmp, cmr, FALSE);

  if (!cmr->found_a_valid) {
    /* Drop surpluses and try again. */
    cmp.minimal_surplus[O_FOOD] = 0;
    cmp.minimal_surplus[O_SHIELD] = 0;
    cmp.minimal_surplus[O_GOLD] = -FC_INFINITY;
    cm_query_result(pcity, &cmp, cmr, FALSE);
  }
  if (!cmr->found_a_valid) {
    /* Emergency management.  Get _some_ result.  This doesn't use
     * cm_init_emergency_parameter so we can keep the factors from
     * above. */
    output_type_iterate(o) {
      cmp.minimal_surplus[o] = MIN(cmp.minimal_surplus[o],
				   MIN(pcity->surplus[o], 0));
    } output_type_iterate_end;
    cmp.require_happy = FALSE;
    cmp.allow_disorder = is_ai(city_owner(pcity)) ? FALSE : TRUE;
    cm_query_result(pcity, &cmp, cmr, FALSE);
  }
  if (!cmr->found_a_valid) {
    CITY_LOG(LOG_DEBUG, pcity, "emergency management");
    cm_init_emergency_parameter(&cmp);
    cm_query_result(pcity, &cmp, cmr, TRUE);
  }
  fc_assert_ret(cmr->found_a_valid);

  apply_cmresult_to_city(pcity, cmr);

  if (pcity->server.debug) {
    /* Print debug output if requested. */
    cm_print_city(pcity);
    cm_print_result(cmr);
  }

  if (city_refresh(pcity)) {
    log_error("%s radius changed when already arranged workers.",
              city_name_get(pcity));
    /* Can't do anything - don't want to enter infinite recursive loop
     * by trying to arrange workers more. */
  }
  sanity_check_city(pcity);

  cm_result_destroy(cmr);
  TIMING_LOG(AIT_CITIZEN_ARRANGE, TIMER_STOP);
}

/**********************************************************************//**
  Notices about cities that should be sent to all players.
**************************************************************************/
static void city_global_turn_notify(struct conn_list *dest)
{
  cities_iterate(pcity) {
    struct impr_type *pimprove = pcity->production.value.building;

    if (VUT_IMPROVEMENT == pcity->production.kind
        && is_great_wonder(pimprove)
        && (1 >= city_production_turns_to_build(pcity, TRUE)
        && can_city_build_improvement_now(pcity, pimprove))) {
      notify_conn(dest, city_tile(pcity),
                  E_WONDER_WILL_BE_BUILT, ftc_server,
                  _("Notice: Wonder %s in %s will be finished next turn."),
                  improvement_name_translation(pimprove), city_link(pcity));
    }
  } cities_iterate_end;
}

/**********************************************************************//**
  Send turn notifications for specified city to specified connections.
  If 'pplayer' is not NULL, the message will be cached for this player.
**************************************************************************/
static void city_turn_notify(const struct city *pcity,
                             struct conn_list *dest,
                             const struct player *cache_for_player)
{
  struct impr_type *pimprove = pcity->production.value.building;
  struct packet_chat_msg packet;
  int turns_growth, turns_granary;

  if (0 < pcity->surplus[O_FOOD]) {
    turns_growth = (city_granary_size(city_size_get(pcity))
                    - pcity->food_stock - 1) / pcity->surplus[O_FOOD];

    if (0 == get_city_bonus(pcity, EFT_GROWTH_FOOD)
        && 0 < get_current_construction_bonus(pcity, EFT_GROWTH_FOOD,
                                              RPT_CERTAIN)
        && 0 < pcity->surplus[O_SHIELD]) {
      /* From the check above, the surplus must always be positive. */
      turns_granary = (impr_build_shield_cost(pcity, pimprove)
                       - pcity->shield_stock) / pcity->surplus[O_SHIELD];
      /* If growth and granary completion occur simultaneously, granary
       * preserves food.  -AJS. */
      if (5 > turns_growth && 5 > turns_granary
          && turns_growth < turns_granary) {
        package_event(&packet, city_tile(pcity),
                      E_CITY_GRAN_THROTTLE, ftc_server,
                      _("Suggest throttling growth in %s to use %s "
                        "(being built) more effectively."),
                      city_link(pcity),
                      improvement_name_translation(pimprove));
        lsend_packet_chat_msg(dest, &packet);
        if (NULL != cache_for_player) {
          event_cache_add_for_player(&packet, cache_for_player);
        }
      }
    }

    if (0 >= turns_growth && !city_celebrating(pcity)
        && city_can_grow_to(pcity, city_size_get(pcity) + 1)) {
      package_event(&packet, city_tile(pcity),
                    E_CITY_MAY_SOON_GROW, ftc_server,
                    _("%s may soon grow to size %i."),
                    city_link(pcity), city_size_get(pcity) + 1);
      lsend_packet_chat_msg(dest, &packet);
      if (NULL != cache_for_player) {
        event_cache_add_for_player(&packet, cache_for_player);
      }
    }
  } else {
    if (0 >= pcity->food_stock + pcity->surplus[O_FOOD]
        && 0 > pcity->surplus[O_FOOD]) {
      package_event(&packet, city_tile(pcity),
                    E_CITY_FAMINE_FEARED, ftc_server,
                    _("Warning: Famine feared in %s."), city_link(pcity));
      lsend_packet_chat_msg(dest, &packet);
      if (NULL != cache_for_player) {
        event_cache_add_for_player(&packet, cache_for_player);
      }
    }
  }
}

/**********************************************************************//**
  Send global and player specific city turn notifications. If 'pconn' is
  NULL, it will send to all connections and cache the events.
**************************************************************************/
void send_city_turn_notifications(struct connection *pconn)
{
  if (NULL != pconn) {
    struct player *pplayer = conn_get_player(pconn);

    if (NULL != pplayer) {
      city_list_iterate(pplayer->cities, pcity) {
        city_turn_notify(pcity, pconn->self, NULL);
      } city_list_iterate_end;
    }
    city_global_turn_notify(pconn->self);
  } else {
    players_iterate(pplayer) {
      city_list_iterate(pplayer->cities, pcity) {
        city_turn_notify(pcity, pplayer->connections, pplayer);
      } city_list_iterate_end;
    } players_iterate_end;
    /* NB: notifications to 'game.est_connections' are automatically
     * cached. */
    city_global_turn_notify(game.est_connections);
  }
}

/**********************************************************************//**
  Update all cities of one nation (costs for buildings, unit upkeep, ...).
**************************************************************************/
void update_city_activities(struct player *pplayer)
{
  char buf[4 * MAX_LEN_NAME];
  int n, gold;

  fc_assert(NULL != pplayer);
  fc_assert(NULL != pplayer->cities);

  n = city_list_size(pplayer->cities);
  gold = pplayer->economic.gold;
  pplayer->server.bulbs_last_turn = 0;

  if (n > 0) {
    struct city *cities[n];
    int i = 0, r;

    city_list_iterate(pplayer->cities, pcity) {

      citizens_convert(pcity);

      /* Cancel traderoutes that cannot exist any more */
      trade_routes_iterate_safe(pcity, proute) {
        struct city *tcity = game_city_by_number(proute->partner);

        if (tcity != NULL) {
          bool cancel = FALSE;

          if (proute->dir != RDIR_FROM && goods_has_flag(proute->goods, GF_DEPLETES)
              && !goods_can_be_provided(tcity, proute->goods, NULL)) {
            cancel = TRUE;
          }
          if (!cancel && !can_cities_trade(pcity, tcity)) {
            enum trade_route_type type = cities_trade_route_type(pcity, tcity);
            struct trade_route_settings *settings = trade_route_settings_by_type(type);

            if (settings->cancelling == TRI_CANCEL) {
              cancel = TRUE;
            }
          }

          if (cancel) {
            struct trade_route *back;

            back = remove_trade_route(pcity, proute, TRUE, FALSE);
            free(proute);
            free(back);
          }
        }
      } trade_routes_iterate_safe_end;

      /* Add cities to array for later random order handling */
      cities[i++] = pcity;
    } city_list_iterate_end;

    /* How gold upkeep is handled depends on the setting
     * 'game.info.gold_upkeep_style':
     * GOLD_UPKEEP_CITY: Each city tries to balance its upkeep individually
     *                   (this is done in update_city_activity()).
     * GOLD_UPKEEP_MIXED: Each city tries to balance its upkeep for
     *                    buildings individually; the upkeep for units is
     *                    paid by the nation.
     * GOLD_UPKEEP_NATION: The nation as a whole balances the treasury. If
     *                     the treasury is not balance units and buildings
     *                     are sold. */

    /* Iterate over cities in a random order. */
    while (i > 0) {
      r = fc_rand(i);
      /* update unit upkeep */
      city_units_upkeep(cities[r]);
      update_city_activity(cities[r]);
      cities[r] = cities[--i];
    }

    if (pplayer->economic.gold < 0) {
      switch (game.info.gold_upkeep_style) {
      case GOLD_UPKEEP_CITY:
        break;
      case GOLD_UPKEEP_MIXED:
        /* Nation pays for units. */
        player_balance_treasury_units(pplayer);
        break;
      case GOLD_UPKEEP_NATION:
        /* Nation pays for units and buildings. */
        player_balance_treasury_units_and_buildings(pplayer);
        break;
      }
    }

    /* Should not happen. */
    fc_assert(pplayer->economic.gold >= 0);
  }

  /* This test includes the cost of the units because
   * units are paid for in update_city_activity() or
   * player_balance_treasury_units(). */
  if (gold - (gold - pplayer->economic.gold) * 3 < 0) {
    notify_player(pplayer, NULL, E_LOW_ON_FUNDS, ftc_server,
                  _("WARNING, we're LOW on FUNDS %s."),
                  ruler_title_for_player(pplayer, buf, sizeof(buf)));
  }

#if 0
  /* Uncomment to unbalance the game, like in civ1 (CLG). */
  if (pplayer->got_tech && pplayer->research->researched > 0) {
    pplayer->research->researched = 0;
  }
#endif

  city_refresh_queue_processing();
}

/**********************************************************************//**
  Try to get rid of a unit because of missing upkeep.

  Won't try to get rid of a unit without any action auto performers for
  AAPC_UNIT_UPKEEP. Those are seen as protected from being destroyed
  because of missing upkeep.

  Can optionally wipe the unit in the end if it survived the actions in
  the selected action auto performer.

  Returns TRUE if the unit went away.
**************************************************************************/
static bool upkeep_kill_unit(struct unit *punit, Output_type_id outp,
                             enum unit_loss_reason wipe_reason,
                             bool wipe_in_the_end)
{
  int punit_id;

  if (!action_auto_perf_unit_sel(AAPC_UNIT_UPKEEP, punit,
                                 NULL, get_output_type(outp))) {
    /* Can't get rid of this unit. It is undisbandable for the current
     * situation. */
    return FALSE;
  }

  punit_id = punit->id;

  /* Try to perform this unit's can't upkeep actions. */
  action_auto_perf_unit_do(AAPC_UNIT_UPKEEP, punit,
                           NULL, get_output_type(outp),
                           NULL, NULL, NULL, NULL);

  if (wipe_in_the_end && unit_is_alive(punit_id)) {
    /* No forced action was able to kill the unit. Finish the job. */
    wipe_unit(punit, wipe_reason, NULL);
  }

  return !unit_is_alive(punit_id);
}

/**********************************************************************//**
  Reduce the city specialists by some (positive) value.
  Return the amount of reduction.
**************************************************************************/
static citizens city_reduce_specialists(struct city *pcity, citizens change)
{
  citizens want = change;

  fc_assert_ret_val(0 < change, 0);

  specialist_type_iterate(sp) {
    citizens fix = MIN(want, pcity->specialists[sp]);

    pcity->specialists[sp] -= fix;
    want -= fix;
  } specialist_type_iterate_end;

  return change - want;
}

/**********************************************************************//**
  Reduce the city workers by some (positive) value.
  Return the amount of reduction.
**************************************************************************/
static citizens city_reduce_workers(struct city *pcity, citizens change)
{
  struct tile *pcenter = city_tile(pcity);
  int want = change;

  fc_assert_ret_val(0 < change, 0);

  city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity), pcenter,
                                     ptile, _index, _x, _y) {
    if (0 < want && tile_worked(ptile) == pcity) {
      city_map_update_empty(pcity, ptile);
      want--;
    }
  } city_tile_iterate_skip_free_worked_end;

  return change - want;
}

/**********************************************************************//**
  Reduce the city size.  Return TRUE if the city survives the population
  loss.
**************************************************************************/
bool city_reduce_size(struct city *pcity, citizens pop_loss,
                      struct player *destroyer, const char *reason)
{
  citizens loss_remain;
  int old_radius_sq;

  if (pop_loss == 0) {
    return TRUE;
  }

  if (city_size_get(pcity) <= pop_loss) {

    script_server_signal_emit("city_destroyed", pcity, pcity->owner,
                              destroyer);

    remove_city(pcity);
    return FALSE;
  }
  old_radius_sq = tile_border_source_radius_sq(pcity->tile);
  city_size_add(pcity, -pop_loss);
  map_update_border(pcity->tile, pcity->owner, old_radius_sq,
                    tile_border_source_radius_sq(pcity->tile));

  /* Cap the food stock at the new granary size. */
  if (pcity->food_stock > city_granary_size(city_size_get(pcity))) {
    pcity->food_stock = city_granary_size(city_size_get(pcity));
  }

  /* First try to kill off the specialists */
  loss_remain = pop_loss - city_reduce_specialists(pcity, pop_loss);

  if (loss_remain > 0) {
    /* Take it out on workers */
    loss_remain -= city_reduce_workers(pcity, loss_remain);
  }

  /* Update citizens. */
  citizens_update(pcity, NULL);

  /* Update number of people in each feelings category.
   * This also updates the city radius if needed. */
  city_refresh(pcity);

  auto_arrange_workers(pcity);

  /* Send city data. */
  sync_cities();

  fc_assert_ret_val_msg(0 == loss_remain, TRUE,
                        "city_reduce_size() has remaining"
                        "%d of %d for \"%s\"[%d]",
                        loss_remain, pop_loss,
                        city_name_get(pcity), city_size_get(pcity));

  /* Update cities that have trade routes with us */
  trade_partners_iterate(pcity, pcity2) {
    if (city_refresh(pcity2)) {
      /* This should never happen, but if it does, make sure not to
       * leave workers outside city radius. */
      auto_arrange_workers(pcity2);
    }
  } trade_partners_iterate_end;

  sanity_check_city(pcity);

  if (reason != NULL) {
    int id = pcity->id;

    script_server_signal_emit("city_size_change", pcity, -pop_loss, reason);

    return city_exist(id);
  }

  return TRUE;
}

/**********************************************************************//**
  Repair the city population without affecting city size.
  Used by savegame.c and sanitycheck.c
**************************************************************************/
void city_repair_size(struct city *pcity, int change)
{
  if (change > 0) {
    pcity->specialists[DEFAULT_SPECIALIST] += change;
  } else if (change < 0) {
    int need = change + city_reduce_specialists(pcity, -change);

    if (0 > need) {
      need += city_reduce_workers(pcity, -need);
    }

    fc_assert_msg(0 == need,
                  "city_repair_size() has remaining %d of %d for \"%s\"[%d]",
                  need, change, city_name_get(pcity), city_size_get(pcity));
  }
}

/**********************************************************************//**
  Return the percentage of food that is lost in this city.

  Normally this value is 0% but this can be increased by EFT_GROWTH_FOOD
  effects.
**************************************************************************/
static int granary_savings(const struct city *pcity)
{
  int savings = get_city_bonus(pcity, EFT_GROWTH_FOOD);

  return CLIP(0, savings, 100);
}

/**********************************************************************//**
  Reset the foodbox, usually when a city grows or shrinks.
  By default it is reset to zero, but this can be increased by Growth_Food
  effects.
  Usually this should be called before the city changes size.
**************************************************************************/
static void city_reset_foodbox(struct city *pcity, int new_size)
{
  fc_assert_ret(pcity != NULL);
  pcity->food_stock = (city_granary_size(new_size)
                       * granary_savings(pcity)) / 100;
}

/**********************************************************************//**
  Increase city size by one. We do not refresh borders or send info about
  the city to the clients as part of this function. There might be several
  calls to this function at once, and those actions are needed only once.
**************************************************************************/
static bool city_increase_size(struct city *pcity, struct player *nationality)
{
  int new_food;
  int savings_pct = granary_savings(pcity);
  bool have_square = FALSE;
  bool rapture_grow = city_rapture_grow(pcity); /* check before size increase! */
  struct tile *pcenter = city_tile(pcity);
  struct player *powner = city_owner(pcity);
  struct impr_type *pimprove = pcity->production.value.building;
  int saved_id = pcity->id;

  if (!city_can_grow_to(pcity, city_size_get(pcity) + 1)) {
    /* need improvement */
    if (get_current_construction_bonus(pcity, EFT_SIZE_ADJ, RPT_CERTAIN) > 0
        || get_current_construction_bonus(pcity, EFT_SIZE_UNLIMIT, RPT_CERTAIN) > 0) {
      notify_player(powner, city_tile(pcity), E_CITY_AQ_BUILDING, ftc_server,
                    _("%s needs %s (being built) to grow beyond size %d."),
                    city_link(pcity),
                    improvement_name_translation(pimprove),
                    city_size_get(pcity));
    } else {
      notify_player(powner, city_tile(pcity), E_CITY_AQUEDUCT, ftc_server,
                    _("%s needs an improvement to grow beyond size %d."),
                    city_link(pcity), city_size_get(pcity));
    }
    /* Granary can only hold so much */
    new_food = (city_granary_size(city_size_get(pcity))
                * (100 * 100 - game.server.aqueductloss * (100 - savings_pct))
                / (100 * 100));
    pcity->food_stock = MIN(pcity->food_stock, new_food);
    return FALSE;
  }

  city_size_add(pcity, 1);

  /* Do not empty food stock if city is growing by celebrating */
  if (rapture_grow) {
    new_food = city_granary_size(city_size_get(pcity));
  } else {
    new_food = city_granary_size(city_size_get(pcity)) * savings_pct / 100;
  }
  pcity->food_stock = MIN(pcity->food_stock, new_food);

  /* If there is enough food, and the city is big enough,
   * make new citizens into scientists or taxmen -- Massimo */

  /* Ignore food if no square can be worked */
  city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity), pcenter,
                                     ptile, _index, _x, _y) {
    if (tile_worked(ptile) != pcity /* quick test */
     && city_can_work_tile(pcity, ptile)) {
      have_square = TRUE;
    }
  } city_tile_iterate_skip_free_worked_end;

  if ((pcity->surplus[O_FOOD] >= 2 || !have_square)
      && is_city_option_set(pcity, CITYO_SCIENCE_SPECIALISTS)) {
    pcity->specialists[best_specialist(O_SCIENCE, pcity)]++;
  } else if ((pcity->surplus[O_FOOD] >= 2 || !have_square)
	     && is_city_option_set(pcity, CITYO_GOLD_SPECIALISTS)) {
    pcity->specialists[best_specialist(O_GOLD, pcity)]++;
  } else {
    pcity->specialists[DEFAULT_SPECIALIST]++; /* or else city is !sane */
  }

  /* Update citizens. */
  citizens_update(pcity, nationality);

  /* Refresh the city data; this also checks the squared city radius. */
  city_refresh(pcity);

  auto_arrange_workers(pcity);

  /* Update cities that have trade routes with us */
  trade_partners_iterate(pcity, pcity2) {
    if (city_refresh(pcity2)) {
      /* This should never happen, but if it does, make sure not to
       * leave workers outside city radius. */
      auto_arrange_workers(pcity2);
    }
  } trade_partners_iterate_end;

  notify_player(powner, city_tile(pcity), E_CITY_GROWTH, ftc_server,
                _("%s grows to size %d."),
                city_link(pcity), city_size_get(pcity));

  /* Deprecated signal. Connect your lua functions to "city_size_change" that's
   * emitted from calling functions which know the 'reason' of the increase. */
  script_server_signal_emit("city_growth", pcity, city_size_get(pcity));
  if (city_exist(saved_id)) {
    /* Script didn't destroy this city */
    sanity_check_city(pcity);
  }
  sync_cities();

  return TRUE;
}

/**********************************************************************//**
  Change the city size.  Return TRUE iff the city is still alive afterwards.
**************************************************************************/
bool city_change_size(struct city *pcity, citizens size,
                      struct player *nationality, const char *reason)
{
  int change = size - city_size_get(pcity);

  if (change > 0) {
    int old_size = city_size_get(pcity);
    int real_change;

    /* Increase city size until size reached, or increase fails */
    while (size > city_size_get(pcity)
           && city_increase_size(pcity, nationality)) {
      /* city_increase_size() does all the work. */
    }

    real_change = city_size_get(pcity) - old_size;

    if (real_change != 0 && reason != NULL) {
      int id = pcity->id;

      script_server_signal_emit("city_size_change", pcity, real_change,
                                reason);

      if (!city_exist(id)) {
        return FALSE;
      }
    }
  } else if (change < 0) {
    /* We assume that city_change_size() is never called because
     * of enemy actions. If that changes, enemy must be passed
     * to city_reduce_size() */
    return city_reduce_size(pcity, -change, NULL, reason);
  }

  map_claim_border(pcity->tile, pcity->owner, -1);

  return TRUE;
}

/**********************************************************************//**
  Check whether the population can be increased or
  if the city is unable to support a 'settler'...
**************************************************************************/
static void city_populate(struct city *pcity, struct player *nationality)
{
  int saved_id = pcity->id;
  int granary_size = city_granary_size(city_size_get(pcity));

  pcity->food_stock += pcity->surplus[O_FOOD];
  if (pcity->food_stock >= granary_size || city_rapture_grow(pcity)) {
    if (city_had_recent_plague(pcity)) {
      notify_player(city_owner(pcity), city_tile(pcity),
                    E_CITY_PLAGUE, ftc_server,
                    _("A recent plague outbreak prevents growth in %s."),
                    city_link(pcity));
      /* Lose excess food */
      pcity->food_stock = MIN(pcity->food_stock, granary_size);
    } else {
      bool success;

      success = city_increase_size(pcity, nationality);
      map_claim_border(pcity->tile, pcity->owner, -1);

      if (success) {
        script_server_signal_emit("city_size_change", pcity, 1, "growth");
      }
    }
  } else if (pcity->food_stock < 0) {
    /* FIXME: should this depend on units with ability to build
     * cities or on units that require food in upkeep?
     * I'll assume citybuilders (units that 'contain' 1 pop) -- sjolie
     * The above may make more logical sense, but in game terms
     * you want to disband a unit that is draining your food
     * reserves.  Hence, I'll assume food upkeep > 0 units. -- jjm
     */
    unit_list_iterate_safe(pcity->units_supported, punit) {
      if (punit->upkeep[O_FOOD] > 0) {
        const char *punit_link = unit_tile_link(punit);

        if (upkeep_kill_unit(punit, O_FOOD, ULR_STARVED,
                             game.info.muuk_food_wipe)) {
          notify_player(city_owner(pcity), city_tile(pcity),
                        E_UNIT_LOST_MISC, ftc_server,
                        _("Famine feared in %s, %s lost!"),
                        city_link(pcity), punit_link);
        }

        if (city_exist(saved_id)) {
          city_reset_foodbox(pcity, city_size_get(pcity));
        }
	return;
      }
    } unit_list_iterate_safe_end;
    if (city_size_get(pcity) > 1) {
      notify_player(city_owner(pcity), city_tile(pcity),
                    E_CITY_FAMINE, ftc_server,
                    _("Famine causes population loss in %s."),
                    city_link(pcity));
    } else {
      notify_player(city_owner(pcity), city_tile(pcity),
                    E_CITY_FAMINE, ftc_server,
		    _("Famine destroys %s entirely."),
		    city_link(pcity));
    }
    city_reset_foodbox(pcity, city_size_get(pcity) - 1);
    city_reduce_size(pcity, 1, NULL, "famine");
  }
}

/**********************************************************************//**
  Examine the worklist and change the build target.  Return 0 if no
  targets are available to change to.  Otherwise return non-zero.  Has
  the side-effect of removing from the worklist any no-longer-available
  targets as well as the target actually selected, if any.
**************************************************************************/
static bool worklist_change_build_target(struct player *pplayer,
					 struct city *pcity)
{
  struct universal target;
  bool success = FALSE;
  int i;
  int saved_id = pcity->id;
  bool city_checked = TRUE; /* This is used to avoid spurious city_exist() calls */
  struct worklist *pwl = &pcity->worklist;

  if (worklist_is_empty(pwl)) {
    /* Nothing in the worklist; bail now. */
    return FALSE;
  }

  i = 0;
  while (!success && i < worklist_length(pwl)) {

    if (!city_checked) {
      if (!city_exist(saved_id)) {
        /* Some script has removed useless city that cannot build
         * what it is told to! */
        return FALSE;
      }
      city_checked = TRUE;
    }

    if (worklist_peek_ith(pwl, &target, i)) {
      success = can_city_build_now(pcity, &target);
    } else {
      success = FALSE;
    }
    i++;

    if (success) {
      break; /* while */
    }

    switch (target.kind) {
    case VUT_UTYPE:
    {
      struct unit_type *ptarget = target.value.utype;
      struct unit_type *pupdate = unit_upgrades_to(pcity, ptarget);

      /* Maybe we can just upgrade the target to what the city /can/ build. */
      if (U_NOT_OBSOLETED == pupdate) {
	/* Nope, we're stuck.  Skip this item from the worklist. */
        if (ptarget->need_government != NULL
            && ptarget->need_government != government_of_player(pplayer)) {
          notify_player(pplayer, city_tile(pcity),
                        E_CITY_CANTBUILD, ftc_server,
                        _("%s can't build %s from the worklist; "
                          "it needs %s government. Postponing..."),
                        city_link(pcity), utype_name_translation(ptarget),
                        government_name_translation(ptarget->need_government));
          script_server_signal_emit("unit_cant_be_built", ptarget, pcity,
                                    "need_government");
        } else if (ptarget->need_improvement != NULL
                   && !city_has_building(pcity, ptarget->need_improvement)) {
          notify_player(pplayer, city_tile(pcity),
                        E_CITY_CANTBUILD, ftc_server,
                        _("%s can't build %s from the worklist; "
                          "need to have %s first. Postponing..."),
                        city_link(pcity), utype_name_translation(ptarget),
                        city_improvement_name_translation(pcity,
                                                  ptarget->need_improvement));
          script_server_signal_emit("unit_cant_be_built", ptarget, pcity,
                                    "need_building");
        } else if (ptarget->require_advance != NULL
                   && TECH_KNOWN != research_invention_state
                          (research_get(pplayer),
                           advance_number(ptarget->require_advance))) {
          notify_player(pplayer, city_tile(pcity),
                        E_CITY_CANTBUILD, ftc_server,
                        _("%s can't build %s from the worklist; "
                          "tech %s not yet available. Postponing..."),
                        city_link(pcity), utype_name_translation(ptarget),
                        advance_name_translation(ptarget->require_advance));
          script_server_signal_emit("unit_cant_be_built", ptarget, pcity,
                                    "need_tech");
        } else {
          /* This shouldn't happen, but in case it does... */
          notify_player(pplayer, city_tile(pcity),
                        E_CITY_CANTBUILD, ftc_server,
                        _("%s can't build %s from the worklist; "
                          "reason unknown! Postponing..."),
                        city_link(pcity), utype_name_translation(ptarget));
        }
        city_checked = FALSE;
	break;
      }
      success = can_city_build_unit_later(pcity, pupdate);
      if (!success) {
	/* If the city can never build this unit or its descendants,
	 * drop it. */
	notify_player(pplayer, city_tile(pcity),
                      E_CITY_CANTBUILD, ftc_server,
                      _("%s can't build %s from the worklist. Purging..."),
                      city_link(pcity),
			 /* Yes, warn about the targets that's actually
			    in the worklist, not its obsolete-closure
			    pupdate. */
			 utype_name_translation(ptarget));
        script_server_signal_emit("unit_cant_be_built", ptarget, pcity,
                                  "never");
        if (city_exist(saved_id)) {
          city_checked = TRUE;
          /* Purge this worklist item. */
          i--;
          worklist_remove(pwl, i);
        } else {
          city_checked = FALSE;
        }
      } else {
	/* Yep, we can go after pupdate instead.  Joy! */
        notify_player(pplayer, city_tile(pcity), E_WORKLIST, ftc_server,
                      _("Production of %s is upgraded to %s in %s."),
                      utype_name_translation(ptarget), 
                      utype_name_translation(pupdate),
                      city_link(pcity));
	target.value.utype = pupdate;
      }
      break;
    }
    case VUT_IMPROVEMENT:
    {
      struct impr_type *ptarget = target.value.building;
      struct impr_type *pupdate = building_upgrades_to(pcity, ptarget);

      /* If the city can never build this improvement, drop it. */
      success = can_city_build_improvement_later(pcity, pupdate);

      /* Maybe this improvement has been obsoleted by something that
	 we can build. */
      if (success && pupdate == ptarget) {
	bool known = FALSE;

	/* Nope, no use.  *sigh*  */
	requirement_vector_iterate(&ptarget->reqs, preq) {
	  if (!is_req_active(pplayer, NULL, pcity, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, preq, RPT_POSSIBLE)) {
	    known = TRUE;
	    switch (preq->source.kind) {
	    case VUT_ADVANCE:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "tech %s not yet available. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              advance_name_translation
                                  (preq->source.value.advance));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_tech");
              } else {
                /* While techs can be unlearned, this isn't useful feedback */
                success = FALSE;
              }
	      break;
            case VUT_TECHFLAG:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "no tech with flag \"%s\" yet available. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              tech_flag_id_name(preq->source.value.techflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_techflag");
              } else {
                /* While techs can be unlearned, this isn't useful feedback */
                success = FALSE;
              }
              break;
	    case VUT_IMPROVEMENT:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "need to have %s first. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              city_improvement_name_translation(pcity,
						  preq->source.value.building));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_building");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "need to not have %s. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              city_improvement_name_translation(pcity,
						  preq->source.value.building));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_building");
              }
	      break;
            case VUT_IMPR_GENUS:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "need to have %s first. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              impr_genus_id_translated_name(
                                preq->source.value.impr_genus));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_building_genus");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "need to not have %s. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              impr_genus_id_translated_name(
                                preq->source.value.impr_genus));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_building_genus");
              }
              break;
            case VUT_GOVERNMENT:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "it needs %s government. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              government_name_translation(preq->source.value.govern));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_government");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "it cannot have %s government. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              government_name_translation(preq->source.value.govern));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_government");
              }
	      break;
	    case VUT_ACHIEVEMENT:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "it needs \"%s\" achievement. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              achievement_name_translation(preq->source.value.achievement));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_achievement");
              } else {
                /* Can't unachieve things. */
                success = FALSE;
              }
	      break;
	    case VUT_EXTRA:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?extra:%s can't build %s from the worklist; "
                                 "%s is required. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              extra_name_translation(preq->source.value.extra));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_extra");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?extra:%s can't build %s from the worklist; "
                                 "%s is prohibited. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              extra_name_translation(preq->source.value.extra));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_extra");
              }
	      break;
            case VUT_GOOD:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?extra:%s can't build %s from the worklist; "
                                 "%s is required. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              goods_name_translation(preq->source.value.good));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_good");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?extra:%s can't build %s from the worklist; "
                                 "%s is prohibited. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              goods_name_translation(preq->source.value.good));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_good");
              }
	      break;
	    case VUT_TERRAIN:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?terrain:%s can't build %s from the worklist; "
                                 "%s terrain is required. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              terrain_name_translation(preq->source.value.terrain));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_terrain");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?terrain:%s can't build %s from the worklist; "
                                 "%s terrain is prohibited. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              terrain_name_translation(preq->source.value.terrain));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_terrain");
              }
	      break;
            case VUT_NATION:
              /* Nation can be required at Alliance range, which may change. */
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: "%s nation" is adjective */
                              Q_("?nation:%s can't build %s from the worklist; "
                                 "%s nation is required. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              nation_adjective_translation(preq->source.value.nation));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_nation");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?nation:%s can't build %s from the worklist; "
                                 "%s nation is prohibited. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              nation_adjective_translation(preq->source.value.nation));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_nation");
              }
              break;
            case VUT_NATIONGROUP:
              /* Nation group can be required at Alliance range, which may
               * change. */
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: "%s nation" is adjective */
                              Q_("?ngroup:%s can't build %s from the worklist; "
                                 "%s nation is required. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              nation_group_name_translation(preq->source.value.nationgroup));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_nationgroup");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?ngroup:%s can't build %s from the worklist; "
                                 "%s nation is prohibited. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              nation_group_name_translation(preq->source.value.nationgroup));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_nationgroup");
              }
              break;
            case VUT_STYLE:
              /* FIXME: City styles sometimes change over time, but it isn't
               * entirely under player control.  Probably better to purge
               * with useful explanation. */
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "only %s style cities may build this. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              style_name_translation(preq->source.value.style));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_style");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "%s style cities may not build this. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              style_name_translation(preq->source.value.style));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_style");
              }
	      break;
	    case VUT_NATIONALITY:
              /* FIXME: Changing citizen nationality is hard: purging might be
               * more useful in this case. */
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: Latter %s is citizen nationality */
                              _("%s can't build %s from the worklist; "
                                "only city with %s may build this. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              nation_plural_translation(preq->source.value.nationality));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_nationality");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: Latter %s is citizen nationality */
                              _("%s can't build %s from the worklist; "
                                "only city without %s may build this. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              nation_plural_translation(preq->source.value.nationality));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_nationality");
              }
              break;
            case VUT_DIPLREL:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: '%s' is a wide range of relationships;
                               * e.g., 'Peace', 'Never met', 'Foreign',
                               * 'Hosts embassy', 'Provided Casus Belli' */
                              _("%s can't build %s from the worklist; "
                                "the relationship '%s' is required."
                                "  Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity,
                                                                ptarget),
                              diplrel_name_translation(
                                preq->source.value.diplrel));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_diplrel");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "the relationship '%s' is prohibited."
                                "  Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity,
                                                                ptarget),
                              diplrel_name_translation(
                                preq->source.value.diplrel));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_diplrel");
              }
              break;
	    case VUT_MINSIZE:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "city must be of size %d or larger. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              preq->source.value.minsize);
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_minsize");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "city must be of size %d or smaller."
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              (preq->source.value.minsize - 1));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_minsize");
              }
	      break;
	    case VUT_MINCULTURE:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "city must have culture of %d. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              preq->source.value.minculture);
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_minculture");
              } else {
                /* What has been written may not be unwritten. */
                success = FALSE;
              }
	      break;
            case VUT_MINTECHS:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "%d techs must be known. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              preq->source.value.min_techs);
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_mintechs");
              } else {
                success = FALSE;
              }
              break;
	    case VUT_MAXTILEUNITS:
	      if (preq->present) {
		notify_player(pplayer, city_tile(pcity),
			      E_CITY_CANTBUILD, ftc_server,
			      PL_("%s can't build %s from the worklist; "
                                  "more than %d unit on tile."
                                  "  Postponing...",
			          "%s can't build %s from the worklist; "
                                  "more than %d units on tile."
                                  "  Postponing...",
                                  preq->source.value.max_tile_units),
			      city_link(pcity),
			      city_improvement_name_translation(pcity,
								ptarget),
                              preq->source.value.max_tile_units);
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_tileunits");
	      } else {
		notify_player(pplayer, city_tile(pcity),
			      E_CITY_CANTBUILD, ftc_server,
			      PL_("%s can't build %s from the worklist; "
                                  "fewer than %d unit on tile."
                                  "  Postponing...",
			          "%s can't build %s from the worklist; "
                                  "fewer than %d units on tile."
                                  "  Postponing...",
                                  preq->source.value.max_tile_units + 1),
			      city_link(pcity),
			      city_improvement_name_translation(pcity,
								ptarget),
                              preq->source.value.max_tile_units + 1);
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_tileunits");
	      }
              break;
            case VUT_AI_LEVEL:
              /* Can't change AI level. */
              success = FALSE;
	      break;
	    case VUT_TERRAINCLASS:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?terrainclass:%s can't build %s from the "
                                 "worklist; %s terrain is required."
                                 "  Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              terrain_class_name_translation(preq->source.value.terrainclass));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_terrainclass");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              Q_("?terrainclass:%s can't build %s from the "
                                 "worklist; %s terrain is prohibited."
                                 "  Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              terrain_class_name_translation(preq->source.value.terrainclass));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_terrainclass");
              }
	      break;
	    case VUT_TERRFLAG:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "terrain with \"%s\" flag is required. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              terrain_flag_id_name(preq->source.value.terrainflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_terrainflag");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "terrain with \"%s\" flag is prohibited. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              terrain_flag_id_name(preq->source.value.terrainflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_terrainflag");
              }
	      break;
            case VUT_BASEFLAG:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "base with \"%s\" flag is required. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              base_flag_id_name(preq->source.value.baseflag));
                script_server_signal_emit("building_cant_be_built",
                                          ptarget, pcity, "need_baseflag");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "base with \"%s\" flag is prohibited. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              base_flag_id_name(preq->source.value.baseflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_baseflag");
              }
              break;
            case VUT_ROADFLAG:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "road with \"%s\" flag is required. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              road_flag_id_name(preq->source.value.roadflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_roadflag");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "road with \"%s\" flag is prohibited. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              road_flag_id_name(preq->source.value.roadflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_roadflag");
              }
	      break;
            case VUT_EXTRAFLAG:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "extra with \"%s\" flag is required. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              extra_flag_id_translated_name(preq->source.value.extraflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_extraflag");
              } else {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "extra with \"%s\" flag is prohibited. "
                                "Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              extra_flag_id_translated_name(preq->source.value.extraflag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_extraflag");
              }
	      break;
	    case VUT_UTYPE:
	    case VUT_UTFLAG:
	    case VUT_UCLASS:
	    case VUT_UCFLAG:
            case VUT_MINVETERAN:
            case VUT_UNITSTATE:
            case VUT_MINMOVES:
            case VUT_MINHP:
            case VUT_ACTION:
	    case VUT_OTYPE:
	    case VUT_SPECIALIST:
	    case VUT_TERRAINALTER: /* XXX could do this in principle */
	    case VUT_CITYTILE:
	      /* Will only happen with a bogus ruleset. */
              log_error("worklist_change_build_target() has bogus preq");
	      break;
            case VUT_MINYEAR:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: last %s is a date */
                              _("%s can't build %s from the worklist; "
                                "only available from %s. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              textyear(preq->source.value.minyear));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_minyear");
              } else {
                /* Can't go back in time. */
                success = FALSE;
              }
              break;
            case VUT_MINCALFRAG:
              /* Unlike VUT_MINYEAR, a requirement in either direction is
               * likely to be fulfilled sooner or later. */
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: last %s is a calendar fragment from
                               * the ruleset; may be a bare number */
                              _("%s can't build %s from the worklist; "
                                "only available from %s. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              textcalfrag(preq->source.value.mincalfrag));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_mincalfrag");
              } else {
                fc_assert_action(preq->source.value.mincalfrag > 0, break);
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              /* TRANS: last %s is a calendar fragment from
                               * the ruleset; may be a bare number */
                              _("%s can't build %s from the worklist; "
                                "not available after %s. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              textcalfrag(preq->source.value.mincalfrag-1));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "have_mincalfrag");
              }
              break;
            case VUT_TOPO:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "only available in worlds with %s map."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              _(topo_flag_name(preq->source.value.topo_property)));
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_topo");
              }
              success = FALSE;
              break;
            case VUT_SERVERSETTING:
              notify_player(pplayer, city_tile(pcity),
                            E_CITY_CANTBUILD, ftc_server,
                            /* TRANS: %s is a server setting, its value and
                             * if it is required to be present or absent.
                             * The string's format is specified in
                             * ssetv_human_readable().
                             * Example: "killstack is enabled". */
                            _("%s can't build %s from the worklist; "
                              "only available when the server setting "
                              "%s."),
                            city_link(pcity),
                            city_improvement_name_translation(pcity,
                                                              ptarget),
                            ssetv_human_readable(preq->source.value.ssetval,
                                                 preq->present));
              script_server_signal_emit("building_cant_be_built", ptarget,
                                        pcity, "need_setting");
              /* Don't assume that the server setting will be changed. */
              success = FALSE;
              break;
            case VUT_AGE:
              if (preq->present) {
                notify_player(pplayer, city_tile(pcity),
                              E_CITY_CANTBUILD, ftc_server,
                              _("%s can't build %s from the worklist; "
                                "only available once %d turns old. Postponing..."),
                              city_link(pcity),
                              city_improvement_name_translation(pcity, ptarget),
                              preq->source.value.age);
                script_server_signal_emit("building_cant_be_built", ptarget,
                                          pcity, "need_age");
              } else {
                /* Can't go back in time. */
                success = FALSE;
              }
              break;
            case VUT_NONE:
            case VUT_COUNT:
              fc_assert_ret_val_msg(FALSE, TRUE,
                                    "worklist_change_build_target() "
                                    "called with invalid preq");
            break;
            /* No default handling here, as we want compiler warning
             * if new requirement type is added to enum and it's not handled
             * here. */
            };
	    break;
	  }

          /* Almost all cases emit signal in the end, so city check needed. */
          if (!city_exist(saved_id)) {
            /* Some script has removed city */
            return FALSE;
          }
          city_checked = TRUE;

	} requirement_vector_iterate_end;

	if (!known) {
	  /* This shouldn't happen...
	     FIXME: make can_city_build_improvement_now() return a reason enum. */
          notify_player(pplayer, city_tile(pcity),
                        E_CITY_CANTBUILD, ftc_server,
                        _("%s can't build %s from the worklist; "
                          "reason unknown! Postponing..."),
                        city_link(pcity),
                        city_improvement_name_translation(pcity, ptarget));
	}
      } else if (success) {
	/* Hey, we can upgrade the improvement! */
        notify_player(pplayer, city_tile(pcity), E_WORKLIST, ftc_server,
                      _("Production of %s is upgraded to %s in %s."),
                      city_improvement_name_translation(pcity, ptarget), 
                      city_improvement_name_translation(pcity, pupdate),
                      city_link(pcity));
	target.value.building = pupdate;
	success = TRUE;
      }

      if (!success) {
	/* Never in a million years. */
        notify_player(pplayer, city_tile(pcity),
                      E_CITY_CANTBUILD, ftc_server,
                      _("%s can't build %s from the worklist. Purging..."),
                      city_link(pcity),
                      city_improvement_name_translation(pcity, ptarget));
        script_server_signal_emit("building_cant_be_built", ptarget, pcity,
                                  "never");
        if (city_exist(saved_id)) {
          city_checked = TRUE;
          /* Purge this worklist item. */
          i--;
          worklist_remove(pwl, i);
        } else {
          city_checked = FALSE;
        }
      }
      break;
    }
    default:
      /* skip useless target */
      log_error("worklist_change_build_target() has unrecognized "
                "target kind (%d)", target.kind);
      break;
    };
  } /* while */

  if (success) {
    /* All okay.  Switch targets. */
    change_build_target(pplayer, pcity, &target, E_WORKLIST);

    /* i is the index immediately _after_ the item we're changing to.
       Remove the (i-1)th item from the worklist. */
    worklist_remove(pwl, i - 1);
  }

  if (worklist_is_empty(pwl)) {
    /* There *was* something in the worklist, but it's empty now.  Bug the
       player about it. */
    notify_player(pplayer, city_tile(pcity), E_WORKLIST, ftc_server,
		  /* TRANS: The <city> worklist .... */
		  _("The %s worklist is now empty."),
		  city_link(pcity));
  }

  return success;
}

/**********************************************************************//**
  Assuming we just finished building something, find something new to
  build.  The policy is: use the worklist if we can; if not, try not
  changing; if we must change, get desparate and use the AI advisor.
**************************************************************************/
void choose_build_target(struct player *pplayer, struct city *pcity)
{
  /* Pick the next thing off the worklist. */
  if (worklist_change_build_target(pplayer, pcity)) {
    return;
  }

  /* Try building the same thing again.  Repeat building doesn't require a
   * call to change_build_target, so just return. */
  switch (pcity->production.kind) {
  case VUT_UTYPE:
    /* We can build a unit again unless it's unique or we have lost the tech. */
    if (!utype_has_flag(pcity->production.value.utype, UTYF_UNIQUE)
        && can_city_build_unit_now(pcity, pcity->production.value.utype)) {
      log_base(LOG_BUILD_TARGET, "%s repeats building %s", city_name_get(pcity),
               utype_rule_name(pcity->production.value.utype));
      return;
    }
    break;
  case VUT_IMPROVEMENT:
    if (can_city_build_improvement_now(pcity, pcity->production.value.building)) {
      /* We can build space and coinage again, and possibly others. */
      log_base(LOG_BUILD_TARGET, "%s repeats building %s", city_name_get(pcity),
               improvement_rule_name(pcity->production.value.building));
      return;
    }
    break;
  default:
    /* fallthru */
    break;
  };

  /* Find *something* to do! */
  log_debug("Trying advisor_choose_build.");
  advisor_choose_build(pplayer, pcity);
  log_debug("Advisor_choose_build didn't kill us.");
}

/**********************************************************************//**
  Follow the list of replacement buildings until we hit something that
  we can build.  Returns NULL if we can't upgrade at all (including if the
  original building is unbuildable).
**************************************************************************/
static struct impr_type *building_upgrades_to(struct city *pcity,
					      struct impr_type *pimprove)
{
  struct impr_type *check = pimprove;
  struct impr_type *best_upgrade = NULL;

  if (!can_city_build_improvement_direct(pcity, check)) {
    return NULL;
  }
  while (valid_improvement(check = improvement_replacement(check))) {
    if (can_city_build_improvement_direct(pcity, check)) {
      best_upgrade = check;
    }
  }

  return best_upgrade;
}

/**********************************************************************//**
  Try to upgrade production in pcity.
**************************************************************************/
static void upgrade_building_prod(struct city *pcity)
{
  struct impr_type *producing = pcity->production.value.building;
  struct impr_type *upgrading = building_upgrades_to(pcity, producing);

  if (upgrading && can_city_build_improvement_now(pcity, upgrading)) {
    notify_player(city_owner(pcity), city_tile(pcity),
                  E_UNIT_UPGRADED, ftc_server,
                  _("Production of %s is upgraded to %s in %s."),
                  improvement_name_translation(producing),
                  improvement_name_translation(upgrading),
                  city_link(pcity));
    pcity->production.kind = VUT_IMPROVEMENT;
    pcity->production.value.building = upgrading;
  }
}

/**********************************************************************//**
  Follow the list of obsoleted_by units until we hit something that
  we can build.  Return NULL when we can't upgrade at all.  NB:  returning
  something doesn't guarantee that pcity really _can_ build it; just that
  pcity can't build whatever _obsoletes_ it.

  FIXME: this function is a duplicate of can_upgrade_unittype.
**************************************************************************/
static struct unit_type *unit_upgrades_to(struct city *pcity,
					  struct unit_type *punittype)
{
  struct unit_type *check = punittype;
  struct unit_type *best_upgrade = U_NOT_OBSOLETED;

  if (!can_city_build_unit_direct(pcity, punittype)) {
    return U_NOT_OBSOLETED;
  }
  while ((check = check->obsoleted_by) != U_NOT_OBSOLETED) {
    if (can_city_build_unit_direct(pcity, check)) {
      best_upgrade = check;
    }
  }

  return best_upgrade;
}

/**********************************************************************//**
  Try to upgrade production in pcity.
**************************************************************************/
static void upgrade_unit_prod(struct city *pcity)
{
  struct unit_type *producing = pcity->production.value.utype;
  struct unit_type *upgrading = unit_upgrades_to(pcity, producing);

  if (upgrading && can_city_build_unit_direct(pcity, upgrading)) {
    notify_player(city_owner(pcity), city_tile(pcity),
                  E_UNIT_UPGRADED, ftc_server,
		  _("Production of %s is upgraded to %s in %s."),
		  utype_name_translation(producing),
		  utype_name_translation(upgrading), 
		  city_link(pcity));
    pcity->production.value.utype = upgrading;
  }
}

/**********************************************************************//**
  Disband units if we don't have enough shields to support them.  Returns
  FALSE if the _city_ is disbanded as a result.
**************************************************************************/
static bool city_distribute_surplus_shields(struct player *pplayer,
					    struct city *pcity)
{
  if (pcity->surplus[O_SHIELD] < 0) {
    unit_list_iterate_safe(pcity->units_supported, punit) {
      if (utype_upkeep_cost(unit_type_get(punit), pplayer, O_SHIELD) > 0
          && pcity->surplus[O_SHIELD] < 0) {
        const char *punit_link = unit_link(punit);

        /* TODO: Should the unit try to help cities on adjacent tiles? That
         * would be a rules change. (This action is performed by the game
         * it self) */
        if (upkeep_kill_unit(punit, O_SHIELD, ULR_DISBANDED,
                             game.info.muuk_shield_wipe)) {
          notify_player(pplayer, city_tile(pcity),
                        E_UNIT_LOST_MISC, ftc_server,
                        _("%s can't upkeep %s, unit disbanded."),
                        city_link(pcity), punit_link);
        }

	/* pcity->surplus[O_SHIELD] is automatically updated. */
      }
    } unit_list_iterate_safe_end;
  }

  if (pcity->surplus[O_SHIELD] < 0) {
    /* Special case: MissingXProtected. This nasty unit won't go so easily.
     * It'd rather make the citizens pay in blood for their failure to upkeep
     * it! If we make it here all normal units are already disbanded, so only
     * undisbandable ones remain. */
    unit_list_iterate_safe(pcity->units_supported, punit) {
      int upkeep = utype_upkeep_cost(unit_type_get(punit), pplayer, O_SHIELD);

      if (upkeep > 0 && pcity->surplus[O_SHIELD] < 0) {
        notify_player(pplayer, city_tile(pcity),
                      E_UNIT_LOST_MISC, ftc_server,
                      _("Citizens in %s perish for their failure to "
                        "upkeep %s!"),
                      city_link(pcity), unit_link(punit));
	if (!city_reduce_size(pcity, 1, NULL, "upkeep_failure")) {
	  return FALSE;
	}

	/* No upkeep for the unit this turn. */
	pcity->surplus[O_SHIELD] += upkeep;
      }
    } unit_list_iterate_safe_end;
  }

  /* Now we confirm changes made last turn. */
  pcity->shield_stock += pcity->surplus[O_SHIELD];
  pcity->before_change_shields = pcity->shield_stock;
  pcity->last_turns_shield_surplus = pcity->surplus[O_SHIELD];

  return TRUE;
}

/**********************************************************************//**
  Returns FALSE when the city is removed, TRUE otherwise.
**************************************************************************/
static bool city_build_building(struct player *pplayer, struct city *pcity)
{
  bool space_part;
  int mod;
  struct impr_type *pimprove = pcity->production.value.building;
  int saved_id = pcity->id;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_assert(pcity->surplus[O_SHIELD] >= 0);
    /* pcity->before_change_shields already contains the surplus from
     * this turn. */
    pplayer->economic.gold += pcity->before_change_shields;
    pcity->before_change_shields = 0;
    pcity->shield_stock = 0;
    choose_build_target(pplayer, pcity);
    return TRUE;
  }
  upgrade_building_prod(pcity);

  if (!can_city_build_improvement_now(pcity, pimprove)) {
    notify_player(pplayer, city_tile(pcity), E_CITY_CANTBUILD, ftc_server,
                  _("%s is building %s, which is no longer available."),
                  city_link(pcity),
                  city_improvement_name_translation(pcity, pimprove));
    script_server_signal_emit("building_cant_be_built", pimprove, pcity,
                              "unavailable");
    return TRUE;
  }
  if (pcity->shield_stock >= impr_build_shield_cost(pcity, pimprove)) {
    int cost;

    if (is_small_wonder(pimprove)) {
      city_list_iterate(pplayer->cities, wcity) {
	if (city_has_building(wcity, pimprove)) {
	  city_remove_improvement(wcity, pimprove);
	  break;
	}
      } city_list_iterate_end;
    }

    space_part = TRUE;
    if (get_current_construction_bonus(pcity, EFT_SS_STRUCTURAL,
                                       RPT_CERTAIN) > 0) {
      pplayer->spaceship.structurals++;
    } else if (get_current_construction_bonus(pcity, EFT_SS_COMPONENT,
                                              RPT_CERTAIN) > 0) {
      pplayer->spaceship.components++;
    } else if (get_current_construction_bonus(pcity, EFT_SS_MODULE,
                                              RPT_CERTAIN) > 0) {
      pplayer->spaceship.modules++;
    } else {
      space_part = FALSE;
      city_add_improvement(pcity, pimprove);
    }
    cost = impr_build_shield_cost(pcity, pimprove);
    pcity->before_change_shields -= cost;
    pcity->shield_stock -= cost;
    pcity->turn_last_built = game.info.turn;
    /* to eliminate micromanagement */
    if (is_great_wonder(pimprove)) {
      notify_player(NULL, city_tile(pcity), E_WONDER_BUILD, ftc_server,
                    _("The %s have finished building %s in %s."),
                    nation_plural_for_player(pplayer),
                    city_improvement_name_translation(pcity, pimprove),
                    city_link(pcity));
    }

    notify_player(pplayer, city_tile(pcity), E_IMP_BUILD, ftc_server,
                  _("%s has finished building %s."),
                  city_link(pcity), improvement_name_translation(pimprove));
    script_server_signal_emit("building_built", pimprove, pcity);

    if (!city_exist(saved_id)) {
      /* Script removed city */
      return FALSE;
    }

    /* Call this function since some buildings may change the
     * the vision range of a city */
    city_refresh_vision(pcity);

    if ((mod = get_current_construction_bonus(pcity, EFT_GIVE_IMM_TECH,
                                              RPT_CERTAIN))) {
      struct research *presearch = research_get(pplayer);
      char research_name[MAX_LEN_NAME * 2];
      int i;
      const char *provider = improvement_name_translation(pimprove);

      notify_research(presearch, NULL, E_TECH_GAIN, ftc_server,
                      PL_("%s boosts research; you gain %d immediate "
                          "advance.",
                          "%s boosts research; you gain %d immediate "
                          "advances.",
                          mod), provider, mod);

      research_pretty_name(presearch, research_name, sizeof(research_name));
      for (i = 0; i < mod; i++) {
        Tech_type_id tech = pick_free_tech(presearch);
        const char *adv_name = research_advance_name_translation(presearch, tech);

        give_immediate_free_tech(presearch, tech);
        notify_research(presearch, NULL, E_TECH_GAIN, ftc_server,
                        /* TRANS: Tech from building (Darwin's Voyage) */
                        Q_("?frombldg:Acquired %s from %s."), adv_name,
                        provider);

        notify_research_embassies(presearch, NULL, E_TECH_EMBASSY, ftc_server,
                                  /* TRANS: Tech from building (Darwin's
                                   * Voyage) */
                                  Q_("?frombldg:The %s have acquired %s "
                                     "from %s."),
                                  research_name, adv_name, provider);
      }
    }
    if (space_part && pplayer->spaceship.state == SSHIP_NONE) {
      notify_player(NULL, city_tile(pcity), E_SPACESHIP, ftc_server,
                    _("The %s have started building a spaceship!"),
                    nation_plural_for_player(pplayer));
      pplayer->spaceship.state = SSHIP_STARTED;
    }
    if (space_part) {
      /* space ship part build */
      send_spaceship_info(pplayer, NULL);
    } else {
      /* Update city data. */
      if (city_refresh(pcity)) {
        auto_arrange_workers(pcity);
      }
    }

    /* Move to the next thing in the worklist */
    choose_build_target(pplayer, pcity);
  }

  return TRUE;
}

/**********************************************************************//**
  Build city units. Several units can be built in one turn if the effect
  City_Build_Slots is used.
**************************************************************************/
static bool city_build_unit(struct player *pplayer, struct city *pcity)
{
  struct unit_type *utype;
  struct worklist *pwl = &pcity->worklist;;
  int unit_shield_cost, num_units, i;

  fc_assert_ret_val(pcity->production.kind == VUT_UTYPE, FALSE);

  /* If the city has already bought a unit which is now obsolete, don't try
   * to upgrade the production. The new unit might require more shields, which
   * would be bad if it was bought to urgently defend a city. (Equally it
   * might be the same cost or cheaper, but tough; you hurried the unit so
   * you miss out on technological advances.) */
  if (city_can_change_build(pcity)) {
    upgrade_unit_prod(pcity);
  }

  utype = pcity->production.value.utype;
  unit_shield_cost = utype_build_shield_cost(pcity, utype);

  /* We must make a special case for barbarians here, because they are
     so dumb. Really. They don't know the prerequisite techs for units
     they build!! - Per */
  if (!can_city_build_unit_direct(pcity, utype)
      && !is_barbarian(pplayer)) {
    notify_player(pplayer, city_tile(pcity), E_CITY_CANTBUILD, ftc_server,
                  _("%s is building %s, which is no longer available."),
                  city_link(pcity), utype_name_translation(utype));

    /* Log before signal emitting, so pointers are certainly valid */
    log_verbose("%s %s tried to build %s, which is not available.",
                nation_rule_name(nation_of_city(pcity)),
                city_name_get(pcity), utype_rule_name(utype));
    script_server_signal_emit("unit_cant_be_built", utype, pcity,
                              "unavailable");
    return TRUE;
  }

  if (pcity->shield_stock >= unit_shield_cost) {
    int pop_cost = utype_pop_value(utype);
    struct unit *punit;
    int saved_city_id = pcity->id;

    /* Should we disband the city? -- Massimo */
    if (city_size_get(pcity) == pop_cost
	&& is_city_option_set(pcity, CITYO_DISBAND)) {
      return !disband_city(pcity);
    }

    if (city_size_get(pcity) <= pop_cost) {
      notify_player(pplayer, city_tile(pcity), E_CITY_CANTBUILD, ftc_server,
                    /* TRANS: city ... utype ... size ... pop_cost */
                    _("%s can't build %s yet. "
                      "(city size: %d, unit population cost: %d)"),
                    city_link(pcity), utype_name_translation(utype),
                    city_size_get(pcity), pop_cost);
      script_server_signal_emit("unit_cant_be_built", utype, pcity,
                                "pop_cost");
      return TRUE;
    }

    fc_assert(pop_cost == 0 || city_size_get(pcity) >= pop_cost);

    /* don't update turn_last_built if we returned above */
    pcity->turn_last_built = game.info.turn;

    /* check if we can build more than one unit (effect City_Build_Slots) */
    (void) city_production_build_units(pcity, FALSE, &num_units);

    /* We should be able to build at least one (by checks above) */
    fc_assert(num_units >= 1);

    for (i = 0; i < num_units; i++) {
      punit = create_unit(pplayer, pcity->tile, utype,
                          do_make_unit_veteran(pcity, utype),
                          pcity->id, 0);
      pplayer->score.units_built++;

      /* After we created the unit remove the citizen. This will also
       * rearrange the worker to take into account the extra resources
       * (food) needed. */
      if (pop_cost > 0) {
        city_reduce_size(pcity, pop_cost, NULL, "unit_built");
      }

      /* to eliminate micromanagement, we only subtract the unit's cost */
      pcity->before_change_shields -= unit_shield_cost;
      pcity->shield_stock -= unit_shield_cost;

      notify_player(pplayer, city_tile(pcity), E_UNIT_BUILT, ftc_server,
                    /* TRANS: <city> is finished building <unit/building>. */
                    _("%s is finished building %s."),
                    city_link(pcity), utype_name_translation(utype));

      if (pop_cost > 0) {
        /* Additional message if the unit has population cost. */
        notify_player(pplayer, city_tile(pcity), E_UNIT_BUILT_POP_COST,
                      ftc_server,
                      /* TRANS: "<unit> cost... <city> shrinks..."
                       * Plural in "%d population", not "size %d". */
                      PL_("%s cost %d population. %s shrinks to size %d.",
                          "%s cost %d population. %s shrinks to size %d.",
                          pop_cost),
                      utype_name_translation(utype), pop_cost,
                      city_link(pcity), city_size_get(pcity));
      }

      script_server_signal_emit("unit_built", punit, pcity);

      /* check if the city still exists */
      if (!city_exist(saved_city_id)) {
        break;
      }

      if (i != 0 && worklist_length(pwl) > 0) {
        /* remove the build unit from the worklist; it has to be one less
         * than units build to preserve the next build target from the
         * worklist */
        worklist_remove(pwl, 0);
      }
    }

    if (city_exist(saved_city_id)) {
      /* Done building this unit; time to move on to the next. */
      choose_build_target(pplayer, pcity);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Returns FALSE when the city is removed, TRUE otherwise.
**************************************************************************/
static bool city_build_stuff(struct player *pplayer, struct city *pcity)
{
  if (!city_distribute_surplus_shields(pplayer, pcity)) {
    return FALSE;
  }

  nullify_caravan_and_disband_plus(pcity);
  define_orig_production_values(pcity);

  switch (pcity->production.kind) {
  case VUT_IMPROVEMENT:
    return city_build_building(pplayer, pcity);
  case VUT_UTYPE:
    return city_build_unit(pplayer, pcity);
  default:
    /* must never happen! */
    fc_assert(FALSE);
    break;
  };
  return FALSE;
}

/**********************************************************************//**
  Randomly sell a building from the given list. Returns TRUE if a building
  was sold.

  NB: It is assumed that gold upkeep for the buildings has already been
  paid this turn, hence when a building is sold its upkeep is given back
  to the player.
  NB: The contents of 'imprs' are usually mangled by this function.
  NB: It is assumed that all buildings in 'imprs' can be sold.
**************************************************************************/
static bool sell_random_building(struct player *pplayer,
                                 struct cityimpr_list *imprs)
{
  struct cityimpr *pcityimpr;
  int r;

  fc_assert_ret_val(pplayer != NULL, FALSE);

  if (!imprs || cityimpr_list_size(imprs) == 0) {
    return FALSE;
  }

  r = fc_rand(cityimpr_list_size(imprs));
  pcityimpr = cityimpr_list_get(imprs, r);

  notify_player(pplayer, city_tile(pcityimpr->pcity), E_IMP_AUCTIONED,
                ftc_server,
                _("Can't afford to maintain %s in %s, building sold!"),
                improvement_name_translation(pcityimpr->pimprove),
                city_link(pcityimpr->pcity));
  log_debug("%s: sold building (%s)", player_name(pplayer),
            improvement_name_translation(pcityimpr->pimprove));

  do_sell_building(pplayer, pcityimpr->pcity, pcityimpr->pimprove, "cant_maintain");

  cityimpr_list_remove(imprs, pcityimpr);

  /* Get back the gold upkeep that was already paid this turn. */
  pplayer->economic.gold += city_improvement_upkeep(pcityimpr->pcity,
                                                    pcityimpr->pimprove);

  city_refresh_queue_add(pcityimpr->pcity);

  FC_FREE(pcityimpr);

  return TRUE;
}

/**********************************************************************//**
  Call back for when a unit in uk_rem_gold dies.

  A unit can die as a side effect of an action another unit in the list is
  forced to perform. This isn't limited to "Explode Nuclear". A Lua call
  back for another action could cause more collateral damage than "Explode
  Nuclear".
**************************************************************************/
static void uk_rem_gold_callback(struct unit *punit)
{
  int gold_upkeep;

  /* Remove the unit from uk_rem_gold. */
  unit_list_remove(uk_rem_gold, punit);

  gold_upkeep = punit->server.upkeep_payed[O_GOLD];

  /* All units in uk_rem_gold should have gold upkeep! */
  fc_assert_ret_msg(gold_upkeep > 0, "%s has %d gold upkeep",
                    unit_rule_name(punit), gold_upkeep);

  /* Get the upkeep gold back. */
  unit_owner(punit)->economic.gold += gold_upkeep;
}

/**********************************************************************//**
  Add a unit to uk_rem_gold and make the unit remove it self from it if
  it dies before it is processed.
**************************************************************************/
static void uk_rem_gold_append(struct unit *punit)
{
  /* Make the unit aware that it is on the uk_rem_gold list. */
  unit_set_removal_callback(punit, uk_rem_gold_callback);

  /* Add the unit to the list. */
  unit_list_append(uk_rem_gold, punit);
}

/**********************************************************************//**
  Destroy a unit list and make the units it contains aware that it no
  longer refers to them.
**************************************************************************/
static void unit_list_referred_destroy(struct unit_list *punitlist)
{
  unit_list_iterate(punitlist, punit) {
    /* Clear the unit's knowledge of the list. */
    unit_unset_removal_callback(punit);
  } unit_list_iterate_end;

  /* Destroy the list it self. */
  unit_list_destroy(punitlist);
}

/**********************************************************************//**
  Randomly "sell" a unit from the given list. Returns pointer to sold unit.
  This pointer is not valid any more, but can be removed from the lists.

  NB: It is assumed that gold upkeep for the units has already been paid
  this turn, hence when a unit is "sold" its upkeep is given back to the
  player.
  NB: The contents of 'units' are usually mangled by this function.
  NB: It is assumed that all units in 'units' have positive gold upkeep.
**************************************************************************/
static struct unit *sell_random_unit(struct player *pplayer,
                                     struct unit_list *punitlist)
{
  struct unit *punit;
  int r;
  struct unit_list *cargo;

  fc_assert_ret_val(pplayer != NULL, FALSE);

  if (!punitlist || unit_list_size(punitlist) == 0) {
    return FALSE;
  }

  r = fc_rand(unit_list_size(punitlist));
  punit = unit_list_get(punitlist, r);

  cargo = unit_list_new();

  /* Check if unit is transporting other units from punitlist,
   * and sell one of those (recursively) instead. */
  unit_list_iterate(unit_transport_cargo(punit), pcargo) {
    /* Optimization, do not iterate over punitlist
     * if we are sure that pcargo is not in it. */
    if (pcargo->server.upkeep_payed[O_GOLD] > 0) {
      unit_list_iterate(punitlist, p2) {
        if (pcargo == p2) {
          unit_list_append(cargo, pcargo);
        }
      } unit_list_iterate_end;
    }
  } unit_list_iterate_end;

  if (unit_list_size(cargo) > 0) {
    struct unit *ret = sell_random_unit(pplayer, cargo);

    unit_list_destroy(cargo);

    return ret;
  }

  unit_list_destroy(cargo);

  {
    const char *punit_link = unit_tile_link(punit);
#ifdef FREECIV_DEBUG
    const char *punit_logname = unit_name_translation(punit);
#endif /* FREECIV_DEBUG */
    struct tile *utile = unit_tile(punit);

    if (upkeep_kill_unit(punit, O_GOLD, ULR_SOLD,
                         game.info.muuk_gold_wipe)) {
      unit_list_remove(punitlist, punit);

      /* The gold was payed back when the unit removal made
       * uk_rem_gold_callback() run as the unit's removal call back. */

      notify_player(pplayer, utile, E_UNIT_LOST_MISC, ftc_server,
                    _("Not enough gold. %s disbanded."),
                    punit_link);
      log_debug("%s: unit sold (%s)", player_name(pplayer),
                punit_logname);
    } else {
      /* Not able to get rid of punit */
      return NULL;
    }
  }

  return punit;
}

/**********************************************************************//**
  Balance the gold of a nation by selling some random units and buildings.
**************************************************************************/
static bool player_balance_treasury_units_and_buildings
            (struct player *pplayer)
{
  struct cityimpr_list *pimprlist;
  bool sell_unit = TRUE;

  if (!pplayer) {
    return FALSE;
  }

  pimprlist = cityimpr_list_new();
  uk_rem_gold = unit_list_new();

  city_list_iterate(pplayer->cities, pcity) {
    city_built_iterate(pcity, pimprove) {
      if (can_city_sell_building(pcity, pimprove)) {
        struct cityimpr *ci = fc_malloc(sizeof(*ci));

        ci->pcity = pcity;
        ci->pimprove = pimprove;
        cityimpr_list_append(pimprlist, ci);
      }
    } city_built_iterate_end;

    unit_list_iterate(pcity->units_supported, punit) {
      if (punit->server.upkeep_payed[O_GOLD] > 0) {
        uk_rem_gold_append(punit);
      }
    } unit_list_iterate_end;
  } city_list_iterate_end;

  while (pplayer->economic.gold < 0
         && (cityimpr_list_size(pimprlist) > 0
             || unit_list_size(uk_rem_gold) > 0)) {
    if ((!sell_unit && cityimpr_list_size(pimprlist) > 0)
        || unit_list_size(uk_rem_gold) == 0) {
      sell_random_building(pplayer, pimprlist);
    } else {
      sell_random_unit(pplayer, uk_rem_gold);
    }
    sell_unit = !sell_unit;
  }

  /* Free remaining entries from list */
  cityimpr_list_iterate(pimprlist, pimpr) {
    FC_FREE(pimpr);
  } cityimpr_list_iterate_end;

  if (pplayer->economic.gold < 0) {
    /* If we get here it means the player has
     * negative gold. This should never happen. */
    fc_assert_msg(FALSE, "Player %s (nb %d) cannot have negative gold!",
                  player_name(pplayer), player_number(pplayer));
  }

  cityimpr_list_destroy(pimprlist);
  unit_list_referred_destroy(uk_rem_gold);

  return pplayer->economic.gold >= 0;
}

/**********************************************************************//**
  Balance the gold of a nation by selling some units which need gold upkeep.
**************************************************************************/
static bool player_balance_treasury_units(struct player *pplayer)
{
  if (!pplayer) {
    return FALSE;
  }

  uk_rem_gold = unit_list_new();

  city_list_iterate(pplayer->cities, pcity) {
    unit_list_iterate(pcity->units_supported, punit) {
      if (punit->server.upkeep_payed[O_GOLD] > 0) {
        uk_rem_gold_append(punit);
      }
    } unit_list_iterate_end;
  } city_list_iterate_end;

  while (pplayer->economic.gold < 0
         && sell_random_unit(pplayer, uk_rem_gold)) {
    /* all done in sell_random_unit() */
  }

  if (pplayer->economic.gold < 0) {
    /* If we get here it means the player has
     * negative gold. This should never happen. */
    fc_assert_msg(FALSE, "Player %s (nb %d) cannot have negative gold!",
                  player_name(pplayer), player_number(pplayer));
  }

  unit_list_referred_destroy(uk_rem_gold);

  return pplayer->economic.gold >= 0;
}

/**********************************************************************//**
  Balance the gold of one city by randomly selling some buildings.
**************************************************************************/
static bool city_balance_treasury_buildings(struct city *pcity)
{
  struct player *pplayer;
  struct cityimpr_list *pimprlist;

  if (!pcity) {
    return TRUE;
  }

  pplayer = city_owner(pcity);
  pimprlist = cityimpr_list_new();

  /* Create a vector of all buildings that can be sold. */
  city_built_iterate(pcity, pimprove) {
    if (can_city_sell_building(pcity, pimprove)) {
      struct cityimpr *ci = fc_malloc(sizeof(*ci));

      ci->pcity = pcity;
      ci->pimprove = pimprove;
      cityimpr_list_append(pimprlist, ci);
    }
  } city_built_iterate_end;

  /* Try to sell some buildings. */
  while (pplayer->economic.gold < 0
         && sell_random_building(pplayer, pimprlist)) {
    /* all done in sell_random_building */
  }

  /* Free remaining entries from list */
  cityimpr_list_iterate(pimprlist, pimpr) {
    FC_FREE(pimpr);
  } cityimpr_list_iterate_end;

  cityimpr_list_destroy(pimprlist);

  return pplayer->economic.gold >= 0;
}

/**********************************************************************//**
  Balance the gold of one city by randomly selling some units which need
  gold upkeep.

  NB: This function adds the gold upkeep of disbanded units back to the
  player's gold. Hence it assumes that this gold was previously taken
  from the player (i.e. in update_city_activity()).
**************************************************************************/
static bool city_balance_treasury_units(struct city *pcity)
{
  struct player *pplayer;

  if (!pcity) {
    return TRUE;
  }

  pplayer = city_owner(pcity);
  uk_rem_gold = unit_list_new();

  /* Create a vector of all supported units with gold upkeep. */
  unit_list_iterate(pcity->units_supported, punit) {
    if (punit->server.upkeep_payed[O_GOLD] > 0) {
       uk_rem_gold_append(punit);
     }
  } unit_list_iterate_end;

  /* Still not enough gold, so try "selling" some units. */
  while (pplayer->economic.gold < 0
         && sell_random_unit(pplayer, uk_rem_gold)) {
    /* all done in sell_random_unit() */
  }

  /* If we get here the player has negative gold, but hopefully
   * another city will be able to pay the deficit, so continue. */

  unit_list_referred_destroy(uk_rem_gold);

  return pplayer->economic.gold >= 0;
}

/**********************************************************************//**
 Add some Pollution if we have waste
**************************************************************************/
static bool place_pollution(struct city *pcity, enum extra_cause cause)
{
  struct tile *ptile;
  struct tile *pcenter = city_tile(pcity);
  int city_radius_sq = city_map_radius_sq_get(pcity);
  int k = 100;

  while (k > 0) {
    /* place pollution on a random city tile */
    int cx, cy;
    int tile_id = fc_rand(city_map_tiles(city_radius_sq));
    struct extra_type *pextra;

    city_tile_index_to_xy(&cx, &cy, tile_id, city_radius_sq);

    /* check for a a real map position */
    if (!(ptile = city_map_to_tile(pcenter, city_radius_sq, cx, cy))) {
      continue;
    }

    pextra = rand_extra_for_tile(ptile, cause, FALSE);

    if (pextra != NULL && !tile_has_extra(ptile, pextra)) {
      tile_add_extra(ptile, pextra);
      update_tile_knowledge(ptile);

      return TRUE;
    }
    k--;
  }
  log_debug("pollution not placed: city: %s", city_name_get(pcity));

  return FALSE;
}

/**********************************************************************//**
 Add some Pollution if we have waste
**************************************************************************/
static void check_pollution(struct city *pcity)
{
  if (fc_rand(100) < pcity->pollution) {
    if (place_pollution(pcity, EC_POLLUTION)) {
      notify_player(city_owner(pcity), city_tile(pcity), E_POLLUTION, ftc_server,
                    _("Pollution near %s."), city_link(pcity));
    }
  }
}

/**********************************************************************//**
  Returns the cost to incite a city. This depends on the size of the city,
  the number of happy, unhappy and angry citizens, whether it is
  celebrating, how close it is to the capital, how many units it has and
  upkeeps, presence of courthouse, its buildings and wonders, and who
  originally built it.
**************************************************************************/
int city_incite_cost(struct player *pplayer, struct city *pcity)
{
  struct city *capital;
  int dist, size;
  double cost; /* Intermediate values can get very large */

  /* Gold factor */
  cost = city_owner(pcity)->economic.gold + game.server.base_incite_cost;

  unit_list_iterate(pcity->tile->units, punit) {
    cost += (unit_build_shield_cost(pcity, punit)
	     * game.server.incite_unit_factor);
  } unit_list_iterate_end;

  /* Buildings */
  city_built_iterate(pcity, pimprove) {
    cost += impr_build_shield_cost(pcity, pimprove)
            * game.server.incite_improvement_factor;
  } city_built_iterate_end;

  /* Stability bonuses */
  if (!city_unhappy(pcity)) {
    cost *= 2;
  }
  if (city_celebrating(pcity)) {
    cost *= 2;
  }

  /* Buy back is cheap, conquered cities are also cheap */
  if (!game.info.citizen_nationality) {
    if (city_owner(pcity) != pcity->original) {
      if (pplayer == pcity->original) {
        cost /= 2;            /* buy back: 50% price reduction */
      } else {
        cost = cost * 2 / 3;  /* buy conquered: 33% price reduction */
      }
    }
  }

  /* Distance from capital */
  capital = player_capital(city_owner(pcity));
  if (capital) {
    int tmp = map_distance(capital->tile, pcity->tile);
    dist = MIN(32, tmp);
  } else {
    /* No capital? Take max penalty! */
    dist = 32;
  }

  size = MAX(1, city_size_get(pcity)
                + pcity->feel[CITIZEN_HAPPY][FEELING_FINAL]
                - pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]
                - pcity->feel[CITIZEN_ANGRY][FEELING_FINAL] * 3);
  cost *= size;
  cost *= game.server.incite_total_factor;
  cost = cost / (dist + 3);

  if (game.info.citizen_nationality) {
    int cost_per_citizen = cost / pcity->size;
    int natives = citizens_nation_get(pcity, city_owner(pcity)->slot);
    int tgt_cit = citizens_nation_get(pcity, pplayer->slot);
    int third_party = pcity->size - natives - tgt_cit;

    cost = cost_per_citizen * (natives + 0.7 * third_party + 0.5 * tgt_cit); 
  }

  cost += (cost * get_city_bonus(pcity, EFT_INCITE_COST_PCT)) / 100;
  cost /= 100;

  if (cost >= INCITE_IMPOSSIBLE_COST) {
    return INCITE_IMPOSSIBLE_COST;
  } else {
    return cost;
  }
}

/**********************************************************************//**
  Called every turn, at beginning of turn, for every city.
**************************************************************************/
static void define_orig_production_values(struct city *pcity)
{
  /* Remember what this city is building last turn, so that on the next turn
   * the player can switch production to something else and then change it
   * back without penalty.  This has to be updated _before_ production for
   * this turn is calculated, so that the penalty will apply if the player
   * changes production away from what has just been completed.  This makes
   * sense if you consider what this value means: all the shields in the
   * city have been dedicated toward the project that was chosen last turn,
   * so the player shouldn't be penalized if the governor has to pick
   * something different.  See city_change_production_penalty(). */
  pcity->changed_from = pcity->production;

  log_debug("In %s, building %s.  Beg of Turn shields = %d",
            city_name_get(pcity), universal_rule_name(&pcity->changed_from),
            pcity->before_change_shields);
}

/**********************************************************************//**
  Let the advisor set up city building target.
**************************************************************************/
static void nullify_caravan_and_disband_plus(struct city *pcity)
{
  pcity->disbanded_shields=0;
  pcity->caravan_shields=0;
}

/**********************************************************************//**
  Initialize all variables containing information about production
  before it was changed.
**************************************************************************/
void nullify_prechange_production(struct city *pcity)
{
  nullify_caravan_and_disband_plus(pcity);
  pcity->before_change_shields=0;
}

/**********************************************************************//**
  Called every turn, at end of turn, for every city.
**************************************************************************/
static void update_city_activity(struct city *pcity)
{
  struct player *pplayer;
  struct government *gov;
  bool is_happy;
  bool is_celebrating;

  if (!pcity) {
    return;
  }

  pplayer = city_owner(pcity);
  gov = government_of_city(pcity);
  is_happy = city_happy(pcity);
  is_celebrating = city_celebrating(pcity);

  if (city_refresh(pcity)) {
    auto_arrange_workers(pcity);
  }

  /* Reporting of celebrations rewritten, copying the treatment of disorder below,
     with the added rapture rounds count.  991219 -- Jing */
  if (city_build_stuff(pplayer, pcity)) {
    int saved_id;
    int revolution_turns;

    pcity->history += city_history_gain(pcity);

    /* History can decrease, but never go below zero */
    pcity->history = MAX(pcity->history, 0);

    /* Keep old behaviour when building new improvement could keep
       city celebrating */
    if (is_happy == FALSE) {
      is_happy = city_happy(pcity);
    }

    if (city_celebrating(pcity) || is_celebrating) {
      pcity->rapture++;
      if (pcity->rapture == 1) {
        notify_player(pplayer, city_tile(pcity), E_CITY_LOVE, ftc_server,
                      _("Celebrations in your honor in %s."),
                      city_link(pcity));
      }
    } else {
      if (pcity->rapture != 0) {
        notify_player(pplayer, city_tile(pcity), E_CITY_NORMAL, ftc_server,
                      _("Celebrations canceled in %s."),
                      city_link(pcity));
      }
      pcity->rapture = 0;
    }
    pcity->was_happy = is_happy;

    /* Handle the illness. */
    if (game.info.illness_on) {
      /* recalculate city illness; illness due to trade has to be saved
       * within the city struct as the client has not all data to
       * calculate it */
      pcity->server.illness
        = city_illness_calc(pcity, NULL, NULL, &(pcity->illness_trade), NULL);

      if (city_illness_check(pcity)) {
        notify_player(pplayer, city_tile(pcity), E_CITY_PLAGUE, ftc_server,
                      _("%s has been struck by a plague! Population lost!"), 
                      city_link(pcity));
        city_reduce_size(pcity, 1, NULL, "plague");
        pcity->turn_plague = game.info.turn;

        /* recalculate illness */
        pcity->server.illness
          = city_illness_calc(pcity, NULL, NULL, &(pcity->illness_trade),
                              NULL);
      }
    }

    /* City population updated here, after the rapture stuff above. --Jing */
    saved_id = pcity->id;
    city_populate(pcity, pplayer);
    if (NULL == player_city_by_number(pplayer, saved_id)) {
      return;
    }

    pcity->did_sell = FALSE;
    pcity->did_buy = FALSE;
    pcity->airlift = city_airlift_max(pcity);
    update_bulbs(pplayer, pcity->prod[O_SCIENCE], FALSE);

    /* Update the treasury. */
    pplayer->economic.gold += pcity->prod[O_GOLD];
    pplayer->economic.gold -= city_total_impr_gold_upkeep(pcity);
    pplayer->economic.gold -= city_total_unit_gold_upkeep(pcity);

    /* Remember how much gold upkeep each unit was payed. */
    unit_list_iterate(pcity->units_supported, punit) {
      punit->server.upkeep_payed[O_GOLD] = punit->upkeep[O_GOLD];
    } unit_list_iterate_end;

    if (pplayer->economic.gold < 0) {
      /* Not enough gold - we have to sell some buildings, and if that
       * is not enough, disband units with gold upkeep, taking into
       * account the setting of 'game.info.gold_upkeep_style':
       * GOLD_UPKEEP_CITY: Cities pay for buildings and units.
       * GOLD_UPKEEP_MIXED: Cities pay only for buildings; the nation pays
       *                    for units.
       * GOLD_UPKEEP_NATION: The nation pays for buildings and units. */
      switch (game.info.gold_upkeep_style) {
      case GOLD_UPKEEP_CITY:
      case GOLD_UPKEEP_MIXED:
        if (!city_balance_treasury_buildings(pcity)
            && game.info.gold_upkeep_style == GOLD_UPKEEP_CITY) {
          city_balance_treasury_units(pcity);
        }
        break;
      case GOLD_UPKEEP_NATION:
        break;
      }
    }

    revolution_turns = get_city_bonus(pcity, EFT_REVOLUTION_UNHAPPINESS);
    if (city_unhappy(pcity)) {
      const char *revomsg;

      pcity->anarchy++;
      if (pcity->anarchy == revolution_turns) {
        /* Revolution next turn if not dealt with */
        /* TRANS: preserve leading space; this string will be appended to
         * another sentence */
        revomsg = _(" Unrest threatens to spread beyond the city.");
      } else {
        revomsg = "";
      }
      if (pcity->anarchy == 1) {
        notify_player(pplayer, city_tile(pcity), E_CITY_DISORDER, ftc_server,
                      /* TRANS: second %s is an optional extra sentence */
                      _("Civil disorder in %s.%s"),
                      city_link(pcity), revomsg);
      } else {
        notify_player(pplayer, city_tile(pcity), E_CITY_DISORDER, ftc_server,
                      /* TRANS: second %s is an optional extra sentence */
                      _("CIVIL DISORDER CONTINUES in %s.%s"),
                      city_link(pcity), revomsg);
      }
    } else {
      if (pcity->anarchy != 0) {
        notify_player(pplayer, city_tile(pcity), E_CITY_NORMAL, ftc_server,
                      _("Order restored in %s."),
                      city_link(pcity));
      }
      pcity->anarchy = 0;
    }
    check_pollution(pcity);

    send_city_info(NULL, pcity);

    if (revolution_turns > 0 && pcity->anarchy > revolution_turns) {
      notify_player(pplayer, city_tile(pcity), E_ANARCHY, ftc_server,
                    /* TRANS: %s - government form, e.g., Democracy */
                    _("The people have overthrown your %s, "
                      "your country is in turmoil."),
                    government_name_translation(gov));
      handle_player_change_government(pplayer, government_number(gov));
    }
    if (city_refresh(pcity)) {
      auto_arrange_workers(pcity);
    }
    sanity_check_city(pcity);
  }
}

/**********************************************************************//**
  Check if city suffers from a plague. Return TRUE if it does, FALSE if not.
**************************************************************************/
static bool city_illness_check(const struct city * pcity)
{
  if (fc_rand(1000) < pcity->server.illness) {
    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Disband a city into the built unit, supported by the closest city.
**************************************************************************/
static bool disband_city(struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  struct tile *ptile = pcity->tile;
  struct city *rcity=NULL;
  struct unit_type *utype = pcity->production.value.utype;
  int saved_id = pcity->id;

  /* find closest city other than pcity */
  rcity = find_closest_city(ptile, pcity, pplayer, FALSE, FALSE, FALSE, TRUE,
                            FALSE, NULL);

  if (!rcity) {
    /* What should we do when we try to disband our only city? */
    notify_player(pplayer, ptile, E_CITY_CANTBUILD, ftc_server,
                  _("%s can't build %s yet, "
                    "and we can't disband our only city."),
                  city_link(pcity), utype_name_translation(utype));
    script_server_signal_emit("unit_cant_be_built", utype, pcity,
                              "pop_cost");
    if (!city_exist(saved_id)) {
      /* Script decided to remove even the last city */
      return TRUE;
    } else {
      return FALSE;
    }
  }

  (void) create_unit(pplayer, ptile, utype,
		     do_make_unit_veteran(pcity, utype),
		     pcity->id, 0);
  pplayer->score.units_built++;

  /* Shift all the units supported by pcity (including the new unit)
   * to rcity.  transfer_city_units does not make sure no units are
   * left floating without a transport, but since all units are
   * transferred this is not a problem. */
  transfer_city_units(pplayer, pplayer, pcity->units_supported, rcity, 
                      pcity, -1, TRUE);

  notify_player(pplayer, ptile, E_UNIT_BUILT, ftc_server,
                /* TRANS: "<city> is disbanded into Settler." */
                _("%s is disbanded into %s."), 
                city_tile_link(pcity), utype_name_translation(utype));

  script_server_signal_emit("city_destroyed", pcity, pcity->owner, NULL);

  remove_city(pcity);
  return TRUE;
}

/**********************************************************************//**
  Helper function to calculate a "score" of a city. The score is used to get
  an estimate of the "migration desirability" of the city. The higher the
  score the more likely citizens will migrate to it.

  The score depends on the city size, the feeling of its citizens, the cost
  of all buildings in the city, and the surplus of trade, luxury and
  science.

  formula:
    score = ([city size] + feeling) * factors

  * feeling of the citizens
    feeling = 1.00 * happy citizens
            + 0.00 * content citizens
            - 0.25 * unhappy citizens
            - 0.50 * angry citizens

  * factors
    * the build costs of all buildings
      f = (1 + (1 - exp(-[build shield cost]/1000))/5)
    * the trade of the city
      f = (1 + (1 - exp(-[city surplus trade]/100))/5)
    * the luxury within the city
      f = (1 + (1 - exp(-[city surplus luxury]/100))/5)
    * the science within the city
      f = (1 + (1 - exp(-[city surplus science]/100))/5)

  all factors f have values between 1 and 1.2; the overall factor will be
  between 1.0 (smaller cities) and 2.0 (bigger cities)

  [build shield cost], [city surplus trade], [city surplus luxury] and
  [city surplus science] _must_ be >= 0!

  The food furplus is considered by an additional factor

    * the food surplus of the city
      f = (1 + [city surplus food; interval -10..20]/10)

  The health factor is defined as:

    * the health of the city
      f = (100 - [city illness; tenth of %]/25)

  * if the city has at least one wonder a factor of 1.25 is added
  * for the capital an additional factor of 1.25 is used
  * the score is also modified by the effect EFT_MIGRATION_PCT
**************************************************************************/
static float city_migration_score(struct city *pcity)
{
  float score = 0.0;
  int build_shield_cost = 0;
  bool has_wonder = FALSE;

  if (!pcity) {
    return score;
  }

  if (pcity->server.mgr_score_calc_turn == game.info.turn) {
    /* up-to-date migration score */
    return pcity->server.migration_score;
  }

  /* feeling of the citizens */
  score = (city_size_get(pcity)
           + 1.00 * pcity->feel[CITIZEN_HAPPY][FEELING_FINAL]
           + 0.00 * pcity->feel[CITIZEN_CONTENT][FEELING_FINAL]
           - 0.25 * pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]
           - 0.50 * pcity->feel[CITIZEN_ANGRY][FEELING_FINAL]);

  /* calculate shield build cost for all buildings */
  city_built_iterate(pcity, pimprove) {
    build_shield_cost += impr_build_shield_cost(pcity, pimprove);
    if (is_wonder(pimprove)) {
      /* this city has a wonder */
      has_wonder = TRUE;
    }
  } city_built_iterate_end;

  /* take shield costs of all buidings into account; normalized by 1000 */
  score *= (1 + (1 - exp(- (float) MAX(0, build_shield_cost) / 1000)) / 5);
  /* take trade into account; normalized by 100 */
  score *= (1 + (1 - exp(- (float) MAX(0, pcity->surplus[O_TRADE]) / 100))
                / 5);
  /* take luxury into account; normalized by 100 */
  score *= (1 + (1 - exp(- (float) MAX(0, pcity->surplus[O_LUXURY]) / 100))
                / 5);
  /* take science into account; normalized by 100 */
  score *= (1 + (1 - exp(- (float) MAX(0, pcity->surplus[O_SCIENCE]) / 100))
                / 5);

  score += city_culture(pcity) * game.info.culture_migration_pml / 1000;

  /* Take food into account; the food surplus is clipped to values between
   * -10..20 and normalize by 10. Thus, the factor is between 0.9 and 1.2. */
  score *= (1 + (float) CLIP(-10, pcity->surplus[O_FOOD], 20) / 10 );

  /* Reduce the score due to city illness (plague). The illness is given in
   * tenth of percent (0..1000) and normalized by 25. Thus, this factor is
   * between 0.6 (ill city) and 1.0 (health city). */
  score *= (100 - (float)city_illness_calc(pcity, NULL, NULL, NULL, NULL)
                  / 25);

  if (has_wonder) {
    /* people like wonders */
    score *= 1.25;
  }

  if (is_capital(pcity)) {
    /* the capital is a magnet for the citizens */
    score *= 1.25;
  }

  /* take into account effects */
  score *= (1.0 + get_city_bonus(pcity, EFT_MIGRATION_PCT) / 100.0);

  log_debug("[M] %s score: %.3f", city_name_get(pcity), score);

  /* set migration score for the city */
  pcity->server.migration_score = score;
  /* set the turn, when the score was calculated */
  pcity->server.mgr_score_calc_turn = game.info.turn;

  return score;
}

/**********************************************************************//**
  Do the migrations between the cities that overlap, if the growth of the
  target city is not blocked due to a missing improvement or missing food.

  Returns TRUE if migration occurred.
**************************************************************************/
static bool do_city_migration(struct city *pcity_from,
                              struct city *pcity_to)
{
  struct player *pplayer_from, *pplayer_to, *pplayer_citizen;
  struct tile *ptile_from, *ptile_to;
  char name_from[MAX_LEN_LINK], name_to[MAX_LEN_LINK];
  const char *nation_from, *nation_to;
  struct city *rcity = NULL;
  bool incr_success;
  int to_id = pcity_to->id;

  if (!pcity_from || !pcity_to) {
    return FALSE;
  }

  pplayer_from = city_owner(pcity_from);
  pplayer_citizen = pplayer_from;
  pplayer_to = city_owner(pcity_to);
  /* We copy that, because city_link always returns the same pointer. */
  sz_strlcpy(name_from, city_link(pcity_from));
  sz_strlcpy(name_to, city_link(pcity_to));
  nation_from = nation_adjective_for_player(pplayer_from);
  nation_to = nation_adjective_for_player(pplayer_to);
  ptile_from = city_tile(pcity_from);
  ptile_to = city_tile(pcity_to);

  /* check food supply in the receiver city */
  if (game.server.mgr_foodneeded) {
    bool migration = FALSE;

    if (pcity_to->surplus[O_FOOD] >= game.info.food_cost) {
      migration = TRUE;
    } else {
      /* check if there is a free tile for the new citizen which, when worked,
       * leads to zero or positive food surplus for the enlarged city */
      int max_food_tile = -1;  /* no free tile */
      city_tile_iterate(city_map_radius_sq_get(pcity_to),
                        city_tile(pcity_to), ptile) {
        if (city_can_work_tile(pcity_to, ptile)
            && tile_worked(ptile) != pcity_to) {
          /* Safest assumption is that city won't be celebrating once an
           * additional citizen is added */
          max_food_tile = MAX(max_food_tile,
                              city_tile_output(pcity_to, ptile, FALSE, O_FOOD));
        }
      } city_tile_iterate_end;
      if (max_food_tile >= 0
          && pcity_to->surplus[O_FOOD] + max_food_tile >= game.info.food_cost) {
        migration = TRUE;
      }
    }

    if (!migration) {
      /* insufficiency food in receiver city; no additional citizens */
      if (pplayer_from == pplayer_to) {
        /* migration between one nation */
        notify_player(pplayer_to, ptile_to, E_CITY_TRANSFER, ftc_server,
                      /* TRANS: From <city1> to <city2>. */
                      _("Migrants from %s can't go to %s because there is "
                        "not enough food available!"),
                      name_from, name_to);
      } else {
        /* migration between different nations */
        notify_player(pplayer_from, ptile_to, E_CITY_TRANSFER, ftc_server,
                      /* TRANS: From <city1> to <city2> (<city2 nation adjective>). */
                      _("Migrants from %s can't go to %s (%s) because there "
                        "is not enough food available!"),
                      name_from, name_to, nation_to);
        notify_player(pplayer_to, ptile_to, E_CITY_TRANSFER, ftc_server,
                      /* TRANS: From <city1> (<city1 nation adjective>) to <city2>. */
                      _("Migrants from %s (%s) can't go to %s because there "
                        "is not enough food available!"),
                      name_from, nation_from, name_to);
      }

      return FALSE;
    }
  }

  if (!city_can_grow_to(pcity_to, city_size_get(pcity_to) + 1)) {
    /* receiver city can't grow  */
    if (pplayer_from == pplayer_to) {
      /* migration between one nation */
      notify_player(pplayer_to, ptile_to, E_CITY_TRANSFER, ftc_server,
                    /* TRANS: From <city1> to <city2>. */
                    _("Migrants from %s can't go to %s because it needs "
                      "an improvement to grow!"),
                    name_from, name_to);
    } else {
      /* migration between different nations */
      notify_player(pplayer_from, ptile_to, E_CITY_TRANSFER, ftc_server,
                    /* TRANS: From <city1> to <city2> of <city2 nation adjective>. */
                    _("Migrants from %s can't go to %s (%s) because it "
                      "needs an improvement to grow!"),
                    name_from, name_to, nation_to);
      notify_player(pplayer_to, ptile_to, E_CITY_TRANSFER, ftc_server,
                    /* TRANS: From <city1> (<city1 nation adjective>) to <city2>. */
                    _("Migrants from %s (%s) can't go to %s because it "
                      "needs an improvement to grow!"),
                    name_from, nation_from, name_to);
    }

    return FALSE;
  }

  /* reduce size of giver */
  if (city_size_get(pcity_from) == 1) {

    if (game.info.citizen_nationality) {
      /* Preserve nationality of city's only citizen */
      pplayer_citizen = player_slot_get_player(citizens_random(pcity_from));
    }

    /* do not destroy wonders */
    city_built_iterate(pcity_from, pimprove) {
      if (is_wonder(pimprove)) {
        return FALSE;
      }
    } city_built_iterate_end;

    /* find closest city other of the same player than pcity_from */
    rcity = find_closest_city(ptile_from, pcity_from, pplayer_from, FALSE,
                              FALSE, FALSE, TRUE, FALSE, NULL);

    if (rcity) {
      int id = pcity_from->id;

      /* transfer all units to the closest city */
      transfer_city_units(pplayer_from, pplayer_from,
                          pcity_from->units_supported, rcity, pcity_from,
                          -1, TRUE);
      sz_strlcpy(name_from, city_tile_link(pcity_from));

      script_server_signal_emit("city_size_change", pcity_from, -1,
                                "migration_from");

      if (city_exist(id)) {
        script_server_signal_emit("city_destroyed", pcity_from,
                                  pcity_from->owner, NULL);

        if (city_exist(id)) {
          remove_city(pcity_from);
        }
      }

      notify_player(pplayer_from, ptile_from, E_CITY_LOST, ftc_server,
                    _("%s was disbanded by its citizens."),
                    name_from);
    } else {
      /* it's the only city of the nation */
      return FALSE;
    }
  } else {
    /* the migrants take half of the food box with them (this prevents
     * migration -> grow -> migration -> ... cycles) */
    pcity_from->food_stock /= 2;

    if (game.info.citizen_nationality) {
      /* Those citizens that are from the target nation are most
       * ones migrating. */
      if (citizens_nation_get(pcity_from, pplayer_to->slot) > 0) {
        pplayer_citizen = pplayer_to;
      } else if (!citizens_nation_get(pcity_from, pplayer_citizen->slot)) {
        /* No native citizens at all in the city, choose random foreigner */
        struct player_slot *cit_slot = citizens_random(pcity_from);

        pplayer_citizen = player_slot_get_player(cit_slot);
      }
      /* This should be followed by city_reduce_size(). */
      citizens_nation_add(pcity_from, pplayer_citizen->slot, -1);
    }
    city_reduce_size(pcity_from, 1, pplayer_from, "migration_from");
    city_refresh_vision(pcity_from);
    if (city_refresh(pcity_from)) {
      auto_arrange_workers(pcity_from);
    }
  }

  /* This should be _before_ the size of the city is increased. Thus, the
   * order of the messages is correct (1: migration; 2: increased size). */
  if (pplayer_from == pplayer_to) {
    /* migration between one nation */
    notify_player(pplayer_from, ptile_to, E_CITY_TRANSFER, ftc_server,
                  /* TRANS: From <city1> to <city2>. */
                  _("Migrants from %s moved to %s in search of a better "
                    "life."), name_from, name_to);
  } else {
    /* migration between different nations */
    notify_player(pplayer_from, ptile_to, E_CITY_TRANSFER, ftc_server,
                  /* TRANS: From <city1> to <city2> (<city2 nation adjective>). */
                  _("Migrants from %s moved to %s (%s) in search of a "
                    "better life."),
                  name_from, name_to, nation_to);
    notify_player(pplayer_to, ptile_to, E_CITY_TRANSFER, ftc_server,
                  /* TRANS: From <city1> (<city1 nation adjective>) to <city2>. */
                  _("Migrants from %s (%s) moved to %s in search of a "
                    "better life."),
                  name_from, nation_from, name_to);
  }

  /* raise size of receiver city */
  if (city_exist(to_id)) {
    incr_success = city_increase_size(pcity_to, pplayer_citizen);
    if (city_exist(to_id)) {
      city_refresh_vision(pcity_to);
      if (city_refresh(pcity_to)) {
        auto_arrange_workers(pcity_to);
      }
      if (incr_success) {
        script_server_signal_emit("city_size_change", pcity_to, 1,
                                  "migration_to");
      }
    }
  }

  log_debug("[M] T%d migration successful (%s -> %s)",
            game.info.turn, name_from, name_to);

  return TRUE;
}

/**********************************************************************//**
  Check for citizens who want to migrate between the cities that overlap.
  Migrants go to the city with higher score, if the growth of the target
  city is not blocked due to a missing improvement.

  The following setting are used:

  'game.server.mgr_turninterval' controls the number of turns between
  migration checks for one city (counted from the founding). If this
  setting is zero, or it is the first turn (T1), migration does no occur.

  'game.server.mgr_distance' is the maximal distance for migration.

  'game.server.mgr_nationchance' gives the chance for migration within one
  nation.

  'game.server.mgr_worldchance' gives the chance for migration between all
  nations.

  Returns TRUE iff there has been INTERNATIONAL migration.
**************************************************************************/
bool check_city_migrations(void)
{
  bool internat = FALSE;

  if (!game.server.migration) {
    return FALSE;
  }

  if (game.server.mgr_turninterval <= 0
      || (game.server.mgr_worldchance <= 0
          && game.server.mgr_nationchance <= 0)) {
    return FALSE;
  }

  /* check for migration */
  players_iterate(pplayer) {
    if (!pplayer->cities) {
      continue;
    }

    if (check_city_migrations_player(pplayer)) {
      internat = TRUE;
    }
  } players_iterate_end;

  return internat;
}

/**********************************************************************//**
  Returns TRUE iff the city's food stock was emptied. Should empty the
  food stock unless it already is empty.
**************************************************************************/
bool city_empty_food_stock(struct city *pcity) {
  struct player *pplayer = city_owner(pcity);
  struct tile *ptile = city_tile(pcity);

  fc_assert_ret_val(pcity != NULL, FALSE);

  if (pcity->food_stock > 0) {
    pcity->food_stock = 0;

    notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                  /* TRANS: %s is a city name */
                  _("All stored food destroyed in %s."), city_link(pcity));

    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Disaster has hit a city. Apply its effects.
**************************************************************************/
static void apply_disaster(struct city *pcity, struct disaster_type *pdis)
{
  struct player *pplayer = city_owner(pcity);
  struct tile *ptile = city_tile(pcity);
  bool had_internal_effect = FALSE;

  log_debug("%s at %s", disaster_rule_name(pdis), city_name_get(pcity));

  notify_player(pplayer, ptile, E_DISASTER,
                ftc_server,
                /* TRANS: Disasters such as Earthquake */
                _("%s was hit by %s."), city_name_get(pcity),
                disaster_name_translation(pdis));

  if (disaster_has_effect(pdis, DE_POLLUTION)) {
    if (place_pollution(pcity, EC_POLLUTION)) {
      notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                    _("Pollution near %s."), city_link(pcity));
      had_internal_effect = TRUE;
    }
  }

  if (disaster_has_effect(pdis, DE_FALLOUT)) {
    if (place_pollution(pcity, EC_FALLOUT)) {
      notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                    _("Fallout near %s."), city_link(pcity));
      had_internal_effect = TRUE;
    }
  }

  if (disaster_has_effect(pdis, DE_REDUCE_DESTROY)
      || (disaster_has_effect(pdis, DE_REDUCE_POP)
          && pcity->size > 1)) {
    if (!city_reduce_size(pcity, 1, NULL, "disaster")) {
      notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                    /* TRANS: "Industrial Accident destroys Bogota entirely." */
                    _("%s destroys %s entirely."),
                    disaster_name_translation(pdis), city_link(pcity));
      pcity = NULL;
    } else {
      notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                    /* TRANS: "Nuclear Accident ... Montreal." */
                    _("%s causes population loss in %s."),
                    disaster_name_translation(pdis), city_link(pcity));
    }

    had_internal_effect = TRUE;
  }

  if (pcity && disaster_has_effect(pdis, DE_DESTROY_BUILDING)) {
    int total = 0;
    struct impr_type *imprs[B_LAST];

    city_built_iterate(pcity, pimprove) {
      if (is_improvement(pimprove)
          && !improvement_has_flag(pimprove, IF_DISASTER_PROOF)) {
        imprs[total++] = pimprove;
      }
    } city_built_iterate_end;

    if (total > 0) {
      int num = fc_rand(total);

      building_lost(pcity, imprs[num], "disaster", NULL);

      notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                    /* TRANS: second %s is the name of a city improvement */
                    _("%s destroys %s in %s."),
                    disaster_name_translation(pdis),
                    improvement_name_translation(imprs[num]),
                    city_link(pcity));

      had_internal_effect = TRUE;
    }
  }

  if (pcity && disaster_has_effect(pdis, DE_EMPTY_FOODSTOCK)) {
    if (city_empty_food_stock(pcity)) {
      had_internal_effect = TRUE;
    }
  }

  if (pcity && disaster_has_effect(pdis, DE_EMPTY_PRODSTOCK)) {
    if (pcity->shield_stock > 0) {
      char prod[256];

      pcity->shield_stock = 0;
      nullify_prechange_production(pcity); /* Make it impossible to recover */

      universal_name_translation(&pcity->production, prod, sizeof(prod));
      notify_player(pplayer, ptile, E_DISASTER, ftc_server,
                    /* TRANS: "Production of Colossus in Rhodes destroyed." */
                    _("Production of %s in %s destroyed."),
                    prod, city_link(pcity));

      had_internal_effect = TRUE;
    }
  }

  script_server_signal_emit("disaster_occurred", pdis, pcity,
                            had_internal_effect);
  script_server_signal_emit("disaster", pdis, pcity);
}

/**********************************************************************//**
  Check for any disasters hitting any city, and apply those disasters.
**************************************************************************/
void check_disasters(void)
{
  if (game.info.disasters == 0) {
    /* Shortcut out as no disaster is possible. */
    return;
  }

  players_iterate(pplayer) {
    /* Safe city iterator needed as disaster may destroy city */
    city_list_iterate_safe(pplayer->cities, pcity) {
      int id = pcity->id;

      disaster_type_iterate(pdis) {
        if (city_exist(id)) {
          /* City survived earlier disasters. */
          int probability = game.info.disasters * pdis->frequency;
          int result = fc_rand(DISASTER_BASE_RARITY);

          if (result < probability)  {
            if (can_disaster_happen(pdis, pcity)) {
              apply_disaster(pcity, pdis);
            }
          }
        }
      } disaster_type_iterate_end;
    } city_list_iterate_safe_end;
  } players_iterate_end;
}

/**********************************************************************//**
  Check for migration for each city of one player.

  For each city of the player do:
  * check each tile within GAME_MAX_MGR_DISTANCE for a city
  * if a city is found check the distance
  * compare the migration score
**************************************************************************/
static bool check_city_migrations_player(const struct player *pplayer)
{
  char city_link_text[MAX_LEN_LINK];
  float best_city_player_score, best_city_world_score;
  struct city *best_city_player, *best_city_world, *acity;
  float score_from, score_tmp, weight;
  int dist, mgr_dist;
  bool internat = FALSE;

  /* check for each city
   * city_list_iterate_safe_end must be used because we could
   * remove one city from the list */
  city_list_iterate_safe(pplayer->cities, pcity) {
    /* no migration out of the capital */
    if (is_capital(pcity)) {
      continue;
    }

    /* check only each (game.server.mgr_turninterval) turn
     * (counted from the funding turn) and do not migrate
     * the same turn a city is founded */
    if (game.info.turn == pcity->turn_founded
        || ((game.info.turn - pcity->turn_founded)
            % game.server.mgr_turninterval) != 0) {
      continue;
    }

    best_city_player_score = 0.0;
    best_city_world_score = 0.0;
    best_city_player = NULL;
    best_city_world = NULL;

    /* score of the actual city
     * taking into account a persistence factor of 3 */
    score_from = city_migration_score(pcity) * 3;

    log_debug("[M] T%d check city: %s score: %6.3f (%s)",
              game.info.turn, city_name_get(pcity), score_from,
              player_name(pplayer));

    /* consider all cities within the maximal possible distance
     * (= CITY_MAP_MAX_RADIUS + GAME_MAX_MGR_DISTANCE) */
    iterate_outward(&(wld.map), city_tile(pcity),
                    CITY_MAP_MAX_RADIUS + GAME_MAX_MGR_DISTANCE, ptile) {
      acity = tile_city(ptile);

      if (!acity || acity == pcity) {
        /* no city or the city in the center */
        continue;
      }

      /* Calculate the migration distance. The value of
       * game.server.mgr_distance is added to the current city radius. If the
       * distance between both cities is lower or equal than this value,
       * migration is possible. */
      mgr_dist = (int)sqrt((double)MAX(city_map_radius_sq_get(acity),0))
                 + game.server.mgr_distance;

      /* distance between the two cities */
      dist = real_map_distance(city_tile(pcity), city_tile(acity));

      if (dist > mgr_dist) {
        /* to far away */
        continue;
      }

      /* score of the second city, weighted by the distance */
      weight = ((float) (mgr_dist + 1 - dist) / (float) (mgr_dist + 1));
      score_tmp = city_migration_score(acity) * weight;

      log_debug("[M] T%d - compare city: %s (%s) dist: %d mgr_dist: %d "
                "score: %6.3f", game.info.turn, city_name_get(acity),
                player_name(city_owner(acity)), dist, mgr_dist, score_tmp);

      if (game.server.mgr_nationchance > 0 && city_owner(acity) == pplayer) {
        /* migration between cities of the same owner */
        if (score_tmp > score_from && score_tmp > best_city_player_score) {
          /* select the best! */
          best_city_player_score = score_tmp;
          best_city_player = acity;

          log_debug("[M] T%d - best city (player): %s (%s) score: "
                    "%6.3f (> %6.3f)", game.info.turn,
                    city_name_get(best_city_player), player_name(pplayer),
                    best_city_player_score, score_from);
        }
      } else if (game.server.mgr_worldchance > 0
                 && city_owner(acity) != pplayer) {
        /* migration between cities of different owners */
        if (game.info.citizen_nationality) {
          /* Modify the score if citizens could migrate to a city of their
           * original nation. */
          if (citizens_nation_get(pcity, city_owner(acity)->slot) > 0) {
            score_tmp *= 2;
          }
        }

        if (score_tmp > score_from && score_tmp > best_city_world_score) {
          /* select the best! */
          best_city_world_score = score_tmp;
          best_city_world = acity;

          log_debug("[M] T%d - best city (world): %s (%s) score: "
                    "%6.3f (> %6.3f)", game.info.turn,
                    city_name_get(best_city_world),
                    player_name(city_owner(best_city_world)),
                    best_city_world_score, score_from);
        }
      }
    } iterate_outward_end;

    if (best_city_player_score > 0) {
      /* first, do the migration within one nation */
      if (fc_rand(100) >= game.server.mgr_nationchance) {
        /* no migration */
        /* N.B.: city_link always returns the same pointer. */
        sz_strlcpy(city_link_text, city_link(pcity));
        notify_player(pplayer, city_tile(pcity), E_CITY_TRANSFER, ftc_server,
                      _("Citizens of %s are thinking about migrating to %s "
                        "for a better life."),
                      city_link_text, city_link(best_city_player));
      } else {
        do_city_migration(pcity, best_city_player);
      }

      /* stop here */
      continue;
    }

    if (best_city_world_score > 0) {
      /* second, do the migration between all nations */
      if (fc_rand(100) >= game.server.mgr_worldchance) {
        const char *nname;

        nname = nation_adjective_for_player(city_owner(best_city_world));
        /* no migration */
        /* N.B.: city_link always returns the same pointer. */
        sz_strlcpy(city_link_text, city_link(pcity));
        notify_player(pplayer, city_tile(pcity), E_CITY_TRANSFER, ftc_server,
                      /* TRANS: <city1> to <city2> (<city2 nation adjective>). */
                      _("Citizens of %s are thinking about migrating to %s "
                        "(%s) for a better life."),
                      city_link_text, city_link(best_city_world), nname);
      } else {
        do_city_migration(pcity, best_city_world);
        internat = TRUE;
      }

      /* stop here */
      continue;
    }
  } city_list_iterate_safe_end;

  return internat;
}

/**********************************************************************//**
  Recheck and store style of the city.
**************************************************************************/
void city_style_refresh(struct city *pcity)
{
  pcity->style = city_style(pcity);
}
