/***********************************************************************
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
#include <fc_config.h>
#endif

/* utility */
#include "bitvector.h"
#include "log.h"
#include "mem.h"
#include "shared.h"

/* common */
#include "city.h"
#include "combat.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"

/* common/aicore */
#include "citymap.h"
#include "pf_tools.h"

/* server */
#include "barbarian.h"
#include "citytools.h"
#include "cityturn.h"
#include "maphand.h"
#include "plrhand.h"
#include "score.h"
#include "srv_log.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advdata.h"
#include "advgoto.h"
#include "advtools.h"
#include "autosettlers.h"
#include "infracache.h" /* adv_city */

/* ai */
#include "handicaps.h"

/* ai/default */
#include "aidata.h"
#include "aiferry.h"
#include "aiguard.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aitech.h"
#include "aiunit.h"
#include "daimilitary.h"

#include "aitools.h"

/**********************************************************************//**
  Return the (untranslated) rule name of the ai_unit_task.
  You don't have to free the return pointer.
**************************************************************************/
const char *dai_unit_task_rule_name(const enum ai_unit_task task)
{
  switch(task) {
   case AIUNIT_NONE:
     return "None";
   case AIUNIT_AUTO_SETTLER:
     return "Auto settler";
   case AIUNIT_BUILD_CITY:
     return "Build city";
   case AIUNIT_DEFEND_HOME:
     return "Defend home";
   case AIUNIT_ATTACK:
     return "Attack";
   case AIUNIT_ESCORT:
     return "Escort";
   case AIUNIT_EXPLORE:
     return "Explore";
   case AIUNIT_RECOVER:
     return "Recover";
   case AIUNIT_HUNTER:
     return "Hunter";
   case AIUNIT_TRADE:
     return "Trade";
   case AIUNIT_WONDER:
     return "Wonder";
  }
  /* no default, ensure all types handled somehow */
  log_error("Unsupported ai_unit_task %d.", task);
  return NULL;
}

/**********************************************************************//**
  Return the (untranslated) rule name of the adv_choice.
  You don't have to free the return pointer.
**************************************************************************/
const char *dai_choice_rule_name(const struct adv_choice *choice)
{
  switch (choice->type) {
  case CT_NONE:
    return "(nothing)";
  case CT_BUILDING:
    return improvement_rule_name(choice->value.building);
  case CT_CIVILIAN:
  case CT_ATTACKER:
  case CT_DEFENDER:
    return utype_rule_name(choice->value.utype);
  case CT_LAST:
    return "(unknown)";
  };
  /* no default, ensure all types handled somehow */
  log_error("Unsupported ai_unit_task %d.", choice->type);
  return NULL;
}

/**********************************************************************//**
  Amortize a want modified by the shields (build_cost) we risk losing.
  We add the build time of the unit(s) we risk to amortize delay.  The
  build time is calculated as the build cost divided by the production
  output of the unit's homecity or the city where we want to produce
  the unit. If the city has less than average shield output, we
  instead use the average, to encourage long-term thinking.
**************************************************************************/
int military_amortize(struct player *pplayer, struct city *pcity,
                      int value, int delay, int build_cost)
{
  struct adv_data *ai = adv_data_get(pplayer, NULL);
  int city_output = (pcity ? pcity->surplus[O_SHIELD] : 1);
  int output = MAX(city_output, ai->stats.average_production);
  int build_time = build_cost / MAX(output, 1);

  if (value <= 0) {
    return 0;
  }

  return amortize(value, delay + build_time);
}

/**********************************************************************//**
  There are some signs that a player might be dangerous: We are at
  war with him, he has done lots of ignoble things to us, he is an
  ally of one of our enemies (a ticking bomb to be sure), he is
  our war target, we don't like him, diplomatic state is neutral
  or we have case fire.
  This function is used for example to check if pplayer can leave
  his city undefended when aplayer's units are near it.
**************************************************************************/
void dai_consider_plr_dangerous(struct ai_type *ait, struct player *plr1,
                                struct player *plr2,
				enum override_bool *result)
{
  struct ai_dip_intel *adip;

  adip = dai_diplomacy_get(ait, plr1, plr2);

  if (adip->countdown >= 0) {
    /* Don't trust our war target */
    *result = OVERRIDE_TRUE;
  }
}

/**********************************************************************//**
  A helper function for ai_gothere.  Estimates the dangers we will
  be facing at our destination and tries to find/request a bodyguard if
  needed.
**************************************************************************/
static bool dai_gothere_bodyguard(struct ai_type *ait,
                                  struct unit *punit, struct tile *dest_tile)
{
  struct player *pplayer = unit_owner(punit);
  unsigned int danger = 0;
  struct city *dcity;
  struct unit *guard = aiguard_guard_of(ait, punit);
  const struct veteran_level *vlevel;
  bool bg_needed = FALSE;

  if (is_barbarian(unit_owner(punit))) {
    /* barbarians must have more courage (ie less brains) */
    aiguard_clear_guard(ait, punit);
    return FALSE;
  }

  /* Estimate enemy attack power. */
  unit_list_iterate(dest_tile->units, aunit) {
    if (POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, unit_owner(aunit))) {
      danger += adv_unit_att_rating(aunit);
    }
  } unit_list_iterate_end;
  dcity = tile_city(dest_tile);
  if (dcity && POTENTIALLY_HOSTILE_PLAYER(ait, pplayer, city_owner(dcity))) {
    /* Assume enemy will build another defender, add it's attack strength */
    struct unit_type *d_type = dai_choose_defender_versus(dcity, punit);

    if (d_type) {
      /* Enemy really can build something */
      danger +=
        adv_unittype_att_rating(d_type, do_make_unit_veteran(dcity, d_type), 
                                SINGLE_MOVE, d_type->hp);
    }
  }
  danger *= POWER_DIVIDER;

  /* If we are fast, there is less danger.
   * FIXME: that assumes that most units have move_rate == SINGLE_MOVE;
   * not true for all rulesets */
  danger /= (unit_type_get(punit)->move_rate / SINGLE_MOVE);
  if (unit_has_type_flag(punit, UTYF_IGTER)) {
    danger /= 1.5;
  }

  vlevel = utype_veteran_level(unit_type_get(punit), punit->veteran);
  fc_assert_ret_val(vlevel != NULL, FALSE);

  /* We look for the bodyguard where we stand. */
  if (guard == NULL || unit_tile(guard) != unit_tile(punit)) {
    int my_def = (punit->hp * unit_type_get(punit)->defense_strength
                  * POWER_FACTOR * vlevel->power_fact / 100);

    if (danger >= my_def) {
      UNIT_LOG(LOGLEVEL_BODYGUARD, punit, 
               "want bodyguard @(%d, %d) danger=%d, my_def=%d", 
               TILE_XY(dest_tile), danger, my_def);
      aiguard_request_guard(ait, punit);
      bg_needed = TRUE;
    } else {
      aiguard_clear_guard(ait, punit);
      bg_needed = FALSE;
    }
  } else if (guard != NULL) {
    bg_needed = TRUE;
  }

  /* What if we have a bodyguard, but don't need one? */

  return bg_needed;
}

#define LOGLEVEL_GOTHERE LOG_DEBUG
/**********************************************************************//**
  This is ferry-enabled goto.  Should not normally be used for non-ferried
  units (i.e. planes or ships), use dai_unit_goto instead.

  Return values: TRUE if got to or next to our destination, FALSE otherwise.

  TODO: A big one is rendezvous points.  When this is implemented, we won't
  have to be at the coast to ask for a boat to come to us.
**************************************************************************/
bool dai_gothere(struct ai_type *ait, struct player *pplayer,
                 struct unit *punit, struct tile *dest_tile)
{
  CHECK_UNIT(punit);
  bool bg_needed;

  if (same_pos(dest_tile, unit_tile(punit)) || punit->moves_left <= 0) {
    /* Nowhere to go */
    return TRUE;
  }

  /* See if we need a bodyguard at our destination */
  /* FIXME: If bodyguard is _really_ necessary, don't go anywhere */
  bg_needed = dai_gothere_bodyguard(ait, punit, dest_tile);

  if (unit_transported(punit)
      || !goto_is_sane(punit, dest_tile)) {
    /* Must go by boat, call an aiferryboat function */
    if (!aiferry_gobyboat(ait, pplayer, punit, dest_tile,
                          bg_needed)) {
      return FALSE;
    }
  }

  /* Go where we should be going if we can, and are at our destination 
   * if we are on a ferry */
  if (goto_is_sane(punit, dest_tile) && punit->moves_left > 0) {
    punit->goto_tile = dest_tile;
    UNIT_LOG(LOGLEVEL_GOTHERE, punit, "Walking to (%d,%d)", TILE_XY(dest_tile));
    if (!dai_unit_goto(ait, punit, dest_tile)) {
      /* died */
      return FALSE;
    }
    /* liable to bump into someone that will kill us.  Should avoid? */
  } else {
    UNIT_LOG(LOGLEVEL_GOTHERE, punit, "Not moving");
    return FALSE;
  }

  if (def_ai_unit_data(punit, ait)->ferryboat > 0
      && !unit_transported(punit)) {
    /* We probably just landed, release our boat */
    aiferry_clear_boat(ait, punit);
  }
  
  /* Dead unit shouldn't reach this point */
  CHECK_UNIT(punit);

  return (same_pos(unit_tile(punit), dest_tile)
          || is_tiles_adjacent(unit_tile(punit), dest_tile));
}

/**********************************************************************//**
  Returns the destination for a unit moving towards a given final destination.
  That is, it gives a suitable way-point, if necessary.
  For example, aircraft need these way-points to refuel.
**************************************************************************/
struct tile *immediate_destination(struct unit *punit,
                                   struct tile *dest_tile)
{
  if (!same_pos(unit_tile(punit), dest_tile)
      && utype_fuel(unit_type_get(punit))) {
    struct pf_parameter parameter;
    struct pf_map *pfm;
    struct pf_path *path;
    size_t i;
    struct player *pplayer = unit_owner(punit);

    pft_fill_unit_parameter(&parameter, punit);
    parameter.omniscience = !has_handicap(pplayer, H_MAP);
    pfm = pf_map_new(&parameter);
    path = pf_map_path(pfm, punit->goto_tile);

    if (path) {
      for (i = 1; i < path->length; i++) {
        if (path->positions[i].tile == path->positions[i - 1].tile) {
          /* The path-finding code advices us to wait there to refuel. */
          struct tile *ptile = path->positions[i].tile;

          pf_path_destroy(path);
          pf_map_destroy(pfm);
          return ptile;
        }
      }
      pf_path_destroy(path);
      pf_map_destroy(pfm);
      /* Seems it's the immediate destination */
      return punit->goto_tile;
    }

    pf_map_destroy(pfm);
    log_verbose("Did not find an air-route for "
                "%s %s[%d] (%d,%d)->(%d,%d)",
                nation_rule_name(nation_of_unit(punit)),
                unit_rule_name(punit),
                punit->id,
                TILE_XY(unit_tile(punit)),
                TILE_XY(dest_tile));
    /* Prevent take off */
    return unit_tile(punit);
  }

  /* else does not need way-points */
  return dest_tile;
}

/**********************************************************************//**
  Log the cost of travelling a path.
**************************************************************************/
void dai_log_path(struct unit *punit,
                  struct pf_path *path, struct pf_parameter *parameter)
{
  const struct pf_position *last = pf_path_last_position(path);
  const int cc = PF_TURN_FACTOR * last->total_MC
                 + parameter->move_rate * last->total_EC;
  const int tc = cc / (PF_TURN_FACTOR *parameter->move_rate); 

  UNIT_LOG(LOG_DEBUG, punit, "path L=%d T=%d(%d) MC=%d EC=%d CC=%d",
	   path->length - 1, last->turn, tc,
	   last->total_MC, last->total_EC, cc);
}

/**********************************************************************//**
  Go to specified destination, subject to given PF constraints,
  but do not disturb existing role or activity
  and do not clear the role's destination. Return FALSE iff we died.

  parameter: the PF constraints on the computed path. The unit will move
  as far along the computed path is it can; the movement code will impose
  all the real constraints (ZoC, etc).
**************************************************************************/
bool dai_unit_goto_constrained(struct ai_type *ait, struct unit *punit,
                               struct tile *ptile,
                               struct pf_parameter *parameter)
{
  bool alive = TRUE;
  struct pf_map *pfm;
  struct pf_path *path;

  UNIT_LOG(LOG_DEBUG, punit, "constrained goto to %d,%d", TILE_XY(ptile));

  ptile = immediate_destination(punit, ptile);

  UNIT_LOG(LOG_DEBUG, punit, "constrained goto: let's go to %d,%d",
           TILE_XY(ptile));

  if (same_pos(unit_tile(punit), ptile)) {
    /* Not an error; sometimes immediate_destination instructs the unit
     * to stay here. For example, to refuel.*/
    UNIT_LOG(LOG_DEBUG, punit, "constrained goto: already there!");
    send_unit_info(NULL, punit);

    return TRUE;
  } else if (!goto_is_sane(punit, ptile)) {
    UNIT_LOG(LOG_DEBUG, punit, "constrained goto: 'insane' goto!");
    punit->activity = ACTIVITY_IDLE;
    send_unit_info(NULL, punit);

    return TRUE;
  } else if (punit->moves_left == 0) {
    UNIT_LOG(LOG_DEBUG, punit, "constrained goto: no moves left!");
    send_unit_info(NULL, punit);

    return TRUE;
  }

  pfm = pf_map_new(parameter);
  path = pf_map_path(pfm, ptile);

  if (path) {
    dai_log_path(punit, path, parameter);
    UNIT_LOG(LOG_DEBUG, punit, "constrained goto: following path.");
    alive = adv_follow_path(punit, path, ptile);
  } else {
    UNIT_LOG(LOG_DEBUG, punit, "no path to destination");
  }

  pf_path_destroy(path);
  pf_map_destroy(pfm);

  return alive;
}

/**********************************************************************//**
  Use pathfinding to determine whether a GOTO is possible, considering all
  aspects of the unit being moved and the terrain under consideration.
  Don't bother with pathfinding if the unit is already there.
**************************************************************************/
bool goto_is_sane(struct unit *punit, struct tile *ptile)
{
  bool can_get_there = FALSE;

  if (same_pos(unit_tile(punit), ptile)) {
    can_get_there = TRUE;
  } else {
    struct pf_parameter parameter;
    struct pf_map *pfm;

    pft_fill_unit_attack_param(&parameter, punit);
    pfm = pf_map_new(&parameter);

    if (pf_map_move_cost(pfm, ptile) != PF_IMPOSSIBLE_MC) {
      can_get_there = TRUE;
    }
    pf_map_destroy(pfm);
  }
  return can_get_there;
}

/*
 * The length of time, in turns, which is long enough to be optimistic
 * that enemy units will have moved from their current position.
 * WAG
 */
#define LONG_TIME 4
/**********************************************************************//**
  Set up the constraints on a path for an AI unit.

  parameter:
     constraints (output)
  risk_cost:
     auxiliary data used by the constraints (output)
  ptile:
     the destination of the unit.
     For ferries, the destination may be a coastal land tile,
     in which case the ferry should stop on an adjacent tile.
**************************************************************************/
void dai_fill_unit_param(struct ai_type *ait, struct pf_parameter *parameter,
                         struct adv_risk_cost *risk_cost,
                         struct unit *punit, struct tile *ptile)
{
  const bool long_path = LONG_TIME < (map_distance(unit_tile(punit),
                                                   unit_tile(punit))
                                      * SINGLE_MOVE
                                      / unit_type_get(punit)->move_rate);
  const bool barbarian = is_barbarian(unit_owner(punit));
  bool is_ferry;
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);
  struct player *pplayer = unit_owner(punit);

  /* This function is now always omniscient and should not be used
   * for human players any more. */
  fc_assert(is_ai(pplayer));

  /* If a unit is hunting, don't expect it to be a ferry. */
  is_ferry = (unit_data->task != AIUNIT_HUNTER
              && dai_is_ferry(punit, ait));

  if (is_ferry) {
    /* The destination may be a coastal land tile,
     * in which case the ferry should stop on an adjacent tile. */
    pft_fill_unit_overlap_param(parameter, punit);
  } else if (!utype_fuel(unit_type_get(punit))
             && is_military_unit(punit)
             && (unit_data->task == AIUNIT_DEFEND_HOME
                 || unit_data->task == AIUNIT_ATTACK
                 || unit_data->task ==  AIUNIT_ESCORT
                 || unit_data->task == AIUNIT_HUNTER)) {
    /* Use attack movement for defenders and escorts so they can
     * make defensive attacks */
    pft_fill_unit_attack_param(parameter, punit);
  } else {
    pft_fill_unit_parameter(parameter, punit);
  }
  parameter->omniscience = !has_handicap(pplayer, H_MAP);

  /* Should we use the risk avoidance code?
   * The risk avoidance code uses omniscience, so do not use for
   * human-player units under temporary AI control.
   * Barbarians bravely/stupidly ignore risks
   */
  if (!uclass_has_flag(unit_class_get(punit), UCF_UNREACHABLE)
      && !barbarian) {
    adv_avoid_risks(parameter, risk_cost, punit, NORMAL_STACKING_FEARFULNESS);
  }

  /* Should we absolutely forbid ending a turn on a dangerous tile?
   * Do not annoy human players by killing their units for them.
   * For AI units be optimistic; allows attacks across dangerous terrain,
   * and polar settlements.
   * TODO: This is compatible with old code,
   * but probably ought to be more cautious for non military units
   */
  if (!is_ferry && !utype_fuel(unit_type_get(punit))) {
    parameter->get_moves_left_req = NULL;
  }

  if (long_path) {
    /* Move as far along the path to the destination as we can;
     * that is, ignore the presence of enemy units when computing the
     * path.
     * Hopefully, ai_avoid_risks will have produced a path that avoids enemy
     * ZoCs. Ignoring ZoCs allows us to move closer to a destination
     * for which there is not yet a clear path.
     * That is good if the destination is several turns away,
     * so we can reasonably expect blocking enemy units to move or
     * be destroyed. But it can be bad if the destination is one turn away
     * or our destination is far but there are enemy units near us and on the
     * shortest path to the destination.
     */
    parameter->get_zoc = NULL;
  }

  if (unit_has_type_flag(punit, UTYF_SETTLERS)) {
    parameter->get_TB = no_fights;
  } else if (long_path && unit_is_cityfounder(punit)) {
    /* Default tile behaviour;
     * move as far along the path to the destination as we can;
     * that is, ignore the presence of enemy units when computing the
     * path.
     */
  } else if (unit_is_cityfounder(punit)) {
    /* Short path */
    parameter->get_TB = no_fights;
  } else if (unit_has_type_role(punit, L_BARBARIAN_LEADER)) {
    /* Avoid capture */
    parameter->get_TB = no_fights;
  } else if (is_ferry) {
    /* Ferries are not warships */
    parameter->get_TB = no_fights;
  } else if (is_losing_hp(punit)) {
    /* Losing hitpoints over time (helicopter in default rules) */
    /* Default tile behaviour */
  } else if (utype_may_act_at_all(unit_type_get(punit))) {
    switch (unit_data->task) {
    case AIUNIT_AUTO_SETTLER:
    case AIUNIT_BUILD_CITY:
      /* Strange, but not impossible */
      parameter->get_TB = no_fights;
      break;
    case AIUNIT_DEFEND_HOME:
    case AIUNIT_ATTACK: /* Includes spy actions */
    case AIUNIT_ESCORT:
    case AIUNIT_HUNTER:
    case AIUNIT_TRADE:
    case AIUNIT_WONDER:
      parameter->get_TB = no_intermediate_fights;
      break;
    case AIUNIT_EXPLORE:
    case AIUNIT_RECOVER:
      parameter->get_TB = no_fights;
      break;
    case AIUNIT_NONE:
      /* Default tile behaviour */
      break;
    }
  } else {
    /* Probably an explorer */
    parameter->get_TB = no_fights;
  }

  if (is_ferry) {
    /* Show the destination in the client when watching an AI: */
    punit->goto_tile = ptile;
  }
}

/**********************************************************************//**
  Go to specified destination but do not disturb existing role or activity
  and do not clear the role's destination. Return FALSE iff we died.
**************************************************************************/
bool dai_unit_goto(struct ai_type *ait, struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct adv_risk_cost risk_cost;

  UNIT_LOG(LOG_DEBUG, punit, "dai_unit_goto to %d,%d", TILE_XY(ptile));
  dai_fill_unit_param(ait, &parameter, &risk_cost, punit, ptile);

  return dai_unit_goto_constrained(ait, punit, ptile, &parameter);
}

/**********************************************************************//**
  Adviser task for unit has been changed.
**************************************************************************/
void dai_unit_new_adv_task(struct ai_type *ait, struct unit *punit,
                           enum adv_unit_task task, struct tile *ptile)
{
  /* Keep ai_unit_task in sync with adv task */
  switch(task) {
   case AUT_AUTO_SETTLER:
     dai_unit_new_task(ait, punit, AIUNIT_AUTO_SETTLER, ptile);
     break;
   case AUT_BUILD_CITY:
     dai_unit_new_task(ait, punit, AIUNIT_BUILD_CITY, ptile);
     break;
   case AUT_NONE:
     dai_unit_new_task(ait, punit, AIUNIT_NONE, ptile);
     break;
  }
}

/**********************************************************************//**
  Ensure unit sanity by telling charge that we won't bodyguard it anymore,
  tell bodyguard it can roam free if our job is done, add and remove city
  spot reservation, and set destination. If we set a unit to hunter, also
  reserve its target, and try to load it with cruise missiles or nukes
  to bring along.
**************************************************************************/
void dai_unit_new_task(struct ai_type *ait, struct unit *punit,
                       enum ai_unit_task task, struct tile *ptile)
{
  struct unit *bodyguard = aiguard_guard_of(ait, punit);
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  /* If the unit is under (human) orders we shouldn't control it.
   * Allow removal of old role with AIUNIT_NONE. */
  fc_assert_ret(!unit_has_orders(punit) || task == AIUNIT_NONE);

  UNIT_LOG(LOG_DEBUG, punit, "changing task from %s to %s",
           dai_unit_task_rule_name(unit_data->task),
           dai_unit_task_rule_name(task));

  /* Free our ferry.  Most likely it has been done already. */
  if (task == AIUNIT_NONE || task == AIUNIT_DEFEND_HOME) {
    aiferry_clear_boat(ait, punit);
  }

  if (punit->activity == ACTIVITY_GOTO) {
    /* It would indicate we're going somewhere otherwise */
    unit_activity_handling(punit, ACTIVITY_IDLE);
  }

  if (unit_data->task == AIUNIT_BUILD_CITY) {
    if (punit->goto_tile) {
      citymap_free_city_spot(punit->goto_tile, punit->id);
    } else {
      /* Print error message instead of crashing in citymap_free_city_spot()
       * This probably means that some city spot reservation has not been
       * properly cleared; bad for the AI, as it will leave that area
       * uninhabited. */
      log_error("%s was on city founding mission without target tile.",
                unit_rule_name(punit));
    }
  }

  if (unit_data->task == AIUNIT_HUNTER) {
    /* Clear victim's hunted bit - we're no longer chasing. */
    struct unit *target = game_unit_by_number(unit_data->target);

    if (target) {
      BV_CLR(def_ai_unit_data(target, ait)->hunted,
             player_index(unit_owner(punit)));
      UNIT_LOG(LOGLEVEL_HUNT, target, "no longer hunted (new task %d, old %d)",
               task, unit_data->task);
    }
  }

  aiguard_clear_charge(ait, punit);
  /* Record the city to defend; our goto may be to transport. */
  if (task == AIUNIT_DEFEND_HOME && ptile && tile_city(ptile)) {
    aiguard_assign_guard_city(ait, tile_city(ptile), punit);
  }

  unit_data->task = task;

  /* Verify and set the goto destination.  Eventually this can be a lot more
   * stringent, but for now we don't want to break things too badly. */
  punit->goto_tile = ptile; /* May be NULL. */

  if (unit_data->task == AIUNIT_NONE && bodyguard) {
    dai_unit_new_task(ait, bodyguard, AIUNIT_NONE, NULL);
  }

  /* Reserve city spot, _unless_ we want to add ourselves to a city. */
  if (unit_data->task == AIUNIT_BUILD_CITY && !tile_city(ptile)) {
    citymap_reserve_city_spot(ptile, punit->id);
  }
  if (unit_data->task == AIUNIT_HUNTER) {
    /* Set victim's hunted bit - the hunt is on! */
    struct unit *target = game_unit_by_number(unit_data->target);

    fc_assert_ret(target != NULL);
    BV_SET(def_ai_unit_data(target, ait)->hunted, player_index(unit_owner(punit)));
    UNIT_LOG(LOGLEVEL_HUNT, target, "is being hunted");

    /* Grab missiles lying around and bring them along */
    unit_list_iterate(unit_tile(punit)->units, missile) {
      if (unit_owner(missile) == unit_owner(punit)
          && def_ai_unit_data(missile, ait)->task != AIUNIT_ESCORT
          && !unit_transported(missile)
          && unit_owner(missile) == unit_owner(punit)
          && utype_can_do_action(unit_type_get(missile),
                                 ACTION_SUICIDE_ATTACK)
          && can_unit_load(missile, punit)) {
        UNIT_LOG(LOGLEVEL_HUNT, missile, "loaded on hunter");
        dai_unit_new_task(ait, missile, AIUNIT_ESCORT, unit_tile(target));
        unit_transport_load_send(missile, punit);
      }
    } unit_list_iterate_end;
  }

  /* Map ai tasks to advisor tasks. For most ai tasks there is
     no advisor, so AUT_NONE is set. */
  switch(unit_data->task) {
   case AIUNIT_AUTO_SETTLER:
     punit->server.adv->task = AUT_AUTO_SETTLER;
     break;
   case AIUNIT_BUILD_CITY:
     punit->server.adv->task = AUT_BUILD_CITY;
     break;
   default:
     punit->server.adv->task = AUT_NONE;
     break;
  }
}

/**********************************************************************//**
  Try to make pcity our new homecity. Fails if we can't upkeep it. Assumes
  success from server.
**************************************************************************/
bool dai_unit_make_homecity(struct unit *punit, struct city *pcity)
{
  CHECK_UNIT(punit);
  fc_assert_ret_val(unit_owner(punit) == city_owner(pcity), TRUE);

  if (punit->homecity == 0 && !unit_has_type_role(punit, L_EXPLORER)) {
    /* This unit doesn't pay any upkeep while it doesn't have a homecity,
     * so it would be stupid to give it one. There can also be good reasons
     * why it doesn't have a homecity. */
    /* However, until we can do something more useful with them, we
       will assign explorers to a city so that they can be disbanded for 
       the greater good -- Per */
    return FALSE;
  }
  if (pcity->surplus[O_SHIELD] >= unit_type_get(punit)->upkeep[O_SHIELD]
      && pcity->surplus[O_FOOD] >= unit_type_get(punit)->upkeep[O_FOOD]) {
    unit_do_action(unit_owner(punit), punit->id, pcity->id,
                   0, "", ACTION_HOME_CITY);
    return TRUE;
  }
  return FALSE;
}

/**********************************************************************//**
  Move a bodyguard along with another unit. We assume that unit has already
  been moved to ptile which is a valid, safe tile, and that our
  bodyguard has not. This is an ai_unit_* auxiliary function, do not use
  elsewhere.
**************************************************************************/
static void dai_unit_bodyguard_move(struct ai_type *ait,
                                    struct unit *bodyguard, struct tile *ptile)
{
  struct unit *punit;
  struct player *pplayer;

  fc_assert_ret(bodyguard != NULL);
  pplayer = unit_owner(bodyguard);
  fc_assert_ret(pplayer != NULL);
  punit = aiguard_charge_unit(ait, bodyguard);
  fc_assert_ret(punit != NULL);

  CHECK_GUARD(ait, bodyguard);
  CHECK_CHARGE_UNIT(ait, punit);

  if (!is_tiles_adjacent(ptile, unit_tile(bodyguard))) {
    return;
  }

  if (bodyguard->moves_left <= 0) {
    /* should generally should not happen */
    BODYGUARD_LOG(ait, LOG_DEBUG, bodyguard, "was left behind by charge");
    return;
  }

  unit_activity_handling(bodyguard, ACTIVITY_IDLE);
  (void) dai_unit_move(ait, bodyguard, ptile);
}

/**********************************************************************//**
  Move and attack with an ai unit. We do not wait for server reply.
**************************************************************************/
bool dai_unit_attack(struct ai_type *ait, struct unit *punit, struct tile *ptile)
{
  struct unit *bodyguard = aiguard_guard_of(ait, punit);
  int sanity = punit->id;
  bool alive;
  struct city *tcity;

  CHECK_UNIT(punit);
  fc_assert_ret_val(is_ai(unit_owner(punit)), TRUE);
  fc_assert_ret_val(is_tiles_adjacent(unit_tile(punit), ptile), TRUE);

  unit_activity_handling(punit, ACTIVITY_IDLE);
  /* FIXME: try the next action if the unit tried to do an illegal action.
   * That would allow the AI to stop using the omniscient
   * is_action_enabled_unit_on_*() functions. */
  if (is_action_enabled_unit_on_units(ACTION_CAPTURE_UNITS,
                                      punit, ptile)) {
    /* Choose capture. */
    unit_do_action(unit_owner(punit), punit->id, tile_index(ptile),
                   0, "", ACTION_CAPTURE_UNITS);
  } else if (is_action_enabled_unit_on_units(ACTION_BOMBARD,
                                        punit, ptile)) {
    /* Choose bombard. */
    unit_do_action(unit_owner(punit), punit->id, tile_index(ptile),
                   0, "", ACTION_BOMBARD);
  } else if (is_action_enabled_unit_on_tile(ACTION_NUKE,
                                            punit, ptile, NULL)) {
    /* Choose explode nuclear. */
    unit_do_action(unit_owner(punit), punit->id, tile_index(ptile),
                   0, "", ACTION_NUKE);
  } else if (is_action_enabled_unit_on_units(ACTION_ATTACK,
                                             punit, ptile)) {
    /* Choose regular attack. */
    unit_do_action(unit_owner(punit), punit->id, tile_index(ptile),
                   0, "", ACTION_ATTACK);
  } else if (is_action_enabled_unit_on_units(ACTION_SUICIDE_ATTACK,
                                             punit, ptile)) {
    /* Choose suicide attack (explode missile). */
    unit_do_action(unit_owner(punit), punit->id, tile_index(ptile),
                   0, "", ACTION_SUICIDE_ATTACK);
  } else if ((tcity = tile_city(ptile))
             && is_action_enabled_unit_on_city(ACTION_CONQUER_CITY,
                                               punit, tcity)) {
    /* Choose "Conquer City". */
    unit_do_action(unit_owner(punit), punit->id, tcity->id,
                   0, "", ACTION_CONQUER_CITY);
  } else {
    /* Other move. */
    (void) unit_move_handling(punit, ptile, FALSE, TRUE, NULL);
  }
  alive = (game_unit_by_number(sanity) != NULL);

  if (alive && same_pos(ptile, unit_tile(punit))
      && bodyguard != NULL  && def_ai_unit_data(bodyguard, ait)->charge == punit->id) {
    dai_unit_bodyguard_move(ait, bodyguard, ptile);
    /* Clumsy bodyguard might trigger an auto-attack */
    alive = (game_unit_by_number(sanity) != NULL);
  }

  return alive;
}

/**********************************************************************//**
  Ai unit moving function called from AI interface.
**************************************************************************/
void dai_unit_move_or_attack(struct ai_type *ait, struct unit *punit,
                             struct tile *ptile, struct pf_path *path, int step)
{
  if (step == path->length - 1) {
    (void) dai_unit_attack(ait, punit, ptile);
  } else {
    (void) dai_unit_move(ait, punit, ptile);
  }
}

/**********************************************************************//**
  Move a unit. Do not attack. Do not leave bodyguard.
  For AI units.

  This function returns only when we have a reply from the server and
  we can tell the calling function what happened to the move request.
  (Right now it is not a big problem, since we call the server directly.)
**************************************************************************/
bool dai_unit_move(struct ai_type *ait, struct unit *punit, struct tile *ptile)
{
  struct unit *bodyguard;
  int sanity = punit->id;
  struct player *pplayer = unit_owner(punit);
  const bool is_plr_ai = is_ai(pplayer);
  int mcost;

  CHECK_UNIT(punit);
  fc_assert_ret_val_msg(is_tiles_adjacent(unit_tile(punit), ptile), FALSE,
                        "Tiles not adjacent: Unit = %d, "
                        "from = (%d, %d]) to = (%d, %d).",
                        punit->id, TILE_XY(unit_tile(punit)),
                        TILE_XY(ptile));

  /* if enemy, stop and give a chance for the ai attack function
   * to handle this case */
  if (is_enemy_unit_tile(ptile, pplayer)
      || is_enemy_city_tile(ptile, pplayer)) {
    UNIT_LOG(LOG_DEBUG, punit, "movement halted due to enemy presence");
    return FALSE;
  }

  /* barbarians shouldn't enter huts */
  if (is_barbarian(pplayer) && tile_has_cause_extra(ptile, EC_HUT)) {
    return FALSE;
  }

  /* don't leave bodyguard behind */
  if (is_plr_ai
      && (bodyguard = aiguard_guard_of(ait, punit))
      && same_pos(unit_tile(punit), unit_tile(bodyguard))
      && bodyguard->moves_left == 0) {
    UNIT_LOG(LOGLEVEL_BODYGUARD, punit, "does not want to leave "
             "its bodyguard");
    return FALSE;
  }

  /* Try not to end move next to an enemy if we can avoid it by waiting */
  mcost = map_move_cost_unit(&(wld.map), punit, ptile);
  if (punit->moves_left <= mcost
      && unit_move_rate(punit) > mcost
      && adv_danger_at(punit, ptile)
      && !adv_danger_at(punit,  unit_tile(punit))) {
    UNIT_LOG(LOG_DEBUG, punit, "ending move early to stay out of trouble");
    return FALSE;
  }

  /* go */
  unit_activity_handling(punit, ACTIVITY_IDLE);
  /* Move */
  (void) unit_move_handling(punit, ptile, FALSE, TRUE, NULL);

  /* handle the results */
  if (game_unit_by_number(sanity) && same_pos(ptile, unit_tile(punit))) {
    bodyguard = aiguard_guard_of(ait, punit);

    if (is_plr_ai && bodyguard != NULL
        && def_ai_unit_data(bodyguard, ait)->charge == punit->id) {
      dai_unit_bodyguard_move(ait, bodyguard, ptile);
    }
    return TRUE;
  }
  return FALSE;
}

/**********************************************************************//**
  Calculate the value of the target unit including the other units which
  will die in a successful attack
**************************************************************************/
int stack_cost(struct unit *pattacker, struct unit *pdefender)
{
  struct tile *ptile = unit_tile(pdefender);
  int victim_cost = 0;

  if (is_stack_vulnerable(ptile)) {
    /* lotsa people die */
    unit_list_iterate(ptile->units, aunit) {
      if (unit_attack_unit_at_tile_result(pattacker, aunit, ptile) == ATT_OK) {
        victim_cost += unit_build_shield_cost_base(aunit);
      }
    } unit_list_iterate_end;
  } else if (unit_attack_unit_at_tile_result(pattacker, pdefender, ptile) == ATT_OK) {
    /* Only one unit dies if attack is successful */
    victim_cost = unit_build_shield_cost_base(pdefender);
  }

  return victim_cost;
}

/**********************************************************************//**
  Change government, pretty fast...
**************************************************************************/
void dai_government_change(struct player *pplayer, struct government *gov)
{
  if (gov == government_of_player(pplayer)) {
    return;
  }

  handle_player_change_government(pplayer, government_number(gov));

  city_list_iterate(pplayer->cities, pcity) {
    auto_arrange_workers(pcity); /* update cities */
  } city_list_iterate_end;
}

/**********************************************************************//**
  Credits the AI wants to have in reserves. We need some gold to bribe
  and incite cities.

  "I still don't trust this function" -- Syela
**************************************************************************/
int dai_gold_reserve(struct player *pplayer)
{
  int i = total_player_citizens(pplayer) * 2;

  return MAX(pplayer->ai_common.maxbuycost, i);
}

/**********************************************************************//**
  Adjust want for choice to 'value' percent
**************************************************************************/
void adjust_choice(int value, struct adv_choice *choice)
{
  choice->want = (choice->want *value)/100;
}

/**********************************************************************//**
  Calls dai_wants_role_unit to choose the best unit with the given role and
  set tech wants.  Sets choice->value.utype when we can build something.
**************************************************************************/
bool dai_choose_role_unit(struct ai_type *ait, struct player *pplayer,
                          struct city *pcity, struct adv_choice *choice,
                          enum choice_type type, int role, int want,
                          bool need_boat)
{
  struct unit_type *iunit = dai_wants_role_unit(ait, pplayer, pcity, role, want);

  if (iunit != NULL) {
    choice->type = type;
    choice->value.utype = iunit;
    choice->want = want;

    choice->need_boat = need_boat;

    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Consider overriding building target selected by common advisor code.
**************************************************************************/
void dai_build_adv_override(struct ai_type *ait, struct city *pcity,
                            struct adv_choice *choice)
{
  struct impr_type *chosen;
  int want;

  if (choice->type == CT_NONE) {
    want = 0;
    chosen = NULL;
  } else {
    want = choice->want;
    chosen = choice->value.building;
  }

  improvement_iterate(pimprove) {
    /* Advisor code did not consider wonders, let's do it here */
    if (is_wonder(pimprove)) {
      if (pcity->server.adv->building_want[improvement_index(pimprove)] > want
          && can_city_build_improvement_now(pcity, pimprove)) {
        want = pcity->server.adv->building_want[improvement_index(pimprove)];
        chosen = pimprove;
      }
    }
  } improvement_iterate_end;

  choice->want = want;
  choice->value.building = chosen;

  if (chosen) {
    choice->type = CT_BUILDING; /* In case advisor had not chosen anything */

    CITY_LOG(LOG_DEBUG, pcity, "ai wants most to build %s at %d",
             improvement_rule_name(chosen),
             want);
  }
}

/**********************************************************************//**
  "The following evaluates the unhappiness caused by military units
  in the field (or aggressive) at a city when at Republic or
  Democracy.

  Now generalised somewhat for government rulesets, though I'm not
  sure whether it is fully general for all possible parameters/
  combinations." --dwp
**************************************************************************/
bool dai_assess_military_unhappiness(struct city *pcity)
{
  int free_unhappy = get_city_bonus(pcity, EFT_MAKE_CONTENT_MIL);
  int unhap = 0;

  /* bail out now if happy_cost is 0 */
  if (get_player_bonus(city_owner(pcity), EFT_UNHAPPY_FACTOR) == 0) {
    return FALSE;
  }

  unit_list_iterate(pcity->units_supported, punit) {
    int happy_cost = city_unit_unhappiness(punit, &free_unhappy);

    if (happy_cost > 0) {
      unhap += happy_cost;
    }
  } unit_list_iterate_end;
 
  if (unhap < 0) {
    unhap = 0;
  }
  return (unhap > 0);
}
