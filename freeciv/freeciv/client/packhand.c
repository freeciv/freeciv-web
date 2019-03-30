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

/* utility */
#include "bitvector.h"
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "achievements.h"
#include "actions.h"
#include "capstr.h"
#include "citizens.h"
#include "events.h"
#include "extras.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "name_translation.h"
#include "movement.h"
#include "multipliers.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "rgbcolor.h"
#include "road.h"
#include "spaceship.h"
#include "specialist.h"
#include "style.h"
#include "traderoutes.h"
#include "unit.h"
#include "unitlist.h"
#include "worklist.h"

/* client/include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "connectdlg_g.h"
#include "dialogs_g.h"
#include "editgui_g.h"
#include "gui_main_g.h"
#include "inteldlg_g.h"
#include "mapctrl_g.h"          /* popup_newcity_dialog() */
#include "mapview_g.h"
#include "menu_g.h"
#include "messagewin_g.h"
#include "pages_g.h"
#include "plrdlg_g.h"
#include "ratesdlg_g.h"
#include "repodlgs_g.h"
#include "spaceshipdlg_g.h"
#include "voteinfo_bar_g.h"
#include "wldlg_g.h"

/* client */
#include "agents.h"
#include "attribute.h"
#include "audio.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "control.h"
#include "editor.h"
#include "goto.h"               /* client_goto_init() */
#include "helpdata.h"           /* boot_help_texts() */
#include "mapview_common.h"
#include "music.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"
#include "update_queue.h"
#include "voteinfo.h"

/* client/luascript */
#include "script_client.h"

#include "packhand.h"

/* Define this macro to get additional debug output about the transport
 * status of the units. */
#undef DEBUG_TRANSPORT

static void city_packet_common(struct city *pcity, struct tile *pcenter,
                               struct player *powner,
                               struct tile_list *worked_tiles,
                               bool is_new, bool popup, bool investigate);
static bool handle_unit_packet_common(struct unit *packet_unit);


/* The dumbest of cities, placeholders for unknown and unseen cities. */
static struct {
  struct city_list *cities;
  struct player *placeholder;
} invisible = {
  .cities = NULL,
  .placeholder = NULL
};

static struct {
  int len;
  enum event_type event;
  char *caption;
  char *headline;
  char *lines;
  int parts;
} page_msg_report = { .parts = 0 };

extern const char forced_tileset_name[];

/************************************************************************//**
  Called below, and by client/client_main.c client_game_free()
****************************************************************************/
void packhand_free(void)
{
  if (NULL != invisible.cities) {
    city_list_iterate(invisible.cities, pcity) {
      idex_unregister_city(&wld, pcity);
      destroy_city_virtual(pcity);
    } city_list_iterate_end;

    city_list_destroy(invisible.cities);
    invisible.cities = NULL;
  }

  if (NULL != invisible.placeholder) {
    free(invisible.placeholder);
    invisible.placeholder = NULL;
  }
}

/************************************************************************//**
  Called only by handle_map_info() below.
****************************************************************************/
static void packhand_init(void)
{
  packhand_free();

  invisible.cities = city_list_new();

  /* Can't use player_new() here, as it will register the player. */
  invisible.placeholder = fc_calloc(1, sizeof(*invisible.placeholder));
  memset(invisible.placeholder, 0, sizeof(*invisible.placeholder));
  /* Set some values to prevent bugs ... */
  sz_strlcpy(invisible.placeholder->name, ANON_PLAYER_NAME);
  sz_strlcpy(invisible.placeholder->username, _(ANON_USER_NAME));
  invisible.placeholder->unassigned_user = TRUE;
  sz_strlcpy(invisible.placeholder->ranked_username, ANON_USER_NAME);
  invisible.placeholder->unassigned_ranked = TRUE;
}

/************************************************************************//**
  Unpackage the unit information into a newly allocated unit structure.

  Information for the client must also be processed in
  handle_unit_packet_common()! Especially notice that unit structure filled
  here is just temporary one. Values must be copied to real unit in
  handle_unit_packet_common().
****************************************************************************/
static struct unit *unpackage_unit(const struct packet_unit_info *packet)
{
  struct unit *punit = unit_virtual_create(player_by_number(packet->owner),
					   NULL,
					   utype_by_number(packet->type),
					   packet->veteran);

  /* Owner, veteran, and type fields are already filled in by
   * unit_virtual_create. */
  punit->nationality = player_by_number(packet->nationality);
  punit->id = packet->id;
  unit_tile_set(punit, index_to_tile(&(wld.map), packet->tile));
  punit->facing = packet->facing;
  punit->homecity = packet->homecity;
  output_type_iterate(o) {
    punit->upkeep[o] = packet->upkeep[o];
  } output_type_iterate_end;
  punit->moves_left = packet->movesleft;
  punit->hp = packet->hp;
  punit->activity = packet->activity;
  punit->activity_count = packet->activity_count;

  if (packet->activity_tgt == EXTRA_NONE) {
    punit->activity_target = NULL;
  } else {
    punit->activity_target = extra_by_number(packet->activity_tgt);
  }

  punit->changed_from = packet->changed_from;
  punit->changed_from_count = packet->changed_from_count;

 if (packet->changed_from_tgt == EXTRA_NONE) {
    punit->changed_from_target = NULL;
  } else {
    punit->changed_from_target = extra_by_number(packet->changed_from_tgt);
  }

  punit->ai_controlled = packet->ai;
  punit->fuel = packet->fuel;
  punit->goto_tile = index_to_tile(&(wld.map), packet->goto_tile);
  punit->paradropped = packet->paradropped;
  punit->done_moving = packet->done_moving;
  punit->stay = packet->stay;

  /* Transporter / transporting information. */
  punit->client.occupied = packet->occupied;
  if (packet->transported) {
    punit->client.transported_by = packet->transported_by;
  } else {
    punit->client.transported_by = -1;
  }
  if (packet->carrying >= 0) {
    punit->carrying = goods_by_number(packet->carrying);
  } else {
    punit->carrying = NULL;
  }

  punit->battlegroup = packet->battlegroup;
  punit->has_orders = packet->has_orders;
  punit->orders.length = packet->orders_length;
  punit->orders.index = packet->orders_index;
  punit->orders.repeat = packet->orders_repeat;
  punit->orders.vigilant = packet->orders_vigilant;
  if (punit->has_orders) {
    int i;

    punit->orders.list
      = fc_malloc(punit->orders.length * sizeof(*punit->orders.list));
    for (i = 0; i < punit->orders.length; i++) {
      punit->orders.list[i].order = packet->orders[i];
      punit->orders.list[i].dir = packet->orders_dirs[i];
      punit->orders.list[i].activity = packet->orders_activities[i];
      punit->orders.list[i].sub_target = packet->orders_sub_targets[i];
      punit->orders.list[i].action = packet->orders_actions[i];

      /* Just an assert. The client doesn't use the action data. */
      fc_assert(punit->orders.list[i].order != ORDER_PERFORM_ACTION
                || action_id_exists(punit->orders.list[i].action));
    }
  }

  punit->action_decision_want = packet->action_decision_want;
  punit->action_decision_tile
    = index_to_tile(&(wld.map), packet->action_decision_tile);

  punit->client.asking_city_name = FALSE;

  return punit;
}

/************************************************************************//**
  Unpackage a short_unit_info packet.  This extracts a limited amount of
  information about the unit, and is sent for units we shouldn't know
  everything about (like our enemies' units).

  Information for the client must also be processed in
  handle_unit_packet_common()! Especially notice that unit structure filled
  here is just temporary one. Values must be copied to real unit in
  handle_unit_packet_common().
****************************************************************************/
static struct unit *
unpackage_short_unit(const struct packet_unit_short_info *packet)
{
  struct unit *punit = unit_virtual_create(player_by_number(packet->owner),
					   NULL,
					   utype_by_number(packet->type),
					   FALSE);

  /* Owner and type fields are already filled in by unit_virtual_create. */
  punit->id = packet->id;
  unit_tile_set(punit, index_to_tile(&(wld.map), packet->tile));
  punit->facing = packet->facing;
  punit->nationality = NULL;
  punit->veteran = packet->veteran;
  punit->hp = packet->hp;
  punit->activity = packet->activity;

  if (packet->activity_tgt == EXTRA_NONE) {
    punit->activity_target = NULL;
  } else {
    punit->activity_target = extra_by_number(packet->activity_tgt);
  }

  /* Transporter / transporting information. */
  punit->client.occupied = packet->occupied;
  if (packet->transported) {
    punit->client.transported_by = packet->transported_by;
  } else {
    punit->client.transported_by = -1;
  }

  return punit;
}

/************************************************************************//**
  After we send a join packet to the server we receive a reply.  This
  function handles the reply.
****************************************************************************/
void handle_server_join_reply(bool you_can_join, const char *message,
                              const char *capability,
                              const char *challenge_file, int conn_id)
{
  const char *s_capability = client.conn.capability;

  conn_set_capability(&client.conn, capability);
  close_connection_dialog();

  if (you_can_join) {
    struct packet_client_info client_info;

    log_verbose("join game accept:%s", message);
    client.conn.established = TRUE;
    client.conn.id = conn_id;

    agents_game_joined();
    set_server_busy(FALSE);

    if (get_client_page() == PAGE_MAIN
        || get_client_page() == PAGE_NETWORK) {
      set_client_page(PAGE_START);
    }

    client_info.gui = get_gui_type();
    strncpy(client_info.distribution, FREECIV_DISTRIBUTOR,
            sizeof(client_info.distribution));
    send_packet_client_info(&client.conn, &client_info);

    /* we could always use hack, verify we're local */
#ifdef FREECIV_DEBUG
    if (!hackless || is_server_running())
#endif
    {
      send_client_wants_hack(challenge_file);
    }

    set_client_state(C_S_PREPARING);
  } else {
    output_window_printf(ftc_client,
                         _("You were rejected from the game: %s"), message);
    client.conn.id = -1; /* not in range of conn_info id */

    if (auto_connect) {
      log_normal(_("You were rejected from the game: %s"), message);
    }
    server_connect();

    set_client_page(PAGE_MAIN);
  }
  if (strcmp(s_capability, our_capability) == 0) {
    return;
  }
  output_window_printf(ftc_client, _("Client capability string: %s"),
                       our_capability);
  output_window_printf(ftc_client, _("Server capability string: %s"),
                       s_capability);
}

/************************************************************************//**
  Handles a remove-city packet, used by the server to tell us any time a
  city is no longer there.
****************************************************************************/
void handle_city_remove(int city_id)
{
  struct city *pcity = game_city_by_number(city_id);
  bool need_menus_update;

  fc_assert_ret_msg(NULL != pcity, "Bad city %d.", city_id);

  need_menus_update = (NULL != get_focus_unit_on_tile(city_tile(pcity)));

  agents_city_remove(pcity);
  editgui_notify_object_changed(OBJTYPE_CITY, pcity->id, TRUE);
  client_remove_city(pcity);

  /* Update menus if the focus unit is on the tile. */
  if (need_menus_update) {
    menus_update();
  }
}

/************************************************************************//**
  Handle a remove-unit packet, sent by the server to tell us any time a
  unit is no longer there.
****************************************************************************/
void handle_unit_remove(int unit_id)
{
  struct unit *punit = game_unit_by_number(unit_id);
  struct unit_list *cargos;
  struct player *powner;
  bool need_economy_report_update;

  if (!punit) {
    log_error("Server wants us to remove unit id %d, "
              "but we don't know about this unit!",
              unit_id);
    return;
  }

  /* Close diplomat dialog if the diplomat is lost */
  if (action_selection_actor_unit() == punit->id) {
    action_selection_close();
    /* Open another action selection dialog if there are other actors in the
     * current selection that want a decision. */
    action_selection_next_in_focus(unit_id);
  }

  need_economy_report_update = (0 < punit->upkeep[O_GOLD]);
  powner = unit_owner(punit);

  /* Unload cargo if this is a transporter. */
  cargos = unit_transport_cargo(punit);
  if (unit_list_size(cargos) > 0) {
    unit_list_iterate(cargos, pcargo) {
      unit_transport_unload(pcargo);
    } unit_list_iterate_end;
  }

  /* Unload unit if it is transported. */
  if (unit_transport_get(punit)) {
    unit_transport_unload(punit);
  }
  punit->client.transported_by = -1;

  agents_unit_remove(punit);
  editgui_notify_object_changed(OBJTYPE_UNIT, punit->id, TRUE);
  client_remove_unit(punit);

  if (!client_has_player() || powner == client_player()) {
    if (need_economy_report_update) {
      economy_report_dialog_update();
    }
    units_report_dialog_update();
  }
}

/************************************************************************//**
  The tile (x,y) has been nuked!
****************************************************************************/
void handle_nuke_tile_info(int tile)
{
  put_nuke_mushroom_pixmaps(index_to_tile(&(wld.map), tile));
}

/************************************************************************//**
  The name of team 'team_id'
****************************************************************************/
void handle_team_name_info(int team_id, const char *team_name)
{
  struct team_slot *tslot = team_slot_by_number(team_id);

  fc_assert_ret(NULL != tslot);
  team_slot_set_defined_name(tslot, team_name);
  conn_list_dialog_update();
}

/************************************************************************//**
  A combat packet.  The server tells us the attacker and defender as well
  as both of their hitpoints after the combat is over (in most combat, one
  unit always dies and their HP drops to zero).  If make_winner_veteran is
  set then the surviving unit becomes veteran.
****************************************************************************/
void handle_unit_combat_info(const struct packet_unit_combat_info *packet)
{
  bool show_combat = FALSE;
  struct unit *punit0 = game_unit_by_number(packet->attacker_unit_id);
  struct unit *punit1 = game_unit_by_number(packet->defender_unit_id);

  if (punit0 && punit1) {
    popup_combat_info(packet->attacker_unit_id, packet->defender_unit_id,
                      packet->attacker_hp, packet->defender_hp,
                      packet->make_att_veteran, packet->make_def_veteran);
    if (tile_visible_mapcanvas(unit_tile(punit0)) &&
	tile_visible_mapcanvas(unit_tile(punit1))) {
      show_combat = TRUE;
    } else if (gui_options.auto_center_on_combat) {
      if (unit_owner(punit0) == client.conn.playing) {
        center_tile_mapcanvas(unit_tile(punit0));
      } else {
        center_tile_mapcanvas(unit_tile(punit1));
      }
      show_combat = TRUE;
    }

    if (show_combat) {
      int hp0 = packet->attacker_hp, hp1 = packet->defender_hp;

      audio_play_sound(unit_type_get(punit0)->sound_fight,
		       unit_type_get(punit0)->sound_fight_alt);
      audio_play_sound(unit_type_get(punit1)->sound_fight,
		       unit_type_get(punit1)->sound_fight_alt);

      if (gui_options.smooth_combat_step_msec > 0) {
        decrease_unit_hp_smooth(punit0, hp0, punit1, hp1);
      } else {
	punit0->hp = hp0;
	punit1->hp = hp1;

	set_units_in_combat(NULL, NULL);
	refresh_unit_mapcanvas(punit0, unit_tile(punit0), TRUE, FALSE);
	refresh_unit_mapcanvas(punit1, unit_tile(punit1), TRUE, FALSE);
      }
    }
    if (packet->make_att_veteran && punit0) {
      punit0->veteran++;
      refresh_unit_mapcanvas(punit0, unit_tile(punit0), TRUE, FALSE);
    }
    if (packet->make_def_veteran && punit1) {
      punit1->veteran++;
      refresh_unit_mapcanvas(punit1, unit_tile(punit1), TRUE, FALSE);
    }
  }
}

/************************************************************************//**
  Updates a city's list of improvements from packet data.
  "have_impr" specifies whether the improvement should be added (TRUE)
  or removed (FALSE). Returns TRUE if the improvement has been actually
  added or removed.
****************************************************************************/
static bool update_improvement_from_packet(struct city *pcity,
                                           struct impr_type *pimprove,
                                           bool have_impr)
{
  if (have_impr) {
    if (pcity->built[improvement_index(pimprove)].turn <= I_NEVER) {
      city_add_improvement(pcity, pimprove);
      return TRUE;
    }
  } else {
    if (pcity->built[improvement_index(pimprove)].turn > I_NEVER) {
      city_remove_improvement(pcity, pimprove);
      return TRUE;
    }
  }
  return FALSE;
}

/************************************************************************//**
  A city-info packet contains all information about a city.  If we receive
  this packet then we know everything about the city internals.
****************************************************************************/
void handle_city_info(const struct packet_city_info *packet)
{
  struct universal product;
  int i;
  bool popup;
  bool city_is_new = FALSE;
  bool city_has_changed_owner = FALSE;
  bool need_science_dialog_update = FALSE;
  bool need_units_dialog_update = FALSE;
  bool need_economy_dialog_update = FALSE;
  bool name_changed = FALSE;
  bool update_descriptions = FALSE;
  bool shield_stock_changed = FALSE;
  bool production_changed = FALSE;
  bool trade_routes_changed = FALSE;
  struct unit_list *pfocus_units = get_units_in_focus();
  struct city *pcity = game_city_by_number(packet->id);
  struct tile_list *worked_tiles = NULL;
  struct tile *pcenter = index_to_tile(&(wld.map), packet->tile);
  struct tile *ptile = NULL;
  struct player *powner = player_by_number(packet->owner);

  fc_assert_ret_msg(NULL != powner, "Bad player number %d.", packet->owner);
  fc_assert_ret_msg(NULL != pcenter, "Invalid tile index %d.", packet->tile);

  if (!universals_n_is_valid(packet->production_kind)) {
    log_error("handle_city_info() bad production_kind %d.",
              packet->production_kind);
    product.kind = VUT_NONE;
  } else {
    product = universal_by_number(packet->production_kind,
                                  packet->production_value);
    if (!universals_n_is_valid(product.kind)) {
      log_error("handle_city_info() "
                "production_kind %d with bad production_value %d.",
                packet->production_kind, packet->production_value);
      product.kind = VUT_NONE;
    }
  }

  if (NULL != pcity) {
    ptile = city_tile(pcity);

    if (NULL == ptile) {
      /* invisible worked city */
      city_list_remove(invisible.cities, pcity);
      city_is_new = TRUE;

      pcity->tile = pcenter;
      ptile = pcenter;
      pcity->owner = powner;
      pcity->original = powner;
    } else if (city_owner(pcity) != powner) {
      /* Remember what were the worked tiles.  The server won't
       * send to us again. */
      city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity),
                                         ptile, pworked, _index, _x, _y) {
        if (pcity == tile_worked(pworked)) {
          if (NULL == worked_tiles) {
            worked_tiles = tile_list_new();
          }
          tile_list_append(worked_tiles, pworked);
        }
      } city_tile_iterate_skip_free_worked_end;
      client_remove_city(pcity);
      pcity = NULL;
      city_has_changed_owner = TRUE;
    }
  }

  if (NULL == pcity) {
    city_is_new = TRUE;
    pcity = create_city_virtual(powner, pcenter, packet->name);
    pcity->id = packet->id;
    idex_register_city(&wld, pcity);
    update_descriptions = TRUE;
  } else if (pcity->id != packet->id) {
    log_error("handle_city_info() city id %d != id %d.",
              pcity->id, packet->id);
    return;
  } else if (ptile != pcenter) {
    log_error("handle_city_info() city tile (%d, %d) != (%d, %d).",
              TILE_XY(ptile), TILE_XY(pcenter));
    return;
  } else {
    name_changed = (0 != strncmp(packet->name, pcity->name,
                                 sizeof(pcity->name)));

    while (trade_route_list_size(pcity->routes) > packet->traderoute_count) {
      struct trade_route *proute = trade_route_list_get(pcity->routes, -1);

      trade_route_list_remove(pcity->routes, proute);
      FC_FREE(proute);
      trade_routes_changed = TRUE;
    }

    /* Descriptions should probably be updated if the
     * city name, production or time-to-grow changes.
     * Note that if either the food stock or surplus
     * have changed, the time-to-grow is likely to
     * have changed as well. */
    update_descriptions = (gui_options.draw_city_names && name_changed)
      || (gui_options.draw_city_productions
          && (!are_universals_equal(&pcity->production, &product)
              || pcity->surplus[O_SHIELD] != packet->surplus[O_SHIELD]
              || pcity->shield_stock != packet->shield_stock))
      || (gui_options.draw_city_growth
          && (pcity->food_stock != packet->food_stock
              || pcity->surplus[O_FOOD] != packet->surplus[O_FOOD]))
      || (gui_options.draw_city_trade_routes && trade_routes_changed);
  }
  
  sz_strlcpy(pcity->name, packet->name);
  
  /* check data */
  city_size_set(pcity, 0);
  for (i = 0; i < FEELING_LAST; i++) {
    pcity->feel[CITIZEN_HAPPY][i] = packet->ppl_happy[i];
    pcity->feel[CITIZEN_CONTENT][i] = packet->ppl_content[i];
    pcity->feel[CITIZEN_UNHAPPY][i] = packet->ppl_unhappy[i];
    pcity->feel[CITIZEN_ANGRY][i] = packet->ppl_angry[i];
  }
  for (i = 0; i < CITIZEN_LAST; i++) {
    city_size_add(pcity, pcity->feel[i][FEELING_FINAL]);
  }
  specialist_type_iterate(sp) {
    pcity->specialists[sp] = packet->specialists[sp];
    city_size_add(pcity, pcity->specialists[sp]);
  } specialist_type_iterate_end;

  if (city_size_get(pcity) != packet->size) {
    log_error("handle_city_info() "
              "%d citizens not equal %d city size in \"%s\".",
              city_size_get(pcity), packet->size, city_name_get(pcity));
    city_size_set(pcity, packet->size);
  }

  /* The nationality of the citizens. */
  if (game.info.citizen_nationality) {
    citizens_init(pcity);
    for (i = 0; i < packet->nationalities_count; i++) {
      citizens_nation_set(pcity, player_slot_by_number(packet->nation_id[i]),
                          packet->nation_citizens[i]);
    }
    fc_assert(citizens_count(pcity) == city_size_get(pcity));
  }

  pcity->history = packet->history;
  pcity->client.culture = packet->culture;
  pcity->client.buy_cost = packet->buy_cost;

  pcity->city_radius_sq = packet->city_radius_sq;

  pcity->city_options = packet->city_options;

  if (pcity->surplus[O_SCIENCE] != packet->surplus[O_SCIENCE]
      || pcity->surplus[O_SCIENCE] != packet->surplus[O_SCIENCE]
      || pcity->waste[O_SCIENCE] != packet->waste[O_SCIENCE]
      || (pcity->unhappy_penalty[O_SCIENCE]
          != packet->unhappy_penalty[O_SCIENCE])
      || pcity->prod[O_SCIENCE] != packet->prod[O_SCIENCE]
      || pcity->citizen_base[O_SCIENCE] != packet->citizen_base[O_SCIENCE]
      || pcity->usage[O_SCIENCE] != packet->usage[O_SCIENCE]) {
    need_science_dialog_update = TRUE;
  }

  pcity->food_stock=packet->food_stock;
  if (pcity->shield_stock != packet->shield_stock) {
    shield_stock_changed = TRUE;
    pcity->shield_stock = packet->shield_stock;
  }
  pcity->pollution = packet->pollution;
  pcity->illness_trade = packet->illness_trade;

  if (!are_universals_equal(&pcity->production, &product)) {
    production_changed = TRUE;
  }
  /* Need to consider shield stock/surplus for unit dialog as used build
   * slots may change, affecting number of "in-progress" units. */
  if ((city_is_new && VUT_UTYPE == product.kind)
      || (production_changed && (VUT_UTYPE == pcity->production.kind
                                 || VUT_UTYPE == product.kind))
      || pcity->surplus[O_SHIELD] != packet->surplus[O_SHIELD]
      || shield_stock_changed) {
    need_units_dialog_update = TRUE;
  }
  pcity->production = product;

  output_type_iterate(o) {
    pcity->surplus[o] = packet->surplus[o];
    pcity->waste[o] = packet->waste[o];
    pcity->unhappy_penalty[o] = packet->unhappy_penalty[o];
    pcity->prod[o] = packet->prod[o];
    pcity->citizen_base[o] = packet->citizen_base[o];
    pcity->usage[o] = packet->usage[o];
  } output_type_iterate_end;

#ifdef DONE_BY_create_city_virtual
  if (city_is_new) {
    worklist_init(&pcity->worklist);

    for (i = 0; i < ARRAY_SIZE(pcity->built); i++) {
      pcity->built[i].turn = I_NEVER;
    }
  }
#endif /* DONE_BY_create_city_virtual */

  worklist_copy(&pcity->worklist, &packet->worklist);

  pcity->airlift = packet->airlift;
  pcity->did_buy=packet->did_buy;
  pcity->did_sell=packet->did_sell;
  pcity->was_happy=packet->was_happy;

  pcity->turn_founded = packet->turn_founded;
  pcity->turn_last_built = packet->turn_last_built;

  if (!universals_n_is_valid(packet->changed_from_kind)) {
    log_error("handle_city_info() bad changed_from_kind %d.",
              packet->changed_from_kind);
    product.kind = VUT_NONE;
  } else {
    product = universal_by_number(packet->changed_from_kind,
                                     packet->changed_from_value);
    if (!universals_n_is_valid(product.kind)) {
      log_error("handle_city_info() bad changed_from_value %d.",
                packet->changed_from_value);
      product.kind = VUT_NONE;
    }
  }
  pcity->changed_from = product;

  pcity->before_change_shields=packet->before_change_shields;
  pcity->disbanded_shields=packet->disbanded_shields;
  pcity->caravan_shields=packet->caravan_shields;
  pcity->last_turns_shield_surplus = packet->last_turns_shield_surplus;

  improvement_iterate(pimprove) {
    bool have = BV_ISSET(packet->improvements, improvement_index(pimprove));

    if (have && !city_is_new
        && pcity->built[improvement_index(pimprove)].turn <= I_NEVER) {
      audio_play_sound(pimprove->soundtag, pimprove->soundtag_alt);
    }
    need_economy_dialog_update |=
        update_improvement_from_packet(pcity, pimprove, have);
  } improvement_iterate_end;

  /* We should be able to see units in the city.  But for a diplomat
   * investigating an enemy city we can't.  In that case we don't update
   * the occupied flag at all: it's already been set earlier and we'll
   * get an update if it changes. */
  if (can_player_see_units_in_city(client.conn.playing, pcity)) {
    pcity->client.occupied
      = (unit_list_size(pcity->tile->units) > 0);
  }

  pcity->client.walls = packet->walls;
  if (pcity->client.walls > NUM_WALL_TYPES) {
    pcity->client.walls = NUM_WALL_TYPES;
  }
  pcity->style = packet->style;
  pcity->client.city_image = packet->city_image;

  pcity->client.happy = city_happy(pcity);
  pcity->client.unhappy = city_unhappy(pcity);

  popup = (city_is_new && can_client_change_view()
           && powner == client.conn.playing
           && gui_options.popup_new_cities)
          || packet->diplomat_investigate;

  city_packet_common(pcity, pcenter, powner, worked_tiles,
                     city_is_new, popup, packet->diplomat_investigate);

  if (city_is_new && !city_has_changed_owner) {
    agents_city_new(pcity);
  } else {
    agents_city_changed(pcity);
  }

  /* Update the description if necessary. */
  if (update_descriptions) {
    update_city_description(pcity);
  }

  /* Update focus unit info label if necessary. */
  if (name_changed) {
    unit_list_iterate(pfocus_units, pfocus_unit) {
      if (pfocus_unit->homecity == pcity->id) {
	update_unit_info_label(pfocus_units);
	break;
      }
    } unit_list_iterate_end;
  }

  /* Update the science dialog if necessary. */
  if (need_science_dialog_update) {
    science_report_dialog_update();
  }

  /* Update the units dialog if necessary. */
  if (need_units_dialog_update) {
    units_report_dialog_update();
  }

  /* Update the economy dialog if necessary. */
  if (need_economy_dialog_update) {
    economy_report_dialog_update();
  }

  /* Update the panel text (including civ population). */
  update_info_label();
  
  /* update caravan dialog */
  if ((production_changed || shield_stock_changed)
      && action_selection_target_city() == pcity->id) {   
    dsend_packet_unit_get_actions(&client.conn,
                                  action_selection_actor_unit(),
                                  action_selection_target_unit(),
                                  city_tile(pcity)->index,
                                  action_selection_target_extra(),
                                  FALSE);
  }

  if (gui_options.draw_city_trade_routes
      && (trade_routes_changed
          || (city_is_new && 0 < city_num_trade_routes(pcity)))) {
    update_map_canvas_visible();
  }
}

/************************************************************************//**
  This is a packet that only the web-client needs. The regular client has no
  use for it.
  TODO: Do not generate code calling this in the C-client.
****************************************************************************/
void handle_web_city_info_addition(int id, int granary_size,
                                   int granary_turns)
{
}

/************************************************************************//**
  A helper function for handling city-info and city-short-info packets.
  Naturally, both require many of the same operations to be done on the
  data.
****************************************************************************/
static void city_packet_common(struct city *pcity, struct tile *pcenter,
                               struct player *powner,
                               struct tile_list *worked_tiles,
                               bool is_new, bool popup, bool investigate)
{
  if (NULL != worked_tiles) {
    /* We need to transfer the worked infos because the server will assume
     * those infos are kept in our side and won't send to us again. */
    tile_list_iterate(worked_tiles, pwork) {
      tile_set_worked(pwork, pcity);
    } tile_list_iterate_end;
    tile_list_destroy(worked_tiles);
  }

  if (is_new) {
    tile_set_worked(pcenter, pcity); /* is_free_worked() */
    city_list_prepend(powner->cities, pcity);

    if (client_is_global_observer() || powner == client_player()) {
      city_report_dialog_update();
    }

    players_iterate(pp) {
      unit_list_iterate(pp->units, punit) { 
	if (punit->homecity == pcity->id) {
          unit_list_prepend(pcity->units_supported, punit);
        }
      } unit_list_iterate_end;
    } players_iterate_end;

    pcity->client.first_citizen_index = fc_rand(MAX_NUM_CITIZEN_SPRITES);
  } else {
    if (client_is_global_observer() || powner == client_player()) {
      city_report_dialog_update_city(pcity);
    }
  }

  if (can_client_change_view()) {
    refresh_city_mapcanvas(pcity, pcenter, FALSE, FALSE);
  }

  if (city_workers_display == pcity)  {
    city_workers_display = NULL;
  }

  if (investigate) {
    /* Commit the collected supported and present units. */
    if (pcity->client.collecting_info_units_supported != NULL) {
      /* We got units, let's move the unit lists. */
      fc_assert(pcity->client.collecting_info_units_present != NULL);

      unit_list_destroy(pcity->client.info_units_present);
      pcity->client.info_units_present =
          pcity->client.collecting_info_units_present;
      pcity->client.collecting_info_units_present = NULL;

      unit_list_destroy(pcity->client.info_units_supported);
      pcity->client.info_units_supported =
          pcity->client.collecting_info_units_supported;
      pcity->client.collecting_info_units_supported = NULL;
    } else {
      /* We didn't get any unit, let's clear the unit lists. */
      fc_assert(pcity->client.collecting_info_units_present == NULL);

      unit_list_clear(pcity->client.info_units_supported);
      unit_list_clear(pcity->client.info_units_present);
    }
  }

  if (popup
      && NULL != client.conn.playing
      && is_human(client.conn.playing)
      && can_client_issue_orders()) {
    menus_update();
    if (!city_dialog_is_open(pcity)) {
      popup_city_dialog(pcity);
    }
  }

  if (!is_new
      && (popup || can_player_see_city_internals(client.conn.playing, pcity))) {
    refresh_city_dialog(pcity);
  }

  /* update menus if the focus unit is on the tile. */
  if (get_focus_unit_on_tile(pcenter)) {
    menus_update();
  }

  if (is_new) {
    log_debug("(%d,%d) creating city %d, %s %s", TILE_XY(pcenter),
              pcity->id, nation_rule_name(nation_of_city(pcity)),
              city_name_get(pcity));
  }

  editgui_notify_object_changed(OBJTYPE_CITY, pcity->id, FALSE);
}

/************************************************************************//**
  A traderoute-info packet contains information about one end of a traderoute
****************************************************************************/
void handle_traderoute_info(const struct packet_traderoute_info *packet)
{
  struct city *pcity = game_city_by_number(packet->city);
  struct trade_route *proute;
  bool city_changed = FALSE;

  if (pcity == NULL) {
    return;
  }

  proute = trade_route_list_get(pcity->routes, packet->index);
  if (proute == NULL) {
    fc_assert(trade_route_list_size(pcity->routes) == packet->index);

    proute = fc_malloc(sizeof(struct trade_route));
    trade_route_list_append(pcity->routes, proute);
    city_changed = TRUE;
  }

  proute->partner = packet->partner;
  proute->value = packet->value;
  proute->dir = packet->direction;
  proute->goods = goods_by_number(packet->goods);

  if (gui_options.draw_city_trade_routes && city_changed) {
    update_city_description(pcity);
    update_map_canvas_visible();
  }
}

/************************************************************************//**
  A city-short-info packet is sent to tell us about any cities we can't see
  the internals of.  Most of the time this includes any cities owned by
  someone else.
****************************************************************************/
void handle_city_short_info(const struct packet_city_short_info *packet)
{
  bool city_has_changed_owner = FALSE;
  bool city_is_new = FALSE;
  bool name_changed = FALSE;
  bool update_descriptions = FALSE;
  struct city *pcity = game_city_by_number(packet->id);
  struct tile *pcenter = index_to_tile(&(wld.map), packet->tile);
  struct tile *ptile = NULL;
  struct tile_list *worked_tiles = NULL;
  struct player *powner = player_by_number(packet->owner);
  int radius_sq = game.info.init_city_radius_sq;

  fc_assert_ret_msg(NULL != powner, "Bad player number %d.", packet->owner);
  fc_assert_ret_msg(NULL != pcenter, "Invalid tile index %d.", packet->tile);

  if (NULL != pcity) {
    ptile = city_tile(pcity);

    if (NULL == ptile) {
      /* invisible worked city */
      city_list_remove(invisible.cities, pcity);
      city_is_new = TRUE;

      pcity->tile = pcenter;
      ptile = pcenter;
      pcity->owner = powner;
      pcity->original = powner;

      whole_map_iterate(&(wld.map), wtile) {
        if (wtile->worked == pcity) {
          int dist_sq = sq_map_distance(pcenter, wtile);

          if (dist_sq > city_map_radius_sq_get(pcity)) {
            city_map_radius_sq_set(pcity, dist_sq);
          }
        }
      } whole_map_iterate_end;
    } else if (city_owner(pcity) != powner) {
      /* Remember what were the worked tiles.  The server won't
       * send to us again. */
      city_tile_iterate_skip_free_worked(city_map_radius_sq_get(pcity), ptile,
                                         pworked, _index, _x, _y) {
        if (pcity == tile_worked(pworked)) {
          if (NULL == worked_tiles) {
            worked_tiles = tile_list_new();
          }
          tile_list_append(worked_tiles, pworked);
        }
      } city_tile_iterate_skip_free_worked_end;
      radius_sq = city_map_radius_sq_get(pcity);
      client_remove_city(pcity);
      pcity = NULL;
      city_has_changed_owner = TRUE;
    }
  }

  if (NULL == pcity) {
    city_is_new = TRUE;
    pcity = create_city_virtual(powner, pcenter, packet->name);
    pcity->id = packet->id;
    city_map_radius_sq_set(pcity, radius_sq);
    idex_register_city(&wld, pcity);
  } else if (pcity->id != packet->id) {
    log_error("handle_city_short_info() city id %d != id %d.",
              pcity->id, packet->id);
    return;
  } else if (city_tile(pcity) != pcenter) {
    log_error("handle_city_short_info() city tile (%d, %d) != (%d, %d).",
              TILE_XY(city_tile(pcity)), TILE_XY(pcenter));
    return;
  } else {
    name_changed = (0 != strncmp(packet->name, pcity->name,
                                 sizeof(pcity->name)));

    /* Check if city descriptions should be updated */
    if (gui_options.draw_city_names && name_changed) {
      update_descriptions = TRUE;
    }

    sz_strlcpy(pcity->name, packet->name);
    
    memset(pcity->feel, 0, sizeof(pcity->feel));
    memset(pcity->specialists, 0, sizeof(pcity->specialists));
  }

  pcity->specialists[DEFAULT_SPECIALIST] = packet->size;
  city_size_set(pcity, packet->size);

  /* We can't actually see the internals of the city, but the server tells
   * us this much. */
  if (pcity->client.occupied != packet->occupied) {
    pcity->client.occupied = packet->occupied;
    if (gui_options.draw_full_citybar) {
      update_descriptions = TRUE;
    }
  }
  pcity->client.walls = packet->walls;
  pcity->style = packet->style;
  pcity->client.city_image = packet->city_image;

  pcity->client.happy = packet->happy;
  pcity->client.unhappy = packet->unhappy;

  improvement_iterate(pimprove) {
    /* Don't update the non-visible improvements, they could hide the
     * previously seen informations about the city (diplomat investigation).
     */
    if (is_improvement_visible(pimprove)) {
      bool have = BV_ISSET(packet->improvements,
                           improvement_index(pimprove));
      update_improvement_from_packet(pcity, pimprove, have);
    }
  } improvement_iterate_end;

  city_packet_common(pcity, pcenter, powner, worked_tiles,
                     city_is_new, FALSE, FALSE);

  if (city_is_new && !city_has_changed_owner) {
    agents_city_new(pcity);
  } else {
    agents_city_changed(pcity);
  }

  /* Update the description if necessary. */
  if (update_descriptions) {
    update_city_description(pcity);
  }
}

/************************************************************************//**
  Handle worker task assigned to the city
****************************************************************************/
void handle_worker_task(const struct packet_worker_task *packet)
{
  struct city *pcity = game_city_by_number(packet->city_id);
  struct worker_task *ptask = NULL;

  if (pcity == NULL
      || (pcity->owner != client.conn.playing && !client_is_global_observer())) {
    return;
  }

  worker_task_list_iterate(pcity->task_reqs, ptask_old) {
    if (tile_index(ptask_old->ptile) == packet->tile_id) {
      ptask = ptask_old;
      break;
    }
  } worker_task_list_iterate_end;

  if (ptask == NULL) {
    if (packet->activity == ACTIVITY_LAST) {
      return;
    } else {
      ptask = fc_malloc(sizeof(struct worker_task));
      worker_task_list_append(pcity->task_reqs, ptask);
    }
  } else {
    if (packet->activity == ACTIVITY_LAST) {
      worker_task_list_remove(pcity->task_reqs, ptask);
      free(ptask);
      ptask = NULL;
    }
  }

  if (ptask != NULL) {
    ptask->ptile = index_to_tile(&(wld.map), packet->tile_id);
    ptask->act = packet->activity;
    if (packet->tgt >= 0) {
      ptask->tgt = extra_by_number(packet->tgt);
    } else {
      ptask->tgt = NULL;
    }
    ptask->want = packet->want;
  }

  refresh_city_dialog(pcity);
}

/************************************************************************//**
  Handle turn and year advancement.
****************************************************************************/
void handle_new_year(int year, int fragments, int turn)
{
  game.info.year = year;
  game.info.fragment_count = fragments;
  /*
   * The turn was increased in handle_end_turn()
   */
  fc_assert(game.info.turn == turn);
  update_info_label();

  unit_focus_update();
  auto_center_on_focus_unit();

  update_unit_info_label(get_units_in_focus());
  menus_update();

  set_seconds_to_turndone(current_turn_timeout());

#if 0
  /* This information shouldn't be needed, but if it is this is the only
   * way we can get it. */
  if (NULL != client.conn.playing) {
    turn_gold_difference =
      client.conn.playing->economic.gold - last_turn_gold_amount;
    last_turn_gold_amount = client.conn.playing->economic.gold;
  }
#endif

  update_city_descriptions();
  link_marks_decrease_turn_counters();

  if (gui_options.sound_bell_at_new_turn) {
    create_event(NULL, E_TURN_BELL, ftc_client,
                 _("Start of turn %d"), game.info.turn);
  }

  agents_new_turn();
}

/************************************************************************//**
  Called by the network code when an end-phase packet is received.  This
  signifies the end of our phase (it's not sent for other player's
  phases).
****************************************************************************/
void handle_end_phase(void)
{
  /* Messagewindow will contain events happened since our own phase ended,
   * so player of the first phase and last phase are in equal situation. */
  meswin_clear_older(game.info.turn, game.info.phase);
}

/************************************************************************//**
  Called by the network code when an start-phase packet is received.  This
  may be the start of our phase or someone else's phase.
****************************************************************************/
void handle_start_phase(int phase)
{
  if (!client_has_player() && !client_is_observer()) {
    /* We are on detached state, let ignore this packet. */
    return;
  }

  if (phase < 0
      || (game.info.phase_mode == PMT_PLAYERS_ALTERNATE
          && phase >= player_count())
      || (game.info.phase_mode == PMT_TEAMS_ALTERNATE
          && phase >= team_count())) {
    log_error("handle_start_phase() illegal phase %d.", phase);
    return;
  }

  set_client_state(C_S_RUNNING);

  game.info.phase = phase;

  /* Possibly replace wait cursor with something else */
  if (phase == 0) {
    /* TODO: Have server set as busy also if switching phase
     * is taking long in a alternating phases mode. */
    set_server_busy(FALSE);
  }

  if (NULL != client.conn.playing
      && is_player_phase(client.conn.playing, phase)) {
    agents_start_turn();
    non_ai_unit_focus = FALSE;

    update_turn_done_button_state();

    if (is_ai(client.conn.playing)
        && !gui_options.ai_manual_turn_done) {
      user_ended_turn();
    }

    unit_focus_set_status(client.conn.playing);

    city_list_iterate(client.conn.playing->cities, pcity) {
      pcity->client.colored = FALSE;
    } city_list_iterate_end;

    unit_list_iterate(client.conn.playing->units, punit) {
      punit->client.colored = FALSE;
    } unit_list_iterate_end;

    update_map_canvas_visible();
  }

  update_info_label();
}

/************************************************************************//**
  Called when begin-turn packet is received. Server has finished processing
  turn change.
****************************************************************************/
void handle_begin_turn(void)
{
  log_debug("handle_begin_turn()");

  /* Server is still considered busy until it handles also the beginning
   * of the first phase. */

  stop_turn_change_wait();
}

/************************************************************************//**
  Called when end-turn packet is received. Server starts processing turn
  change.
****************************************************************************/
void handle_end_turn(void)
{
  log_debug("handle_end_turn()");

  /* Make sure wait cursor is in use */
  set_server_busy(TRUE);

  start_turn_change_wait();

  /*
   * The local idea of the game.info.turn is increased here since the
   * client will get unit updates (reset of move points for example)
   * between handle_end_turn() and handle_new_year(). These
   * unit updates will look like they did take place in the old turn
   * which is incorrect. If we get the authoritative information about
   * the game.info.turn in handle_new_year() we will check it.
   */
  game.info.turn++;
  agents_before_new_turn();
}

/************************************************************************//**
  Plays sound associated with event
****************************************************************************/
void play_sound_for_event(enum event_type type)
{
  const char *sound_tag = get_event_tag(type);

  if (sound_tag) {
    audio_play_sound(sound_tag, NULL);
  }
}

/************************************************************************//**
  Handle a message packet.  This includes all messages - both
  in-game messages and chats from other players.
****************************************************************************/
void handle_chat_msg(const struct packet_chat_msg *packet)
{
  handle_event(packet->message,
               index_to_tile(&(wld.map), packet->tile),
               packet->event,
               packet->turn,
               packet->phase,
               packet->conn_id);
}

/************************************************************************//**
  Handle an early message packet. Thease have format like other chat
  messages but server sends them only about events related to establishing
  the connection and other setup in the early phase. They are a separate
  packet just so that client knows thse to be already relevant when it's
  only setting itself up - other chat messages might be just something
  sent to all clients, and we might want to still consider ourselves
  "not connected" (not receivers of those messages) until we are fully
  in the game.
****************************************************************************/
void handle_early_chat_msg(const struct packet_early_chat_msg *packet)
{
  handle_event(packet->message,
               index_to_tile(&(wld.map), packet->tile),
               packet->event,
               packet->turn,
               packet->phase,
               packet->conn_id);
}

/************************************************************************//**
  Handle a connect message packet. Server sends connect message to
  client immediately when client connects.
****************************************************************************/
void handle_connect_msg(const char *message)
{
  popup_connect_msg(_("Welcome"), message);
}

/************************************************************************//**
  Handle server info packet. Server sends info packet as soon as it knows
  client to be compatible.
****************************************************************************/
void handle_server_info(const char *version_label, int major_version,
                        int minor_version, int patch_version, int emerg_version)
{
  if (emerg_version > 0) {
    log_verbose("Server has version %d.%d.%d.%d%s",
                major_version, minor_version, patch_version, emerg_version,
                version_label);
  } else {
    log_verbose("Server has version %d.%d.%d%s",
                major_version, minor_version, patch_version, version_label);
  }
}

/************************************************************************//**
  Page_msg header handler.
****************************************************************************/
void handle_page_msg(const char *caption, const char *headline,
                     enum event_type event, int len, int parts)
{
  if (!client_has_player()
      || is_human(client_player())
      || event != E_BROADCAST_REPORT) {
    if (page_msg_report.parts > 0) {
      /* Previous one was never finished */
      free(page_msg_report.caption);
      free(page_msg_report.headline);
      free(page_msg_report.lines);
    }
    page_msg_report.len = len;
    page_msg_report.event = event;
    page_msg_report.caption = fc_strdup(caption);
    page_msg_report.headline = fc_strdup(headline);
    page_msg_report.parts = parts;
    page_msg_report.lines = fc_malloc(len + 1);
    page_msg_report.lines[0] = '\0';

    if (parts == 0) {
      /* Empty report - handle as if last part was just received. */
      page_msg_report.parts = 1;
      handle_page_msg_part("");
    }
  }
}

/************************************************************************//**
  Page_msg part handler.
****************************************************************************/
void handle_page_msg_part(const char *lines)
{
  if (page_msg_report.lines != NULL) {
    /* We have already decided to show the message at the time we got
     * the header packet. */
    fc_strlcat(page_msg_report.lines, lines, page_msg_report.len + 1);
    page_msg_report.parts--;

    if (page_msg_report.parts == 0) {
      /* This is the final part */
      popup_notify_dialog(page_msg_report.caption,
                          page_msg_report.headline,
                          page_msg_report.lines);
      play_sound_for_event(page_msg_report.event);

      free(page_msg_report.caption);
      free(page_msg_report.headline);
      free(page_msg_report.lines);
      page_msg_report.lines = NULL;
    }
  }
}

/************************************************************************//**
  Packet unit_info.
****************************************************************************/
void handle_unit_info(const struct packet_unit_info *packet)
{
  struct unit *punit;

  punit = unpackage_unit(packet);
  if (handle_unit_packet_common(punit)) {
    punit->client.transported_by = -1;
    unit_virtual_destroy(punit);
  }
}

/************************************************************************//**
  Called to do basic handling for a unit_info or short_unit_info packet.

  Both owned and foreign units are handled; you may need to check unit
  owner, or if unit equals focus unit, depending on what you are doing.

  Note: Normally the server informs client about a new "activity" here.
  For owned units, the new activity can be a result of:
  - The player issued a command (a request) with the client.
  - The server side AI did something.
  - An enemy encounter caused a sentry to idle. (See "Wakeup Focus").

  Depending on what caused the change, different actions may be taken.
  Therefore, this function is a bit of a jungle, and it is advisable
  to read thoroughly before changing.
****************************************************************************/
static bool handle_unit_packet_common(struct unit *packet_unit)
{
  struct city *pcity;
  struct unit *punit;
  bool need_menus_update = FALSE;
  bool need_economy_report_update = FALSE;
  bool need_units_report_update = FALSE;
  bool repaint_unit = FALSE;
  bool repaint_city = FALSE;	/* regards unit's homecity */
  struct tile *old_tile = NULL;
  bool check_focus = FALSE;     /* conservative focus change */
  bool moved = FALSE;
  bool ret = FALSE;

  punit = player_unit_by_number(unit_owner(packet_unit), packet_unit->id);
  if (!punit && game_unit_by_number(packet_unit->id)) {
    /* This means unit has changed owner. We deal with this here
     * by simply deleting the old one and creating a new one. */
    handle_unit_remove(packet_unit->id);
  }

  if (punit) {
    /* In some situations, the size of repaint units require can change;
     * in particular, city-builder units sometimes get a potential-city
     * outline, but to speed up redraws we don't repaint this whole area
     * unnecessarily. We need to ensure that when the footprint shrinks,
     * old bits aren't left behind on the canvas.
     * If the current (old) status of the unit is such that it gets a large
     * repaint, as a special case, queue a large repaint immediately, to
     * schedule the correct amount/location to be redrawn; but rely on the
     * repaint being deferred until the unit is updated, so that what's
     * drawn reflects the new status (e.g., no city outline). */
    if (unit_drawn_with_city_outline(punit, TRUE)) {
      refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);
    }

    ret = TRUE;
    punit->activity_count = packet_unit->activity_count;
    unit_change_battlegroup(punit, packet_unit->battlegroup);
    if (punit->ai_controlled != packet_unit->ai_controlled) {
      punit->ai_controlled = packet_unit->ai_controlled;
      repaint_unit = TRUE;
      /* AI is set:     may change focus */
      /* AI is cleared: keep focus */
      if (packet_unit->ai_controlled && unit_is_in_focus(punit)) {
        check_focus = TRUE;
      }
    }

    if (punit->facing != packet_unit->facing) {
      punit->facing = packet_unit->facing;
      repaint_unit = TRUE;
    }

    if (punit->activity != packet_unit->activity
        || punit->activity_target == packet_unit->activity_target
        || punit->client.transported_by != packet_unit->client.transported_by
        || punit->client.occupied != packet_unit->client.occupied
	|| punit->has_orders != packet_unit->has_orders
	|| punit->orders.repeat != packet_unit->orders.repeat
	|| punit->orders.vigilant != packet_unit->orders.vigilant
	|| punit->orders.index != packet_unit->orders.index) {

      /*** Change in activity or activity's target. ***/

      /* May change focus if focus unit gets a new activity.
       * But if new activity is Idle, it means user specifically selected
       * the unit */
      if (unit_is_in_focus(punit)
	  && (packet_unit->activity != ACTIVITY_IDLE
	      || packet_unit->has_orders)) {
        check_focus = TRUE;
      }

      repaint_unit = TRUE;

      /* Wakeup Focus */
      if (gui_options.wakeup_focus 
          && NULL != client.conn.playing
          && is_human(client.conn.playing)
          && unit_owner(punit) == client.conn.playing
          && punit->activity == ACTIVITY_SENTRY
          && packet_unit->activity == ACTIVITY_IDLE
          && !unit_is_in_focus(punit)
          && is_player_phase(client.conn.playing, game.info.phase)) {
        /* many wakeup units per tile are handled */
        unit_focus_urgent(punit);
        check_focus = FALSE; /* and keep it */
      }

      punit->activity = packet_unit->activity;
      punit->activity_target = packet_unit->activity_target;

      if (punit->client.transported_by
          != packet_unit->client.transported_by) {
        if (packet_unit->client.transported_by == -1) {
          /* The unit was unloaded from its transport. The check for a new
           * transport is done below. */
          unit_transport_unload(punit);
        }

        punit->client.transported_by = packet_unit->client.transported_by;
      }

      if (punit->client.occupied != packet_unit->client.occupied) {
        if (get_focus_unit_on_tile(unit_tile(packet_unit))) {
          /* Special case: (un)loading a unit in a transporter on the same
           *tile as the focus unit may (dis)allow the focus unit to be
           * loaded.  Thus the orders->(un)load menu item needs updating. */
          need_menus_update = TRUE;
        }
        punit->client.occupied = packet_unit->client.occupied;
      }

      punit->has_orders = packet_unit->has_orders;
      punit->orders.length = packet_unit->orders.length;
      punit->orders.index = packet_unit->orders.index;
      punit->orders.repeat = packet_unit->orders.repeat;
      punit->orders.vigilant = packet_unit->orders.vigilant;

      /* We cheat by just stealing the packet unit's list. */
      if (punit->orders.list) {
	free(punit->orders.list);
      }
      punit->orders.list = packet_unit->orders.list;
      packet_unit->orders.list = NULL;

      if (NULL == client.conn.playing
          || unit_owner(punit) == client.conn.playing) {
        refresh_unit_city_dialogs(punit);
      }
    } /*** End of Change in activity or activity's target. ***/

    /* These two lines force the menus to be updated as appropriate when
     * the focus unit changes. */
    if (unit_is_in_focus(punit)) {
      need_menus_update = TRUE;
    }

    if (punit->homecity != packet_unit->homecity) {
      /* change homecity */
      struct city *hcity;

      if ((hcity = game_city_by_number(punit->homecity))) {
	unit_list_remove(hcity->units_supported, punit);
	refresh_city_dialog(hcity);
      }

      punit->homecity = packet_unit->homecity;
      if ((hcity = game_city_by_number(punit->homecity))) {
	unit_list_prepend(hcity->units_supported, punit);
	repaint_city = TRUE;
      }

      /* This can change total upkeep figures */
      need_units_report_update = TRUE;
    }

    if (punit->hp != packet_unit->hp) {
      /* hp changed */
      punit->hp = packet_unit->hp;
      repaint_unit = TRUE;
    }

    if (punit->utype != unit_type_get(packet_unit)) {
      /* Unit type has changed (been upgraded) */
      struct city *ccity = tile_city(unit_tile(punit));

      punit->utype = unit_type_get(packet_unit);
      repaint_unit = TRUE;
      repaint_city = TRUE;
      if (ccity != NULL && (ccity->id != punit->homecity)) {
	refresh_city_dialog(ccity);
      }
      if (unit_is_in_focus(punit)) {
        /* Update the orders menu -- the unit might have new abilities */
        need_menus_update = TRUE;
      }
      need_units_report_update = TRUE;
    }

    /* May change focus if an attempted move or attack exhausted unit */
    if (punit->moves_left != packet_unit->moves_left
        && unit_is_in_focus(punit)) {
      check_focus = TRUE;
    }

    if (!same_pos(unit_tile(punit), unit_tile(packet_unit))) {
      /*** Change position ***/
      struct city *ccity = tile_city(unit_tile(punit));

      old_tile = unit_tile(punit);
      moved = TRUE;

      /* Show where the unit is going. */
      do_move_unit(punit, packet_unit);

      if (ccity != NULL)  {
	if (can_player_see_units_in_city(client.conn.playing, ccity)) {
	  /* Unit moved out of a city - update the occupied status. */
	  bool new_occupied =
	    (unit_list_size(ccity->tile->units) > 0);

          if (ccity->client.occupied != new_occupied) {
            ccity->client.occupied = new_occupied;
            refresh_city_mapcanvas(ccity, ccity->tile, FALSE, FALSE);
            if (gui_options.draw_full_citybar) {
              update_city_description(ccity);
            }
          }
        }

        if (ccity->id == punit->homecity) {
          repaint_city = TRUE;
        } else {
          refresh_city_dialog(ccity);
        }
      }

      if ((ccity = tile_city(unit_tile(punit)))) {
        if (can_player_see_units_in_city(client.conn.playing, ccity)) {
          /* Unit moved into a city - obviously it's occupied. */
          if (!ccity->client.occupied) {
            ccity->client.occupied = TRUE;
            refresh_city_mapcanvas(ccity, ccity->tile, FALSE, FALSE);
            if (gui_options.draw_full_citybar) {
              update_city_description(ccity);
            }
          }
        }

        if (ccity->id == punit->homecity) {
          repaint_city = TRUE;
        } else {
          refresh_city_dialog(ccity);
        }
      }

    }  /*** End of Change position. ***/

    if (repaint_city || repaint_unit) {
      /* We repaint the city if the unit itself needs repainting or if
       * there is a special city-only redrawing to be done. */
      if ((pcity = game_city_by_number(punit->homecity))) {
	refresh_city_dialog(pcity);
      }
      if (repaint_unit && tile_city(unit_tile(punit))
          && tile_city(unit_tile(punit)) != pcity) {
        /* Refresh the city we're occupying too. */
        refresh_city_dialog(tile_city(unit_tile(punit)));
      }
    }

    need_economy_report_update = (punit->upkeep[O_GOLD]
                                  != packet_unit->upkeep[O_GOLD]);
    /* unit upkeep information */
    output_type_iterate(o) {
      punit->upkeep[o] = packet_unit->upkeep[o];
    } output_type_iterate_end;

    punit->nationality = packet_unit->nationality;
    punit->veteran = packet_unit->veteran;
    punit->moves_left = packet_unit->moves_left;
    punit->fuel = packet_unit->fuel;
    punit->goto_tile = packet_unit->goto_tile;
    punit->paradropped = packet_unit->paradropped;
    punit->stay = packet_unit->stay;
    if (punit->done_moving != packet_unit->done_moving) {
      punit->done_moving = packet_unit->done_moving;
      check_focus = TRUE;
    }

    /* This won't change punit; it enqueues the call for later handling. */
    agents_unit_changed(punit);
    editgui_notify_object_changed(OBJTYPE_UNIT, punit->id, FALSE);

    punit->action_decision_tile = packet_unit->action_decision_tile;
    if (punit->action_decision_want != packet_unit->action_decision_want
        && should_ask_server_for_actions(packet_unit)) {
      /* The unit wants the player to decide. */
      action_decision_request(punit);
      check_focus = TRUE;
    }
    punit->action_decision_want = packet_unit->action_decision_want;
  } else {
    /*** Create new unit ***/
    punit = packet_unit;
    idex_register_unit(&wld, punit);

    unit_list_prepend(unit_owner(punit)->units, punit);
    unit_list_prepend(unit_tile(punit)->units, punit);

    unit_register_battlegroup(punit);

    if ((pcity = game_city_by_number(punit->homecity))) {
      unit_list_prepend(pcity->units_supported, punit);
    }

    log_debug("New %s %s id %d (%d %d) hc %d %s",
              nation_rule_name(nation_of_unit(punit)),
              unit_rule_name(punit), TILE_XY(unit_tile(punit)),
              punit->id, punit->homecity,
              (pcity ? city_name_get(pcity) : "(unknown)"));

    repaint_unit = !unit_transported(punit);
    agents_unit_new(punit);

    /* Check if we should link cargo units.
     * (This might be necessary if the cargo info was sent to us before
     * this transporter.) */
    if (punit->client.occupied) {
      unit_list_iterate(unit_tile(punit)->units, aunit) {
        if (aunit->client.transported_by == punit->id) {
          fc_assert(aunit->transporter == NULL);
          unit_transport_load(aunit, punit, TRUE);
        }
      } unit_list_iterate_end;
    }

    if ((pcity = tile_city(unit_tile(punit)))) {
      /* The unit is in a city - obviously it's occupied. */
      pcity->client.occupied = TRUE;
    }

    if (should_ask_server_for_actions(punit)) {
      /* The unit wants the player to decide. */
      action_decision_request(punit);
      check_focus = TRUE;
    }

    need_units_report_update = TRUE;
  } /*** End of Create new unit ***/

  fc_assert_ret_val(punit != NULL, ret);

  /* Check if we have to load the unit on a transporter. */
  if (punit->client.transported_by != -1) {
    struct unit *ptrans
      = game_unit_by_number(packet_unit->client.transported_by);

    /* Load unit only if transporter is known by the client.
     * (If not, cargo will be loaded later when the transporter info is
     * sent to the client.) */
    if (ptrans && ptrans != unit_transport_get(punit)) {
      /* First, we have to unload the unit from its old transporter. */
      unit_transport_unload(punit);
      unit_transport_load(punit, ptrans, TRUE);
#ifdef DEBUG_TRANSPORT
      log_debug("load %s (ID: %d) onto %s (ID: %d)",
                unit_name_translation(punit), punit->id,
                unit_name_translation(ptrans), ptrans->id);
    } else if (ptrans && ptrans == unit_transport_get(punit)) {
      log_debug("%s (ID: %d) is loaded onto %s (ID: %d)",
                unit_name_translation(punit), punit->id,
                unit_name_translation(ptrans), ptrans->id);
    } else {
      log_debug("%s (ID: %d) is not loaded", unit_name_translation(punit),
                punit->id);
#endif /* DEBUG_TRANSPORT */
    }
  }

  if (unit_is_in_focus(punit)
      || get_focus_unit_on_tile(unit_tile(punit))
      || (moved && get_focus_unit_on_tile(old_tile))) {
    update_unit_info_label(get_units_in_focus());
    /* Update (an possible active) unit select dialog. */
    unit_select_dialog_update();
  }

  if (repaint_unit) {
    refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);
  }

  if ((check_focus || get_num_units_in_focus() == 0)
      && NULL != client.conn.playing
      && is_human(client.conn.playing)
      && is_player_phase(client.conn.playing, game.info.phase)) {
    unit_focus_update();
  }

  if (need_menus_update) {
    menus_update();
  }

  if (!client_has_player() || unit_owner(punit) == client_player()) {
    if (need_economy_report_update) {
      economy_report_dialog_update();
    }
    if (need_units_report_update) {
      units_report_dialog_update();
    }
  }

  return ret;
}

/************************************************************************//**
  Receive a short_unit info packet.
****************************************************************************/
void handle_unit_short_info(const struct packet_unit_short_info *packet)
{
  struct city *pcity;
  struct unit *punit;

  /* Special case for a diplomat/spy investigating a city: The investigator
   * needs to know the supported and present units of a city, whether or not
   * they are fogged. So, we send a list of them all before sending the city
   * info. */
  if (packet->packet_use == UNIT_INFO_CITY_SUPPORTED
      || packet->packet_use == UNIT_INFO_CITY_PRESENT) {
    static int last_serial_num = 0;

    pcity = game_city_by_number(packet->info_city_id);
    if (!pcity) {
      log_error("Investigate city: unknown city id %d!",
                packet->info_city_id);
      return;
    }

    /* New serial number: start collecting supported and present units. */
    if (last_serial_num
        != client.conn.client.request_id_of_currently_handled_packet) {
      last_serial_num =
          client.conn.client.request_id_of_currently_handled_packet;
      /* Ensure we are not already in an investigate cycle. */
      fc_assert(pcity->client.collecting_info_units_supported == NULL);
      fc_assert(pcity->client.collecting_info_units_present == NULL);
      pcity->client.collecting_info_units_supported =
          unit_list_new_full(unit_virtual_destroy);
      pcity->client.collecting_info_units_present =
          unit_list_new_full(unit_virtual_destroy);
    }

    /* Okay, append a unit struct to the proper list. */
    punit = unpackage_short_unit(packet);
    if (packet->packet_use == UNIT_INFO_CITY_SUPPORTED) {
      fc_assert(pcity->client.collecting_info_units_supported != NULL);
      unit_list_append(pcity->client.collecting_info_units_supported, punit);
    } else {
      fc_assert(packet->packet_use == UNIT_INFO_CITY_PRESENT);
      fc_assert(pcity->client.collecting_info_units_present != NULL);
      unit_list_append(pcity->client.collecting_info_units_present, punit);
    }

    /* Done with special case. */
    return;
  }

  if (player_by_number(packet->owner) == client.conn.playing) {
    log_error("handle_unit_short_info() for own unit.");
  }

  punit = unpackage_short_unit(packet);
  if (handle_unit_packet_common(punit)) {
    punit->client.transported_by = -1;
    unit_virtual_destroy(punit);
  }
}

/************************************************************************//**
  Server requested topology change.
****************************************************************************/
void handle_set_topology(int topology_id)
{
  wld.map.topology_id = topology_id;

  if (forced_tileset_name[0] == '\0'
      && (tileset_map_topo_compatible(topology_id, tileset) == TOPO_INCOMP_HARD
          || strcmp(tileset_basename(tileset), game.control.preferred_tileset))) {
    const char *ts_to_load;

    ts_to_load = tileset_name_for_topology(topology_id);

    if (ts_to_load != NULL && ts_to_load[0] != '\0') {
      tilespec_reread_frozen_refresh(ts_to_load);
    }
  }
}

/************************************************************************//**
  Receive information about the map size and topology from the server.  We
  initialize some global variables at the same time.
****************************************************************************/
void handle_map_info(int xsize, int ysize, int topology_id)
{
  if (!map_is_empty()) {
    map_free(&(wld.map));
    free_city_map_index();
  }

  wld.map.xsize = xsize;
  wld.map.ysize = ysize;

  if (tileset_map_topo_compatible(topology_id, tileset) == TOPO_INCOMP_HARD) {
    tileset_error(LOG_NORMAL, _("Map topology and tileset incompatible."));
  }

  wld.map.topology_id = topology_id;

  map_init_topology();
  main_map_allocate();
  client_player_maps_reset();
  init_client_goto();
  mapdeco_init();

  generate_citydlg_dimensions();

  calculate_overview_dimensions();

  packhand_init();
}

/************************************************************************//**
  Packet game_info handler.
****************************************************************************/
void handle_game_info(const struct packet_game_info *pinfo)
{
  bool boot_help;
  bool update_aifill_button = FALSE, update_ai_skill_level = FALSE;

  if (game.info.aifill != pinfo->aifill) {
    update_aifill_button = TRUE;
  }
  if (game.info.skill_level != pinfo->skill_level) {
    update_ai_skill_level = TRUE;
  }

  if (game.info.is_edit_mode != pinfo->is_edit_mode) {
    popdown_all_city_dialogs();
    /* Clears the current goto command. */
    clear_hover_state();

    if (pinfo->is_edit_mode && game.scenario.handmade) {
      if (!handmade_scenario_warning()) {
        /* Gui didn't handle this */
        output_window_append(ftc_client,
                             _("This scenario may have manually set properties the editor "
                               "cannot handle."));
        output_window_append(ftc_client,
                             _("They won't be saved when scenario is saved from the editor."));
      }
    }
  }

  game.info = *pinfo;

  /* check the values! */
#define VALIDATE(_count, _maximum, _string)                                 \
  if (game.info._count > _maximum) {                                        \
    log_error("handle_game_info(): Too many " _string "; using %d of %d",   \
              _maximum, game.info._count);                                  \
    game.info._count = _maximum;                                            \
  }

  VALIDATE(granary_num_inis,	MAX_GRANARY_INIS,	"granary entries");
#undef VALIDATE

  game.default_government =
    government_by_number(game.info.default_government_id);
  game.government_during_revolution =
    government_by_number(game.info.government_during_revolution_id);

  boot_help = (can_client_change_view()
	       && game.info.victory_conditions != pinfo->victory_conditions);
  if (boot_help) {
    boot_help_texts(); /* reboot, after setting game.spacerace */
  }
  unit_focus_update();
  menus_update();
  players_dialog_update();
  if (update_aifill_button || update_ai_skill_level) {
    update_start_page();
  }
  
  if (can_client_change_view()) {
    update_info_label();
  }

  editgui_notify_object_changed(OBJTYPE_GAME, 1, FALSE);
}

/************************************************************************//**
  Packet calendar_info handler.
****************************************************************************/
void handle_calendar_info(const struct packet_calendar_info *pcalendar)
{
  game.calendar = *pcalendar;
}

/************************************************************************//**
  Sets the remaining turn time.
****************************************************************************/
void handle_timeout_info(float seconds_to_phasedone, float last_turn_change_time)
{
  if (current_turn_timeout() != 0 && seconds_to_phasedone >= 0) {
    /* If this packet is received in the middle of a turn, this value
     * represents the number of seconds from now to the end of the turn
     * (not from the start of the turn). So we need to restart our
     * timer. */
    set_seconds_to_turndone(seconds_to_phasedone);
  }

  game.tinfo.last_turn_change_time = last_turn_change_time;
}

/************************************************************************//**
  Sets the target government.  This will automatically start a revolution
  if the target government differs from the current one.
****************************************************************************/
void set_government_choice(struct government *government)
{
  if (NULL != client.conn.playing
      && can_client_issue_orders()
      && government != government_of_player(client.conn.playing)) {
    dsend_packet_player_change_government(&client.conn, government_number(government));
  }
}

/************************************************************************//**
  Begin a revolution by telling the server to start it.  This also clears
  the current government choice.
****************************************************************************/
void start_revolution(void)
{
  dsend_packet_player_change_government(&client.conn,
                                        game.info.government_during_revolution_id);
}

/************************************************************************//**
  Handle a notification that the player slot identified by 'playerno' has
  become unused. If the slot is already unused, then just ignore. Otherwise
  update the total player count and the GUI.
****************************************************************************/
void handle_player_remove(int playerno)
{
  struct player_slot *pslot;
  struct player *pplayer;
  int plr_nbr;

  pslot = player_slot_by_number(playerno);

  if (NULL == pslot || !player_slot_is_used(pslot)) {
    /* Ok, just ignore. */
    return;
  }

  pplayer = player_slot_get_player(pslot);

  if (can_client_change_view()) {
    close_intel_dialog(pplayer);
  }

  /* Update the connection informations. */
  if (client_player() == pplayer) {
    client.conn.playing = NULL;
  }
  conn_list_iterate(pplayer->connections, pconn) {
    pconn->playing = NULL;
  } conn_list_iterate_end;
  conn_list_clear(pplayer->connections);

  /* Save player number before player is freed */
  plr_nbr = player_number(pplayer);

  player_destroy(pplayer);

  players_dialog_update();
  conn_list_dialog_update();

  editgui_refresh();
  editgui_notify_object_changed(OBJTYPE_PLAYER, plr_nbr, TRUE);
}

/************************************************************************//**
  Handle information about a player. If the packet refers to a player slot
  that is not currently used, then this function will set that slot to
  used and update the total player count.
****************************************************************************/
void handle_player_info(const struct packet_player_info *pinfo)
{
  bool is_new_nation = FALSE;
  bool turn_done_changed = FALSE;
  bool new_player = FALSE;
  int i;
  struct player *pplayer, *my_player;
  struct nation_type *pnation;
  struct government *pgov, *ptarget_gov;
  struct player_slot *pslot;
  struct team_slot *tslot;

  /* Player. */
  pslot = player_slot_by_number(pinfo->playerno);
  fc_assert(NULL != pslot);
  new_player = !player_slot_is_used(pslot);
  pplayer = player_new(pslot);

  if ((pplayer->rgb == NULL) != !pinfo->color_valid
      || (pinfo->color_valid &&
          (pplayer->rgb->r != pinfo->color_red
           || pplayer->rgb->g != pinfo->color_green
           || pplayer->rgb->b != pinfo->color_blue))) {
    struct rgbcolor *prgbcolor;

    if (pinfo->color_valid) {
      prgbcolor = rgbcolor_new(pinfo->color_red,
                               pinfo->color_green,
                               pinfo->color_blue);
      fc_assert_ret(prgbcolor != NULL);
    } else {
      prgbcolor = NULL;
    }

    player_set_color(pplayer, prgbcolor);
    tileset_player_init(tileset, pplayer);

    rgbcolor_destroy(prgbcolor);

    /* Queue a map update -- may need to redraw borders, etc. */
    update_map_canvas_visible();
  }
  pplayer->client.color_changeable = pinfo->color_changeable;

  if (new_player) {
    /* Initialise client side player data (tile vision). At the moment
     * redundant as the values are initialised with 0 due to fc_calloc(). */
    client_player_init(pplayer);
  }

  /* Team. */
  tslot = team_slot_by_number(pinfo->team);
  fc_assert(NULL != tslot);
  team_add_player(pplayer, team_new(tslot));

  pnation = nation_by_number(pinfo->nation);
  pgov = government_by_number(pinfo->government);
  ptarget_gov = government_by_number(pinfo->target_government);

  /* Now update the player information. */
  sz_strlcpy(pplayer->name, pinfo->name);
  sz_strlcpy(pplayer->username, pinfo->username);
  pplayer->unassigned_user = pinfo->unassigned_user;

  is_new_nation = player_set_nation(pplayer, pnation);
  pplayer->is_male = pinfo->is_male;
  pplayer->score.game = pinfo->score;
  pplayer->was_created = pinfo->was_created;

  pplayer->economic.gold = pinfo->gold;
  pplayer->economic.tax = pinfo->tax;
  pplayer->economic.science = pinfo->science;
  pplayer->economic.luxury = pinfo->luxury;
  pplayer->client.tech_upkeep = pinfo->tech_upkeep;
  pplayer->government = pgov;
  pplayer->target_government = ptarget_gov;
  /* Don't use player_iterate here, because we ignore the real number
   * of players and we want to read all the datas. */
  BV_CLR_ALL(pplayer->real_embassy);
  fc_assert(8 * sizeof(pplayer->real_embassy)
            >= ARRAY_SIZE(pinfo->real_embassy));
  for (i = 0; i < ARRAY_SIZE(pinfo->real_embassy); i++) {
    if (pinfo->real_embassy[i]) {
      BV_SET(pplayer->real_embassy, i);
    }
  }
  pplayer->gives_shared_vision = pinfo->gives_shared_vision;
  pplayer->style = style_by_number(pinfo->style);

  if (pplayer == client.conn.playing) {
    bool music_change = FALSE;

    if (pplayer->music_style != pinfo->music_style) {
      pplayer->music_style = pinfo->music_style;
      music_change = TRUE;
    }
    if (pplayer->client.mood != pinfo->mood) {
      pplayer->client.mood = pinfo->mood;
      music_change = TRUE;
    }

    if (music_change) {
      start_style_music();
    }
  }

  pplayer->culture = pinfo->culture;

  /* Don't use player_iterate or player_slot_count here, because we ignore
   * the real number of players and we want to read all the datas. */
  fc_assert(ARRAY_SIZE(pplayer->ai_common.love) >= ARRAY_SIZE(pinfo->love));
  for (i = 0; i < ARRAY_SIZE(pinfo->love); i++) {
    pplayer->ai_common.love[i] = pinfo->love[i];
  }

  my_player = client_player();

  pplayer->is_connected = pinfo->is_connected;

  for (i = 0; i < B_LAST; i++) {
    pplayer->wonders[i] = pinfo->wonders[i];
  }

  /* Set AI.control. */
  if (is_ai(pplayer) != BV_ISSET(pinfo->flags, PLRF_AI)) {
    BV_SET_VAL(pplayer->flags, PLRF_AI, BV_ISSET(pinfo->flags, PLRF_AI));
    if (pplayer == my_player)  {
      if (is_ai(my_player)) {
        output_window_append(ftc_client, _("AI mode is now ON."));
        if (!gui_options.ai_manual_turn_done && !pplayer->phase_done) {
          /* End turn immediately */
          user_ended_turn();
        }
      } else {
        output_window_append(ftc_client, _("AI mode is now OFF."));
      }
    }
  }

  pplayer->flags = pinfo->flags;

  pplayer->ai_common.science_cost = pinfo->science_cost;

  turn_done_changed = (pplayer->phase_done != pinfo->phase_done
                       || (BV_ISSET(pplayer->flags, PLRF_AI) !=
                           BV_ISSET(pinfo->flags, PLRF_AI)));
  pplayer->phase_done = pinfo->phase_done;

  pplayer->is_ready = pinfo->is_ready;
  pplayer->nturns_idle = pinfo->nturns_idle;
  pplayer->is_alive = pinfo->is_alive;
  pplayer->turns_alive = pinfo->turns_alive;
  pplayer->ai_common.barbarian_type = pinfo->barbarian_type;
  pplayer->revolution_finishes = pinfo->revolution_finishes;
  pplayer->ai_common.skill_level = pinfo->ai_skill_level;

  fc_assert(pinfo->multip_count == multiplier_count());
  game.control.num_multipliers = pinfo->multip_count;
  multipliers_iterate(pmul) {
    pplayer->multipliers[multiplier_index(pmul)] =
        pinfo->multiplier[multiplier_index(pmul)];
    pplayer->multipliers_target[multiplier_index(pmul)] =
        pinfo->multiplier_target[multiplier_index(pmul)];
  } multipliers_iterate_end;

  /* if the server requests that the client reset, then information about
   * connections to this player are lost. If this is the case, insert the
   * correct conn back into the player->connections list */
  if (conn_list_size(pplayer->connections) == 0) {
    conn_list_iterate(game.est_connections, pconn) {
      if (pplayer == pconn->playing) {
        /* insert the controller into first position */
        if (pconn->observer) {
          conn_list_append(pplayer->connections, pconn);
        } else {
          conn_list_prepend(pplayer->connections, pconn);
        }
      }
    } conn_list_iterate_end;
  }


  /* The player information is now fully set. Update the GUI. */

  if (pplayer == my_player && can_client_change_view()) {
    if (turn_done_changed) {
      update_turn_done_button_state();
    }
    science_report_dialog_update();
    economy_report_dialog_update();
    units_report_dialog_update();
    city_report_dialog_update();
    multipliers_dialog_update();
    update_info_label();
  }

  upgrade_canvas_clipboard();

  players_dialog_update();
  update_start_page();

  if (is_new_nation) {
    races_toggles_set_sensitive();

    /* When changing nation during a running game, some refreshing is needed.
     * This may not be the only one! */
    update_map_canvas_visible();
  }

  if (can_client_change_view()) {
    /* Just about any changes above require an update to the intelligence
     * dialog. */
    update_intel_dialog(pplayer);
  }

  editgui_refresh();
  editgui_notify_object_changed(OBJTYPE_PLAYER, player_number(pplayer),
                                FALSE);
}

/************************************************************************//**
  This is a packet that only the web-client needs. The regular client has no
  use for it.
  TODO: Do not generate code calling this in the C-client.
****************************************************************************/
void handle_web_player_info_addition(int playerno, int expected_income)
{
}

/************************************************************************//**
  Receive a research info packet.
****************************************************************************/
void handle_research_info(const struct packet_research_info *packet)
{
  struct research *presearch;
  bool tech_changed = FALSE;
  bool poptechup = FALSE;
  Tech_type_id gained_techs[advance_count()];
  int gained_techs_num = 0, i;
  enum tech_state newstate, oldstate;

#ifdef FREECIV_DEBUG
  log_verbose("Research nb %d inventions: %s",
              packet->id,
              packet->inventions);
#endif
  presearch = research_by_number(packet->id);
  fc_assert_ret(NULL != presearch);

  poptechup = (presearch->researching != packet->researching
               || presearch->tech_goal != packet->tech_goal);
  presearch->techs_researched = packet->techs_researched;
  if (presearch->future_tech == 0 && packet->future_tech > 0) {
    gained_techs[gained_techs_num++] = A_FUTURE;
  }
  presearch->future_tech = packet->future_tech;
  presearch->researching = packet->researching;
  presearch->client.researching_cost = packet->researching_cost;
  presearch->bulbs_researched = packet->bulbs_researched;
  presearch->tech_goal = packet->tech_goal;
  presearch->client.total_bulbs_prod = packet->total_bulbs_prod;

  advance_index_iterate(A_NONE, advi) {
    newstate = packet->inventions[advi] - '0';
    oldstate = research_invention_set(presearch, advi, newstate);

    if (newstate != oldstate) {
      if (TECH_KNOWN == newstate) {
        tech_changed = TRUE;
        if (A_NONE != advi) {
          gained_techs[gained_techs_num++] = advi;
        }
      } else if (TECH_KNOWN == oldstate) {
        tech_changed = TRUE;
      }
    }
  } advance_index_iterate_end;

  research_update(presearch);

  if (C_S_RUNNING == client_state()) {
    if (presearch == research_get(client_player())) {
      if (poptechup && is_human(client_player())) {
        science_report_dialog_popup(FALSE);
      }
      science_report_dialog_update();
      if (tech_changed) {
        /* If we just learned bridge building and focus is on a settler
         * on a river the road menu item will remain disabled unless we
         * do this. (applies in other cases as well.) */
        if (0 < get_num_units_in_focus()) {
          menus_update();
        }
	script_client_signal_emit("new_tech");

        /* If we got a new tech the tech tree news an update. */
        science_report_dialog_redraw();
      }
      for (i = 0; i < gained_techs_num; i++) {
        show_tech_gained_dialog(gained_techs[i]);
      }
    }
    if (editor_is_active()) {
      editgui_refresh();
      research_players_iterate(presearch, pplayer) {
        editgui_notify_object_changed(OBJTYPE_PLAYER, player_number(pplayer),
                                      FALSE);
      } research_players_iterate_end;
    }
  }
}

/************************************************************************//**
  Packet player_diplstate handler.
****************************************************************************/
void handle_player_diplstate(const struct packet_player_diplstate *packet)
{
  struct player *plr1 = player_by_number(packet->plr1);
  struct player *plr2 = player_by_number(packet->plr2);
  struct player *my_player = client_player();
  struct player_diplstate *ds = player_diplstate_get(plr1, plr2);
  bool need_players_dialog_update = FALSE;

  fc_assert_ret(ds != NULL);

  if (client_has_player() && my_player == plr2) {
    if (ds->type != packet->type) {
      need_players_dialog_update = TRUE;
    }

    /* Check if we detect change to armistice with us. If so,
     * ready all units for movement out of the territory in
     * question; otherwise they will be disbanded. */
    if (DS_ARMISTICE != player_diplstate_get(plr1, my_player)->type
        && DS_ARMISTICE == packet->type) {
      unit_list_iterate(my_player->units, punit) {
        if (!tile_owner(unit_tile(punit))
            || tile_owner(unit_tile(punit)) != plr1) {
          continue;
        }
        if (punit->client.focus_status == FOCUS_WAIT) {
          punit->client.focus_status = FOCUS_AVAIL;
        }
        if (punit->activity != ACTIVITY_IDLE) {
          request_new_unit_activity(punit, ACTIVITY_IDLE);
        }
      } unit_list_iterate_end;
    }
  }

  ds->type = packet->type;
  ds->turns_left = packet->turns_left;
  ds->has_reason_to_cancel = packet->has_reason_to_cancel;
  ds->contact_turns_left = packet->contact_turns_left;

  if (need_players_dialog_update) {
    players_dialog_update();
  }

  if (need_players_dialog_update
      && action_selection_actor_unit() != IDENTITY_NUMBER_ZERO) {
    /* An action selection dialog is open and our diplomatic state just
     * changed. Find out if the relationship that changed was to a
     * potential target. */
    struct tile *tgt_tile = NULL;

    /* Is a refresh needed because of a unit target? */
    if (action_selection_target_unit() != IDENTITY_NUMBER_ZERO) {
      struct unit *tgt_unit;

      tgt_unit = game_unit_by_number(action_selection_target_unit());

      if (tgt_unit != NULL && tgt_unit->owner == plr1) {
        /* An update is needed because of this unit target. */
        tgt_tile = unit_tile(tgt_unit);
        fc_assert(tgt_tile != NULL);
      }
    }

    /* Is a refresh needed because of a city target? */
    if (action_selection_target_city() != IDENTITY_NUMBER_ZERO) {
      struct city *tgt_city;

      tgt_city = game_city_by_number(action_selection_target_city());

      if (tgt_city != NULL && tgt_city->owner == plr1) {
        /* An update is needed because of this city target.
         * Overwrites any target tile from a unit. */
        tgt_tile = city_tile(tgt_city);
        fc_assert(tgt_tile != NULL);
      }
    }

    if (tgt_tile
        || ((tgt_tile = index_to_tile(&(wld.map),
                                      action_selection_target_tile()))
            && tile_owner(tgt_tile) == plr1)) {
      /* The diplomatic relationship to the target in an open action
       * selection dialog have changed. This probably changes
       * the set of available actions. */
      dsend_packet_unit_get_actions(&client.conn,
                                    action_selection_actor_unit(),
                                    action_selection_target_unit(),
                                    tgt_tile->index,
                                    action_selection_target_extra(),
                                    FALSE);
    }
  }
}

/************************************************************************//**
  Remove, add, or update dummy connection struct representing some
  connection to the server, with info from packet_conn_info.
  Updates player and game connection lists.
  Calls players_dialog_update() in case info for that has changed.
****************************************************************************/
void handle_conn_info(const struct packet_conn_info *pinfo)
{
  struct connection *pconn = conn_by_number(pinfo->id);
  bool preparing_client_state = FALSE;

  log_debug("conn_info id%d used%d est%d plr%d obs%d acc%d",
            pinfo->id, pinfo->used, pinfo->established, pinfo->player_num,
            pinfo->observer, (int) pinfo->access_level);
  log_debug("conn_info \"%s\" \"%s\" \"%s\"",
            pinfo->username, pinfo->addr, pinfo->capability);
  
  if (!pinfo->used) {
    /* Forget the connection */
    if (!pconn) {
      log_verbose("Server removed unknown connection %d", pinfo->id);
      return;
    }
    client_remove_cli_conn(pconn);
    pconn = NULL;
  } else {
    struct player_slot *pslot = player_slot_by_number(pinfo->player_num);
    struct player *pplayer = NULL;

    if (NULL != pslot) {
      pplayer = player_slot_get_player(pslot);
    }

    if (!pconn) {
      log_verbose("Server reports new connection %d %s",
                  pinfo->id, pinfo->username);

      pconn = fc_calloc(1, sizeof(struct connection));
      pconn->buffer = NULL;
      pconn->send_buffer = NULL;
      pconn->ping_time = -1.0;
      if (pplayer) {
        conn_list_append(pplayer->connections, pconn);
      }
      conn_list_append(game.all_connections, pconn);
      conn_list_append(game.est_connections, pconn);
    } else {
      log_packet("Server reports updated connection %d %s",
                 pinfo->id, pinfo->username);
      if (pplayer != pconn->playing) {
	if (NULL != pconn->playing) {
	  conn_list_remove(pconn->playing->connections, pconn);
	}
	if (pplayer) {
	  conn_list_append(pplayer->connections, pconn);
	}
      }
    }

    pconn->id = pinfo->id;
    pconn->established = pinfo->established;
    pconn->observer = pinfo->observer;
    pconn->access_level = pinfo->access_level;
    pconn->playing = pplayer;

    sz_strlcpy(pconn->username, pinfo->username);
    sz_strlcpy(pconn->addr, pinfo->addr);
    sz_strlcpy(pconn->capability, pinfo->capability);

    if (pinfo->id == client.conn.id) {
      /* NB: In this case, pconn is not a duplication of client.conn.
       *
       * pconn->addr is our address that the server knows whereas
       * client.conn.addr is the address to the server. Also,
       * pconn->capability stores our capabilites known at server side
       * whereas client.conn.capability represents the capabilities of the
       * server. */
      if (client.conn.playing != pplayer
          || client.conn.observer != pinfo->observer) {
        /* Our connection state changed, let prepare the changes and reset
         * the game. */
        preparing_client_state = TRUE;
      }

      /* Copy our current state into the static structure (our connection
       * to the server). */
      client.conn.established = pinfo->established;
      client.conn.observer = pinfo->observer;
      client.conn.access_level = pinfo->access_level;
      client.conn.playing = pplayer;
      sz_strlcpy(client.conn.username, pinfo->username);
    }
  }

  players_dialog_update();
  conn_list_dialog_update();

  if (pinfo->used && pinfo->id == client.conn.id) {
    /* For updating the sensitivity of the "Edit Mode" menu item,
     * among other things. */
    menus_update();
  }

  if (preparing_client_state) {
    set_client_state(C_S_PREPARING);
  }
}

/************************************************************************//**
  Handles a conn_ping_info packet from the server.  This packet contains
  ping times for each connection.
****************************************************************************/
void handle_conn_ping_info(int connections, const int *conn_id,
                           const float *ping_time)
{
  int i;

  for (i = 0; i < connections; i++) {
    struct connection *pconn = conn_by_number(conn_id[i]);

    if (!pconn) {
      continue;
    }

    pconn->ping_time = ping_time[i];
    log_debug("conn-id=%d, ping=%fs", pconn->id, pconn->ping_time);
  }
  /* The old_ping_time data is ignored. */

  players_dialog_update();
}

/************************************************************************//**
  Received package about gaining an achievement.
****************************************************************************/
void handle_achievement_info(int id, bool gained, bool first)
{
  struct achievement *pach;

  if (id < 0 || id >= game.control.num_achievement_types) {
    log_error("Received illegal achievement info %d", id);
    return;
  }

  pach = achievement_by_number(id);

  if (gained) {
    BV_SET(pach->achievers, player_index(client_player()));
  } else {
    BV_CLR(pach->achievers, player_index(client_player()));
  }

  if (first) {
    pach->first = client_player();
  }
}

/************************************************************************//**
  Ideally the client should let the player choose which type of
  modules and components to build, and (possibly) where to extend
  structurals.  The protocol now makes this possible, but the
  client is not yet that good (would require GUI improvements)
  so currently the client choices stuff automatically if there
  is anything unplaced.

  This function makes a choice (sends spaceship_action) and
  returns 1 if we placed something, else 0.

  Do things one at a time; the server will send us an updated
  spaceship_info packet, and we'll be back here to do anything
  which is left.
****************************************************************************/
static bool spaceship_autoplace(struct player *pplayer,
                                struct player_spaceship *ship)
{
  if (can_client_issue_orders()) {
    struct spaceship_component place;

    if (next_spaceship_component(pplayer, ship, &place)) {
      dsend_packet_spaceship_place(&client.conn, place.type, place.num);

      return TRUE;
    }
  }

  return FALSE;
}

/************************************************************************//**
  Packet spaceship_info handler.
****************************************************************************/
void handle_spaceship_info(const struct packet_spaceship_info *p)
{
  struct player_spaceship *ship;
  struct player *pplayer = player_by_number(p->player_num);

  fc_assert_ret_msg(NULL != pplayer, "Invalid player number %d.",
                    p->player_num);

  ship = &pplayer->spaceship;
  ship->state        = p->sship_state;
  ship->structurals  = p->structurals;
  ship->components   = p->components;
  ship->modules      = p->modules;
  ship->fuel         = p->fuel;
  ship->propulsion   = p->propulsion;
  ship->habitation   = p->habitation;
  ship->life_support = p->life_support;
  ship->solar_panels = p->solar_panels;
  ship->launch_year  = p->launch_year;
  ship->population   = p->population;
  ship->mass         = p->mass;
  ship->support_rate = p->support_rate;
  ship->energy_rate  = p->energy_rate;
  ship->success_rate = p->success_rate;
  ship->travel_time  = p->travel_time;
  ship->structure    = p->structure;

  if (pplayer != client_player()) {
    refresh_spaceship_dialog(pplayer);
    menus_update();
    return;
  }

  if (!spaceship_autoplace(pplayer, ship)) {
    /* We refresh the dialog when the packet did *not* cause placing
     * of new part. That's because those cases where part is placed, are
     * followed by exactly one case where there's no more parts to place -
     * we want to refresh the dialog only when that last packet comes. */
    refresh_spaceship_dialog(pplayer);
  }
}

/************************************************************************//**
  Packet tile_info handler.
****************************************************************************/
void handle_tile_info(const struct packet_tile_info *packet)
{
  enum known_type new_known;
  enum known_type old_known;
  bool known_changed = FALSE;
  bool tile_changed = FALSE;
  struct player *powner = player_by_number(packet->owner);
  struct player *eowner = player_by_number(packet->extras_owner);
  struct extra_type *presource = NULL;
  struct terrain *pterrain = terrain_by_number(packet->terrain);
  struct tile *ptile = index_to_tile(&(wld.map), packet->tile);

  fc_assert_ret_msg(NULL != ptile, "Invalid tile index %d.", packet->tile);
  old_known = client_tile_get_known(ptile);

  if (packet->resource != MAX_EXTRA_TYPES) {
    presource = extra_by_number(packet->resource);
  }

  if (NULL == tile_terrain(ptile) || pterrain != tile_terrain(ptile)) {
    tile_changed = TRUE;
    switch (old_known) {
    case TILE_UNKNOWN:
      tile_set_terrain(ptile, pterrain);
      break;
    case TILE_KNOWN_UNSEEN:
    case TILE_KNOWN_SEEN:
      if (NULL != pterrain || TILE_UNKNOWN == packet->known) {
        tile_set_terrain(ptile, pterrain);
      } else {
        tile_changed = FALSE;
        log_error("handle_tile_info() unknown terrain (%d, %d).",
                  TILE_XY(ptile));
      }
      break;
    };
  }

  if (!BV_ARE_EQUAL(ptile->extras, packet->extras)) {
    ptile->extras = packet->extras;
    tile_changed = TRUE;
  }

  tile_changed = tile_changed || (tile_resource(ptile) != presource);

  /* always called after setting terrain */
  tile_set_resource(ptile, presource);

  if (tile_owner(ptile) != powner) {
    tile_set_owner(ptile, powner, NULL);
    tile_changed = TRUE;
  }
  if (extra_owner(ptile) != eowner) {
    ptile->extras_owner = eowner;
    tile_changed = TRUE;
  }

  if (NULL == tile_worked(ptile)
      || tile_worked(ptile)->id != packet->worked) {
    if (IDENTITY_NUMBER_ZERO != packet->worked) {
      struct city *pwork = game_city_by_number(packet->worked);

      if (NULL == pwork) {
        char named[MAX_LEN_CITYNAME];

        /* new unseen ("invisible") city, or before city_info */
        fc_snprintf(named, sizeof(named), "%06u", packet->worked);

        pwork = create_city_virtual(invisible.placeholder, NULL, named);
        pwork->id = packet->worked;
        idex_register_city(&wld, pwork);

        city_list_prepend(invisible.cities, pwork);

        log_debug("(%d,%d) invisible city %d, %s",
                  TILE_XY(ptile), pwork->id, city_name_get(pwork));
      } else if (NULL == city_tile(pwork)) {
        /* old unseen ("invisible") city, or before city_info */
        if (NULL != powner && city_owner(pwork) != powner) {
          /* update placeholder with current owner */
          pwork->owner = powner;
          pwork->original = powner;
        }
      } else {
        /* We have a real (not invisible) city record for this ID, but
         * perhaps our info about that city is out of date. */
        int dist_sq = sq_map_distance(city_tile(pwork), ptile);

        if (dist_sq > city_map_radius_sq_get(pwork)) {
          /* This is probably enemy city which has grown in diameter since we
           * last saw it. We need city_radius_sq to be at least big enough so
           * that all workers fit in, so set it so. */
          city_map_radius_sq_set(pwork, dist_sq);
        }
        /* This might be a known city that is open in a dialog.
         * (And this might be our only prompt to refresh the worked tiles
         * display in its city map, if a worker rearrangement does not
         * change anything else about the city such as output.) */
        {
          struct city *oldwork = tile_worked(ptile);
          if (oldwork && NULL != city_tile(oldwork)) {
            /* Refresh previous city too if it's real and different */
            refresh_city_dialog(oldwork);
          }
          /* Refresh new city working tile (which we already know is real) */
          refresh_city_dialog(pwork);
        }
      }

      /* This marks tile worked by (possibly invisible) city. Other
       * parts of the code have to handle invisible cities correctly
       * (ptile->worked->tile == NULL) */
      tile_set_worked(ptile, pwork);
    } else {
      /* Tile is no longer being worked by a city.
       * (Again, this might be our only prompt to refresh the worked tiles
       * display for the previous working city.) */
      if (tile_worked(ptile) && NULL != city_tile(tile_worked(ptile))) {
        refresh_city_dialog(tile_worked(ptile));
      }
      tile_set_worked(ptile, NULL);
    }

    tile_changed = TRUE;
  }

  if (old_known != packet->known) {
    known_changed = TRUE;
  }

  if (NULL != client.conn.playing) {
    dbv_clr(&client.conn.playing->tile_known, tile_index(ptile));
    vision_layer_iterate(v) {
      dbv_clr(&client.conn.playing->client.tile_vision[v], tile_index(ptile));
    } vision_layer_iterate_end;

    switch (packet->known) {
    case TILE_KNOWN_SEEN:
      dbv_set(&client.conn.playing->tile_known, tile_index(ptile));
      vision_layer_iterate(v) {
        dbv_set(&client.conn.playing->client.tile_vision[v], tile_index(ptile));
      } vision_layer_iterate_end;
      break;
    case TILE_KNOWN_UNSEEN:
      dbv_set(&client.conn.playing->tile_known, tile_index(ptile));
      break;
    case TILE_UNKNOWN:
      break;
    default:
      log_error("handle_tile_info() invalid known (%d).", packet->known);
      break;
    };
  }
  new_known = client_tile_get_known(ptile);

  if (packet->spec_sprite[0] != '\0') {
    if (!ptile->spec_sprite
	|| strcmp(ptile->spec_sprite, packet->spec_sprite) != 0) {
      if (ptile->spec_sprite) {
	free(ptile->spec_sprite);
      }
      ptile->spec_sprite = fc_strdup(packet->spec_sprite);
      tile_changed = TRUE;
    }
  } else {
    if (ptile->spec_sprite) {
      free(ptile->spec_sprite);
      ptile->spec_sprite = NULL;
      tile_changed = TRUE;
    }
  }

  if (TILE_KNOWN_SEEN == old_known && TILE_KNOWN_SEEN != new_known) {
    /* This is an error. So first we log the error,
     * then make an assertion. */
    unit_list_iterate(ptile->units, punit) {
      log_error("%p %d %s at (%d,%d) %s", punit, punit->id,
                unit_rule_name(punit), TILE_XY(unit_tile(punit)),
                player_name(unit_owner(punit)));
    } unit_list_iterate_end;
    fc_assert_msg(0 == unit_list_size(ptile->units), "Ghost units seen");
    /* Repairing... */
    unit_list_clear(ptile->units);
  }

  ptile->continent = packet->continent;
  wld.map.num_continents = MAX(ptile->continent, wld.map.num_continents);

  if (packet->label[0] == '\0') {
    if (ptile->label != NULL) {
      FC_FREE(ptile->label);
      ptile->label = NULL;
      tile_changed = TRUE;
    }
  } else if (ptile->label == NULL || strcmp(packet->label, ptile->label)) {
      tile_set_label(ptile, packet->label);
      tile_changed = TRUE;
  }

  if (known_changed || tile_changed) {
    /* 
     * A tile can only change if it was known before and is still
     * known. In the other cases the tile is new or removed.
     */
    if (known_changed && TILE_KNOWN_SEEN == new_known) {
      agents_tile_new(ptile);
    } else if (known_changed && TILE_KNOWN_UNSEEN == new_known) {
      agents_tile_remove(ptile);
    } else {
      agents_tile_changed(ptile);
    }
    editgui_notify_object_changed(OBJTYPE_TILE, tile_index(ptile), FALSE);
  }

  /* refresh tiles */
  if (can_client_change_view()) {
    /* the tile itself (including the necessary parts of adjacent tiles) */
    if (tile_changed || old_known != new_known) {
      refresh_tile_mapcanvas(ptile, TRUE, FALSE);
    }
  }

  /* update menus if the focus unit is on the tile. */
  if (tile_changed) {
    if (get_focus_unit_on_tile(ptile)) {
      menus_update();
    }
  }

  /* FIXME: we really ought to call refresh_city_dialog() for any city
   * whose radii include this tile, to update the city map display.
   * But that would be expensive. We deal with the (common) special
   * case of changes in worked tiles above. */
}

/************************************************************************//**
  Received packet containing info about current scenario
****************************************************************************/
void handle_scenario_info(const struct packet_scenario_info *packet)
{
  game.scenario.is_scenario = packet->is_scenario;
  sz_strlcpy(game.scenario.name, packet->name);
  sz_strlcpy(game.scenario.authors, packet->authors);
  game.scenario.players = packet->players;
  game.scenario.startpos_nations = packet->startpos_nations;
  game.scenario.prevent_new_cities = packet->prevent_new_cities;
  game.scenario.lake_flooding = packet->lake_flooding;
  game.scenario.have_resources = packet->have_resources;
  game.scenario.ruleset_locked = packet->ruleset_locked;
  game.scenario.save_random = packet->save_random;
  game.scenario.handmade = packet->handmade;
  game.scenario.allow_ai_type_fallback = packet->allow_ai_type_fallback;

  editgui_notify_object_changed(OBJTYPE_GAME, 1, FALSE);
}

/************************************************************************//**
  Received packet containing description of current scenario
****************************************************************************/
void handle_scenario_description(const char *description)
{
  sz_strlcpy(game.scenario_desc.description, description);

  editgui_notify_object_changed(OBJTYPE_GAME, 1, FALSE);
}

/************************************************************************//**
  Take arrival of ruleset control packet to indicate that
  current allocated governments should be free'd, and new
  memory allocated for new size. The same for nations.
****************************************************************************/
void handle_ruleset_control(const struct packet_ruleset_control *packet)
{
  /* The ruleset is going to load new nations. So close
   * the nation selection dialog if it is open. */
  popdown_races_dialog();

  game.client.ruleset_init = FALSE;
  game.client.ruleset_ready = FALSE;
  game_ruleset_free();
  game_ruleset_init();
  game.client.ruleset_init = TRUE;
  game.control = *packet;

  /* check the values! */
#define VALIDATE(_count, _maximum, _string)                                 \
  if (game.control._count > _maximum) {                                     \
    log_error("handle_ruleset_control(): Too many " _string                 \
              "; using %d of %d", _maximum, game.control._count);           \
    game.control._count = _maximum;                                         \
  }

  VALIDATE(num_unit_classes,	UCL_LAST,		"unit classes");
  VALIDATE(num_unit_types,	U_LAST,			"unit types");
  VALIDATE(num_impr_types,	B_LAST,			"improvements");
  VALIDATE(num_tech_types,      A_LAST,                 "advances");
  VALIDATE(num_base_types,	MAX_BASE_TYPES,		"bases");
  VALIDATE(num_road_types,      MAX_ROAD_TYPES,         "roads");
  VALIDATE(num_resource_types,	MAX_RESOURCE_TYPES,     "resources");
  VALIDATE(num_disaster_types,  MAX_DISASTER_TYPES,     "disasters");
  VALIDATE(num_achievement_types, MAX_ACHIEVEMENT_TYPES, "achievements");

  /* game.control.government_count, game.control.nation_count and
   * game.control.styles_count are allocated dynamically, and does
   * not need a size check.  See the allocation bellow. */

  VALIDATE(terrain_count,	MAX_NUM_TERRAINS,	"terrains");

  VALIDATE(num_specialist_types, SP_MAX,		"specialists");
#undef VALIDATE

  governments_alloc(game.control.government_count);
  nations_alloc(game.control.nation_count);
  styles_alloc(game.control.num_styles);
  city_styles_alloc(game.control.styles_count);
  music_styles_alloc(game.control.num_music_styles);

  if (game.control.desc_length > 0) {
    game.ruleset_description = fc_malloc(game.control.desc_length + 1);
    game.ruleset_description[0] = '\0';
  }

  if (packet->preferred_tileset[0] != '\0') {
    /* There is tileset suggestion */
    if (strcmp(packet->preferred_tileset, tileset_basename(tileset))) {
      /* It's not currently in use */
      if (gui_options.autoaccept_tileset_suggestion) {
        tilespec_reread(game.control.preferred_tileset, FALSE, 1.0f);
      } else {
        popup_tileset_suggestion_dialog();
      }
    }
  }

  if (packet->preferred_soundset[0] != '\0') {
    /* There is soundset suggestion */
    if (strcmp(packet->preferred_soundset, sound_set_name)) {
      /* It's not currently in use */
      if (gui_options.autoaccept_soundset_suggestion) {
        audio_restart(game.control.preferred_soundset, music_set_name);
      } else {
        popup_soundset_suggestion_dialog();
      }
    }
  }

  if (packet->preferred_musicset[0] != '\0') {
    /* There is musicset suggestion */
    if (strcmp(packet->preferred_musicset, music_set_name)) {
      /* It's not currently in use */
      if (gui_options.autoaccept_musicset_suggestion) {
        audio_restart(sound_set_name, game.control.preferred_musicset);
      } else {
        popup_musicset_suggestion_dialog();
      }
    }
  }

  tileset_ruleset_reset(tileset);
}

/************************************************************************//**
  Ruleset summary.
****************************************************************************/
void handle_ruleset_summary(const struct packet_ruleset_summary *packet)
{
  int len;

  if (game.ruleset_summary != NULL) {
    free(game.ruleset_summary);
  }

  len = strlen(packet->text);

  game.ruleset_summary = fc_malloc(len + 1);

  fc_strlcpy(game.ruleset_summary, packet->text, len + 1);
}

/************************************************************************//**
  Next part of ruleset description.
****************************************************************************/
void handle_ruleset_description_part(
                        const struct packet_ruleset_description_part *packet)
{
  fc_strlcat(game.ruleset_description, packet->text,
             game.control.desc_length + 1);
}

/************************************************************************//**
  Received packet indicating that all rulesets have now been received.
****************************************************************************/
void handle_rulesets_ready(void)
{
  /* Setup extra hiders caches */
  extra_type_iterate(pextra) {
    pextra->hiders = extra_type_list_new();
    extra_type_iterate(phider) {
      if (BV_ISSET(pextra->hidden_by, extra_index(phider))) {
        extra_type_list_append(pextra->hiders, phider);
      }
    } extra_type_iterate_end;
    pextra->bridged = extra_type_list_new();
    extra_type_iterate(pbridged) {
      if (BV_ISSET(pextra->bridged_over, extra_index(pbridged))) {
        extra_type_list_append(pextra->bridged, pbridged);
      }
    } extra_type_iterate_end;
  } extra_type_iterate_end;

  unit_class_iterate(pclass) {
    set_unit_class_caches(pclass);
    set_unit_move_type(pclass);
  } unit_class_iterate_end;

  /* Setup improvement feature caches */
  improvement_feature_cache_init();

  /* Setup road integrators caches */
  road_integrators_cache_init();

  /* Pre calculate action related data. */
  actions_rs_pre_san_gen();

  /* Setup unit unknown move cost caches */
  unit_type_iterate(ptype) {
    ptype->unknown_move_cost = utype_unknown_move_cost(ptype);
    set_unit_type_caches(ptype);
    unit_type_action_cache_set(ptype);
  } unit_type_iterate_end;

  /* Cache what city production can receive help from caravans. */
  city_production_caravan_shields_init();

  /* Adjust editor for changed ruleset. */
  editor_ruleset_changed();

  /* We are not going to crop any more sprites from big sprites, free them. */
  finish_loading_sprites(tileset);

  game.client.ruleset_ready = TRUE;
}

/************************************************************************//**
  Packet ruleset_unit_class handler.
****************************************************************************/
void handle_ruleset_unit_class(const struct packet_ruleset_unit_class *p)
{
  struct unit_class *c = uclass_by_number(p->id);

  fc_assert_ret_msg(NULL != c, "Bad unit_class %d.", p->id);

  names_set(&c->name, NULL, p->name, p->rule_name);
  c->min_speed          = p->min_speed;
  c->hp_loss_pct        = p->hp_loss_pct;
  c->hut_behavior       = p->hut_behavior;
  c->non_native_def_pct = p->non_native_def_pct;
  c->flags              = p->flags;

  PACKET_STRVEC_EXTRACT(c->helptext, p->helptext);
}

/************************************************************************//**
  Packet ruleset_unit handler.
****************************************************************************/
void handle_ruleset_unit(const struct packet_ruleset_unit *p)
{
  int i;
  struct unit_type *u = utype_by_number(p->id);

  fc_assert_ret_msg(NULL != u, "Bad unit_type %d.", p->id);

  names_set(&u->name, NULL, p->name, p->rule_name);
  sz_strlcpy(u->graphic_str, p->graphic_str);
  sz_strlcpy(u->graphic_alt, p->graphic_alt);
  sz_strlcpy(u->sound_move, p->sound_move);
  sz_strlcpy(u->sound_move_alt, p->sound_move_alt);
  sz_strlcpy(u->sound_fight, p->sound_fight);
  sz_strlcpy(u->sound_fight_alt, p->sound_fight_alt);

  u->uclass             = uclass_by_number(p->unit_class_id);
  u->build_cost         = p->build_cost;
  u->pop_cost           = p->pop_cost;
  u->attack_strength    = p->attack_strength;
  u->defense_strength   = p->defense_strength;
  u->move_rate          = p->move_rate;
  u->require_advance    = advance_by_number(p->tech_requirement);
  u->need_improvement   = improvement_by_number(p->impr_requirement);
  u->need_government    = government_by_number(p->gov_requirement);
  u->vision_radius_sq = p->vision_radius_sq;
  u->transport_capacity = p->transport_capacity;
  u->hp                 = p->hp;
  u->firepower          = p->firepower;
  u->obsoleted_by       = utype_by_number(p->obsoleted_by);
  u->converted_to       = utype_by_number(p->converted_to);
  u->convert_time       = p->convert_time;
  u->fuel               = p->fuel;
  u->flags              = p->flags;
  u->roles              = p->roles;
  u->happy_cost         = p->happy_cost;
  output_type_iterate(o) {
    u->upkeep[o] = p->upkeep[o];
  } output_type_iterate_end;
  u->paratroopers_range = p->paratroopers_range;
  u->paratroopers_mr_req = p->paratroopers_mr_req;
  u->paratroopers_mr_sub = p->paratroopers_mr_sub;
  u->bombard_rate       = p->bombard_rate;
  u->city_size          = p->city_size;
  u->city_slots         = p->city_slots;
  u->cargo              = p->cargo;
  u->targets            = p->targets;
  u->embarks            = p->embarks;
  u->disembarks         = p->disembarks;
  u->vlayer             = p->vlayer;

  if (p->veteran_levels == 0) {
    u->veteran = NULL;
  } else {
    u->veteran = veteran_system_new(p->veteran_levels);

    for (i = 0; i < p->veteran_levels; i++) {
      veteran_system_definition(u->veteran, i, p->veteran_name[i],
                                p->power_fact[i], p->move_bonus[i],
                                p->raise_chance[i], p->work_raise_chance[i]);
    }
  }

  PACKET_STRVEC_EXTRACT(u->helptext, p->helptext);

  u->adv.worker = p->worker;

  tileset_setup_unit_type(tileset, u);
}

/************************************************************************//**
  This is a packet that only the web-client needs. The regular client has no
  use for it.
  TODO: Do not generate code calling this in the C-client.
****************************************************************************/
void handle_web_ruleset_unit_addition(int id, bv_actions utype_actions)
{
}

/************************************************************************//**
  Packet ruleset_unit_bonus handler.
****************************************************************************/
void handle_ruleset_unit_bonus(const struct packet_ruleset_unit_bonus *p)
{
  struct unit_type *u = utype_by_number(p->unit);
  struct combat_bonus *bonus;

  fc_assert_ret_msg(NULL != u, "Bad unit_type %d.", p->unit);

  bonus = malloc(sizeof(*bonus));

  bonus->flag  = p->flag;
  bonus->type  = p->type;
  bonus->value = p->value;
  bonus->quiet = p->quiet;

  combat_bonus_list_append(u->bonuses, bonus);
}

/************************************************************************//**
  Packet ruleset_unit_flag handler.
****************************************************************************/
void handle_ruleset_unit_flag(const struct packet_ruleset_unit_flag *p)
{
  const char *flagname;
  const char *helptxt;

  fc_assert_ret_msg(p->id >= UTYF_USER_FLAG_1 && p->id <= UTYF_LAST_USER_FLAG, "Bad user flag %d.", p->id);

  if (p->name[0] == '\0') {
    flagname = NULL;
  } else {
    flagname = p->name;
  }

  if (p->helptxt[0] == '\0') {
    helptxt = NULL;
  } else {
    helptxt = p->helptxt;
  }

  set_user_unit_type_flag_name(p->id, flagname, helptxt);
}

/************************************************************************//**
  Packet ruleset_unit_class_flag handler.
****************************************************************************/
void handle_ruleset_unit_class_flag(
    const struct packet_ruleset_unit_class_flag *p)
{
  const char *flagname;
  const char *helptxt;

  fc_assert_ret_msg(p->id >= UCF_USER_FLAG_1 && p->id <= UCF_LAST_USER_FLAG,
                    "Bad user flag %d.", p->id);

  if (p->name[0] == '\0') {
    flagname = NULL;
  } else {
    flagname = p->name;
  }

  if (p->helptxt[0] == '\0') {
    helptxt = NULL;
  } else {
    helptxt = p->helptxt;
  }

  set_user_unit_class_flag_name(p->id, flagname, helptxt);
}

/************************************************************************//**
  Unpack a traditional tech req from a standard requirement vector (that
  still is in the network serialized format rather than a proper
  requirement vector).

  Returns the position in the requirement vector after unpacking. It will
  increase if a tech req was extracted.
****************************************************************************/
static int unpack_tech_req(const enum tech_req r_num,
                           const int reqs_size,
                           const struct requirement *reqs,
                           struct advance *a,
                           int i)
{
  if (i < reqs_size
      && reqs[i].source.kind == VUT_ADVANCE) {
    /* Extract the tech req so the old code can reason about it. */

    /* This IS a traditional tech req... right? */
    fc_assert(reqs[i].present);
    fc_assert(reqs[i].range == REQ_RANGE_PLAYER);

    /* Put it in the advance structure. */
    a->require[r_num] = reqs[i].source.value.advance;

    /* Move on in the requirement vector. */
    i++;
  } else {
    /* No tech req. */
    a->require[r_num] = advance_by_number(A_NONE);
  }

  return i;
}

/************************************************************************//**
  Packet ruleset_tech handler.
****************************************************************************/
void handle_ruleset_tech(const struct packet_ruleset_tech *p)
{
  int i;
  struct advance *a = advance_by_number(p->id);

  fc_assert_ret_msg(NULL != a, "Bad advance %d.", p->id);

  names_set(&a->name, NULL, p->name, p->rule_name);
  sz_strlcpy(a->graphic_str, p->graphic_str);
  sz_strlcpy(a->graphic_alt, p->graphic_alt);

  i = 0;

  fc_assert(game.control.num_tech_classes == 0 || p->tclass < game.control.num_tech_classes);
  if (p->tclass >= 0) {
    a->tclass = tech_class_by_number(p->tclass);
  } else {
    a->tclass = NULL;
  }

  /* The tech requirements req1 and req2 are send inside research_reqs
   * since they too are required to be fulfilled before the tech can be
   * researched. */

  if (p->removed) {
    /* The Freeciv data structures currently records that a tech is removed
     * by setting req1 and req2 to "Never". */
    a->require[AR_ONE] = A_NEVER;
    a->require[AR_TWO] = A_NEVER;
  } else {
    /* Unpack req1 and req2 from the research_reqs requirement vector. */
    i = unpack_tech_req(AR_ONE, p->research_reqs_count, p->research_reqs, a, i);
    i = unpack_tech_req(AR_TWO, p->research_reqs_count, p->research_reqs, a, i);
  }

  /* Any remaining requirements are a part of the research_reqs requirement
   * vector. */
  for (; i < p->research_reqs_count; i++) {
    requirement_vector_append(&a->research_reqs, p->research_reqs[i]);
  }

  /* The packet's research_reqs should contain req1, req2 and the
   * requirements of the tech's research_reqs. */
  fc_assert((a->research_reqs.size
             + ((a->require[AR_ONE]
                 && (advance_number(a->require[AR_ONE]) != A_NONE)) ?
                  1 : 0)
             + ((a->require[AR_TWO]
                 && (advance_number(a->require[AR_TWO]) != A_NONE)) ?
                  1 : 0))
            == p->research_reqs_count);

  a->require[AR_ROOT] = advance_by_number(p->root_req);

  a->flags = p->flags;
  a->cost = p->cost;
  a->num_reqs = p->num_reqs;
  PACKET_STRVEC_EXTRACT(a->helptext, p->helptext);

  tileset_setup_tech_type(tileset, a);
}

/************************************************************************//**
  Packet ruleset_tech_class handler.
****************************************************************************/
void handle_ruleset_tech_class(const struct packet_ruleset_tech_class *p)
{
  struct tech_class *ptclass = tech_class_by_number(p->id);

  fc_assert_ret_msg(NULL != ptclass, "Bad tech_class %d.", p->id);

  names_set(&ptclass->name, NULL, p->name, p->rule_name);
  ptclass->cost_pct = p->cost_pct;
}

/************************************************************************//**
  Packet ruleset_tech_flag handler.
****************************************************************************/
void handle_ruleset_tech_flag(const struct packet_ruleset_tech_flag *p)
{
  const char *flagname;
  const char *helptxt;

  fc_assert_ret_msg(p->id >= TECH_USER_1 && p->id <= TECH_USER_LAST, "Bad user flag %d.", p->id);

  if (p->name[0] == '\0') {
    flagname = NULL;
  } else {
    flagname = p->name;
  }

  if (p->helptxt[0] == '\0') {
    helptxt = NULL;
  } else {
    helptxt = p->helptxt;
  }

  set_user_tech_flag_name(p->id, flagname, helptxt);
}

/************************************************************************//**
  Packet ruleset_building handler.
****************************************************************************/
void handle_ruleset_building(const struct packet_ruleset_building *p)
{
  int i;
  struct impr_type *b = improvement_by_number(p->id);

  fc_assert_ret_msg(NULL != b, "Bad improvement %d.", p->id);

  b->genus = p->genus;
  names_set(&b->name, NULL, p->name, p->rule_name);
  sz_strlcpy(b->graphic_str, p->graphic_str);
  sz_strlcpy(b->graphic_alt, p->graphic_alt);
  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&b->reqs, p->reqs[i]);
  }
  fc_assert(b->reqs.size == p->reqs_count);
  for (i = 0; i < p->obs_count; i++) {
    requirement_vector_append(&b->obsolete_by, p->obs_reqs[i]);
  }
  fc_assert(b->obsolete_by.size == p->obs_count);
  b->build_cost = p->build_cost;
  b->upkeep = p->upkeep;
  b->sabotage = p->sabotage;
  b->flags = p->flags;
  PACKET_STRVEC_EXTRACT(b->helptext, p->helptext);
  sz_strlcpy(b->soundtag, p->soundtag);
  sz_strlcpy(b->soundtag_alt, p->soundtag_alt);

#ifdef FREECIV_DEBUG
  if (p->id == improvement_count() - 1) {
    improvement_iterate(bdbg) {
      log_debug("Improvement: %s...", improvement_rule_name(bdbg));
      log_debug("  build_cost %3d", bdbg->build_cost);
      log_debug("  upkeep      %2d", bdbg->upkeep);
      log_debug("  sabotage   %3d", bdbg->sabotage);
      if (NULL != bdbg->helptext) {
        strvec_iterate(bdbg->helptext, text) {
          log_debug("  helptext    %s", text);
        } strvec_iterate_end;
      }
    } improvement_iterate_end;
  }
#endif /* FREECIV_DEBUG */

  tileset_setup_impr_type(tileset, b);
}

/************************************************************************//**
  Packet ruleset_multiplier handler.
****************************************************************************/
void handle_ruleset_multiplier(const struct packet_ruleset_multiplier *p)
{
  struct multiplier *pmul = multiplier_by_number(p->id);
  int j;

  fc_assert_ret_msg(NULL != pmul, "Bad multiplier %d.", p->id);

  pmul->start  = p->start;
  pmul->stop   = p->stop;
  pmul->step   = p->step;
  pmul->def    = p->def;
  pmul->offset = p->offset;
  pmul->factor = p->factor;

  names_set(&pmul->name, NULL, p->name, p->rule_name);

  for (j = 0; j < p->reqs_count; j++) {
    requirement_vector_append(&pmul->reqs, p->reqs[j]);
  }
  fc_assert(pmul->reqs.size == p->reqs_count);

  PACKET_STRVEC_EXTRACT(pmul->helptext, p->helptext);
}

/************************************************************************//**
  Packet ruleset_government handler.
****************************************************************************/
void handle_ruleset_government(const struct packet_ruleset_government *p)
{
  int j;
  struct government *gov = government_by_number(p->id);

  fc_assert_ret_msg(NULL != gov, "Bad government %d.", p->id);

  gov->item_number = p->id;

  for (j = 0; j < p->reqs_count; j++) {
    requirement_vector_append(&gov->reqs, p->reqs[j]);
  }
  fc_assert(gov->reqs.size == p->reqs_count);

  names_set(&gov->name, NULL, p->name, p->rule_name);
  sz_strlcpy(gov->graphic_str, p->graphic_str);
  sz_strlcpy(gov->graphic_alt, p->graphic_alt);

  PACKET_STRVEC_EXTRACT(gov->helptext, p->helptext);

  tileset_setup_government(tileset, gov);
}

/************************************************************************//**
  Packet ruleset_government_ruler_title handler.
****************************************************************************/
void handle_ruleset_government_ruler_title
    (const struct packet_ruleset_government_ruler_title *packet)
{
  struct government *gov = government_by_number(packet->gov);

  fc_assert_ret_msg(NULL != gov, "Bad government %d.", packet->gov);

  (void) government_ruler_title_new(gov, nation_by_number(packet->nation),
                                    packet->male_title,
                                    packet->female_title);
}

/************************************************************************//**
  Packet ruleset_terrain handler.
****************************************************************************/
void handle_ruleset_terrain(const struct packet_ruleset_terrain *p)
{
  int j;
  struct terrain *pterrain = terrain_by_number(p->id);

  fc_assert_ret_msg(NULL != pterrain, "Bad terrain %d.", p->id);

  pterrain->tclass = p->tclass;
  pterrain->native_to = p->native_to;
  names_set(&pterrain->name, NULL, p->name, p->rule_name);
  sz_strlcpy(pterrain->graphic_str, p->graphic_str);
  sz_strlcpy(pterrain->graphic_alt, p->graphic_alt);
  pterrain->movement_cost = p->movement_cost;
  pterrain->defense_bonus = p->defense_bonus;

  output_type_iterate(o) {
    pterrain->output[o] = p->output[o];
  } output_type_iterate_end;

  if (pterrain->resources != NULL) {
    free(pterrain->resources);
  }
  pterrain->resources = fc_calloc(p->num_resources + 1,
                                  sizeof(*pterrain->resources));
  for (j = 0; j < p->num_resources; j++) {
    pterrain->resources[j] = extra_by_number(p->resources[j]);
    if (!pterrain->resources[j]) {
      log_error("handle_ruleset_terrain() "
                "Mismatched resource %d for terrain \"%s\".",
                p->resources[j], terrain_rule_name(pterrain));
    }
  }
  pterrain->resources[p->num_resources] = NULL;

  output_type_iterate(o) {
    pterrain->road_output_incr_pct[o] = p->road_output_incr_pct[o];
  } output_type_iterate_end;

  pterrain->base_time = p->base_time;
  pterrain->road_time = p->road_time;
  pterrain->irrigation_result = terrain_by_number(p->irrigation_result);
  pterrain->irrigation_food_incr = p->irrigation_food_incr;
  pterrain->irrigation_time = p->irrigation_time;
  pterrain->mining_result = terrain_by_number(p->mining_result);
  pterrain->mining_shield_incr = p->mining_shield_incr;
  pterrain->mining_time = p->mining_time;
  if (p->animal < 0) {
    pterrain->animal = NULL;
  } else {
    pterrain->animal = utype_by_number(p->animal);
  }
  pterrain->transform_result = terrain_by_number(p->transform_result);
  pterrain->transform_time = p->transform_time;
  pterrain->pillage_time = p->pillage_time;
  pterrain->clean_pollution_time = p->clean_pollution_time;
  pterrain->clean_fallout_time = p->clean_fallout_time;

  pterrain->flags = p->flags;

  fc_assert_ret(pterrain->rgb == NULL);
  pterrain->rgb = rgbcolor_new(p->color_red, p->color_green, p->color_blue);

  PACKET_STRVEC_EXTRACT(pterrain->helptext, p->helptext);

  tileset_setup_tile_type(tileset, pterrain);
}

/************************************************************************//**
  Packet ruleset_terrain_flag handler.
****************************************************************************/
void handle_ruleset_terrain_flag(const struct packet_ruleset_terrain_flag *p)
{
  const char *flagname;
  const char *helptxt;

  fc_assert_ret_msg(p->id >= TER_USER_1 && p->id <= TER_USER_LAST, "Bad user flag %d.", p->id);

  if (p->name[0] == '\0') {
    flagname = NULL;
  } else {
    flagname = p->name;
  }

  if (p->helptxt[0] == '\0') {
    helptxt = NULL;
  } else {
    helptxt = p->helptxt;
  }

  set_user_terrain_flag_name(p->id, flagname, helptxt);
}

/************************************************************************//**
  Handle a packet about a particular terrain resource.
****************************************************************************/
void handle_ruleset_resource(const struct packet_ruleset_resource *p)
{
  struct resource_type *presource;

  if (p->id < 0 || p->id > MAX_EXTRA_TYPES) {
    log_error("Bad resource %d.", p->id);
    return;
  }

  presource = resource_type_init(extra_by_number(p->id));

  output_type_iterate(o) {
    presource->output[o] = p->output[o];
  } output_type_iterate_end;
}

/************************************************************************//**
  Handle a packet about a particular extra type.
****************************************************************************/
void handle_ruleset_extra(const struct packet_ruleset_extra *p)
{
  struct extra_type *pextra = extra_by_number(p->id);
  int i;
  bool cbase;
  bool croad;
  bool cres;

  fc_assert_ret_msg(NULL != pextra, "Bad extra %d.", p->id);

  names_set(&pextra->name, NULL, p->name, p->rule_name);

  pextra->category = p->category;

  pextra->causes = 0;
  for (i = 0; i < EC_COUNT; i++) {
    if (BV_ISSET(p->causes, i)) {
      pextra->causes |= (1 << i);
    }
  }

  pextra->rmcauses = 0;
  for (i = 0; i < ERM_COUNT; i++) {
    if (BV_ISSET(p->rmcauses, i)) {
      pextra->rmcauses |= (1 << i);
    }
  }

  if (pextra->causes == 0) {
    extra_to_caused_by_list(pextra, EC_NONE);
  } else {
    for (i = 0; i < EC_COUNT; i++) {
      if (is_extra_caused_by(pextra, i)) {
        extra_to_caused_by_list(pextra, i);
      }
    }
  }

  cbase = is_extra_caused_by(pextra, EC_BASE);
  croad = is_extra_caused_by(pextra, EC_ROAD);
  cres  = is_extra_caused_by(pextra, EC_RESOURCE);
  if (cbase) {
    /* Index is one less than size of list when this base is already added. */
    base_type_init(pextra, extra_type_list_size(extra_type_list_by_cause(EC_BASE)) - 1);
  }
  if (croad) {
    /* Index is one less than size of list when this road is already added. */
    road_type_init(pextra, extra_type_list_size(extra_type_list_by_cause(EC_ROAD)) - 1);
  }
  if (!cbase && !croad && !cres) {
    pextra->data.special_idx = extra_type_list_size(extra_type_list_by_cause(EC_SPECIAL));
    extra_to_caused_by_list(pextra, EC_SPECIAL);
  }

  for (i = 0; i < ERM_COUNT; i++) {
    if (is_extra_removed_by(pextra, i)) {
      extra_to_removed_by_list(pextra, i);
    }
  }

  sz_strlcpy(pextra->activity_gfx, p->activity_gfx);
  sz_strlcpy(pextra->act_gfx_alt, p->act_gfx_alt);
  sz_strlcpy(pextra->act_gfx_alt2, p->act_gfx_alt2);
  sz_strlcpy(pextra->rmact_gfx, p->rmact_gfx);
  sz_strlcpy(pextra->rmact_gfx_alt, p->rmact_gfx_alt);
  sz_strlcpy(pextra->graphic_str, p->graphic_str);
  sz_strlcpy(pextra->graphic_alt, p->graphic_alt);

  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&pextra->reqs, p->reqs[i]);
  }
  fc_assert(pextra->reqs.size == p->reqs_count);

  for (i = 0; i < p->rmreqs_count; i++) {
    requirement_vector_append(&pextra->rmreqs, p->rmreqs[i]);
  }
  fc_assert(pextra->rmreqs.size == p->rmreqs_count);

  pextra->appearance_chance = p->appearance_chance;
  for (i = 0; i < p->appearance_reqs_count; i++) {
    requirement_vector_append(&pextra->appearance_reqs, p->appearance_reqs[i]);
  }
  fc_assert(pextra->appearance_reqs.size == p->appearance_reqs_count);

  pextra->disappearance_chance = p->disappearance_chance;
  for (i = 0; i < p->disappearance_reqs_count; i++) {
    requirement_vector_append(&pextra->disappearance_reqs, p->disappearance_reqs[i]);
  }
  fc_assert(pextra->disappearance_reqs.size == p->disappearance_reqs_count);

  pextra->visibility_req = p->visibility_req;
  pextra->buildable = p->buildable;
  pextra->generated = p->generated;
  pextra->build_time = p->build_time;
  pextra->build_time_factor = p->build_time_factor;
  pextra->removal_time = p->removal_time;
  pextra->removal_time_factor = p->removal_time_factor;
  pextra->defense_bonus = p->defense_bonus;

  if (pextra->defense_bonus != 0) {
    if (extra_has_flag(pextra, EF_NATURAL_DEFENSE)) {
      extra_to_caused_by_list(pextra, EC_NATURAL_DEFENSIVE);
    } else {
      extra_to_caused_by_list(pextra, EC_DEFENSIVE);
    }
  }

  pextra->eus = p->eus;
  if (pextra->eus == EUS_HIDDEN) {
    extra_type_list_append(extra_type_list_of_unit_hiders(), pextra);
  }

  pextra->native_to = p->native_to;

  pextra->flags = p->flags;
  pextra->hidden_by = p->hidden_by;
  pextra->bridged_over = p->bridged_over;
  pextra->conflicts = p->conflicts;

  PACKET_STRVEC_EXTRACT(pextra->helptext, p->helptext);

  tileset_setup_extra(tileset, pextra);
}

/************************************************************************//**
  Packet ruleset_extra_flag handler.
****************************************************************************/
void handle_ruleset_extra_flag(const struct packet_ruleset_extra_flag *p)
{
  const char *flagname;
  const char *helptxt;

  fc_assert_ret_msg(p->id >= EF_USER_FLAG_1 && p->id <= EF_LAST_USER_FLAG,
                    "Bad user flag %d.", p->id);

  if (p->name[0] == '\0') {
    flagname = NULL;
  } else {
    flagname = p->name;
  }

  if (p->helptxt[0] == '\0') {
    helptxt = NULL;
  } else {
    helptxt = p->helptxt;
  }

  set_user_extra_flag_name(p->id, flagname, helptxt);
}

/************************************************************************//**
  Handle a packet about a particular base type.
****************************************************************************/
void handle_ruleset_base(const struct packet_ruleset_base *p)
{
  struct base_type *pbase = base_by_number(p->id);

  fc_assert_ret_msg(NULL != pbase, "Bad base %d.", p->id);

  pbase->gui_type = p->gui_type;
  pbase->border_sq  = p->border_sq;
  pbase->vision_main_sq = p->vision_main_sq;
  pbase->vision_invis_sq = p->vision_invis_sq;
  pbase->vision_subs_sq = p->vision_subs_sq;

  pbase->flags = p->flags;
}

/************************************************************************//**
  Handle a packet about a particular road type.
****************************************************************************/
void handle_ruleset_road(const struct packet_ruleset_road *p)
{
  int i;
  struct road_type *proad = road_by_number(p->id);

  fc_assert_ret_msg(NULL != proad, "Bad road %d.", p->id);

  for (i = 0; i < p->first_reqs_count; i++) {
    requirement_vector_append(&proad->first_reqs, p->first_reqs[i]);
  }
  fc_assert(proad->first_reqs.size == p->first_reqs_count);

  proad->move_cost = p->move_cost;
  proad->move_mode = p->move_mode;

  output_type_iterate(o) {
    proad->tile_incr_const[o] = p->tile_incr_const[o];
    proad->tile_incr[o] = p->tile_incr[o];
    proad->tile_bonus[o] = p->tile_bonus[o];
  } output_type_iterate_end;

  proad->compat = p->compat;
  proad->integrates = p->integrates;
  proad->flags = p->flags;
}

/************************************************************************//**
  Handle a packet about a particular goods type.
****************************************************************************/
void handle_ruleset_goods(const struct packet_ruleset_goods *p)
{
  struct goods_type *pgood = goods_by_number(p->id);
  int i;

  fc_assert_ret_msg(NULL != pgood, "Bad goods %d.", p->id);

  names_set(&pgood->name, NULL, p->name, p->rule_name);

  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&pgood->reqs, p->reqs[i]);
  }
  fc_assert(pgood->reqs.size == p->reqs_count);

  pgood->from_pct = p->from_pct;
  pgood->to_pct = p->to_pct;
  pgood->onetime_pct = p->onetime_pct;
  pgood->flags = p->flags;

  PACKET_STRVEC_EXTRACT(pgood->helptext, p->helptext);
}

/************************************************************************//**
  Handle a packet about a particular action.
****************************************************************************/
void handle_ruleset_action(const struct packet_ruleset_action *p)
{
  struct action *act;

  if (!action_id_exists(p->id)) {
    /* Action id out of range */
    log_error("handle_ruleset_action() the action id %d is out of range.",
              p->id);

    return;
  }

  act = action_by_number(p->id);

  sz_strlcpy(act->ui_name, p->ui_name);
  act->quiet = p->quiet;

  act->actor_kind  = p->act_kind;
  act->target_kind = p->tgt_kind;

  act->min_distance = p->min_distance;
  act->max_distance = p->max_distance;
  act->blocked_by = p->blocked_by;
}

/************************************************************************//**
  Handle a packet about a particular action enabler.
****************************************************************************/
void
handle_ruleset_action_enabler(const struct packet_ruleset_action_enabler *p)
{
  struct action_enabler *enabler;
  int i;

  if (!action_id_exists(p->enabled_action)) {
    /* Non existing action */
    log_error("handle_ruleset_action_enabler() the action %d "
              "doesn't exist.",
              p->enabled_action);

    return;
  }

  enabler = action_enabler_new();

  enabler->action = p->enabled_action;

  for (i = 0; i < p->actor_reqs_count; i++) {
    requirement_vector_append(&enabler->actor_reqs, p->actor_reqs[i]);
  }
  fc_assert(enabler->actor_reqs.size == p->actor_reqs_count);

  for (i = 0; i < p->target_reqs_count; i++) {
    requirement_vector_append(&enabler->target_reqs, p->target_reqs[i]);
  }
  fc_assert(enabler->target_reqs.size == p->target_reqs_count);

  action_enabler_add(enabler);
}

/************************************************************************//**
  Handle a packet about a particular action auto performer rule.
****************************************************************************/
void handle_ruleset_action_auto(const struct packet_ruleset_action_auto *p)
{
  struct action_auto_perf *auto_perf;
  int i;

  auto_perf = action_auto_perf_slot_number(p->id);

  auto_perf->cause = p->cause;

  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&auto_perf->reqs, p->reqs[i]);
  }
  fc_assert(auto_perf->reqs.size == p->reqs_count);

  for (i = 0; i < p->alternatives_count; i++) {
    auto_perf->alternatives[i] = p->alternatives[i];
  }
}

/************************************************************************//**
  Handle a packet about a particular disaster type.
****************************************************************************/
void handle_ruleset_disaster(const struct packet_ruleset_disaster *p)
{
  struct disaster_type *pdis = disaster_by_number(p->id);
  int i;

  fc_assert_ret_msg(NULL != pdis, "Bad disaster %d.", p->id);

  names_set(&pdis->name, NULL, p->name, p->rule_name);

  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&pdis->reqs, p->reqs[i]);
  }
  fc_assert(pdis->reqs.size == p->reqs_count);

  pdis->frequency = p->frequency;

  pdis->effects = p->effects;
}

/************************************************************************//**
  Handle a packet about a particular achievement type.
****************************************************************************/
void handle_ruleset_achievement(const struct packet_ruleset_achievement *p)
{
  struct achievement *pach = achievement_by_number(p->id);

  fc_assert_ret_msg(NULL != pach, "Bad achievement %d.", p->id);

  names_set(&pach->name, NULL, p->name, p->rule_name);

  pach->type = p->type;
  pach->unique = p->unique;
  pach->value = p->value;
}

/************************************************************************//**
  Handle a packet about a particular trade route type.
****************************************************************************/
void handle_ruleset_trade(const struct packet_ruleset_trade *p)
{
  struct trade_route_settings *pset = trade_route_settings_by_type(p->id);

  if (pset != NULL) {
    pset->trade_pct  = p->trade_pct;
    pset->cancelling = p->cancelling;
    pset->bonus_type = p->bonus_type;
  }
}

/************************************************************************//**
  Handle the terrain control ruleset packet sent by the server.
****************************************************************************/
void handle_ruleset_terrain_control
    (const struct packet_ruleset_terrain_control *p)
{
  /* Since terrain_control is the same as packet_ruleset_terrain_control
   * we can just copy the data directly. */
  terrain_control = *p;
  /* terrain_control.move_fragments likely changed */
  init_move_fragments();
}

/************************************************************************//**
  Handle the list of nation sets, sent as part of the ruleset.
****************************************************************************/
void handle_ruleset_nation_sets
    (const struct packet_ruleset_nation_sets *packet)
{
  int i;

  for (i = 0; i < packet->nsets; i++) {
    struct nation_set *pset;

    pset = nation_set_new(packet->names[i], packet->rule_names[i],
                          packet->descriptions[i]);
    fc_assert(NULL != pset);
    fc_assert(i == nation_set_index(pset));
  }
}

/************************************************************************//**
  Handle the list of nation groups, sent as part of the ruleset.
****************************************************************************/
void handle_ruleset_nation_groups
    (const struct packet_ruleset_nation_groups *packet)
{
  int i;

  for (i = 0; i < packet->ngroups; i++) {
    struct nation_group *pgroup;

    pgroup = nation_group_new(packet->groups[i]);
    fc_assert_action(NULL != pgroup, continue);
    fc_assert(i == nation_group_index(pgroup));
    pgroup->hidden = packet->hidden[i];
  }
}

/************************************************************************//**
  Handle initial ruleset nation info.
****************************************************************************/
void handle_ruleset_nation(const struct packet_ruleset_nation *packet)
{
  struct nation_type *pnation = nation_by_number(packet->id);
  int i;

  fc_assert_ret_msg(NULL != pnation, "Bad nation %d.", packet->id);

  if (packet->translation_domain[0] != '\0') {
    size_t len = strlen(packet->translation_domain) + 1;
    pnation->translation_domain = fc_malloc(len);
    fc_strlcpy(pnation->translation_domain, packet->translation_domain, len);
  } else {
    pnation->translation_domain = NULL;
  }
  names_set(&pnation->adjective, pnation->translation_domain,
            packet->adjective, packet->rule_name);
  name_set(&pnation->noun_plural, pnation->translation_domain, packet->noun_plural);
  sz_strlcpy(pnation->flag_graphic_str, packet->graphic_str);
  sz_strlcpy(pnation->flag_graphic_alt, packet->graphic_alt);
  pnation->style = style_by_number(packet->style);
  for (i = 0; i < packet->leader_count; i++) {
    (void) nation_leader_new(pnation, packet->leader_name[i],
                             packet->leader_is_male[i]);
  }

  /* set later by PACKET_NATION_AVAILABILITY */
  pnation->client.is_pickable = FALSE; 
  pnation->is_playable = packet->is_playable;
  pnation->barb_type = packet->barbarian_type;

  if ('\0' != packet->legend[0]) {
    pnation->legend = fc_strdup(nation_legend_translation(pnation, packet->legend));
  } else {
    pnation->legend = fc_strdup("");
  }

  for (i = 0; i < packet->nsets; i++) {
    struct nation_set *pset = nation_set_by_number(packet->sets[i]);

    if (NULL != pset) {
      nation_set_list_append(pnation->sets, pset);
    } else {
      log_error("handle_ruleset_nation() \"%s\" have unknown set %d.",
                nation_rule_name(pnation), packet->sets[i]);
    }
  }

  for (i = 0; i < packet->ngroups; i++) {
    struct nation_group *pgroup = nation_group_by_number(packet->groups[i]);

    if (NULL != pgroup) {
      nation_group_list_append(pnation->groups, pgroup);
    } else {
      log_error("handle_ruleset_nation() \"%s\" have unknown group %d.",
                nation_rule_name(pnation), packet->groups[i]);
    }
  }

  /* init_government may be NULL */
  pnation->init_government = government_by_number(packet->init_government_id);
  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    if (i < packet->init_techs_count) {
      pnation->init_techs[i] = packet->init_techs[i];
    } else {
      pnation->init_techs[i] = A_LAST;
    }
  }
  for (i = 0; i < MAX_NUM_UNIT_LIST; i++) {
    if (i < packet->init_units_count) {
      pnation->init_units[i] = utype_by_number(packet->init_units[i]);
    } else {
      /* TODO: should init_units be initialized in common/nation.c? */
      pnation->init_units[i] = utype_by_number(U_LAST);
    }
  }
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    if (i < packet->init_buildings_count) {
      pnation->init_buildings[i] = packet->init_buildings[i];
    } else {
      pnation->init_buildings[i] = B_LAST;
    }
  }

  tileset_setup_nation_flag(tileset, pnation);
}

/************************************************************************//**
  Handle nation availability info.
  This can change during pregame so is separate from ruleset_nation.
****************************************************************************/
void handle_nation_availability(int ncount, const bool *is_pickable,
                                bool nationset_change)
{
  int i;

  fc_assert_action(ncount == nation_count(),
                   ncount = MIN(ncount, nation_count()));

  for (i = 0; i < ncount; i++) {
    nation_by_number(i)->client.is_pickable = is_pickable[i];
  }

  races_update_pickable(nationset_change);
}

/************************************************************************//**
  Handle a packet about a particular style.
****************************************************************************/
void handle_ruleset_style(const struct packet_ruleset_style *p)
{
  struct nation_style *pstyle = style_by_number(p->id);

  fc_assert_ret_msg(NULL != pstyle, "Bad style %d.", p->id);

  names_set(&pstyle->name, NULL, p->name, p->rule_name);
}

/************************************************************************//**
  Handle a packet about a particular clause.
****************************************************************************/
void handle_ruleset_clause(const struct packet_ruleset_clause *p)
{
  struct clause_info *info = clause_info_get(p->type);

  fc_assert_ret_msg(NULL != info, "Bad clause %d.", p->type);

  info->enabled = p->enabled;
}

/************************************************************************//**
  Handle city style packet.
****************************************************************************/
void handle_ruleset_city(const struct packet_ruleset_city *packet)
{
  int id, j;
  struct citystyle *cs;

  id = packet->style_id;
  fc_assert_ret_msg(0 <= id && game.control.styles_count > id,
                    "Bad citystyle %d.", id);
  cs = &city_styles[id];

  for (j = 0; j < packet->reqs_count; j++) {
    requirement_vector_append(&cs->reqs, packet->reqs[j]);
  }
  fc_assert(cs->reqs.size == packet->reqs_count);

  names_set(&cs->name, NULL, packet->name, packet->rule_name);
  sz_strlcpy(cs->graphic, packet->graphic);
  sz_strlcpy(cs->graphic_alt, packet->graphic_alt);
  sz_strlcpy(cs->citizens_graphic, packet->citizens_graphic);
  sz_strlcpy(cs->citizens_graphic_alt, packet->citizens_graphic_alt);

  tileset_setup_city_tiles(tileset, id);
}

/************************************************************************//**
  Handle music style packet.
****************************************************************************/
void handle_ruleset_music(const struct packet_ruleset_music *packet)
{
  int id, j;
  struct music_style *pmus;

  id = packet->id;
  fc_assert_ret_msg(0 <= id && game.control.num_music_styles > id,
                    "Bad music_style %d.", id);

  pmus = music_style_by_number(id);

  for (j = 0; j < packet->reqs_count; j++) {
    requirement_vector_append(&pmus->reqs, packet->reqs[j]);
  }
  fc_assert(pmus->reqs.size == packet->reqs_count);

  sz_strlcpy(pmus->music_peaceful, packet->music_peaceful);
  sz_strlcpy(pmus->music_combat, packet->music_combat);
}

/************************************************************************//**
  Packet ruleset_game handler.
****************************************************************************/
void handle_ruleset_game(const struct packet_ruleset_game *packet)
{
  int i;

  /* Must set num_specialist_types before iterating over them. */
  DEFAULT_SPECIALIST = packet->default_specialist;

  fc_assert_ret(packet->veteran_levels > 0);

  game.veteran = veteran_system_new(packet->veteran_levels);
  game.veteran->levels = packet->veteran_levels;

  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    if (i < packet->global_init_techs_count) {
      game.rgame.global_init_techs[i] = packet->global_init_techs[i];
    } else {
      game.rgame.global_init_techs[i] = A_LAST;
    }
  }
  for (i = 0; i < MAX_NUM_BUILDING_LIST; i++) {
    if (i < packet->global_init_buildings_count) {
      game.rgame.global_init_buildings[i] = packet->global_init_buildings[i];
    } else {
      game.rgame.global_init_buildings[i] = B_LAST;
    }
  }

  for (i = 0; i < packet->veteran_levels; i++) {
    veteran_system_definition(game.veteran, i, packet->veteran_name[i],
                              packet->power_fact[i], packet->move_bonus[i],
                              packet->raise_chance[i],
                              packet->work_raise_chance[i]);
  }

  fc_assert(game.plr_bg_color == NULL);
  game.plr_bg_color = rgbcolor_new(packet->background_red,
                                   packet->background_green,
                                   packet->background_blue);

  tileset_background_init(tileset);
}

/************************************************************************//**
  Handle info about a single specialist.
****************************************************************************/
void handle_ruleset_specialist(const struct packet_ruleset_specialist *p)
{
  int j;
  struct specialist *s = specialist_by_number(p->id);

  fc_assert_ret_msg(NULL != s, "Bad specialist %d.", p->id);

  names_set(&s->name, NULL, p->plural_name, p->rule_name);
  name_set(&s->abbreviation, NULL, p->short_name);

  sz_strlcpy(s->graphic_str, p->graphic_str);
  sz_strlcpy(s->graphic_alt, p->graphic_alt);

  for (j = 0; j < p->reqs_count; j++) {
    requirement_vector_append(&s->reqs, p->reqs[j]);
  }
  fc_assert(s->reqs.size == p->reqs_count);

  PACKET_STRVEC_EXTRACT(s->helptext, p->helptext);

  tileset_setup_specialist_type(tileset, p->id);
}

/************************************************************************//**
  Handle reply to our city name request.
****************************************************************************/
void handle_city_name_suggestion_info(int unit_id, const char *name)
{
  struct unit *punit = player_unit_by_number(client_player(), unit_id);

  if (!can_client_issue_orders()) {
    return;
  }

  if (punit) {
    if (gui_options.ask_city_name) {
      bool other_asking = FALSE;

      unit_list_iterate(unit_tile(punit)->units, other) {
        if (other->client.asking_city_name) {
          other_asking = TRUE;
        }
      } unit_list_iterate_end;
      punit->client.asking_city_name = TRUE;

      if (!other_asking) {
        popup_newcity_dialog(punit, name);
      }
    } else {
      request_do_action(ACTION_FOUND_CITY,
                        unit_id, tile_index(unit_tile(punit)),
                        0, name);
    }
  }
}

/************************************************************************//**
  Handle the requested follow up question about an action

  The action can be a valid action or the special value ACTION_NONE.
  ACTION_NONE indicates that performing the action is impossible.
****************************************************************************/
void handle_unit_action_answer(int diplomat_id, int target_id, int cost,
                               action_id action_type,
                               bool disturb_player)
{
  struct city *pcity = game_city_by_number(target_id);
  struct unit *punit = game_unit_by_number(target_id);
  struct unit *pdiplomat = player_unit_by_number(client_player(),
                                                 diplomat_id);
  struct action *paction = action_by_number(action_type);

  if (ACTION_NONE != action_type
      && !action_id_exists(action_type)) {
    /* Non existing action */
    log_error("handle_unit_action_answer() the action %d doesn't exist.",
              action_type);

    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_decision_clear_want(diplomat_id);
      action_selection_next_in_focus(diplomat_id);
    }

    return;
  }

  if (!pdiplomat) {
    log_debug("Bad actor %d.", diplomat_id);

    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_selection_next_in_focus(diplomat_id);
    }

    return;
  }

  switch ((enum gen_action)action_type) {
  case ACTION_SPY_BRIBE_UNIT:
    if (punit && client.conn.playing
        && is_human(client.conn.playing)) {
      if (disturb_player) {
        /* Focus on the unit so the player knows where it is */
        unit_focus_set(pdiplomat);

        popup_bribe_dialog(pdiplomat, punit, cost, paction);
      } else {
        /* Not in use (yet). */
        log_error("Unimplemented: received background unit bribe cost.");
      }
    } else {
      log_debug("Bad target %d.", target_id);
      if (disturb_player) {
        action_selection_no_longer_in_progress(diplomat_id);
        action_decision_clear_want(diplomat_id);
        action_selection_next_in_focus(diplomat_id);
      }
    }
    break;
  case ACTION_SPY_INCITE_CITY:
  case ACTION_SPY_INCITE_CITY_ESC:
    if (pcity && client.conn.playing
        && is_human(client.conn.playing)) {
      if (disturb_player) {
        /* Focus on the unit so the player knows where it is */
        unit_focus_set(pdiplomat);

        popup_incite_dialog(pdiplomat, pcity, cost, paction);
      } else {
        /* Not in use (yet). */
        log_error("Unimplemented: received background city incite cost.");
      }
    } else {
      log_debug("Bad target %d.", target_id);
      if (disturb_player) {
        action_selection_no_longer_in_progress(diplomat_id);
        action_decision_clear_want(diplomat_id);
        action_selection_next_in_focus(diplomat_id);
      }
    }
    break;
  case ACTION_UPGRADE_UNIT:
    if (pcity && client.conn.playing
        && is_human(client.conn.playing)) {
      /* TODO: The bundled clients will have to start showing the upgrade
       * price sent from the server before it can be allowed to rely on
       * things the player can't see. (Example: it becomes legal to upgrade
       * a unit in a foreign city.) */

      /* Getting unit upgrade cost from the server is currently only used by
       * Freeciv-web.  */
      log_error("Received upgrade unit price but can't forward it.");
    }
    break;
  case ACTION_NONE:
    log_debug("Server didn't respond to query.");
    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_decision_clear_want(diplomat_id);
      action_selection_next_in_focus(diplomat_id);
    }
    break;
  default:
    log_error("handle_unit_action_answer() invalid action_type (%d).",
              action_type);
    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_decision_clear_want(diplomat_id);
      action_selection_next_in_focus(diplomat_id);
    }
    break;
  };
}

/************************************************************************//**
  Returns a possibly legal attack action iff it is the only interesting
  action that currently is legal.
****************************************************************************/
static action_id auto_attack_act(const struct act_prob *act_probs)
{
  action_id attack_action = ACTION_NONE;

  action_iterate(act) {
    if (action_prob_possible(act_probs[act])) {
      switch ((enum gen_action)act) {
      case ACTION_DISBAND_UNIT:
      case ACTION_FORTIFY:
      case ACTION_CONVERT:
        /* Not interesting. */
        break;
      case ACTION_CAPTURE_UNITS:
      case ACTION_BOMBARD:
      case ACTION_NUKE:
      case ACTION_ATTACK:
      case ACTION_SUICIDE_ATTACK:
      case ACTION_CONQUER_CITY:
        /* An attack. */
        if (attack_action == ACTION_NONE) {
          /* No previous attack action found. */
          attack_action = act;
        } else {
          /* More than one legal attack action found. */
          return ACTION_NONE;
        }
        break;
      case ACTION_ESTABLISH_EMBASSY:
      case ACTION_ESTABLISH_EMBASSY_STAY:
      case ACTION_SPY_INVESTIGATE_CITY:
      case ACTION_INV_CITY_SPEND:
      case ACTION_SPY_POISON:
      case ACTION_SPY_POISON_ESC:
      case ACTION_SPY_STEAL_GOLD:
      case ACTION_SPY_STEAL_GOLD_ESC:
      case ACTION_SPY_SABOTAGE_CITY:
      case ACTION_SPY_SABOTAGE_CITY_ESC:
      case ACTION_SPY_TARGETED_SABOTAGE_CITY:
      case ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC:
      case ACTION_SPY_STEAL_TECH:
      case ACTION_SPY_STEAL_TECH_ESC:
      case ACTION_SPY_TARGETED_STEAL_TECH:
      case ACTION_SPY_TARGETED_STEAL_TECH_ESC:
      case ACTION_SPY_INCITE_CITY:
      case ACTION_SPY_INCITE_CITY_ESC:
      case ACTION_TRADE_ROUTE:
      case ACTION_MARKETPLACE:
      case ACTION_HELP_WONDER:
      case ACTION_SPY_BRIBE_UNIT:
      case ACTION_SPY_SABOTAGE_UNIT:
      case ACTION_SPY_SABOTAGE_UNIT_ESC:
      case ACTION_FOUND_CITY:
      case ACTION_JOIN_CITY:
      case ACTION_STEAL_MAPS:
      case ACTION_STEAL_MAPS_ESC:
      case ACTION_SPY_NUKE:
      case ACTION_SPY_NUKE_ESC:
      case ACTION_DESTROY_CITY:
      case ACTION_EXPEL_UNIT:
      case ACTION_RECYCLE_UNIT:
      case ACTION_HOME_CITY:
      case ACTION_UPGRADE_UNIT:
      case ACTION_PARADROP:
      case ACTION_AIRLIFT:
      case ACTION_HEAL_UNIT:
      case ACTION_TRANSFORM_TERRAIN:
      case ACTION_IRRIGATE_TF:
      case ACTION_MINE_TF:
      case ACTION_PILLAGE:
      case ACTION_ROAD:
      case ACTION_BASE:
      case ACTION_MINE:
      case ACTION_IRRIGATE:
        /* An interesting non attack action has been found. */
        return ACTION_NONE;
        break;
      case ACTION_COUNT:
        fc_assert(act != ACTION_COUNT);
        break;
      }
    }
  } action_iterate_end;

  return attack_action;
}

/************************************************************************//**
  Handle reply to possible actions.

  Note that a reply to a foreground request (a reply where disturb_player
  is true) must result in its clean up.
****************************************************************************/
void handle_unit_actions(const struct packet_unit_actions *packet)
{
  struct unit *actor_unit = game_unit_by_number(packet->actor_unit_id);

  struct tile *target_tile = index_to_tile(&(wld.map), packet->target_tile_id);
  struct extra_type *target_extra = packet->target_extra_id == EXTRA_NONE ?
      NULL : extra_by_number(packet->target_extra_id);
  struct city *target_city = game_city_by_number(packet->target_city_id);
  struct unit *target_unit = game_unit_by_number(packet->target_unit_id);

  const struct act_prob *act_probs = packet->action_probabilities;

  bool disturb_player = packet->disturb_player;
  bool valid = FALSE;

  /* The dead can't act */
  if (actor_unit && (target_tile || target_city || target_unit)) {
    /* At least one action must be possible */
    action_iterate(act) {
      if (action_prob_possible(act_probs[act])) {
        valid = TRUE;
        break;
      }
    } action_iterate_end;
  }

  if (valid && disturb_player) {
    /* The player can select an action and should be informed. */

    action_id auto_action;

    if (gui_options.popup_attack_actions) {
      /* Pop up the action selection dialog no matter what. */
      auto_action = ACTION_NONE;
    } else {
      /* Pop up the action selection dialog unless the only interesting
       * action the unit may be able to do is an attack action. */
      auto_action = auto_attack_act(act_probs);
    }

    if (auto_action != ACTION_NONE) {
      /* No interesting actions except a single attack action has been
       * found. The player wants it performed without questions. */

      /* The order requests below doesn't send additional details. */
      fc_assert(!action_requires_details(auto_action));

      /* Give the order. */
      switch(action_id_get_target_kind(auto_action)) {
      case ATK_TILE:
      case ATK_UNITS:
        request_do_action(auto_action,
                          packet->actor_unit_id, packet->target_tile_id,
                          0, "");
        break;
      case ATK_CITY:
        request_do_action(auto_action,
                          packet->actor_unit_id, packet->target_city_id,
                          0, "");
        break;
      case ATK_UNIT:
        request_do_action(auto_action,
                          packet->actor_unit_id, packet->target_unit_id,
                          0, "");
        break;
      case ATK_SELF:
        request_do_action(auto_action,
                          packet->actor_unit_id, packet->actor_unit_id,
                          0, "");
        break;
      case ATK_COUNT:
        fc_assert(action_id_get_target_kind(auto_action) != ATK_COUNT);
        break;
      }

      /* Clean up. */
      action_selection_no_longer_in_progress(packet->actor_unit_id);
      action_decision_clear_want(packet->actor_unit_id);
      action_selection_next_in_focus(packet->actor_unit_id);
    } else {
      /* Show the client specific action dialog */
      popup_action_selection(actor_unit,
                             target_city, target_unit,
                             target_tile, target_extra,
                             act_probs);
    }
  } else if (disturb_player) {
    /* Nothing to do. */
    action_selection_no_longer_in_progress(packet->actor_unit_id);
    action_decision_clear_want(packet->actor_unit_id);
    action_selection_next_in_focus(packet->actor_unit_id);
  } else {
    /* This was a background request. */

    if (action_selection_actor_unit() == actor_unit->id) {
      /* The situation may have changed. */
      action_selection_refresh(actor_unit,
                               target_city, target_unit,
                               target_tile, target_extra,
                               act_probs);
    }
  }
}

/************************************************************************//**
  Handle list of potenttial buildings to sabotage.
****************************************************************************/
void handle_city_sabotage_list(int diplomat_id, int city_id,
                               bv_imprs improvements,
                               action_id act_id,
                               bool disturb_player)
{
  struct city *pcity = game_city_by_number(city_id);
  struct unit *pdiplomat = player_unit_by_number(client_player(),
                                                 diplomat_id);
  struct action *paction = action_by_number(act_id);

  if (!pdiplomat) {
    log_debug("Bad diplomat %d.", diplomat_id);

    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_selection_next_in_focus(diplomat_id);
    }

    return;
  }

  if (!pcity) {
    log_debug("Bad city %d.", city_id);

    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_decision_clear_want(diplomat_id);
      action_selection_next_in_focus(diplomat_id);
    }

    return;
  }

  if (can_client_issue_orders()) {
    improvement_iterate(pimprove) {
      update_improvement_from_packet(pcity, pimprove,
                                     BV_ISSET(improvements,
                                              improvement_index(pimprove)));
    } improvement_iterate_end;

    if (disturb_player) {
      /* Focus on the unit so the player knows where it is */
      unit_focus_set(pdiplomat);

      popup_sabotage_dialog(pdiplomat, pcity, paction);
    } else {
      /* Not in use (yet). */
      log_error("Unimplemented: received background city building list.");
    }
  } else {
    log_debug("Can't issue orders");
    if (disturb_player) {
      action_selection_no_longer_in_progress(diplomat_id);
      action_decision_clear_want(diplomat_id);
    }
  }
}

/************************************************************************//**
  Pass the header information about things be displayed in a gui-specific
  endgame dialog.
****************************************************************************/
void handle_endgame_report(const struct packet_endgame_report *packet)
{
  set_client_state(C_S_OVER);
  endgame_report_dialog_start(packet);
}

/************************************************************************//**
  Pass endgame report about single player.
****************************************************************************/
void handle_endgame_player(const struct packet_endgame_player *packet)
{
  if (client_has_player()
      && packet->player_id == player_number(client_player())) {
    if (packet->winner) {
      start_menu_music("music_victory", NULL);
    } else {
      start_menu_music("music_defeat", NULL);
    }
  }
  endgame_report_dialog_player(packet);
}

/************************************************************************//**
  Packet player_attribute_chunk handler.
****************************************************************************/
void handle_player_attribute_chunk
    (const struct packet_player_attribute_chunk *packet)
{
  if (!client_has_player()) {
    return;
  }

  generic_handle_player_attribute_chunk(client_player(), packet);

  if (packet->offset + packet->chunk_length == packet->total_length) {
    /* We successful received the last chunk. The attribute block is
       now complete. */
      attribute_restore();
  }
}

/************************************************************************//**
  Handle request to start processing packet.
****************************************************************************/
void handle_processing_started(void)
{
  agents_processing_started();

  fc_assert(client.conn.client.request_id_of_currently_handled_packet == 0);
  client.conn.client.request_id_of_currently_handled_packet =
      get_next_request_id(client.conn.
                          client.last_processed_request_id_seen);
  update_queue_processing_started(client.conn.client.
                                  request_id_of_currently_handled_packet);

  log_debug("start processing packet %d",
            client.conn.client.request_id_of_currently_handled_packet);
}

/************************************************************************//**
  Handle request to stop processing packet.
****************************************************************************/
void handle_processing_finished(void)
{
  log_debug("finish processing packet %d",
            client.conn.client.request_id_of_currently_handled_packet);

  fc_assert(client.conn.client.request_id_of_currently_handled_packet != 0);

  client.conn.client.last_processed_request_id_seen =
      client.conn.client.request_id_of_currently_handled_packet;
  update_queue_processing_finished(client.conn.client.
                                   last_processed_request_id_seen);

  client.conn.client.request_id_of_currently_handled_packet = 0;

  agents_processing_finished();
}

/************************************************************************//**
  Notify interested parties about incoming packet.
****************************************************************************/
void notify_about_incoming_packet(struct connection *pc,
                                  int packet_type, int size)
{
  fc_assert(pc == &client.conn);
  log_debug("incoming packet={type=%d, size=%d}", packet_type, size);
}

/************************************************************************//**
  Notify interested parties about outgoing packet.
****************************************************************************/
void notify_about_outgoing_packet(struct connection *pc,
                                  int packet_type, int size,
                                  int request_id)
{
  fc_assert(pc == &client.conn);
  log_debug("outgoing packet={type=%d, size=%d, request_id=%d}",
            packet_type, size, request_id);

  fc_assert(request_id);
}

/************************************************************************//**
  We have received PACKET_FREEZE_CLIENT.
****************************************************************************/
void handle_freeze_client(void)
{
  log_debug("handle_freeze_client");

  agents_freeze_hint();
}

/************************************************************************//**
  We have received PACKET_THAW_CLIENT
****************************************************************************/
void handle_thaw_client(void)
{
  log_debug("handle_thaw_client");

  agents_thaw_hint();
  update_turn_done_button_state();
}

/************************************************************************//**
  Reply to 'ping' packet with 'pong'
****************************************************************************/
void handle_conn_ping(void)
{
  send_packet_conn_pong(&client.conn);
}

/************************************************************************//**
  Handle server shutdown.
****************************************************************************/
void handle_server_shutdown(void)
{
  log_verbose("server shutdown");
}

/************************************************************************//**
  Add effect data to ruleset cache.
****************************************************************************/
void handle_ruleset_effect(const struct packet_ruleset_effect *packet)
{
  recv_ruleset_effect(packet);
}

/************************************************************************//**
  Handle a notification from the server that an object was successfully
  created. The 'tag' was previously sent to the server when the client
  requested the creation. The 'id' is the identifier of the newly created
  object.
****************************************************************************/
void handle_edit_object_created(int tag, int id)
{
  editgui_notify_object_created(tag, id);
}

/************************************************************************//**
  Handle start position creation/removal.
****************************************************************************/
void handle_edit_startpos(const struct packet_edit_startpos *packet)
{
  struct tile *ptile = index_to_tile(&(wld.map), packet->id);
  bool changed = FALSE;

  /* Check. */
  if (NULL == ptile) {
    log_error("%s(): invalid tile index %d.", __FUNCTION__, packet->id);
    return;
  }

  /* Handle. */
  if (packet->removal) {
    changed = map_startpos_remove(ptile);
  } else {
    if (NULL != map_startpos_get(ptile)) {
      changed = FALSE;
    } else {
      map_startpos_new(ptile);
      changed = TRUE;
    }
  }

  /* Notify. */
  if (changed && can_client_change_view()) {
    refresh_tile_mapcanvas(ptile, TRUE, FALSE);
    if (packet->removal) {
      editgui_notify_object_changed(OBJTYPE_STARTPOS,
                                    packet->id, TRUE);
    } else {
      editgui_notify_object_created(packet->tag, packet->id);
    }
  }
}

/************************************************************************//**
  Handle start position internal information.
****************************************************************************/
void handle_edit_startpos_full(const struct packet_edit_startpos_full *
                               packet)
{
  struct tile *ptile = index_to_tile(&(wld.map), packet->id);
  struct startpos *psp;

  /* Check. */
  if (NULL == ptile) {
    log_error("%s(): invalid tile index %d.", __FUNCTION__, packet->id);
    return;
  }

  psp = map_startpos_get(ptile);
  if (NULL == psp) {
    log_error("%s(): no start position at (%d, %d)",
              __FUNCTION__, TILE_XY(ptile));
    return;
  }

  /* Handle. */
  if (startpos_unpack(psp, packet) && can_client_change_view()) {
    /* Notify. */
    refresh_tile_mapcanvas(ptile, TRUE, FALSE);
    editgui_notify_object_changed(OBJTYPE_STARTPOS, startpos_number(psp),
                                  FALSE);
  }
}

/************************************************************************//**
  A vote no longer exists. Remove from queue and update gui.
****************************************************************************/
void handle_vote_remove(int vote_no)
{
  voteinfo_queue_delayed_remove(vote_no);
  voteinfo_gui_update();
}

/************************************************************************//**
  Find and update the corresponding vote and refresh the GUI.
****************************************************************************/
void handle_vote_update(int vote_no, int yes, int no, int abstain,
                        int num_voters)
{
  struct voteinfo *vi;

  vi = voteinfo_queue_find(vote_no);
  fc_assert_ret_msg(NULL != vi,
                    "Got packet_vote_update for non-existant vote %d!",
                    vote_no);

  vi->yes = yes;
  vi->no = no;
  vi->abstain = abstain;
  vi->num_voters = num_voters;

  voteinfo_gui_update();
}

/************************************************************************//**
  Create a new vote and add it to the queue. Refresh the GUI.
****************************************************************************/
void handle_vote_new(const struct packet_vote_new *packet)
{
  fc_assert_ret_msg(NULL == voteinfo_queue_find(packet->vote_no),
                    "Got a packet_vote_new for already existing "
                    "vote %d!", packet->vote_no);

  voteinfo_queue_add(packet->vote_no,
                     packet->user,
                     packet->desc,
                     packet->percent_required,
                     packet->flags);
  voteinfo_gui_update();
}

/************************************************************************//**
  Update the vote's status and refresh the GUI.
****************************************************************************/
void handle_vote_resolve(int vote_no, bool passed)
{
  struct voteinfo *vi;

  vi = voteinfo_queue_find(vote_no);
  fc_assert_ret_msg(NULL != vi,
                    "Got packet_vote_resolve for non-existant vote %d!",
                    vote_no);

  vi->resolved = TRUE;
  vi->passed = passed;

  voteinfo_gui_update();
}

/************************************************************************//**
  Shows image with text and play music
****************************************************************************/
void handle_show_img_play_sound(const char *img_path, const char *snd_path,
                                const char *desc, bool fullsize)
{
  show_img_play_snd(img_path, snd_path, desc, fullsize);
}

/************************************************************************//**
  Play suitable music
****************************************************************************/
void handle_play_music(const char *tag)
{
  play_single_track(tag);
}
