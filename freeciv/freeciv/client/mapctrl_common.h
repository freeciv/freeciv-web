/***********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__MAPCTRL_COMMON_H
#define FC__MAPCTRL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "map.h"                /* enum direction8 */

/* client */
#include "control.h"            /* quickselect_type */

extern bool rbutton_down;
extern bool rectangle_active;
extern bool tiles_hilited_cities;

extern bool keyboardless_goto_button_down;
extern bool keyboardless_goto_active;
extern struct tile *keyboardless_goto_start_tile;

void anchor_selection_rectangle(int canvas_x, int canvas_y);
void update_selection_rectangle(float canvas_x, float canvas_y);
void redraw_selection_rectangle(void);
void cancel_selection_rectangle(void);

bool is_city_hilited(struct city *pcity);

void cancel_tile_hiliting(void);
void toggle_tile_hilite(struct tile *ptile);

void key_city_overlay(int canvas_x, int canvas_y);

bool clipboard_copy_production(struct tile *ptile);
void clipboard_paste_production(struct city *pcity);
void upgrade_canvas_clipboard(void);

void release_right_button(int canvas_x, int canvas_y, bool shift);

void release_goto_button(int canvas_x, int canvas_y);
void maybe_activate_keyboardless_goto(int canvas_x, int canvas_y);

bool get_turn_done_button_state(void);
bool can_end_turn(void);
void scroll_mapview(enum direction8 gui_dir);
void action_button_pressed(int canvas_x, int canvas_y,
                enum quickselect_type qtype);
void wakeup_button_pressed(int canvas_x, int canvas_y);
void adjust_workers_button_pressed(int canvas_x, int canvas_y);
void recenter_button_pressed(int canvas_x, int canvas_y);
void update_turn_done_button_state(void);
void update_line(int canvas_x, int canvas_y);
void overview_update_line(int overview_x, int overview_y);

void fill_tile_unit_list(const struct tile *ptile, struct unit **unit_list);

extern struct city *city_workers_display;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__MAPCTRL_COMMON_H */
