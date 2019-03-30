/*****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifndef FC__API_GAME_FIND_H
#define FC__API_GAME_FIND_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* dependencies/lua */
#include "lua.h"

/* common/scriptcore */
#include "luascript_types.h"

/* Object find module. */
Player *api_find_player(lua_State *L, int player_id);

City *api_find_city(lua_State *L, Player *pplayer, int city_id);

Unit *api_find_unit(lua_State *L, Player *pplayer, int unit_id);
Unit *api_find_transport_unit(lua_State *L, Player *pplayer, Unit_Type *ptype,
                              Tile *ptile);
Tile *api_find_tile(lua_State *L, int nat_x, int nat_y);
Tile *api_find_tile_by_index(lua_State *L, int tindex);

Government *api_find_government(lua_State *L, int government_id);
Government *api_find_government_by_name(lua_State *L, const char *name_orig);

Nation_Type *api_find_nation_type(lua_State *L, int nation_type_id);
Nation_Type *api_find_nation_type_by_name(lua_State *L,
                                          const char *name_orig);
Action *api_find_action(lua_State *L, action_id act_id);
Action *api_find_action_by_name(lua_State *L, const char *name_orig);
Building_Type *api_find_building_type(lua_State *L, int building_type_id);
Building_Type *api_find_building_type_by_name(lua_State *L,
                                              const char *name_orig);
Unit_Type *api_find_unit_type(lua_State *L, int unit_type_id);
Unit_Type *api_find_unit_type_by_name(lua_State *L, const char *name_orig);
Unit_Type *api_find_role_unit_type(lua_State *L, const char *role_name,
                                   Player *pplayer);
Tech_Type *api_find_tech_type(lua_State *L, int tech_type_id);
Tech_Type *api_find_tech_type_by_name(lua_State *L, const char *name_orig);

Terrain *api_find_terrain(lua_State *L, int terrain_id);
Terrain *api_find_terrain_by_name(lua_State *L, const char *name_orig);

Nonexistent *api_find_nonexistent(lua_State *L);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* API_GAME_FIND */

