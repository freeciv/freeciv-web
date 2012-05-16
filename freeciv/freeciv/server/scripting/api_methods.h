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

#ifndef FC__API_METHODS_H
#define FC__API_METHODS_H

#include "api_types.h"

bool api_methods_unit_city_can_be_built_here(Unit *punit);

int api_methods_player_num_cities(Player *pplayer);
int api_methods_player_num_units(Player *pplayer);

bool api_methods_unit_type_has_flag(Unit_Type *punit_type, const char *flag);
bool api_methods_unit_type_has_role(Unit_Type *punit_type, const char *role);

bool api_methods_tile_city_exists_within_city_radius(Tile *ptile, 
                                                     bool may_be_on_center);

bool api_methods_building_type_is_wonder(Building_Type *pbuilding);
bool api_methods_building_type_is_great_wonder(Building_Type *pbuilding);
bool api_methods_building_type_is_small_wonder(Building_Type *pbuilding);
bool api_methods_building_type_is_improvement(Building_Type *pbuilding);

/* rule name and translated name methods */
const char *api_methods_government_rule_name(Government *pgovernment);
const char *api_methods_government_name_translation(Government *pgovernment);
const char *api_methods_nation_type_rule_name(Nation_Type *pnation);
const char *api_methods_nation_type_name_translation(Nation_Type *pnation);
const char *api_methods_nation_type_plural_translation(Nation_Type
                                                       *pnation);
const char *api_methods_building_type_rule_name(Building_Type *pbuilding);
const char *api_methods_building_type_name_translation(Building_Type 
                                                       *pbuilding);
const char *api_methods_unit_type_rule_name(Unit_Type *punit_type);
const char *api_methods_unit_type_name_translation(Unit_Type *punit_type);
const char *api_methods_tech_type_rule_name(Tech_Type *ptech);
const char *api_methods_tech_type_name_translation(Tech_Type *ptech);
const char *api_methods_terrain_rule_name(Terrain *pterrain);
const char *api_methods_terrain_name_translation(Terrain *pterrain);

bool api_methods_city_has_building(City *pcity, Building_Type *building);
bool api_methods_player_has_wonder(Player *pplayer, Building_Type *building);

void api_methods_player_victory(Player *pplayer);

#endif /* FC__API_METHODS_H */
