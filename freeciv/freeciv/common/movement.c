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
#include <fc_config.h>
#endif

/* utility */
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "astring.h"
#include "base.h"
#include "effects.h"
#include "fc_types.h"
#include "game.h"
#include "map.h"
#include "road.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"
#include "terrain.h"

#include "movement.h"

/************************************************************************//**
  This function calculates the move rate of the unit, taking into account
  the penalty for reduced hitpoints, the active effects, and any veteran
  bonuses.

  'utype' and 'pplayer' must be set. 'ptile' can be NULL.
****************************************************************************/
int utype_move_rate(const struct unit_type *utype, const struct tile *ptile,
                    const struct player *pplayer, int veteran_level,
                    int hitpoints)
{
  const struct unit_class *uclass;
  const struct veteran_level *vlevel;
  int base_move_rate, move_rate;

  fc_assert_ret_val(NULL != utype, 0);
  fc_assert_ret_val(NULL != pplayer, 0);
  vlevel = utype_veteran_level(utype, veteran_level);
  fc_assert_ret_val(NULL != vlevel, 0);
  uclass = utype_class(utype);

  base_move_rate = utype->move_rate + vlevel->move_bonus;
  move_rate = base_move_rate;

  if (uclass_has_flag(uclass, UCF_DAMAGE_SLOWS)) {
    /* Scale the MP based on how many HP the unit has. */
    move_rate = (move_rate * hitpoints) / utype->hp;
  }

  /* Add on effects bonus (Magellan's Expedition, Lighthouse,
   * Nuclear Power). */
  move_rate += (get_unittype_bonus(pplayer, ptile, utype, EFT_MOVE_BONUS)
                * SINGLE_MOVE);

  /* Don't let the move_rate be less than min_speed unless the base_move_rate is
   * also less than min_speed. */
  if (move_rate < uclass->min_speed) {
    move_rate = MIN(uclass->min_speed, base_move_rate);
  }

  return move_rate;
}

/************************************************************************//**
  This function calculates the move rate of the unit. See utype_move_rate()
  for further details.
****************************************************************************/
int unit_move_rate(const struct unit *punit)
{
  fc_assert_ret_val(NULL != punit, 0);

  return utype_move_rate(unit_type_get(punit), unit_tile(punit),
                         unit_owner(punit), punit->veteran, punit->hp);
}

/************************************************************************//**
  This function calculates the movement cost to unknown tiles. The base
  value is equal to the highest movement cost the unit can encounter. If
  the unit cannot enter all terrains, a malus is applied.
  The returned value is usually cached into utype->unknown_move_cost and
  used in the path-finding module.
****************************************************************************/
int utype_unknown_move_cost(const struct unit_type *utype)
{
  const struct unit_class *uclass = utype_class(utype);
  int move_cost;

  if (!uclass_has_flag(uclass, UCF_TERRAIN_SPEED)) {
    /* Unit is not subject to terrain movement costs. */
    move_cost = SINGLE_MOVE;
  } else if (utype_has_flag(utype, UTYF_IGTER)) {
    /* All terrain movement costs are equal! */
    move_cost = MOVE_COST_IGTER;
  } else {
    /* Unit is subject to terrain movement costs. */
    move_cost = 1; /* Arbitrary minimum. */
    terrain_type_iterate(pterrain) {
      if (is_native_to_class(uclass, pterrain, NULL)
          && pterrain->movement_cost > move_cost) {
        /* Exact movement cost matters only if we can enter
         * the tile. */
        move_cost = pterrain->movement_cost;
      }
    } terrain_type_iterate_end;
    move_cost *= SINGLE_MOVE; /* Real value. */
  }

  /* Let's see if we can cross over all terrain types, else apply a malus.
   * We want units that may encounter unsuitable terrain explore less.
   * N.B.: We don't take in account terrain no unit can enter here. */
  terrain_type_iterate(pterrain) {
    if (BV_ISSET_ANY(pterrain->native_to)
        && !is_native_to_class(uclass, pterrain, NULL)) {
      /* Units that may encounter unsuitable terrain explore less. */
      move_cost *= 2;
      break;
    }
  } terrain_type_iterate_end;

  return move_cost;
}


/************************************************************************//**
  Return TRUE iff the unit can be a defender at its current location.  This
  should be checked when looking for a defender - not all units on the
  tile are valid defenders.
****************************************************************************/
bool unit_can_defend_here(const struct civ_map *nmap, const struct unit *punit)
{
  struct unit *ptrans = unit_transport_get(punit);

  /* Do not just check if unit is transported.
   * Even transported units may step out from transport to fight,
   * if this is their native terrain. */
  return (can_unit_exist_at_tile(nmap, punit, unit_tile(punit))
          && (ptrans == NULL || can_unit_unload(punit, ptrans)));
}

/************************************************************************//**
 This unit can attack non-native tiles (eg. Ships ability to
 shore bombardment)
****************************************************************************/
bool can_attack_non_native(const struct unit_type *utype)
{
  return uclass_has_flag(utype_class(utype), UCF_ATTACK_NON_NATIVE)
         && utype->attack_strength > 0
         && !utype_has_flag(utype, UTYF_ONLY_NATIVE_ATTACK);
}

/************************************************************************//**
 This unit can attack from non-native tiles (Marines can attack from
 transport, ships from harbour cities)
****************************************************************************/
bool can_attack_from_non_native(const struct unit_type *utype)
{
  return (utype_can_do_act_when_ustate(utype, ACTION_ATTACK,
                                       USP_NATIVE_TILE, FALSE)
          || utype_can_do_act_when_ustate(utype, ACTION_SUICIDE_ATTACK,
                                          USP_NATIVE_TILE, FALSE)
          || utype_can_do_act_when_ustate(utype, ACTION_CONQUER_CITY,
                                          USP_LIVABLE_TILE, FALSE));
}

/************************************************************************//**
  Check for a city channel.
****************************************************************************/
bool is_city_channel_tile(const struct unit_class *punitclass,
                          const struct tile *ptile,
                          const struct tile *pexclude)
{
  struct dbv tile_processed;
  struct tile_list *process_queue = tile_list_new();
  bool found = FALSE;

  dbv_init(&tile_processed, map_num_tiles());
  for (;;) {
    dbv_set(&tile_processed, tile_index(ptile));
    adjc_iterate(&(wld.map), ptile, piter) {
      if (dbv_isset(&tile_processed, tile_index(piter))) {
        continue;
      } else if (piter != pexclude
                 && is_native_to_class(punitclass, tile_terrain(piter),
                                       tile_extras(piter))) {
        found = TRUE;
        break;
      } else if (piter != pexclude
                 && NULL != tile_city(piter)) {
        tile_list_append(process_queue, piter);
      } else {
        dbv_set(&tile_processed, tile_index(piter));
      }
    } adjc_iterate_end;

    if (found || 0 == tile_list_size(process_queue)) {
      break; /* No more tile to process. */
    } else {
      ptile = tile_list_front(process_queue);
      tile_list_pop_front(process_queue);
    }
  }

  dbv_free(&tile_processed);
  tile_list_destroy(process_queue);
  return found;
}

/************************************************************************//**
  Return TRUE iff a unit of the given unit type can "exist" at this location.
  This means it can physically be present on the tile (without the use of a
  transporter). See also can_unit_survive_at_tile.
****************************************************************************/
bool can_exist_at_tile(const struct civ_map *nmap,
                       const struct unit_type *utype,
                       const struct tile *ptile)
{
  /* Cities are safe havens except for units in the middle of non-native
   * terrain. This can happen if adjacent terrain is changed after unit
   * arrived to city. */
  if (NULL != tile_city(ptile)
      && (uclass_has_flag(utype_class(utype), UCF_BUILD_ANYWHERE)
          || is_native_near_tile(nmap, utype_class(utype), ptile)
          || (1 == game.info.citymindist
              && is_city_channel_tile(utype_class(utype), ptile, NULL)))) {
    return TRUE;
  }

  /* UTYF_COAST_STRICT unit cannot exist in an ocean tile without access to land. */
  if (utype_has_flag(utype, UTYF_COAST_STRICT)
      && !is_safe_ocean(nmap, ptile)) {
    return FALSE;
  }

  return is_native_tile(utype, ptile);
}

/************************************************************************//**
  Return TRUE iff the unit can "exist" at this location.  This means it can
  physically be present on the tile (without the use of a transporter).  See
  also can_unit_survive_at_tile.
****************************************************************************/
bool can_unit_exist_at_tile(const struct civ_map *nmap,
                            const struct unit *punit,
                            const struct tile *ptile)
{
  return can_exist_at_tile(nmap, unit_type_get(punit), ptile);
}

/************************************************************************//**
  This tile is native to unit.

  See is_native_to_class()
****************************************************************************/
bool is_native_tile(const struct unit_type *punittype,
                    const struct tile *ptile)
{
  return is_native_to_class(utype_class(punittype), tile_terrain(ptile),
                            tile_extras(ptile));
}

/************************************************************************//**
  This terrain is native to unit class. Units that require fuel dont survive
  even on native terrain.
****************************************************************************/
bool is_native_to_class(const struct unit_class *punitclass,
                        const struct terrain *pterrain,
                        const bv_extras *extras)
{
  if (!pterrain) {
    /* Unknown is considered native terrain */
    return TRUE;
  }

  if (BV_ISSET(pterrain->native_to, uclass_index(punitclass))) {
    return TRUE;
  }

  if (extras != NULL) {
    extra_type_list_iterate(punitclass->cache.native_tile_extras, pextra) {
      if (BV_ISSET(*extras, extra_index(pextra))) {
        return TRUE;
      }
    } extra_type_list_iterate_end;
  }

  return FALSE;
}


/************************************************************************//**
  Is the move under consideration a native move?
  Note that this function does not check for possible moves, only native
  moves, so that callers are responsible for checking for other sources of
  legal moves (e.g. cities, transports, etc.).
****************************************************************************/
bool is_native_move(const struct unit_class *punitclass,
                    const struct tile *src_tile,
                    const struct tile *dst_tile)
{
  const struct road_type *proad;

  if (is_native_to_class(punitclass, tile_terrain(dst_tile), NULL)) {
    /* We aren't using extras to make the destination native. */
    return TRUE;
  } else if (!is_native_tile_to_class(punitclass, src_tile)) {
    /* Disembarking or leaving port, so ignore road connectivity. */
    return TRUE;
  } else if (is_native_to_class(punitclass, tile_terrain(src_tile), NULL)) {
    /* Native source terrain depends entirely on destination tile nativity. */
    return is_native_tile_to_class(punitclass, dst_tile);
  }

  /* Check for non-road native extras on the source tile. */
  extra_type_list_iterate(punitclass->cache.native_tile_extras, pextra) {
    if (tile_has_extra(src_tile, pextra)
        && !is_extra_caused_by(pextra, EC_ROAD)
        && is_native_tile_to_class(punitclass, dst_tile)) {
      /* If there is one, and the destination is native, the move is native. */
      return TRUE;
    }
  } extra_type_list_iterate_end;

  extra_type_list_iterate(punitclass->cache.native_tile_extras, pextra) {
    if (!tile_has_extra(dst_tile, pextra)) {
      continue;
    } else if (!is_extra_caused_by(pextra, EC_ROAD)) {
      /* The destination is native because of a non-road extra. */
      return TRUE;
    }

    proad = extra_road_get(pextra);

    if (road_has_flag(proad, RF_JUMP_TO)) {
      extra_type_list_iterate(punitclass->cache.native_tile_extras, jextra) {
        if (pextra != jextra
            && is_extra_caused_by(jextra, EC_ROAD)
            && tile_has_extra(src_tile, jextra)
            && road_has_flag(jextra->data.road, RF_JUMP_FROM)) {
          return TRUE;
        }
      } extra_type_list_iterate_end;
    }

    extra_type_list_iterate(proad->integrators, iextra) {
      if (!tile_has_extra(src_tile, iextra)) {
        continue;
      }
      if (ALL_DIRECTIONS_CARDINAL()) {
        /* move_mode does not matter as all of them accept cardinal move */
        return TRUE;
      }
      switch (extra_road_get(iextra)->move_mode) {
      case RMM_FAST_ALWAYS:
        /* Road connects source and destination, so we're fine. */
        return TRUE;
      case RMM_CARDINAL:
        /* Road connects source and destination if cardinal move. */
        if (is_move_cardinal(&(wld.map), src_tile, dst_tile)) {
          return TRUE;
        }
        break;
      case RMM_RELAXED:
        if (is_move_cardinal(&(wld.map), src_tile, dst_tile)) {
          /* Cardinal moves have no between tiles, so connected. */
          return TRUE;
        }
        cardinal_between_iterate(&(wld.map), src_tile, dst_tile, between) {
          if (tile_has_extra(between, iextra)
              || (pextra != iextra && tile_has_extra(between, pextra))) {
            /* We have a link for the connection.
             * 'pextra != iextra' is there just to avoid tile_has_extra()
             * in by far more common case that 'pextra == iextra' */
            return TRUE;
          }
        } cardinal_between_iterate_end;
        break;
      }
    } extra_type_list_iterate_end;
  } extra_type_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Is there native tile adjacent to given tile
****************************************************************************/
bool is_native_near_tile(const struct civ_map *nmap,
                         const struct unit_class *uclass,
                         const struct tile *ptile)
{
  if (is_native_tile_to_class(uclass, ptile)) {
    return TRUE;
  }

  adjc_iterate(nmap, ptile, ptile2) {
    if (is_native_tile_to_class(uclass, ptile2)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return TRUE iff the unit can "survive" at this location.  This means it can
  not only be physically present at the tile but will be able to survive
  indefinitely on its own (without a transporter).  Units that require fuel
  or have a danger of drowning are examples of non-survivable units.  See
  also can_unit_exist_at_tile().

  (This function could be renamed as unit_wants_transporter.)
****************************************************************************/
bool can_unit_survive_at_tile(const struct civ_map *nmap,
                              const struct unit *punit,
                              const struct tile *ptile)
{
  if (!can_unit_exist_at_tile(nmap, punit, ptile)) {
    return FALSE;
  }

  if (tile_city(ptile)) {
    return TRUE;
  }

  if (tile_has_refuel_extra(ptile, unit_type_get(punit))) {
    /* Unit can always survive at refueling base */
    return TRUE;
  }

  if (utype_fuel(unit_type_get(punit))) {
    /* Unit requires fuel and this is not refueling tile */
    return FALSE;
  }

  if (is_losing_hp(punit)) {
    /* Unit is losing HP over time in this tile (no city or native base) */
    return FALSE;
  }

  return TRUE;
}


/************************************************************************//**
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
                               const struct tile *dst_tile,
                               const struct civ_map *zmap)
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
  if (terrain_has_flag(tile_terrain(src_tile), TER_NO_ZOC)
      || terrain_has_flag(tile_terrain(dst_tile), TER_NO_ZOC)) {
    return TRUE;
  }

  return (is_my_zoc(unit_owner, src_tile, zmap)
	  || is_my_zoc(unit_owner, dst_tile, zmap));
}

/************************************************************************//**
  See can_step_take_wrt_to_zoc().  This function is exactly the same but
  it takes a unit instead of a unittype and player.
****************************************************************************/
static bool zoc_ok_move_gen(const struct unit *punit,
                            const struct tile *src_tile,
                            const struct tile *dst_tile,
                            const struct civ_map *zmap)
{
  return can_step_taken_wrt_to_zoc(unit_type_get(punit), unit_owner(punit),
				   src_tile, dst_tile, zmap);
}

/************************************************************************//**
  Returns whether the unit can safely move from its current position to
  the adjacent dst_tile.  This function checks only ZOC.

  See can_step_taken_wrt_to_zoc().
****************************************************************************/
bool zoc_ok_move(const struct unit *punit, const struct tile *dst_tile,
                 const struct civ_map *zmap)
{
  return zoc_ok_move_gen(punit, unit_tile(punit), dst_tile, zmap);
}

/************************************************************************//**
  Returns whether the unit can move from its current tile to the destination
  tile.

  See unit_move_to_tile_test().
****************************************************************************/
bool unit_can_move_to_tile(const struct civ_map *nmap,
                           const struct unit *punit,
                           const struct tile *dst_tile,
                           bool igzoc,
                           bool enter_enemy_city)
{
  return (MR_OK == unit_move_to_tile_test(nmap, punit,
                                          punit->activity, unit_tile(punit),
                                          dst_tile, igzoc, NULL,
                                          enter_enemy_city));
}

/************************************************************************//**
  Returns whether the unit can move from its current tile to the
  destination tile.  An enumerated value is returned indication the error
  or success status.

  The unit can move if:
    1) The unit is idle or on server goto.
    2) Unit is not prohibited from moving by scenario
    3) The target location is next to the unit.
    4) There are no non-allied units on the target tile.
    5) Animals cannot move out from home terrains
    6) Unit can move to a tile where it can't survive on its own if there
       is free transport capacity.
    7) There are no peaceful but non allied units on the target tile.
    8) There is not a non allied city on the target tile when
       enter_enemy_city is false. When enter_enemy_city is true a non
       peaceful city is also accepted.
    9) There is no non-allied unit blocking (zoc) [or igzoc is true].
   10) Triremes cannot move out of sight from land.
   11) It is not the territory of a player we are at peace with.
   12) The unit is unable to disembark from current transporter.
   13) The unit is making a non-native move (e.g. lack of road)
****************************************************************************/
enum unit_move_result
unit_move_to_tile_test(const struct civ_map *nmap,
                       const struct unit *punit,
                       enum unit_activity activity,
                       const struct tile *src_tile,
                       const struct tile *dst_tile, bool igzoc,
                       struct unit *embark_to,
                       bool enter_enemy_city)
{
  bool zoc;
  struct city *pcity;
  const struct unit_type *punittype = unit_type_get(punit);
  const struct player *puowner = unit_owner(punit);

  /* 1) */
  if (activity != ACTIVITY_IDLE
      && activity != ACTIVITY_GOTO) {
    /* For other activities the unit must be stationary. */
    return MR_BAD_ACTIVITY;
  }

  /* 2) */
  if (punit->stay) {
    return MR_UNIT_STAY;
  }

  /* 3) */
  if (!is_tiles_adjacent(src_tile, dst_tile)) {
    /* Of course you can only move to adjacent positions. */
    return MR_BAD_DESTINATION;
  }

  /* 4) */
  if (is_non_allied_unit_tile(dst_tile, puowner)) {
    /* You can't move onto a tile with non-allied units on it (try
     * attacking instead). */
    return MR_DESTINATION_OCCUPIED_BY_NON_ALLIED_UNIT;
  }

  /* 5) */
  if (puowner->ai_common.barbarian_type == ANIMAL_BARBARIAN
      && dst_tile->terrain->animal != punittype) {
    return MR_ANIMAL_DISALLOWED;
  }

  /* 6) */
  if (embark_to != NULL) {
    if (!could_unit_load(punit, embark_to)) {
      return MR_NO_TRANSPORTER_CAPACITY;
    }
  } else if (!(can_exist_at_tile(nmap, punittype, dst_tile)
               || unit_could_load_at(punit, dst_tile))) {
    return MR_NO_TRANSPORTER_CAPACITY;
  }

  /* 7) */
  if (is_non_attack_unit_tile(dst_tile, puowner)) {
    /* You can't move into a non-allied tile.
     *
     * FIXME: this should never happen since it should be caught by check
     * #3. */
    return MR_NO_WAR;
  }

  /* 8) */
  if ((pcity = tile_city(dst_tile))) {
    if (enter_enemy_city) {
      if (pplayers_non_attack(city_owner(pcity), puowner)) {
        /* You can't move into an empty city of a civilization you're at
         * peace with - you must first either declare war or make an
         * alliance. */
        return MR_NO_WAR;
      }
    } else {
      if (!pplayers_allied(city_owner(pcity), puowner)) {
        /* You can't move into an empty city of a civilization you're not
         * allied to. Assume that the player tried to attack. */
        return MR_NO_WAR;
      }
    }
  }

  /* 9) */
  zoc = igzoc
    || can_step_taken_wrt_to_zoc(punittype, puowner, src_tile, dst_tile, nmap);
  if (!zoc) {
    /* The move is illegal because of zones of control. */
    return MR_ZOC;
  }

  /* 10) */
  if (utype_has_flag(punittype, UTYF_COAST_STRICT)
      && !is_safe_ocean(&(wld.map), dst_tile)) {
    return MR_TRIREME;
  }

  /* 11) */
  if (!utype_has_flag(punittype, UTYF_CIVILIAN)
      && !player_can_invade_tile(puowner, dst_tile)) {
    return MR_PEACE;
  }

  /* 12) */
  if (unit_transported(punit)
     && !can_unit_unload(punit, unit_transport_get(punit))) {
    return MR_CANNOT_DISEMBARK;
  }

  /* 13) */
  if (!(is_native_move(utype_class(punittype), src_tile, dst_tile)
        /* Allow non-native moves into cities or boarding transport. */
        || pcity
        || unit_could_load_at(punit, dst_tile))) {
    return MR_NON_NATIVE_MOVE;
  }

  return MR_OK;
}

/************************************************************************//**
  Return true iff transporter has ability to transport transported.
****************************************************************************/
bool can_unit_transport(const struct unit *transporter,
                        const struct unit *transported)
{
  fc_assert_ret_val(transporter != NULL, FALSE);
  fc_assert_ret_val(transported != NULL, FALSE);

  return can_unit_type_transport(unit_type_get(transporter),
                                 unit_class_get(transported));
}

/************************************************************************//**
  Return TRUE iff transporter type has ability to transport transported class.
****************************************************************************/
bool can_unit_type_transport(const struct unit_type *transporter,
                             const struct unit_class *transported)
{
  if (transporter->transport_capacity <= 0) {
    return FALSE;
  }

  return BV_ISSET(transporter->cargo, uclass_index(transported));
}

/************************************************************************//**
  Return whether we can find a suitable transporter for given unit at
  current location. It needs to have free space. To find the best transporter,
  see transporter_for_unit().
****************************************************************************/
bool unit_can_load(const struct unit *punit)
{
  if (unit_transported(punit)) {
    /* In another transport already. Can it unload first? */
    if (!can_unit_unload(punit, punit->transporter)) {
      return FALSE;
    }
  }

  unit_list_iterate(unit_tile(punit)->units, ptransport) {
    /* could_unit_load() instead of can_unit_load() since latter
     * would check against unit already being transported, and we need
     * to support unload+load to a new transport. */
    if (ptransport != punit->transporter) {
      if (could_unit_load(punit, ptransport)) {
        return TRUE;
      }
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Return whether we could find a suitable transporter for given unit at
  'ptile'. It needs to have free space. To find the best transporter, see
  transporter_for_unit_at().
****************************************************************************/
bool unit_could_load_at(const struct unit *punit, const struct tile *ptile)
{
  unit_list_iterate(ptile->units, ptransport) {
    if (could_unit_load(punit, ptransport)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

static int move_points_denomlen = 0;

/************************************************************************//**
  Call whenever terrain_control.move_fragments / SINGLE_MOVE changes.
****************************************************************************/
void init_move_fragments(void)
{
  char denomstr[10];
  /* String length of maximum denominator for fractional representation of
   * movement points, for padding of text representation */
  fc_snprintf(denomstr, sizeof(denomstr), "%d", SINGLE_MOVE);
  move_points_denomlen = strlen(denomstr);
}

/************************************************************************//**
  Render positive movement points as text, including fractional movement
  points, scaled by SINGLE_MOVE. Returns a pointer to a static buffer.
  'reduce' is whether fractional movement points should be reduced to
    lowest terms (this might be confusing in some cases).
  'prefix' is a string put in front of all numeric output.
  'none' is the string to display in place of the integer part if no
    movement points (or NULL to just say 0).
  'align' controls whether this is for a fixed-width table, in which case
    padding spaces will be included to make all such strings line up when
    right-aligned.
****************************************************************************/
const char *move_points_text_full(int mp, bool reduce, const char *prefix,
                                  const char *none, bool align)
{
  static struct astring str = ASTRING_INIT;
  int pad1, pad2;

  if (align && SINGLE_MOVE > 1) {
    /* Align to worst-case denominator even if we might be reducing to
     * lowest terms, as other entries in a table might not reduce */
    pad1 = move_points_denomlen;      /* numerator or denominator */
    pad2 = move_points_denomlen*2+2;  /* everything right of integer part */
  } else {
    /* If no possible fractional part, alignment unneeded even if requested */
    pad1 = pad2 = 0;
  }
  if (!prefix) {
    prefix = "";
  }
  astr_clear(&str);
  if ((mp == 0 && none) || SINGLE_MOVE == 0) {
    /* No movement points, and we have a special representation to use */
    /* (Also used when SINGLE_MOVE == 0, to avoid dividing by zero, which is
     * important for client before ruleset has been received. Doesn't much
     * matter what we print in this case.) */
    astr_add(&str, "%s%*s", none ? none : "", pad2, "");
  } else if ((mp % SINGLE_MOVE) == 0) {
    /* Integer move points */
    astr_add(&str, "%s%d%*s", prefix, mp / SINGLE_MOVE, pad2, "");
  } else {
    /* Fractional part */
    int cancel;

    fc_assert(SINGLE_MOVE > 1);
    if (reduce) {
      /* Reduce to lowest terms */
      int gcd = mp;
      /* Calculate greatest common divisor with Euclid's algorithm */
      int b = SINGLE_MOVE;

      while (b != 0) {
        int t = b;
        b = gcd % b;
        gcd = t;
      }
      cancel = gcd;
    } else {
      /* No cancellation */
      cancel = 1;
    }
    if (mp < SINGLE_MOVE) {
      /* Fractional move points */
      astr_add(&str, "%s%*d/%*d", prefix,
               pad1, (mp % SINGLE_MOVE) / cancel, pad1, SINGLE_MOVE / cancel);
    } else {
      /* Integer + fractional move points */
      astr_add(&str,
               "%s%d %*d/%*d", prefix, mp / SINGLE_MOVE,
               pad1, (mp % SINGLE_MOVE) / cancel, pad1, SINGLE_MOVE / cancel);
    }
  }
  return astr_str(&str);
}

/************************************************************************//**
  Simple version of move_points_text_full() -- render positive movement
  points as text without any prefix or alignment.
****************************************************************************/
const char *move_points_text(int mp, bool reduce)
{
  return move_points_text_full(mp, reduce, NULL, NULL, FALSE);
}
