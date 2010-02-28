/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
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

#include <assert.h>

#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

#include "base.h"
#include "effects.h"
#include "fc_types.h"
#include "map.h"
#include "movement.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"
#include "terrain.h"

static const char *move_type_names[] = {
  "Land", "Sea", "Both"
};

/****************************************************************************
  This function calculates the move rate of the unit, taking into 
  account the penalty for reduced hitpoints (affects sea and land 
  units only), the effects of wonders for sea units, and any veteran
  bonuses.
****************************************************************************/
int unit_move_rate(const struct unit *punit)
{
  int move_rate = 0;
  int base_move_rate = unit_type(punit)->move_rate 
    + unit_type(punit)->veteran[punit->veteran].move_bonus;
  struct unit_class *pclass = unit_class(punit);

  move_rate = base_move_rate;

  if (uclass_has_flag(pclass, UCF_DAMAGE_SLOWS)) {
    /* Scale the MP based on how many HP the unit has. */
    move_rate = (move_rate * punit->hp) / unit_type(punit)->hp;
  }

  /* Add on effects bonus (Magellan's Expedition, Lighthouse,
   * Nuclear Power). */
  move_rate += (get_unit_bonus(punit, EFT_MOVE_BONUS) * SINGLE_MOVE);

  /* Don't let the move_rate be less than min_speed unless the base_move_rate is
   * also less than min_speed. */
  if (move_rate < pclass->min_speed) {
    move_rate = MIN(pclass->min_speed, base_move_rate);
  }

  return move_rate;
}


/****************************************************************************
  Return TRUE iff the unit can be a defender at its current location.  This
  should be checked when looking for a defender - not all units on the
  tile are valid defenders.
****************************************************************************/
bool unit_can_defend_here(const struct unit *punit)
{
  /* Do not just check if unit is transported.
   * Even transported units may step out from transport to fight,
   * if this is their native terrain. */
  return can_unit_exist_at_tile(punit, punit->tile);
}

/****************************************************************************
 This unit can attack non-native tiles (eg. Ships ability to
 shore bombardment)
****************************************************************************/
bool can_attack_non_native(const struct unit_type *utype)
{
  return (utype_class(utype)->move_type == SEA_MOVING
          && !utype_has_flag(utype, F_NO_LAND_ATTACK));
}

/****************************************************************************
 This unit can attack from non-native tiles (Marines can attack from
 transport, ships from harbour cities)
****************************************************************************/
bool can_attack_from_non_native(const struct unit_type *utype)
{
  /* It's clear that LAND_MOVING should not be able to attack from
   * non-native (unless F_MARINES) and it's clear that SEA_MOVING
   * should be able to attack from non-native. It's not clear what to do
   * with BOTH_MOVING. At the moment we return FALSE for
   * them. One can always give "Marines" flag for them. This should be
   * generalized for unit_classes anyway. */
  return (utype_class(utype)->move_type == SEA_MOVING
          || utype_has_flag(utype, F_MARINES));
}

/****************************************************************************
  Return TRUE iff the unit is a sailing/naval/sea/water unit.
****************************************************************************/
bool is_sailing_unit(const struct unit *punit)
{
  return (uclass_move_type(unit_class(punit)) == SEA_MOVING);
}


/****************************************************************************
  Return TRUE iff this unit is a ground/land/normal unit.
****************************************************************************/
bool is_ground_unit(const struct unit *punit)
{
  return (uclass_move_type(unit_class(punit)) == LAND_MOVING);
}


/****************************************************************************
  Return TRUE iff this unit type is a sailing/naval/sea/water unit type.
****************************************************************************/
bool is_sailing_unittype(const struct unit_type *punittype)
{
  return (utype_move_type(punittype) == SEA_MOVING);
}


/****************************************************************************
  Return TRUE iff this unit type is a ground/land/normal unit type.
****************************************************************************/
bool is_ground_unittype(const struct unit_type *punittype)
{
  return (utype_move_type(punittype) == LAND_MOVING);
}

/****************************************************************************
  Return TRUE iff a unit of the given unit type can "exist" at this location.
  This means it can physically be present on the tile (without the use of a
  transporter). See also can_unit_survive_at_tile.
****************************************************************************/
bool can_exist_at_tile(const struct unit_type *utype,
                       const struct tile *ptile)
{
  /* Cities are safe havens except for units in the middle of non-native
   * terrain. This can happen if adjacent terrain is changed after unit
   * arrived to city */
  if (tile_city(ptile)
      && (uclass_has_flag(utype_class(utype), UCF_BUILD_ANYWHERE)
          || is_native_near_tile(utype, ptile))) {
    return TRUE;
  }

  /* A trireme unit cannot exist in an ocean tile without access to land. */
  if (utype_has_flag(utype, F_TRIREME) && !is_safe_ocean(ptile)) {
    return FALSE;
  }

  return is_native_tile(utype, ptile);
}

/****************************************************************************
  Return TRUE iff the unit can "exist" at this location.  This means it can
  physically be present on the tile (without the use of a transporter).  See
  also can_unit_survive_at_tile.
****************************************************************************/
bool can_unit_exist_at_tile(const struct unit *punit,
                            const struct tile *ptile)
{
  return can_exist_at_tile(unit_type(punit), ptile);
}

/****************************************************************************
  This tile is native to unit.

  See is_native_to_class()
****************************************************************************/
bool is_native_tile(const struct unit_type *punittype,
                    const struct tile *ptile)
{
  return is_native_to_class(utype_class(punittype), tile_terrain(ptile),
                            ptile->special, ptile->bases);
}


/****************************************************************************
  This terrain is native to unit.

  See is_native_to_class()
****************************************************************************/
bool is_native_terrain(const struct unit_type *punittype,
                       const struct terrain *pterrain,
                       bv_special special, bv_bases bases)
{
  return is_native_to_class(utype_class(punittype), pterrain, special, bases);
}

/****************************************************************************
  This tile is native to unit class.

  See is_native_to_class()
****************************************************************************/
bool is_native_tile_to_class(const struct unit_class *punitclass,
                             const struct tile *ptile)
{
  return is_native_to_class(punitclass, tile_terrain(ptile),
                            tile_specials(ptile), tile_bases(ptile));
}

/****************************************************************************
  This terrain is native to unit class. Units that require fuel dont survive
  even on native terrain.
****************************************************************************/
bool is_native_to_class(const struct unit_class *punitclass,
                        const struct terrain *pterrain,
                        bv_special special, bv_bases bases)
{
  if (!pterrain) {
    /* Unknown is considered native terrain */
    return TRUE;
  }

  if (BV_ISSET(pterrain->native_to, uclass_index(punitclass))) {
    return TRUE;
  }
  if (uclass_has_flag(punitclass, UCF_ROAD_NATIVE)
      && contains_special(special, S_ROAD)) {
    return TRUE;
  }
  if (uclass_has_flag(punitclass, UCF_RIVER_NATIVE)
      && contains_special(special, S_RIVER)) {
    return TRUE;
  }

  base_type_iterate(pbase) {
    if (BV_ISSET(bases, base_index(pbase))
        && base_has_flag(pbase, BF_NATIVE_TILE)
        && is_native_base_to_uclass(pbase, punitclass)) {
      return TRUE;
    }
  } base_type_iterate_end;

  return FALSE;
}

/****************************************************************************
  Is there native tile adjacent to given tile
****************************************************************************/
bool is_native_near_tile(const struct unit_type *utype, const struct tile *ptile)
{
  if (is_native_tile(utype, ptile)) {
    return TRUE;
  }

  adjc_iterate(ptile, ptile2) {
    if (is_native_tile(utype, ptile2)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/****************************************************************************
  Return TRUE iff the unit can "survive" at this location.  This means it can
  not only be physically present at the tile but will be able to survive
  indefinitely on its own (without a transporter).  Units that require fuel
  or have a danger of drowning are examples of non-survivable units.  See
  also can_unit_exist_at_tile.

  (This function could be renamed as unit_wants_transporter.)
****************************************************************************/
bool can_unit_survive_at_tile(const struct unit *punit,
                              const struct tile *ptile)
{
  if (!can_unit_exist_at_tile(punit, ptile)) {
    return FALSE;
  }

  if (tile_city(ptile)) {
    return TRUE;
  }

  if (tile_has_native_base(ptile, unit_type(punit))) {
    /* Unit can always survive at native base */
    return TRUE;
  }

  if (utype_fuel(unit_type(punit))) {
    /* Unit requires fuel and this is not refueling tile */
    return FALSE;
  }

  if (is_losing_hp(punit)) {
    /* Unit is losing HP over time in this tile (no city or native base) */
    return FALSE;
  }

  return TRUE;
}


/****************************************************************************
  Returns whether the unit is allowed (by ZOC) to move from src_tile
  to dest_tile (assumed adjacent).

  You CAN move if:
    1. You have units there already
    2. Your unit isn't a ground unit
    3. Your unit ignores ZOC (diplomat, freight, etc.)
    4. You're moving from or to a city
    5. You're moving from an ocean square (from a boat)
    6. The spot you're moving from or to is in your ZOC
****************************************************************************/
bool can_step_taken_wrt_to_zoc(const struct unit_type *punittype,
                               const struct player *unit_owner,
                               const struct tile *src_tile,
                               const struct tile *dst_tile)
{
  if (unit_type_really_ignores_zoc(punittype)) {
    return TRUE;
  }
  if (is_allied_unit_tile(dst_tile, unit_owner)) {
    return TRUE;
  }
  if (tile_city(src_tile) || tile_city(dst_tile)) {
    return TRUE;
  }
  if (is_ocean_tile(src_tile)
      || is_ocean_tile(dst_tile)) {
    return TRUE;
  }
  return (is_my_zoc(unit_owner, src_tile)
	  || is_my_zoc(unit_owner, dst_tile));
}


/****************************************************************************
  See can_step_take_wrt_to_zoc().  This function is exactly the same but
  it takes a unit instead of a unittype and player.
****************************************************************************/
static bool zoc_ok_move_gen(const struct unit *punit,
                            const struct tile *src_tile,
                            const struct tile *dst_tile)
{
  return can_step_taken_wrt_to_zoc(unit_type(punit), unit_owner(punit),
				   src_tile, dst_tile);
}


/****************************************************************************
  Returns whether the unit can safely move from its current position to
  the adjacent dst_tile.  This function checks only ZOC.

  See can_step_taken_wrt_to_zoc().
****************************************************************************/
bool zoc_ok_move(const struct unit *punit, const struct tile *dst_tile)
{
  return zoc_ok_move_gen(punit, punit->tile, dst_tile);
}


/****************************************************************************
  Returns whether the unit can move from its current tile to the destination
  tile.

  See test_unit_move_to_tile().
****************************************************************************/
bool can_unit_move_to_tile(const struct unit *punit,
                           const struct tile *dst_tile,
                           bool igzoc)
{
  return MR_OK == test_unit_move_to_tile(unit_type(punit), unit_owner(punit),
					 punit->activity,
					 punit->tile, dst_tile,
					 igzoc);
}


/**************************************************************************
  Returns whether the unit can move from its current tile to the
  destination tile.  An enumerated value is returned indication the error
  or success status.

  The unit can move if:
    1) The unit is idle or on server goto.
    2) The target location is next to the unit.
    3) There are no non-allied units on the target tile.
    4) Unit can move to non-native tile if there is city
       or free transport capacity.
    5) Marines are the only land units that can attack from a ocean square.
    6) There are no peaceful but un-allied units on the target tile.
    7) There is not a peaceful but un-allied city on the target tile.
    8) There is no non-allied unit blocking (zoc) [or igzoc is true].
    9) Triremes cannot move out of sight from land.
   10) It is not the territory of a player we are at peace with.
**************************************************************************/
enum unit_move_result test_unit_move_to_tile(const struct unit_type *punittype,
					     const struct player *unit_owner,
					     enum unit_activity activity,
					     const struct tile *src_tile,
					     const struct tile *dst_tile,
					     bool igzoc)
{
  bool zoc;
  struct city *pcity;

  /* 1) */
  if (activity != ACTIVITY_IDLE
      && activity != ACTIVITY_GOTO) {
    /* For other activities the unit must be stationary. */
    return MR_BAD_ACTIVITY;
  }

  /* 2) */
  if (!is_tiles_adjacent(src_tile, dst_tile)) {
    /* Of course you can only move to adjacent positions. */
    return MR_BAD_DESTINATION;
  }

  /* 3) */
  if (is_non_allied_unit_tile(dst_tile, unit_owner)) {
    /* You can't move onto a tile with non-allied units on it (try
     * attacking instead). */
    return MR_DESTINATION_OCCUPIED_BY_NON_ALLIED_UNIT;
  }

  /* 4) */
  if (!is_native_tile(punittype, dst_tile)
      && !is_allied_city_tile(dst_tile, unit_owner)
      && unit_class_transporter_capacity(dst_tile, unit_owner, utype_class(punittype)) <= 0) {
    return MR_NO_TRANSPORTER_CAPACITY;
  }

  if (utype_move_type(punittype) == LAND_MOVING) {

    /* Moving from ocean */
    if (is_ocean_tile(src_tile)) {
      /* 5) */
      if (!utype_has_flag(punittype, F_MARINES)
	  && is_enemy_city_tile(dst_tile, unit_owner)) {
	/* Most ground units can't move into cities from ships.  (Note this
	 * check is only for movement, not attacking: most ground units
	 * can't attack from ship at *any* units on land.) */
	return MR_BAD_TYPE_FOR_CITY_TAKE_OVER;
      }
    }
  }

  /* 6) */
  if (is_non_attack_unit_tile(dst_tile, unit_owner)) {
    /* You can't move into a non-allied tile.
     *
     * FIXME: this should never happen since it should be caught by check
     * #3. */
    return MR_NO_WAR;
  }

  /* 7) */
  pcity = tile_city(dst_tile);
  if (pcity && pplayers_non_attack(city_owner(pcity), unit_owner)) {
    /* You can't move into an empty city of a civilization you're at
     * peace with - you must first either declare war or make alliance. */
    return MR_NO_WAR;
  }

  /* 8) */
  zoc = igzoc
    || can_step_taken_wrt_to_zoc(punittype, unit_owner, src_tile, dst_tile);
  if (!zoc) {
    /* The move is illegal because of zones of control. */
    return MR_ZOC;
  }

  /* 9) */
  if (utype_has_flag(punittype, F_TRIREME) && !is_safe_ocean(dst_tile)) {
    return MR_TRIREME;
  }

  /* 9) */
  if (!utype_has_flag(punittype, F_CIVILIAN)
      && !player_can_invade_tile(unit_owner, dst_tile)) {
    return MR_PEACE;
  }

  return MR_OK;
}

/**************************************************************************
  Return true iff transporter has ability to transport transported.
**************************************************************************/
bool can_unit_transport(const struct unit *transporter,
                        const struct unit *transported)
{
  return can_unit_type_transport(unit_type(transporter), unit_class(transported));
}

/**************************************************************************
  Return TRUE iff transporter type has ability to transport transported class.
**************************************************************************/
bool can_unit_type_transport(const struct unit_type *transporter,
                             const struct unit_class *transported)
{
  if (transporter->transport_capacity <= 0) {
    return FALSE;
  }

  return BV_ISSET(transporter->cargo, uclass_index(transported));
}

/**************************************************************************
  Convert move type names to enum; case insensitive;
  returns MOVETYPE_LAST if can't match.
**************************************************************************/
enum unit_move_type move_type_from_str(const char *s)
{
  int i;

  for (i = 0; i < MOVETYPE_LAST; i++) {
    if (mystrcasecmp(move_type_names[i], s)==0) {
      return i;
    }
  }
  return MOVETYPE_LAST;
}

/**************************************************************************
  Search transport suitable for given unit from tile. It has to have
  free space in it.
**************************************************************************/
struct unit *find_transport_from_tile(struct unit *punit, struct tile *ptile)
{
  unit_list_iterate(ptile->units, ptransport) {
    if (could_unit_load(ptransport, punit)) {
      return ptransport;
    }
  } unit_list_iterate_end;

  return NULL;
}
 
/**************************************************************************
 Returns the number of free spaces for units of given class.
 Can be 0.
**************************************************************************/
int unit_class_transporter_capacity(const struct tile *ptile,
                                    const struct player *pplayer,
                                    const struct unit_class *pclass)
{
  int availability = 0;

  unit_list_iterate(ptile->units, punit) {
    if (unit_owner(punit) == pplayer
        || pplayers_allied(unit_owner(punit), pplayer)) {

      if (can_unit_type_transport(unit_type(punit), pclass)) {
        availability += get_transporter_capacity(punit);
        availability -= get_transporter_occupancy(punit);
      }
    }
  } unit_list_iterate_end;

  return availability;
}
