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

#ifndef FC__API_SERVER_BASE_H
#define FC__API_SERVER_BASE_H

/* common/scriptcore */
#include "luascript_types.h"

struct lua_State;

/* Additional methods on the server. */
int api_server_player_civilization_score(lua_State *L, Player *pplayer);

bool api_server_was_started(lua_State *L);

bool api_server_save(lua_State *L, const char *filename);

const char *api_server_setting_get(lua_State *L, const char *sett_name);

bool api_play_music(lua_State *L, Player *pplayer, const char *tag);
bool api_showimg_playsnd(lua_State *L, Player *pplayer,
                         const char *img_filename, const char *snd_filename,
                         const char *desc, bool fullsize);

#endif /* FC__API_SERVER_BASE_H */
