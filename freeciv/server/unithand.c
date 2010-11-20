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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"

/* common */
#include "city.h"
#include "combat.h"
#include "events.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"

/* ai */
#include "aiexplorer.h"
#include "aitools.h"

/* server */
#include "barbarian.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplomats.h"
#include "gotohand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "settlers.h"
#include "spacerace.h"
#include "srv_main.h"
#include "techtools.h"
#include "unittools.h"

#include "pf_tools.h"

#include "unithand.h"

static void city_add_or_build_error(struct player *pplayer,
				    struct unit *punit,
				    enum add_build_city_result res);
static void city_add_unit(struct player *pplayer, struct unit *punit);
static void city_build(struct player *pplayer, struct unit *punit,
		       char *name);
static void unit_activity_handling_targeted(struct unit *punit,
					    enum unit_activity new_activity,
					    enum tile_special_type new_target,
                                            Base_type_id base);
static void unit_activity_handling_base(struct unit *punit,
                                        Base_type_id base);
static bool base_handle_unit_establish_trade(struct player *pplayer, int unit_id, struct city *pcity_dest);
static bool unit_bombard(struct unit *punit, struct tile *ptile);

/**************************************************************************
...
**************************************************************************/
void handle_unit_airlift(struct player *pplayer, int unit_id, int city_id)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct city *pcity = game_find_city_by_number(city_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_airlift()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (NULL == pcity) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_airlift()"
	    " invalid city %d",
	    city_id);
    return;
  }

  (void) do_airline(punit, pcity);
}

/**************************************************************************
 Upgrade all units of a given type.
**************************************************************************/
void handle_unit_type_upgrade(struct player *pplayer, Unit_type_id uti)
{
  struct unit_type *to_unittype;
  struct unit_type *from_unittype = utype_by_number(uti);
  int number_of_upgraded_units = 0;

  if (NULL == from_unittype) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_type_upgrade()"
	    " invalid unit type %d",
	    uti);
    return;
  }

  to_unittype = can_upgrade_unittype(pplayer, from_unittype);
  if (!to_unittype) {
    notify_player(pplayer, NULL, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
		  _("Illegal packet, can't upgrade %s (yet)."),
		  utype_name_translation(from_unittype));
    return;
  }

  /* 
   * Try to upgrade units. The order we upgrade in is arbitrary (if
   * the player really cared they should have done it manually). 
   */
  conn_list_do_buffer(pplayer->connections);
  unit_list_iterate(pplayer->units, punit) {
    if (unit_type(punit) == from_unittype) {
      enum unit_upgrade_result result = test_unit_upgrade(punit, FALSE);

      if (result == UR_OK) {
	number_of_upgraded_units++;
	transform_unit(punit, to_unittype, FALSE);
      } else if (result == UR_NO_MONEY) {
	break;
      }
    }
  } unit_list_iterate_end;
  conn_list_do_unbuffer(pplayer->connections);

  /* Alert the player about what happened. */
  if (number_of_upgraded_units > 0) {
    const int cost = unit_upgrade_price(pplayer, from_unittype, to_unittype);
    notify_player(pplayer, NULL, E_UNIT_UPGRADED, FTC_SERVER_INFO, NULL,
		  _("%d %s upgraded to %s for %d gold."),
		  number_of_upgraded_units,
		  utype_name_translation(from_unittype),
		  utype_name_translation(to_unittype),
		  cost * number_of_upgraded_units);
    send_player_info(pplayer, pplayer);
  } else {
    notify_player(pplayer, NULL, E_UNIT_UPGRADED, FTC_SERVER_INFO, NULL,
		  _("No units could be upgraded."));
  }
}

/**************************************************************************
 Upgrade a single unit.
**************************************************************************/
void handle_unit_upgrade(struct player *pplayer, int unit_id)
{
  char buf[512];
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_upgrade()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (get_unit_upgrade_info(buf, sizeof(buf), punit) == UR_OK) {
    struct unit_type *from_unit = unit_type(punit);
    struct unit_type *to_unit = can_upgrade_unittype(pplayer, from_unit);
    int cost = unit_upgrade_price(pplayer, from_unit, to_unit);

    transform_unit(punit, to_unit, FALSE);
    send_player_info(pplayer, pplayer);
    notify_player(pplayer, punit->tile, E_UNIT_UPGRADED,
                  FTC_SERVER_INFO, NULL,
		  _("%s upgraded to %s for %d gold."), 
		  utype_name_translation(from_unit),
		  unit_link(punit),
		  cost);
  } else {
    notify_player(pplayer, punit->tile, E_UNIT_UPGRADED,
                  FTC_SERVER_INFO, NULL,
		  "%s", buf);
  }
}

/**************************************************************************
  Transform a single unit.
**************************************************************************/
void handle_unit_transform(struct player *pplayer, int unit_id)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct unit_type *to_type, *from_type;

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_transform() invalid unit %d",
	    unit_id);
    return;
  }

  from_type = unit_type(punit);
  to_type = from_type->transformed_to;

  if (test_unit_transform(punit)) {
    transform_unit(punit, to_type, TRUE);
    notify_player(pplayer, punit->tile, E_UNIT_UPGRADED,
                  FTC_SERVER_INFO, NULL,
                  _("%s transformed to %s."),
		  utype_name_translation(from_type),
		  utype_name_translation(to_type));
  } else {
    notify_player(pplayer, punit->tile, E_UNIT_UPGRADED,
                  FTC_SERVER_INFO, NULL,
		  "%s cannot transform.",
                  utype_name_translation(from_type));
  }
}

/**************************************************************************
  Tell the client the cost of bribing a unit, inciting a revolt, or
  any other parameters needed for action.

  Only send result back to the requesting connection, not all
  connections for that player.
**************************************************************************/
void handle_unit_diplomat_query(struct connection *pc,
				int diplomat_id,
				int target_id,
				int value,
				enum diplomat_actions action_type)
{
  struct player *pplayer = pc->playing;
  struct unit *pdiplomat = player_find_unit_by_id(pplayer, diplomat_id);
  struct unit *punit = game_find_unit_by_number(target_id);
  struct city *pcity = game_find_city_by_number(target_id);

  if (NULL == pdiplomat) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_diplomat_query()"
	    " invalid diplomat %d",
	    diplomat_id);
    return;
  }

  if (!unit_has_type_flag(pdiplomat, F_DIPLOMAT)) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_diplomat_query()"
	    " %s (%d) is not diplomat",
	    unit_rule_name(pdiplomat),
	    diplomat_id);
    return;
  }

  switch (action_type) {
  case DIPLOMAT_BRIBE:
    if (punit && diplomat_can_do_action(pdiplomat, DIPLOMAT_BRIBE,
					punit->tile)) {
      dsend_packet_unit_diplomat_answer(pc,
					diplomat_id, target_id,
					unit_bribe_cost(punit),
					action_type);
    }
    break;
  case DIPLOMAT_INCITE:
    if (pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_INCITE,
					pcity->tile)) {
      dsend_packet_unit_diplomat_answer(pc,
					diplomat_id, target_id,
					city_incite_cost(pplayer, pcity),
					action_type);
    }
    break;
  case DIPLOMAT_SABOTAGE:
    if (pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_SABOTAGE,
					pcity->tile)
     && unit_has_type_flag(pdiplomat, F_SPY)) {
      spy_send_sabotage_list(pc, pdiplomat, pcity);
    }
    break;
  default:
    /* Nothing */
    break;
  };
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_diplomat_action(struct player *pplayer,
				 int diplomat_id,
				 int target_id,
				 int value,
				 enum diplomat_actions action_type)
{
  struct unit *pdiplomat = player_find_unit_by_id(pplayer, diplomat_id);
  struct unit *punit = game_find_unit_by_number(target_id);
  struct city *pcity = game_find_city_by_number(target_id);

  if (NULL == pdiplomat) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_diplomat_action()"
	    " invalid diplomat %d",
	    diplomat_id);
    return;
  }

  if (!unit_has_type_flag(pdiplomat, F_DIPLOMAT)) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_diplomat_action()"
	    " %s (%d) is not diplomat",
	    unit_rule_name(pdiplomat),
	    diplomat_id);
    return;
  }

  if(pdiplomat->moves_left > 0) {
    switch(action_type) {
    case DIPLOMAT_BRIBE:
      if (punit && diplomat_can_do_action(pdiplomat, DIPLOMAT_BRIBE,
					  punit->tile)) {
	diplomat_bribe(pplayer, pdiplomat, punit);
      }
      break;
    case SPY_SABOTAGE_UNIT:
      if (punit && diplomat_can_do_action(pdiplomat, SPY_SABOTAGE_UNIT,
					  punit->tile)) {
	spy_sabotage_unit(pplayer, pdiplomat, punit);
      }
      break;
     case DIPLOMAT_SABOTAGE:
      if(pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_SABOTAGE,
					 pcity->tile)) {
	/* packet value is improvement ID + 1 (or some special codes) */
	diplomat_sabotage(pplayer, pdiplomat, pcity, value - 1);
      }
      break;
    case SPY_POISON:
      if(pcity && diplomat_can_do_action(pdiplomat, SPY_POISON,
					 pcity->tile)) {
	spy_poison(pplayer, pdiplomat, pcity);
      }
      break;
    case DIPLOMAT_INVESTIGATE:
      if(pcity && diplomat_can_do_action(pdiplomat,DIPLOMAT_INVESTIGATE,
					 pcity->tile)) {
	diplomat_investigate(pplayer, pdiplomat, pcity);
      }
      break;
    case DIPLOMAT_EMBASSY:
      if(pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_EMBASSY,
					 pcity->tile)) {
	diplomat_embassy(pplayer, pdiplomat, pcity);
      }
      break;
    case DIPLOMAT_INCITE:
      if(pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_INCITE,
					 pcity->tile)) {
	diplomat_incite(pplayer, pdiplomat, pcity);
      }
      break;
    case DIPLOMAT_MOVE:
      if(pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_MOVE,
					 pcity->tile)) {
	(void) unit_move_handling(pdiplomat, pcity->tile, FALSE, TRUE);
      }
      break;
    case DIPLOMAT_STEAL:
      if(pcity && diplomat_can_do_action(pdiplomat, DIPLOMAT_STEAL,
					 pcity->tile)) {
	/* packet value is technology ID (or some special codes) */
	diplomat_get_tech(pplayer, pdiplomat, pcity, value);
      }
      break;
    case DIPLOMAT_ANY_ACTION:
      /* Nothing */
      break;
    }
  }
}

/**************************************************************************
  Transfer a unit from one homecity to another. This new homecity must
  be valid for this unit.
**************************************************************************/
void unit_change_homecity_handling(struct unit *punit, struct city *new_pcity)
{
  struct city *old_pcity = game_find_city_by_number(punit->homecity);
  struct player *old_owner = unit_owner(punit);
  struct player *new_owner = city_owner(new_pcity);

  /* Calling this function when new_pcity is same as old_pcity should
   * be safe with current implementation, but it is not meant to
   * be used that way. */
  assert(new_pcity != old_pcity);

  if (old_owner != new_owner) {
    vision_clear_sight(punit->server.vision);
    vision_free(punit->server.vision);

    ai_reinit(punit);

    unit_list_unlink(old_owner->units, punit);
    unit_list_prepend(new_owner->units, punit);
    punit->owner = new_owner;

    punit->server.vision = vision_new(new_owner, punit->tile);
    unit_refresh_vision(punit);
  }

  /* Remove from old city first and add to new city only after that.
   * This is more robust in case old_city == new_city (currently
   * prohibited by assert in the beginning of the function).
   */
  if (old_pcity) {
    /* Even if unit is dead, we have to unlink unit pointer (punit). */
    unit_list_unlink(old_pcity->units_supported, punit);
    /* update unit upkeep */
    city_units_upkeep(old_pcity);
  }

  unit_list_prepend(new_pcity->units_supported, punit);

  /* update unit upkeep */
  city_units_upkeep(new_pcity);

  punit->homecity = new_pcity->id;
  if (old_owner == new_owner) {
    /* Only changed homecity only owner can see it. */
    send_unit_info(new_owner, punit);
  } else {
    /* Player owner changed, send info to all able to see it. */
    send_unit_info(NULL, punit);
  }

  city_refresh(new_pcity);
  send_city_info(new_owner, new_pcity);

  if (old_pcity) {
    assert(city_owner(old_pcity) == old_owner);
    city_refresh(old_pcity);
    send_city_info(old_owner, old_pcity);
  }

  assert(unit_owner(punit) == city_owner(new_pcity));
}

/**************************************************************************
  Change a unit's home city. The unit must be present in the city to 
  be set as its new home city.
**************************************************************************/
void handle_unit_change_homecity(struct player *pplayer, int unit_id,
				 int city_id)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct city *pcity = player_find_city_by_id(pplayer, city_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_change_homecity()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (pcity && can_unit_change_homecity_to(punit, pcity)) {
    unit_change_homecity_handling(punit, pcity);
  }
}

/**************************************************************************
  Disband a unit.  If its in a city, add 1/2 of the worth of the unit
  to the city's shield stock for the current production.
**************************************************************************/
void handle_unit_disband(struct player *pplayer, int unit_id)
{
  struct city *pcity;
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_disband()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (unit_has_type_flag(punit, F_UNDISBANDABLE)) {
    /* refuse to kill ourselves */
    notify_player(unit_owner(punit), punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s refuses to disband!"),
                  unit_link(punit));
    return;
  }

  pcity = tile_city(punit->tile);
  if (pcity) {
    /* If you disband inside a city, it gives some shields to that city.
     *
     * Note: Nowadays it's possible to disband unit in allied city and
     * your ally receives those shields. Should it be like this? Why not?
     * That's why we must use city_owner instead of pplayer -- Zamar */

    if (unit_has_type_flag(punit, F_HELP_WONDER)) {
      /* Count this just like a caravan that was added to a wonder.
       * However don't actually give the city the extra shields unless
       * they are building a wonder (but switching to a wonder later in
       * the turn will give the extra shields back). */
      pcity->caravan_shields += unit_build_shield_cost(punit);
      if (unit_can_help_build_wonder(punit, pcity)) {
	pcity->shield_stock += unit_build_shield_cost(punit);
      } else {
	pcity->shield_stock += unit_disband_shields(punit);
      }
    } else {
      pcity->shield_stock += unit_disband_shields(punit);
      /* If we change production later at this turn. No penalty is added. */
      pcity->disbanded_shields += unit_disband_shields(punit);
    }

    send_city_info(city_owner(pcity), pcity);
  }

  wipe_unit(punit);
}

/**************************************************************************
 This function assumes that there is a valid city at punit->(x,y) for
 certain values of test_add_build_or_city.  It should only be called
 after a call to unit_add_build_city_result, which does the
 consistency checking.
**************************************************************************/
static void city_add_or_build_error(struct player *pplayer,
				    struct unit *punit,
				    enum add_build_city_result res)
{
  /* Given that res came from test_unit_add_or_build_city, pcity will
     be non-null for all required status values. */
  struct city *pcity = tile_city(punit->tile);

  switch (res) {
  case AB_NOT_BUILD_LOC:
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("Can't place city here."));
    break;
  case AB_NOT_BUILD_UNIT:
    {
      const char *us = role_units_translations(F_CITIES);
      if (us) {
	notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("Only %s can build a city."),
                      us);
	free((void *) us);
      } else {
	notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("Can't build a city."));
      }
    }
    break;
  case AB_NOT_ADDABLE_UNIT:
    {
      const char *us = role_units_translations(F_ADD_TO_CITY);
      if (us) {
	notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("Only %s can add to a city."),
			 us);
	free((void *) us);
      } else {
	notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("Can't add to a city."));
      }
    }
    break;
  case AB_NO_MOVES_ADD:
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s unit has no moves left to add to %s."),
                  unit_link(punit),
                  city_link(pcity));
    break;
  case AB_NO_MOVES_BUILD:
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s unit has no moves left to build city."),
                  unit_link(punit));
    break;
  case AB_NOT_OWNER:
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  /* TRANS: <city> is owned by <nation>, cannot add <unit>. */
                  _("%s is owned by %s, cannot add %s."),
                  city_link(pcity),
                  nation_plural_for_player(city_owner(pcity)),
                  unit_link(punit));
    break;
  case AB_TOO_BIG:
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s is too big to add %s."),
                  city_link(pcity),
                  unit_link(punit));
    break;
  case AB_NO_SPACE:
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s needs an improvement to grow, so "
                    "you cannot add %s."),
                  city_link(pcity),
                  unit_link(punit));
    break;
  default:
    /* Shouldn't happen */
    freelog(LOG_ERROR, "Cannot add %s to %s for unknown reason (%d)",
	    unit_rule_name(punit),
	    city_name(pcity), res);
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("Can't add %s to %s."),
                  unit_link(punit),
                  city_link(pcity));
    break;
  }
}

/**************************************************************************
 This function assumes that there is a valid city at punit->(x,y) It
 should only be called after a call to a function like
 test_unit_add_or_build_city, which does the checking.
**************************************************************************/
static void city_add_unit(struct player *pplayer, struct unit *punit)
{
  struct city *pcity = tile_city(punit->tile);

  assert(unit_pop_value(punit) > 0);
  pcity->size += unit_pop_value(punit);
  /* Make the new people something, otherwise city fails the checks */
  pcity->specialists[DEFAULT_SPECIALIST] += unit_pop_value(punit);
  auto_arrange_workers(pcity);
  notify_player(pplayer, pcity->tile, E_CITY_BUILD,
                FTC_SERVER_INFO, NULL,
                _("%s added to aid %s in growing."),
                unit_link(punit),
                city_link(pcity));
  wipe_unit(punit);
  send_city_info(NULL, pcity);
}

/**************************************************************************
 This function assumes a certain level of consistency checking: There
 is no city under punit->(x,y), and that location is a valid one on
 which to build a city. It should only be called after a call to a
 function like test_unit_add_or_build_city, which does the checking.
**************************************************************************/
static void city_build(struct player *pplayer, struct unit *punit,
		       char *name)
{
  char message[1024];
  int size;

  if (!is_allowed_city_name(pplayer, name, message, sizeof(message))) {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  "%s", message);
    return;
  }

  create_city(pplayer, punit->tile, name);
  size = unit_type(punit)->city_size;
  if (size > 1) {
    struct city *pcity = tile_city(punit->tile);

    assert(pcity != NULL);

    city_change_size(pcity, size);
  }
  wipe_unit(punit);
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_build_city(struct player *pplayer, int unit_id, char *name)
{
  enum add_build_city_result res;
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  char *wname = city_name_suggestion(pplayer, punit->tile);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_build_city()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  res = test_unit_add_or_build_city(punit);

  if (res == AB_BUILD_OK) {
    city_build(pplayer, punit, wname);
  } else if (res == AB_ADD_OK) {
    city_add_unit(pplayer, punit);
  } else {
    city_add_or_build_error(pplayer, punit, res);
  }
}

/**************************************************************************
  Handle change in unit activity.
**************************************************************************/
void handle_unit_change_activity(struct player *pplayer, int unit_id,
				 enum unit_activity activity,
				 enum tile_special_type activity_target,
                                 Base_type_id activity_base)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_change_activity()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (punit->activity == activity
   && punit->activity_target == activity_target
   && punit->activity_base == activity_base
   && !punit->ai.control) {
    /* Treat change in ai.control as change in activity, so
     * idle autosettlers behave correctly when selected --dwp
     */
    return;
  }

  /* Remove city spot reservations for AI settlers on city founding
   * mission, before goto_tile reset. */
  if (punit->ai.ai_role != AIUNIT_NONE) {
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);
  }

  punit->ai.control = FALSE;
  punit->goto_tile = NULL;

  switch (activity) {
  case ACTIVITY_BASE:
    if (!base_by_number(activity_base)) {
      /* Illegal base type */
      return;
    }
    unit_activity_handling_base(punit, activity_base);
    break;

  case ACTIVITY_EXPLORE:
    unit_activity_handling_targeted(punit, activity, activity_target, -1);

    /* Exploring is handled here explicitly, since the player expects to
     * see an immediate response from setting a unit to auto-explore.
     * Handling it deeper in the code leads to some tricky recursive loops -
     * see PR#2631. */
    if (punit->moves_left > 0) {
      do_explore(punit);
    }
    break;

  default:
    unit_activity_handling_targeted(punit, activity, activity_target,
                                    activity_base);
    break;
  };
}

/**************************************************************************
  This function handles GOTO path requests from the client.
**************************************************************************/
void handle_goto_path_req(struct player *pplayer, int unit_id, int x, int y)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct tile *ptile = map_pos_to_tile(x, y);
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path;
  struct tile *old_tile;
  int i = 0;
  struct packet_goto_path p;

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_move()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (NULL == ptile) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_move()"
	    " invalid %s (%d) tile (%d,%d)",
	    unit_rule_name(punit),
	    unit_id,
	    x, y);
    return;
  }

  if (!is_player_phase(unit_owner(punit), game.info.phase)) {
    /* Client is out of sync, ignore */
    freelog(LOG_VERBOSE, "handle_unit_move()"
	    " invalid %s (%d) %s != phase %d",
	    unit_rule_name(punit),
	    unit_id,
	    nation_rule_name(nation_of_unit(punit)),
	    game.info.phase);
    return;
  }

  p.unit_id = punit->id;
  p.dest_x = x;
  p.dest_y = y;


  /* Use path-finding to find a goto path. */
  pft_fill_unit_parameter(&parameter, punit);
  pfm = pf_map_new(&parameter);
  path = pf_map_get_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (path) {

    p.length = path->length - 1;

    old_tile = path->positions[0].tile;

    /* Remove city spot reservations for AI settlers on city founding
     * mission, before goto_tile reset. */
    if (punit->ai.ai_role != AIUNIT_NONE) {
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    }

    punit->ai.control = FALSE;
    punit->goto_tile = NULL;

    free_unit_orders(punit);
    /* If we waited on a tile, reset punit->done_moving */
    punit->done_moving = (punit->moves_left <= 0);
    punit->has_orders = TRUE;
    punit->orders.length = path->length - 1;
    punit->orders.index = 0;
    punit->orders.repeat = FALSE;
    punit->orders.vigilant = FALSE;
    punit->orders.list
      = fc_malloc(path->length * sizeof(*(punit->orders.list)));
    for (i = 0; i < path->length - 1; i++) {
      struct tile *new_tile = path->positions[i + 1].tile;
      int dir;
      if (same_pos(new_tile, old_tile)) {
        dir = -1;
      } else {
        dir = get_direction_for_step(old_tile, new_tile);
      }
      old_tile = new_tile;
      p.dir[i] = dir;

    }
    pf_path_destroy(path);

    send_packet_goto_path(pplayer->current_conn, &p);

  } else {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("The unit can't go there."));
    return ;
  }

}

/**************************************************************************
  This has been rewritten to handle server-side gotos for the web client.
**************************************************************************/
void handle_unit_move(struct player *pplayer, int unit_id, int x, int y)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct tile *ptile = map_pos_to_tile(x, y);
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path;
  struct tile *old_tile;
  int i = 0;

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_move()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (NULL == ptile) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_move()"
	    " invalid %s (%d) tile (%d,%d)",
	    unit_rule_name(punit),
	    unit_id,
	    x, y);
    return;
  }

  if (!is_player_phase(unit_owner(punit), game.info.phase)) {
    /* Client is out of sync, ignore */
    freelog(LOG_VERBOSE, "handle_unit_move()"
	    " invalid %s (%d) %s != phase %d",
	    unit_rule_name(punit),
	    unit_id,
	    nation_rule_name(nation_of_unit(punit)),
	    game.info.phase);
    return;
  }

  /* Use path-finding to find a goto path. */
  pft_fill_unit_parameter(&parameter, punit);
  pfm = pf_map_new(&parameter);
  path = pf_map_get_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (path) {

    old_tile = path->positions[0].tile;

    /* Remove city spot reservations for AI settlers on city founding
     * mission, before goto_tile reset. */
    if (punit->ai.ai_role != AIUNIT_NONE) {
      ai_unit_new_role(punit, AIUNIT_NONE, NULL);
    }

    punit->ai.control = FALSE;
    punit->goto_tile = NULL;

    free_unit_orders(punit);
    /* If we waited on a tile, reset punit->done_moving */
    punit->done_moving = (punit->moves_left <= 0);
    punit->has_orders = TRUE;
    punit->orders.length = path->length - 1;
    punit->orders.index = 0;
    punit->orders.repeat = FALSE;
    punit->orders.vigilant = FALSE;
    punit->orders.list
      = fc_malloc(path->length * sizeof(*(punit->orders.list)));
    for (i = 0; i < path->length - 1; i++) {
      struct tile *new_tile = path->positions[i + 1].tile;

      if (same_pos(new_tile, old_tile)) {
        punit->orders.list[i].order = ORDER_FULL_MP;
        punit->orders.list[i].dir = -1;
      } else {
        punit->orders.list[i].order = ORDER_MOVE;
        punit->orders.list[i].dir = get_direction_for_step(old_tile, new_tile);
        punit->orders.list[i].activity = ACTIVITY_LAST;
        punit->orders.list[i].base = -1;
      }
      old_tile = new_tile;

    }

    punit->goto_tile = map_pos_to_tile(x, y);

   if (!is_player_phase(unit_owner(punit), game.info.phase)
      || execute_orders(punit)) {
    /* Looks like the unit survived. */
     send_unit_info(NULL, punit);
    }

    pf_path_destroy(path);
    return ;
  } else {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("The unit can't go there."));
    return ;
  }

}

/**************************************************************************
 Make sure everyone who can see combat does.
**************************************************************************/
static void see_combat(struct unit *pattacker, struct unit *pdefender)
{  
  struct packet_unit_short_info unit_att_short_packet, unit_def_short_packet;

  /* 
   * Special case for attacking/defending:
   * 
   * Normally the player doesn't get the information about the units inside a
   * city. However for attacking/defending the player has to know the unit of
   * the other side.  After the combat a remove_unit packet will be sent
   * to the client to tidy up.
   *
   * Note these packets must be sent out before unit_versus_unit is called,
   * so that the original unit stats (HP) will be sent.
   */
  package_short_unit(pattacker, &unit_att_short_packet, FALSE,
		     UNIT_INFO_IDENTITY, 0);
  package_short_unit(pdefender, &unit_def_short_packet, FALSE,
		     UNIT_INFO_IDENTITY, 0);
  players_iterate(other_player) {
    /* NOTE: this means the player can see combat between submarines even
     * if neither sub is visible.  See similar comment in send_combat. */
    if (map_is_known_and_seen(pattacker->tile, other_player, V_MAIN)
	|| map_is_known_and_seen(pdefender->tile, other_player, V_MAIN)) {
      if (!can_player_see_unit(other_player, pattacker)) {
	assert(other_player != unit_owner(pattacker));
	lsend_packet_unit_short_info(other_player->connections,
				     &unit_att_short_packet);
      }

      if (!can_player_see_unit(other_player, pdefender)) {
	assert(other_player != unit_owner(pdefender));
	lsend_packet_unit_short_info(other_player->connections,
				     &unit_def_short_packet);
      }
    }
  } players_iterate_end;
}

/**************************************************************************
 Send combat info to players.
**************************************************************************/
static void send_combat(struct unit *pattacker, struct unit *pdefender, 
			int veteran, int bombard)
{
  struct packet_unit_combat_info combat;

  combat.attacker_unit_id=pattacker->id;
  combat.defender_unit_id=pdefender->id;
  combat.attacker_hp=pattacker->hp;
  combat.defender_hp=pdefender->hp;
  combat.make_winner_veteran=veteran;

  players_iterate(other_player) {
    /* NOTE: this means the player can see combat between submarines even
     * if neither sub is visible.  See similar comment in see_combat. */
    if (map_is_known_and_seen(pattacker->tile, other_player, V_MAIN)
	|| map_is_known_and_seen(pdefender->tile, other_player, V_MAIN)) {
      lsend_packet_unit_combat_info(other_player->connections, &combat);

      /* 
       * Remove the client knowledge of the units.  This corresponds to the
       * send_packet_unit_short_info calls up above.
       */
      if (!can_player_see_unit(other_player, pattacker)) {
	unit_goes_out_of_sight(other_player, pattacker);
      }
      if (!can_player_see_unit(other_player, pdefender)) {
	unit_goes_out_of_sight(other_player, pdefender);
      }
    }
  } players_iterate_end;

  /* Send combat info to non-player observers as well.  They already know
   * about the unit so no unit_info is needed. */
  conn_list_iterate(game.est_connections, pconn) {
    if (NULL == pconn->playing && pconn->observer) {
      send_packet_unit_combat_info(pconn, &combat);
    }
  } conn_list_iterate_end;
}

/**************************************************************************
  This function assumes the bombard is legal. The calling function should
  have already made all necessary checks.
**************************************************************************/
static bool unit_bombard(struct unit *punit, struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(ptile);

  freelog(LOG_DEBUG, "Start bombard: %s %s to %d, %d.",
	  nation_rule_name(nation_of_player(pplayer)),
	  unit_rule_name(punit),
	  TILE_XY(ptile));

  unit_list_iterate_safe(ptile->units, pdefender) {

    /* Sanity checks */
    if (pplayers_non_attack(unit_owner(punit), unit_owner(pdefender))) {
      die("Trying to attack a unit with which you have peace "
	  "or cease-fire at %i, %i", TILE_XY(pdefender->tile));
    }
    if (pplayers_allied(unit_owner(punit), unit_owner(pdefender))
	&& !(unit_has_type_flag(punit, F_NUCLEAR) && punit == pdefender)) {
      die("Trying to attack a unit with which you have alliance at %i, %i",
	  TILE_XY(pdefender->tile));
    }

    if (is_unit_reachable_by_unit(pdefender, punit)
	|| pcity || tile_has_native_base(ptile, unit_type(pdefender))) {
      see_combat(punit, pdefender);

      unit_versus_unit(punit, pdefender, TRUE);

      send_combat(punit, pdefender, 0, 1);
  
      send_unit_info(NULL, pdefender);
    }

  } unit_list_iterate_safe_end;

  punit->moves_left = 0;
  
  if (pcity
      && pcity->size > 1
      && get_city_bonus(pcity, EFT_UNIT_NO_LOSE_POP) == 0
      && kills_citizen_after_attack(punit)) {
    city_reduce_size(pcity, 1, pplayer);
    city_refresh(pcity);
    send_city_info(NULL, pcity);
  }

  if (maybe_make_veteran(punit)) {
    notify_unit_experience(punit);
  }

  send_unit_info(NULL, punit);
  return TRUE;
}

/**************************************************************************
This function assumes the attack is legal. The calling function should have
already made all necessary checks.
**************************************************************************/
static void unit_attack_handling(struct unit *punit, struct unit *pdefender)
{
  char looser_link[MAX_LEN_NAME], winner_link[MAX_LEN_NAME];
  struct unit *plooser, *pwinner;
  struct city *pcity;
  int moves_used, def_moves_used; 
  int old_unit_vet, old_defender_vet, vet;
  int winner_id;
  struct tile *def_tile = pdefender->tile;
  struct player *pplayer = unit_owner(punit);
  
  freelog(LOG_DEBUG, "Start attack: %s %s against %s %s.",
	  nation_rule_name(nation_of_player(pplayer)),
	  unit_rule_name(punit), 
	  nation_rule_name(nation_of_unit(pdefender)),
	  unit_rule_name(pdefender));

  /* Sanity checks */
  if (pplayers_non_attack(pplayer, unit_owner(pdefender))) {
    die("Trying to attack a unit with which you have peace "
	"or cease-fire at %i, %i", TILE_XY(def_tile));
  }
  if (pplayers_allied(pplayer, unit_owner(pdefender))
      && !(unit_has_type_flag(punit, F_NUCLEAR) && punit == pdefender)) {
    die("Trying to attack a unit with which you have alliance at %i, %i",
	TILE_XY(def_tile));
  }

  if (unit_has_type_flag(punit, F_NUCLEAR)) {
    if ((pcity = sdi_try_defend(pplayer, def_tile))) {
      notify_player(pplayer, punit->tile, E_UNIT_LOST_ATT,
                    FTC_SERVER_INFO, NULL,
                    _("Your Nuclear missile was shot down by"
                      " SDI defences, what a waste."));
      notify_player(city_owner(pcity), def_tile, E_UNIT_WIN,
                    FTC_SERVER_INFO, NULL,
                    _("The nuclear attack on %s was avoided by"
                      " your SDI defense."), city_link(pcity));
      wipe_unit(punit);
      return;
    } 

    dlsend_packet_nuke_tile_info(game.est_connections,
				 def_tile->x, def_tile->y);

    wipe_unit(punit);
    do_nuclear_explosion(pplayer, def_tile);
    return;
  }
  moves_used = unit_move_rate(punit) - punit->moves_left;
  def_moves_used = unit_move_rate(pdefender) - pdefender->moves_left;

  see_combat(punit, pdefender);

  old_unit_vet = punit->veteran;
  old_defender_vet = pdefender->veteran;
  unit_versus_unit(punit, pdefender, FALSE);

  /* Adjust attackers moves_left _after_ unit_versus_unit() so that
   * the movement attack modifier is correct! --dwp
   *
   * For greater Civ2 compatibility (and game balance issues), we recompute 
   * the new total MP based on the HP the unit has left after being damaged, 
   * and subtract the MPs that had been used before the combat (plus the 
   * points used in the attack itself, for the attacker). -GJW, Glip
   */
  punit->moves_left = unit_move_rate(punit) - moves_used - SINGLE_MOVE;
  pdefender->moves_left = unit_move_rate(pdefender) - def_moves_used;
  
  if (punit->moves_left < 0) {
    punit->moves_left = 0;
  }
  if (pdefender->moves_left < 0) {
    pdefender->moves_left = 0;
  }

  if (punit->hp > 0
      && (pcity = tile_city(def_tile))
      && pcity->size > 1
      && get_city_bonus(pcity, EFT_UNIT_NO_LOSE_POP) == 0
      && kills_citizen_after_attack(punit)) {
    city_reduce_size(pcity, 1, pplayer);
    city_refresh(pcity);
    send_city_info(NULL, pcity);
  }
  if (unit_has_type_flag(punit, F_ONEATTACK)) 
    punit->moves_left = 0;
  pwinner = (punit->hp > 0) ? punit : pdefender;
  winner_id = pwinner->id;
  plooser = (pdefender->hp > 0) ? punit : pdefender;

  vet = (pwinner->veteran == ((punit->hp > 0) ? old_unit_vet :
	old_defender_vet)) ? 0 : 1;

  send_combat(punit, pdefender, vet, 0);

  /* N.B.: unit_link always returns the same pointer. */
  sz_strlcpy(looser_link, unit_link(plooser));
  sz_strlcpy(winner_link, unit_link(pwinner));

  if (punit == plooser) {
    /* The attacker lost */
    freelog(LOG_DEBUG, "Attacker lost: %s %s against %s %s.",
	    nation_rule_name(nation_of_player(pplayer)),
	    unit_rule_name(punit),
	    nation_rule_name(nation_of_unit(pdefender)),
	    unit_rule_name(pdefender));

    notify_player(unit_owner(pwinner), pwinner->tile, E_UNIT_WIN,
                  FTC_SERVER_INFO, NULL,
		  /* TRANS: "Your Cannon ... the Polish Destroyer." */
		  _("Your %s survived the pathetic attack from the %s %s."),
		  winner_link,
		  nation_adjective_for_player(unit_owner(plooser)),
		  looser_link);
    if (vet) {
      notify_unit_experience(pwinner);
    }
    notify_player(unit_owner(plooser), def_tile, E_UNIT_LOST_ATT,
                  FTC_SERVER_INFO, NULL,
		  /* TRANS: "... Cannon ... the Polish Destroyer." */
		  _("Your attacking %s failed against the %s %s!"),
		  looser_link,
		  nation_adjective_for_player(unit_owner(pwinner)),
		  winner_link);
    wipe_unit(plooser);
  } else {
    /* The defender lost, the attacker punit lives! */
    int winner_id = pwinner->id;

    freelog(LOG_DEBUG, "Defender lost: %s %s against %s %s.",
	    nation_rule_name(nation_of_player(pplayer)),
	    unit_rule_name(punit),
	    nation_rule_name(nation_of_unit(pdefender)),
	    unit_rule_name(pdefender));

    punit->moved = TRUE;	/* We moved */
    kill_unit(pwinner, plooser,
              vet && !uclass_has_flag(unit_class(punit), UCF_MISSILE));
    if (unit_alive(winner_id)) {
      if (uclass_has_flag(unit_class(pwinner), UCF_MISSILE)) {
        wipe_unit(pwinner);
        return;
      }
    } else {
      return;
    }
  }

  /* If attacker wins, and occupychance > 0, it might move in.  Don't move in
   * if there are enemy units in the tile (a fortress, city or air base with
   * multiple defenders and unstacked combat). Note that this could mean 
   * capturing (or destroying) a city. */

  if (pwinner == punit && myrand(100) < game.info.occupychance &&
      !is_non_allied_unit_tile(def_tile, pplayer)) {

    /* Hack: make sure the unit has enough moves_left for the move to succeed,
       and adjust moves_left to afterward (if successful). */

    int old_moves = punit->moves_left;
    int full_moves = unit_move_rate(punit);
    punit->moves_left = full_moves;
    if (unit_move_handling(punit, def_tile, FALSE, FALSE)) {
      punit->moves_left = old_moves - (full_moves - punit->moves_left);
      if (punit->moves_left < 0) {
	punit->moves_left = 0;
      }
    } else {
      punit->moves_left = old_moves;
    }
  }

  /* The attacker may have died for many reasons */
  if (game_find_unit_by_number(winner_id) != NULL) {
    send_unit_info(NULL, pwinner);
  }
}

/**************************************************************************
  see also aiunit could_unit_move_to_tile()
**************************************************************************/
static bool can_unit_move_to_tile_with_notify(struct unit *punit,
					      struct tile *dest_tile,
					      bool igzoc)
{
  struct tile *src_tile = punit->tile;
  enum unit_move_result reason =
      test_unit_move_to_tile(unit_type(punit), unit_owner(punit),
			     punit->activity,
			     src_tile, dest_tile, igzoc);

  switch (reason) {
  case MR_OK:
    return TRUE;

  case MR_BAD_TYPE_FOR_CITY_TAKE_OVER:
  {
    const char *units_str = role_units_translations(F_MARINES);
    if (units_str) {
      notify_player(unit_owner(punit), src_tile, E_BAD_COMMAND,
                    FTC_SERVER_INFO, NULL,
                    _("Only %s can attack from sea."),
                    units_str);
      free((void *) units_str);
    } else {
      notify_player(unit_owner(punit), src_tile, E_BAD_COMMAND,
                    FTC_SERVER_INFO, NULL,
                    _("Cannot attack from sea."));
    }
    break;
  }

  case MR_NO_WAR:
    notify_player(unit_owner(punit), src_tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("Cannot attack unless you declare war first."));
    break;

  case MR_ZOC:
    notify_player(unit_owner(punit), src_tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s can only move into your own zone of control."),
                  unit_link(punit));
    break;

  case MR_TRIREME:
    notify_player(unit_owner(punit), src_tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("%s cannot move that far from the coast line."),
                  unit_link(punit));
    break;

  case MR_PEACE:
    notify_player(unit_owner(punit), src_tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("Cannot invade unless you break peace with "
                    "%s first."),
                  player_name(tile_owner(dest_tile)));
    break;

  default:
    /* FIXME: need more explanations someday! */
    break;
  };

  return FALSE;
}

/**************************************************************************
  Will try to move to/attack the tile dest_x,dest_y.  Returns TRUE if this
  could be done, FALSE if it couldn't for some reason. Even if this
  returns TRUE, unit may have died upon arrival to new tile.
  
  'igzoc' means ignore ZOC rules - not necessary for igzoc units etc, but
  done in some special cases (moving barbarians out of initial hut).
  Should normally be FALSE.

  'move_diplomat_city' is another special case which should normally be
  FALSE.  If TRUE, try to move diplomat (or spy) into city (should be
  allied) instead of telling client to popup diplomat/spy dialog.

  FIXME: This function needs a good cleaning.
**************************************************************************/
bool unit_move_handling(struct unit *punit, struct tile *pdesttile,
                        bool igzoc, bool move_diplomat_city)
{
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(pdesttile);

  /*** Phase 1: Basic checks ***/

  /* this occurs often during lag, and to the AI due to some quirks -- Syela */
  if (!is_tiles_adjacent(punit->tile, pdesttile)) {
    freelog(LOG_DEBUG, "tiles not adjacent in move request");
    return FALSE;
  }


  if (punit->moves_left<=0)  {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("This unit has no moves left."));
    return FALSE;
  }

  /*** Phase 2: Special abilities checks ***/

  /* Caravans.  If city is allied (inc. ours) we would have a popup
   * asking if we are moving on. */
  if (unit_has_type_flag(punit, F_TRADE_ROUTE) && pcity
      && !pplayers_allied(city_owner(pcity), pplayer) ) {
    return base_handle_unit_establish_trade(pplayer, punit->id, pcity);
  }

  /* Diplomats. Pop up a diplomat action dialog in the client.  
   * If the AI has used a goto to send a diplomat to a target do not 
   * pop up a dialog in the client.  
   * For allied cities, keep moving if move_diplomat_city tells us to, 
   * or if the unit is on goto and the city is not the final destination. */
  if (is_diplomat_unit(punit)) {
    struct unit *target = is_non_allied_unit_tile(pdesttile, pplayer);

    if (target || is_non_allied_city_tile(pdesttile, pplayer)
        || !move_diplomat_city) {
      if (is_diplomat_action_available(punit, DIPLOMAT_ANY_ACTION,
				       pdesttile)) {
	int target_id = 0;
        
        if (pplayer->ai_data.control) {
          return FALSE;
        }
        
        /* If we didn't send_unit_info the client would sometimes
         * think that the diplomat didn't have any moves left and so
         * don't pop up the box.  (We are in the middle of the unit
         * restore cycle when doing goto's, and the unit's movepoints
         * have been restored, but we only send the unit info at the
         * end of the function.) */
        send_unit_info(pplayer, punit);
        
        /* if is_diplomat_action_available() then there must be 
         * a city or a unit */
        if (pcity) {
          target_id = pcity->id;
        } else if (target) {
          target_id = target->id;
        } else {
          die("Bug in unithand.c: no diplomat target.");
        }
	dlsend_packet_unit_diplomat_answer(player_reply_dest(pplayer),
					   punit->id, target_id,
					   0, DIPLOMAT_MOVE);
        return FALSE;
      } else if (!can_unit_move_to_tile(punit, pdesttile, igzoc)) {
        if (can_unit_exist_at_tile(punit, punit->tile)) {
          notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                        FTC_SERVER_INFO, NULL,
                        _("No diplomat action possible."));
        } else {
          notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                        FTC_SERVER_INFO, NULL,
                        _("Unit cannot perform diplomatic action from %s."),
                        terrain_name_translation(tile_terrain(punit->tile)));
        }
        return FALSE;
      }
    }
  }

  /*** Phase 3: Is it attack? ***/

  if (is_non_allied_unit_tile(pdesttile, pplayer) 
      || is_non_allied_city_tile(pdesttile, pplayer)) {
    struct unit *victim = NULL;

    /* We can attack ONLY in enemy cities */
    if ((pcity && !pplayers_at_war(city_owner(pcity), pplayer))
        || (victim = is_non_attack_unit_tile(pdesttile, pplayer))) {
      notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                    FTC_SERVER_INFO, NULL,
                    _("You must declare war on %s first.  Try using "
                      "players dialog."),
                    victim == NULL
                    ? player_name(city_owner(pcity))
                    : player_name(unit_owner(victim)));
      return FALSE;
    }

    /* Are we a bombarder? */
    if (unit_has_type_flag(punit, F_BOMBARDER)) {
      /* Only land can be bombarded, if the target is on ocean, fall
       * through to attack. */
      if (!is_ocean_tile(pdesttile)) {
	if (can_unit_bombard(punit)) {
	  unit_bombard(punit, pdesttile);
	  return TRUE;
	} else {
	  notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                        FTC_SERVER_INFO, NULL,
                        _("This unit is being transported, and"
                          " so cannot bombard."));
	  return FALSE;
	}
      }
    }


    /* The attack is legal wrt the alliances */
    victim = get_defender(punit, pdesttile);

    if (victim) {
      /* Must be physically able to attack EVERY unit there */
      if (!can_unit_attack_all_at_tile(punit, pdesttile)) {
        notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("You can't attack there."));
        return FALSE;
      }
      
      unit_attack_handling(punit, victim);
      return TRUE;
    } else {
      assert(is_enemy_city_tile(pdesttile, pplayer) != NULL);

      if (unit_has_type_flag(punit, F_NUCLEAR)) {
        if (move_unit(punit, pcity->tile, 0)) {
          /* Survived dangers of moving */
          unit_attack_handling(punit, punit); /* Boom! */
        }
        return TRUE;
      }

      /* If there is an enemy city it is empty.
       * If not it would have been caught in the attack case. 
       * FIXME: Move this check into test_unit_move_tile */
      if (!COULD_OCCUPY(punit)) {
        notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("This type of troops cannot take over a city."));
        return FALSE;
      }
      /* Taking over a city is considered a move, so fall through */
    }
  } 

  /*** Phase 4: OK now move the unit ***/

  /* We cannot move a transport into a tile that holds
   * units or cities not allied with all of our cargo. */
  if (get_transporter_capacity(punit) > 0) {
    unit_list_iterate(punit->tile->units, pcargo) {
      if (pcargo->transported_by == punit->id
          && (is_non_allied_unit_tile(pdesttile, unit_owner(pcargo))
              || is_non_allied_city_tile(pdesttile, unit_owner(pcargo)))) {
         notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                       FTC_SERVER_INFO, NULL,
                       _("A transported unit is not allied to all "
                         "units or city on target tile."));
         return FALSE;
      }
    } unit_list_iterate_end;
  }

  if (can_unit_move_to_tile_with_notify(punit, pdesttile, igzoc)) {
    int move_cost = map_move_cost_unit(punit, pdesttile);

    move_unit(punit, pdesttile, move_cost);

    return TRUE;
  } else {
    return FALSE;
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_help_build_wonder(struct player *pplayer, int unit_id)
{
  const char *text;
  struct city *pcity_dest;
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_help_build_wonder()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (!unit_has_type_flag(punit, F_HELP_WONDER)) {
    return;
  }
  pcity_dest = tile_city(punit->tile);
  
  if (!pcity_dest || !unit_can_help_build_wonder(punit, pcity_dest)) {
    return;
  }

  pcity_dest->shield_stock += unit_build_shield_cost(punit);
  pcity_dest->caravan_shields += unit_build_shield_cost(punit);

  conn_list_do_buffer(pplayer->connections);

  if (build_points_left(pcity_dest) >= 0) {
    text = _("Your %s helps build the %s in %s (%d remaining).");
  } else {
    text = _("Your %s helps build the %s in %s (%d surplus).");
  }
  notify_player(pplayer, pcity_dest->tile, E_CARAVAN_ACTION,
                FTC_SERVER_INFO, NULL,
                text, /* Must match arguments below. */
                unit_link(punit),
                improvement_name_translation(pcity_dest->production.value.building),
                city_link(pcity_dest), 
                abs(build_points_left(pcity_dest)));

  wipe_unit(punit);
  send_player_info(pplayer, pplayer);
  send_city_info(pplayer, pcity_dest);
  conn_list_do_unbuffer(pplayer->connections);
}

/**************************************************************************
...
**************************************************************************/
static bool base_handle_unit_establish_trade(struct player *pplayer, int unit_id, struct city *pcity_dest)
{
  char homecity_link[MAX_LEN_NAME], destcity_link[MAX_LEN_NAME];
  char punit_link[MAX_LEN_NAME];
  int revenue, i;
  bool can_establish, home_full = FALSE, dest_full = FALSE;
  struct city *pcity_homecity; 
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct city *pcity_out_of_home = NULL, *pcity_out_of_dest = NULL;

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "base_handle_unit_establish_trade()"
	    " invalid unit %d",
	    unit_id);
    return FALSE;
  }

  if (!unit_has_type_flag(punit, F_TRADE_ROUTE)) {
    return FALSE;
  }

  /* if no destination city is passed in,
   *  check whether the unit is already in the city */
  if (!pcity_dest) { 
    pcity_dest = tile_city(punit->tile);
  }

  if (!pcity_dest) {
    return FALSE;
  }

  pcity_homecity = player_find_city_by_id(pplayer, punit->homecity);

  if (!pcity_homecity) {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("Sorry, your %s cannot establish"
                    " a trade route because it has no home city"),
                  unit_link(punit));
    return FALSE;
   
  }

  sz_strlcpy(homecity_link, city_link(pcity_homecity));
  sz_strlcpy(destcity_link, city_link(pcity_dest));
  sz_strlcpy(punit_link, unit_link(punit));

  if (!can_cities_trade(pcity_homecity, pcity_dest)) {
    notify_player(pplayer, pcity_dest->tile, E_BAD_COMMAND,
                  FTC_SERVER_INFO, NULL,
                  _("Sorry, your %s cannot establish"
                    " a trade route between %s and %s"),
                  punit_link,
                  homecity_link,
                  destcity_link);
    return FALSE;
  }
  
  /* This part of code works like can_establish_trade_route, except
   * that we actually do the action of making the trade route. */

  /* If we can't make a new trade route we can still get the trade bonus. */
  can_establish = !have_cities_trade_route(pcity_homecity, pcity_dest);
    
  if (can_establish) {
    home_full = (city_num_trade_routes(pcity_homecity) == NUM_TRADEROUTES);
    dest_full = (city_num_trade_routes(pcity_dest) == NUM_TRADEROUTES);
  }
  
  if (home_full || dest_full) {
    int slot, trade = trade_between_cities(pcity_homecity, pcity_dest);

    /* See if there's a trade route we can cancel at the home city. */
    if (home_full) {
      if (get_city_min_trade_route(pcity_homecity, &slot) < trade) {
	pcity_out_of_home = game_find_city_by_number(pcity_homecity->trade[slot]);
	assert(pcity_out_of_home != NULL);
      } else {
	notify_player(pplayer, pcity_dest->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
		     _("Sorry, your %s cannot establish"
		       " a trade route here!"),
		       punit_link);
        notify_player(pplayer, pcity_dest->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("      The city of %s already has %d "
                        "better trade routes!"),
                      homecity_link,
                      NUM_TRADEROUTES);
	can_establish = FALSE;
      }
    }
    
    /* See if there's a trade route we can cancel at the dest city. */
    if (can_establish && dest_full) {
      if (get_city_min_trade_route(pcity_dest, &slot) < trade) {
	pcity_out_of_dest = game_find_city_by_number(pcity_dest->trade[slot]);
	assert(pcity_out_of_dest != NULL);
      } else {
	notify_player(pplayer, pcity_dest->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("Sorry, your %s cannot establish"
                        " a trade route here!"),
                      punit_link);
        notify_player(pplayer, pcity_dest->tile, E_BAD_COMMAND,
                      FTC_SERVER_INFO, NULL,
                      _("      The city of %s already has %d "
                        "better trade routes!"),
                      destcity_link,
                      NUM_TRADEROUTES);
	can_establish = FALSE;
      }
    }

    /* Now cancel the trade route from the home city. */
    if (can_establish && pcity_out_of_home) {
      remove_trade_route(pcity_homecity, pcity_out_of_home);
      notify_player(city_owner(pcity_out_of_home),
                    pcity_out_of_home->tile, E_CARAVAN_ACTION,
                    FTC_SERVER_INFO, NULL,
                    _("Sorry, %s has canceled the trade route "
                      "from %s to your city %s."),
                    player_name(city_owner(pcity_homecity)),
                    homecity_link,
                    city_link(pcity_out_of_home));
    }

    /* And the same for the dest city. */
    if (can_establish && pcity_out_of_dest) {
      remove_trade_route(pcity_dest, pcity_out_of_dest);
      notify_player(city_owner(pcity_out_of_dest),
                    pcity_out_of_dest->tile, E_CARAVAN_ACTION,
                    FTC_SERVER_INFO, NULL,
                    _("Sorry, %s has canceled the trade route "
                      "from %s to your city %s."),
                    player_name(city_owner(pcity_dest)),
                    destcity_link,
                    city_link(pcity_out_of_dest));
    }
  }
  
  revenue = get_caravan_enter_city_trade_bonus(pcity_homecity, pcity_dest);
  if (can_establish) {
    /* establish traderoute */
    for (i = 0; i < NUM_TRADEROUTES; i++) {
      if (pcity_homecity->trade[i] == 0) {
        pcity_homecity->trade[i] = pcity_dest->id;
        break;
      }
    }
    assert(i < NUM_TRADEROUTES);
  
    for (i = 0; i < NUM_TRADEROUTES; i++) {
      if (pcity_dest->trade[i] == 0) {
        pcity_dest->trade[i] = pcity_homecity->id;
        break;
      }
    }
    assert(i < NUM_TRADEROUTES);
  } else {
    /* enter marketplace */
    revenue = (revenue + 2) / 3;
  }
  
  conn_list_do_buffer(pplayer->connections);
  notify_player(pplayer, pcity_dest->tile, E_CARAVAN_ACTION,
                FTC_SERVER_INFO, NULL,
                _("Your %s from %s has arrived in %s,"
                  " and revenues amount to %d in gold and research."),
                punit_link,
                homecity_link,
                destcity_link,
                revenue);
  wipe_unit(punit);
  pplayer->economic.gold += revenue;
  update_tech(pplayer, revenue);

  /* Inform everyone about tech changes */
  send_player_info(pplayer, NULL);
  
  if (can_establish) {
    /* Refresh the cities. */
    city_refresh(pcity_homecity);
    city_refresh(pcity_dest);
    if (pcity_out_of_home) {
      city_refresh(pcity_out_of_home);
    }
    if (pcity_out_of_dest) {
      city_refresh(pcity_out_of_dest);
    }
  
    /* Notify the owners of the cities. */
    send_city_info(pplayer, pcity_homecity);
    send_city_info(city_owner(pcity_dest), pcity_dest);
    if(pcity_out_of_home) {
      send_city_info(city_owner(pcity_out_of_home), pcity_out_of_home);
    }
    if(pcity_out_of_dest) {
      send_city_info(city_owner(pcity_out_of_dest), pcity_out_of_dest);
    }

    /* Notify each player about the other cities so that they know about
     * the tile_trade value. */
    if (pplayer != city_owner(pcity_dest)) {
      send_city_info(city_owner(pcity_dest), pcity_homecity);
      send_city_info(pplayer, pcity_dest);
    }

    if (pcity_out_of_home) {
      if (city_owner(pcity_dest) != city_owner(pcity_out_of_home)) {
        send_city_info(city_owner(pcity_dest), pcity_out_of_home);
	 send_city_info(city_owner(pcity_out_of_home), pcity_dest);
      }
      if (pplayer != city_owner(pcity_out_of_home)) {
        send_city_info(pplayer, pcity_out_of_home);
	 send_city_info(city_owner(pcity_out_of_home), pcity_homecity);
      }
      if (pcity_out_of_dest && city_owner(pcity_out_of_home) !=
					city_owner(pcity_out_of_dest)) {
	 send_city_info(city_owner(pcity_out_of_home), pcity_out_of_dest);
      }
    }

    if (pcity_out_of_dest) {
      if (city_owner(pcity_dest) != city_owner(pcity_out_of_dest)) {
        send_city_info(city_owner(pcity_dest), pcity_out_of_dest);
	 send_city_info(city_owner(pcity_out_of_dest), pcity_dest);
      }
      if (pplayer != city_owner(pcity_out_of_dest)) {
	 send_city_info(pplayer, pcity_out_of_dest);
	 send_city_info(city_owner(pcity_out_of_dest), pcity_homecity);
      }
      if (pcity_out_of_home && city_owner(pcity_out_of_home) !=
					city_owner(pcity_out_of_dest)) {
	 send_city_info(city_owner(pcity_out_of_dest), pcity_out_of_home);
      }
    }
  }
  
  /* The research has changed, we have to update all
   * players sharing it */
  players_iterate(aplayer) {
    if (!players_on_same_team(pplayer, aplayer)) {
      continue;
    }
    send_player_info(aplayer, aplayer);
  } players_iterate_end;
  conn_list_do_unbuffer(pplayer->connections);
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_establish_trade(struct player *pplayer, int unit_id)
{
  (void) base_handle_unit_establish_trade(pplayer, unit_id, NULL);
}

/**************************************************************************
  Assign the unit to the battlegroup.

  Battlegroups are handled entirely by the client, so all we have to
  do here is save the battlegroup ID so that it'll be persistent.
**************************************************************************/
void handle_unit_battlegroup(struct player *pplayer,
			     int unit_id, int battlegroup)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_battlegroup()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  punit->battlegroup = CLIP(-1, battlegroup, MAX_NUM_BATTLEGROUPS);
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_autosettlers(struct player *pplayer, int unit_id)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_autosettlers()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (!can_unit_do_autosettlers(punit))
    return;

  punit->ai.control = TRUE;
  send_unit_info(pplayer, punit);
}

/**************************************************************************
...
**************************************************************************/
static void unit_activity_dependencies(struct unit *punit,
				       enum unit_activity old_activity,
				       enum tile_special_type old_target)
{
  switch (punit->activity) {
  case ACTIVITY_IDLE:
    switch (old_activity) {
    case ACTIVITY_PILLAGE: 
      {
        enum tile_special_type prereq =
	  get_infrastructure_prereq(old_target);
        if (prereq != S_LAST) {
          unit_list_iterate (punit->tile->units, punit2)
            if ((punit2->activity == ACTIVITY_PILLAGE) &&
                (punit2->activity_target == prereq)) {
              set_unit_activity(punit2, ACTIVITY_IDLE);
              send_unit_info(NULL, punit2);
            }
          unit_list_iterate_end;
        }
        break;
      }
    case ACTIVITY_EXPLORE:
      /* Restore unit's control status */
      punit->ai.control = FALSE;
      break;
    default: 
      ; /* do nothing */
    }
    break;
  case ACTIVITY_EXPLORE:
    punit->ai.control = TRUE;
    set_unit_activity(punit, ACTIVITY_EXPLORE);
    send_unit_info(NULL, punit);
    break;
  default:
    /* do nothing */
    break;
  }
}

/**************************************************************************
...
**************************************************************************/
void unit_activity_handling(struct unit *punit,
                            enum unit_activity new_activity)
{
  if (can_unit_do_activity(punit, new_activity)) {
    enum unit_activity old_activity = punit->activity;
    enum tile_special_type old_target = punit->activity_target;

    free_unit_orders(punit);
    set_unit_activity(punit, new_activity);
    send_unit_info(NULL, punit);
    unit_activity_dependencies(punit, old_activity, old_target);
  }
}

/**************************************************************************
  Handle request for targeted activity.
**************************************************************************/
static void unit_activity_handling_targeted(struct unit *punit,
					    enum unit_activity new_activity,
					    enum tile_special_type new_target,
                                            Base_type_id base)
{
  if (can_unit_do_activity_targeted(punit, new_activity, new_target,
                                    -1)) {
    enum unit_activity old_activity = punit->activity;
    enum tile_special_type old_target = punit->activity_target;

    free_unit_orders(punit);
    set_unit_activity_targeted(punit, new_activity, new_target, base);
    send_unit_info(NULL, punit);    
    unit_activity_dependencies(punit, old_activity, old_target);
  }
}

/**************************************************************************
  Handle request for military base building.
**************************************************************************/
static void unit_activity_handling_base(struct unit *punit,
                                        Base_type_id base)
{
  if (can_unit_do_activity_base(punit, base)) {
    enum unit_activity old_activity = punit->activity;
    enum tile_special_type old_target = punit->activity_target;

    free_unit_orders(punit);
    set_unit_activity_base(punit, base);
    send_unit_info(NULL, punit);
    unit_activity_dependencies(punit, old_activity, old_target);
  }
}

/****************************************************************************
  Handle a client request to load the given unit into the given transporter.
****************************************************************************/
void handle_unit_load(struct player *pplayer, int cargo_id, int trans_id)
{
  struct unit *pcargo = player_find_unit_by_id(pplayer, cargo_id);
  struct unit *ptrans = game_find_unit_by_number(trans_id);

  if (NULL == pcargo) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_load()"
	    " invalid cargo %d",
	    cargo_id);
    return;
  }

  if (NULL == ptrans) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_load()"
	    " invalid transport %d",
	    trans_id);
    return;
  }

  /* A player may only load their units, but they may be loaded into
   * other player's transporters, depending on the rules in
   * can_unit_load(). */
  if (!can_unit_load(pcargo, ptrans)) {
    return;
  }

  /* Load the unit and send out info to clients. */
  load_unit_onto_transporter(pcargo, ptrans);
}

/****************************************************************************
  Handle a client request to unload the given unit from the given
  transporter.
****************************************************************************/
void handle_unit_unload(struct player *pplayer, int cargo_id, int trans_id)
{
  struct unit *pcargo = game_find_unit_by_number(cargo_id);
  struct unit *ptrans = game_find_unit_by_number(trans_id);

  if (NULL == pcargo) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_unload()"
	    " invalid cargo %d",
	    cargo_id);
    return;
  }

  if (NULL == ptrans) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_unload()"
	    " invalid transport %d",
	    trans_id);
    return;
  }

  /* You are allowed to unload a unit if it is yours or if the transporter
   * is yours. */
  if (unit_owner(pcargo) != pplayer && unit_owner(ptrans) != pplayer) {
    return;
  }

  if (!can_unit_unload(pcargo, ptrans)) {
    return;
  }

  if (!can_unit_survive_at_tile(pcargo, pcargo->tile)) {
    return;
  }

  /* Unload the unit and send out info to clients. */
  unload_unit_from_transporter(pcargo);
}

/**************************************************************************
Explode nuclear at a tile without enemy units
**************************************************************************/
void handle_unit_nuke(struct player *pplayer, int unit_id)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_nuke()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  unit_attack_handling(punit, punit);
}

/**************************************************************************
...
**************************************************************************/
void handle_unit_paradrop_to(struct player *pplayer, int unit_id, int x,
			     int y)
{
  struct unit *punit = player_find_unit_by_id(pplayer, unit_id);
  struct tile *ptile = map_pos_to_tile(x, y);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_paradrop_to()"
	    " invalid unit %d",
	    unit_id);
    return;
  }

  if (NULL == ptile) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_paradrop_to()"
	    " invalid %s (%d) tile (%d,%d)",
	    unit_rule_name(punit),
	    unit_id,
	    x, y);
    return;
  }

  (void) do_paradrop(punit, ptile);
}

/**************************************************************************
Receives route packages.
**************************************************************************/
void handle_unit_orders(struct player *pplayer,
			struct packet_unit_orders *packet)
{
  int i;
  struct unit *punit = player_find_unit_by_id(pplayer, packet->unit_id);
  struct tile *src_tile = map_pos_to_tile(packet->src_x, packet->src_y);

  if (NULL == punit) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_orders()"
	    " invalid unit %d",
	    packet->unit_id);
    return;
  }

  if (0 > packet->length || MAX_LEN_ROUTE < packet->length) {
    /* Shouldn't happen */
    freelog(LOG_ERROR, "handle_unit_orders()"
	    " invalid %s (%d) packet length %d (max %d)",
	    unit_rule_name(punit),
	    packet->unit_id,
	    packet->length,
	    MAX_LEN_ROUTE);
    return;
  }

  if (ACTIVITY_IDLE != punit->activity) {
    freelog(LOG_ERROR, "handle_unit_orders()"
	    " invalid %s (%d) activity %d (should be %d)",
	    unit_rule_name(punit),
	    packet->unit_id,
	    punit->activity,
	    ACTIVITY_IDLE);
    return;
  }

  if (src_tile != punit->tile) {
    /* Failed sanity check.  Usually this happens if the orders were sent
     * in the previous turn, and the client thought the unit was in a
     * different position than it's actually in.  The easy solution is to
     * discard the packet.  We don't send an error message to the client
     * here (though maybe we should?). */
    freelog(LOG_DEBUG, "handle_unit_orders()"
	    " invalid %s (%d) tile (%d,%d != %d,%d)",
	    unit_rule_name(punit),
	    packet->unit_id,
	    packet->src_x, packet->src_y,
	    TILE_XY(punit->tile));
    return;
  }

  for (i = 0; i < packet->length; i++) {
    if (packet->orders[i] < 0 || packet->orders[i] > ORDER_LAST) {
      packet->orders[i] = ORDER_LAST;
    }
    switch (packet->orders[i]) {
    case ORDER_MOVE:
      if (!is_valid_dir(packet->dir[i])) {
	return;
      }
      break;
    case ORDER_ACTIVITY:
      switch (packet->activity[i]) {
      case ACTIVITY_POLLUTION:
      case ACTIVITY_ROAD:
      case ACTIVITY_MINE:
      case ACTIVITY_IRRIGATE:
      case ACTIVITY_FORTRESS:
      case ACTIVITY_RAILROAD:
      case ACTIVITY_TRANSFORM:
      case ACTIVITY_AIRBASE:
	/* Simple activities. */
	break;
      case ACTIVITY_SENTRY:
	if (i != packet->length - 1) {
	  /* Only allowed as the last order. */
	  return;
	}
	break;
      case ACTIVITY_BASE:
        if (!base_by_number(packet->base[i])) {
          return;
        }
      default:
	return;
      }
      break;
    case ORDER_FULL_MP:
    case ORDER_BUILD_CITY:
    case ORDER_DISBAND:
    case ORDER_BUILD_WONDER:
    case ORDER_TRADEROUTE:
    case ORDER_HOMECITY:
      break;
    case ORDER_LAST:
      /* An invalid order.  This is handled in execute_orders. */
      break;
    }
  }

  /* This must be before old orders are freed. If this is is
   * settlers on city founding mission, city spot reservation
   * from goto_tile must be freed, and free_unit_orders() loses
   * goto_tile information */
  if (punit->ai.ai_role != AIUNIT_NONE) {
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);
  }

  free_unit_orders(punit);
  /* If we waited on a tile, reset punit->done_moving */
  punit->done_moving = (punit->moves_left <= 0);

  if (packet->length == 0) {
    assert(!unit_has_orders(punit));
    send_unit_info(NULL, punit);
    return;
  }

  punit->has_orders = TRUE;
  punit->orders.length = packet->length;
  punit->orders.index = 0;
  punit->orders.repeat = packet->repeat;
  punit->orders.vigilant = packet->vigilant;
  punit->orders.list
    = fc_malloc(packet->length * sizeof(*(punit->orders.list)));
  for (i = 0; i < packet->length; i++) {
    punit->orders.list[i].order = packet->orders[i];
    punit->orders.list[i].dir = packet->dir[i];
    punit->orders.list[i].activity = packet->activity[i];
    punit->orders.list[i].base = packet->base[i];
  }

  if (!packet->repeat) {
    if (is_normal_map_pos(packet->dest_x, packet->dest_y)) {
      punit->goto_tile = map_pos_to_tile(packet->dest_x, packet->dest_y);
    }
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "Orders for unit %d: length:%d",
	  packet->unit_id, packet->length);
  for (i = 0; i < packet->length; i++) {
    freelog(LOG_DEBUG, "  %d,%s", packet->orders[i],
	    dir_get_name(packet->dir[i]));
  }
#endif

  if (!is_player_phase(unit_owner(punit), game.info.phase)
      || execute_orders(punit)) {
    /* Looks like the unit survived. */
    send_unit_info(NULL, punit);
  }
}
