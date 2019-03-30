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
#ifndef FC__TEXAIWORLD_H
#define FC__TEXAIWORLD_H

#include "texaimsg.h"

void texai_world_init(void);
void texai_world_close(void);

void texai_map_init(void);
void texai_map_close(void);
struct civ_map *texai_map_get(void);

void texai_tile_info(struct tile *ptile);
void texai_tile_info_recv(void *data);

void texai_city_created(struct city *pcity);
void texai_city_info_recv(void *data, enum texaimsgtype msgtype);
void texai_city_destroyed(struct city *pcity);
void texai_city_destruction_recv(void *data);
struct city *texai_map_city(int city_id);

void texai_unit_created(struct unit *punit);
void texai_unit_info_recv(void *data, enum texaimsgtype msgtype);
void texai_unit_destroyed(struct unit *punit);
void texai_unit_destruction_recv(void *data);
void texai_unit_move_seen(struct unit *punit);
void texai_unit_moved_recv(void *data);

#endif /* FC__TEXAIWORLD_H */
