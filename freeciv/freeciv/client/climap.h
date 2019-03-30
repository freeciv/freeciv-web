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

#ifndef FC__CLIMAP_H
#define FC__CLIMAP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h"           /* enum direction8, struct tile */
#include "tile.h"               /* enum known_type */

enum known_type client_tile_get_known(const struct tile *ptile);

enum direction8 gui_to_map_dir(enum direction8 gui_dir);
enum direction8 map_to_gui_dir(enum direction8 map_dir);

struct tile *client_city_tile(const struct city *pcity);
bool client_city_can_work_tile(const struct city *pcity,
                               const struct tile *ptile);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__CLIMAP_H */
