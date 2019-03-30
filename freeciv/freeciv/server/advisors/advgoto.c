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

/* common */
#include "ai.h"
#include "combat.h"
#include "game.h"
#include "movement.h"
#include "unit.h"
#include "tile.h"

/* aicore */
#include "path_finding.h"

/* server */
#include "maphand.h"
#include "srv_log.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advtools.h"

#include "advgoto.h"

static bool adv_unit_move(struct unit *punit, struct tile *ptile);

/**********************************************************************//**
  Move a unit along a path without disturbing its activity, role
  or assigned destination
  Return FALSE iff we died.
**************************************************************************/
bool adv_follow_path(struct unit *punit, struct pf_path *path,
                     struct tile *ptile)
{
  struct tile *old_tile = punit->goto_tile;
  enum unit_activity activity = punit->activity;
  struct extra_type *tgt = punit->activity_target;
  bool alive;

  if (punit->moves_left <= 0) {
    return TRUE;
  }
  punit->goto_tile = ptile;
  unit_activity_handling(punit, ACTIVITY_GOTO);
  alive = adv_unit_execute_path(punit, path);
  if (alive) {
    if (activity != ACTIVITY_GOTO) {
      /* Only go via ACTIVITY_IDLE if we are actually changing the activity */
      unit_activity_handling(punit, ACTIVITY_IDLE);
      send_unit_info(NULL, punit); /* FIXME: probably duplicate */
      unit_activity_handling_targeted(punit, activity, &tgt);
    }
    punit->goto_tile = old_tile; /* May be NULL. */
    send_unit_info(NULL, punit);
  }
  return alive;
}


/**********************************************************************//**
  This is a function to execute paths returned by the path-finding engine,
  for units controlled by advisors.

  Brings our bodyguard along.
  Returns FALSE only if died.
**************************************************************************/
bool adv_unit_execute_path(struct unit *punit, struct pf_path *path)
{
  const bool is_plr_ai = is_ai(unit_owner(punit));
  int i;

  /* We start with i = 1 for i = 0 is our present position */
  for (i = 1; i < path->length; i++) {
    struct tile *ptile = path->positions[i].tile;
    int id = punit->id;

    if (same_pos(unit_tile(punit), ptile)) {
      UNIT_LOG(LOG_DEBUG, punit, "execute_path: waiting this turn");
      return TRUE;
    }

    /* We use ai_unit_move() for everything but the last step
     * of the way so that we abort if unexpected opposition
     * shows up. Any enemy on the target tile is expected to
     * be our target and any attack there intentional.
     * However, do not annoy human players by automatically attacking
     * using units temporarily under AI control (such as auto-explorers)
     */

    if (is_plr_ai) {
      CALL_PLR_AI_FUNC(unit_move, unit_owner(punit), punit, ptile, path, i);
    } else {
      (void) adv_unit_move(punit, ptile);
    }
    if (!game_unit_by_number(id)) {
      /* Died... */
      return FALSE;
    }

    if (!same_pos(unit_tile(punit), ptile) || punit->moves_left <= 0) {
      /* Stopped (or maybe fought) or ran out of moves */
      return TRUE;
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Move a unit. Do not attack. Do not leave bodyguard.
  For advisor controlled units.

  This function returns only when we have a reply from the server and
  we can tell the calling function what happened to the move request.
  (Right now it is not a big problem, since we call the server directly.)
**************************************************************************/
static bool adv_unit_move(struct unit *punit, struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);
  int mcost;

  /* if enemy, stop and give a chance for the human player to
     handle this case */
  if (is_enemy_unit_tile(ptile, pplayer)
      || is_enemy_city_tile(ptile, pplayer)) {
    UNIT_LOG(LOG_DEBUG, punit, "movement halted due to enemy presence");
    return FALSE;
  }

  /* Try not to end move next to an enemy if we can avoid it by waiting */
  mcost = map_move_cost_unit(&(wld.map), punit, ptile);
  if (punit->moves_left <= mcost
      && unit_move_rate(punit) > mcost
      && adv_danger_at(punit, ptile)
      && !adv_danger_at(punit, unit_tile(punit))) {
    UNIT_LOG(LOG_DEBUG, punit, "ending move early to stay out of trouble");
    return FALSE;
  }

  /* go */
  unit_activity_handling(punit, ACTIVITY_IDLE);
  /* Move */
  (void) unit_move_handling(punit, ptile, FALSE, TRUE, NULL);

  return TRUE;
}

/**********************************************************************//**
  Similar to is_my_zoc(), but with some changes:
  - destination (x0,y0) need not be adjacent?
  - don't care about some directions?

  Fix to bizarre did-not-find bug.  Thanks, Katvrr -- Syela
**************************************************************************/
static bool adv_could_be_my_zoc(struct unit *myunit, struct tile *ptile)
{
  if (same_pos(ptile, unit_tile(myunit))) {
    return FALSE; /* can't be my zoc */
  }
  if (is_tiles_adjacent(ptile, unit_tile(myunit))
      && !is_non_allied_unit_tile(ptile, unit_owner(myunit))) {
    return FALSE;
  }

  adjc_iterate(&(wld.map), ptile, atile) {
    if (!terrain_has_flag(tile_terrain(atile), TER_NO_ZOC)
        && is_non_allied_unit_tile(atile, unit_owner(myunit))) {
      return FALSE;
    }
  } adjc_iterate_end;
  
  return TRUE;
}

/**********************************************************************//**
  returns:
    0 if can't move
    1 if zoc_ok
   -1 if zoc could be ok?

  see also unithand can_unit_move_to_tile_with_notify()
**************************************************************************/
int adv_could_unit_move_to_tile(struct unit *punit, struct tile *dest_tile)
{
  enum unit_move_result reason =
    unit_move_to_tile_test(&(wld.map), punit, ACTIVITY_IDLE, unit_tile(punit),
                           dest_tile, unit_has_type_flag(punit, UTYF_IGZOC),
                           NULL, FALSE);

  switch (reason) {
  case MR_OK:
    return 1;

  case MR_ZOC:
    if (adv_could_be_my_zoc(punit, unit_tile(punit))) {
      return -1;
    }
    break;

  default:
    break;
  };
  return 0;
}

/**********************************************************************//**
  Attack rating of this kind of unit.
**************************************************************************/
int adv_unittype_att_rating(const struct unit_type *punittype, int veteran,
                            int moves_left, int hp)
{
  return base_get_attack_power(punittype, veteran, moves_left) * hp
         * punittype->firepower / POWER_DIVIDER;
}

/**********************************************************************//**
  Attack rating of this particular unit assuming that it has a
  complete move left.
**************************************************************************/
int adv_unit_att_rating(const struct unit *punit)
{
  return adv_unittype_att_rating(unit_type_get(punit), punit->veteran,
                                 SINGLE_MOVE, punit->hp);
}

/**********************************************************************//**
  Basic (i.e. not taking attacker specific corections into account)
  defense rating of this particular unit.
**************************************************************************/
int adv_unit_def_rating_basic(const struct unit *punit)
{
  return base_get_defense_power(punit) * punit->hp *
    unit_type_get(punit)->firepower / POWER_DIVIDER;
}

/**********************************************************************//**
  Square of the previous function - used in actual computations.
**************************************************************************/
int adv_unit_def_rating_basic_squared(const struct unit *punit)
{
  int v = adv_unit_def_rating_basic(punit);

  return v * v;
}

/**********************************************************************//**
  Are there dangerous enemies at or adjacent to the tile 'ptile'?
**************************************************************************/
bool adv_danger_at(struct unit *punit, struct tile *ptile)
{
  int a = 0, d, db;
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(ptile);
  enum override_bool dc = NO_OVERRIDE;
  int extras_bonus = 0;

  /* Give AI code possibility to decide itself */
  CALL_PLR_AI_FUNC(consider_tile_dangerous, unit_owner(punit), ptile, punit, &dc);
  if (dc == OVERRIDE_TRUE) {
    return TRUE;
  } else if (dc == OVERRIDE_FALSE) {
    return FALSE;
  }

  if (pcity && pplayers_allied(city_owner(pcity), unit_owner(punit))
      && !is_non_allied_unit_tile(ptile, pplayer)) {
    /* We will be safe in a friendly city */
    return FALSE;
  }

  /* Calculate how well we can defend at (x,y) */
  db = 10 + tile_terrain(ptile)->defense_bonus / 10;
  extras_bonus += tile_extras_defense_bonus(ptile, unit_type_get(punit));
  db += (db * extras_bonus) / 100;
  d = adv_unit_def_rating_basic_squared(punit) * db;

  adjc_iterate(&(wld.map), ptile, ptile1) {
    if (!map_is_known_and_seen(ptile1, unit_owner(punit), V_MAIN)) {
      /* We cannot see danger at (ptile1) => assume there is none */
      continue;
    }
    unit_list_iterate(ptile1->units, enemy) {
      if (pplayers_at_war(unit_owner(enemy), unit_owner(punit))
          && unit_attack_unit_at_tile_result(enemy, punit, ptile) == ATT_OK
          && unit_attack_units_at_tile_result(enemy, ptile) == ATT_OK) {
        a += adv_unit_att_rating(enemy);
        if ((a * a * 10) >= d) {
          /* The enemies combined strength is too big! */
          return TRUE;
        }
      }
    } unit_list_iterate_end;
  } adjc_iterate_end;

  return FALSE; /* as good a quick'n'dirty should be -- Syela */
}

/**********************************************************************//**
  The value of the units belonging to a given player on a given tile.
**************************************************************************/
static int stack_value(const struct tile *ptile,
		       const struct player *pplayer)
{
  int cost = 0;

  if (is_stack_vulnerable(ptile)) {
    unit_list_iterate(ptile->units, punit) {
      if (unit_owner(punit) == pplayer) {
	cost += unit_build_shield_cost_base(punit);
      }
    } unit_list_iterate_end;
  }

  return cost;
}

/**********************************************************************//**
  How dangerous would it be stop on a particular tile,
  because of enemy attacks,
  expressed as the probability of being killed.

  TODO: This implementation is a kludge until we compute a more accurate
  probability using the movemap.
  Also, we should take into account the reduced probability of death
  if we have a bodyguard travelling with us.
**************************************************************************/
static double chance_killed_at(const struct tile *ptile,
                               struct adv_risk_cost *risk_cost,
                               const struct pf_parameter *param)
{
  double db;
  int extras_bonus = 0;
  /* Compute the basic probability */
  /* WAG */
  /* In the early stages of a typical game, ferries
   * are effectively invulnerable (not until Frigates set sail),
   * so we make seas appear safer.
   * If we don't do this, the amphibious movement code has too strong a
   * desire to minimise the length of the path,
   * leading to poor choice for landing beaches */
  double p = is_ocean_tile(ptile)? 0.05: 0.15;

  /* If we are on defensive terrain, we are more likely to survive */
  db = 10 + tile_terrain(ptile)->defense_bonus / 10;
  extras_bonus += tile_extras_class_defense_bonus(ptile,
                                                  utype_class(param->utype));
  db += (extras_bonus) / 100;
  p *= 10.0 / db;

  return p;
}

/**********************************************************************//**
  PF stack risk cost. How undesirable is passing through a tile
  because of risks?
  Weight by the cost of destruction, for risks that can kill the unit.

  Why use the build cost when assessing the cost of destruction?
  The reasoning is thus.
  - Assume that all our units are doing necessary jobs;
    none are surplus to requirements.
    If that is not the case, we have problems elsewhere :-)
  - Then any units that are destroyed will have to be replaced.
  - The cost of replacing them will be their build cost.
  - Therefore the total (re)build cost is a good representation of the
    the cost of destruction.
**************************************************************************/
static int stack_risk(const struct tile *ptile,
                      struct adv_risk_cost *risk_cost,
                      const struct pf_parameter *param)
{
  double risk = 0;
  /* Compute the risk of destruction, assuming we will stop at this tile */
  const double value = risk_cost->base_value
                       + stack_value(ptile, param->owner);
  const double p_killed = chance_killed_at(ptile, risk_cost, param);
  double danger = value * p_killed;

  /* Adjust for the fact that we might not stop at this tile,
   * and for our fearfulness */
  risk += danger * risk_cost->fearfulness;

  /* Adjust for the risk that we might become stuck (for an indefinite period)
   * if we enter or try to enter the tile. */
  if (risk_cost->enemy_zoc_cost != 0
      && (is_non_allied_city_tile(ptile, param->owner)
	  || !is_my_zoc(param->owner, ptile, param->map)
	  || is_non_allied_unit_tile(ptile, param->owner))) {
    /* We could become stuck. */
    risk += risk_cost->enemy_zoc_cost;
  }

  return risk;
}

/**********************************************************************//**
  PF extra cost call back to avoid creating tall stacks or
  crossing dangerous tiles.
  By setting this as an extra-cost call-back, paths will avoid tall stacks.
  Avoiding tall stacks *all* along a path is useful because a unit following a
  path might have to stop early because of ZoCs.
**************************************************************************/
static int prefer_short_stacks(const struct tile *ptile,
                               enum known_type known,
                               const struct pf_parameter *param)
{
  return stack_risk(ptile, (struct adv_risk_cost *)param->data, param);
}

/**********************************************************************//**
  Set PF callbacks to favour paths that do not create tall stacks
  or cross dangerous tiles.
**************************************************************************/
void adv_avoid_risks(struct pf_parameter *parameter,
                     struct adv_risk_cost *risk_cost,
                     struct unit *punit,
                     const double fearfulness)
{
  /* If we stay a short time on each tile, the danger of each individual tile
   * is reduced. If we do not do this,
   * we will not favour longer but faster routs. */
  const double linger_fraction = (double)SINGLE_MOVE / parameter->move_rate;

  parameter->data = risk_cost;
  parameter->get_EC = prefer_short_stacks;
  risk_cost->base_value = unit_build_shield_cost_base(punit);
  risk_cost->fearfulness = fearfulness * linger_fraction;

  risk_cost->enemy_zoc_cost = PF_TURN_FACTOR * 20;
}
