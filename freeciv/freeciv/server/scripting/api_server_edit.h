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

#ifndef FC__API_SERVER_EDIT_H
#define FC__API_SERVER_EDIT_H

/* common/scriptcore */
#include "luascript_types.h"

struct lua_State;

/* type of clima change */
enum climate_change_type {
  CLIMATE_CHANGE_GLOBAL_WARMING,
  CLIMATE_CHANGE_NUCLEAR_WINTER
};

bool api_edit_unleash_barbarians(lua_State *L, Tile *ptile);
void api_edit_place_partisans(lua_State *L, Tile *ptile, Player *pplayer,
                              int count, int sq_radius);
Unit *api_edit_create_unit(lua_State *L, Player *pplayer, Tile *ptile,
                           Unit_Type *ptype, int veteran_level,
                           City *homecity, int moves_left);
Unit *api_edit_create_unit_full(lua_State *L, Player *pplayer, Tile *ptile,
                                Unit_Type *ptype, int veteran_level,
                                City *homecity, int moves_left, int hp_left,
                                Unit *ptransport);
bool api_edit_unit_teleport(lua_State *L, Unit *punit, Tile *dest);

void api_edit_unit_turn(lua_State *L, Unit *punit, Direction dir);

void api_edit_unit_kill(lua_State *L, Unit *punit, const char *reason,
                        Player *killer);

bool api_edit_change_terrain(lua_State *L, Tile *ptile, Terrain *pterr);

void api_edit_create_city(lua_State *L, Player *pplayer, Tile *ptile,
                          const char *name);
Player *api_edit_create_player(lua_State *L, const char *username,
                               Nation_Type *pnation, const char *ai);
void api_edit_change_gold(lua_State *L, Player *pplayer, int amount);
Tech_Type *api_edit_give_technology(lua_State *L, Player *pplayer,
                                    Tech_Type *ptech, int cost, bool notify,
                                    const char *reason);
bool api_edit_trait_mod_set(lua_State *L, Player *pplayer,
                            const char *tname, const int mod);

void api_edit_create_owned_extra(lua_State *L, Tile *ptile, const char *name,
                                 struct player *pplayer);
void api_edit_create_extra(lua_State *L, Tile *ptile, const char *name);
void api_edit_create_base(lua_State *L, Tile *ptile, const char *name,
                          struct player *pplayer);
void api_edit_create_road(lua_State *L, Tile *ptile, const char *name);
void api_edit_remove_extra(lua_State *L, Tile *ptile, const char *name);

void api_edit_tile_set_label(lua_State *L, Tile *ptile, const char *label);

void api_edit_climate_change(lua_State *L, enum climate_change_type type,
                             int effect);
Player *api_edit_civil_war(lua_State *L, Player *pplayer, int probability);


void api_edit_player_victory(lua_State *L, Player *pplayer);
bool api_edit_unit_move(lua_State *L, Unit *punit, Tile *ptile,
                        int movecost);
void api_edit_unit_moving_disallow(lua_State *L, Unit *punit);
void api_edit_unit_moving_allow(lua_State *L, Unit *punit);

void api_edit_city_add_history(lua_State *L, City *pcity, int amount);
void api_edit_player_add_history(lua_State *L, Player *pplayer, int amount);

#endif /* API_SERVER_EDIT_H */
