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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "support.h"

/* common */
#include "city.h"
#include "events.h"
#include "game.h"
#include "idex.h"
#include "map.h"
#include "player.h"
#include "specialist.h"
#include "unit.h"
#include "worklist.h"

/* server */
#include "citytools.h"
#include "cityturn.h"
#include "notify.h"
#include "plrhand.h"
#include "sanitycheck.h"

#include "cityhand.h"

/**************************************************************************
  Send city_name_suggestion packet back to requesting conn, with
  suggested name and with same id which was passed in (either unit id
  for city builder or existing city id for rename, we don't care here).
**************************************************************************/
void handle_city_name_suggestion_req(struct player *pplayer, int value)
{
  struct unit *punit = player_find_unit_by_id(pplayer, value);
  
  if (!punit) {
    return;
  }

  freelog(LOG_VERBOSE, "handle_city_name_suggest_req(unit_pos=(%d,%d))",
	  punit->tile->x, punit->tile->y);

  dlsend_packet_city_name_suggestion_info(pplayer->connections, value, 
      city_name_suggestion(pplayer, punit->tile));
}

/**************************************************************************
...
**************************************************************************/
void handle_city_change_specialist(struct player *pplayer, int city_id,
				   Specialist_type_id from,
				   Specialist_type_id to)
{
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  if (to < 0 || to >= specialist_count()
      || from < 0 || from >= specialist_count()
      || !city_can_use_specialist(pcity, to)
      || pcity->specialists[from] == 0) {
    /* This could easily just be due to clicking faster on the specialist
     * than the server can cope with. */
    freelog(LOG_VERBOSE, "Error in specialist change request from client.");
    return;
  }

  pcity->specialists[from]--;
  pcity->specialists[to]++;

  sanity_check_city(pcity);
  city_refresh(pcity);
  send_city_info(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_make_specialist(struct player *pplayer, int city_id,
				 int worker_x, int worker_y)
{
  struct tile *ptile;
  struct tile *pcenter;
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (NULL == pcity) {
    freelog(LOG_ERROR,
            "handle_city_make_specialist() bad city number %d.",
            city_id);
    return;
  }

  if (!is_valid_city_coords(worker_x, worker_y)) {
    freelog(LOG_ERROR,
            "handle_city_make_specialist() invalid city map {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }
  pcenter = city_tile(pcity);

  if (NULL == (ptile = city_map_to_tile(pcenter, worker_x, worker_y))) {
    freelog(LOG_ERROR,
            "handle_city_make_specialist() unavailable city map {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }

  if (is_free_worked(pcity, ptile)) {
    auto_arrange_workers(pcity);
    sync_cities();
    return;
  }

  if (tile_worked(ptile) == pcity) {
    city_map_update_empty(pcity, ptile, worker_x, worker_y);
    pcity->specialists[DEFAULT_SPECIALIST]++;
    city_refresh(pcity);
    sync_cities();
  } else {
    freelog(LOG_VERBOSE,
            "handle_city_make_specialist() not working {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
  }

  sanity_check_city(pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_make_worker(struct player *pplayer, int city_id,
			     int worker_x, int worker_y)
{
  struct tile *ptile;
  struct tile *pcenter;
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (NULL == pcity) {
    freelog(LOG_ERROR,
            "handle_city_make_worker() bad city number %d.",
            city_id);
    return;
  }

  if (!is_valid_city_coords(worker_x, worker_y)) {
    freelog(LOG_ERROR,
            "handle_city_make_worker() invalid city map {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }
  pcenter = city_tile(pcity);

  if (NULL == (ptile = city_map_to_tile(pcenter, worker_x, worker_y))) {
    freelog(LOG_ERROR,
            "handle_city_make_worker() unavailable city map {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }

  if (is_free_worked(pcity, ptile)) {
    auto_arrange_workers(pcity);
    sync_cities();
    return;
  }

  if (tile_worked(ptile) == pcity) {
    freelog(LOG_VERBOSE,
            "handle_city_make_worker() already working {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }

  if (0 == city_specialists(pcity)) {
    freelog(LOG_VERBOSE,
            "handle_city_make_worker() no specialists {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }

  if (!city_can_work_tile(pcity, ptile)) {
    freelog(LOG_VERBOSE,
            "handle_city_make_worker() cannot work here {%d,%d} \"%s\".",
            worker_x, worker_y,
            city_name(pcity));
    return;
  }

  city_map_update_worker(pcity, ptile, worker_x, worker_y);

  specialist_type_iterate(i) {
    if (pcity->specialists[i] > 0) {
      pcity->specialists[i]--;
      break;
    }
  } specialist_type_iterate_end;

  sanity_check_city(pcity);
  city_refresh(pcity);
  sync_cities();
}

/**************************************************************************
...
**************************************************************************/
void really_handle_city_sell(struct player *pplayer, struct city *pcity,
			     struct impr_type *pimprove)
{
  if (pcity->did_sell) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
		  _("You have already sold something here this turn."));
    return;
  }

  if (!can_city_sell_building(pcity, pimprove))
    return;

  pcity->did_sell=TRUE;
  notify_player(pplayer, pcity->tile, E_IMP_SOLD, FTC_SERVER_INFO, NULL,
                _("You sell %s in %s for %d gold."), 
                improvement_name_translation(pimprove),
                city_link(pcity),
                impr_sell_gold(pimprove));
  do_sell_building(pplayer, pcity, pimprove);

  city_refresh(pcity);

  /* If we sold the walls the other players should see it */
  send_city_info(NULL, pcity);
  send_player_info(pplayer, pplayer);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_sell(struct player *pplayer, int city_id, int build_id)
{
  struct city *pcity = player_find_city_by_id(pplayer, city_id);
  struct impr_type *pimprove = improvement_by_number(build_id);

  if (!pcity || !pimprove) {
    return;
  }
  really_handle_city_sell(pplayer, pcity, pimprove);
}

/**************************************************************************
...
**************************************************************************/
void really_handle_city_buy(struct player *pplayer, struct city *pcity)
{
  int cost, total;

  /* This function corresponds to city_can_buy() in the client. */

  assert(pcity && player_owns_city(pplayer, pcity));
 
  if (pcity->turn_founded == game.info.turn) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("Cannot buy in city created this turn."));
    return;
  }

  if (pcity->did_buy) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
		  _("You have already bought this turn."));
    return;
  }

  if (city_production_has_flag(pcity, IF_GOLD)) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("You don't buy %s!"),
                  improvement_name_translation(pcity->production.value.building));
    return;
  }

  if (VUT_UTYPE == pcity->production.kind && pcity->anarchy != 0) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("Can't buy units when city is in disorder."));
    return;
  }

  total = city_production_build_shield_cost(pcity);
  cost = city_production_buy_gold_cost(pcity);
  if (cost <= 0) {
    return; /* sanity */
  }
  if (cost > pplayer->economic.gold) {
    /* In case something changed while player tried to buy, or player 
     * tried to cheat! */
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("%d gold required.  You only have %d gold."), cost,
                  pplayer->economic.gold);
    return;
  }

  pplayer->economic.gold-=cost;
  if (pcity->shield_stock < total){
    /* As we never put penalty on disbanded_shields, we can
     * fully well add the missing shields there. */
    pcity->disbanded_shields += total - pcity->shield_stock;
    pcity->shield_stock=total; /* AI wants this -- Syela */
    pcity->did_buy = TRUE;	/* !PS: no need to set buy flag otherwise */
  }
  city_refresh(pcity);
  
  conn_list_do_buffer(pplayer->connections);
  send_city_info(pplayer, pcity);
  send_player_info(pplayer,pplayer);
  conn_list_do_unbuffer(pplayer->connections);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_worklist(struct player *pplayer, int city_id,
			  struct worklist *worklist)
{
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  worklist_copy(&pcity->worklist, worklist);

  send_city_info(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_buy(struct player *pplayer, int city_id)
{
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  really_handle_city_buy(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_refresh(struct player *pplayer, int city_id)
{
  if (city_id != 0) {
    struct city *pcity = player_find_city_by_id(pplayer, city_id);

    if (!pcity) {
      return;
    }

    city_refresh(pcity);
    send_city_info(pplayer, pcity);
  } else {
    city_refresh_for_player(pplayer);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_city_change(struct player *pplayer, int city_id,
			int production_kind, int production_value)
{
  struct universal prod;
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (production_kind < VUT_NONE || production_kind >= VUT_LAST) {
    freelog(LOG_ERROR, "handle_city_change()"
            " bad production_kind %d.",
            production_kind);
    prod.kind = VUT_NONE;
    return;
  } else {
    prod = universal_by_number(production_kind, production_value);
    if (prod.kind < VUT_NONE || prod.kind >= VUT_LAST) {
      freelog(LOG_ERROR, "handle_city_change()"
              " production_kind %d with bad production_value %d.",
              production_kind,
              production_value);
      prod.kind = VUT_NONE;
      return;
    }
  }

  if (!pcity) {
    return;
  }

  if (are_universals_equal(&pcity->production, &prod)) {
    /* The client probably shouldn't send such a packet. */
    return;
  }

  if (!can_city_build_now(pcity, prod)) {
    return;
  }
  if (!city_can_change_build(pcity)) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("You have bought this turn, can't change."));
    return;
  }

  change_build_target(pplayer, pcity, prod, E_CITY_PRODUCTION_CHANGED);

  sanity_check_city(pcity);
  city_refresh(pcity);
  send_city_info(pplayer, pcity);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_rename(struct player *pplayer, int city_id, char *name)
{
  struct city *pcity = player_find_city_by_id(pplayer, city_id);
  char message[1024];

  if (!pcity) {
    return;
  }

  if (!is_allowed_city_name(pplayer, name, message, sizeof(message))) {
    notify_player(pplayer, pcity->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL, "%s", message);
    return;
  }

  sz_strlcpy(pcity->name, name);
  city_refresh(pcity);
  send_city_info(NULL, pcity);
}

/****************************************************************************
  Handles a packet from the client that requests the city options for the
  given city be changed.
****************************************************************************/
void handle_city_options_req(struct player *pplayer, int city_id,
			     bv_city_options options)
{
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (!pcity) {
    return;
  }

  pcity->city_options = options;

  send_city_info(pplayer, pcity);
}
