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

/**********************************************************************
                          mapview.h  -  description
                             -------------------
    begin                : Aug 10 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 *********************************************************************/

#ifndef FC__MAPVIEW_H
#define FC__MAPVIEW_H

#include "SDL.h"

#include "mapview_g.h"
#include "mapview_common.h"
#include "unitlist.h"

void redraw_unit_info_label(struct unit_list *punitlist);
SDL_Surface * create_city_map(struct city *pCity);
SDL_Surface * get_terrain_surface(struct tile *ptile);  
void toggle_overview_mode(void);
void refresh_overview(void);

void flush_rect(SDL_Rect rect, bool force_flush);
void sdl_dirty_rect(SDL_Rect rect);
void unqueue_flush(void);
void queue_flush(void);
void flush_all(void);

#endif	/* FC__MAPVIEW_H */
