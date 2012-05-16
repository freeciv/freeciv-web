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

/* server */
#include "barbarian.h"
#include "citytools.h"
#include "maphand.h"
#include "plrhand.h"
#include "techtools.h"
#include "unittools.h"

#include "api_find.h"
#include "script_signal.h"

#include "api_actions.h"


/**************************************************************************
  Unleash barbarians on a tile, for example from a hut
**************************************************************************/
bool api_actions_unleash_barbarians(Tile *ptile)
{
  return unleash_barbarians(ptile);
}

/**************************************************************************
  Create a new unit.
**************************************************************************/
Unit *api_actions_create_unit(Player *pplayer, Tile *ptile, Unit_Type *ptype,
		  	      int veteran_level, City *homecity,
			      int moves_left)
{
  if (ptype == NULL
      || ptype < unit_type_array_first() || ptype > unit_type_array_last()) {
    return NULL;
  }

  return create_unit(pplayer, ptile, ptype, veteran_level,
		     homecity ? homecity->id : 0, moves_left);
}

/**************************************************************************
  Create a new city.
**************************************************************************/
void api_actions_create_city(Player *pplayer, Tile *ptile, const char *name)
{
  if (!name || name[0] == '\0') {
    name = city_name_suggestion(pplayer, ptile);
  }
  create_city(pplayer, ptile, name);
}

/**************************************************************************
  Change pplayer's gold by amount.
**************************************************************************/
void api_actions_change_gold(Player *pplayer, int amount)
{
  pplayer->economic.gold += amount;
}

/**************************************************************************
  Give pplayer technology ptech.  Quietly returns A_NONE (zero) if 
  player already has this tech; otherwise returns the tech granted.
  Use NULL for ptech to grant a random tech.
  sends script signal "tech_researched" with the given reason
**************************************************************************/
Tech_Type *api_actions_give_technology(Player *pplayer, Tech_Type *ptech,
                                       const char *reason)
{
  Tech_type_id id;
  Tech_Type *result;

  if (ptech) {
    id = advance_number(ptech);
  } else {
    if (get_player_research(pplayer)->researching == A_UNSET) {
      choose_random_tech(pplayer);
    }
    id = get_player_research(pplayer)->researching;
  }

  if (player_invention_state(pplayer, id) != TECH_KNOWN) {
    do_free_cost(pplayer, id);
    found_new_tech(pplayer, id, FALSE, TRUE);
    result = advance_by_number(id);
    script_signal_emit("tech_researched", 3,
                       API_TYPE_TECH_TYPE, result,
                       API_TYPE_PLAYER, pplayer,
                       API_TYPE_STRING, reason);
    return result;
  } else {
    return advance_by_number(A_NONE);
  }
}

/**************************************************************************
  Create a new base.
**************************************************************************/
void api_actions_create_base(Tile *ptile, const char *name, Player *pplayer)
{
  struct base_type *pbase;

  if (!name) {
    return;
  }

  pbase = find_base_type_by_rule_name(name);

  if (pbase) {
    create_base(ptile, pbase, pplayer);
  }
}
