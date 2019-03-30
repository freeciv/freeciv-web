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

#ifndef FC__API_GAME_EFFECTS_H
#define FC__API_GAME_EFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common/scriptcore */
#include "luascript_types.h"

struct lua_State;

int api_effects_world_bonus(lua_State *L, const char *effect_type);
int api_effects_player_bonus(lua_State *L, Player *pplayer,
                             const char *effect_type);
int api_effects_city_bonus(lua_State *L, City *pcity,
                           const char *effect_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* API_GAME_EFFECTS */
