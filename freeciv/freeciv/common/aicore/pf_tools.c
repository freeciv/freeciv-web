/***********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
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
#include <fc_config.h>
#endif

#include <string.h>

/* utility */
#include "bitvector.h"
#include "log.h"
#include "mem.h"

/* common */
#include "base.h"
#include "combat.h"
#include "game.h"
#include "movement.h"
#include "tile.h"
#include "unit.h"
#include "unittype.h"

#include "pf_tools.h"

/* ===================== Capability Functions ======================== */

/************************************************************************//**
  Can we attack 'ptile'? At this point, it assumes there are non-allied
  units on the tile.
****************************************************************************/
static inline bool pf_attack_possible(const struct tile *ptile,
                                      enum known_type known,
                                      const struct pf_parameter *param)
{
  bool attack_any;

  if (!can_attack_non_native(param->utype)
      && !is_native_tile(param->utype, ptile)) {
    return FALSE;
  }

  if (TILE_KNOWN_SEEN != known) {
    /* We cannot see units, let's assume we can attack. */
    return TRUE;
  }

  attack_any = FALSE;
  unit_list_iterate(ptile->units, punit) {
    if (!pplayers_at_war(unit_owner(punit), param->owner)) {
      return FALSE;
    }

    /* Unit reachability test. */
    if (BV_ISSET(param->utype->targets, uclass_index(unit_class_get(punit)))
        || tile_has_native_base(ptile, unit_type_get(punit))) {
      attack_any = TRUE;
    } else if (game.info.unreachable_protects) {
      /* We would need to be able to attack all, this is not the case. */
      return FALSE;
    }
  } unit_list_iterate_end;

  return attack_any;
}

/************************************************************************//**
  Determines if a path to 'ptile' would be considered as action rather than
  normal move: attack, diplomat action, caravan action.

  FIXME: For diplomat actions, we should take in account action enablers.
****************************************************************************/
static enum pf_action pf_get_action(const struct tile *ptile,
                                    enum known_type known,
                                    const struct pf_parameter *param)
{
  bool non_allied_city = (NULL != is_non_allied_city_tile(ptile,
                                                          param->owner));

  if (non_allied_city) {
    if (PF_AA_TRADE_ROUTE & param->actions) {
      return PF_ACTION_TRADE_ROUTE;
    }

    if (PF_AA_DIPLOMAT & param->actions) {
      return PF_ACTION_DIPLOMAT;
    }
  }

  if (is_non_allied_unit_tile(ptile, param->owner)) {
    if (PF_AA_DIPLOMAT & param->actions) {
      return PF_ACTION_DIPLOMAT;
    }

    if (PF_AA_UNIT_ATTACK & param->actions) {
      return (pf_attack_possible(ptile, known, param)
              ? PF_ACTION_ATTACK : PF_ACTION_IMPOSSIBLE);
    }
  }

  if (non_allied_city && PF_AA_CITY_ATTACK & param->actions) {
    /* Consider that there are potentially units, even if
     * is_non_allied_unit_tile() returned NULL (usually when
     * '!param->omniscience'). */
    return ((utype_can_take_over(param->utype)
             || pf_attack_possible(ptile, TILE_KNOWN_UNSEEN, param))
            ? PF_ACTION_ATTACK : PF_ACTION_IMPOSSIBLE);
  }

  return PF_ACTION_NONE;
}

/************************************************************************//**
  Determines if an action is possible from 'src' to 'dst': attack, diplomat
  action, or caravan action.
****************************************************************************/
static bool pf_action_possible(const struct tile *src,
                               enum pf_move_scope src_scope,
                               const struct tile *dst,
                               enum pf_action action,
                               const struct pf_parameter *param)
{
  if (PF_ACTION_ATTACK == action) {
    return (PF_MS_NATIVE & src_scope
            || can_attack_from_non_native(param->utype));
            
  } else if (PF_ACTION_DIPLOMAT == action
             || PF_ACTION_TRADE_ROUTE == action) {
    /* Don't try to act when inside of a transport over non native terrain
     * when all actions the unit type can do requires the unit to be on
     * native terrain. */
    if (can_unit_act_when_ustate_is(param->utype, USP_LIVABLE_TILE, FALSE)) {
      return (PF_MS_NATIVE | PF_MS_CITY | PF_MS_TRANSPORT) & src_scope;
    } else {
      return (PF_MS_NATIVE | PF_MS_CITY) & src_scope;
    }
  }
  return TRUE;
}

/************************************************************************//**
  Special case for reverse maps. Always consider the target tile as
  attackable, notably for transports.
****************************************************************************/
static enum pf_action pf_reverse_get_action(const struct tile *ptile,
                                            enum known_type known,
                                            const struct pf_parameter *param)
{
  return (ptile == param->data ? PF_ACTION_ATTACK : PF_ACTION_NONE);
}

/************************************************************************//**
  Determine if we could load into 'ptrans' and its parents.
****************************************************************************/
static inline bool pf_transport_check(const struct pf_parameter *param,
                                      const struct unit *ptrans,
                                      const struct unit_type *trans_utype)
{
  if (!pplayers_allied(unit_owner(ptrans), param->owner)
      || unit_has_orders(ptrans)
      || param->utype == trans_utype
      || !can_unit_type_transport(trans_utype, utype_class(param->utype))
      || can_unit_type_transport(param->utype, utype_class(trans_utype))
      || (GAME_TRANSPORT_MAX_RECURSIVE
          < 1 + unit_transport_depth(ptrans) + param->cargo_depth)) {
    return FALSE;
  }

  if (1 <= param->cargo_depth) {
    unit_type_iterate(cargo_utype) {
      if (BV_ISSET(param->cargo_types, utype_index(cargo_utype))
          && (cargo_utype == trans_utype
              || can_unit_type_transport(cargo_utype,
                                         utype_class(trans_utype)))) {
        return FALSE;
      }
    } unit_type_iterate_end;
  }

  unit_transports_iterate(ptrans, pparent) {
    if (unit_has_orders(pparent)
        || param->utype == (trans_utype = unit_type_get(pparent))
        || can_unit_type_transport(param->utype, utype_class(trans_utype))) {
      return FALSE;
    }

    if (1 <= param->cargo_depth) {
      unit_type_iterate(cargo_utype) {
        if (BV_ISSET(param->cargo_types, utype_index(cargo_utype))
          && (cargo_utype == trans_utype
              || can_unit_type_transport(cargo_utype,
                                         utype_class(trans_utype)))) {
          return FALSE;
        }
      } unit_type_iterate_end;
    }
  } unit_transports_iterate_end;

  return TRUE;
}

/************************************************************************//**
  Determine how it is possible to move from/to 'ptile'. The checks for
  specific move from tile to tile is done in pf_move_possible().
****************************************************************************/
static enum pf_move_scope
pf_get_move_scope(const struct tile *ptile,
                  bool *can_disembark,
                  enum pf_move_scope previous_scope,
                  const struct pf_parameter *param)
{
  enum pf_move_scope scope = PF_MS_NONE;
  const struct unit_class *uclass = utype_class(param->utype);
  struct city *pcity = tile_city(ptile);

  if ((is_native_tile_to_class(uclass, ptile)
       && (!utype_has_flag(param->utype, UTYF_COAST_STRICT)
           || is_safe_ocean(param->map, ptile)))) {
    scope |= PF_MS_NATIVE;
  }

  if (NULL != pcity
      && (utype_can_take_over(param->utype)
          || pplayers_allied(param->owner, city_owner(pcity)))
      && ((previous_scope & PF_MS_CITY) /* City channel previously tested */
          || uclass_has_flag(uclass, UCF_BUILD_ANYWHERE)
          || is_native_near_tile(param->map, uclass, ptile)
          || (1 == game.info.citymindist
              && is_city_channel_tile(uclass, ptile, NULL)))) {
    scope |= PF_MS_CITY;
  }

  if (PF_MS_NONE == scope) {
    /* Check for transporters. Useless if we already got another way to
     * move. */
    bool allied_city_tile = (NULL != pcity
                             && pplayers_allied(param->owner,
                                                city_owner(pcity)));
    const struct unit_type *utype;

    *can_disembark = FALSE;

    unit_list_iterate(ptile->units, punit) {
      utype = unit_type_get(punit);

      if (!pf_transport_check(param, punit, utype)) {
        continue;
      }

      if (allied_city_tile
          || tile_has_native_base(ptile, utype)) {
        scope |= PF_MS_TRANSPORT;
        *can_disembark = TRUE;
        break;
      }

      if (!utype_can_freely_load(param->utype, utype)) {
        continue;
      }

      scope |= PF_MS_TRANSPORT;

      if (utype_can_freely_unload(param->utype, utype)) {
        *can_disembark = TRUE;
        break;
      }
    } unit_list_iterate_end;
  }

  return scope;
}

/************************************************************************//**
  A cost function for amphibious movement.
****************************************************************************/
static enum pf_move_scope
amphibious_move_scope(const struct tile *ptile,
                      bool *can_disembark,
                      enum pf_move_scope previous_scope,
                      const struct pf_parameter *param)
{
  struct pft_amphibious *amphibious = param->data;
  enum pf_move_scope land_scope, sea_scope;
  bool dumb;

  land_scope = pf_get_move_scope(ptile, &dumb, previous_scope,
                                 &amphibious->land);
  sea_scope = pf_get_move_scope(ptile, &dumb, land_scope, &amphibious->sea);

  if ((PF_MS_NATIVE | PF_MS_CITY) & sea_scope) {
    *can_disembark = (PF_MS_CITY & sea_scope
                      || utype_can_freely_unload(amphibious->land.utype,
                                                 amphibious->sea.utype)
                      || tile_has_native_base(ptile, amphibious->sea.utype));
    return PF_MS_TRANSPORT | land_scope;
  }
  return ~PF_MS_TRANSPORT & land_scope;
}

/************************************************************************//**
  Determines if the move between two tiles is possible.
  Do not use this function as part of a test of whether a unit may attack a
  tile: many tiles that pass this test may be unsuitable for some units to
  attack to/from.

  Does not check if the tile is occupied by non-allied units.
****************************************************************************/
static inline bool pf_move_possible(const struct tile *src,
                                    enum pf_move_scope src_scope,
                                    const struct tile *dst,
                                    enum pf_move_scope dst_scope,
                                    const struct pf_parameter *param)
{
  fc_assert(PF_MS_NONE != src_scope);

  if (PF_MS_NONE == dst_scope) {
    return FALSE;
  }

  if (PF_MS_NATIVE == dst_scope
      && (PF_MS_NATIVE & src_scope)
      && !is_native_move(utype_class(param->utype), src, dst)) {
    return FALSE;
  }

  return TRUE;
}


/* ===================== Move Cost Callbacks ========================= */

/************************************************************************//**
  A cost function for regular movement. Permits attacks.
  Use with a TB callback to prevent passing through occupied tiles.
  Does not permit passing through non-native tiles without transport.
****************************************************************************/
static int normal_move(const struct tile *src,
                       enum pf_move_scope src_scope,
                       const struct tile *dst,
                       enum pf_move_scope dst_scope,
                       const struct pf_parameter *param)
{
  if (pf_move_possible(src, src_scope, dst, dst_scope, param)) {
    return map_move_cost(param->map, param->owner, param->utype, src, dst);
  }
  return PF_IMPOSSIBLE_MC;
}

/************************************************************************//**
  A cost function for overlap movement. Do not consider enemy units and
  attacks.
  Permits moves one step into non-native terrain (for ferries, etc.)
  Use with a TB callback to prevent passing through occupied tiles.
  Does not permit passing through non-native tiles without transport.
****************************************************************************/
static int overlap_move(const struct tile *src,
                        enum pf_move_scope src_scope,
                        const struct tile *dst,
                        enum pf_move_scope dst_scope,
                        const struct pf_parameter *param)
{
  if (pf_move_possible(src, src_scope, dst, dst_scope, param)) {
    return map_move_cost(param->map, param->owner, param->utype, src, dst);
  } else if (!(PF_MS_NATIVE & dst_scope)) {
    /* This should always be the last tile reached. */
    return param->move_rate;
  }
  return PF_IMPOSSIBLE_MC;
}

/************************************************************************//**
  A cost function for amphibious movement.
****************************************************************************/
static int amphibious_move(const struct tile *ptile,
                           enum pf_move_scope src_scope,
                           const struct tile *ptile1,
                           enum pf_move_scope dst_scope,
                           const struct pf_parameter *param)
{
  struct pft_amphibious *amphibious = param->data;
  int cost, scale;

  if (PF_MS_TRANSPORT & src_scope) {
    if (PF_MS_TRANSPORT & dst_scope) {
      /* Sea move, moving from native terrain to a city, or leaving port. */
      cost = amphibious->sea.get_MC(ptile,
                                    (PF_MS_CITY & src_scope) | PF_MS_NATIVE,
                                    ptile1,
                                    (PF_MS_CITY & dst_scope) | PF_MS_NATIVE,
                                    &amphibious->sea);
      scale = amphibious->sea_scale;
    } else if (PF_MS_NATIVE & dst_scope) {
      /* Disembark; use land movement function to handle non-native attacks. */
      cost = amphibious->land.get_MC(ptile, PF_MS_TRANSPORT, ptile1,
                                     PF_MS_NATIVE, &amphibious->land);
      scale = amphibious->land_scale;
    } else {
      /* Neither ferry nor passenger can enter tile. */
      return PF_IMPOSSIBLE_MC;
    }
  } else if ((PF_MS_NATIVE | PF_MS_CITY) & dst_scope) {
    /* Land move */
    cost = amphibious->land.get_MC(ptile, PF_MS_NATIVE, ptile1,
                                   PF_MS_NATIVE, &amphibious->land);
    scale = amphibious->land_scale;
  } else {
    /* Now we have disembarked, our ferry can not help us - we have to
     * stay on the land. */
    return PF_IMPOSSIBLE_MC;
  }
  if (cost != PF_IMPOSSIBLE_MC && cost < FC_INFINITY) {
    cost *= scale;
  }
  return cost;
}

/* ===================== Extra Cost Callbacks ======================== */

/************************************************************************//**
  Extra cost call back for amphibious movement
****************************************************************************/
static int amphibious_extra_cost(const struct tile *ptile,
                                 enum known_type known,
                                 const struct pf_parameter *param)
{
  struct pft_amphibious *amphibious = param->data;
  const bool ferry_move = is_native_tile(amphibious->sea.utype, ptile);
  int cost, scale;

  if (known == TILE_UNKNOWN) {
    /* We can travel almost anywhere */
    cost = SINGLE_MOVE;
    scale = MAX(amphibious->sea_scale, amphibious->land_scale);
  } else if (ferry_move && amphibious->sea.get_EC) {
    /* Do the EC callback for sea moves. */
    cost = amphibious->sea.get_EC(ptile, known, &amphibious->sea);
    scale = amphibious->sea_scale;
  } else if (!ferry_move && amphibious->land.get_EC) {
    /* Do the EC callback for land moves. */
    cost = amphibious->land.get_EC(ptile, known, &amphibious->land);
    scale = amphibious->land_scale;
  } else {
    cost = 0;
    scale = 1;
  }

  if (cost != PF_IMPOSSIBLE_MC) {
    cost *= scale;
  }
  return cost;
}


/* ===================== Tile Behaviour Callbacks ==================== */

/************************************************************************//**
  PF callback to prohibit going into the unknown.  Also makes sure we 
  don't plan to attack anyone.
****************************************************************************/
enum tile_behavior no_fights_or_unknown(const struct tile *ptile,
                                        enum known_type known,
                                        const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN
      || is_non_allied_unit_tile(ptile, param->owner)
      || is_non_allied_city_tile(ptile, param->owner)) {
    /* Can't attack */
    return TB_IGNORE;
  }
  return TB_NORMAL;
}

/************************************************************************//**
  PF callback to prohibit attacking anyone.
****************************************************************************/
enum tile_behavior no_fights(const struct tile *ptile,
                             enum known_type known,
                             const struct pf_parameter *param)
{
  if (is_non_allied_unit_tile(ptile, param->owner)
      || is_non_allied_city_tile(ptile, param->owner)) {
    /* Can't attack */
    return TB_IGNORE;
  }
  return TB_NORMAL;
}

/************************************************************************//**
  PF callback to prohibit attacking anyone, except at the destination.
****************************************************************************/
enum tile_behavior no_intermediate_fights(const struct tile *ptile,
                                          enum known_type known,
                                          const struct pf_parameter *param)
{
  if (is_non_allied_unit_tile(ptile, param->owner)
      || is_non_allied_city_tile(ptile, param->owner)) {
    return TB_DONT_LEAVE;
  }
  return TB_NORMAL;
}

/************************************************************************//**
  A callback for amphibious movement
****************************************************************************/
static enum tile_behavior
amphibious_behaviour(const struct tile *ptile, enum known_type known,
                     const struct pf_parameter *param)
{
  struct pft_amphibious *amphibious = param->data;
  const bool ferry_move = is_native_tile(amphibious->sea.utype, ptile);

  /* Simply a wrapper for the sea or land tile_behavior callbacks. */
  if (ferry_move && amphibious->sea.get_TB) {
    return amphibious->sea.get_TB(ptile, known, &amphibious->sea);
  } else if (!ferry_move && amphibious->land.get_TB) {
    return amphibious->land.get_TB(ptile, known, &amphibious->land);
  }
  return TB_NORMAL;
}

/* ===================== Required Moves Lefts Callbacks ================= */

/************************************************************************//**
  Refueling base for air units.
****************************************************************************/
static bool is_possible_base_fuel(const struct tile *ptile,
                                  const struct pf_parameter *param)
{
  const struct unit_class *uclass;
  enum known_type tile_known = (param->omniscience ? TILE_KNOWN_SEEN
                                : tile_get_known(ptile, param->owner));

  if (tile_known == TILE_UNKNOWN) {
    /* Cannot guess if it is */
    return FALSE;
  }

  if (is_allied_city_tile(ptile, param->owner)) {
    return TRUE;
  }

  uclass = utype_class(param->utype);
  extra_type_list_iterate(uclass->cache.refuel_bases, pextra) {
    /* All airbases are considered possible, simply attack enemies. */
    if (tile_has_extra(ptile, pextra)) {
      return TRUE;
    }
  } extra_type_list_iterate_end;

  if (utype_has_flag(param->utype, UTYF_COAST)) {
    return is_safe_ocean(param->map, ptile);
  }

  if (tile_known == TILE_KNOWN_UNSEEN) {
    /* Cannot see units */
    return FALSE;
  }

  /* Check for carriers */
  unit_list_iterate(ptile->units, ptrans) {
    const struct unit_type *trans_utype = unit_type_get(ptrans);

    if (pf_transport_check(param, ptrans, trans_utype)
        && (utype_can_freely_load(param->utype, trans_utype)
            || tile_has_native_base(ptile, trans_utype))) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Check if there is a safe position to move.
****************************************************************************/
static int get_closest_safe_tile_distance(const struct tile *src_tile,
                                          const struct pf_parameter *param,
                                          int max_distance)
{
  /* This iteration should, according to the documentation in map.h iterate
   * tiles from the center tile, so we stop the iteration to the first found
   * refuel point (as it should be the closest). */
  iterate_outward_dxy(param->map, src_tile, max_distance, ptile, x, y) {
    if (tile_get_known(ptile, param->owner) == TILE_UNKNOWN) {
      /* Cannot guess if the tile is safe */
      continue;
    }
    if (is_possible_base_fuel(ptile, param)) {
      return map_vector_to_real_distance(x, y);
    }
  } iterate_outward_dxy_end;

  return -1;
}

/* ====================  Postion Dangerous Callbacks =================== */

/************************************************************************//**
  Position-dangerous callback for air units.
****************************************************************************/
static int get_fuel_moves_left_req(const struct tile *ptile,
                                   enum known_type known,
                                   const struct pf_parameter *param)
{
  int dist, max;

  if (is_possible_base_fuel(ptile, param)) {
    return 0;
  }

  /* Upper bound for search for refuel point. Sometimes unit can have more
   * moves left than its own move rate due to wonder transfer. Compare
   * pf_moves_left_initially(). */
  max = MAX(param->moves_left_initially
            + (param->fuel_left_initially - 1) * param->move_rate,
            param->move_rate * param->fuel);
  dist = get_closest_safe_tile_distance(ptile, param, max / SINGLE_MOVE);

  return dist != -1 ? dist * SINGLE_MOVE : PF_IMPOSSIBLE_MC;
}

/************************************************************************//**
  Position-dangerous callback for amphibious movement.
****************************************************************************/
static bool amphibious_is_pos_dangerous(const struct tile *ptile,
                                        enum known_type known,
                                        const struct pf_parameter *param)
{
  struct pft_amphibious *amphibious = param->data;
  const bool ferry_move = is_native_tile(amphibious->sea.utype, ptile);

  /* Simply a wrapper for the sea or land danger callbacks. */
  if (ferry_move && amphibious->sea.is_pos_dangerous) {
    return amphibious->sea.is_pos_dangerous(ptile, known, param);
  } else if (!ferry_move && amphibious->land.is_pos_dangerous) {
    return amphibious->land.is_pos_dangerous(ptile, known, param);
  }
  return FALSE;
}

/* =======================  Tools for filling parameters ================= */

/************************************************************************//**
  Fill general use parameters to defaults.
****************************************************************************/
static inline void
pft_fill_default_parameter(struct pf_parameter *parameter,
                           const struct unit_type *punittype)
{
  parameter->map = &(wld.map);
  parameter->get_TB = NULL;
  parameter->get_EC = NULL;
  parameter->is_pos_dangerous = NULL;
  parameter->get_moves_left_req = NULL;
  parameter->get_costs = NULL;
  parameter->get_zoc = NULL;
  parameter->get_move_scope = pf_get_move_scope;
  parameter->get_action = NULL;
  parameter->is_action_possible = NULL;
  parameter->actions = PF_AA_NONE;

  parameter->utype = punittype;
}

/************************************************************************//**
  Enable default actions.
****************************************************************************/
static inline void
pft_enable_default_actions(struct pf_parameter *parameter)
{
  if (!utype_has_flag(parameter->utype, UTYF_CIVILIAN)) {
    parameter->actions |= PF_AA_UNIT_ATTACK;
    parameter->get_action = pf_get_action;
    parameter->is_action_possible = pf_action_possible;
    if (!parameter->omniscience) {
      /* Consider units hided in cities. */
      parameter->actions |= PF_AA_CITY_ATTACK;
    }
  }
  if (utype_may_act_at_all(parameter->utype)) {
    /* FIXME: it should consider action enablers. */
    if (utype_can_do_action(parameter->utype, ACTION_TRADE_ROUTE)
        || utype_can_do_action(parameter->utype, ACTION_MARKETPLACE)) {
      parameter->actions |= PF_AA_TRADE_ROUTE;
    }
    if (utype_can_do_action(parameter->utype, ACTION_SPY_POISON)
        || utype_can_do_action(parameter->utype, ACTION_SPY_POISON_ESC)
        || utype_can_do_action(parameter->utype, ACTION_SPY_SABOTAGE_UNIT)
        || utype_can_do_action(parameter->utype, ACTION_SPY_SABOTAGE_UNIT_ESC)
        || utype_can_do_action(parameter->utype, ACTION_SPY_BRIBE_UNIT)
        || utype_can_do_action(parameter->utype, ACTION_SPY_SABOTAGE_CITY)
        || utype_can_do_action(parameter->utype,
                               ACTION_SPY_SABOTAGE_CITY_ESC)
        || utype_can_do_action(parameter->utype,
                               ACTION_SPY_TARGETED_SABOTAGE_CITY)
        || utype_can_do_action(parameter->utype,
                               ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC)
        || utype_can_do_action(parameter->utype, ACTION_SPY_INCITE_CITY)
        || utype_can_do_action(parameter->utype, ACTION_SPY_INCITE_CITY_ESC)
        || utype_can_do_action(parameter->utype, ACTION_SPY_STEAL_TECH)
        || utype_can_do_action(parameter->utype, ACTION_SPY_STEAL_TECH_ESC)
        || utype_can_do_action(parameter->utype,
                               ACTION_SPY_TARGETED_STEAL_TECH)
        || utype_can_do_action(parameter->utype,
                               ACTION_SPY_TARGETED_STEAL_TECH_ESC)
        || utype_can_do_action(parameter->utype, ACTION_SPY_STEAL_GOLD)
        || utype_can_do_action(parameter->utype, ACTION_SPY_STEAL_GOLD_ESC)
        || utype_can_do_action(parameter->utype, ACTION_STEAL_MAPS)
        || utype_can_do_action(parameter->utype, ACTION_STEAL_MAPS_ESC)
        || utype_can_do_action(parameter->utype, ACTION_SPY_NUKE)
        || utype_can_do_action(parameter->utype, ACTION_SPY_NUKE_ESC)
        || utype_can_do_action(parameter->utype,
                               ACTION_SPY_INVESTIGATE_CITY)
        || utype_can_do_action(parameter->utype,
                               ACTION_INV_CITY_SPEND)
        || utype_can_do_action(parameter->utype,
                               ACTION_ESTABLISH_EMBASSY_STAY)
        || utype_can_do_action(parameter->utype,
                               ACTION_ESTABLISH_EMBASSY)) {
      parameter->actions |= PF_AA_DIPLOMAT;
    }
    parameter->get_action = pf_get_action;
    parameter->is_action_possible = pf_action_possible;
  }
}

/************************************************************************//**
  Fill general use parameters to defaults for an unit type.
****************************************************************************/
static inline void
pft_fill_utype_default_parameter(struct pf_parameter *parameter,
                                 const struct unit_type *punittype,
                                 struct tile *pstart_tile,
                                 struct player *powner)
{
  int veteran_level = get_unittype_bonus(powner, pstart_tile, punittype,
                                         EFT_VETERAN_BUILD);

  if (veteran_level >= utype_veteran_levels(punittype)) {
    veteran_level = utype_veteran_levels(punittype) - 1;
  }

  pft_fill_default_parameter(parameter, punittype);

  parameter->start_tile = pstart_tile;
  parameter->moves_left_initially = punittype->move_rate;
  parameter->move_rate = utype_move_rate(punittype, pstart_tile, powner,
                                         veteran_level, punittype->hp);
  if (utype_fuel(punittype)) {
    parameter->fuel_left_initially = utype_fuel(punittype);
    parameter->fuel = utype_fuel(punittype);
  } else {
    parameter->fuel = 1;
    parameter->fuel_left_initially = 1;
  }
  parameter->transported_by_initially = NULL;
  parameter->cargo_depth = 0;
  BV_CLR_ALL(parameter->cargo_types);
  parameter->owner = powner;

  parameter->omniscience = FALSE;
}

/************************************************************************//**
  Fill general use parameters to defaults for an unit.
****************************************************************************/
static inline void
pft_fill_unit_default_parameter(struct pf_parameter *parameter,
                                const struct unit *punit)
{
  const struct unit *ptrans = unit_transport_get(punit);
  struct unit_type *ptype = unit_type_get(punit);

  pft_fill_default_parameter(parameter, ptype);

  parameter->start_tile = unit_tile(punit);
  parameter->moves_left_initially = punit->moves_left;
  parameter->move_rate = unit_move_rate(punit);
  if (utype_fuel(ptype)) {
    parameter->fuel_left_initially = punit->fuel;
    parameter->fuel = utype_fuel(ptype);
  } else {
    parameter->fuel = 1;
    parameter->fuel_left_initially = 1;
  }
  parameter->transported_by_initially = (NULL != ptrans ? unit_type_get(ptrans)
                                         : NULL);
  parameter->cargo_depth = unit_cargo_depth(punit);
  BV_CLR_ALL(parameter->cargo_types);
  unit_cargo_iterate(punit, pcargo) {
    BV_SET(parameter->cargo_types, utype_index(unit_type_get(pcargo)));
  } unit_cargo_iterate_end;
  parameter->owner = unit_owner(punit);

  parameter->omniscience = FALSE;
}

/************************************************************************//**
  Base function to fill classic parameters.
****************************************************************************/
static inline void pft_fill_parameter(struct pf_parameter *parameter,
                                      const struct unit_type *punittype)
{
  parameter->get_MC = normal_move;
  parameter->ignore_none_scopes = TRUE;
  pft_enable_default_actions(parameter);

  if (!parameter->get_moves_left_req && utype_fuel(punittype)) {
    /* Unit needs fuel */
    parameter->get_moves_left_req = get_fuel_moves_left_req;
  }

  if (!unit_type_really_ignores_zoc(punittype)) {
    parameter->get_zoc = is_my_zoc;
  } else {
    parameter->get_zoc = NULL;
  }
}

/************************************************************************//**
  Fill classic parameters for an unit type.
****************************************************************************/
void pft_fill_utype_parameter(struct pf_parameter *parameter,
                              const struct unit_type *punittype,
                              struct tile *pstart_tile,
                              struct player *pplayer)
{
  pft_fill_utype_default_parameter(parameter, punittype,
                                   pstart_tile, pplayer);
  pft_fill_parameter(parameter, punittype);
}

/************************************************************************//**
  Fill classic parameters for an unit.
****************************************************************************/
void pft_fill_unit_parameter(struct pf_parameter *parameter,
			     const struct unit *punit)
{
  pft_fill_unit_default_parameter(parameter, punit);
  pft_fill_parameter(parameter, unit_type_get(punit));
}

/************************************************************************//**
  pft_fill_*_overlap_param() base function.

  Switch on one tile overlapping into the non-native terrain.
  For sea/land bombardment and for ferries.
****************************************************************************/
static void pft_fill_overlap_param(struct pf_parameter *parameter,
                                   const struct unit_type *punittype)
{
  parameter->get_MC = overlap_move;
  parameter->ignore_none_scopes = FALSE;

  if (!unit_type_really_ignores_zoc(punittype)) {
    parameter->get_zoc = is_my_zoc;
  } else {
    parameter->get_zoc = NULL;
  }

  if (!parameter->get_moves_left_req && utype_fuel(punittype)) {
    /* Unit needs fuel */
    parameter->get_moves_left_req = get_fuel_moves_left_req;
  }
}

/************************************************************************//**
  Switch on one tile overlapping into the non-native terrain.
  For sea/land bombardment and for ferry types.
****************************************************************************/
void pft_fill_utype_overlap_param(struct pf_parameter *parameter,
                                  const struct unit_type *punittype,
                                  struct tile *pstart_tile,
                                  struct player *pplayer)
{
  pft_fill_utype_default_parameter(parameter, punittype,
                                   pstart_tile, pplayer);
  pft_fill_overlap_param(parameter, punittype);
}

/************************************************************************//**
  Switch on one tile overlapping into the non-native terrain.
  For sea/land bombardment and for ferries.
****************************************************************************/
void pft_fill_unit_overlap_param(struct pf_parameter *parameter,
				 const struct unit *punit)
{
  pft_fill_unit_default_parameter(parameter, punit);
  pft_fill_overlap_param(parameter, unit_type_get(punit));
}

/************************************************************************//**
  pft_fill_*_attack_param() base function.

  Consider attacking and non-attacking possibilities properly.
****************************************************************************/
static void pft_fill_attack_param(struct pf_parameter *parameter,
                                  const struct unit_type *punittype)
{
  parameter->get_MC = normal_move;
  parameter->ignore_none_scopes = TRUE;
  pft_enable_default_actions(parameter);
  /* We want known units! */
  parameter->actions &= ~PF_AA_CITY_ATTACK;

  if (!unit_type_really_ignores_zoc(punittype)) {
    parameter->get_zoc = is_my_zoc;
  } else {
    parameter->get_zoc = NULL;
  }

  /* It is too complicated to work with danger here */
  parameter->is_pos_dangerous = NULL;

  if (!parameter->get_moves_left_req && utype_fuel(punittype)) {
    /* Unit needs fuel */
    parameter->get_moves_left_req = get_fuel_moves_left_req;
  }
}

/************************************************************************//**
  pft_fill_*_attack_param() base function.

  Consider attacking and non-attacking possibilities properly.
****************************************************************************/
void pft_fill_utype_attack_param(struct pf_parameter *parameter,
                                 const struct unit_type *punittype,
                                 struct tile *pstart_tile,
                                 struct player *pplayer)
{
  pft_fill_utype_default_parameter(parameter, punittype,
                                   pstart_tile, pplayer);
  pft_fill_attack_param(parameter, punittype);
}

/************************************************************************//**
  pft_fill_*_attack_param() base function.

  Consider attacking and non-attacking possibilities properly.
****************************************************************************/
void pft_fill_unit_attack_param(struct pf_parameter *parameter,
                                const struct unit *punit)
{
  pft_fill_unit_default_parameter(parameter, punit);
  pft_fill_attack_param(parameter, unit_type_get(punit));
}

/************************************************************************//**
  Fill default parameters for reverse map.
****************************************************************************/
void pft_fill_reverse_parameter(struct pf_parameter *parameter,
                                struct tile *target_tile)
{
  memset(parameter, 0, sizeof(*parameter));

  parameter->map = &(wld.map);

  /* We ignore refuel bases in reverse mode. */
  parameter->fuel = 1;
  parameter->fuel_left_initially = 1;

  parameter->get_MC = normal_move;
  parameter->get_move_scope = pf_get_move_scope;
  parameter->ignore_none_scopes = TRUE;

  parameter->get_action = pf_reverse_get_action;
  parameter->data = target_tile;

  /* Other data may stay at zero. */
}

/************************************************************************//**
  Fill parameters for combined sea-land movement.
  This is suitable for the case of a land unit riding a ferry.
  The starting position of the ferry is taken to be the starting position for
  the PF. The passenger is assumed to initailly be on the given ferry.
  The destination may be inland, in which case the passenger will ride
  the ferry to a beach head, disembark, then continue on land.
  One complexity of amphibious movement is that the movement rate on land
  might be different from that at sea. We therefore scale up the movement
  rates (and the corresponding movement consts) to the product of the two
  rates.
****************************************************************************/
void pft_fill_amphibious_parameter(struct pft_amphibious *parameter)
{
  const int move_rate = parameter->land.move_rate * parameter->sea.move_rate;

  parameter->sea.cargo_depth = 1;
  BV_SET(parameter->sea.cargo_types, utype_index(parameter->land.utype));

  parameter->combined = parameter->sea;
  parameter->land_scale = move_rate / parameter->land.move_rate;
  parameter->sea_scale = move_rate / parameter->sea.move_rate;
  parameter->combined.moves_left_initially *= parameter->sea_scale;
  parameter->combined.move_rate = move_rate;
  parameter->combined.get_MC = amphibious_move;
  parameter->combined.get_move_scope = amphibious_move_scope;
  parameter->combined.get_TB = amphibious_behaviour;
  parameter->combined.get_EC = amphibious_extra_cost;
  if (NULL != parameter->land.is_pos_dangerous
      || NULL != parameter->sea.is_pos_dangerous) {
    parameter->combined.is_pos_dangerous = amphibious_is_pos_dangerous;
  } else {
    parameter->combined.is_pos_dangerous = NULL;
  }
  if (parameter->sea.get_moves_left_req != NULL) {
    parameter->combined.get_moves_left_req = parameter->sea.get_moves_left_req;
  } else if (parameter->land.get_moves_left_req != NULL) {
    parameter->combined.get_moves_left_req = parameter->land.get_moves_left_req;
  } else {
    parameter->combined.get_moves_left_req = NULL;
  }
  parameter->combined.get_action = NULL;
  parameter->combined.is_action_possible = NULL;

  parameter->combined.data = parameter;
}
