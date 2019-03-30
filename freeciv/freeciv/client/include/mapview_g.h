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

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "fc_types.h"
#include "unitlist.h"

/* client */
#include "mapview_common.h"

/* client/include */
#include "canvas_g.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, update_info_label, void)
GUI_FUNC_PROTO(void, update_unit_info_label, struct unit_list *punitlist)
GUI_FUNC_PROTO(void, update_mouse_cursor, enum cursor_type new_cursor_type)
GUI_FUNC_PROTO(void, update_timeout_label, void)
GUI_FUNC_PROTO(void, update_turn_done_button, bool do_restore)
GUI_FUNC_PROTO(void, update_city_descriptions, void)
GUI_FUNC_PROTO(void, set_indicator_icons, struct sprite *bulb, struct sprite *sol,
               struct sprite *flake, struct sprite *gov)

GUI_FUNC_PROTO(void, overview_size_changed, void)
GUI_FUNC_PROTO(void, update_overview_scroll_window_pos, int x, int y)
GUI_FUNC_PROTO(void, get_overview_area_dimensions, int *width, int *height)
GUI_FUNC_PROTO(struct canvas *, get_overview_window, void)

GUI_FUNC_PROTO(void, flush_mapcanvas, int canvas_x, int canvas_y,
               int pixel_width, int pixel_height)
GUI_FUNC_PROTO(void, dirty_rect, int canvas_x, int canvas_y,
               int pixel_width, int pixel_height)
GUI_FUNC_PROTO(void, dirty_all, void)
GUI_FUNC_PROTO(void, flush_dirty, void)
GUI_FUNC_PROTO(void, gui_flush, void)

GUI_FUNC_PROTO(void, update_map_canvas_scrollbars, void)
GUI_FUNC_PROTO(void, update_map_canvas_scrollbars_size, void)

GUI_FUNC_PROTO(void, put_cross_overlay_tile, struct tile *ptile)

GUI_FUNC_PROTO(void, draw_selection_rectangle, int canvas_x, int canvas_y,
               int w, int h)
GUI_FUNC_PROTO(void, tileset_changed, void)

#endif  /* FC__MAPVIEW_G_H */
