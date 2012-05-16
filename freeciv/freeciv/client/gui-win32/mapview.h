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
#ifndef FC__MAPVIEW_H
#define FC__MAPVIEW_H

#include "mapview_g.h"

void check_mapstore(void);
void map_resize(void);
void init_map_win(void);
void map_expose(int x, int y, int width, int height);
void overview_expose(HDC hdc);
void map_handle_hscroll(int pos);
void map_handle_vscroll(int pos);
void anim_cursor(float time);

/* These values are stored in the mapview struct now. */
#define map_view_x 	mapview.map_x0
#define map_view_y 	mapview.map_y0
#define map_win_width 	mapview.width
#define map_win_height 	mapview.height
#define map_view_width 	mapview.tile_width
#define map_view_height mapview.tile_height
#define mapstorebitmap	(mapview.store->bmp)

/* Use of these wrapper functions is deprecated. */
#define get_canvas_xy(map_x, map_y, canvas_x, canvas_y) \
  tile_to_canvas_pos(canvas_x, canvas_y, map_x, map_y)

#endif  /* FC__MAPVIEW_H */
