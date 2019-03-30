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

#ifndef FC__API_GAME_METHODS_H
#define FC__API_GAME_METHODS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common/scriptcore */
#include "luascript_types.h"

struct lua_State;

/* Game */
int api_methods_game_turn(lua_State *L);
int api_methods_game_turn_deprecated(lua_State *L);
int api_methods_game_year(lua_State *L);
int api_methods_game_year_fragment(lua_State *L);
const char *api_methods_game_year_text(lua_State *L);
const char *api_methods_game_rulesetdir(lua_State *L);
const char *api_methods_game_ruleset_name(lua_State *L);

/* Building Type */
bool api_methods_building_type_is_wonder(lua_State *L,
                                         Building_Type *pbuilding);
bool api_methods_building_type_is_great_wonder(lua_State *L,
                                               Building_Type *pbuilding);
bool api_methods_building_type_is_small_wonder(lua_State *L,
                                               Building_Type *pbuilding);
bool api_methods_building_type_is_improvement(lua_State *L,
                                              Building_Type *pbuilding);
const char *api_methods_building_type_rule_name(lua_State *L,
                                                Building_Type *pbuilding);
const char *api_methods_building_type_name_translation(lua_State *L,
                                                       Building_Type *pbuilding);

/* City */
bool api_methods_city_has_building(lua_State *L, City *pcity,
                                   Building_Type *building);
int api_methods_city_map_sq_radius(lua_State *L, City *pcity);
int api_methods_city_size_get(lua_State *L, City *pcity);
Tile *api_methods_city_tile_get(lua_State *L, City *pcity);
int api_methods_city_inspire_partisans(lua_State *L, City *self, Player *inspirer);

int api_methods_city_culture_get(lua_State *L, City *pcity);

bool api_methods_is_city_happy(lua_State *L, City *pcity);
bool api_methods_is_city_unhappy(lua_State *L, City *pcity);
bool api_methods_is_city_celebrating(lua_State *L, City *pcity);
bool api_methods_is_gov_center(lua_State *L, City *pcity);
bool api_methods_is_capital(lua_State *L, City *pcity);

/* Government */
const char *api_methods_government_rule_name(lua_State *L,
                                             Government *pgovernment);
const char *api_methods_government_name_translation(lua_State *L,
                                                    Government *pgovernment);

/* Nation */
const char *api_methods_nation_type_rule_name(lua_State *L,
                                              Nation_Type *pnation);
const char *api_methods_nation_type_name_translation(lua_State *L,
                                                     Nation_Type *pnation);
const char *api_methods_nation_type_plural_translation(lua_State *L,
                                                       Nation_Type *pnation);

/* Player */
const char *api_methods_player_controlling_gui(lua_State *L, Player *pplayer);
bool api_methods_player_has_wonder(lua_State *L, Player *pplayer,
                                   Building_Type *building);
int api_methods_player_number(lua_State *L, Player *pplayer);
int api_methods_player_num_cities(lua_State *L, Player *pplayer);
int api_methods_player_num_units(lua_State *L, Player *pplayer);
int api_methods_player_gold(lua_State *L, Player *pplayer);
bool api_methods_player_knows_tech(lua_State *L, Player *pplayer,
                                   Tech_Type *ptech);
bool api_methods_player_shares_research(lua_State *L, Player *pplayer,
                                        Player *aplayer);
const char *api_methods_research_rule_name(lua_State *L, Player *pplayer);
const char *api_methods_research_name_translation(lua_State *L, Player *pplayer);
Unit_List_Link *api_methods_private_player_unit_list_head(lua_State *L,
                                                          Player *pplayer);
City_List_Link *api_methods_private_player_city_list_head(lua_State *L,
                                                          Player *pplayer);
int api_methods_player_culture_get(lua_State *L, Player *pplayer);

bool api_methods_player_has_flag(lua_State *L, Player *pplayer, const char *flag);

/* Tech Type */
const char *api_methods_tech_type_rule_name(lua_State *L, Tech_Type *ptech);
const char *api_methods_tech_type_name_translation(lua_State *L, Tech_Type *ptech);

/* Terrain */
const char *api_methods_terrain_rule_name(lua_State *L, Terrain *pterrain);
const char *api_methods_terrain_name_translation(lua_State *L, Terrain *pterrain);
const char *api_methods_terrain_class_name(lua_State *L, Terrain *pterrain);

/* Disaster */
const char *api_methods_disaster_rule_name(lua_State *L, Disaster *pdis);
const char *api_methods_disaster_name_translation(lua_State *L,
                                                  Disaster *pdis);

/* Achievement */
const char *api_methods_achievement_rule_name(lua_State *L, Achievement *pach);
const char *api_methods_achievement_name_translation(lua_State *L,
                                                     Achievement *pach);

/* Action */
const char *api_methods_action_rule_name(lua_State *L, Action *pact);
const char *api_methods_action_name_translation(lua_State *L,
                                                Action *pact);

/* Tile */
int api_methods_tile_nat_x(lua_State *L, Tile *ptile);
int api_methods_tile_nat_y(lua_State *L, Tile *ptile);
int api_methods_tile_map_x(lua_State *L, Tile *ptile);
int api_methods_tile_map_y(lua_State *L, Tile *ptile);
City *api_methods_tile_city(lua_State *L, Tile *ptile);
bool api_methods_tile_city_exists_within_max_city_map(lua_State *L,
                                                      Tile *ptile,
                                                      bool may_be_on_center);
bool api_methods_tile_has_extra(lua_State *L, Tile *ptile, const char *name);
bool api_methods_tile_has_base(lua_State *L, Tile *ptile, const char *name);
bool api_methods_tile_has_road(lua_State *L, Tile *ptile, const char *name);
bool api_methods_enemy_tile(lua_State *L, Tile *ptile, Player *against);
int api_methods_tile_num_units(lua_State *L, Tile *ptile);
int api_methods_tile_sq_distance(lua_State *L, Tile *ptile1, Tile *ptile2);
int api_methods_private_tile_next_outward_index(lua_State *L, Tile *pstart,
                                                int tindex, int max_dist);
Tile *api_methods_private_tile_for_outward_index(lua_State *L, Tile *pstart,
                                                 int tindex);
Unit_List_Link *api_methods_private_tile_unit_list_head(lua_State *L,
                                                        Tile *ptile);

/* Unit */
bool api_methods_unit_city_can_be_built_here(lua_State *L, Unit *punit);
Tile *api_methods_unit_tile_get(lua_State *L, Unit * punit);
Direction api_methods_unit_orientation_get(lua_State *L, Unit *punit);
Unit *api_methods_unit_transporter(lua_State *L, Unit *punit);
Unit_List_Link *api_methods_private_unit_cargo_list_head(lua_State *L,
                                                         Unit *punit);

/* Unit Type */
bool api_methods_unit_type_has_flag(lua_State *L, Unit_Type *punit_type,
                                    const char *flag);
bool api_methods_unit_type_has_role(lua_State *L, Unit_Type *punit_type,
                                    const char *role);
bool api_methods_unit_type_can_exist_at_tile(lua_State *L,
                                             Unit_Type *punit_type,
                                             Tile *ptile);
const char *api_methods_unit_type_rule_name(lua_State *L,
                                            Unit_Type *punit_type);
const char *api_methods_unit_type_name_translation(lua_State *L,
                                                   Unit_Type *punit_type);

/* Unit_List_Link Type */
Unit *api_methods_unit_list_link_data(lua_State *L, Unit_List_Link *link);
Unit_List_Link *api_methods_unit_list_next_link(lua_State *L,
                                                Unit_List_Link *link);

/* City_List_Link Type */
City *api_methods_city_list_link_data(lua_State *L, City_List_Link *link);
City_List_Link *api_methods_city_list_next_link(lua_State *L,
                                                City_List_Link *link);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__API_GAME_METHODS_H */
