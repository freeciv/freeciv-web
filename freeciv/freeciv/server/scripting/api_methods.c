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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "government.h"
#include "improvement.h"
#include "nation.h"
#include "tech.h"
#include "terrain.h"
#include "unitlist.h"
#include "unittype.h"

#include "api_methods.h"
#include "script.h"

/**************************************************************************
  Can punit found a city on its tile?
**************************************************************************/
bool api_methods_unit_city_can_be_built_here(Unit *punit)
{
  return city_can_be_built_here(punit->tile, punit);
}

/**************************************************************************
  Return the number of cities pplayer has.
**************************************************************************/
int api_methods_player_num_cities(Player *pplayer)
{
  return city_list_size(pplayer->cities);
}

/**************************************************************************
  Return the number of units pplayer has.
**************************************************************************/
int api_methods_player_num_units(Player *pplayer)
{
  return unit_list_size(pplayer->units);
}

/**************************************************************************
  Return TRUE if punit_type has flag.
**************************************************************************/
bool api_methods_unit_type_has_flag(Unit_Type *punit_type, const char *flag)
{
  enum unit_flag_id id = find_unit_flag_by_rule_name(flag);

  if (id != F_LAST) {
    return utype_has_flag(punit_type, id);
  } else {
    script_error("Unit flag \"%s\" does not exist", flag);
    return FALSE;
  }
}

/**************************************************************************
  Return TRUE if punit_type has role.
**************************************************************************/
bool api_methods_unit_type_has_role(Unit_Type *punit_type, const char *role)
{
  enum unit_role_id id = find_unit_role_by_rule_name(role);

  if (id != L_LAST) {
    return utype_has_role(punit_type, id);
  } else {
    script_error("Unit role \"%s\" does not exist", role);
    return FALSE;
  }
}

/**************************************************************************
  Return TRUE there is a city inside city radius from ptile
**************************************************************************/

bool api_methods_tile_city_exists_within_city_radius(Tile *ptile, 
                                              bool may_be_on_center)
{
  return city_exists_within_city_radius(ptile, may_be_on_center);
}

/**************************************************************************
  Return TRUE if pbuilding is a wonder.
**************************************************************************/
bool api_methods_building_type_is_wonder(Building_Type *pbuilding)
{
  return is_wonder(pbuilding);
}

/**************************************************************************
  Return TRUE if pbuilding is a great wonder.
**************************************************************************/
bool api_methods_building_type_is_great_wonder(Building_Type *pbuilding)
{
  return is_great_wonder(pbuilding);
}

/**************************************************************************
  Return TRUE if pbuilding is a small wonder.
**************************************************************************/
bool api_methods_building_type_is_small_wonder(Building_Type *pbuilding)
{
  return is_small_wonder(pbuilding);
}

/**************************************************************************
  Return TRUE if pbuilding is a building.
**************************************************************************/
bool api_methods_building_type_is_improvement(Building_Type *pbuilding)
{
  return is_improvement(pbuilding);
}

/**************************************************************************
  Return rule name for Government
**************************************************************************/
const char *api_methods_government_rule_name(Government *pgovernment)
{
  return government_rule_name(pgovernment);
}

/**************************************************************************
  Return translated name for Government
**************************************************************************/
const char *api_methods_government_name_translation(Government *pgovernment)
{
  return government_name_translation(pgovernment);
}

/**************************************************************************
  Return rule name for Nation_Type
**************************************************************************/
const char *api_methods_nation_type_rule_name(Nation_Type *pnation)
{
  return nation_rule_name(pnation);
}

/**************************************************************************
  Return translated adjective for Nation_Type
**************************************************************************/
const char *api_methods_nation_type_name_translation(Nation_Type *pnation)
{
  return nation_adjective_translation(pnation);
}

/**************************************************************************
  Return translated plural noun for Nation_Type
**************************************************************************/
const char *api_methods_nation_type_plural_translation(Nation_Type *pnation)
{
  return nation_plural_translation(pnation);
}

/**************************************************************************
  Return rule name for Building_Type
**************************************************************************/
const char *api_methods_building_type_rule_name(Building_Type *pbuilding)
{
  return improvement_rule_name(pbuilding);
}

/**************************************************************************
  Return translated name for Building_Type
**************************************************************************/
const char *api_methods_building_type_name_translation(Building_Type 
                                                       *pbuilding)
{
  return improvement_name_translation(pbuilding);
}

/**************************************************************************
  Return rule name for Unit_Type
**************************************************************************/
const char *api_methods_unit_type_rule_name(Unit_Type *punit_type)
{
  return utype_rule_name(punit_type);
}

/**************************************************************************
  Return translated name for Unit_Type
**************************************************************************/
const char *api_methods_unit_type_name_translation(Unit_Type *punit_type)
{
  return utype_name_translation(punit_type);
}

/**************************************************************************
  Return rule name for Tech_Type
**************************************************************************/
const char *api_methods_tech_type_rule_name(Tech_Type *ptech)
{
  return advance_rule_name(ptech);
}

/**************************************************************************
  Return translated name for Tech_Type
**************************************************************************/
const char *api_methods_tech_type_name_translation(Tech_Type *ptech)
{
  return advance_name_translation(ptech);
}

/**************************************************************************
  Return rule name for Terrain
**************************************************************************/
const char *api_methods_terrain_rule_name(Terrain *pterrain)
{
  return terrain_rule_name(pterrain);
}

/**************************************************************************
  Return translated name for Terrain
**************************************************************************/
const char *api_methods_terrain_name_translation(Terrain *pterrain)
{
  return terrain_name_translation(pterrain);
}

/**************************************************************************
  Return TRUE iff city has building
**************************************************************************/
bool api_methods_city_has_building(City *pcity, Building_Type *building)
{
  return city_has_building(pcity, building);
}

/**************************************************************************
  Return TRUE iff player has wonder
**************************************************************************/
bool api_methods_player_has_wonder(Player *pplayer, Building_Type *building)
{
  return wonder_is_built(pplayer, building);
}

/**************************************************************************
  Make player winner of the scenario
**************************************************************************/
void api_methods_player_victory(Player *pplayer)
{
  player_set_winner(pplayer);
}
