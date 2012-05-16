/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__API_FIND_H
#define FC__API_FIND_H

#include "api_types.h"

/* Object find module. */
Player *api_find_player(int player_id);
City *api_find_city(Player *pplayer, int city_id);
Unit *api_find_unit(Player *pplayer, int unit_id);
Tile *api_find_tile(int nat_x, int nat_y);

Government *api_find_government(int government_id);
Government *api_find_government_by_name(const char *name_orig);
Nation_Type *api_find_nation_type(int nation_type_id);
Nation_Type *api_find_nation_type_by_name(const char *name_orig);
Building_Type *api_find_building_type(int building_type_id);
Building_Type *api_find_building_type_by_name(const char *name_orig);
Unit_Type *api_find_unit_type(int unit_type_id);
Unit_Type *api_find_unit_type_by_name(const char *name_orig);
Unit_Type *api_find_role_unit_type(const char *role_name, Player *pplayer);
Tech_Type *api_find_tech_type(int tech_type_id);
Tech_Type *api_find_tech_type_by_name(const char *name_orig);
Terrain *api_find_terrain(int terrain_id);
Terrain *api_find_terrain_by_name(const char *name_orig);

#endif

