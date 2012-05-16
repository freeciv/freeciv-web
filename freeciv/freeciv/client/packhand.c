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
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "capstr.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "spaceship.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"
#include "worklist.h"

/* include */
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
#include "ggzclient.h"
#include "goto.h"               /* client_goto_init() */
#include "helpdata.h"           /* boot_help_texts() */
#include "mapview_common.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"
#include "voteinfo.h"

#include "packhand.h"

static void city_packet_common(struct city *pcity, struct tile *pcenter,
                               struct player *powner, bool is_new,
                               bool popup, bool investigate);
static bool handle_unit_packet_common(struct unit *packet_unit);


static int *reports_thaw_requests = NULL;
static int reports_thaw_requests_size = 0;

/* The dumbest of cities, placeholders for unknown and unseen cities. */
static struct city_list *invisible_cities = NULL;


/****************************************************************************
  Called below, and by client/civclient.c client_game_free()
****************************************************************************/
void packhand_free(void)
{
  if (NULL != invisible_cities) {
    city_list_iterate(invisible_cities, pcity) {
      idex_unregister_city(pcity);
      destroy_city_virtual(pcity);
    } city_list_iterate_end;

    city_list_free(invisible_cities);
    invisible_cities = NULL;
  }
}

/****************************************************************************
  Called only by handle_map_info() below.
****************************************************************************/
static void packhand_init(void)
{
  packhand_free();

  invisible_cities = city_list_new();
}

/**************************************************************************
  Unpackage the unit information into a newly allocated unit structure.

  Information for the client must also be processed in
  handle_unit_packet_common()!
**************************************************************************/
static struct unit * unpackage_unit(struct packet_unit_info *packet)
{
  struct unit *punit = create_unit_virtual(valid_player_by_number(packet->owner),
					   NULL,
					   utype_by_number(packet->type),
					   packet->veteran);

  /* Owner, veteran, and type fields are already filled in by
   * create_unit_virtual. */
  punit->id = packet->id;
  punit->tile = map_pos_to_tile(packet->x, packet->y);
  punit->homecity = packet->homecity;
  output_type_iterate(o) {
    punit->upkeep[o] = packet->upkeep[o];
  } output_type_iterate_end;
  punit->moves_left = packet->movesleft;
  punit->hp = packet->hp;
  punit->activity = packet->activity;
  punit->activity_count = packet->activity_count;
  punit->ai.control = packet->ai;
  punit->fuel = packet->fuel;
  if (is_normal_map_pos(packet->goto_dest_x, packet->goto_dest_y)) {
    punit->goto_tile = map_pos_to_tile(packet->goto_dest_x,
				       packet->goto_dest_y);
  } else {
    punit->goto_tile = NULL;
  }
  punit->activity_target = packet->activity_target;
  punit->activity_base = packet->activity_base;
  punit->paradropped = packet->paradropped;
  punit->done_moving = packet->done_moving;
  punit->occupy = packet->occupy;
  if (packet->transported) {
    punit->transported_by = packet->transported_by;
  } else {
    punit->transported_by = -1;
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
      punit->orders.list[i].base = packet->orders_bases[i];
    }
  }
  return punit;
}

/**************************************************************************
  Unpackage a short_unit_info packet.  This extracts a limited amount of
  information about the unit, and is sent for units we shouldn't know
  everything about (like our enemies' units).

  Information for the client must also be processed in
  handle_unit_packet_common()!
**************************************************************************/
static struct unit *unpackage_short_unit(struct packet_unit_short_info *packet)
{
  struct unit *punit = create_unit_virtual(valid_player_by_number(packet->owner),
					   NULL,
					   utype_by_number(packet->type),
					   FALSE);

  /* Owner and type fields are already filled in by create_unit_virtual. */
  punit->id = packet->id;
  punit->tile = map_pos_to_tile(packet->x, packet->y);
  punit->veteran = packet->veteran;
  punit->hp = packet->hp;
  punit->activity = packet->activity;
  punit->activity_base = packet->activity_base;
  punit->occupy = (packet->occupied ? 1 : 0);
  if (packet->transported) {
    punit->transported_by = packet->transported_by;
  } else {
    punit->transported_by = -1;
  }

  return punit;
}

/****************************************************************************
  After we send a join packet to the server we receive a reply.  This
  function handles the reply.
****************************************************************************/
void handle_server_join_reply(bool you_can_join, char *message,
                              char *capability, char *challenge_file,
                              int conn_id)
{
  char *s_capability = client.conn.capability;

  sz_strlcpy(client.conn.capability, capability);
  close_connection_dialog();

  if (you_can_join) {
    struct packet_client_info client_info;

    freelog(LOG_VERBOSE, "join game accept:%s", message);
    client.conn.established = TRUE;
    client.conn.id = conn_id;

    agents_game_joined();
    update_menus();
    set_server_busy(FALSE);
    
    if (get_client_page() == PAGE_MAIN
	|| get_client_page() == PAGE_NETWORK
	|| get_client_page() == PAGE_GGZ) {
      set_client_page(PAGE_START);
    }

    client_info.gui = get_gui_type();
    send_packet_client_info(&client.conn, &client_info);

    /* we could always use hack, verify we're local */ 
    send_client_wants_hack(challenge_file);

    set_client_state(C_S_PREPARING);
  } else {
    output_window_printf(FTC_SERVER_INFO, NULL,
                         _("You were rejected from the game: %s"), message);
    client.conn.id = -1; /* not in range of conn_info id */

    if (auto_connect) {
      freelog(LOG_NORMAL, _("You were rejected from the game: %s"), message);
    }
    gui_server_connect();

    if (!with_ggz) {
      set_client_page(in_ggz ? PAGE_MAIN : PAGE_GGZ);
    }
  }
  if (strcmp(s_capability, our_capability) == 0) {
    return;
  }
  output_window_printf(FTC_SERVER_INFO, NULL,
                       _("Client capability string: %s"), our_capability);
  output_window_printf(FTC_SERVER_INFO, NULL,
                       _("Server capability string: %s"), s_capability);
}

/****************************************************************************
  Handles a remove-city packet, used by the server to tell us any time a
  city is no longer there.
****************************************************************************/
void handle_city_remove(int city_id)
{
  struct city *pcity = game_find_city_by_number(city_id);

  if (NULL == pcity) {
    freelog(LOG_ERROR,
	    "handle_city_remove() bad city %d.",
	    city_id);
    return;
  }

  agents_city_remove(pcity);
  editgui_notify_object_changed(OBJTYPE_CITY, pcity->id, TRUE);
  client_remove_city(pcity);

  /* update menus if the focus unit is on the tile. */
  if (get_num_units_in_focus() > 0) {
    update_menus();
  }
}

/**************************************************************************
  Handle a remove-unit packet, sent by the server to tell us any time a
  unit is no longer there.
**************************************************************************/
void handle_unit_remove(int unit_id)
{
  struct unit *punit = game_find_unit_by_number(unit_id);
  struct player *powner;

  if (!punit) {
    return;
  }
  
  /* Close diplomat dialog if the diplomat is lost */
  if (diplomat_handled_in_diplomat_dialog() == punit->id) {
    close_diplomat_dialog();
    /* Open another diplomat dialog if there are other diplomats waiting */
    process_diplomat_arrival(NULL, 0);
  }

  powner = unit_owner(punit);

  agents_unit_remove(punit);
  editgui_notify_object_changed(OBJTYPE_UNIT, punit->id, TRUE);
  client_remove_unit(punit);

  if (powner == client.conn.playing) {
    activeunits_report_dialog_update();
  }
}

/****************************************************************************
  The tile (x,y) has been nuked!
****************************************************************************/
void handle_nuke_tile_info(int x, int y)
{
  put_nuke_mushroom_pixmaps(map_pos_to_tile(x, y));
}

/****************************************************************************
  A combat packet.  The server tells us the attacker and defender as well
  as both of their hitpoints after the combat is over (in most combat, one
  unit always dies and their HP drops to zero).  If make_winner_veteran is
  set then the surviving unit becomes veteran.
****************************************************************************/
void handle_unit_combat_info(int attacker_unit_id, int defender_unit_id,
			     int attacker_hp, int defender_hp,
			     bool make_winner_veteran)
{
  bool show_combat = FALSE;
  struct unit *punit0 = game_find_unit_by_number(attacker_unit_id);
  struct unit *punit1 = game_find_unit_by_number(defender_unit_id);

  if (punit0 && punit1) {
    if (tile_visible_mapcanvas(punit0->tile) &&
	tile_visible_mapcanvas(punit1->tile)) {
      show_combat = TRUE;
    } else if (auto_center_on_combat) {
      if (unit_owner(punit0) == client.conn.playing)
	center_tile_mapcanvas(punit0->tile);
      else
	center_tile_mapcanvas(punit1->tile);
      show_combat = TRUE;
    }

    if (show_combat) {
      int hp0 = attacker_hp, hp1 = defender_hp;

      audio_play_sound(unit_type(punit0)->sound_fight,
		       unit_type(punit0)->sound_fight_alt);
      audio_play_sound(unit_type(punit1)->sound_fight,
		       unit_type(punit1)->sound_fight_alt);

      if (do_combat_animation) {
	decrease_unit_hp_smooth(punit0, hp0, punit1, hp1);
      } else {
	punit0->hp = hp0;
	punit1->hp = hp1;

	set_units_in_combat(NULL, NULL);
	refresh_unit_mapcanvas(punit0, punit0->tile, TRUE, FALSE);
	refresh_unit_mapcanvas(punit1, punit1->tile, TRUE, FALSE);
      }
    }
    if (make_winner_veteran) {
      struct unit *pwinner = (defender_hp == 0 ? punit0 : punit1);

      pwinner->veteran++;
      refresh_unit_mapcanvas(pwinner, pwinner->tile, TRUE, FALSE);
    }
  }
}

/**************************************************************************
  Updates a city's list of improvements from packet data.
  "have_impr" specifies whether the improvement should
  be added (TRUE) or removed (FALSE).
**************************************************************************/
static void update_improvement_from_packet(struct city *pcity,
					   struct impr_type *pimprove,
					   bool have_impr)
{
  if (have_impr) {
    if (pcity->built[improvement_index(pimprove)].turn <= I_NEVER) {
      city_add_improvement(pcity, pimprove);
    }
  } else {
    if (pcity->built[improvement_index(pimprove)].turn > I_NEVER) {
      city_remove_improvement(pcity, pimprove);
    }
  }
}

/****************************************************************************
  A city-info packet contains all information about a city.  If we receive
  this packet then we know everything about the city internals.
****************************************************************************/
void handle_city_info(struct packet_city_info *packet)
{
  struct universal product;
  int caravan_city_id;
  int i;
  bool popup;
  bool city_is_new = FALSE;
  bool city_has_changed_owner = FALSE;
  bool need_units_dialog_update = FALSE;
  bool name_changed = FALSE;
  bool update_descriptions = FALSE;
  bool shield_stock_changed = FALSE;
  bool production_changed = FALSE;
  struct unit_list *pfocus_units = get_units_in_focus();
  struct city *pcity = game_find_city_by_number(packet->id);
  struct tile *pcenter = map_pos_to_tile(packet->x, packet->y);
  struct tile *ptile = NULL;
  struct player *powner = valid_player_by_number(packet->owner);

  if (NULL == powner) {
    freelog(LOG_ERROR,
            "handle_city_info() bad player number %d.",
            packet->owner);
    return;
  }

  if (NULL == pcenter) {
    freelog(LOG_ERROR,
            "handle_city_info() invalid tile (%d,%d).",
            TILE_XY(packet));
    return;
  }

  if (packet->production_kind < VUT_NONE || packet->production_kind >= VUT_LAST) {
    freelog(LOG_ERROR, "handle_city_info()"
            " bad production_kind %d.",
            packet->production_kind);
    product.kind = VUT_NONE;
  } else {
    product = universal_by_number(packet->production_kind,
                                  packet->production_value);
    if (product.kind < VUT_NONE || product.kind >= VUT_LAST) {
      freelog(LOG_ERROR, "handle_city_info()"
              " production_kind %d with bad production_value %d.",
              packet->production_kind,
              packet->production_value);
      product.kind = VUT_NONE;
    }
  }

  if (NULL != pcity) {
    ptile = city_tile(pcity);

    if (NULL == ptile) {
      /* invisible worked city */
      city_list_unlink(invisible_cities, pcity);
      city_is_new = TRUE;

      pcity->tile = pcenter;
      ptile = pcenter;
      pcity->owner = powner;
      pcity->original = powner;

    } else if (city_owner(pcity) != powner) {
      client_remove_city(pcity);
      pcity = NULL;
      city_has_changed_owner = TRUE;
    }
  }

  if (NULL == pcity) {
    city_is_new = TRUE;
    pcity = create_city_virtual(powner, pcenter, packet->name);
    pcity->id = packet->id;
    idex_register_city(pcity);
    update_descriptions = TRUE;
  } else if (pcity->id != packet->id) {
    freelog(LOG_ERROR, "handle_city_info()"
            " city id %d != id %d.",
            pcity->id,
            packet->id);
    return;
  } else if (ptile != pcenter) {
    freelog(LOG_ERROR, "handle_city_info()"
            " city tile (%d,%d) != (%d,%d).",
            TILE_XY(ptile),
            TILE_XY(packet));
    return;
  } else {
    bool traderoutes_changed = FALSE;
    name_changed = (0 != strncmp(packet->name, pcity->name,
                                 sizeof(pcity->name)));

    if (draw_city_traderoutes
        && (0 != memcmp(pcity->trade, packet->trade,
                        sizeof(pcity->trade))
            || 0 != memcmp(pcity->trade_value, packet->trade_value,
                           sizeof(pcity->trade_value)))) {
      traderoutes_changed = TRUE;
    }

    /* Descriptions should probably be updated if the
     * city name, production or time-to-grow changes.
     * Note that if either the food stock or surplus
     * have changed, the time-to-grow is likely to
     * have changed as well. */
    update_descriptions = (draw_city_names && name_changed)
      || (draw_city_productions
          && (!are_universals_equal(&pcity->production, &product)
              || pcity->surplus[O_SHIELD] != packet->surplus[O_SHIELD]
              || pcity->shield_stock != packet->shield_stock))
      || (draw_city_growth
          && (pcity->food_stock != packet->food_stock
              || pcity->surplus[O_FOOD] != packet->surplus[O_FOOD]))
      || (draw_city_traderoutes && traderoutes_changed);
  }
  
  sz_strlcpy(pcity->name, packet->name);
  
  /* check data */
  pcity->size = 0;
  for (i = 0; i < FEELING_LAST; i++) {
    pcity->feel[CITIZEN_HAPPY][i] = packet->ppl_happy[i];
    pcity->feel[CITIZEN_CONTENT][i] = packet->ppl_content[i];
    pcity->feel[CITIZEN_UNHAPPY][i] = packet->ppl_unhappy[i];
    pcity->feel[CITIZEN_ANGRY][i] = packet->ppl_angry[i];
  }
  for (i = 0; i < CITIZEN_LAST; i++) {
    pcity->size += pcity->feel[i][FEELING_FINAL];
  }
  specialist_type_iterate(sp) {
    pcity->size +=
    pcity->specialists[sp] = packet->specialists[sp];
  } specialist_type_iterate_end;

  if (pcity->size != packet->size) {
    freelog(LOG_ERROR, "handle_city_info()"
            " %d citizens not equal %d city size in \"%s\".",
            pcity->size,
            packet->size,
            city_name(pcity));
    pcity->size = packet->size;
  }

  pcity->city_options = packet->city_options;

  for (i = 0; i < NUM_TRADEROUTES; i++) {
    pcity->trade[i]=packet->trade[i];
    pcity->trade_value[i]=packet->trade_value[i];
  }

  output_type_iterate(o) {
    pcity->surplus[o] = packet->surplus[o];
    pcity->waste[o] = packet->waste[o];
    pcity->unhappy_penalty[o] = packet->unhappy_penalty[o];
    pcity->prod[o] = packet->prod[o];
    pcity->citizen_base[o] = packet->citizen_base[o];
    pcity->usage[o] = packet->usage[o];
  } output_type_iterate_end;

  pcity->food_stock=packet->food_stock;
  if (pcity->shield_stock != packet->shield_stock) {
    shield_stock_changed = TRUE;
    pcity->shield_stock = packet->shield_stock;
  }
  pcity->pollution=packet->pollution;
  pcity->illness = packet->illness;

  if (city_is_new
      || !are_universals_equal(&pcity->production, &product)) {
    need_units_dialog_update = TRUE;
  }
  if (!are_universals_equal(&pcity->production, &product)) {
    production_changed = TRUE;
  }
  pcity->production = product;

#ifdef DONE_BY_create_city_virtual
  if (city_is_new) {
    worklist_init(&pcity->worklist);

    for (i = 0; i < ARRAY_SIZE(pcity->built); i++) {
      pcity->built[i].turn = I_NEVER;
    }
  }
#endif

  worklist_copy(&pcity->worklist, &packet->worklist);

  pcity->airlift = packet->airlift;
  pcity->did_buy=packet->did_buy;
  pcity->did_sell=packet->did_sell;
  pcity->was_happy=packet->was_happy;

  pcity->turn_founded = packet->turn_founded;
  pcity->turn_last_built = packet->turn_last_built;
  
  if (packet->changed_from_kind < VUT_NONE || packet->changed_from_kind >= VUT_LAST) {
    freelog(LOG_ERROR, "handle_city_info() bad changed_from_kind %d.",
            packet->changed_from_kind);
    product.kind = VUT_NONE;
  } else {
    product = universal_by_number(packet->changed_from_kind,
                                     packet->changed_from_value);
    if (product.kind < VUT_NONE ||  product.kind >= VUT_LAST) {
      freelog(LOG_ERROR, "handle_city_info() bad changed_from_value %d.",
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
    if (have  &&  !city_is_new
     && pcity->built[improvement_index(pimprove)].turn <= I_NEVER) {
      audio_play_sound(pimprove->soundtag, pimprove->soundtag_alt);
    }
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

  pcity->client.happy = city_happy(pcity);
  pcity->client.unhappy = city_unhappy(pcity);

  popup = (city_is_new && can_client_change_view()
           && powner == client.conn.playing
           && popup_new_cities)
          || packet->diplomat_investigate;

  city_packet_common(pcity, pcenter, powner, city_is_new, popup,
                     packet->diplomat_investigate);

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

  /* Update the units dialog if necessary. */
  if (need_units_dialog_update) {
    activeunits_report_dialog_update();
  }

  /* Update the panel text (including civ population). */
  update_info_label();
  
  /* update caravan dialog */
  if ((production_changed || shield_stock_changed)
      && caravan_dialog_is_open(NULL, &caravan_city_id)
      && caravan_city_id == pcity->id) {
    caravan_dialog_update();
  }
}

/****************************************************************************
  A helper function for handling city-info and city-short-info packets.
  Naturally, both require many of the same operations to be done on the
  data.
****************************************************************************/
static void city_packet_common(struct city *pcity, struct tile *pcenter,
                               struct player *powner, bool is_new,
                               bool popup, bool investigate)
{
  if (is_new) {
    pcity->units_supported = unit_list_new();
    pcity->info_units_supported = unit_list_new();
    pcity->info_units_present = unit_list_new();

    tile_set_worked(pcenter, pcity); /* is_free_worked() */
    city_list_prepend(powner->cities, pcity);

    if (powner == client.conn.playing) {
      city_report_dialog_update();
    }

    players_iterate(pp) {
      unit_list_iterate(pp->units, punit) 
	if(punit->homecity==pcity->id)
	  unit_list_prepend(pcity->units_supported, punit);
      unit_list_iterate_end;
    } players_iterate_end;
  } else {
    if (powner == client.conn.playing) {
      city_report_dialog_update_city(pcity);
    }
  }

  if (can_client_change_view()) {
    refresh_city_mapcanvas(pcity, pcenter, FALSE, FALSE);
  }

  if (city_workers_display==pcity)  {
    city_workers_display=NULL;
  }

  if (popup
      && NULL != client.conn.playing
      && !client.conn.playing->ai_data.control
      && can_client_issue_orders()) {
    update_menus();
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
    update_menus();
  }

  if (is_new) {
    freelog(LOG_DEBUG, "(%d,%d) creating city %d, %s %s",
	    TILE_XY(pcenter),
	    pcity->id,
	    nation_rule_name(nation_of_city(pcity)),
	    city_name(pcity));
  }

  editgui_notify_object_changed(OBJTYPE_CITY, pcity->id, FALSE);
}

/****************************************************************************
  A city-short-info packet is sent to tell us about any cities we can't see
  the internals of.  Most of the time this includes any cities owned by
  someone else.
****************************************************************************/
void handle_city_short_info(struct packet_city_short_info *packet)
{
  bool city_has_changed_owner = FALSE;
  bool city_is_new = FALSE;
  bool name_changed = FALSE;
  bool update_descriptions = FALSE;
  struct city *pcity = game_find_city_by_number(packet->id);
  struct tile *pcenter = map_pos_to_tile(packet->x, packet->y);
  struct tile *ptile = NULL;
  struct player *powner = valid_player_by_number(packet->owner);

  if (NULL == powner) {
    freelog(LOG_ERROR,
            "handle_city_short_info() bad player number %d.",
            packet->owner);
    return;
  }

  if (NULL == pcenter) {
    freelog(LOG_ERROR,
            "handle_city_short_info() invalid tile (%d,%d).",
            TILE_XY(packet));
    return;
  }

  if (NULL != pcity) {
    ptile = city_tile(pcity);

    if (NULL == ptile) {
      /* invisible worked city */
      city_list_unlink(invisible_cities, pcity);
      city_is_new = TRUE;

      pcity->tile = pcenter;
      ptile = pcenter;
      pcity->owner = powner;
      pcity->original = powner;
    } else if (city_owner(pcity) != powner) {
      client_remove_city(pcity);
      pcity = NULL;
      city_has_changed_owner = TRUE;
    }
  }

  if (NULL == pcity) {
    city_is_new = TRUE;
    pcity = create_city_virtual(powner, pcenter, packet->name);
    pcity->id = packet->id;
    idex_register_city(pcity);
  } else if (pcity->id != packet->id) {
    freelog(LOG_ERROR, "handle_city_short_info()"
            " city id %d != id %d.",
            pcity->id,
            packet->id);
    return;
  } else if (city_tile(pcity) != pcenter) {
    freelog(LOG_ERROR, "handle_city_short_info()"
            " city tile (%d,%d) != (%d,%d).",
            TILE_XY(city_tile(pcity)),
            TILE_XY(packet));
    return;
  } else {
    name_changed = (0 != strncmp(packet->name, pcity->name,
                                 sizeof(pcity->name)));

    /* Check if city desciptions should be updated */
    if (draw_city_names && name_changed) {
      update_descriptions = TRUE;
    }

    sz_strlcpy(pcity->name, packet->name);
    
    memset(pcity->feel, 0, sizeof(pcity->feel));
    memset(pcity->specialists, 0, sizeof(pcity->specialists));
  }

  pcity->specialists[DEFAULT_SPECIALIST] =
  pcity->size = packet->size;

  /* HACK: special case for trade routes */
  pcity->citizen_base[O_TRADE] = packet->tile_trade;

  /* We can't actually see the internals of the city, but the server tells
   * us this much. */
  pcity->client.occupied = packet->occupied;
  pcity->client.walls = packet->walls;

  pcity->client.happy = packet->happy;
  pcity->client.unhappy = packet->unhappy;

#ifdef DONE_BY_create_city_virtual
  if (city_is_new) {
    int i;

    for (i = 0; i < ARRAY_SIZE(pcity->built); i++) {
      pcity->built[i].turn = I_NEVER;
    }
  }
#endif

  improvement_iterate(pimprove) {
    bool have = BV_ISSET(packet->improvements, improvement_index(pimprove));
    update_improvement_from_packet(pcity, pimprove, have);
  } improvement_iterate_end;

#ifdef DONE_BY_create_city_virtual
  /* This sets dumb values for everything else. This is not really required,
     but just want to be at the safe side. */
  {
    int i;
    int x, y;

    for (i = 0; i < NUM_TRADEROUTES; i++) {
      pcity->trade[i] = 0;
      pcity->trade_value[i] = 0;
    }
    memset(pcity->surplus, 0, O_LAST * sizeof(*pcity->surplus));
    memset(pcity->waste, 0, O_LAST * sizeof(*pcity->waste));
    memset(pcity->prod, 0, O_LAST * sizeof(*pcity->prod));
    pcity->food_stock         = 0;
    pcity->shield_stock       = 0;
    pcity->pollution          = 0;
    pcity->illness            = 0;
    BV_CLR_ALL(pcity->city_options);
    pcity->production.kind    = VUT_NONE;
    pcity->production.value.building = NULL;
    worklist_init(&pcity->worklist);
    pcity->airlift            = FALSE;
    pcity->did_buy            = FALSE;
    pcity->did_sell           = FALSE;
    pcity->was_happy          = FALSE;
  } /* Dumb values */
#endif

  city_packet_common(pcity, pcenter, powner, city_is_new, FALSE, FALSE);

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

/**************************************************************************
...
**************************************************************************/
void handle_new_year(int year, int turn)
{
  game.info.year = year;
  /*
   * The turn was increased in handle_before_new_year()
   */
  assert(game.info.turn == turn);
  update_info_label();

  update_unit_focus();
  auto_center_on_focus_unit();

  update_unit_info_label(get_units_in_focus());
  update_menus();

  set_seconds_to_turndone(game.info.timeout);

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

  if (sound_bell_at_new_turn) {
    create_event(NULL, E_TURN_BELL, FTC_CLIENT_INFO, NULL,
                 _("Start of turn %d"), game.info.turn);
  }

  agents_new_turn();
}

/**************************************************************************
  Called by the network code when an end-phase packet is received.  This
  signifies the end of our phase (it's not sent for other player's
  phases).
**************************************************************************/
void handle_end_phase(void)
{
  clear_notify_window();
  /*
   * The local idea of the game.info.turn is increased here since the
   * client will get unit updates (reset of move points for example)
   * between handle_before_new_year() and handle_new_year(). These
   * unit updates will look like they did take place in the old turn
   * which is incorrect. If we get the authoritative information about
   * the game.info.turn in handle_new_year() we will check it.
   */
  game.info.turn++;
  agents_before_new_turn();
}

/**************************************************************************
  Called by the network code when an start-phase packet is received.  This
  may be the start of our phase or someone else's phase.
**************************************************************************/
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
    freelog(LOG_ERROR,
            "handle_start_phase() illegal phase %d.",
            phase);
    return;
  }

  set_client_state(C_S_RUNNING);

  game.info.phase = phase;

  if (NULL != client.conn.playing
      && is_player_phase(client.conn.playing, phase)) {
    agents_start_turn();
    non_ai_unit_focus = FALSE;

    turn_done_sent = FALSE;
    update_turn_done_button_state();

    if (client.conn.playing->ai_data.control && !ai_manual_turn_done) {
      user_ended_turn();
    }

    player_set_unit_focus_status(client.conn.playing);

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

/**************************************************************************
  Called when begin-turn packet is received. Server has finished processing
  turn change.
**************************************************************************/
void handle_begin_turn(void)
{
  freelog(LOG_DEBUG, "handle_begin_turn()");

  /* Possibly replace wait cursor with something else */
  set_server_busy(FALSE);
}

/**************************************************************************
  Called when end-turn packet is received. Server starts processing turn
  change.
**************************************************************************/
void handle_end_turn(void)
{
  freelog(LOG_DEBUG, "handle_end_turn()");

  /* Make sure wait cursor is in use */
  set_server_busy(TRUE);
}

/**************************************************************************
...
**************************************************************************/
void play_sound_for_event(enum event_type type)
{
  const char *sound_tag = get_event_sound_tag(type);

  if (sound_tag) {
    audio_play_sound(sound_tag, NULL);
  }
}  
  
/**************************************************************************
  Handle a message packet.  This includes all messages - both
  in-game messages and chats from other players.
**************************************************************************/
void handle_chat_msg(char *message, int x, int y,
		     enum event_type event, int conn_id)
{
  struct tile *ptile = NULL;

  if (is_normal_map_pos(x, y)) {
    ptile = map_pos_to_tile(x, y);
  }

  handle_event(message, ptile, event, conn_id);
}

/**************************************************************************
  Handle a connect message packet. Server sends connect message to
  client immediately when client connects.
**************************************************************************/
void handle_connect_msg(char *message)
{
  popup_connect_msg(_("Welcome"), message);
}

/**************************************************************************
...
**************************************************************************/
void handle_page_msg(char *message, enum event_type event)
{
  char *caption;
  char *headline;
  char *lines;

  caption = message;
  headline = strchr (caption, '\n');
  if (headline) {
    *(headline++) = '\0';
    lines = strchr (headline, '\n');
    if (lines) {
      *(lines++) = '\0';
    } else {
      lines = "";
    }
  } else {
    headline = "";
    lines = "";
  }

  if (NULL == client.conn.playing
      || !client.conn.playing->ai_data.control
      || event != E_BROADCAST_REPORT) {
    popup_notify_dialog(caption, headline, lines);
    play_sound_for_event(event);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_info(struct packet_unit_info *packet)
{
  struct unit *punit;

  punit = unpackage_unit(packet);
  if (handle_unit_packet_common(punit)) {
    free(punit);
  }
}

/**************************************************************************
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

  Exception: When the client puts a unit in focus, it's status is set to
  idle immediately, before informing the server about the new status. This
  is because the server can never deny a request for idle, and should not
  be concerned about which unit the client is focusing on.
**************************************************************************/
static bool handle_unit_packet_common(struct unit *packet_unit)
{
  struct city *pcity;
  struct unit *punit;
  bool need_update_menus = FALSE;
  bool repaint_unit = FALSE;
  bool repaint_city = FALSE;	/* regards unit's homecity */
  struct tile *old_tile = NULL;
  bool check_focus = FALSE;     /* conservative focus change */
  bool moved = FALSE;
  bool ret = FALSE;

  punit = player_find_unit_by_id(unit_owner(packet_unit), packet_unit->id);
  if (!punit && game_find_unit_by_number(packet_unit->id)) {
    /* This means unit has changed owner. We deal with this here
     * by simply deleting the old one and creating a new one. */
    handle_unit_remove(packet_unit->id);
  }

  if (punit) {
    ret = TRUE;
    punit->activity_count = packet_unit->activity_count;
    unit_change_battlegroup(punit, packet_unit->battlegroup);
    if (punit->ai.control != packet_unit->ai.control) {
      punit->ai.control = packet_unit->ai.control;
      repaint_unit = TRUE;
      /* AI is set:     may change focus */
      /* AI is cleared: keep focus */
      if (packet_unit->ai.control && unit_is_in_focus(punit)) {
        check_focus = TRUE;
      }
    }

    if (punit->activity != packet_unit->activity
	|| punit->activity_target != packet_unit->activity_target
        || punit->activity_base != packet_unit->activity_base
	|| punit->transported_by != packet_unit->transported_by
	|| punit->occupy != packet_unit->occupy
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
      if (wakeup_focus 
          && NULL != client.conn.playing
          && !client.conn.playing->ai_data.control
          && unit_owner(punit) == client.conn.playing
          && punit->activity == ACTIVITY_SENTRY
          && packet_unit->activity == ACTIVITY_IDLE
          && !unit_is_in_focus(punit)
          && is_player_phase(client.conn.playing, game.info.phase)) {
        /* many wakeup units per tile are handled */
        urgent_unit_focus(punit);
        check_focus = FALSE; /* and keep it */
      }

      punit->activity = packet_unit->activity;
      punit->activity_target = packet_unit->activity_target;
      punit->activity_base = packet_unit->activity_base;

      punit->transported_by = packet_unit->transported_by;
      if (punit->occupy != packet_unit->occupy
	  && get_focus_unit_on_tile(packet_unit->tile)) {
	/* Special case: (un)loading a unit in a transporter on the
	 * same tile as the focus unit may (dis)allow the focus unit to be
	 * loaded.  Thus the orders->(un)load menu item needs updating. */
	need_update_menus = TRUE;
      }
      punit->occupy = packet_unit->occupy;
    
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
      need_update_menus = TRUE;
    }

    if (punit->homecity != packet_unit->homecity) {
      /* change homecity */
      struct city *pcity;
      if ((pcity=game_find_city_by_number(punit->homecity))) {
	unit_list_unlink(pcity->units_supported, punit);
	refresh_city_dialog(pcity);
      }
      
      punit->homecity = packet_unit->homecity;
      if ((pcity=game_find_city_by_number(punit->homecity))) {
	unit_list_prepend(pcity->units_supported, punit);
	repaint_city = TRUE;
      }
    }

    if (punit->hp != packet_unit->hp) {
      /* hp changed */
      punit->hp = packet_unit->hp;
      repaint_unit = TRUE;
    }

    if (punit->utype != unit_type(packet_unit)) {
      /* Unit type has changed (been upgraded) */
      struct city *pcity = tile_city(punit->tile);
      
      punit->utype = unit_type(packet_unit);
      repaint_unit = TRUE;
      repaint_city = TRUE;
      if (pcity && (pcity->id != punit->homecity)) {
	refresh_city_dialog(pcity);
      }
      if (unit_is_in_focus(punit)) {
        /* Update the orders menu -- the unit might have new abilities */
	need_update_menus = TRUE;
      }
    }

    /* May change focus if an attempted move or attack exhausted unit */
    if (punit->moves_left != packet_unit->moves_left
        && unit_is_in_focus(punit)) {
      check_focus = TRUE;
    }

    if (!same_pos(punit->tile, packet_unit->tile)) { 
      /*** Change position ***/
      struct city *pcity = tile_city(punit->tile);

      old_tile = punit->tile;
      moved = TRUE;

      /* Show where the unit is going. */
      do_move_unit(punit, packet_unit);

      if(pcity)  {
	if (can_player_see_units_in_city(client.conn.playing, pcity)) {
	  /* Unit moved out of a city - update the occupied status. */
	  bool new_occupied =
	    (unit_list_size(pcity->tile->units) > 0);

	  if (pcity->client.occupied != new_occupied) {
	    pcity->client.occupied = new_occupied;
	    refresh_city_mapcanvas(pcity, pcity->tile, FALSE, FALSE);
	    update_city_description(pcity);
	  }
	}

        if(pcity->id==punit->homecity)
	  repaint_city = TRUE;
	else
	  refresh_city_dialog(pcity);
      }
      
      if((pcity=tile_city(punit->tile)))  {
	if (can_player_see_units_in_city(client.conn.playing, pcity)) {
	  /* Unit moved into a city - obviously it's occupied. */
	  if (!pcity->client.occupied) {
	    pcity->client.occupied = TRUE;
	    refresh_city_mapcanvas(pcity, pcity->tile, FALSE, FALSE);
	  }
	}

        if(pcity->id == punit->homecity)
	  repaint_city = TRUE;
	else
	  refresh_city_dialog(pcity);
	
        if((unit_has_type_flag(punit, F_TRADE_ROUTE) || unit_has_type_flag(punit, F_HELP_WONDER))
	   && NULL != client.conn.playing
	   && !client.conn.playing->ai_data.control
	   && unit_owner(punit) == client.conn.playing
	   && !unit_has_orders(punit)
	   && can_client_issue_orders()
	   && popup_caravan_arrival
	   && (unit_can_help_build_wonder_here(punit)
	       || unit_can_est_traderoute_here(punit))) {
	  process_caravan_arrival(punit);
	}
      }

    }  /*** End of Change position. ***/

    if (repaint_city || repaint_unit) {
      /* We repaint the city if the unit itself needs repainting or if
       * there is a special city-only redrawing to be done. */
      if((pcity=game_find_city_by_number(punit->homecity))) {
	refresh_city_dialog(pcity);
      }
      if (repaint_unit && tile_city(punit->tile) && tile_city(punit->tile) != pcity) {
	/* Refresh the city we're occupying too. */
	refresh_city_dialog(tile_city(punit->tile));
      }
    }

    /* unit upkeep information */
    output_type_iterate(o) {
      punit->upkeep[o] = packet_unit->upkeep[o];
    } output_type_iterate_end;

    punit->veteran = packet_unit->veteran;
    punit->moves_left = packet_unit->moves_left;
    punit->fuel = packet_unit->fuel;
    punit->goto_tile = packet_unit->goto_tile;
    punit->paradropped = packet_unit->paradropped;
    if (punit->done_moving != packet_unit->done_moving) {
      punit->done_moving = packet_unit->done_moving;
      check_focus = TRUE;
    }

    /* This won't change punit; it enqueues the call for later handling. */
    agents_unit_changed(punit);
    editgui_notify_object_changed(OBJTYPE_UNIT, punit->id, FALSE);
  } else {
    /*** Create new unit ***/
    punit = packet_unit;
    idex_register_unit(punit);

    unit_list_prepend(unit_owner(punit)->units, punit);
    unit_list_prepend(punit->tile->units, punit);

    unit_register_battlegroup(punit);

    if((pcity=game_find_city_by_number(punit->homecity))) {
      unit_list_prepend(pcity->units_supported, punit);
    }

    freelog(LOG_DEBUG, "New %s %s id %d (%d %d) hc %d %s", 
	    nation_rule_name(nation_of_unit(punit)),
	    unit_rule_name(punit),
	    TILE_XY(punit->tile),
	    punit->id,
	    punit->homecity,
	    (pcity ? city_name(pcity) : "(unknown)"));

    repaint_unit = (punit->transported_by == -1);
    agents_unit_new(punit);

    if ((pcity = tile_city(punit->tile))) {
      /* The unit is in a city - obviously it's occupied. */
      pcity->client.occupied = TRUE;
    }
  } /*** End of Create new unit ***/

  assert(punit != NULL);

  if (unit_is_in_focus(punit)
      || get_focus_unit_on_tile(punit->tile)
      || (moved && get_focus_unit_on_tile(old_tile))) {
    update_unit_info_label(get_units_in_focus());
  }

  if (repaint_unit) {
    refresh_unit_mapcanvas(punit, punit->tile, TRUE, FALSE);
  }

  if ((check_focus || get_num_units_in_focus() == 0)
      && NULL != client.conn.playing
      && !client.conn.playing->ai_data.control
      && is_player_phase(client.conn.playing, game.info.phase)) {
    update_unit_focus();
  }

  if (need_update_menus) {
    update_menus();
  }

  return ret;
}

/**************************************************************************
  Receive a short_unit info packet.
**************************************************************************/
void handle_unit_short_info(struct packet_unit_short_info *packet)
{
  struct city *pcity;
  struct unit *punit;

  if (packet->goes_out_of_sight) {
    punit = game_find_unit_by_number(packet->id);
    if (punit) {
      client_remove_unit(punit);
    }
    return;
  }

  /* Special case for a diplomat/spy investigating a city: The investigator
   * needs to know the supported and present units of a city, whether or not
   * they are fogged. So, we send a list of them all before sending the city
   * info. */
  if (packet->packet_use == UNIT_INFO_CITY_SUPPORTED
      || packet->packet_use == UNIT_INFO_CITY_PRESENT) {
    static int last_serial_num = 0;

    /* fetch city -- abort if not found */
    pcity = game_find_city_by_number(packet->info_city_id);
    if (!pcity) {
      return;
    }

    /* New serial number -- clear (free) everything */
    if (last_serial_num != packet->serial_num) {
      last_serial_num = packet->serial_num;
      unit_list_iterate(pcity->info_units_supported, psunit) {
	destroy_unit_virtual(psunit);
      } unit_list_iterate_end;
      unit_list_clear(pcity->info_units_supported);
      unit_list_iterate(pcity->info_units_present, ppunit) {
	destroy_unit_virtual(ppunit);
      } unit_list_iterate_end;
      unit_list_clear(pcity->info_units_present);
    }

    /* Okay, append a unit struct to the proper list. */
    punit = unpackage_short_unit(packet);
    if (packet->packet_use == UNIT_INFO_CITY_SUPPORTED) {
      unit_list_prepend(pcity->info_units_supported, punit);
    } else {
      assert(packet->packet_use == UNIT_INFO_CITY_PRESENT);
      unit_list_prepend(pcity->info_units_present, punit);
    }

    /* Done with special case. */
    return;
  }

  if (valid_player_by_number(packet->owner) == client.conn.playing) {
    freelog(LOG_ERROR, "handle_unit_short_info() for own unit.");
  }

  punit = unpackage_short_unit(packet);
  if (handle_unit_packet_common(punit)) {
    free(punit);
  }
}

/****************************************************************************
  Receive information about the map size and topology from the server.  We
  initialize some global variables at the same time.
****************************************************************************/
void handle_map_info(int xsize, int ysize, int topology_id)
{
  if (!map_is_empty()) {
    map_free();
  }

  map.xsize = xsize;
  map.ysize = ysize;
  map.topology_id = topology_id;

  /* Parameter is FALSE so that sizes are kept unchanged. */
  map_init_topology(FALSE);

  map_allocate();
  init_client_goto();
  mapdeco_init();

  generate_citydlg_dimensions();

  calculate_overview_dimensions();

  packhand_init();
}

/**************************************************************************
...
**************************************************************************/
void handle_game_info(struct packet_game_info *pinfo)
{
  bool boot_help;
  bool update_aifill_button = FALSE;


  if (game.info.aifill != pinfo->aifill) {
    update_aifill_button = TRUE;
  }
  
  if (game.info.is_edit_mode != pinfo->is_edit_mode) {
    popdown_all_city_dialogs();
  }

  game.info = *pinfo;

  /* check the values! */
#define VALIDATE(_count, _maximum, _string)				\
  if (game.info._count > _maximum) {					\
    freelog(LOG_ERROR, "handle_game_info():"				\
            " Too many " _string "; using %d of %d",			\
            _maximum, game.info._count);				\
    game.info._count = _maximum;					\
  }

  VALIDATE(granary_num_inis,	MAX_GRANARY_INIS,	"granary entries");
  VALIDATE(num_teams,		MAX_NUM_TEAMS,		"teams");
#undef VALIDATE

  game.government_during_revolution =
    government_by_number(game.info.government_during_revolution_id);

  boot_help = (can_client_change_view()
	       && game.info.spacerace != pinfo->spacerace);
  if (game.info.timeout != 0 && pinfo->seconds_to_phasedone >= 0) {
    set_seconds_to_turndone(pinfo->seconds_to_phasedone);
  }
  if (boot_help) {
    boot_help_texts(client.conn.playing); /* reboot, after setting game.spacerace */
  }
  update_unit_focus();
  update_menus();
  update_players_dialog();
  if (update_aifill_button) {
    update_start_page();
  }
  
  if (can_client_change_view()) {
    update_info_label();
  }

  editgui_notify_object_changed(OBJTYPE_GAME, 1, FALSE);
}

/**************************************************************************
...
**************************************************************************/
static bool read_player_info_techs(struct player *pplayer,
				   char *inventions)
{
  bool need_effect_update = FALSE;

#ifdef DEBUG
  freelog(LOG_VERBOSE, "Player%d inventions:%s",
          player_number(pplayer),
          inventions);
#endif

  advance_index_iterate(A_NONE, i) {
    enum tech_state newstate = inventions[i] - '0';
    enum tech_state oldstate = player_invention_set(pplayer, i, newstate);

    if (newstate != oldstate
	&& (newstate == TECH_KNOWN || oldstate == TECH_KNOWN)) {
      need_effect_update = TRUE;
    }
  } advance_index_iterate_end;

  if (need_effect_update) {
    update_menus();
  }

  player_research_update(pplayer);
  return need_effect_update;
}

/**************************************************************************
  Sets the target government.  This will automatically start a revolution
  if the target government differs from the current one.
**************************************************************************/
void set_government_choice(struct government *government)
{
  if (NULL != client.conn.playing
      && can_client_issue_orders()
      && government != government_of_player(client.conn.playing)) {
    dsend_packet_player_change_government(&client.conn, government_number(government));
  }
}

/**************************************************************************
  Begin a revolution by telling the server to start it.  This also clears
  the current government choice.
**************************************************************************/
void start_revolution(void)
{
  dsend_packet_player_change_government(&client.conn,
				    game.info.government_during_revolution_id);
}

/**************************************************************************
  Handle a notification that the player slot identified by 'playerno' has
  become unused. If the slot is already unused, then just ignore. Otherwise
  update the total player count and the GUI.
**************************************************************************/
void handle_player_remove(int playerno)
{
  struct player *pplayer;
  pplayer = player_slot_by_number(playerno);

  if (NULL == pplayer) {
    freelog(LOG_ERROR, "Invalid player slot number %d in "
            "handle_player_remove().", playerno);
    return;
  }

  if (!player_slot_is_used(pplayer)) {
    /* Ok, just ignore. */
    return;
  }

  game_remove_player(pplayer);
  player_init(pplayer);

  player_slot_set_used(pplayer, FALSE);
  set_player_count(player_count() - 1);

  update_players_dialog();
  update_conn_list_dialog();

  if (can_client_change_view()) {
    update_intel_dialog(pplayer);
  }

  editgui_refresh();
  editgui_notify_object_changed(OBJTYPE_PLAYER, player_number(pplayer),
                                TRUE);
}

/**************************************************************************
  Handle information about a player. If the packet refers to a player slot
  that is not currently used, then this function will set that slot to
  used and update the total player count.
**************************************************************************/
void handle_player_info(struct packet_player_info *pinfo)
{
  bool is_new_nation = FALSE;
  bool new_tech = FALSE;
  bool poptechup = FALSE;
  bool turn_done_changed = FALSE;
  int i, my_id;
  struct player_research *research;
  struct player *pplayer, *my_player;
  struct nation_type *pnation;
  struct government *pgov, *ptarget_gov;


  /* First verify packet fields. */

  pplayer = player_slot_by_number(pinfo->playerno);

  if (NULL == pplayer) {
    freelog(LOG_ERROR, "Invalid player slot number %d in "
            "handle_player_info().", pinfo->playerno);
    return;
  }

  pnation = nation_by_number(pinfo->nation);
  pgov = government_by_number(pinfo->government);
  ptarget_gov = government_by_number(pinfo->target_government);


  /* Now update the player information. */

  if (!player_slot_is_used(pplayer)) {
    player_slot_set_used(pplayer, TRUE);
    set_player_count(player_count() + 1);
  }

  sz_strlcpy(pplayer->name, pinfo->name);
  sz_strlcpy(pplayer->username, pinfo->username);

  is_new_nation = player_set_nation(pplayer, pnation);
  pplayer->is_male = pinfo->is_male;
  team_add_player(pplayer, team_by_number(pinfo->team));
  pplayer->score.game = pinfo->score;
  pplayer->was_created = pinfo->was_created;

  pplayer->economic.gold = pinfo->gold;
  pplayer->economic.tax = pinfo->tax;
  pplayer->economic.science = pinfo->science;
  pplayer->economic.luxury = pinfo->luxury;
  pplayer->government = pgov;
  pplayer->target_government = ptarget_gov;
  BV_CLR_ALL(pplayer->embassy);
  players_iterate(pother) {
    if (pinfo->embassy[player_index(pother)]) {
      BV_SET(pplayer->embassy, player_index(pother));
    }
  } players_iterate_end;
  pplayer->gives_shared_vision = pinfo->gives_shared_vision;
  pplayer->city_style = pinfo->city_style;
  for (i = 0; i < player_slot_count(); i++) {
    pplayer->ai_data.love[i] = pinfo->love[i];
  }

  my_id = client_player_number();
  my_player = client_player();

  /* Check if we detect change to armistice with us. If so,
   * ready all units for movement out of the territory in
   * question; otherwise they will be disbanded. */
  if (client_has_player()
      && DS_ARMISTICE != pplayer->diplstates[my_id].type
      && DS_ARMISTICE == pinfo->diplstates[my_id].type) {
    unit_list_iterate(my_player->units, punit) {
      if (!tile_owner(unit_tile(punit))
          || tile_owner(unit_tile(punit)) != pplayer) {
        continue;
      }
      if (punit->focus_status == FOCUS_WAIT) {
        punit->focus_status = FOCUS_AVAIL;
      }
      if (punit->activity != ACTIVITY_IDLE) {
        request_new_unit_activity(punit, ACTIVITY_IDLE);
      }
    } unit_list_iterate_end;
  }

  for (i = 0; i < player_slot_count(); i++) {
    pplayer->diplstates[i].type = pinfo->diplstates[i].type;
    pplayer->diplstates[i].turns_left = pinfo->diplstates[i].turns_left;
    pplayer->diplstates[i].contact_turns_left
      = pinfo->diplstates[i].contact_turns_left;
    pplayer->diplstates[i].has_reason_to_cancel
      = pinfo->diplstates[i].has_reason_to_cancel;
  }
  pplayer->is_connected = pinfo->is_connected;

  for (i = 0; i < B_LAST; i++) {
    pplayer->wonders[i] = pinfo->wonders[i];
  }

  /* We need to set ai.control before read_player_info_techs */
  if (pplayer->ai_data.control != pinfo->ai)  {
    pplayer->ai_data.control = pinfo->ai;
    if (pplayer == my_player)  {
      if (my_player->ai_data.control) {
        output_window_append(FTC_CLIENT_INFO, NULL,
                             _("AI mode is now ON."));
      } else {
        output_window_append(FTC_CLIENT_INFO, NULL,
                             _("AI mode is now OFF."));
      }
    }
  }

  pplayer->ai_data.science_cost = pinfo->science_cost;

  /* If the server sends out player information at the wrong time, it is
   * likely to give us inconsistent player tech information, causing a
   * sanity-check failure within this function.  Fixing this at the client
   * end is very tricky; it's hard to figure out when to read the techs
   * and when to ignore them.  The current solution is that the server should
   * only send the player info out at appropriate times - e.g., while the
   * game is running. */
  new_tech = read_player_info_techs(pplayer, pinfo->inventions);
  
  research = get_player_research(pplayer);

  poptechup = (research->researching != pinfo->researching
               || research->tech_goal != pinfo->tech_goal);
  pplayer->bulbs_last_turn = pinfo->bulbs_last_turn;
  research->bulbs_researched = pinfo->bulbs_researched;
  research->techs_researched = pinfo->techs_researched;

  /* check for bad values, complicated by discontinuous range */
  if (NULL == advance_by_number(pinfo->researching)
      && A_UNKNOWN != pinfo->researching
      && A_FUTURE != pinfo->researching
      && A_UNSET != pinfo->researching) {
    research->researching = A_NONE; /* should never happen */
  } else {
    research->researching = pinfo->researching;
  }
  research->future_tech = pinfo->future_tech;
  research->tech_goal = pinfo->tech_goal;
  
  turn_done_changed = pplayer->phase_done != pinfo->phase_done;
  pplayer->phase_done = pinfo->phase_done;

  pplayer->is_ready = pinfo->is_ready;
  pplayer->nturns_idle = pinfo->nturns_idle;
  pplayer->is_alive = pinfo->is_alive;
  pplayer->ai_data.barbarian_type = pinfo->barbarian_type;
  pplayer->revolution_finishes = pinfo->revolution_finishes;
  pplayer->ai_data.skill_level = pinfo->ai_skill_level;

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
    if (poptechup || new_tech) {
      science_dialog_update();
    }
    if (poptechup) {
      if (client_has_player() && !my_player->ai_data.control) {
        popup_science_dialog(FALSE);
      }
    }
    if (new_tech) {
      /* If we just learned bridge building and focus is on a settler
         on a river the road menu item will remain disabled unless we
         do this. (applys in other cases as well.) */
      if (get_num_units_in_focus() > 0) {
        update_menus();
      }
    }
    if (turn_done_changed) {
      update_turn_done_button_state();
    }
    economy_report_dialog_update();
    activeunits_report_dialog_update();
    city_report_dialog_update();
    update_info_label();
  }

  upgrade_canvas_clipboard();

  update_players_dialog();
  update_conn_list_dialog();

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

/**************************************************************************
  Remove, add, or update dummy connection struct representing some
  connection to the server, with info from packet_conn_info.
  Updates player and game connection lists.
  Calls update_players_dialog() in case info for that has changed.
**************************************************************************/
void handle_conn_info(struct packet_conn_info *pinfo)
{
  struct connection *pconn = find_conn_by_id(pinfo->id);
  bool preparing_client_state = FALSE;

  freelog(LOG_DEBUG, "conn_info id%d used%d est%d plr%d obs%d acc%d",
	  pinfo->id, pinfo->used, pinfo->established, pinfo->player_num,
	  pinfo->observer, (int)pinfo->access_level);
  freelog(LOG_DEBUG, "conn_info \"%s\" \"%s\" \"%s\"",
	  pinfo->username, pinfo->addr, pinfo->capability);
  
  if (!pinfo->used) {
    /* Forget the connection */
    if (!pconn) {
      freelog(LOG_VERBOSE, "Server removed unknown connection %d", pinfo->id);
      return;
    }
    client_remove_cli_conn(pconn);
    pconn = NULL;
  } else {
    struct player *pplayer = player_slot_by_number(pinfo->player_num);

    if (!pconn) {
      freelog(LOG_VERBOSE, "Server reports new connection %d %s",
              pinfo->id, pinfo->username);
      if (pinfo->id == client.conn.id) {
        /* Our connection. */
        pconn = &client.conn;
      } else {
        pconn = fc_calloc(1, sizeof(struct connection));
        pconn->buffer = NULL;
        pconn->send_buffer = NULL;
        pconn->ping_time = -1.0;
      }
      if (pplayer) {
	conn_list_append(pplayer->connections, pconn);
      }
      conn_list_append(game.all_connections, pconn);
      conn_list_append(game.est_connections, pconn);
    } else {
      freelog(LOG_PACKET, "Server reports updated connection %d %s",
	      pinfo->id, pinfo->username);
      if (pplayer != pconn->playing) {
	if (NULL != pconn->playing) {
	  conn_list_unlink(pconn->playing->connections, pconn);
	}
	if (pplayer) {
	  conn_list_append(pplayer->connections, pconn);
	}
      }
    }

    if (pconn == &client.conn
        && (pconn->playing != pplayer
            || pconn->observer != pinfo->observer)) {
      /* Our connection state changed, let prepare the changes and reset
       * the game. */
      preparing_client_state = TRUE;
    }

    pconn->id = pinfo->id;
    pconn->established = pinfo->established;
    pconn->observer = pinfo->observer;
    pconn->access_level = pinfo->access_level;
    pconn->playing = pplayer;

    sz_strlcpy(pconn->username, pinfo->username);
    sz_strlcpy(pconn->addr, pinfo->addr);
    sz_strlcpy(pconn->capability, pinfo->capability);
  }

  update_players_dialog();
  update_conn_list_dialog();

  if (pinfo->used && pconn == &client.conn) {
    /* For updating the sensitivity of the "Edit Mode" menu item,
     * among other things. */
    update_menus();
  }

  if (preparing_client_state) {
    set_client_state(C_S_PREPARING);
  }
}

/*************************************************************************
  Handles a conn_ping_info packet from the server.  This packet contains
  ping times for each connection.
**************************************************************************/
void handle_conn_ping_info(int connections, int *conn_id, float *ping_time)
{
  int i;

  for (i = 0; i < connections; i++) {
    struct connection *pconn = find_conn_by_id(conn_id[i]);

    if (!pconn) {
      continue;
    }

    pconn->ping_time = ping_time[i];
    freelog(LOG_DEBUG, "conn-id=%d, ping=%fs", pconn->id,
	    pconn->ping_time);
  }
  /* The old_ping_time data is ignored. */

  update_players_dialog();
}

/**************************************************************************
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
**************************************************************************/
static bool spaceship_autoplace(struct player *pplayer,
			       struct player_spaceship *ship)
{
  int i, num;
  enum spaceship_place_type type;
  
  if (ship->modules > (ship->habitation + ship->life_support
		       + ship->solar_panels)) {
    /* "nice" governments prefer to keep success 100%;
     * others build habitation first (for score?)  (Thanks Massimo.)
     */
    type =
      (ship->habitation==0)   ? SSHIP_PLACE_HABITATION :
      (ship->life_support==0) ? SSHIP_PLACE_LIFE_SUPPORT :
      (ship->solar_panels==0) ? SSHIP_PLACE_SOLAR_PANELS :
      ((ship->habitation < ship->life_support)
       && (ship->solar_panels*2 >= ship->habitation + ship->life_support + 1))
                              ? SSHIP_PLACE_HABITATION :
      (ship->solar_panels*2 < ship->habitation + ship->life_support)
                              ? SSHIP_PLACE_SOLAR_PANELS :
      (ship->life_support<ship->habitation)
                              ? SSHIP_PLACE_LIFE_SUPPORT :
      ((ship->life_support <= ship->habitation)
       && (ship->solar_panels*2 >= ship->habitation + ship->life_support + 1))
                              ? SSHIP_PLACE_LIFE_SUPPORT :
                                SSHIP_PLACE_SOLAR_PANELS;

    if (type == SSHIP_PLACE_HABITATION) {
      num = ship->habitation + 1;
    } else if(type == SSHIP_PLACE_LIFE_SUPPORT) {
      num = ship->life_support + 1;
    } else {
      num = ship->solar_panels + 1;
    }
    assert(num <= NUM_SS_MODULES / 3);

    dsend_packet_spaceship_place(&client.conn, type, num);
    return TRUE;
  }
  if (ship->components > ship->fuel + ship->propulsion) {
    if (ship->fuel <= ship->propulsion) {
      type = SSHIP_PLACE_FUEL;
      num = ship->fuel + 1;
    } else {
      type = SSHIP_PLACE_PROPULSION;
      num = ship->propulsion + 1;
    }
    dsend_packet_spaceship_place(&client.conn, type, num);
    return TRUE;
  }
  if (ship->structurals > num_spaceship_structurals_placed(ship)) {
    /* Want to choose which structurals are most important.
       Else we first want to connect one of each type of module,
       then all placed components, pairwise, then any remaining
       modules, or else finally in numerical order.
    */
    int req = -1;
    
    if (!ship->structure[0]) {
      /* if we don't have the first structural, place that! */
      type = SSHIP_PLACE_STRUCTURAL;
      num = 0;
      dsend_packet_spaceship_place(&client.conn, type, num);
      return TRUE;
    }
    
    if (ship->habitation >= 1
	&& !ship->structure[modules_info[0].required]) {
      req = modules_info[0].required;
    } else if (ship->life_support >= 1
	       && !ship->structure[modules_info[1].required]) {
      req = modules_info[1].required;
    } else if (ship->solar_panels >= 1
	       && !ship->structure[modules_info[2].required]) {
      req = modules_info[2].required;
    } else {
      int i;
      for(i=0; i<NUM_SS_COMPONENTS; i++) {
	if ((i%2==0 && ship->fuel > (i/2))
	    || (i%2==1 && ship->propulsion > (i/2))) {
	  if (!ship->structure[components_info[i].required]) {
	    req = components_info[i].required;
	    break;
	  }
	}
      }
    }
    if (req == -1) {
      for(i=0; i<NUM_SS_MODULES; i++) {
	if ((i%3==0 && ship->habitation > (i/3))
	    || (i%3==1 && ship->life_support > (i/3))
	    || (i%3==2 && ship->solar_panels > (i/3))) {
	  if (!ship->structure[modules_info[i].required]) {
	    req = modules_info[i].required;
	    break;
	  }
	}
      }
    }
    if (req == -1) {
      for(i=0; i<NUM_SS_STRUCTURALS; i++) {
	if (!ship->structure[i]) {
	  req = i;
	  break;
	}
      }
    }
    /* sanity: */
    assert(req!=-1);
    assert(!ship->structure[req]);
    
    /* Now we want to find a structural we can build which leads to req.
       This loop should bottom out, because everything leads back to s0,
       and we made sure above that we do s0 first.
     */
    while(!ship->structure[structurals_info[req].required]) {
      req = structurals_info[req].required;
    }
    type = SSHIP_PLACE_STRUCTURAL;
    num = req;
    dsend_packet_spaceship_place(&client.conn, type, num);
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
void handle_spaceship_info(struct packet_spaceship_info *p)
{
  int i;
  struct player_spaceship *ship;
  struct player *pplayer = valid_player_by_number(p->player_num);

  if (NULL == pplayer) {
    freelog(LOG_ERROR,
	    "handle_spaceship_info() bad player number %d.",
	    p->player_num);
    return;
  }
  ship = &pplayer->spaceship;
  ship->state        = p->sship_state;
  ship->structurals  = p->structurals;
  ship->components   = p->components;
  ship->modules      = p->modules;
  ship->fuel         = p->fuel;
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
  
  for(i=0; i<NUM_SS_STRUCTURALS; i++) {
    if (p->structure[i] == '0') {
      ship->structure[i] = FALSE;
    } else if (p->structure[i] == '1') {
      ship->structure[i] = TRUE;
    } else {
      freelog(LOG_ERROR,
	      "handle_spaceship_info()"
	      " invalid spaceship structure '%c' (%d).",
	      p->structure[i], p->structure[i]);
      ship->structure[i] = FALSE;
    }
  }

  if (pplayer != client.conn.playing) {
    refresh_spaceship_dialog(pplayer);
    return;
  }
  update_menus();

  if (!spaceship_autoplace(pplayer, ship)) {
    refresh_spaceship_dialog(pplayer);
  }
}

/**************************************************************************
This was once very ugly...
**************************************************************************/
void handle_tile_info(struct packet_tile_info *packet)
{
  enum known_type new_known;
  enum known_type old_known;
  bool known_changed = FALSE;
  bool tile_changed = FALSE;
  struct player *powner = valid_player_by_number(packet->owner);
  struct resource *presource = resource_by_number(packet->resource);
  struct terrain *pterrain = terrain_by_number(packet->terrain);
  struct tile *ptile = map_pos_to_tile(packet->x, packet->y);
  const struct nation_type *pnation;

  if (NULL == ptile) {
    freelog(LOG_ERROR,
            "handle_tile_info() invalid tile (%d,%d).",
            TILE_XY(packet));
    return;
  }
  old_known = client_tile_get_known(ptile);

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
        freelog(LOG_ERROR,
                "handle_tile_info() unknown terrain (%d,%d).",
                TILE_XY(packet));
      }
      break;
    };
  }

  tile_special_type_iterate(spe) {
    if (packet->special[spe]) {
      if (!tile_has_special(ptile, spe)) {
	tile_set_special(ptile, spe);
	tile_changed = TRUE;
      }
    } else {
      if (tile_has_special(ptile, spe)) {
	tile_clear_special(ptile, spe);
	tile_changed = TRUE;
      }
    }
  } tile_special_type_iterate_end;
  if (!BV_ARE_EQUAL(ptile->bases, packet->bases)) {
    ptile->bases = packet->bases;
    tile_changed = TRUE;
  }

  tile_changed = tile_changed || (tile_resource(ptile) != presource);

  /* always called after setting terrain */
  tile_set_resource(ptile, presource);

  if (tile_owner(ptile) != powner) {
    tile_set_owner(ptile, powner, NULL);
    tile_changed = TRUE;
  }

  if (NULL == tile_worked(ptile)
   || tile_worked(ptile)->id != packet->worked) {
    if (IDENTITY_NUMBER_ZERO != packet->worked) {
      struct city *pwork = game_find_city_by_number(packet->worked);

      if (NULL == pwork) {
        char named[MAX_LEN_NAME];
        struct player *placeholder = powner;

        /* new unseen city, or before city_info */
        if (NULL == placeholder) {
          /* worker outside border allowed in earlier versions,
           * use non-player as placeholder.
           */
          placeholder = player_by_number(MAX_NUM_PLAYERS);
        }
        my_snprintf(named, sizeof(named), "%06u", packet->worked);

        pwork = create_city_virtual(placeholder, NULL, named);
        pwork->id = packet->worked;
        idex_register_city(pwork);

        city_list_prepend(invisible_cities, pwork);

        freelog(LOG_DEBUG, "(%d,%d) invisible city %d, %s",
                TILE_XY(ptile),
                pwork->id,
                city_name(pwork));
      } else if (NULL == city_tile(pwork)) {
        /* old unseen city, or before city_info */
        if (NULL != powner && city_owner(pwork) != powner) {
          /* update placeholder with current owner */
          pwork->owner =
          pwork->original = powner;
        }
      }

      /* This marks tile worked by invisible city. Other
       * parts of the code have to handle invisible cities correctly
       * (ptile->worked->tile == NULL) */
      tile_set_worked(ptile, pwork);
    } else {
      tile_set_worked(ptile, NULL);
    }

    tile_changed = TRUE;
  }

  if (old_known != packet->known) {
    known_changed = TRUE;
  }

  if (NULL != client.conn.playing) {
    BV_CLR(ptile->tile_known, player_index(client.conn.playing));
    vision_layer_iterate(v) {
      BV_CLR(ptile->tile_seen[v], player_index(client.conn.playing));
    } vision_layer_iterate_end;

    switch (packet->known) {
    case TILE_KNOWN_SEEN:
      BV_SET(ptile->tile_known, player_index(client.conn.playing));
      vision_layer_iterate(v) {
	BV_SET(ptile->tile_seen[v], player_index(client.conn.playing));
      } vision_layer_iterate_end;
      break;
    case TILE_KNOWN_UNSEEN:
      BV_SET(ptile->tile_known, player_index(client.conn.playing));
      break;
    case TILE_UNKNOWN:
      break;
    default:
      freelog(LOG_ERROR,
              "handle_tile_info() invalid known (%d).",
              packet->known);
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
      ptile->spec_sprite = mystrdup(packet->spec_sprite);
      tile_changed = TRUE;
    }
  } else {
    if (ptile->spec_sprite) {
      free(ptile->spec_sprite);
      ptile->spec_sprite = NULL;
      tile_changed = TRUE;
    }
  }

  pnation = nation_by_number(packet->nation_start);
  if (map_get_startpos(ptile) != pnation) {
    map_set_startpos(ptile, pnation);
    tile_changed = TRUE;
  }

  if (TILE_KNOWN_SEEN == old_known && TILE_KNOWN_SEEN != new_known) {
    /* This is an error.  So first we log the error, then make an assertion.
     * But for NDEBUG clients we fix the error. */
    unit_list_iterate(ptile->units, punit) {
      freelog(LOG_ERROR, "%p %d %s at (%d,%d) %s",
              punit,
              punit->id,
              unit_rule_name(punit),
              TILE_XY(punit->tile),
              player_name(unit_owner(punit)));
    } unit_list_iterate_end;
    assert(unit_list_size(ptile->units) == 0);
    unit_list_clear(ptile->units);
  }

  ptile->continent = packet->continent;
  map.num_continents = MAX(ptile->continent, map.num_continents);

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
      update_menus();
    }
  }
}

/**************************************************************************
  Received packet containing info about current scenario
**************************************************************************/
void handle_scenario_info(struct packet_scenario_info *packet)
{
  game.scenario.is_scenario = packet->is_scenario;
  sz_strlcpy(game.scenario.name, packet->name);
  sz_strlcpy(game.scenario.description, packet->description);
  game.scenario.players = packet->players;

  editgui_notify_object_changed(OBJTYPE_GAME, 1, FALSE);
}

/**************************************************************************
  Take arrival of ruleset control packet to indicate that
  current allocated governments should be free'd, and new
  memory allocated for new size. The same for nations.
**************************************************************************/
void handle_ruleset_control(struct packet_ruleset_control *packet)
{
  /* The ruleset is going to load new nations. So close
   * the nation selection dialog if it is open. */
  popdown_races_dialog();

  game_ruleset_free();
  game_ruleset_init();
  game.control = *packet;

  /* check the values! */
#define VALIDATE(_count, _maximum, _string)				\
  if (game.control._count > _maximum) {					\
    freelog(LOG_ERROR, "handle_ruleset_control():"			\
            " Too many " _string "; using %d of %d",			\
            _maximum, game.control._count);				\
    game.control._count = _maximum;					\
  }

  VALIDATE(num_unit_classes,	UCL_LAST,		"unit classes");
  VALIDATE(num_unit_types,	U_LAST,			"unit types");
  VALIDATE(num_impr_types,	B_LAST,			"improvements");
  VALIDATE(num_tech_types,	A_LAST_REAL,		"advances");
  VALIDATE(num_base_types,	MAX_BASE_TYPES,		"bases");

  VALIDATE(government_count,	MAX_NUM_ITEMS,		"governments");
  VALIDATE(nation_count,	MAX_NUM_ITEMS,		"nations");
  VALIDATE(styles_count,	MAX_NUM_ITEMS,		"city styles");
  VALIDATE(terrain_count,	MAX_NUM_TERRAINS,	"terrains");
  VALIDATE(resource_count,	MAX_NUM_RESOURCES,	"resources");

  VALIDATE(num_specialist_types, SP_MAX,		"specialists");
#undef VALIDATE

  governments_alloc(game.control.government_count);
  nations_alloc(game.control.nation_count);
  city_styles_alloc(game.control.styles_count);

  /* After nation ruleset free/alloc, nation->player pointers are NULL.
   * We have to initialize player->nation too, to keep state consistent.
   * In case of /taking player, number of players has been reseted, so
   * we can't use players_iterate() here, but have to go through all
   * possible player slots instead. */ 
  player_slots_iterate(pslot) {
    pslot->nation = NULL;
  } player_slots_iterate_end;

  if (packet->prefered_tileset[0] != '\0') {
    /* There is tileset suggestion */
    if (strcmp(packet->prefered_tileset, tileset_get_name(tileset))) {
      /* It's not currently in use */
      popup_tileset_suggestion_dialog();
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_unit_class(struct packet_ruleset_unit_class *p)
{
  struct unit_class *c = uclass_by_number(p->id);

  if (!c) {
    freelog(LOG_ERROR,
            "handle_ruleset_unit_class() bad unit_class %d.",
	    p->id);
    return;
  }

  sz_strlcpy(c->name.vernacular, p->name);
  c->name.translated = NULL;	/* unittype.c uclass_name_translation */
  c->move_type   = p->move_type;
  c->min_speed   = p->min_speed;
  c->hp_loss_pct = p->hp_loss_pct;
  c->hut_behavior = p->hut_behavior;
  c->flags       = p->flags;
}


/**************************************************************************
...
**************************************************************************/
void handle_ruleset_unit(struct packet_ruleset_unit *p)
{
  int i;
  struct unit_type *u = utype_by_number(p->id);

  if(!u) {
    freelog(LOG_ERROR,
	    "handle_ruleset_unit() bad unit_type %d.",
	    p->id);
    return;
  }

  sz_strlcpy(u->name.vernacular, p->name);
  u->name.translated = NULL;	/* unittype.c utype_name_translation */
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
  u->transformed_to     = utype_by_number(p->transformed_to);
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
  u->cargo              = p->cargo;
  u->targets            = p->targets;

  for (i = 0; i < MAX_VET_LEVELS; i++) {
    sz_strlcpy(u->veteran[i].name, p->veteran_name[i]);
    u->veteran[i].power_fact = p->power_fact[i];
    u->veteran[i].move_bonus = p->move_bonus[i];
  }

  u->helptext = mystrdup(p->helptext);

  tileset_setup_unit_type(tileset, u);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_tech(struct packet_ruleset_tech *p)
{
  struct advance *a = advance_by_number(p->id);

  if(!a) {
    freelog(LOG_ERROR,
	    "handle_ruleset_tech() bad advance %d.",
	    p->id);
    return;
  }

  sz_strlcpy(a->name.vernacular, p->name);
  a->name.translated = NULL;	/* tech.c advance_name_translation */
  sz_strlcpy(a->graphic_str, p->graphic_str);
  sz_strlcpy(a->graphic_alt, p->graphic_alt);
  a->require[AR_ONE] = advance_by_number(p->req[AR_ONE]);
  a->require[AR_TWO] = advance_by_number(p->req[AR_TWO]);
  a->require[AR_ROOT] = advance_by_number(p->root_req);
  a->flags = p->flags;
  a->preset_cost = p->preset_cost;
  a->num_reqs = p->num_reqs;
  a->helptext = mystrdup(p->helptext);

  tileset_setup_tech_type(tileset, a);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_building(struct packet_ruleset_building *p)
{
  int i;
  struct impr_type *b = improvement_by_number(p->id);

  if (!b) {
    freelog(LOG_ERROR,
	    "handle_ruleset_building() bad improvement %d.",
	    p->id);
    return;
  }

  b->genus = p->genus;
  sz_strlcpy(b->name.vernacular, p->name);
  b->name.translated = NULL;	/* improvement.c improvement_name_translation */
  sz_strlcpy(b->graphic_str, p->graphic_str);
  sz_strlcpy(b->graphic_alt, p->graphic_alt);
  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&b->reqs, &p->reqs[i]);
  }
  assert(b->reqs.size == p->reqs_count);
  b->obsolete_by = advance_by_number(p->obsolete_by);
  b->replaced_by = improvement_by_number(p->replaced_by);
  b->build_cost = p->build_cost;
  b->upkeep = p->upkeep;
  b->sabotage = p->sabotage;
  b->flags = p->flags;
  b->helptext = mystrdup(p->helptext);
  sz_strlcpy(b->soundtag, p->soundtag);
  sz_strlcpy(b->soundtag_alt, p->soundtag_alt);

#ifdef DEBUG
  if(p->id == improvement_count()-1) {
    improvement_iterate(b) {
      freelog(LOG_DEBUG, "Improvement: %s...",
	      improvement_rule_name(b));
      if (A_NEVER != b->obsolete_by) {
	freelog(LOG_DEBUG, "  obsolete_by %2d \"%s\"",
		advance_number(b->obsolete_by),
		advance_rule_name(b->obsolete_by));
      }
      freelog(LOG_DEBUG, "  build_cost %3d", b->build_cost);
      freelog(LOG_DEBUG, "  upkeep      %2d", b->upkeep);
      freelog(LOG_DEBUG, "  sabotage   %3d", b->sabotage);
      freelog(LOG_DEBUG, "  helptext    %s", b->helptext);
    } improvement_iterate_end;
  }
#endif

  b->allows_units = FALSE;
  unit_type_iterate(ut) {
    if (ut->need_improvement == b) {
      b->allows_units = TRUE;
      break;
    }
  } unit_type_iterate_end;

  tileset_setup_impr_type(tileset, b);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_government(struct packet_ruleset_government *p)
{
  int j;
  struct government *gov = government_by_number(p->id);

  if (!gov) {
    freelog(LOG_ERROR,
	    "handle_ruleset_government() bad government %d.",
	    p->id);
    return;
  }

  gov->item_number = p->id;

  /*for (j = 0; j < p->reqs_count; j++) {
    requirement_vector_append(&gov->reqs, &p->reqs[j]);
  }
  assert(gov->reqs.size == p->reqs_count);*/

  gov->num_ruler_titles    = p->num_ruler_titles;
    
  sz_strlcpy(gov->name.vernacular, p->name);
  gov->name.translated = NULL;	/* government.c government_name_translation */
  sz_strlcpy(gov->graphic_str, p->graphic_str);
  sz_strlcpy(gov->graphic_alt, p->graphic_alt);

  gov->ruler_titles = fc_calloc(gov->num_ruler_titles,
				sizeof(struct ruler_title));

  gov->helptext = mystrdup(p->helptext);
  
  tileset_setup_government(tileset, gov);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_government_ruler_title
  (struct packet_ruleset_government_ruler_title *p)
{
  struct government *gov = government_by_number(p->gov);

  if(!gov) {
    freelog(LOG_ERROR,
            "handle_ruleset_government_ruler_title()"
            " bad government %d.",
            p->gov);
    return;
  }

  if(p->id < 0 || p->id >= gov->num_ruler_titles) {
    freelog(LOG_ERROR,
            "handle_ruleset_government_ruler_title()"
            " bad ruler title %d for government \"%s\".",
            p->id, gov->name.vernacular);
    return;
  }
  gov->ruler_titles[p->id].nation = nation_by_number(p->nation);
  /* government.c ruler_title_translation */
  sz_strlcpy(gov->ruler_titles[p->id].male.vernacular, p->male_title);
  gov->ruler_titles[p->id].male.translated = NULL;
  sz_strlcpy(gov->ruler_titles[p->id].female.vernacular, p->female_title);
  gov->ruler_titles[p->id].female.translated = NULL;
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_terrain(struct packet_ruleset_terrain *p)
{
  int j;
  struct terrain *pterrain = terrain_by_number(p->id);

  if (!pterrain) {
    freelog(LOG_ERROR,
	    "handle_ruleset_terrain() bad terrain %d.",
	    p->id);
    return;
  }

  pterrain->native_to = p->native_to;
  sz_strlcpy(pterrain->name.vernacular, p->name_orig);
  pterrain->name.translated = NULL;	/* terrain.c terrain_name_translation */
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
    pterrain->resources[j] = resource_by_number(p->resources[j]);
    if (!pterrain->resources[j]) {
      freelog(LOG_ERROR,
              "handle_ruleset_terrain()"
              " Mismatched resource %d for terrain \"%s\".",
              p->resources[j],
              terrain_rule_name(pterrain));
    }
  }
  pterrain->resources[p->num_resources] = NULL;

  pterrain->road_time = p->road_time;
  pterrain->road_trade_incr = p->road_trade_incr;
  pterrain->irrigation_result = terrain_by_number(p->irrigation_result);
  pterrain->irrigation_food_incr = p->irrigation_food_incr;
  pterrain->irrigation_time = p->irrigation_time;
  pterrain->mining_result = terrain_by_number(p->mining_result);
  pterrain->mining_shield_incr = p->mining_shield_incr;
  pterrain->mining_time = p->mining_time;
  pterrain->transform_result = terrain_by_number(p->transform_result);
  pterrain->transform_time = p->transform_time;
  pterrain->rail_time = p->rail_time;
  pterrain->clean_pollution_time = p->clean_pollution_time;
  pterrain->clean_fallout_time = p->clean_fallout_time;

  pterrain->flags = p->flags;

  if (pterrain->helptext != NULL) {
    free(pterrain->helptext);
  }
  pterrain->helptext = mystrdup(p->helptext);
  
  tileset_setup_tile_type(tileset, pterrain);
}

/****************************************************************************
  Handle a packet about a particular terrain resource.
****************************************************************************/
void handle_ruleset_resource(struct packet_ruleset_resource *p)
{
  struct resource *presource = resource_by_number(p->id);

  if (!presource) {
    freelog(LOG_ERROR,
	    "handle_ruleset_resource() bad resource %d.",
	    p->id);
    return;
  }

  sz_strlcpy(presource->name.vernacular, p->name_orig);
  presource->name.translated = NULL;	/* terrain.c resource_name_translation */
  sz_strlcpy(presource->graphic_str, p->graphic_str);
  sz_strlcpy(presource->graphic_alt, p->graphic_alt);

  output_type_iterate(o) {
    presource->output[o] = p->output[o];
  } output_type_iterate_end;

  tileset_setup_resource(tileset, presource);
}

/****************************************************************************
  Handle a packet about a particular base type.
****************************************************************************/
void handle_ruleset_base(struct packet_ruleset_base *p)
{
  int i;
  struct base_type *pbase = base_by_number(p->id);

  if (!pbase) {
    freelog(LOG_ERROR,
            "handle_ruleset_base() bad base %d.",
            p->id);
    return;
  }

  sz_strlcpy(pbase->name.vernacular, p->name);
  pbase->name.translated = NULL;	/* base.c base_name_translation */
  sz_strlcpy(pbase->graphic_str, p->graphic_str);
  sz_strlcpy(pbase->graphic_alt, p->graphic_alt);
  sz_strlcpy(pbase->activity_gfx, p->activity_gfx);
  pbase->buildable = p->buildable;
  pbase->pillageable = p->pillageable;

  for (i = 0; i < p->reqs_count; i++) {
    requirement_vector_append(&pbase->reqs, &p->reqs[i]);
  }
  assert(pbase->reqs.size == p->reqs_count);

  pbase->native_to = p->native_to;

  pbase->gui_type = p->gui_type;

  pbase->build_time = p->build_time;
  pbase->border_sq  = p->border_sq;

  pbase->flags = p->flags;

  tileset_setup_base(tileset, pbase);
}

/**************************************************************************
  Handle the terrain control ruleset packet sent by the server.
**************************************************************************/
void handle_ruleset_terrain_control(struct packet_ruleset_terrain_control *p)
{
  /* Since terrain_control is the same as packet_ruleset_terrain_control
   * we can just copy the data directly. */
  terrain_control = *p;
}

/**************************************************************************
  Handle the list of groups, sent by the ruleset.
**************************************************************************/
void handle_ruleset_nation_groups(struct packet_ruleset_nation_groups *packet)
{
  int i;

  nation_groups_free();
  for (i = 0; i < packet->ngroups; i++) {
    struct nation_group *group = add_new_nation_group(packet->groups[i]);

    assert(group != NULL && nation_group_index(group) == i);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_nation(struct packet_ruleset_nation *p)
{
  int i;
  struct nation_type *pl = nation_by_number(p->id);

  if (!pl) {
    freelog(LOG_ERROR,
	    "handle_ruleset_nation() bad nation %d.",
	    p->id);
    return;
  }

  sz_strlcpy(pl->adjective.vernacular, p->adjective);
  pl->adjective.translated = NULL;
  sz_strlcpy(pl->noun_plural.vernacular, p->noun_plural);
  pl->noun_plural.translated = NULL;
  sz_strlcpy(pl->flag_graphic_str, p->graphic_str);
  sz_strlcpy(pl->flag_graphic_alt, p->graphic_alt);
  pl->leader_count = p->leader_count;
  pl->leaders = fc_malloc(sizeof(*pl->leaders) * pl->leader_count);
  for (i = 0; i < pl->leader_count; i++) {
    pl->leaders[i].name = mystrdup(p->leader_name[i]);
    pl->leaders[i].is_male = p->leader_sex[i];
  }
  pl->city_style = p->city_style;

  pl->is_playable = p->is_playable;
  pl->barb_type = p->barbarian_type;

  //memcpy(pl->init_techs, p->init_techs, sizeof(pl->init_techs)); FIXME: removed for webclient - Andreas.
  memcpy(pl->init_buildings, p->init_buildings, 
         sizeof(pl->init_buildings));
  memcpy(pl->init_units, p->init_units, 
         sizeof(pl->init_units));
  pl->init_government = government_by_number(p->init_government);

  if (p->legend[0] != '\0') {
    pl->legend = mystrdup(_(p->legend));
  } else {
    pl->legend = mystrdup("");
  }

  pl->num_groups = p->ngroups;
  pl->groups = fc_calloc(sizeof(*(pl->groups)), pl->num_groups);
  for (i = 0; i < p->ngroups; i++) {
    pl->groups[i] = nation_group_by_number(p->groups[i]);
    if (!pl->groups[i]) {
      freelog(LOG_ERROR,
              "handle_ruleset_nation() \"%s\" have unknown group %d.",
              nation_rule_name(pl),
              p->groups[i]);
      pl->groups[i] = nation_group_by_number(0); /* default group */
    }
  }

  pl->is_available = p->is_available;

  tileset_setup_nation_flag(tileset, pl);
}

/**************************************************************************
  Handle city style packet.
**************************************************************************/
void handle_ruleset_city(struct packet_ruleset_city *packet)
{
  int id, j;
  struct citystyle *cs;

  id = packet->style_id;
  if (id < 0 || id >= game.control.styles_count) {
    freelog(LOG_ERROR,
	    "handle_ruleset_city() bad citystyle %d.",
	    id);
    return;
  }
  cs = &city_styles[id];

  for (j = 0; j < packet->reqs_count; j++) {
    requirement_vector_append(&cs->reqs, &packet->reqs[j]);
  }
  assert(cs->reqs.size == packet->reqs_count);
  cs->replaced_by = packet->replaced_by;

  sz_strlcpy(cs->name.vernacular, packet->name);
  cs->name.translated = NULL;
  sz_strlcpy(cs->graphic, packet->graphic);
  sz_strlcpy(cs->graphic_alt, packet->graphic_alt);
  sz_strlcpy(cs->oceanic_graphic, packet->oceanic_graphic);
  sz_strlcpy(cs->oceanic_graphic_alt, packet->oceanic_graphic_alt);
  sz_strlcpy(cs->citizens_graphic, packet->citizens_graphic);
  sz_strlcpy(cs->citizens_graphic_alt, packet->citizens_graphic_alt);

  tileset_setup_city_tiles(tileset, id);
}

/**************************************************************************
...
**************************************************************************/
void handle_ruleset_game(struct packet_ruleset_game *packet)
{
  int i;

  /* Must set num_specialist_types before iterating over them. */
  DEFAULT_SPECIALIST = packet->default_specialist;

  for (i = 0; i < MAX_VET_LEVELS; i++) {
    game.work_veteran_chance[i] = packet->work_veteran_chance[i];
    game.veteran_chance[i] = packet->work_veteran_chance[i];
  }
}

/**************************************************************************
   Handle info about a single specialist.
**************************************************************************/
void handle_ruleset_specialist(struct packet_ruleset_specialist *p)
{
  int j;
  struct specialist *s = specialist_by_number(p->id);

  if (!s) {
    freelog(LOG_ERROR,
	    "handle_ruleset_specialist() bad specialist %d.",
	    p->id);
    return;
  }

  sz_strlcpy(s->name.vernacular, p->name);
  s->name.translated = NULL;
  sz_strlcpy(s->abbreviation.vernacular, p->short_name);
  s->abbreviation.translated = NULL;

  for (j = 0; j < p->reqs_count; j++) {
    requirement_vector_append(&s->reqs, &p->reqs[j]);
  }
  assert(s->reqs.size == p->reqs_count);

  tileset_setup_specialist_type(tileset, p->id);
}

/**************************************************************************
...
**************************************************************************/
void handle_city_name_suggestion_info(int unit_id, char *name)
{
  struct unit *punit = player_find_unit_by_id(client.conn.playing, unit_id);

  if (!can_client_issue_orders()) {
    return;
  }

  if (punit) {
    if (ask_city_name) {
      popup_newcity_dialog(punit, name);
    } else {
      dsend_packet_unit_build_city(&client.conn, unit_id,name);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_diplomat_answer(int diplomat_id, int target_id, int cost,
				 enum diplomat_actions action_type)
{
  struct city *pcity = game_find_city_by_number(target_id);
  struct unit *punit = game_find_unit_by_number(target_id);
  struct unit *pdiplomat = player_find_unit_by_id(client.conn.playing, diplomat_id);

  if (NULL == pdiplomat) {
    freelog(LOG_ERROR,
	    "handle_unit_diplomat_answer() bad diplomat %d.",
	    diplomat_id);
    return;
  }

  switch (action_type) {
  case DIPLOMAT_BRIBE:
    if (punit) {
      if (NULL != client.conn.playing
          && !client.conn.playing->ai_data.control) {
        popup_bribe_dialog(punit, cost);
      }
    }
    break;
  case DIPLOMAT_INCITE:
    if (pcity) {
      if (NULL != client.conn.playing
          && !client.conn.playing->ai_data.control) {
        popup_incite_dialog(pcity, cost);
      }
    }
    break;
  case DIPLOMAT_MOVE:
    if (can_client_issue_orders()) {
      process_diplomat_arrival(pdiplomat, target_id);
    }
    break;
  default:
    freelog(LOG_ERROR,
	    "handle_unit_diplomat_answer()"
	    " invalid action_type (%d).",
	    action_type);
    break;
  };
}

/**************************************************************************
...
**************************************************************************/
void handle_city_sabotage_list(int diplomat_id, int city_id,
			       bv_imprs improvements)
{
  struct city *pcity = game_find_city_by_number(city_id);
  struct unit *pdiplomat = player_find_unit_by_id(client.conn.playing, diplomat_id);

  if (NULL == pdiplomat) {
    freelog(LOG_ERROR,
	    "handle_city_sabotage_list() bad diplomat %d.",
	    diplomat_id);
    return;
  }

  if (NULL == pcity) {
    freelog(LOG_ERROR,
	    "handle_city_sabotage_list() bad city %d.",
	    city_id);
    return;
  }

  if (can_client_issue_orders()) {
    improvement_iterate(pimprove) {
      if (!BV_ISSET(improvements, improvement_index(pimprove))) {
        update_improvement_from_packet(pcity, pimprove, FALSE);
      }
    } improvement_iterate_end;

    popup_sabotage_dialog(pcity);
  }
}

/**************************************************************************
 Pass the packet on to be displayed in a gui-specific endgame dialog. 
**************************************************************************/
void handle_endgame_report(struct packet_endgame_report *packet)
{
  set_client_state(C_S_OVER);

  popup_endgame_report_dialog(packet);
}

/**************************************************************************
...
**************************************************************************/
void handle_player_attribute_chunk(struct packet_player_attribute_chunk *packet)
{
  if (NULL == client.conn.playing) {
    return;
  }

  generic_handle_player_attribute_chunk(client.conn.playing, packet);

  if (packet->offset + packet->chunk_length == packet->total_length) {
    /* We successful received the last chunk. The attribute block is
       now complete. */
      attribute_restore();
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_processing_started(void)
{
  agents_processing_started();

  assert(client.conn.client.request_id_of_currently_handled_packet == 0);
  client.conn.client.request_id_of_currently_handled_packet =
      get_next_request_id(client.conn.
			  client.last_processed_request_id_seen);

  freelog(LOG_DEBUG, "start processing packet %d",
	  client.conn.client.request_id_of_currently_handled_packet);
}

/**************************************************************************
...
**************************************************************************/
void handle_processing_finished(void)
{
  int i;

  freelog(LOG_DEBUG, "finish processing packet %d",
	  client.conn.client.request_id_of_currently_handled_packet);

  assert(client.conn.client.request_id_of_currently_handled_packet != 0);

  client.conn.client.last_processed_request_id_seen =
      client.conn.client.request_id_of_currently_handled_packet;

  client.conn.client.request_id_of_currently_handled_packet = 0;

  for (i = 0; i < reports_thaw_requests_size; i++) {
    if (reports_thaw_requests[i] != 0 &&
	reports_thaw_requests[i] ==
	client.conn.client.last_processed_request_id_seen) {
      reports_thaw();
      reports_thaw_requests[i] = 0;
    }
  }

  agents_processing_finished();
}

/**************************************************************************
...
**************************************************************************/
void notify_about_incoming_packet(struct connection *pc,
				   int packet_type, int size)
{
  assert(pc == &client.conn);
  freelog(LOG_DEBUG, "incoming packet={type=%d, size=%d}", packet_type,
	  size);
}

/**************************************************************************
...
**************************************************************************/
void notify_about_outgoing_packet(struct connection *pc,
				  int packet_type, int size,
				  int request_id)
{
  assert(pc == &client.conn);
  freelog(LOG_DEBUG, "outgoing packet={type=%d, size=%d, request_id=%d}",
	  packet_type, size, request_id);

  assert(request_id);
}

/**************************************************************************
  ...
**************************************************************************/
void set_reports_thaw_request(int request_id)
{
  int i;

  for (i = 0; i < reports_thaw_requests_size; i++) {
    if (reports_thaw_requests[i] == 0) {
      reports_thaw_requests[i] = request_id;
      return;
    }
  }

  reports_thaw_requests_size++;
  reports_thaw_requests =
      fc_realloc(reports_thaw_requests,
		 reports_thaw_requests_size * sizeof(int));
  reports_thaw_requests[reports_thaw_requests_size - 1] = request_id;
}

/**************************************************************************
  We have received PACKET_FREEZE_HINT. It is packet internal to network code
**************************************************************************/
void handle_freeze_hint(void)
{
  freelog(LOG_DEBUG, "handle_freeze_hint");
}

/**************************************************************************
  We have received PACKET_FREEZE_CLIENT.
**************************************************************************/
void handle_freeze_client(void)
{
  freelog(LOG_DEBUG, "handle_freeze_client");

  reports_freeze();

  agents_freeze_hint();
}

/**************************************************************************
  We have received PACKET_THAW_HINT. It is packet internal to network code
**************************************************************************/
void handle_thaw_hint(void)
{
  freelog(LOG_DEBUG, "handle_thaw_hint");
}

/**************************************************************************
  We have received PACKET_THAW_CLIENT
**************************************************************************/
void handle_thaw_client(void)
{
  freelog(LOG_DEBUG, "handle_thaw_client");

  reports_thaw();

  agents_thaw_hint();
  update_turn_done_button_state();
}

/**************************************************************************
...
**************************************************************************/
void handle_conn_ping(void)
{
  send_packet_conn_pong(&client.conn);
}

/**************************************************************************
...
**************************************************************************/
void handle_server_shutdown(void)
{
  freelog(LOG_VERBOSE, "server shutdown");
}

/**************************************************************************
  Add effect data to ruleset cache.  
**************************************************************************/
void handle_ruleset_effect(struct packet_ruleset_effect *packet)
{
  recv_ruleset_effect(packet);
}

/**************************************************************************
  Add effect requirement data to ruleset cache.  
**************************************************************************/
void handle_ruleset_effect_req(struct packet_ruleset_effect_req *packet)
{
  recv_ruleset_effect_req(packet);
}

/**************************************************************************
  Handle a notification from the server that an object was successfully
  created. The 'tag' was previously sent to the server when the client
  requested the creation. The 'id' is the identifier of the newly created
  object.
**************************************************************************/
void handle_edit_object_created(int tag, int id)
{
  editgui_notify_object_created(tag, id);
}

/**************************************************************************
  A vote no longer exists. Remove from queue and update gui.
**************************************************************************/
void handle_vote_remove(int vote_no)
{
  voteinfo_queue_delayed_remove(vote_no);
  voteinfo_gui_update();
}

/**************************************************************************
  Find and update the corresponding vote and refresh the GUI.
**************************************************************************/
void handle_vote_update(int vote_no, int yes, int no, int abstain,
                        int num_voters)
{
  struct voteinfo *vi;

  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    freelog(LOG_ERROR, "Got packet_vote_update for non-existant vote %d!",
            vote_no);
    return;
  }

  vi->yes = yes;
  vi->no = no;
  vi->abstain = abstain;
  vi->num_voters = num_voters;

  voteinfo_gui_update();
}

/**************************************************************************
  Create a new vote and add it to the queue. Refresh the GUI.
**************************************************************************/
void handle_vote_new(struct packet_vote_new *packet)
{
  if (voteinfo_queue_find(packet->vote_no)) {
    freelog(LOG_ERROR, "Got a packet_vote_new for already existing "
            "vote %d!", packet->vote_no);
    return;
  }

  voteinfo_queue_add(packet->vote_no,
                     packet->user,
                     packet->desc,
                     packet->percent_required,
                     packet->flags);
  voteinfo_gui_update();
}

/**************************************************************************
  Update the vote's status and refresh the GUI.
**************************************************************************/
void handle_vote_resolve(int vote_no, bool passed)
{
  struct voteinfo *vi;

  vi = voteinfo_queue_find(vote_no);
  if (vi == NULL) {
    freelog(LOG_ERROR, "Got packet_vote_resolve for non-existant "
            "vote %d!", vote_no);
    return;
  }

  vi->resolved = TRUE;
  vi->passed = passed;

  voteinfo_gui_update();
}
