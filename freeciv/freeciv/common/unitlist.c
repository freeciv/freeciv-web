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
#include <config.h>
#endif

/* utility */
#include "log.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unitlist.h"

/****************************************************************************
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

/****************************************************************************
 Comparison function for unit_list_sort, sorting by ord_map:
 The indirection is a bit gory:
 Read from the right:
   1. cast arg "a" to "ptr to void*"   (we're sorting a list of "void*"'s)
   2. dereference to get the "void*"
   3. cast that "void*" to a "struct unit*"
****************************************************************************/
static int compar_unit_ord_map(const struct unit *const *ua,
                               const struct unit *const *ub)
{
  return (*ua)->ord_map - (*ub)->ord_map;
}

/****************************************************************************
 Comparison function for unit_list_sort, sorting by ord_city: see above.
****************************************************************************/
static int compar_unit_ord_city(const struct unit *const *ua,
                                const struct unit *const *ub)
{
  return (*ua)->ord_city - (*ub)->ord_city;
}

/****************************************************************************
  Sorts the unit list by punit->ord_map values.
****************************************************************************/
void unit_list_sort_ord_map(struct unit_list *punitlist)
{
  unit_list_sort(punitlist, compar_unit_ord_map);
}

/****************************************************************************
  Sorts the unit list by punit->ord_city values.
****************************************************************************/
void unit_list_sort_ord_city(struct unit_list *punitlist)
{
  unit_list_sort(punitlist, compar_unit_ord_city);
}


/****************************************************************************
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

/****************************************************************************
  Returns TRUE if any of the units can do the activity.
****************************************************************************/
bool can_units_do_activity(const struct unit_list *punits,
			   enum unit_activity activity)
{
  /* Make sure nobody uses these old activities any more */
  assert(activity != ACTIVITY_FORTRESS && activity != ACTIVITY_AIRBASE);

  unit_list_iterate(punits, punit) {
    if (can_unit_do_activity(punit, activity)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Returns TRUE if any of the units can do the base building activity
****************************************************************************/
bool can_units_do_base(const struct unit_list *punits,
                       Base_type_id base)
{
  unit_list_iterate(punits, punit) {
    if (can_unit_do_activity_base(punit, base)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Returns TRUE if any of the units can build base with given gui_type.
****************************************************************************/
bool can_units_do_base_gui(const struct unit_list *punits,
                           enum base_gui_type base_gui)
{
  unit_list_iterate(punits, punit) {
    struct base_type *pbase = get_base_by_gui_type(base_gui, punit, punit->tile);

    if (pbase) {
      /* Some unit can build base of given gui_type */
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Returns TRUE if any of the units can do the activity.
****************************************************************************/
bool can_units_do_diplomat_action(const struct unit_list *punits,
				  enum diplomat_actions action)
{
  unit_list_iterate(punits, punit) {
    if (is_diplomat_unit(punit)
	&& diplomat_can_do_action(punit, action, punit->tile)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  If has_flag is true, returns true iff any of the units have the flag.

  If has_flag is false, returns true iff any of the units don't have the
  flag.
****************************************************************************/
bool units_have_flag(const struct unit_list *punits, enum unit_flag_id flag,
		     bool has_flag)
{
  unit_list_iterate(punits, punit) {
    if (EQ(has_flag, unit_has_type_flag(punit, flag))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
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

/****************************************************************************
  Returns TRUE iff any of these units can load.
****************************************************************************/
bool units_can_load(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (find_transporter_for_unit(punit)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Return TRUE iff any of these units can unload.
****************************************************************************/
bool units_can_unload(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (can_unit_unload(punit, game_find_unit_by_number(punit->transported_by))
	&& can_unit_exist_at_tile(punit, punit->tile)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Return TRUE iff any of the units' tiles have the activity running
  on them.
****************************************************************************/
bool units_have_activity_on_tile(const struct unit_list *punits,
				 enum unit_activity activity)
{
  unit_list_iterate(punits, punit) {
    if (is_unit_activity_on_tile(activity, punit->tile)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Return TRUE iff any of the units can transform to another unit
****************************************************************************/
bool units_can_transform(const struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    if (test_unit_transform(punit)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}
