/***********************************************************************
 Freeciv - Copyright (C) 1996-2015 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__API_SERVER_GAME_METHODS_H
#define FC__API_SERVER_GAME_METHODS_H

/* common/scriptcore */
#include "luascript_types.h"

/* server/scripting */
#include "api_server_edit.h"

/* Server-only methods added to the modules defined in
 * the common tolua_game.pkg. */

int api_methods_player_trait(lua_State *L, Player *pplayer,
                             const char *tname);
int api_methods_player_trait_base(lua_State *L, Player *pplayer,
                                  const char *tname);
int api_methods_player_trait_current_mod(lua_State *L, Player *pplayer,
                                         const char *tname);

int api_methods_nation_trait_min(lua_State *L, Nation_Type *pnation,
                                 const char *tname);
int api_methods_nation_trait_max(lua_State *L, Nation_Type *pnation,
                                 const char *tname);
int api_methods_nation_trait_default(lua_State *L, Nation_Type *pnation,
                                     const char *tname);

#endif /* FC__API_SERVER_GAME_METHODS_H */
