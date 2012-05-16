/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
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

#include <assert.h>

#include "astring.h"
#include "fcintl.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "base.h"
#include "city.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "tech.h"
#include "unit.h"
#include "unitlist.h"

/**************************************************************************
bribe unit
investigate
poison
make revolt
establish embassy
sabotage city
**************************************************************************/

/**************************************************************************
Whether a diplomat can move to a particular tile and perform a
particular action there.
**************************************************************************/
bool diplomat_can_do_action(const struct unit *pdiplomat,
			    enum diplomat_actions action, 
			    const struct tile *ptile)
{
  if (!is_diplomat_action_available(pdiplomat, action, ptile)) {
    return FALSE;
  }

  if (!is_tiles_adjacent(pdiplomat->tile, ptile)
      && !same_pos(pdiplomat->tile, ptile)) {
    return FALSE;
  }

  if(pdiplomat->moves_left == 0)
    return FALSE;

  return TRUE;
}

/**************************************************************************
Whether a diplomat can perform a particular action at a particular
tile.  This does _not_ check whether the diplomat can move there.
If the action is DIPLOMAT_ANY_ACTION, checks whether there is any
action the diplomat can perform at the tile.
**************************************************************************/
bool is_diplomat_action_available(const struct unit *pdiplomat,
				  enum diplomat_actions action, 
				  const struct tile *ptile)
{
  struct city *pcity=tile_city(ptile);

  if (action != DIPLOMAT_MOVE
      && !can_unit_exist_at_tile(pdiplomat, pdiplomat->tile)) {
    return FALSE;
  }

  if (pcity) {
    if (city_owner(pcity) != unit_owner(pdiplomat)
       && real_map_distance(pdiplomat->tile, pcity->tile) <= 1) {
      if(action==DIPLOMAT_SABOTAGE)
	return pplayers_at_war(unit_owner(pdiplomat), city_owner(pcity));
      if(action==DIPLOMAT_MOVE)
        return pplayers_allied(unit_owner(pdiplomat), city_owner(pcity));
      if (action == DIPLOMAT_EMBASSY
          && !get_player_bonus(city_owner(pcity), EFT_NO_DIPLOMACY)
          && !player_has_embassy(unit_owner(pdiplomat), city_owner(pcity))) {
	return TRUE;
      }
      if(action==SPY_POISON &&
	 pcity->size>1 &&
	 unit_has_type_flag(pdiplomat, F_SPY))
	return pplayers_at_war(unit_owner(pdiplomat), city_owner(pcity));
      if(action==DIPLOMAT_INVESTIGATE)
        return TRUE;
      if (action == DIPLOMAT_STEAL && !is_barbarian(city_owner(pcity))) {
	return TRUE;
      }
      if(action==DIPLOMAT_INCITE)
        return !pplayers_allied(city_owner(pcity), unit_owner(pdiplomat));
      if(action==DIPLOMAT_ANY_ACTION)
        return TRUE;
    }
  } else { /* Action against a unit at a tile */
    /* If it is made possible to do action against allied units
       unit_move_handling() should be changed so that pdefender
       is also set to allied units */
    struct unit *punit;

    if ((action == SPY_SABOTAGE_UNIT || action == DIPLOMAT_ANY_ACTION) 
        && unit_list_size(ptile->units) == 1
        && unit_has_type_flag(pdiplomat, F_SPY)) {
      punit = unit_list_get(ptile->units, 0);
      if (pplayers_at_war(unit_owner(pdiplomat), unit_owner(punit))) {
        return TRUE;
      }
    }

    if ((action == DIPLOMAT_BRIBE || action == DIPLOMAT_ANY_ACTION)
        && unit_list_size(ptile->units) == 1) {
      punit = unit_list_get(ptile->units, 0);
      if (!pplayers_allied(unit_owner(punit), unit_owner(pdiplomat))) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**************************************************************************
FIXME: Maybe we should allow airlifts between allies
**************************************************************************/
bool unit_can_airlift_to(const struct unit *punit, const struct city *pcity)
{
  struct city *acity = tile_city(punit->tile);

  if(punit->moves_left == 0)
    return FALSE;
  if (!acity) {
    return FALSE;
  }
  if (acity == pcity) {
    return FALSE;
  }
  if (city_owner(acity) != city_owner(pcity)) {
    return FALSE;
  }
  if (acity->airlift <= 0 || pcity->airlift <= 0) {
    return FALSE;
  }
  if (!is_ground_unit(punit))
    return FALSE;

  return TRUE;
}

/****************************************************************************
  Return TRUE iff the unit is following client-side orders.
****************************************************************************/
bool unit_has_orders(const struct unit *punit)
{
  return punit->has_orders;
}

/**************************************************************************
  Return TRUE iff this unit can be disbanded at the given city to get full
  shields for building a wonder.
**************************************************************************/
bool unit_can_help_build_wonder(const struct unit *punit,
				const struct city *pcity)
{
  if (!is_tiles_adjacent(punit->tile, pcity->tile)
      && !same_pos(punit->tile, pcity->tile)) {
    return FALSE;
  }

  return (unit_has_type_flag(punit, F_HELP_WONDER)
	  && unit_owner(punit) == city_owner(pcity)
	  && VUT_IMPROVEMENT == pcity->production.kind
	  && is_wonder(pcity->production.value.building)
	  && (pcity->shield_stock
	      < impr_build_shield_cost(pcity->production.value.building)));
}


/**************************************************************************
  Return TRUE iff this unit can be disbanded at its current position to
  get full shields for building a wonder.
**************************************************************************/
bool unit_can_help_build_wonder_here(const struct unit *punit)
{
  struct city *pcity = tile_city(punit->tile);

  return pcity && unit_can_help_build_wonder(punit, pcity);
}


/**************************************************************************
  Return TRUE iff this unit can be disbanded at its current location to
  provide a trade route from the homecity to the target city.
**************************************************************************/
bool unit_can_est_traderoute_here(const struct unit *punit)
{
  struct city *phomecity, *pdestcity;

  return (unit_has_type_flag(punit, F_TRADE_ROUTE)
	  && (pdestcity = tile_city(punit->tile))
	  && (phomecity = game_find_city_by_number(punit->homecity))
	  && can_cities_trade(phomecity, pdestcity));
}

/**************************************************************************
  Return the number of units the transporter can hold (or 0).
**************************************************************************/
int get_transporter_capacity(const struct unit *punit)
{
  return unit_type(punit)->transport_capacity;
}

/**************************************************************************
  Is the unit capable of attacking?
**************************************************************************/
bool is_attack_unit(const struct unit *punit)
{
  return (unit_type(punit)->attack_strength > 0);
}

/**************************************************************************
  Military units are capable of enforcing martial law. Military ground
  and heli units can occupy empty cities -- see COULD_OCCUPY(punit).
  Some military units, like the Galleon, have no attack strength.
**************************************************************************/
bool is_military_unit(const struct unit *punit)
{
  return !unit_has_type_flag(punit, F_CIVILIAN);
}

/**************************************************************************
  Return TRUE iff this unit is a diplomat (spy) unit.  Diplomatic units
  can do diplomatic actions (not to be confused with diplomacy).
**************************************************************************/
bool is_diplomat_unit(const struct unit *punit)
{
  return (unit_has_type_flag(punit, F_DIPLOMAT));
}

/**************************************************************************
  Return TRUE iff the player should consider this unit to be a threat on
  the ground.
**************************************************************************/
static bool is_ground_threat(const struct player *pplayer,
			     const struct unit *punit)
{
  return (pplayers_at_war(pplayer, unit_owner(punit))
	  && (unit_has_type_flag(punit, F_DIPLOMAT)
	      || (is_ground_unit(punit) && is_military_unit(punit))));
}

/**************************************************************************
  Return TRUE iff this tile is threatened from any threatening ground unit
  within 2 tiles.
**************************************************************************/
bool is_square_threatened(const struct player *pplayer,
			  const struct tile *ptile)
{
  square_iterate(ptile, 2, ptile1) {
    unit_list_iterate(ptile1->units, punit) {
      if (is_ground_threat(pplayer, punit)) {
	return TRUE;
      }
    } unit_list_iterate_end;
  } square_iterate_end;

  return FALSE;
}

/**************************************************************************
  This checks the "field unit" flag on the unit.  Field units cause
  unhappiness (under certain governments) even when they aren't abroad.
**************************************************************************/
bool is_field_unit(const struct unit *punit)
{
  return unit_has_type_flag(punit, F_FIELDUNIT);
}


/**************************************************************************
  Is the unit one that is invisible on the map. A unit is invisible if
  it has the F_PARTIAL_INVIS flag or if it transported by a unit with
  this flag.
**************************************************************************/
bool is_hiding_unit(const struct unit *punit)
{
  struct unit *transporter = game_find_unit_by_number(punit->transported_by);

  return (unit_has_type_flag(punit, F_PARTIAL_INVIS)
	  || (transporter && unit_has_type_flag(transporter, F_PARTIAL_INVIS)));
}

/**************************************************************************
  Return TRUE iff an attack from this unit would kill a citizen in a city
  (city walls protect against this).
**************************************************************************/
bool kills_citizen_after_attack(const struct unit *punit)
{
  return TEST_BIT(game.info.killcitizen, 
                  (int) (uclass_move_type(unit_class(punit))) - 1);
}

/**************************************************************************
  Return TRUE iff this unit may be disbanded to add its pop_cost to a
  city at its current location.
**************************************************************************/
bool can_unit_add_to_city(const struct unit *punit)
{
  return (test_unit_add_or_build_city(punit) == AB_ADD_OK);
}

/**************************************************************************
  Return TRUE iff this unit is capable of building a new city at its
  current location.
**************************************************************************/
bool can_unit_build_city(const struct unit *punit)
{
  return (test_unit_add_or_build_city(punit) == AB_BUILD_OK);
}

/**************************************************************************
  Return TRUE iff this unit can add to a current city or build a new city
  at its current location.
**************************************************************************/
bool can_unit_add_or_build_city(const struct unit *punit)
{
  enum add_build_city_result r = test_unit_add_or_build_city(punit);

  return (r == AB_BUILD_OK || r == AB_ADD_OK);
}

/**************************************************************************
  See if the unit can add to an existing city or build a new city at
  its current location, and return a 'result' value telling what is
  allowed.
**************************************************************************/
enum add_build_city_result test_unit_add_or_build_city(const struct unit *
						       punit)
{
  struct city *pcity = tile_city(punit->tile);
  bool is_build = unit_has_type_flag(punit, F_CITIES);
  bool is_add = unit_has_type_flag(punit, F_ADD_TO_CITY);
  int new_pop;

  /* See if we can build */
  if (!pcity) {
    if (!is_build)
      return AB_NOT_BUILD_UNIT;
    if (punit->moves_left == 0)
      return AB_NO_MOVES_BUILD;
    if (!city_can_be_built_here(punit->tile, punit)) {
      return AB_NOT_BUILD_LOC;
    }
    return AB_BUILD_OK;
  }
  
  /* See if we can add */

  if (!is_add)
    return AB_NOT_ADDABLE_UNIT;
  if (punit->moves_left == 0)
    return AB_NO_MOVES_ADD;

  assert(unit_pop_value(punit) > 0);
  new_pop = pcity->size + unit_pop_value(punit);

  if (new_pop > game.info.add_to_size_limit)
    return AB_TOO_BIG;
  if (city_owner(pcity) != unit_owner(punit))
    return AB_NOT_OWNER;
  if (!city_can_grow_to(pcity, new_pop))
    return AB_NO_SPACE;
  return AB_ADD_OK;
}

/**************************************************************************
  Return TRUE iff the unit can change homecity to the given city.
**************************************************************************/
bool can_unit_change_homecity_to(const struct unit *punit,
				 const struct city *pcity)
{
  struct city *acity = tile_city(punit->tile);

  /* Requirements to change homecity:
   *
   * 1. Homeless units can't change homecity (this is a feature since
   *    being homeless is a big benefit).
   * 2. The unit must be inside the city it is rehoming to.
   * 3. Of course you can only have your own cities as homecity.
   * 4. You can't rehome to the current homecity. */
  return (punit && pcity
	  && punit->homecity > 0
	  && acity
	  && city_owner(acity) == unit_owner(punit)
	  && punit->homecity != acity->id);
}

/**************************************************************************
  Return TRUE iff the unit can change homecity at its current location.
**************************************************************************/
bool can_unit_change_homecity(const struct unit *punit)
{
  return can_unit_change_homecity_to(punit, tile_city(punit->tile));
}

/**************************************************************************
  Returns the speed of a unit doing an activity.  This depends on the
  veteran level and the base move_rate of the unit (regardless of HP or
  effects).  Usually this is just used for settlers but the value is also
  used for military units doing fortify/pillage activities.

  The speed is multiplied by ACTIVITY_COUNT.
**************************************************************************/
int get_activity_rate(const struct unit *punit)
{
  double fact = unit_type(punit)->veteran[punit->veteran].power_fact;

  /* The speed of the settler depends on its base move_rate, not on
   * the number of moves actually remaining or the adjusted move rate.
   * This means sea formers won't have their activity rate increased by
   * Magellan's, and it means injured units work just as fast as
   * uninjured ones.  Note the value is never less than SINGLE_MOVE. */
  int move_rate = unit_type(punit)->move_rate;

  /* All settler actions are multiplied by ACTIVITY_COUNT. */
  return ACTIVITY_FACTOR * fact * move_rate / SINGLE_MOVE;
}

/**************************************************************************
  Returns the amount of work a unit does (will do) on an activity this
  turn.  Units that have no MP do no work.

  The speed is multiplied by ACTIVITY_COUNT.
**************************************************************************/
int get_activity_rate_this_turn(const struct unit *punit)
{
  /* This logic is also coded in client/goto.c. */
  if (punit->moves_left > 0) {
    return get_activity_rate(punit);
  } else {
    return 0;
  }
}

/**************************************************************************
  Return the estimated number of turns for the worker unit to start and
  complete the activity at the given location.  This assumes no other
  worker units are helping out.
**************************************************************************/
int get_turns_for_activity_at(const struct unit *punit,
			      enum unit_activity activity,
			      const struct tile *ptile)
{
  /* FIXME: This is just an approximation since we don't account for
   * get_activity_rate_this_turn. */
  int speed = get_activity_rate(punit);
  int time = tile_activity_time(activity, ptile);

  if (time >= 0 && speed >= 0) {
    return (time - 1) / speed + 1; /* round up */
  } else {
    return FC_INFINITY;
  }
}

/**************************************************************************
  Return whether the unit can be put in auto-settler mode.

  NOTE: we used to have "auto" mode including autosettlers and auto-attack.
  This was bad because the two were indestinguishable even though they
  are very different.  Now auto-attack is done differently so we just have
  auto-settlers.  If any new auto modes are introduced they should be
  handled separately.
**************************************************************************/
bool can_unit_do_autosettlers(const struct unit *punit) 
{
  return unit_has_type_flag(punit, F_SETTLERS);
}

/**************************************************************************
  Return if given activity really is in game. For savegame compatibility
  activity enum cannot be reordered and there is holes in it.
**************************************************************************/
bool is_real_activity(enum unit_activity activity)
{
  /* ACTIVITY_FORTRESS and ACTIVITY_AIRBASE are deprecated */
  return activity != ACTIVITY_FORTRESS
    && activity != ACTIVITY_AIRBASE
    && activity != ACTIVITY_UNKNOWN
    && activity != ACTIVITY_PATROL_UNUSED;
}

/**************************************************************************
  Return the name of the activity in a static buffer.
**************************************************************************/
const char *get_activity_text(enum unit_activity activity)
{
  /* The switch statement has just the activities listed with no "default"
   * handling.  This enables the compiler to detect missing entries
   * automatically, and still handles everything correctly. */
  switch (activity) {
  case ACTIVITY_IDLE:
    return _("Idle");
  case ACTIVITY_POLLUTION:
    return _("Pollution");
  case ACTIVITY_ROAD:
    return _("Road");
  case ACTIVITY_MINE:
    return _("Mine");
  case ACTIVITY_IRRIGATE:
    return _("Irrigation");
  case ACTIVITY_FORTIFYING:
    return _("Fortifying");
  case ACTIVITY_FORTIFIED:
    return _("Fortified");
  case ACTIVITY_FORTRESS:
    return _("Fortress");
  case ACTIVITY_SENTRY:
    return _("Sentry");
  case ACTIVITY_RAILROAD:
    return _("Railroad");
  case ACTIVITY_PILLAGE:
    return _("Pillage");
  case ACTIVITY_GOTO:
    return _("Goto");
  case ACTIVITY_EXPLORE:
    return _("Explore");
  case ACTIVITY_TRANSFORM:
    return _("Transform");
  case ACTIVITY_AIRBASE:
    return _("Airbase");
  case ACTIVITY_FALLOUT:
    return _("Fallout");
  case ACTIVITY_BASE:
    return _("Base");
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
    break;
  }

  assert(0);
  return _("Unknown");
}

/****************************************************************************
  Return TRUE iff the given unit could be loaded into the transporter
  if we moved there.
****************************************************************************/
bool could_unit_load(const struct unit *pcargo, const struct unit *ptrans)
{
  if (!pcargo || !ptrans) {
    return FALSE;
  }

  /* Double-check ownership of the units: you can load into an allied unit
   * (of course only allied units can be on the same tile). */
  if (!pplayers_allied(unit_owner(pcargo), unit_owner(ptrans))) {
    return FALSE;
  }

  /* Only top-level transporters may be loaded or loaded into. */
  if (pcargo->transported_by != -1 || ptrans->transported_by != -1) {
    return FALSE;
  }

  /* Recursive transporting is not allowed (for now). */
  if (get_transporter_occupancy(pcargo) > 0) {
    return FALSE;
  }

  /* Make sure this transporter can carry this type of unit. */
  if (!can_unit_transport(ptrans, pcargo)) {
    return FALSE;
  }

  /* Transporter must be native to the tile it is on. */
  if (!can_unit_exist_at_tile(ptrans, ptrans->tile)) {
    return FALSE;
  }

  /* Make sure there's room in the transporter. */
  return (get_transporter_occupancy(ptrans)
	  < get_transporter_capacity(ptrans));
}

/****************************************************************************
  Return TRUE iff the given unit can be loaded into the transporter.
****************************************************************************/
bool can_unit_load(const struct unit *pcargo, const struct unit *ptrans)
{
  /* This function needs to check EVERYTHING. */

  /* Check positions of the units.  Of course you can't load a unit onto
   * a transporter on a different tile... */
  if (!same_pos(pcargo->tile, ptrans->tile)) {
    return FALSE;
  }

  return could_unit_load(pcargo, ptrans);
}

/****************************************************************************
  Return TRUE iff the given unit can be unloaded from its current
  transporter.

  This function checks everything *except* the legality of the position
  after the unloading.  The caller may also want to call
  can_unit_exist_at_tile() to check this, unless the unit is unloading and
  moving at the same time.
****************************************************************************/
bool can_unit_unload(const struct unit *pcargo, const struct unit *ptrans)
{
  if (!pcargo || !ptrans) {
    return FALSE;
  }

  /* Make sure the unit's transporter exists and is known. */
  if (pcargo->transported_by != ptrans->id) {
    return FALSE;
  }

  /* Only top-level transporters may be unloaded.  However the unit being
   * unloaded may be transporting other units (well, at least it's allowed
   * here: elsewhere this may be disallowed). */
  if (ptrans->transported_by != -1) {
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  Return whether the unit can be paradropped - that is, if the unit is in
  a friendly city or on an airbase special, has enough movepoints left, and
  has not paradropped yet this turn.
**************************************************************************/
bool can_unit_paradrop(const struct unit *punit)
{
  struct unit_type *utype;

  if (!unit_has_type_flag(punit, F_PARATROOPERS))
    return FALSE;

  if(punit->paradropped)
    return FALSE;

  utype = unit_type(punit);

  if(punit->moves_left < utype->paratroopers_mr_req)
    return FALSE;

  if (tile_has_base_flag(punit->tile, BF_PARADROP_FROM)) {
    /* Paradrop has to be possible from non-native base.
     * Paratroopers are "Land" units, but they can paradrom from Airbase. */
    return TRUE;
  }

  if (!tile_city(punit->tile)) {
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  Return whether the unit can bombard.
  Basically if it is a bombarder, isn't being transported, and hasn't 
  moved this turn.
**************************************************************************/
bool can_unit_bombard(const struct unit *punit)
{
  if (!unit_has_type_flag(punit, F_BOMBARDER)) {
    return FALSE;
  }

  if (punit->transported_by != -1) {
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  Check if the unit's current activity is actually legal.
**************************************************************************/
bool can_unit_continue_current_activity(struct unit *punit)
{
  enum unit_activity current = punit->activity;
  enum tile_special_type target = punit->activity_target;
  Base_type_id base = punit->activity_base;
  enum unit_activity current2 = 
              (current == ACTIVITY_FORTIFIED) ? ACTIVITY_FORTIFYING : current;
  bool result;

  punit->activity = ACTIVITY_IDLE;
  punit->activity_target = S_LAST;
  punit->activity_base = -1;

  result = can_unit_do_activity_targeted(punit, current2, target, base);

  punit->activity = current;
  punit->activity_target = target;
  punit->activity_base = base;

  return result;
}

/**************************************************************************
  Return TRUE iff the unit can do the given untargeted activity at its
  current location.

  Note that some activities must be targeted; see
  can_unit_do_activity_targeted.
**************************************************************************/
bool can_unit_do_activity(const struct unit *punit,
			  enum unit_activity activity)
{
  return can_unit_do_activity_targeted(punit, activity, S_LAST, -1);
}

/**************************************************************************
  Return TRUE iff the unit can do the given base building activity at its
  current location.
**************************************************************************/
bool can_unit_do_activity_base(const struct unit *punit,
                               Base_type_id base)
{
  return can_unit_do_activity_targeted(punit, ACTIVITY_BASE, S_LAST, base);
}

/**************************************************************************
  Return whether the unit can do the targeted activity at its current
  location.
**************************************************************************/
bool can_unit_do_activity_targeted(const struct unit *punit,
				   enum unit_activity activity,
				   enum tile_special_type target,
                                   Base_type_id base)
{
  return can_unit_do_activity_targeted_at(punit, activity, target,
					  punit->tile, base);
}

/**************************************************************************
  Return TRUE if the unit can do the targeted activity at the given
  location.

  Note that if you make changes here you should also change the code for
  autosettlers in server/settler.c. The code there does not use this
  function as it would be a major CPU hog.
**************************************************************************/
bool can_unit_do_activity_targeted_at(const struct unit *punit,
				      enum unit_activity activity,
				      enum tile_special_type target,
				      const struct tile *ptile,
                                      Base_type_id base)
{
  struct player *pplayer = unit_owner(punit);
  struct terrain *pterrain = tile_terrain(ptile);
  struct unit_class *pclass = unit_class(punit);
  struct base_type *pbase = base_by_number(base);

  switch(activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_GOTO:
    return TRUE;

  case ACTIVITY_POLLUTION:
    return (unit_has_type_flag(punit, F_SETTLERS)
	    && tile_has_special(ptile, S_POLLUTION));

  case ACTIVITY_FALLOUT:
    return (unit_has_type_flag(punit, F_SETTLERS)
	    && tile_has_special(ptile, S_FALLOUT));

  case ACTIVITY_ROAD:
    return (terrain_control.may_road
	    && unit_has_type_flag(punit, F_SETTLERS)
	    && !tile_has_special(ptile, S_ROAD)
	    && pterrain->road_time != 0
	    && (!tile_has_special(ptile, S_RIVER)
		|| player_knows_techs_with_flag(pplayer, TF_BRIDGE)));

  case ACTIVITY_MINE:
    /* Don't allow it if someone else is irrigating this tile.
     * *Do* allow it if they're transforming - the mine may survive */
    if (terrain_control.may_mine
	&& unit_has_type_flag(punit, F_SETTLERS)
	&& ((pterrain == pterrain->mining_result
	     && !tile_has_special(ptile, S_MINE))
	    || (pterrain != pterrain->mining_result
		&& pterrain->mining_result != T_NONE
		&& (!is_ocean(pterrain)
		    || is_ocean(pterrain->mining_result)
		    || can_reclaim_ocean(ptile))
		&& (is_ocean(pterrain)
		    || !is_ocean(pterrain->mining_result)
		    || can_channel_land(ptile))
		&& (!is_ocean(pterrain->mining_result)
		    || !tile_city(ptile))))) {
      unit_list_iterate(ptile->units, tunit) {
	if (tunit->activity == ACTIVITY_IRRIGATE) {
	  return FALSE;
	}
      } unit_list_iterate_end;
      return TRUE;
    } else {
      return FALSE;
    }

  case ACTIVITY_IRRIGATE:
    /* Don't allow it if someone else is mining this tile.
     * *Do* allow it if they're transforming - the irrigation may survive */
    if (terrain_control.may_irrigate
	&& unit_has_type_flag(punit, F_SETTLERS)
	&& (!tile_has_special(ptile, S_IRRIGATION)
	    || (!tile_has_special(ptile, S_FARMLAND)
		&& player_knows_techs_with_flag(pplayer, TF_FARMLAND)))
	&& ((pterrain == pterrain->irrigation_result
	     && is_water_adjacent_to_tile(ptile))
	    || (pterrain != pterrain->irrigation_result
		&& pterrain->irrigation_result != T_NONE
		&& (!is_ocean(pterrain)
		    || is_ocean(pterrain->irrigation_result)
		    || can_reclaim_ocean(ptile))
		&& (is_ocean(pterrain)
		    || !is_ocean(pterrain->irrigation_result)
		    || can_channel_land(ptile))
		&& (!is_ocean(pterrain->irrigation_result)
		    || !tile_city(ptile))))) {
      unit_list_iterate(ptile->units, tunit) {
	if (tunit->activity == ACTIVITY_MINE) {
	  return FALSE;
	}
      } unit_list_iterate_end;
      return TRUE;
    } else {
      return FALSE;
    }

  case ACTIVITY_FORTIFYING:
    return (uclass_has_flag(pclass, UCF_CAN_FORTIFY)
	    && punit->activity != ACTIVITY_FORTIFIED
	    && !unit_has_type_flag(punit, F_SETTLERS)
	    && (!is_ocean(pterrain) || tile_city(ptile)));

  case ACTIVITY_FORTIFIED:
    return FALSE;

  case ACTIVITY_BASE:
    return can_build_base(punit, pbase, ptile);

  case ACTIVITY_SENTRY:
    if (!can_unit_survive_at_tile(punit, punit->tile)
	&& punit->transported_by == -1) {
      /* Don't let units sentry on tiles they will die on. */
      return FALSE;
    }
    return TRUE;

  case ACTIVITY_RAILROAD:
    /* if the tile has road, the terrain must be ok.. */
    return (terrain_control.may_road
	    && unit_has_type_flag(punit, F_SETTLERS)
	    && tile_has_special(ptile, S_ROAD)
	    && !tile_has_special(ptile, S_RAILROAD)
	    && player_knows_techs_with_flag(pplayer, TF_RAILROAD));

  case ACTIVITY_PILLAGE:
    {
      int numpresent;
      bv_special pspresent = get_tile_infrastructure_set(ptile, &numpresent);
      bv_bases bases;

      if ((numpresent > 0 || tile_has_any_bases(ptile))
          && uclass_has_flag(unit_class(punit), UCF_CAN_PILLAGE)) {
	bv_special psworking;
	int i;

	if (tile_city(ptile) && (target == S_ROAD || target == S_RAILROAD)) {
	  return FALSE;
	}
	psworking = get_unit_tile_pillage_set(ptile);

        BV_CLR_ALL(bases);
        base_type_iterate(pbase) {
          if (tile_has_base(ptile, pbase)) {
            if (pbase->pillageable) {
              BV_SET(bases, base_index(pbase));
              numpresent++;
            }
          }
        } base_type_iterate_end;

        if (numpresent == 0) {
          return FALSE;
        }

	if (target == S_LAST && base == -1) {
	  for (i = 0; infrastructure_specials[i] != S_LAST; i++) {
	    enum tile_special_type spe = infrastructure_specials[i];

	    if (tile_city(ptile) && (spe == S_ROAD || spe == S_RAILROAD)) {
	      /* Can't pillage this. */
	      continue;
	    }
	    if (BV_ISSET(pspresent, spe) && !BV_ISSET(psworking, spe)) {
	      /* Can pillage this! */
	      return TRUE;
	    }
	  }
	} else if (!game.info.pillage_select
		   && target != get_preferred_pillage(pspresent, bases)) {
	  return FALSE;
	} else {
          if (target == S_LAST && base != -1) {
            return BV_ISSET(bases, base);
          } else {
            return BV_ISSET(pspresent, target) && !BV_ISSET(psworking, target);
          }
	}
      } else {
	return FALSE;
      }
    }

  case ACTIVITY_EXPLORE:
    return (is_ground_unit(punit) || is_sailing_unit(punit));

  case ACTIVITY_TRANSFORM:
    return (terrain_control.may_transform
	    && pterrain->transform_result != T_NONE
	    && pterrain != pterrain->transform_result
	    && (!is_ocean(pterrain)
		|| is_ocean(pterrain->transform_result)
		|| can_reclaim_ocean(ptile))
	    && (is_ocean(pterrain)
		|| !is_ocean(pterrain->transform_result)
		|| can_channel_land(ptile))
	    && (!terrain_has_flag(pterrain->transform_result, TER_NO_CITIES)
		|| !(tile_city(ptile)))
	    && unit_has_type_flag(punit, F_TRANSFORM));

  case ACTIVITY_FORTRESS:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
  case ACTIVITY_UNKNOWN:
    break;
  }
  freelog(LOG_ERROR,
	  "can_unit_do_activity_targeted_at() unknown activity %d",
	  activity);
  return FALSE;
}

/**************************************************************************
  assign a new task to a unit.
**************************************************************************/
void set_unit_activity(struct unit *punit, enum unit_activity new_activity)
{
  punit->activity=new_activity;
  punit->activity_count=0;
  punit->activity_target = S_LAST;
  punit->activity_base = -1;
  if (new_activity == ACTIVITY_IDLE && punit->moves_left > 0) {
    /* No longer done. */
    punit->done_moving = FALSE;
  }
}

/**************************************************************************
  assign a new targeted task to a unit.
**************************************************************************/
void set_unit_activity_targeted(struct unit *punit,
				enum unit_activity new_activity,
				enum tile_special_type new_target,
                                Base_type_id base)
{
  assert(new_target != ACTIVITY_FORTRESS
         && new_target != ACTIVITY_AIRBASE);

  set_unit_activity(punit, new_activity);
  punit->activity_target = new_target;
  punit->activity_base = base;
}

/**************************************************************************
  Assign a new base building task to unit
**************************************************************************/
void set_unit_activity_base(struct unit *punit,
                            Base_type_id base)
{
  set_unit_activity(punit, ACTIVITY_BASE);
  punit->activity_base = base;
}

/**************************************************************************
  Return whether any units on the tile are doing this activity.
**************************************************************************/
bool is_unit_activity_on_tile(enum unit_activity activity,
			      const struct tile *ptile)
{
  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == activity) {
      return TRUE;
    }
  } unit_list_iterate_end;
  return FALSE;
}

/****************************************************************************
  Return a mask of the specials which are actively (currently) being
  pillaged on the given tile.
****************************************************************************/
bv_special get_unit_tile_pillage_set(const struct tile *ptile)
{
  bv_special tgt_ret;

  BV_CLR_ALL(tgt_ret);
  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == ACTIVITY_PILLAGE
        && punit->activity_target != S_LAST) {
      assert(punit->activity_target < S_LAST);
      BV_SET(tgt_ret, punit->activity_target);
    }
  } unit_list_iterate_end;

  return tgt_ret;
}

/**************************************************************************
  Return text describing the unit's current activity as a static string.

  FIXME: Convert all callers of this function to unit_activity_astr()
  because this function is not re-entrant.
**************************************************************************/
const char *unit_activity_text(const struct unit *punit) {
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);
  unit_activity_astr(punit, &str);

  return str.str;
}

/**************************************************************************
  Append text describing the unit's current activity to the given astring.
**************************************************************************/
void unit_activity_astr(const struct unit *punit, struct astring *astr)
{
  if (!punit || !astr) {
    return;
  }

  switch (punit->activity) {
  case ACTIVITY_IDLE:
    if (utype_fuel(unit_type(punit))) {
      int rate, f;
      rate = unit_type(punit)->move_rate / SINGLE_MOVE;
      f = ((punit->fuel) - 1);

      if ((punit->moves_left % SINGLE_MOVE) != 0) {
        if (punit->moves_left / SINGLE_MOVE > 0) {
          astr_add_line(astr, "%s: (%d)%d %d/%d", _("Moves"),
                        ((rate * f) + (punit->moves_left / SINGLE_MOVE)),
                        punit->moves_left / SINGLE_MOVE,
                        punit->moves_left % SINGLE_MOVE, SINGLE_MOVE);
        } else {
          astr_add_line(astr, "%s: (%d)%d/%d", _("Moves"),
                        ((rate * f) + (punit->moves_left / SINGLE_MOVE)),
                        punit->moves_left % SINGLE_MOVE, SINGLE_MOVE);
        }
      } else {
        astr_add_line(astr, "%s: (%d)%d", _("Moves"),
                      rate * f + punit->moves_left / SINGLE_MOVE,
                      punit->moves_left / SINGLE_MOVE);
      }
    } else {
      if ((punit->moves_left % SINGLE_MOVE) != 0) {
        if (punit->moves_left / SINGLE_MOVE > 0) {
          astr_add_line(astr, "%s: %d %d/%d", _("Moves"),
                        punit->moves_left / SINGLE_MOVE,
                        punit->moves_left % SINGLE_MOVE, SINGLE_MOVE);
        } else {
          astr_add_line(astr, "%s: %d/%d", _("Moves"),
                        punit->moves_left % SINGLE_MOVE, SINGLE_MOVE);
        }
      } else {
        astr_add_line(astr, "%s: %d", _("Moves"),
                      punit->moves_left / SINGLE_MOVE);
      }
    }
    break;
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_ROAD:
  case ACTIVITY_RAILROAD:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_SENTRY:
  case ACTIVITY_GOTO:
  case ACTIVITY_EXPLORE:
    astr_add_line(astr, "%s", get_activity_text(punit->activity));
    break;
  case ACTIVITY_PILLAGE:
    if (punit->activity_target == S_LAST) {
      astr_add_line(astr, "%s", get_activity_text(punit->activity));
    } else {
      bv_special pset;
      bv_bases bases;

      BV_CLR_ALL(pset);
      BV_SET(pset, punit->activity_target);
      BV_CLR_ALL(bases);
      if (0 <= punit->activity_base && punit->activity_base < base_count()) {
        BV_SET(bases, punit->activity_base);
      }
      astr_add_line(astr, "%s: %s", get_activity_text(punit->activity),
                    get_infrastructure_text(pset, bases));
    }
    break;
  case ACTIVITY_BASE:
    {
      struct base_type *pbase;
      pbase = base_by_number(punit->activity_base);
      astr_add_line(astr, "%s: %s", get_activity_text(punit->activity),
                    base_name_translation(pbase));
    }
    break;
  default:
    die("Unknown unit activity %d in unit_activity_text()",
        punit->activity);
  }
}

/**************************************************************************
  Append a line of text describing the unit's upkeep to the astring.

  NB: In the client it is assumed that this information is only available
  for units owned by the client's player; the caller must check this.
**************************************************************************/
void unit_upkeep_astr(const struct unit *punit, struct astring *astr)
{
  if (!punit || !astr) {
    return;
  }

  astr_add_line(astr, "%s %d/%d/%d", _("Food/Shield/Gold:"),
                punit->upkeep[O_FOOD], punit->upkeep[O_SHIELD],
                punit->upkeep[O_GOLD]);
}

/**************************************************************************
  Return the owner of the unit.
**************************************************************************/
struct player *unit_owner(const struct unit *punit)
{
  assert(NULL != punit);
  assert(NULL != punit->owner);
  return punit->owner;
}

/**************************************************************************
  Return the tile location of the unit.
  Not (yet) always used, mostly for debugging.
**************************************************************************/
struct tile *unit_tile(const struct unit *punit)
{
  assert(NULL != punit);
  return punit->tile;
}

/**************************************************************************
Returns true if the tile contains an allied unit and only allied units.
(ie, if your nation A is allied with B, and B is allied with C, a tile
containing units from B and C will return false)
**************************************************************************/
struct unit *is_allied_unit_tile(const struct tile *ptile,
				 const struct player *pplayer)
{
  struct unit *punit = NULL;

  unit_list_iterate(ptile->units, cunit) {
    if (pplayers_allied(pplayer, unit_owner(cunit)))
      punit = cunit;
    else
      return NULL;
  }
  unit_list_iterate_end;

  return punit;
}

/****************************************************************************
  Is there an enemy unit on this tile?  Returns the unit or NULL if none.

  This function is likely to fail if used at the client because the client
  doesn't see all units.  (Maybe it should be moved into the server code.)
****************************************************************************/
struct unit *is_enemy_unit_tile(const struct tile *ptile,
				const struct player *pplayer)
{
  unit_list_iterate(ptile->units, punit) {
    if (pplayers_at_war(unit_owner(punit), pplayer))
      return punit;
  } unit_list_iterate_end;

  return NULL;
}

/**************************************************************************
 is there an non-allied unit on this tile?
**************************************************************************/
struct unit *is_non_allied_unit_tile(const struct tile *ptile,
				     const struct player *pplayer)
{
  unit_list_iterate(ptile->units, punit) {
    if (!pplayers_allied(unit_owner(punit), pplayer))
      return punit;
  }
  unit_list_iterate_end;

  return NULL;
}

/**************************************************************************
 is there an unit we have peace or ceasefire with on this tile?
**************************************************************************/
struct unit *is_non_attack_unit_tile(const struct tile *ptile,
				     const struct player *pplayer)
{
  unit_list_iterate(ptile->units, punit) {
    if (pplayers_non_attack(unit_owner(punit), pplayer))
      return punit;
  }
  unit_list_iterate_end;

  return NULL;
}

/****************************************************************************
  Is there an occupying unit on this tile?

  Intended for both client and server; assumes that hiding units are not
  sent to client.  First check tile for known and seen.

  called by city_can_work_tile().
****************************************************************************/
struct unit *unit_occupies_tile(const struct tile *ptile,
				const struct player *pplayer)
{
  unit_list_iterate(ptile->units, punit) {
    if (!is_military_unit(punit)) {
      continue;
    }

    if (uclass_has_flag(unit_class(punit), UCF_DOESNT_OCCUPY_TILE)) {
      continue;
    }

    if (pplayers_at_war(unit_owner(punit), pplayer)) {
      return punit;
    }
  } unit_list_iterate_end;

  return NULL;
}

/**************************************************************************
  Is this square controlled by the pplayer?

  Here "is_my_zoc" means essentially a square which is *not* adjacent to an
  enemy unit on a land tile.

  Note this function only makes sense for ground units.

  Since this function is also used in the client, it has to deal with some
  client-specific features, like FoW and the fact that the client cannot 
  see units inside enemy cities.
**************************************************************************/
bool is_my_zoc(const struct player *pplayer, const struct tile *ptile0)
{
  square_iterate(ptile0, 1, ptile) {
    if (is_ocean_tile(ptile)) {
      continue;
    }
    if (is_non_allied_unit_tile(ptile, pplayer)) {
      /* Note: in the client, the above function will return NULL 
       * if there is a city there, even if the city is occupied */
      return FALSE;
    }
    
    if (!is_server()) {
      struct city *pcity = is_non_allied_city_tile(ptile, pplayer);

      if (pcity 
          && (pcity->client.occupied 
              || TILE_KNOWN_UNSEEN == tile_get_known(ptile, pplayer))) {
        /* If the city is fogged, we assume it's occupied */
        return FALSE;
      }
    }
  } square_iterate_end;

  return TRUE;
}

/**************************************************************************
  Takes into account unit class flag UCF_ZOC as well as IGZOC
**************************************************************************/
bool unit_type_really_ignores_zoc(const struct unit_type *punittype)
{
  return (!uclass_has_flag(utype_class(punittype), UCF_ZOC)
	  || utype_has_flag(punittype, F_IGZOC));
}

/**************************************************************************
An "aggressive" unit is a unit which may cause unhappiness
under a Republic or Democracy.
A unit is *not* aggressive if one or more of following is true:
- zero attack strength
- inside a city
- ground unit inside a fortress within 3 squares of a friendly city
**************************************************************************/
bool unit_being_aggressive(const struct unit *punit)
{
  if (!is_attack_unit(punit)) {
    return FALSE;
  }
  if (tile_city(punit->tile)) {
    return FALSE;
  }
  if (game.info.borders > 0
      && game.info.happyborders
      && tile_owner(punit->tile) == unit_owner(punit)) {
    return FALSE;
  }
  if (tile_has_base_flag_for_unit(punit->tile,
                                  unit_type(punit),
                                  BF_NOT_AGGRESSIVE)) {
    return !is_unit_near_a_friendly_city (punit);
  }
  
  return TRUE;
}

/**************************************************************************
  Returns true if given activity is some kind of building/cleaning.
**************************************************************************/
bool is_build_or_clean_activity(enum unit_activity activity)
{
  switch (activity) {
  case ACTIVITY_POLLUTION:
  case ACTIVITY_ROAD:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_RAILROAD:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_BASE:
    return TRUE;
  default:
    return FALSE;
  }
}

/**************************************************************************
  Create a virtual unit skeleton. pcity can be NULL, but then you need
  to set x, y and homecity yourself.
**************************************************************************/
struct unit *create_unit_virtual(struct player *pplayer, struct city *pcity,
                                 struct unit_type *punittype,
				 int veteran_level)
{
  struct unit *punit = fc_calloc(1, sizeof(*punit));

  /* It does not register the unit so the id is set to 0. */
  punit->id = IDENTITY_NUMBER_ZERO;

  CHECK_UNIT_TYPE(punittype); /* No untyped units! */
  punit->utype = punittype;

  assert(pplayer != NULL); /* No unowned units! */
  punit->owner = pplayer;

  if (pcity) {
    punit->tile = pcity->tile;
    punit->homecity = pcity->id;
  } else {
    punit->tile = NULL;
    punit->homecity = IDENTITY_NUMBER_ZERO;
  }
  memset(punit->upkeep, 0, O_LAST * sizeof(*punit->upkeep));
  punit->goto_tile = NULL;
  punit->veteran = veteran_level;
  /* A unit new and fresh ... */
  punit->debug = FALSE;
  punit->fuel = utype_fuel(unit_type(punit));
  punit->birth_turn = game.info.turn;
  punit->hp = unit_type(punit)->hp;
  punit->moves_left = unit_move_rate(punit);
  punit->moved = FALSE;
  punit->paradropped = FALSE;
  punit->done_moving = FALSE;
  punit->ai.done = FALSE;
  punit->ai.cur_pos = NULL;
  punit->ai.prev_pos = NULL;
  punit->ai.target = 0;
  punit->ai.hunted = 0;
  punit->ai.control = FALSE;
  punit->ai.ai_role = AIUNIT_NONE;
  punit->ai.ferryboat = 0;
  punit->ai.passenger = 0;
  punit->ai.bodyguard = 0;
  punit->ai.charge = 0;
  punit->transported_by = -1;
  punit->focus_status = FOCUS_AVAIL;
  punit->ord_map = 0;
  punit->ord_city = 0;
  set_unit_activity(punit, ACTIVITY_IDLE);
  punit->occupy = 0;
  punit->battlegroup = BATTLEGROUP_NONE;
  punit->client.colored = FALSE;
  punit->server.vision = NULL; /* No vision. */
  punit->has_orders = FALSE;

  return punit;
}

/**************************************************************************
  Free the memory used by virtual unit. By the time this function is
  called, you should already have unregistered it everywhere.
**************************************************************************/
void destroy_unit_virtual(struct unit *punit)
{
  free_unit_orders(punit);
  memset(punit, 0, sizeof(*punit)); /* ensure no pointers remain */
  free(punit);
}

/**************************************************************************
  Free and reset the unit's goto route (punit->pgr).  Only used by the
  server.
**************************************************************************/
void free_unit_orders(struct unit *punit)
{
  if (punit->has_orders) {
    punit->goto_tile = NULL;
    free(punit->orders.list);
    punit->orders.list = NULL;
  }
  punit->has_orders = FALSE;
}

/****************************************************************************
  Expensive function to check how many units are in the transport.
****************************************************************************/
int get_transporter_occupancy(const struct unit *ptrans)
{
  int occupied = 0;

  unit_list_iterate(ptrans->tile->units, pcargo) {
    if (pcargo->transported_by == ptrans->id) {
      occupied++;
    }
  } unit_list_iterate_end;

  return occupied;
}

/****************************************************************************
  Find a transporter at the given location for the unit.
****************************************************************************/
struct unit *find_transporter_for_unit(const struct unit *pcargo)
{
  struct tile *ptile = pcargo->tile;

  unit_list_iterate(ptile->units, ptrans) {
    if (can_unit_load(pcargo, ptrans)) {
      return ptrans;
    }
  } unit_list_iterate_end;

  return NULL;
}

/***************************************************************************
  Tests if the unit could be updated. Returns UR_OK if is this is
  possible.

  is_free should be set if the unit upgrade is "free" (e.g., Leonardo's).
  Otherwise money is needed and the unit must be in an owned city.

  Note that this function is strongly tied to unittools.c:upgrade_unit().
***************************************************************************/
enum unit_upgrade_result test_unit_upgrade(const struct unit *punit,
					   bool is_free)
{
  struct player *pplayer = unit_owner(punit);
  struct unit_type *to_unittype = can_upgrade_unittype(pplayer, unit_type(punit));
  struct city *pcity;
  int cost;

  if (!to_unittype) {
    return UR_NO_UNITTYPE;
  }

  if (!is_free) {
    cost = unit_upgrade_price(pplayer, unit_type(punit), to_unittype);
    if (pplayer->economic.gold < cost) {
      return UR_NO_MONEY;
    }

    pcity = tile_city(punit->tile);
    if (!pcity) {
      return UR_NOT_IN_CITY;
    }
    if (city_owner(pcity) != pplayer) {
      /* TODO: should upgrades in allied cities be possible? */
      return UR_NOT_CITY_OWNER;
    }
  }

  if (get_transporter_occupancy(punit) > to_unittype->transport_capacity) {
    /* TODO: allow transported units to be reassigned.  Check for
     * unit_class_transporter_capacity() here and make changes to
     * upgrade_unit. */
    return UR_NOT_ENOUGH_ROOM;
  }

  return UR_OK;
}

/**************************************************************************
  Tests if unit can be transformed.
**************************************************************************/
bool test_unit_transform(const struct unit *punit)
{
  return unit_type(punit)->transformed_to != NULL;
}

/**************************************************************************
  Find the result of trying to upgrade the unit, and a message that
  most callers can use directly.
**************************************************************************/
enum unit_upgrade_result get_unit_upgrade_info(char *buf, size_t bufsz,
					       const struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  enum unit_upgrade_result result = test_unit_upgrade(punit, FALSE);
  int upgrade_cost;
  struct unit_type *from_unittype = unit_type(punit);
  struct unit_type *to_unittype = can_upgrade_unittype(pplayer,
						  unit_type(punit));

  switch (result) {
  case UR_OK:
    upgrade_cost = unit_upgrade_price(pplayer, from_unittype, to_unittype);
    /* This message is targeted toward the GUI callers. */
    my_snprintf(buf, bufsz, _("Upgrade %s to %s for %d gold?\n"
			      "Treasury contains %d gold."),
		utype_name_translation(from_unittype),
		utype_name_translation(to_unittype),
		upgrade_cost, pplayer->economic.gold);
    break;
  case UR_NO_UNITTYPE:
    my_snprintf(buf, bufsz,
		_("Sorry, cannot upgrade %s (yet)."),
		utype_name_translation(from_unittype));
    break;
  case UR_NO_MONEY:
    upgrade_cost = unit_upgrade_price(pplayer, from_unittype, to_unittype);
    my_snprintf(buf, bufsz,
		_("Upgrading %s to %s costs %d gold.\n"
		  "Treasury contains %d gold."),
		utype_name_translation(from_unittype),
		utype_name_translation(to_unittype),
		upgrade_cost, pplayer->economic.gold);
    break;
  case UR_NOT_IN_CITY:
  case UR_NOT_CITY_OWNER:
    my_snprintf(buf, bufsz,
		_("You can only upgrade units in your cities."));
    break;
  case UR_NOT_ENOUGH_ROOM:
    my_snprintf(buf, bufsz,
		_("Upgrading this %s would strand units it transports."),
		utype_name_translation(from_unittype));
    break;
  }

  return result;
}

/**************************************************************************
  Does unit lose hitpoints each turn?
**************************************************************************/
bool is_losing_hp(const struct unit *punit)
{
  return unit_type_is_losing_hp(unit_owner(punit), unit_type(punit));
}

/**************************************************************************
  Does unit lose hitpoints each turn?
**************************************************************************/
bool unit_type_is_losing_hp(const struct player *pplayer,
                            const struct unit_type *punittype)
{
  return get_player_bonus(pplayer, EFT_UNIT_RECOVER)
    < (punittype->hp *
       utype_class(punittype)->hp_loss_pct / 100);
}

/**************************************************************************
  Check if unit with given id is still alive. Use this before using
  old unit pointers when unit might have dead.
**************************************************************************/
bool unit_alive(int id)
{
  /* Check if unit exist in game */
  if (game_find_unit_by_number(id)) {
    return TRUE;
  }

  return FALSE;
}

/**************************************************************************
  Return TRUE if this is a valid unit pointer but does not correspond to
  any unit that exists in the game.

  NB: A return value of FALSE implies that either the pointer is NULL or
  that the unit exists in the game.
**************************************************************************/
bool unit_is_virtual(const struct unit *punit)
{
  if (!punit) {
    return FALSE;
  }

  return punit != game_find_unit_by_number(punit->id);
}
