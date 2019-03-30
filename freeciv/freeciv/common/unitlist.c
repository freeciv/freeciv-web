/****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "log.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unitlist.h"

/************************************************************************//**
  Look for a unit with the given ID in the unit list.  Returns NULL if none
  is found.
****************************************************************************/
struct unit *unit_list_find(const struct unit_list *punitlist, int unit_id)
{
  unit_list_iterate(punitlist, punit) {
    if (punit->id == unit_id) {
      return punit;
    }
  } unit_list_iterate_end;

  return NULL;
}

/************************************************************************//**
  Comparison function for unit_list_sort, sorting by ord_map:
  The indirection is a bit gory:
  Read from the right:
    1. cast arg "a" to "ptr to void*"   (we're sorting a list of "void*"'s)
    2. dereference to get the "void*"
    3. cast that "void*" to a "struct unit*"

  Only used in server/savegame.c.
****************************************************************************/
static int compar_unit_ord_map(const struct unit *const *ua,
                               const struct unit *const *ub)
{
  return (*ua)->server.ord_map - (*ub)->server.ord_map;
}

/************************************************************************//**
  Comparison function for unit_list_sort, sorting by ord_city: see above.

  Only used in server/savegame.c.
****************************************************************************/
static int compar_unit_ord_city(const struct unit *const *ua,
                                const struct unit *const *ub)
{
  return (*ua)->server.ord_city - (*ub)->server.ord_city;
}

/************************************************************************//**
  Sorts the unit list by punit->server.ord_map values.

  Only used in server/savegame.c.
****************************************************************************/
void unit_list_sort_ord_map(struct unit_list *punitlist)
{
  fc_assert_ret(is_server());
  unit_list_sort(punitlist, compar_unit_ord_map);
}

/************************************************************************//**
  Sorts the unit list by punit->server.ord_city values.

  Only used in server/savegame.c.
****************************************************************************/
void unit_list_sort_ord_city(struct unit_list *punitlist)
{
  fc_assert_ret(is_server());
  unit_list_sort(punitlist, compar_unit_ord_city);
}


/************************************************************************//**
  Return TRUE if the function returns true for any of the units.
****************************************************************************/
bool can_units_do(const struct unit_list *punits,
		  bool (can_fn)(const struct unit *punit))
{
  unit_list_iterate(punits, punit) {
    if (can_fn(punit)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE if any of the units can do the activity.
****************************************************************************/
bool can_units_do_activity(const struct unit_list *punits,
			   enum unit_activity activity)
{
  /* Make sure nobody uses these old activities any more */
  fc_assert_ret_val(activity != ACTIVITY_FORTRESS
                    && activity != ACTIVITY_AIRBASE, FALSE);

  unit_list_iterate(punits, punit) {
    if (can_unit_do_activity(punit, activity)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE if any of the units can do the targeted activity.
****************************************************************************/
bool can_units_do_activity_targeted(const struct unit_list *punits,
                                    enum unit_activity activity,
                                    struct extra_type *pextra)
{
  unit_list_iterate(punits, punit) {
    if (can_unit_do_activity_targeted(punit, activity, pextra)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE if any of the units can build any road.
****************************************************************************/
bool can_units_do_any_road(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    extra_type_by_cause_iterate(EC_ROAD, pextra) {
      struct road_type *proad = extra_road_get(pextra);

      if (can_build_road(proad, punit, unit_tile(punit))) {
        return TRUE;
      }
    } extra_type_by_cause_iterate_end;
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE if any of the units can build base with given gui_type.
****************************************************************************/
bool can_units_do_base_gui(const struct unit_list *punits,
                           enum base_gui_type base_gui)
{
  unit_list_iterate(punits, punit) {
    struct base_type *pbase = get_base_by_gui_type(base_gui, punit,
                                                   unit_tile(punit));

    if (pbase) {
      /* Some unit can build base of given gui_type */
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  If has_flag is true, returns true iff any of the units have the flag.

  If has_flag is false, returns true iff any of the units don't have the
  flag.
****************************************************************************/
bool units_have_type_flag(const struct unit_list *punits,
                          enum unit_type_flag_id flag, bool has_flag)
{
  unit_list_iterate(punits, punit) {
    if (EQ(has_flag, unit_has_type_flag(punit, flag))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Does the list contain any cityfounder units
****************************************************************************/
bool units_contain_cityfounder(const struct unit_list *punits)
{
  if (game.scenario.prevent_new_cities) {
    return FALSE;
  }

  unit_list_iterate(punits, punit) {
    if (EQ(TRUE, unit_can_do_action(punit, ACTION_FOUND_CITY))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  If has_flag is true, returns true iff any of the units are able to do
  the specified action.

  If has_flag is false, returns true iff any of the units are unable do
  the specified action.
****************************************************************************/
bool units_can_do_action(const struct unit_list *punits,
                         action_id act_id, bool can_do)
{
  unit_list_iterate(punits, punit) {
    if (EQ(can_do, unit_can_do_action(punit, act_id))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return TRUE iff any of the units is a transporter that is occupied.
****************************************************************************/
bool units_are_occupied(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (get_transporter_occupancy(punit) > 0) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE iff any of these units can load.
****************************************************************************/
bool units_can_load(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (unit_can_load(punit)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return TRUE iff any of these units can unload.
****************************************************************************/
bool units_can_unload(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (unit_transported(punit)
        && can_unit_unload(punit, unit_transport_get(punit))
        && can_unit_exist_at_tile(&(wld.map), punit, unit_tile(punit))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return TRUE iff any of the units' tiles have the activity running
  on them.
****************************************************************************/
bool units_have_activity_on_tile(const struct unit_list *punits,
				 enum unit_activity activity)
{
  unit_list_iterate(punits, punit) {
    if (is_unit_activity_on_tile(activity, unit_tile(punit))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return TRUE iff any of the units can be upgraded to another unit type
  (for money)
****************************************************************************/
bool units_can_upgrade(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (UU_OK == unit_upgrade_test(punit, FALSE)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return TRUE iff any of the units can convert to another unit type
****************************************************************************/
bool units_can_convert(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (utype_can_do_action(unit_type_get(punit), ACTION_CONVERT)
        && unit_can_convert(punit)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}
