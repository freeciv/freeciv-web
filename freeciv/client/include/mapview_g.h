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
#ifndef FC__MAPVIEW_G_H
#define FC__MAPVIEW_G_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

#include "canvas_g.h"

#include "mapview_common.h"

#include "unitlist.h"

void update_info_label(void);
void update_unit_info_label(struct unit_list *punitlist);
void update_mouse_cursor(enum cursor_type new_cursor_type);
void update_timeout_label(void);
void update_turn_done_button(bool do_restore);
void update_city_descriptions(void);
void set_indicator_icons(struct sprite *bulb, struct sprite *sol,
			 struct sprite *flake, struct sprite *gov);

void overview_size_changed(void);
void get_overview_area_dimensions(int *width, int *height);
struct canvas *get_overview_window(void);

void flush_mapcanvas(int canvas_x, int canvas_y,
		     int pixel_width, int pixel_height);
void dirty_rect(int canvas_x, int canvas_y,
		int pixel_width, int pixel_height);
void dirty_all(void);
void flush_dirty(void);
void gui_flush(void);

void update_map_canvas_scrollbars(void);
void update_map_canvas_scrollbars_size(void);

void put_cross_overlay_tile(struct tile *ptile);

void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h);
void tileset_changed(void);

#endif  /* FC__MAPVIEW_G_H */
