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
#ifndef FC__CONTROL_H
#define FC__CONTROL_H

#include "packets.h"
#include "unitlist.h"

enum cursor_hover_state {
  HOVER_NONE = 0,
  HOVER_GOTO,
  HOVER_NUKE,
  HOVER_PARADROP,
  HOVER_CONNECT,
  HOVER_PATROL
};

/* Selecting unit from a stack without popup. */
enum quickselect_type {
  SELECT_POPUP = 0, SELECT_SEA, SELECT_LAND, SELECT_APPEND
};

void control_init(void);
void control_done(void);
void control_unit_killed(struct unit *punit);

void unit_change_battlegroup(struct unit *punit, int battlegroup);
void unit_register_battlegroup(struct unit *punit);

extern enum cursor_hover_state hover_state;
extern enum unit_activity connect_activity;
extern enum unit_orders goto_last_order;
extern bool non_ai_unit_focus;

bool can_unit_do_connect(struct unit *punit, enum unit_activity activity);

void do_move_unit(struct unit *punit, struct unit *target_unit);
void do_unit_goto(struct tile *ptile);
void do_unit_nuke(struct unit *punit);
void do_unit_paradrop_to(struct unit *punit, struct tile *ptile);
void do_unit_patrol_to(struct tile *ptile);
void do_unit_connect(struct tile *ptile,
		     enum unit_activity activity);
void do_map_click(struct tile *ptile, enum quickselect_type qtype);
void control_mouse_cursor(struct tile *ptile);

void set_hover_state(struct unit_list *punits, enum cursor_hover_state state,
		     enum unit_activity connect_activity,
		     enum unit_orders goto_last_order);
void request_center_focus_unit(void);
void request_move_unit_direction(struct unit *punit, int dir);
void request_new_unit_activity(struct unit *punit, enum unit_activity act);
void request_new_unit_activity_targeted(struct unit *punit,
					enum unit_activity act,
					enum tile_special_type tgt,
                                        Base_type_id base);
void request_new_unit_activity_base(struct unit *punit,
				    const struct base_type *pbase);
void request_unit_load(struct unit *pcargo, struct unit *ptransporter);
void request_unit_unload(struct unit *pcargo);
void request_unit_autosettlers(const struct unit *punit);
void request_unit_build_city(struct unit *punit);
void request_unit_caravan_action(struct unit *punit, enum packet_type action);
void request_unit_change_homecity(struct unit *punit);
void request_unit_connect(enum unit_activity activity);
void request_unit_disband(struct unit *punit);
void request_unit_fortify(struct unit *punit);
void request_unit_goto(enum unit_orders last_order);
void request_unit_move_done(void);
void request_unit_nuke(struct unit_list *punits);
void request_unit_paradrop(struct unit_list *punits);
void request_unit_patrol(void);
void request_unit_pillage(struct unit *punit);
void request_unit_sentry(struct unit *punit);
struct unit *request_unit_unload_all(struct unit *punit);
void request_unit_airlift(struct unit *punit, struct city *pcity);
void request_unit_return(struct unit *punit);
void request_unit_upgrade(struct unit *punit);
void request_unit_transform(struct unit *punit);
void request_units_wait(struct unit_list *punits);
void request_unit_wakeup(struct unit *punit);

enum unit_select_type_mode {
  SELTYPE_SINGLE = 0,
  SELTYPE_SAME,
  SELTYPE_ALL,
  NUM_SELTYPES
};

enum unit_select_location_mode {
  SELLOC_TILE = 0,
  SELLOC_CONT, /* Continent. */
  SELLOC_ALL,
  NUM_SELLOCS
};

void request_unit_select(struct unit_list *punits,
                         enum unit_select_type_mode seltype,
                         enum unit_select_location_mode selloc);

void request_diplomat_action(enum diplomat_actions action, int dipl_id,
			     int target_id, int value);
void request_diplomat_answer(enum diplomat_actions action, int dipl_id,
			     int target_id, int value);
void request_toggle_city_outlines(void);
void request_toggle_city_output(void);
void request_toggle_map_grid(void);
void request_toggle_map_borders(void);
void request_toggle_city_names(void);
void request_toggle_city_growth(void);
void request_toggle_city_productions(void);
void request_toggle_city_buycost(void);
void request_toggle_city_traderoutes(void);
void request_toggle_terrain(void);
void request_toggle_coastline(void);
void request_toggle_roads_rails(void);
void request_toggle_irrigation(void);
void request_toggle_mines(void);
void request_toggle_fortress_airbase(void);
void request_toggle_specials(void);
void request_toggle_pollution(void);
void request_toggle_cities(void);
void request_toggle_units(void);
void request_toggle_focus_unit(void);
void request_toggle_fog_of_war(void);

void wakeup_sentried_units(struct tile *ptile);
void clear_unit_orders(struct unit *punit);

bool unit_is_in_focus(const struct unit *punit);
struct unit *get_focus_unit_on_tile(const struct tile *ptile);
struct unit *head_of_units_in_focus(void);
struct unit_list *get_units_in_focus(void);
int get_num_units_in_focus(void);

void set_unit_focus(struct unit *punit);
void set_unit_focus_and_select(struct unit *punit);
void add_unit_focus(struct unit *punit);
void urgent_unit_focus(struct unit *punit);

void advance_unit_focus(void);
void auto_center_on_focus_unit(void);
void update_unit_focus(void);
void update_unit_pix_label(struct unit_list *punitlist);

struct unit *find_visible_unit(struct tile *ptile);
void set_units_in_combat(struct unit *pattacker, struct unit *pdefender);
double blink_active_unit(void);
double blink_turn_done_button(void);

void process_caravan_arrival(struct unit *punit);
void process_diplomat_arrival(struct unit *pdiplomat, int victim_id);

void key_cancel_action(void);
void key_center_capital(void);
void key_city_names_toggle(void);
void key_city_growth_toggle(void);
void key_city_productions_toggle(void);
void key_city_buycost_toggle(void);
void key_city_traderoutes_toggle(void);
void key_terrain_toggle(void);
void key_coastline_toggle(void);
void key_roads_rails_toggle(void);
void key_irrigation_toggle(void);
void key_mines_toggle(void);
void key_fortress_airbase_toggle(void);
void key_specials_toggle(void);
void key_pollution_toggle(void);
void key_cities_toggle(void);
void key_units_toggle(void);
void key_focus_unit_toggle(void);
void key_fog_of_war_toggle(void);
void key_end_turn(void);
void key_city_outlines_toggle(void);
void key_city_output_toggle(void);
void key_map_grid_toggle(void);
void key_map_borders_toggle(void);
void key_recall_previous_focus_unit(void);
void key_unit_move(enum direction8 gui_dir);
void key_unit_airbase(void);
void key_unit_auto_explore(void);
void key_unit_auto_settle(void);
void key_unit_build_city(void);
void key_unit_build_wonder(void);
void key_unit_connect(enum unit_activity activity);
void key_unit_diplomat_actions(void);
void key_unit_disband(void);
void key_unit_transform_unit(void);
void key_unit_done(void);
void key_unit_fallout(void);
void key_unit_fortify(void);
void key_unit_fortress(void);
void key_unit_goto(void);
void key_unit_homecity(void);
void key_unit_irrigate(void);
void key_unit_mine(void);
void key_unit_nuke(void);
void key_unit_patrol(void);
void key_unit_paradrop(void);
void key_unit_pillage(void);
void key_unit_pollution(void);
void key_unit_road(void);
void key_unit_sentry(void);
void key_unit_traderoute(void);
void key_unit_transform(void);
void key_unit_unload_all(void);
void key_unit_wait(void);
void key_unit_wakeup_others(void);
void key_unit_assign_battlegroup(int battlegroup, bool append);
void key_unit_select_battlegroup(int battlegroup, bool append);

void key_editor_toggle(void);
void key_editor_recalculate_borders(void);
void key_editor_toggle_fogofwar(void);

/* don't change this unless you also put more entries in data/Freeciv */
#define MAX_NUM_UNITS_BELOW 4

extern int num_units_below;


#endif  /* FC__CONTROL_H */
