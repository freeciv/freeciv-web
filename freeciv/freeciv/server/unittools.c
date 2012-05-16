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
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "base.h"
#include "city.h"
#include "combat.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"

/* aicore */
#include "path_finding.h"
#include "pf_tools.h"

/* ai */
#include "advdiplomacy.h"
#include "aiexplorer.h"
#include "aiferry.h"
#include "aitools.h"
#include "aiunit.h"

/* server */
#include "aiiface.h"
#include "barbarian.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplhand.h"
#include "gamehand.h"
#include "gotohand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "script_signal.h"
#include "sernet.h"
#include "settlers.h"
#include "srv_main.h"
#include "techtools.h"
#include "unithand.h"
#include "unittools.h"

/* We need this global variable for our sort algorithm */
static struct tile *autoattack_target;

static void unit_restore_hitpoints(struct unit *punit);
static void unit_restore_movepoints(struct player *pplayer, struct unit *punit);
static void update_unit_activity(struct unit *punit);
static void wakeup_neighbor_sentries(struct unit *punit);
static void do_upgrade_effects(struct player *pplayer);

static bool maybe_cancel_patrol_due_to_enemy(struct unit *punit);
static int hp_gain_coord(struct unit *punit);

static void put_unit_onto_transporter(struct unit *punit, struct unit *ptrans);
static void pull_unit_from_transporter(struct unit *punit,
				       struct unit *ptrans);

/**************************************************************************
  Returns a unit type that matches the role_tech or role roles.

  If role_tech is given, then we look at all units with this role
  whose requirements are met by any player, and return a random one.  This
  can be used to give a unit to barbarians taken from the set of most
  advanced units researched by the 'real' players.

  If role_tech is not give (-1) or if there are no matching unit types,
  then we look at 'role' value and return a random matching unit type.

  It is an error if there are no available units.  This function will
  always return a valid unit.
**************************************************************************/
struct unit_type *find_a_unit_type(enum unit_role_id role,
				   enum unit_role_id role_tech)
{
  struct unit_type *which[U_LAST];
  int i, num=0;

  if (role_tech != -1) {
    for(i=0; i<num_role_units(role_tech); i++) {
      struct unit_type *iunit = get_role_unit(role_tech, i);
      const int minplayers = 2;
      int players = 0;

      /* Note, if there's only one player in the game this check will always
       * fail. */
      players_iterate(pplayer) {
	if (!is_barbarian(pplayer)
	    && can_player_build_unit_direct(pplayer, iunit)) {
	  players++;
	}
      } players_iterate_end;
      if (players > minplayers) {
	which[num++] = iunit;
      }
    }
  }
  if(num==0) {
    for(i=0; i<num_role_units(role); i++) {
      which[num++] = get_role_unit(role, i);
    }
  }
  if(num==0) {
    /* Ruleset code should ensure there is at least one unit for each
     * possibly-required role, or check before calling this function.
     */
    die("No unit types in find_a_unit_type(%d,%d)!", role, role_tech);
  }
  return which[myrand(num)];
}

/**************************************************************************
  After a battle, after diplomatic aggression and after surviving trireme
  loss chance, this routine is called to decide whether or not the unit
  should become more experienced.

  There is a specified chance for it to happen, (+50% if player got SUNTZU)
  the chances are specified in the units.ruleset file.
**************************************************************************/
bool maybe_make_veteran(struct unit *punit)
{
  if (punit->veteran + 1 >= MAX_VET_LEVELS
      || unit_type(punit)->veteran[punit->veteran].name[0] == '\0'
      || unit_has_type_flag(punit, F_NO_VETERAN)) {
    return FALSE;
  } else {
    int mod = 100 + get_unittype_bonus(unit_owner(punit), punit->tile,
				       unit_type(punit), EFT_VETERAN_COMBAT);

    /* The modification is tacked on as a multiplier to the base chance.
     * For example with a base chance of 50% for green units and a modifier
     * of +50% the end chance is 75%. */
    if (myrand(100) < game.veteran_chance[punit->veteran] * mod / 100) {
      punit->veteran++;
      return TRUE;
    }
    return FALSE;
  }
}

/**************************************************************************
  This is the basic unit versus unit combat routine.
  1) ALOT of modifiers bonuses etc is added to the 2 units rates.
  2) If the attack is a bombardment, do rate attacks and don't kill the
     defender, then return.
  3) the combat loop, which continues until one of the units are dead
  4) the aftermath, the loser (and potentially the stack which is below it)
     is wiped, and the winner gets a chance of gaining veteran status
**************************************************************************/
void unit_versus_unit(struct unit *attacker, struct unit *defender,
		      bool bombard)
{
  int attackpower = get_total_attack_power(attacker,defender);
  int defensepower = get_total_defense_power(attacker,defender);

  int attack_firepower, defense_firepower;
  get_modified_firepower(attacker, defender,
			 &attack_firepower, &defense_firepower);

  freelog(LOG_VERBOSE, "attack:%d, defense:%d, attack firepower:%d, defense firepower:%d",
	  attackpower, defensepower, attack_firepower, defense_firepower);

  if (bombard) {
    int i;
    int rate = unit_type(attacker)->bombard_rate;

    for (i = 0; i < rate; i++) {
      if (myrand(attackpower+defensepower) >= defensepower) {
	defender->hp -= attack_firepower;
      }
    }

    /* Don't kill the target. */
    if (defender->hp <= 0) {
      defender->hp = 1;
    }
    return;
  }

  if (attackpower == 0) {
      attacker->hp=0; 
  } else if (defensepower == 0) {
      defender->hp=0;
  }
  while (attacker->hp>0 && defender->hp>0) {
    if (myrand(attackpower+defensepower) >= defensepower) {
      defender->hp -= attack_firepower;
    } else {
      attacker->hp -= defense_firepower;
    }
  }
  if (attacker->hp<0) attacker->hp = 0;
  if (defender->hp<0) defender->hp = 0;

  if (attacker->hp > 0)
    maybe_make_veteran(attacker); 
  else if (defender->hp > 0)
    maybe_make_veteran(defender);
}

/***************************************************************************
  Do unit auto-upgrades to players with the EFT_UNIT_UPGRADE effect
  (traditionally from Leonardo's Workshop).
****************************************************************************/
static void do_upgrade_effects(struct player *pplayer)
{
  int upgrades = get_player_bonus(pplayer, EFT_UPGRADE_UNIT);
  struct unit_list *candidates;

  if (upgrades <= 0) {
    return;
  }
  candidates = unit_list_new();

  unit_list_iterate(pplayer->units, punit) {
    /* We have to be careful not to strand units at sea, for example by
     * upgrading a frigate to an ironclad while it was carrying a unit. */
    if (test_unit_upgrade(punit, TRUE) == UR_OK) {
      unit_list_prepend(candidates, punit);	/* Potential candidate :) */
    }
  } unit_list_iterate_end;

  while (upgrades > 0 && unit_list_size(candidates) > 0) {
    /* Upgrade one unit.  The unit is chosen at random from the list of
     * available candidates. */
    int candidate_to_upgrade = myrand(unit_list_size(candidates));
    struct unit *punit = unit_list_get(candidates, candidate_to_upgrade);
    struct unit_type *type_from = unit_type(punit);
    struct unit_type *type_to = can_upgrade_unittype(pplayer, type_from);

    transform_unit(punit, type_to, TRUE);
    notify_player(pplayer, unit_tile(punit), E_UNIT_UPGRADED,
                  FTC_SERVER_INFO, NULL,
                  _("%s was upgraded for free to %s."),
                  utype_name_translation(type_from),
                  unit_link(punit));
    unit_list_unlink(candidates, punit);
    upgrades--;
  }

  unit_list_free(candidates);
}

/***************************************************************************
  1. Do Leonardo's Workshop upgrade if applicable.

  2. Restore/decrease unit hitpoints.

  3. Kill dead units.

  4. Rescue airplanes by returning them to base automatically.

  5. Decrease fuel of planes in the air.

  6. Refuel planes that are in bases.

  7. Kill planes that are out of fuel.
****************************************************************************/
void player_restore_units(struct player *pplayer)
{
  /* 1) get Leonardo out of the way first: */
  do_upgrade_effects(pplayer);

  unit_list_iterate_safe(pplayer->units, punit) {

    /* 2) Modify unit hitpoints. Helicopters can even lose them. */
    unit_restore_hitpoints(punit);

    /* 3) Check that unit has hitpoints */
    if (punit->hp<=0) {
      /* This should usually only happen for heli units,
	 but if any other units get 0 hp somehow, catch
	 them too.  --dwp  */
      notify_player(pplayer, punit->tile, E_UNIT_LOST_MISC,
                    FTC_SERVER_INFO, NULL,
                    _("Your %s has run out of hit points."), 
                    unit_link(punit));
      wipe_unit(punit);
      continue; /* Continue iterating... */
    }

    /* 4) Rescue planes if needed */
    if (utype_fuel(unit_type(punit))) {
      /* Shall we emergency return home on the last vapors? */

      /* I think this is strongly against the spirit of client goto.
       * The problem is (again) that here we know too much. -- Zamar */

      if (punit->fuel <= 1
          && !is_unit_being_refueled(punit)) {
        struct unit *carrier;

        carrier = find_transport_from_tile(punit, punit->tile);
        if (carrier) {
          put_unit_onto_transporter(punit, carrier);
        } else {
          bool alive = true;

          struct pf_map *pfm;
          struct pf_parameter parameter;

          pft_fill_unit_parameter(&parameter, punit);
          pfm = pf_map_new(&parameter);

          pf_map_iterate_move_costs(pfm, ptile, move_cost, TRUE) {
            if (move_cost > punit->moves_left) {
              /* Too far */
              break;
            }

            if (is_airunit_refuel_point(ptile, pplayer,
					unit_type(punit), FALSE)) {

              struct pf_path *path;
              int id = punit->id;

              /* Client orders may be running for this unit - if so
               * we free them before engaging goto. */
              free_unit_orders(punit);

              path = pf_map_get_path(pfm, ptile);

	      alive = ai_follow_path(punit, path, ptile);

	      if (!alive) {
                freelog(LOG_ERROR, "rescue plane: unit %d died enroute!", id);
              } else if (!same_pos(punit->tile, ptile)) {
                  /* Enemy units probably blocked our route
                   * FIXME: We should try find alternative route around
                   * the enemy unit instead of just giving up and crashing. */
                  freelog(LOG_DEBUG,
                          "rescue plane: unit %d could not move to refuel point!",
                          punit->id);
              }

              if (alive) {
                /* Clear activity. Unit info will be sent in the end of
	         * the function. */
                unit_activity_handling(punit, ACTIVITY_IDLE);
                punit->goto_tile = NULL;

                if (!is_unit_being_refueled(punit)) {
                  carrier = find_transport_from_tile(punit, punit->tile);
                  if (carrier) {
                    put_unit_onto_transporter(punit, carrier);
                  }
                }

                notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                              FTC_SERVER_INFO, NULL,
                              _("Your %s has returned to refuel."),
                              unit_link(punit));
	      }
              pf_path_destroy(path);
              break;
            }
          } pf_map_iterate_move_costs_end;
          pf_map_destroy(pfm);

          if (!alive) {
            /* Unit died trying to move to refuel point. */
            return;
	  }
        }
      }

      /* 5) Update fuel */
      punit->fuel--;

      /* 6) Automatically refuel air units in cities, airbases, and
       *    transporters (carriers). */
      if (is_unit_being_refueled(punit)) {
	punit->fuel = utype_fuel(unit_type(punit));
      }
    }
  } unit_list_iterate_safe_end;

  /* 7) Check if there are air units without fuel */
  unit_list_iterate_safe(pplayer->units, punit) {
    if (punit->fuel <= 0 && utype_fuel(unit_type(punit))) {
      notify_player(pplayer, punit->tile, E_UNIT_LOST_MISC,
                    FTC_SERVER_INFO, NULL,
                    _("Your %s has run out of fuel."),
                    unit_link(punit));
      wipe_unit(punit);
    } 
  } unit_list_iterate_safe_end;

  /* Send all updates. */
  unit_list_iterate(pplayer->units, punit) {
    send_unit_info(NULL, punit);
  } unit_list_iterate_end;
}

/****************************************************************************
  add hitpoints to the unit, hp_gain_coord returns the amount to add
  united nations will speed up the process by 2 hp's / turn, means helicopters
  will actually not loose hp's every turn if player have that wonder.
  Units which have moved don't gain hp, except the United Nations and
  helicopter effects still occur.
*****************************************************************************/
static void unit_restore_hitpoints(struct unit *punit)
{
  bool was_lower;
  struct unit_class *class = unit_class(punit);
  struct city *pcity = tile_city(punit->tile);

  was_lower=(punit->hp < unit_type(punit)->hp);

  if(!punit->moved) {
    punit->hp+=hp_gain_coord(punit);
  }

  /* Bonus recovery HP (traditionally from the United Nations) */
  punit->hp += get_unit_bonus(punit, EFT_UNIT_RECOVER);

  if (!pcity && !tile_has_native_base(punit->tile, unit_type(punit))
      && punit->transported_by == -1) {
    punit->hp -= unit_type(punit)->hp * class->hp_loss_pct / 100;
  }

  if(punit->hp>=unit_type(punit)->hp) {
    punit->hp=unit_type(punit)->hp;
    if(was_lower&&punit->activity==ACTIVITY_SENTRY){
      set_unit_activity(punit,ACTIVITY_IDLE);
    }
  }
  if(punit->hp<0)
    punit->hp=0;

  punit->moved = FALSE;
  punit->paradropped = FALSE;
}
  
/***************************************************************************
  Move points are trivial, only modifiers to the base value is if it's
  sea units and the player has certain wonders/techs. Then add veteran
  bonus, if any.
***************************************************************************/
static void unit_restore_movepoints(struct player *pplayer, struct unit *punit)
{
  punit->moves_left = unit_move_rate(punit);
  punit->done_moving = FALSE;
}

/**************************************************************************
  iterate through all units and update them.
**************************************************************************/
void update_unit_activities(struct player *pplayer)
{
  unit_list_iterate_safe(pplayer->units, punit)
    update_unit_activity(punit);
  unit_list_iterate_safe_end;
}

/**************************************************************************
  returns how many hp's a unit will gain on this square
  depends on whether or not it's inside city or fortress.
  barracks will regen landunits completely
  airports will regen airunits  completely
  ports    will regen navalunits completely
  fortify will add a little extra.
***************************************************************************/
static int hp_gain_coord(struct unit *punit)
{
  int hp = 0;
  const int base = unit_type(punit)->hp;

  /* Includes barracks (100%), fortress (25%), etc. */
  hp += base * get_unit_bonus(punit, EFT_HP_REGEN) / 100;

  if (tile_city(punit->tile)) {
    hp = MAX(hp, base / 3);
  }

  if (!unit_class(punit)->hp_loss_pct) {
    hp += (base + 9) / 10;
  }

  if (punit->activity == ACTIVITY_FORTIFIED) {
    hp += (base + 9) / 10;
  }

  return MAX(hp, 0);
}

/**************************************************************************
  Calculate the total amount of activity performed by all units on a tile
  for a given task.
**************************************************************************/
static int total_activity(struct tile *ptile, enum unit_activity act)
{
  int total = 0;

  unit_list_iterate (ptile->units, punit)
    if (punit->activity == act) {
      total += punit->activity_count;
    }
  unit_list_iterate_end;
  return total;
}

/**************************************************************************
  Calculate the total amount of activity performed by all units on a tile
  for a given task and target.
**************************************************************************/
static int total_activity_targeted(struct tile *ptile, enum unit_activity act,
				   enum tile_special_type tgt)
{
  int total = 0;

  unit_list_iterate (ptile->units, punit)
    if ((punit->activity == act) && (punit->activity_target == tgt))
      total += punit->activity_count;
  unit_list_iterate_end;
  return total;
}

/**************************************************************************
  Calculate the total amount of base building activity performed by all
  units on a tile for a given base.
**************************************************************************/
static int total_activity_base(struct tile *ptile, Base_type_id base)
{
  int total = 0;

  unit_list_iterate (ptile->units, punit)
    if (punit->activity == ACTIVITY_BASE
        && punit->activity_base == base) {
      total += punit->activity_count;
    }
  unit_list_iterate_end;
  return total;
}

/**************************************************************************
  Check the total amount of activity performed by all units on a tile
  for a given task.
**************************************************************************/
static bool total_activity_done(struct tile *ptile, enum unit_activity act)
{
  return total_activity(ptile, act) >= tile_activity_time(act, ptile);
}

/***************************************************************************
  Maybe settler/worker gains a veteran level?
****************************************************************************/
static bool maybe_settler_become_veteran(struct unit *punit)
{
  if (punit->veteran + 1 >= MAX_VET_LEVELS
      || unit_type(punit)->veteran[punit->veteran].name[0] == '\0'
      || unit_has_type_flag(punit, F_NO_VETERAN)) {
    return FALSE;
  }
  if (unit_has_type_flag(punit, F_SETTLERS)
      && myrand(100) < game.work_veteran_chance[punit->veteran]) {
    punit->veteran++;
    return TRUE;
  }
  return FALSE;  
}

/**************************************************************************
  Common notification for all experience levels.
**************************************************************************/
void notify_unit_experience(struct unit *punit)
{
  if (!punit) {
    return;
  }

  notify_player(unit_owner(punit), unit_tile(punit), E_UNIT_BECAME_VET,
                FTC_SERVER_INFO, NULL,
                /* TRANS: Your <unit> became ... */
                _("Your %s became more experienced!"),
                unit_link(punit));
}

/**************************************************************************
  Pillages base from tile
**************************************************************************/
static void unit_pillage_base(struct tile *ptile, struct base_type *pbase)
{
  if (territory_claiming_base(pbase)) {
    /* Clearing borders will take care of the vision providing
     * bases as well. */
    map_clear_border(ptile);
  } else {
    struct player *owner = tile_owner(ptile);

    if (pbase->vision_main_sq >= 0 && owner) {
    /* Base provides vision, but no borders. */
      map_refog_circle(owner, ptile, pbase->vision_main_sq, -1,
                       game.info.vision_reveal_tiles, V_MAIN);
    }
    if (pbase->vision_invis_sq >= 0 && owner) {
    /* Base provides vision, but no borders. */
      map_refog_circle(owner, ptile, pbase->vision_invis_sq, -1,
                       game.info.vision_reveal_tiles, V_INVIS);
    }
  }
  tile_remove_base(ptile, pbase);
}

/**************************************************************************
  progress settlers in their current tasks, 
  and units that is pillaging.
  also move units that is on a goto.
  restore unit move points (information needed for settler tasks)
**************************************************************************/
static void update_unit_activity(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  int id = punit->id;
  bool unit_activity_done = FALSE;
  enum unit_activity activity = punit->activity;
  struct tile *ptile = punit->tile;
  bool check_adjacent_units = FALSE;
  
  switch (activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_GOTO:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_LAST:
    /*  We don't need the activity_count for the above */
    break;

  case ACTIVITY_FORTIFYING:
  case ACTIVITY_SENTRY:
    punit->activity_count += get_activity_rate_this_turn(punit);
    break;

  case ACTIVITY_POLLUTION:
  case ACTIVITY_ROAD:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_RAILROAD:
  case ACTIVITY_PILLAGE:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_BASE:
    punit->activity_count += get_activity_rate_this_turn(punit);

    /* settler may become veteran when doing something useful */
    if (maybe_settler_become_veteran(punit)) {
      notify_unit_experience(punit);
    }
    break;
  };

  unit_restore_movepoints(pplayer, punit);

  switch (activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_SENTRY:
  case ACTIVITY_GOTO:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
    /* no default, ensure all handled */
    break;

  case ACTIVITY_EXPLORE:
    do_explore(punit);
    return;

  case ACTIVITY_PILLAGE:
    if (punit->activity_target == S_LAST
        && punit->activity_base == -1) { /* case for old save files */
      if (punit->activity_count >= 1) {
        enum tile_special_type what;
        bv_bases bases;

        BV_CLR_ALL(bases);
        base_type_iterate(pbase) {
          if (tile_has_base(ptile, pbase)) {
            if (pbase->pillageable) {
              BV_SET(bases, base_index(pbase));
            }
          }
        } base_type_iterate_end;

        what = get_preferred_pillage(get_tile_infrastructure_set(ptile, NULL),
                                     bases);

	if (what != S_LAST) {
          if (what > S_LAST) {
            unit_pillage_base(ptile, base_by_number(what - S_LAST - 1));
          } else {
            tile_clear_special(ptile, what);
          }
	  update_tile_knowledge(ptile);
	  set_unit_activity(punit, ACTIVITY_IDLE);
	  check_adjacent_units = TRUE;
	}

	/* Change vision if effects have changed. */
	unit_list_refresh_vision(ptile->units);
      }
    }
    else if (total_activity_targeted(ptile, ACTIVITY_PILLAGE, 
                                     punit->activity_target) >= 1) {
      enum tile_special_type what_pillaged = punit->activity_target;

      if (what_pillaged == S_LAST && punit->activity_base != -1) {
        unit_pillage_base(ptile, base_by_number(punit->activity_base));
      } else {
        tile_clear_special(ptile, what_pillaged);
      }
      unit_list_iterate (ptile->units, punit2) {
        if ((punit2->activity == ACTIVITY_PILLAGE) &&
	    (punit2->activity_target == what_pillaged)) {
	  set_unit_activity(punit2, ACTIVITY_IDLE);
	  send_unit_info(NULL, punit2);
	}
      } unit_list_iterate_end;
      update_tile_knowledge(ptile);

      call_incident(INCIDENT_PILLAGE, unit_owner(punit), tile_owner(ptile));

      /* Change vision if effects have changed. */
      unit_list_refresh_vision(ptile->units);
    }
    break;

  case ACTIVITY_POLLUTION:
    if (total_activity_done(ptile, ACTIVITY_POLLUTION)) {
      tile_clear_special(ptile, S_POLLUTION);
      unit_activity_done = TRUE;
    }
    break;

  case ACTIVITY_FALLOUT:
    if (total_activity_done(ptile, ACTIVITY_FALLOUT)) {
      tile_clear_special(ptile, S_FALLOUT);
      unit_activity_done = TRUE;
    }
    break;

  case ACTIVITY_BASE:
    if (total_activity_base(ptile, punit->activity_base)
        >= tile_activity_base_time(ptile, punit->activity_base)) {
      struct base_type *new_base = base_by_number(punit->activity_base);

      create_base(ptile, new_base, unit_owner(punit));

      unit_activity_done = TRUE;
    }
    break;

  case ACTIVITY_IRRIGATE:
    if (total_activity_done(ptile, ACTIVITY_IRRIGATE)) {
      struct terrain *old = tile_terrain(ptile);

      tile_apply_activity(ptile, ACTIVITY_IRRIGATE);
      check_terrain_change(ptile, old);
      unit_activity_done = TRUE;
    }
    break;

  case ACTIVITY_MINE:
  case ACTIVITY_TRANSFORM:
    if (total_activity_done(ptile, activity)) {
      struct terrain *old = tile_terrain(ptile);

      tile_apply_activity(ptile, activity);
      check_terrain_change(ptile, old);
      unit_activity_done = TRUE;
      check_adjacent_units = TRUE;
    }
    break;

  case ACTIVITY_ROAD:
    if (total_activity (ptile, ACTIVITY_ROAD)
	+ total_activity (ptile, ACTIVITY_RAILROAD)
        >= tile_activity_time(ACTIVITY_ROAD, ptile)) {
      tile_set_special(ptile, S_ROAD);
      unit_activity_done = TRUE;
    }
    break;

  case ACTIVITY_RAILROAD:
    if (total_activity_done(ptile, ACTIVITY_RAILROAD)) {
      tile_set_special(ptile, S_RAILROAD);
      unit_activity_done = TRUE;
    }
    break;
  };

  if (unit_activity_done) {
    update_tile_knowledge(ptile);
    unit_list_iterate (ptile->units, punit2) {
      if (punit2->activity == activity) {
	set_unit_activity(punit2, ACTIVITY_IDLE);
	send_unit_info(NULL, punit2);
      }
    } unit_list_iterate_end;
  }

  /* Some units nearby can not continue irrigating */
  if (check_adjacent_units) {
    adjc_iterate(ptile, ptile2) {
      unit_list_iterate(ptile2->units, punit2) {
        if (!can_unit_continue_current_activity(punit2)) {
          unit_activity_handling(punit2, ACTIVITY_IDLE);
        }
      } unit_list_iterate_end;
    } adjc_iterate_end;
  }

  if (activity == ACTIVITY_FORTIFYING) {
    if (punit->activity_count >= 1) {
      set_unit_activity(punit,ACTIVITY_FORTIFIED);
    }
  }

  if (game_find_unit_by_number(id) && unit_has_orders(punit)) {
    if (!execute_orders(punit)) {
      /* Unit died. */
      return;
    }
  }

  if (game_find_unit_by_number(id)) {
    send_unit_info(NULL, punit);
  }

  unit_list_iterate(ptile->units, punit2) {
    if (!can_unit_continue_current_activity(punit2))
    {
      unit_activity_handling(punit2, ACTIVITY_IDLE);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
enum goto_move_restriction get_activity_move_restriction(enum unit_activity activity)
{
  enum goto_move_restriction restr;

  switch (activity) {
  case ACTIVITY_IRRIGATE:
    restr = GOTO_MOVE_CARDINAL_ONLY;
    break;
  case ACTIVITY_POLLUTION:
  case ACTIVITY_ROAD:
  case ACTIVITY_MINE:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_RAILROAD:
  case ACTIVITY_PILLAGE:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FALLOUT:
    restr = GOTO_MOVE_STRAIGHTEST;
    break;
  default:
    restr = GOTO_MOVE_ANY;
    break;
  }

  return (restr);
}

/**************************************************************************
...
**************************************************************************/
static bool find_a_good_partisan_spot(struct city *pcity,
				      struct unit_type *u_type,
				      struct tile **dst_tile)
{
  struct tile *pcenter = city_tile(pcity);
  struct player *powner = city_owner(pcity);
  int bestvalue = 0;

  /* coords of best tile in arg pointers */
  city_tile_iterate(pcenter, ptile) {
    int value;

    if (is_ocean_tile(ptile)) {
      continue;
    }

    if (NULL != tile_city(ptile)) {
      continue;
    }

    if (0 < unit_list_size(ptile->units)) {
      continue;
    }

    /* City has not changed hands yet; see place_partisans(). */
    value = get_virtual_defense_power(NULL, u_type, powner,
				      ptile, FALSE, 0);
    value *= 10;

    if (tile_continent(ptile) != tile_continent(pcenter)) {
      value /= 2;
    }

    value -= myrand(value/3);

    if (value > bestvalue) {
      *dst_tile = ptile;
      bestvalue = value;
    }
  } city_tile_iterate_end;

  return bestvalue > 0;
}

/**************************************************************************
  finds a spot around pcity and place a partisan.
**************************************************************************/
static void place_partisans(struct city *pcity, int count)
{
  struct tile *ptile = NULL;
  struct unit_type *u_type = get_role_unit(L_PARTISAN, 0);

  while ((count--) > 0 && find_a_good_partisan_spot(pcity, u_type, &ptile)) {
    struct unit *punit;

    punit = create_unit(city_owner(pcity), ptile, u_type, 0, 0, -1);
    if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      punit->activity = ACTIVITY_FORTIFIED; /* yes; directly fortified */
      send_unit_info(NULL, punit);
    }
  }
}

/**************************************************************************
  if requirements to make partisans when a city is conquered is fullfilled
  this routine makes a lot of partisans based on the city's size.
  To be candidate for partisans the following things must be satisfied:
  1) Guerilla warfare must be known by atleast 1 player
  2) The owner of the city is the original player.
  3) The player must know about communism and gunpowder
  4) the player must run either a democracy or a communist society.
**************************************************************************/
void make_partisans(struct city *pcity)
{
  int partisans;

  if (num_role_units(L_PARTISAN) <= 0
      || pcity->original != city_owner(pcity)
      || get_city_bonus(pcity, EFT_INSPIRE_PARTISANS) <= 0) {
    return;
  }
  
  partisans = myrand(1 + pcity->size/2) + 1;
  if (partisans > 8) 
    partisans = 8;
  
  place_partisans(pcity,partisans);
}

/**************************************************************************
Are there dangerous enemies at or adjacent to (x, y)?

N.B. This function should only be used by (cheating) AI, as it iterates 
through all units stacked on the tiles, an info not normally available 
to the human player.
**************************************************************************/
bool enemies_at(struct unit *punit, struct tile *ptile)
{
  int a = 0, d, db;
  struct player *pplayer = unit_owner(punit);
  struct city *pcity = tile_city(ptile);

  if (pcity && pplayers_allied(city_owner(pcity), unit_owner(punit))
      && !is_non_allied_unit_tile(ptile, pplayer)) {
    /* We will be safe in a friendly city */
    return FALSE;
  }

  /* Calculate how well we can defend at (x,y) */
  db = 10 + tile_terrain(ptile)->defense_bonus / 10;
  if (tile_has_special(ptile, S_RIVER))
    db += (db * terrain_control.river_defense_bonus) / 100;
  d = unit_def_rating_basic_sq(punit) * db;

  adjc_iterate(ptile, ptile1) {
    if (ai_handicap(pplayer, H_FOG)
	&& !map_is_known_and_seen(ptile1, unit_owner(punit), V_MAIN)) {
      /* We cannot see danger at (ptile1) => assume there is none */
      continue;
    }
    unit_list_iterate(ptile1->units, enemy) {
      if (pplayers_at_war(unit_owner(enemy), unit_owner(punit)) 
          && can_unit_attack_unit_at_tile(enemy, punit, ptile)
          && can_unit_attack_all_at_tile(enemy, ptile)) {
	a += unit_att_rating(enemy);
	if ((a * a * 10) >= d) {
          /* The enemies combined strength is too big! */
          return TRUE;
        }
      }
    } unit_list_iterate_end;
  } adjc_iterate_end;

  return FALSE; /* as good a quick'n'dirty should be -- Syela */
}

/**************************************************************************
Teleport punit to city at cost specified.  Returns success.
(If specified cost is -1, then teleportation costs all movement.)
                         - Kris Bubendorfer
**************************************************************************/
bool teleport_unit_to_city(struct unit *punit, struct city *pcity,
			  int move_cost, bool verbose)
{
  struct tile *src_tile = punit->tile, *dst_tile = pcity->tile;

  if (city_owner(pcity) == unit_owner(punit)){
    freelog(LOG_VERBOSE, "Teleported %s %s from (%d,%d) to %s",
	    nation_rule_name(nation_of_unit(punit)),
	    unit_rule_name(punit),
	    TILE_XY(src_tile),
	    city_name(pcity));
    if (verbose) {
      notify_player(unit_owner(punit), pcity->tile, E_UNIT_RELOCATED,
                    FTC_SERVER_INFO, NULL,
                    _("Teleported your %s to %s."),
                    unit_link(punit),
                    city_link(pcity));
    }

    /* Silently free orders since they won't be applicable anymore. */
    free_unit_orders(punit);

    if (move_cost == -1)
      move_cost = punit->moves_left;
    move_unit(punit, dst_tile, move_cost);
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  Move or remove a unit due to stack conflicts. This function will try to
  find a random safe tile within a two tile distance of the unit's current
  tile and move the unit there. If no tiles are found, the unit is
  disbanded. If 'verbose' is TRUE, a message is sent to the unit owner
  regarding what happened.
**************************************************************************/
void bounce_unit(struct unit *punit, bool verbose)
{
  struct player *pplayer;
  struct tile *punit_tile;
  int count = 0;

  /* I assume that there are no topologies that have more than
   * (2d + 1)^2 tiles in the "square" of "radius" d. */
  const int DIST = 2;
  struct tile *tiles[(2 * DIST + 1) * (2 * DIST + 1)];

  if (!punit) {
    return;
  }

  pplayer = unit_owner(punit);
  punit_tile = unit_tile(punit);

  square_iterate(punit_tile, DIST, ptile) {
    if (count >= ARRAY_SIZE(tiles)) {
      break;
    }

    if (ptile == punit_tile) {
      continue;
    }

    if (can_unit_survive_at_tile(punit, ptile)
        && !is_non_allied_city_tile(ptile, pplayer)
        && !is_non_allied_unit_tile(ptile, pplayer)) {
      tiles[count++] = ptile;
    }
  } square_iterate_end;

  if (count > 0) {
    struct tile *ptile = tiles[myrand(count)];

    if (verbose) {
      /* TRANS: A unit is moved to resolve stack conflicts. */
      notify_player(pplayer, ptile, E_UNIT_RELOCATED,
                    FTC_SERVER_INFO, NULL,
                    _("Moved your %s."),
                    unit_link(punit));
    }
    move_unit(punit, ptile, 0);
    return;
  }

  /* Didn't find a place to bounce the unit, just disband it. */
  if (verbose) {
    /* TRANS: A unit is disbanded to resolve stack conflicts. */
    notify_player(pplayer, punit_tile, E_UNIT_LOST_MISC,
                  FTC_SERVER_INFO, NULL,
                  _("Disbanded your %s."),
                  unit_link(punit));
  }
  wipe_unit(punit);
}


/**************************************************************************
  Throw pplayer's units from non allied cities

  If verbose is true, pplayer gets messages about where each units goes.
**************************************************************************/
static void throw_units_from_illegal_cities(struct player *pplayer,
                                           bool verbose)
{
  unit_list_iterate_safe(pplayer->units, punit) {
    struct tile *ptile = punit->tile;
    struct city *pcity = tile_city(ptile);

    if (NULL != pcity && !pplayers_allied(city_owner(pcity), pplayer)) {
      bounce_unit(punit, verbose);
    }
  } unit_list_iterate_safe_end;    
}

/**************************************************************************
  For each pplayer's unit, check if we stack illegally, if so,
  bounce both players' units. If on ocean tile, bounce everyone but ships
  to avoid drowning. This function assumes that cities are clean.

  If verbose is true, the unit owner gets messages about where each
  units goes.
**************************************************************************/
static void resolve_stack_conflicts(struct player *pplayer,
                                    struct player *aplayer, bool verbose)
{
  unit_list_iterate_safe(pplayer->units, punit) {
    struct tile *ptile = punit->tile;

    if (is_non_allied_unit_tile(ptile, pplayer)) {
      unit_list_iterate_safe(ptile->units, aunit) {
        if (unit_owner(aunit) == pplayer
            || unit_owner(aunit) == aplayer
            || !can_unit_survive_at_tile(aunit, ptile)) {
          bounce_unit(aunit, verbose);
        }
      } unit_list_iterate_safe_end;
    }    
  } unit_list_iterate_safe_end;
}
				
/**************************************************************************
  When in civil war or an alliance breaks there will potentially be units 
  from both sides coexisting on the same squares.  This routine resolves 
  this by first bouncing off non-allied units from their cities, then by 
  bouncing both players' units in now illegal multiowner stacks.  To avoid
  drowning due to removal of transports, we bounce everyone (including
  third parties' units) from ocean tiles.

  If verbose is true, the unit owner gets messages about where each
  units goes.
**************************************************************************/
void resolve_unit_stacks(struct player *pplayer, struct player *aplayer,
                         bool verbose)
{
  throw_units_from_illegal_cities(pplayer, verbose);
  throw_units_from_illegal_cities(aplayer, verbose);
  
  resolve_stack_conflicts(pplayer, aplayer, verbose);
  resolve_stack_conflicts(aplayer, pplayer, verbose);
}

/****************************************************************************
  When two players cancel an alliance, a lot of units that were visible may
  no longer be visible (this includes units in transporters and cities).
  Call this function to inform the clients that these units are no longer
  visible.  Note that this function should be called _after_
  resolve_unit_stacks().
****************************************************************************/
void remove_allied_visibility(struct player* pplayer, struct player* aplayer)
{
  unit_list_iterate(aplayer->units, punit) {
    /* We don't know exactly which units have been hidden.  But only a unit
     * whose tile is visible but who aren't visible themselves are
     * candidates.  This solution just tells the client to drop all such
     * units.  If any of these are unknown to the client the client will
     * just ignore them. */
    if (map_is_known_and_seen(punit->tile, pplayer, V_MAIN) &&
        !can_player_see_unit(pplayer, punit)) {
      unit_goes_out_of_sight(pplayer, punit);
    }
  } unit_list_iterate_end;

  city_list_iterate(aplayer->cities, pcity) {
    /* The player used to know what units were in these cities.  Now that he
     * doesn't, he needs to get a new short city packet updating the
     * occupied status. */
    if (map_is_known_and_seen(pcity->tile, pplayer, V_MAIN)) {
      send_city_info(pplayer, pcity);
    }
  } city_list_iterate_end;
}

/**************************************************************************
 Is unit being refueled in its current position
**************************************************************************/
bool is_unit_being_refueled(const struct unit *punit)
{
  return (punit->transported_by != -1                   /* Carrier */
          || tile_city(punit->tile)                 /* City    */
          || tile_has_native_base(punit->tile,
                                  unit_type(punit))); /* Airbase */
}

/**************************************************************************
...
**************************************************************************/
bool is_airunit_refuel_point(struct tile *ptile, struct player *pplayer,
			     const struct unit_type *type,
			     bool unit_is_on_carrier)
{
  int cap;
  struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

  if (!is_non_allied_unit_tile(ptile, pplayer)) {
    if (is_allied_city_tile(ptile, pplayer)) {
      return TRUE;
    }

    base_type_iterate(pbase) {
      if (BV_ISSET(plrtile->bases, base_index(pbase))
          && is_native_base_to_utype(pbase, type)) {
        return TRUE;
      }
    } base_type_iterate_end;
  }

  cap = unit_class_transporter_capacity(ptile, pplayer, utype_class(type));

  if (unit_is_on_carrier) {
    cap++;
  }

  return cap > 0;
}

/**************************************************************************
  Really transforms a single unit.

  If calling this function for upgrade, you should use unit_upgrade
  before to test if this is possible.

  is_free: Does unit owner need to pay upgrade price.

  Note that this function is strongly tied to unit.c:test_unit_upgrade().
**************************************************************************/
void transform_unit(struct unit *punit, struct unit_type *to_unit,
                    bool is_free)
{
  struct player *pplayer = unit_owner(punit);
  int old_mr = unit_move_rate(punit), old_hp = unit_type(punit)->hp;

  if (!is_free) {
    pplayer->economic.gold -=
	unit_upgrade_price(pplayer, unit_type(punit), to_unit);
  }

  punit->utype = to_unit;

  /* Scale HP and MP, rounding down.  Be careful with integer arithmetic,
   * and don't kill the unit.  unit_move_rate is used to take into account
   * global effects like Magellan's Expedition. */
  punit->hp = MAX(punit->hp * unit_type(punit)->hp / old_hp, 1);
  punit->moves_left = punit->moves_left * unit_move_rate(punit) / old_mr;

  if (utype_has_flag(to_unit, F_NO_VETERAN)) {
    punit->veteran = 0;
  } else if (is_free) {
    punit->veteran = MAX(punit->veteran
                         - game.info.autoupgrade_veteran_loss, 0);
  } else {
    punit->veteran = MAX(punit->veteran
                         - game.info.upgrade_veteran_loss, 0);
  }

  /* update unit upkeep */
  city_units_upkeep(game_find_city_by_number(punit->homecity));

  conn_list_do_buffer(pplayer->connections);

  unit_refresh_vision(punit);

  send_unit_info(NULL, punit);
  conn_list_do_unbuffer(pplayer->connections);
}

/************************************************************************* 
  Wrapper of the below
*************************************************************************/
struct unit *create_unit(struct player *pplayer, struct tile *ptile, 
			 struct unit_type *type, int veteran_level, 
                         int homecity_id, int moves_left)
{
  return create_unit_full(pplayer, ptile, type, veteran_level, homecity_id, 
                          moves_left, -1, NULL);
}

/**************************************************************************
  Creates a unit, and set it's initial values, and put it into the right 
  lists.
  If moves_left is less than zero, unit will get max moves.
**************************************************************************/
struct unit *create_unit_full(struct player *pplayer, struct tile *ptile,
			      struct unit_type *type, int veteran_level, 
                              int homecity_id, int moves_left, int hp_left,
			      struct unit *ptrans)
{
  struct unit *punit = create_unit_virtual(pplayer, NULL, type, veteran_level);
  struct city *pcity;

  /* Register unit */
  punit->id = identity_number();
  idex_register_unit(punit);

  assert(ptile != NULL);
  punit->tile = ptile;

  pcity = game_find_city_by_number(homecity_id);
  if (utype_has_flag(type, F_NOHOME)) {
    punit->homecity = 0; /* none */
  } else {
    punit->homecity = homecity_id;
  }

  if (hp_left >= 0) {
    /* Override default full HP */
    punit->hp = hp_left;
  }

  if (moves_left >= 0) {
    /* Override default full MP */
    punit->moves_left = MIN(moves_left, unit_move_rate(punit));
  }

  if (ptrans) {
    /* Set transporter for unit. */
    punit->transported_by = ptrans->id;
  } else {
    assert(!ptile || can_unit_exist_at_tile(punit, ptile));
  }

  /* Assume that if moves_left < 0 then the unit is "fresh",
   * and not moved; else the unit has had something happen
   * to it (eg, bribed) which we treat as equivalent to moved.
   * (Otherwise could pass moved arg too...)  --dwp */
  punit->moved = (moves_left >= 0);

  unit_list_prepend(pplayer->units, punit);
  unit_list_prepend(ptile->units, punit);
  if (pcity && !utype_has_flag(type, F_NOHOME)) {
    assert(city_owner(pcity) == pplayer);
    unit_list_prepend(pcity->units_supported, punit);
    /* Refresh the unit's homecity. */
    city_refresh(pcity);
    send_city_info(pplayer, pcity);
  }

  punit->server.vision = vision_new(pplayer, ptile);
  unit_refresh_vision(punit);

  send_unit_info(NULL, punit);
  maybe_make_contact(ptile, unit_owner(punit));
  wakeup_neighbor_sentries(punit);

  /* update unit upkeep */
  city_units_upkeep(game_find_city_by_number(homecity_id));

  /* The unit may have changed the available tiles in nearby cities. */
  city_map_update_tile_now(ptile);
  sync_cities();

  /* Initialize aiferry stuff for new unit */
  aiferry_init_ferry(punit);

  return punit;
}

/**************************************************************************
We remove the unit and see if it's disappearance has affected the homecity
and the city it was in.
**************************************************************************/
static void server_remove_unit(struct unit *punit)
{
  struct tile *ptile = punit->tile;
  struct city *pcity = tile_city(ptile);
  struct city *phomecity = game_find_city_by_number(punit->homecity);

#ifndef NDEBUG
  unit_list_iterate(ptile->units, pcargo) {
    assert(pcargo->transported_by != punit->id);
  } unit_list_iterate_end;
#endif

  /* Since settlers plot in new cities in the minimap before they
     are built, so that no two settlers head towards the same city
     spot, we need to ensure this reservation is cleared should
     the settler disappear on the way. */
  if (punit->ai.ai_role != AIUNIT_NONE) {
    ai_unit_new_role(punit, AIUNIT_NONE, NULL);
  }

  conn_list_iterate(game.est_connections, pconn) {
    if ((NULL == pconn->playing && pconn->observer)
	|| (NULL != pconn->playing
            && map_is_known_and_seen(ptile, pconn->playing, V_MAIN))) {
      /* FIXME: this sends the remove packet to all players, even those who
       * can't see the unit.  This potentially allows some limited cheating.
       * However fixing it requires changes elsewhere since sometimes the
       * client is informed about unit disappearance only after the unit
       * disappears.  For instance when building a city the settler unit
       * is wiped only after the city is built...at which point the settler
       * is already "hidden" inside the city and can_player_see_unit would
       * return FALSE.  One possible solution is to have a bv_player for
       * each unit to record which players (clients) currently know about
       * the unit; then we could just use a BV_TEST here and not have to
       * worry about any synchronization problems. */
      dsend_packet_unit_remove(pconn, punit->id);
    }
  } conn_list_iterate_end;

  vision_clear_sight(punit->server.vision);
  vision_free(punit->server.vision);
  punit->server.vision = NULL;

  /* check if this unit had F_GAMELOSS flag */
  if (unit_has_type_flag(punit, F_GAMELOSS) && unit_owner(punit)->is_alive) {
    notify_conn(game.est_connections, ptile, E_UNIT_LOST_MISC,
                FTC_SERVER_INFO, NULL,
                _("Unable to defend %s, %s has lost the game."),
                unit_link(punit),
                player_name(unit_owner(punit)));
    notify_player(unit_owner(punit), ptile, E_GAME_END,
                  FTC_SERVER_INFO, NULL,
		  _("Losing %s meant losing the game! "
                  "Be more careful next time!"),
                  unit_link(punit));
    unit_owner(punit)->is_dying = TRUE;
  }

  game_remove_unit(punit);
  punit = NULL;

  /* This unit may have blocked tiles of adjacent cities. Update them. */
  city_map_update_tile_now(ptile);
  sync_cities();

  if (phomecity) {
    city_refresh(phomecity);
    send_city_info(city_owner(phomecity), phomecity);
  }

  if (pcity && pcity != phomecity) {
    city_refresh(pcity);
    send_city_info(city_owner(pcity), pcity);
  }

  if (pcity && unit_list_size(ptile->units) == 0) {
    /* The last unit in the city was killed: update the occupied flag. */
    send_city_info(NULL, pcity);
  }
}

/**************************************************************************
  Handle units destroyed when their transport is destroyed
**************************************************************************/
static void unit_lost_with_transport(const struct player *pplayer,
                                     struct unit *pcargo,
                                     struct unit_type *ptransport)
{
  notify_player(pplayer, pcargo->tile, E_UNIT_LOST_MISC,
                FTC_SERVER_INFO, NULL,
                _("%s lost when %s was lost."),
                unit_link(pcargo),
                utype_name_translation(ptransport));
  server_remove_unit(pcargo);
}

/**************************************************************************
  Remove the unit, and passengers if it is a carrying any. Remove the 
  _minimum_ number, eg there could be another boat on the square.
**************************************************************************/
void wipe_unit(struct unit *punit)
{
  struct tile *ptile = punit->tile;
  struct player *pplayer = unit_owner(punit);
  struct unit_type *putype_save = unit_type(punit); /* for notify messages */
  int drowning = 0;
  int saved_id = punit->id;
  int homecity_id = punit->homecity;

  /* First pull all units off of the transporter. */
  if (get_transporter_capacity(punit) > 0) {
    unit_list_iterate(ptile->units, pcargo) {
      if (pcargo->transported_by == punit->id) {
	/* Could use unload_unit_from_transporter here, but that would
	 * call send_unit_info for the transporter unnecessarily. */
	pull_unit_from_transporter(pcargo, punit);
        if (!can_unit_exist_at_tile(pcargo, ptile)) {
          drowning++;
        }
	if (pcargo->activity == ACTIVITY_SENTRY) {
	  /* Activate sentried units - like planes on a disbanded carrier.
	   * Note this will activate ground units even if they just change
	   * transporter. */
	  set_unit_activity(pcargo, ACTIVITY_IDLE);
	}
        if (!can_unit_exist_at_tile(pcargo, ptile)) {
          drowning++;
          /* No need for send_unit_info() here. Unit info will be sent
           * when it is assigned to a new transport or it will be removed. */
        } else {
          send_unit_info(NULL, pcargo);
        }
      }
    } unit_list_iterate_end;
  }

  script_signal_emit("unit_lost", 2,
                     API_TYPE_UNIT, punit,
                     API_TYPE_PLAYER, pplayer);

  if (unit_alive(saved_id)) {
    /* Now remove the unit. */
    server_remove_unit(punit);
  }

  /* update unit upkeep */
  city_units_upkeep(game_find_city_by_number(homecity_id));

  /* Finally reassign, bounce, or destroy all units that cannot exist at this
   * location without transport. */
  if (drowning) {
    struct city *pcity = NULL;

    /* First save undisbandable and gameloss units */
    unit_list_iterate_safe(ptile->units, pcargo) {
      if (pcargo->transported_by == -1
          && !can_unit_exist_at_tile(pcargo, ptile)
          && (unit_has_type_flag(pcargo, F_UNDISBANDABLE)
           || unit_has_type_flag(pcargo, F_GAMELOSS))) {
        struct unit *ptransport = find_transport_from_tile(pcargo, ptile);
        if (ptransport != NULL) {
          put_unit_onto_transporter(pcargo, ptransport);
          send_unit_info(NULL, pcargo);
        } else {
	  if (unit_has_type_flag(pcargo, F_UNDISBANDABLE)) {
	    pcity = find_closest_owned_city(unit_owner(pcargo),
					    pcargo->tile, TRUE, NULL);
	    if (pcity && teleport_unit_to_city(pcargo, pcity, 0, FALSE)) {
	      notify_player(pplayer, ptile, E_UNIT_RELOCATED,
                            FTC_SERVER_INFO, NULL,
                            _("%s escaped the destruction of %s, and "
                              "fled to %s."),
                            unit_link(pcargo),
                            utype_name_translation(putype_save),
                            city_link(pcity));
	    }
          }
          if (!unit_has_type_flag(pcargo, F_UNDISBANDABLE) || !pcity) {
            unit_lost_with_transport(pplayer, pcargo, putype_save);
          }
        }

        drowning--;
        if (!drowning) {
          break;
        }
      }
    } unit_list_iterate_safe_end;
  }

  /* Then other units */
  if (drowning) {
    unit_list_iterate_safe(ptile->units, pcargo) {
      if (pcargo->transported_by == -1
          && !can_unit_exist_at_tile(pcargo, ptile)) {
        struct unit *ptransport = find_transport_from_tile(pcargo, ptile);

        if (ptransport != NULL) {
          put_unit_onto_transporter(pcargo, ptransport);
          send_unit_info(NULL, pcargo);
        } else {
          unit_lost_with_transport(pplayer, pcargo, putype_save);
        }

        drowning--;
        if (!drowning) {
          break;
        }
      }
    } unit_list_iterate_safe_end;
  }
}

/**************************************************************************
  Called when one unit kills another in combat (this function is only
  called in one place).  It handles all side effects including
  notifications and killstack.
**************************************************************************/
void kill_unit(struct unit *pkiller, struct unit *punit, bool vet)
{
  char pkiller_link[MAX_LEN_NAME], punit_link[MAX_LEN_NAME];
  struct player *pvictim = unit_owner(punit);
  struct player *pvictor = unit_owner(pkiller);
  int ransom, unitcount = 0;

  sz_strlcpy(pkiller_link, unit_link(pkiller));
  sz_strlcpy(punit_link, unit_link(punit));

  /* barbarian leader ransom hack */
  if( is_barbarian(pvictim) && unit_has_type_role(punit, L_BARBARIAN_LEADER)
      && (unit_list_size(punit->tile->units) == 1)
      && uclass_has_flag(unit_class(pkiller), UCF_COLLECT_RANSOM)) {
    /* Occupying units can collect ransom if leader is alone in the tile */
    ransom = (pvictim->economic.gold >= game.info.ransom_gold) 
             ? game.info.ransom_gold : pvictim->economic.gold;
    notify_player(pvictor, pkiller->tile, E_UNIT_WIN_ATT,
                  FTC_SERVER_INFO, NULL,
                  _("Barbarian leader captured, %d gold ransom paid."),
                  ransom);
    pvictor->economic.gold += ransom;
    pvictim->economic.gold -= ransom;
    send_player_info(pvictor, NULL);   /* let me see my new gold :-) */
    unitcount = 1;
  }

  if (unitcount == 0) {
    unit_list_iterate(punit->tile->units, vunit)
      if (pplayers_at_war(pvictor, unit_owner(vunit)))
	unitcount++;
    unit_list_iterate_end;
  }

  if (!is_stack_vulnerable(punit->tile) || unitcount == 1) {
    notify_player(pvictor, pkiller->tile, E_UNIT_WIN_ATT,
                  FTC_SERVER_INFO, NULL,
		  /* TRANS: "... Cannon ... the Polish Destroyer." */
		  _("Your attacking %s succeeded against the %s %s!"),
		  pkiller_link,
		  nation_adjective_for_player(pvictim),
		  punit_link);
    if (vet) {
      notify_unit_experience(pkiller);
    }
    notify_player(pvictim, punit->tile, E_UNIT_LOST_DEF,
                  FTC_SERVER_INFO, NULL,
		  /* TRANS: "Cannon ... the Polish Destroyer." */
		  _("%s lost to an attack by the %s %s."),
		  punit_link,
		  nation_adjective_for_player(pvictor),
		  pkiller_link);

    wipe_unit(punit);
  } else { /* unitcount > 1 */
    int i;
    int num_killed[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
    struct unit *other_killed[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];

    assert(unitcount > 1);

    /* initialize */
    for (i = 0; i<MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS; i++) {
      num_killed[i] = 0;
      other_killed[i] = NULL;
    }

    /* count killed units */
    unit_list_iterate(punit->tile->units, vunit) {
      struct player *vplayer = unit_owner(vunit);
      if (pplayers_at_war(pvictor, vplayer)) {
	num_killed[player_index(vplayer)]++;
	if (vunit != punit) {
	  other_killed[player_index(vplayer)] = vunit;
	  other_killed[player_index(pvictor)] = vunit;
	}
      }
    } unit_list_iterate_end;

    /* Inform the destroyer: lots of different cases here! */
    notify_player(pvictor, pkiller->tile, E_UNIT_WIN_ATT,
                  FTC_SERVER_INFO, NULL,
		  /* TRANS: "... Cannon ... the Polish Destroyer ...." */
		  PL_("Your attacking %s succeeded against the %s %s "
		      "(and %d other unit)!",
		      "Your attacking %s succeeded against the %s %s "
		      "(and %d other units)!", unitcount - 1),
		  pkiller_link,
		  nation_adjective_for_player(pvictim),
		  punit_link,
		  unitcount - 1);
    if (vet) {
      notify_unit_experience(pkiller);
    }

    /* inform the owners: this only tells about owned units that were killed.
     * there may have been 20 units who died but if only 2 belonged to the
     * particular player they'll only learn about those.
     *
     * Also if a large number of units die you don't find out what type
     * they all are. */
    for (i = 0; i<MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS; i++) {
      if (num_killed[i] == 1) {
	if (i == player_index(pvictim)) {
	  assert(other_killed[i] == NULL);
	  notify_player(player_by_number(i), punit->tile, E_UNIT_LOST_DEF,
                        FTC_SERVER_INFO, NULL,
			/* TRANS: "Cannon ... the Polish Destroyer." */
			_("%s lost to an attack by the %s %s."),
			punit_link,
			nation_adjective_for_player(pvictor),
			pkiller_link);
	} else {
	  assert(other_killed[i] != punit);
	  notify_player(player_by_number(i), punit->tile, E_UNIT_LOST_DEF,
                        FTC_SERVER_INFO, NULL,
			/* TRANS: "Cannon lost when the Polish Destroyer
			 * attacked the German Musketeers." */
			_("%s lost when the %s %s attacked the %s %s."),
			unit_link(other_killed[i]),
			nation_adjective_for_player(pvictor),
			pkiller_link,
			nation_adjective_for_player(pvictim),
			punit_link);
	}
      } else if (num_killed[i] > 1) {
	if (i == player_index(pvictim)) {
	  int others = num_killed[i] - 1;

	  if (others == 1) {
	    notify_player(player_by_number(i), punit->tile, E_UNIT_LOST_DEF,
                          FTC_SERVER_INFO, NULL,
			  /* TRANS: "Musketeers (and Cannon) lost to an
			   * attack from the Polish Destroyer." */
			  _("%s (and %s) lost to an attack from the %s %s."),
			  punit_link,
			  unit_link(other_killed[i]),
			  nation_adjective_for_player(pvictor),
			  pkiller_link);
	  } else {
	    notify_player(player_by_number(i), punit->tile, E_UNIT_LOST_DEF,
                          FTC_SERVER_INFO, NULL,
			  /* TRANS: "Musketeers and 3 other units lost to
			   * an attack from the Polish Destroyer."
			   * (only happens with at least 2 other units) */
			  PL_("%s and %d other unit lost to an attack "
			      "from the %s %s.",
			      "%s and %d other units lost to an attack "
			      "from the %s %s.", others),
			  punit_link,
			  others,
			  nation_adjective_for_player(pvictor),
			  pkiller_link);
	  }
	} else {
	  notify_player(player_by_number(i), punit->tile, E_UNIT_LOST_DEF,
                        FTC_SERVER_INFO, NULL,
			/* TRANS: "2 units lost when the Polish Destroyer
			 * attacked the German Musketeers."
			 * (only happens with at least 2 other units) */
			PL_("%d unit lost when the %s %s attacked the %s %s.",
			    "%d units lost when the %s %s attacked the %s %s.",
			    num_killed[i]),
			num_killed[i],
			nation_adjective_for_player(pvictor),
			pkiller_link,
			nation_adjective_for_player(pvictim),
			punit_link);
	}
      }
    }

    /* remove the units */
    unit_list_iterate_safe(punit->tile->units, punit2) {
      if (pplayers_at_war(pvictor, unit_owner(punit2))) {
	wipe_unit(punit2);
      }
    } unit_list_iterate_safe_end;
  }
}

/**************************************************************************
  Package a unit_info packet.  This packet contains basically all
  information about a unit.
**************************************************************************/
void package_unit(struct unit *punit, struct packet_unit_info *packet)
{
  packet->id = punit->id;
  packet->owner = player_number(unit_owner(punit));
  packet->x = punit->tile->x;
  packet->y = punit->tile->y;
  packet->homecity = punit->homecity;
  output_type_iterate(o) {
    packet->upkeep[o] = punit->upkeep[o];
  } output_type_iterate_end;
  packet->veteran = punit->veteran;
  packet->type = utype_number(unit_type(punit));
  packet->movesleft = punit->moves_left;
  packet->hp = punit->hp;
  packet->activity = punit->activity;
  packet->activity_base = punit->activity_base;
  packet->activity_count = punit->activity_count;
  packet->ai = punit->ai.control;
  packet->fuel = punit->fuel;
  if (punit->goto_tile) {
    packet->goto_dest_x = punit->goto_tile->x;
    packet->goto_dest_y = punit->goto_tile->y;
  } else {
    packet->goto_dest_x = 255;
    packet->goto_dest_y = 255;
    assert(!is_normal_map_pos(255, 255));
  }
  packet->activity_target = punit->activity_target;
  packet->activity_base = punit->activity_base;
  packet->paradropped = punit->paradropped;
  packet->done_moving = punit->done_moving;
  if (punit->transported_by == -1) {
    packet->transported = FALSE;
    packet->transported_by = 0;
  } else {
    packet->transported = TRUE;
    packet->transported_by = punit->transported_by;
  }
  packet->occupy = get_transporter_occupancy(punit);
  packet->battlegroup = punit->battlegroup;
  packet->has_orders = punit->has_orders;
  if (punit->has_orders) {
    int i;

    packet->orders_length = punit->orders.length;
    packet->orders_index = punit->orders.index;
    packet->orders_repeat = punit->orders.repeat;
    packet->orders_vigilant = punit->orders.vigilant;
    for (i = 0; i < punit->orders.length; i++) {
      packet->orders[i] = punit->orders.list[i].order;
      packet->orders_dirs[i] = punit->orders.list[i].dir;
      packet->orders_activities[i] = punit->orders.list[i].activity;
      packet->orders_bases[i] = punit->orders.list[i].base;
    }
  } else {
    packet->orders_length = packet->orders_index = 0;
    packet->orders_repeat = packet->orders_vigilant = FALSE;
    /* No need to initialize array. */
  }
}

/**************************************************************************
  Package a short_unit_info packet.  This contains a limited amount of
  information about the unit, and is sent to players who shouldn't know
  everything (like the unit's owner's enemies).
**************************************************************************/
void package_short_unit(struct unit *punit,
			struct packet_unit_short_info *packet,
			enum unit_info_use packet_use,
			int info_city_id, bool new_serial_num)
{
  static unsigned int serial_num = 0;

  /* a 16-bit unsigned number, never zero */
  if (new_serial_num) {
    serial_num = (serial_num + 1) & 0xFFFF;
    if (serial_num == 0) {
      serial_num++;
    }
  }
  packet->serial_num = serial_num;
  packet->packet_use = packet_use;
  packet->info_city_id = info_city_id;

  packet->id = punit->id;
  packet->owner = player_number(unit_owner(punit));
  packet->x = punit->tile->x;
  packet->y = punit->tile->y;
  packet->veteran = punit->veteran;
  packet->type = utype_number(unit_type(punit));
  packet->hp = punit->hp;
  packet->occupied = (get_transporter_occupancy(punit) > 0);
  if (punit->activity == ACTIVITY_EXPLORE
      || punit->activity == ACTIVITY_GOTO) {
    packet->activity = ACTIVITY_IDLE;
    packet->activity_base = -1;
  } else {
    packet->activity = punit->activity;
    packet->activity_base = punit->activity_base;
  }

  /* Transported_by information is sent to the client even for units that
   * aren't fully known.  Note that for non-allied players, any transported
   * unit can't be seen at all.  For allied players we have to know if
   * transporters have room in them so that we can load units properly. */
  if (punit->transported_by == -1) {
    packet->transported = FALSE;
    packet->transported_by = 0;
  } else {
    packet->transported = TRUE;
    packet->transported_by = punit->transported_by;
  }

  packet->goes_out_of_sight = FALSE;
}

/**************************************************************************
...
**************************************************************************/
void unit_goes_out_of_sight(struct player *pplayer, struct unit *punit)
{
  if (unit_owner(punit) == pplayer) {
    /* Unit is either about to die, or to change owner (civil war, bribe) */
    struct packet_unit_remove packet;

    packet.unit_id = punit->id;
    lsend_packet_unit_remove(pplayer->connections, &packet);
  } else {
    struct packet_unit_short_info packet;

    memset(&packet, 0, sizeof(packet));
    packet.id = punit->id;
    packet.goes_out_of_sight = TRUE;
    lsend_packet_unit_short_info(pplayer->connections, &packet);
  }
}

/**************************************************************************
  Send the unit into to those connections in dest which can see the units
  at its position, or the specified ptile (if different).
  Eg, use ptile as where the unit came from, so that the info can be
  sent if the other players can see either the target or destination tile.
  dest = NULL means all connections (game.est_connections)
**************************************************************************/
void send_unit_info_to_onlookers(struct conn_list *dest, struct unit *punit,
				 struct tile *ptile, bool remove_unseen)
{
  struct packet_unit_info info;
  struct packet_unit_short_info sinfo;
  
  if (!dest) {
    dest = game.est_connections;
  }

  CHECK_UNIT(punit);

  package_unit(punit, &info);
  package_short_unit(punit, &sinfo, UNIT_INFO_IDENTITY, FALSE, FALSE);
            
  conn_list_iterate(dest, pconn) {
    struct player *pplayer = pconn->playing;

    /* Be careful to consider all cases where pplayer is NULL... */
    if ((!pplayer && pconn->observer) || pplayer == unit_owner(punit)) {
      send_packet_unit_info(pconn, &info);
    } else if (pplayer) {
      bool see_in_old;
      bool see_in_new = can_player_see_unit_at(pplayer, punit, punit->tile);

      if (punit->tile == ptile) {
	/* This is not about movement */
	see_in_old = see_in_new;
      } else {
	see_in_old = can_player_see_unit_at(pplayer, punit, ptile);
      }

      if (see_in_new || see_in_old) {
	/* First send movement */
	send_packet_unit_short_info(pconn, &sinfo);

	if (!see_in_new) {
	  /* Then remove unit if necessary */
	  unit_goes_out_of_sight(pplayer, punit);
	}
      } else {
	if (remove_unseen) {
	  dsend_packet_unit_remove(pconn, punit->id);
	}
      }
    }
  } conn_list_iterate_end;
}

/**************************************************************************
  send the unit to the players who need the info 
  dest = NULL means all connections (game.est_connections)
**************************************************************************/
void send_unit_info(struct player *dest, struct unit *punit)
{
  struct conn_list *conn_dest = (dest ? dest->connections
				 : game.est_connections);
  send_unit_info_to_onlookers(conn_dest, punit, punit->tile, FALSE);
}

/**************************************************************************
  For each specified connections, send information about all the units
  known to that player/conn.
**************************************************************************/
void send_all_known_units(struct conn_list *dest)
{
  conn_list_do_buffer(dest);
  conn_list_iterate(dest, pconn) {
    struct player *pplayer = pconn->playing;

    if (NULL == pplayer && !pconn->observer) {
      continue;
    }

    players_iterate(unitowner) {
      unit_list_iterate(unitowner->units, punit) {
	if (!pplayer || can_player_see_unit(pplayer, punit)) {
	  send_unit_info_to_onlookers(pconn->self, punit,
				      punit->tile, FALSE);
	}
      } unit_list_iterate_end;
    } players_iterate_end;
  }
  conn_list_iterate_end;
  conn_list_do_unbuffer(dest);
  flush_packets();
}

/**************************************************************************
  Nuke a square: 1) remove all units on the square, and 2) halve the 
  size of the city on the square.

  If it isn't a city square or an ocean square then with 50% chance add 
  some fallout, then notify the client about the changes.
**************************************************************************/
static void do_nuke_tile(struct player *pplayer, struct tile *ptile)
{
  struct city *pcity = NULL;

  unit_list_iterate_safe(ptile->units, punit) {
    notify_player(unit_owner(punit), ptile, E_UNIT_LOST_MISC,
                  FTC_SERVER_INFO, NULL,
		  _("Your %s was nuked by %s."),
		  unit_link(punit),
		  pplayer == unit_owner(punit)
		  ? _("yourself")
		  : nation_plural_for_player(pplayer));
    if (unit_owner(punit) != pplayer) {
      notify_player(pplayer, ptile, E_UNIT_WIN,
                    FTC_SERVER_INFO, NULL,
		    _("The %s %s was nuked."),
		    nation_adjective_for_player(unit_owner(punit)),
		    unit_link(punit));
    }
    wipe_unit(punit);
  } unit_list_iterate_safe_end;

  pcity = tile_city(ptile);

  if (pcity) {
    notify_player(city_owner(pcity), ptile, E_CITY_NUKED,
                  FTC_SERVER_INFO, NULL,
		  _("%s was nuked by %s."),
		  city_link(pcity),
		  pplayer == city_owner(pcity)
		  ? _("yourself")
		  : nation_plural_for_player(pplayer));

    if (city_owner(pcity) != pplayer) {
      notify_player(pplayer, ptile, E_CITY_NUKED,
                    FTC_SERVER_INFO, NULL,
		    _("You nuked %s."),
		    city_link(pcity));
    }

    city_reduce_size(pcity, pcity->size / 2, pplayer);
  }

  if (!is_ocean_tile(ptile) && myrand(2) == 1) {
    if (game.info.nuke_contamination == CONTAMINATION_POLLUTION) {
      if (!tile_has_special(ptile, S_POLLUTION)) {
	tile_set_special(ptile, S_POLLUTION);
	update_tile_knowledge(ptile);
      }
    } else {
      if (!tile_has_special(ptile, S_FALLOUT)) {
	tile_set_special(ptile, S_FALLOUT);
	update_tile_knowledge(ptile);
      }
    }
  }
}

/**************************************************************************
  Nuke all the squares in a 3x3 square around the center of the explosion
  pplayer is the player that caused the explosion.
**************************************************************************/
void do_nuclear_explosion(struct player *pplayer, struct tile *ptile)
{
  struct player *victim = tile_owner(ptile);

  call_incident(INCIDENT_NUCLEAR, pplayer, victim);

  square_iterate(ptile, 1, ptile1) {
    do_nuke_tile(pplayer, ptile1);
  } square_iterate_end;

  notify_conn(NULL, ptile, E_NUKE,
              FTC_SERVER_INFO, NULL,
	      _("The %s detonated a nuke!"),
	      nation_plural_for_player(pplayer));
}

/**************************************************************************
  go by airline, if both cities have an airport and neither has been used this
  turn the unit will be transported by it and have it's moves set to 0
**************************************************************************/
bool do_airline(struct unit *punit, struct city *city2)
{
  struct tile *src_tile = punit->tile;
  struct city *city1 = tile_city(src_tile);

  if (!city1)
    return FALSE;
  if (!unit_can_airlift_to(punit, city2))
    return FALSE;
  if (get_transporter_occupancy(punit) > 0) {
    return FALSE;
  }
  city1->airlift--;
  city2->airlift--;

  notify_player(unit_owner(punit), city2->tile, E_UNIT_RELOCATED,
                FTC_SERVER_INFO, NULL,
                _("%s transported successfully."),
                unit_link(punit));

  move_unit(punit, city2->tile, punit->moves_left);

  /* airlift fields have changed. */
  send_city_info(city_owner(city1), city1);
  send_city_info(city_owner(city2), city2);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
void do_explore(struct unit *punit)
{
  struct player *owner = unit_owner(punit);

  if (owner->ai->funcs.auto_explorer) {
    switch (owner->ai->funcs.auto_explorer(punit)) {
      case MR_DEATH:
        /* don't use punit! */
        return;
      case MR_OK:
        /* FIXME: ai_manage_explorer() isn't supposed to change the activity,
         * but don't count on this.  See PR#39792.
         */
        if (punit->activity == ACTIVITY_EXPLORE) {
          break;
        }
        /* fallthru */
      default:
        unit_activity_handling(punit, ACTIVITY_IDLE);

        /* FIXME: When the ai_manage_explorer() call changes the activity from
         * EXPLORE to IDLE, in unit_activity_handling() ai.control is left
         * alone.  We reset it here.  See PR#12931. */
        punit->ai.control = FALSE;
        break;
    };
  } else {
    unit_activity_handling(punit, ACTIVITY_IDLE);
    punit->ai.control = FALSE;
  }
  send_unit_info(NULL, punit); /* probably duplicate */
}

/**************************************************************************
  Returns whether the drop was made or not. Note that it also returns 1 
  in the case where the drop was succesful, but the unit was killed by
  barbarians in a hut.
**************************************************************************/
bool do_paradrop(struct unit *punit, struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);

  if (!unit_has_type_flag(punit, F_PARATROOPERS)) {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("This unit type can not be paradropped."));
    return FALSE;
  }

  if (!can_unit_paradrop(punit)) {
    return FALSE;
  }

  if (get_transporter_occupancy(punit) > 0) {
    notify_player(pplayer, punit->tile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("You cannot paradrop a unit that is "
                    "transporting other units."));
  }

  if (!map_is_known(ptile, pplayer)) {
    notify_player(pplayer, ptile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("The destination location is not known."));
    return FALSE;
  }

  /* Safe terrain according to player map? */
  if (!is_native_terrain(unit_type(punit),
                         map_get_player_tile(ptile, pplayer)->terrain,
                         map_get_player_tile(ptile, pplayer)->special,
                         map_get_player_tile(ptile, pplayer)->bases)) {
    notify_player(pplayer, ptile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("This unit cannot paradrop into %s."),
                  terrain_name_translation(
                      map_get_player_tile(ptile, pplayer)->terrain));
    return FALSE;
  }

  if (map_is_known_and_seen(ptile, pplayer, V_MAIN)
      && ((tile_city(ptile)
	  && pplayers_non_attack(pplayer, tile_owner(ptile)))
      || is_non_attack_unit_tile(ptile, pplayer))) {
    notify_player(pplayer, ptile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                  _("Cannot attack unless you declare war first."));
    return FALSE;    
  }

  {
    int range = unit_type(punit)->paratroopers_range;
    int distance = real_map_distance(punit->tile, ptile);
    if (distance > range) {
      notify_player(pplayer, ptile, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
                    _("The distance to the target (%i) "
                      "is greater than the unit's range (%i)."),
                    distance, range);
      return FALSE;
    }
  }

  /* Safe terrain, really? Not transformed since player last saw it. */
  if (!can_unit_exist_at_tile(punit, ptile)) {
    map_show_circle(pplayer, ptile, unit_type(punit)->vision_radius_sq);
    notify_player(pplayer, ptile, E_UNIT_LOST_MISC, FTC_SERVER_INFO, NULL,
                  _("Your %s paradropped into the %s and was lost."),
                  unit_link(punit),
                  terrain_name_translation(tile_terrain(ptile)));
    server_remove_unit(punit);
    return TRUE;
  }

  if ((tile_city(ptile) && pplayers_non_attack(pplayer, tile_owner(ptile)))
      || is_non_allied_unit_tile(ptile, pplayer)) {
    map_show_circle(pplayer, ptile, unit_type(punit)->vision_radius_sq);
    maybe_make_contact(ptile, pplayer);
    notify_player(pplayer, ptile, E_UNIT_LOST_MISC, FTC_SERVER_INFO, NULL,
                  _("Your %s was killed by enemy units at the "
                    "paradrop destination."),
                  unit_link(punit));
    server_remove_unit(punit);
    return TRUE;
  }

  /* All ok */
  {
    int move_cost = unit_type(punit)->paratroopers_mr_sub;
    punit->paradropped = TRUE;
    move_unit(punit, ptile, move_cost);
    return TRUE;
  }
}

/**************************************************************************
  Give 25 Gold or kill the unit. For H_LIMITEDHUTS
  Return TRUE if unit is alive, and FALSE if it was killed
**************************************************************************/
static bool hut_get_limited(struct unit *punit)
{
  bool ok = TRUE;
  int hut_chance = myrand(12);
  struct player *pplayer = unit_owner(punit);
  /* 1 in 12 to get barbarians */
  if (hut_chance != 0) {
    int cred = 25;
    notify_player(pplayer, punit->tile, E_HUT_GOLD, FTC_SERVER_INFO, NULL,
                  _("You found %d gold."), cred);
    pplayer->economic.gold += cred;
  } else if (city_exists_within_city_radius(punit->tile, TRUE)
             || unit_has_type_flag(punit, F_GAMELOSS)) {
    notify_player(pplayer, punit->tile, E_HUT_BARB_CITY_NEAR,
                  FTC_SERVER_INFO, NULL,
                  _("An abandoned village is here."));
  } else {
    notify_player(pplayer, unit_tile(punit), E_HUT_BARB_KILLED,
                  FTC_SERVER_INFO, NULL,
                  _("Your %s has been killed by barbarians!"),
                  unit_link(punit));
    wipe_unit(punit);
    ok = FALSE;
  }
  return ok;
}

/**************************************************************************
  Due to the effects in the scripted hut behavior can not be predicted,
  unit_enter_hut returns nothing.
**************************************************************************/
static void unit_enter_hut(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  enum hut_behavior behavior = unit_class(punit)->hut_behavior;

  /* FIXME: Should we still run "hut_enter" script when
   *        hut_behavior is HUT_NOTHING or HUT_FRIGHTEN? */ 
  if (behavior == HUT_NOTHING) {
    return;
  }

  tile_clear_special(punit->tile, S_HUT);
  update_tile_knowledge(punit->tile);

  if (behavior == HUT_FRIGHTEN) {
    notify_player(pplayer, punit->tile, E_HUT_BARB, FTC_SERVER_INFO, NULL,
                  _("Your overflight frightens the tribe;"
                    " they scatter in terror."));
    return;
  }
  
  /* AI with H_LIMITEDHUTS only gets 25 gold (or barbs if unlucky) */
  if (pplayer->ai_data.control && ai_handicap(pplayer, H_LIMITEDHUTS)) {
    (void) hut_get_limited(punit);
    return;
  }

  script_signal_emit("hut_enter", 1, API_TYPE_UNIT, punit);

  send_player_info(pplayer, pplayer);       /* eg, gold */
  return;
}

/****************************************************************************
  Put the unit onto the transporter.  Don't do any other work.
****************************************************************************/
static void put_unit_onto_transporter(struct unit *punit, struct unit *ptrans)
{
  /* In the future we may updated ptrans->occupancy. */
  assert(punit->transported_by == -1);
  punit->transported_by = ptrans->id;
}

/****************************************************************************
  Put the unit onto the transporter.  Don't do any other work.
****************************************************************************/
static void pull_unit_from_transporter(struct unit *punit,
				       struct unit *ptrans)
{
  /* In the future we may updated ptrans->occupancy. */
  assert(punit->transported_by == ptrans->id);
  punit->transported_by = -1;
}

/****************************************************************************
  Put the unit onto the transporter, and tell everyone.
****************************************************************************/
void load_unit_onto_transporter(struct unit *punit, struct unit *ptrans)
{
  put_unit_onto_transporter(punit, ptrans);
  send_unit_info(NULL, punit);
  send_unit_info(NULL, ptrans);
}

/****************************************************************************
  Pull the unit off of the transporter, and tell everyone.
****************************************************************************/
void unload_unit_from_transporter(struct unit *punit)
{
  struct unit *ptrans = game_find_unit_by_number(punit->transported_by);

  pull_unit_from_transporter(punit, ptrans);
  send_unit_info(NULL, punit);
  send_unit_info(NULL, ptrans);
}

/*****************************************************************
  This function is passed to unit_list_sort() to sort a list of
  units according to their win chance against autoattack_x|y.
  If the unit is being transported, then push it to the front of
  the list, since we wish to leave its transport out of combat
  if at all possible.
*****************************************************************/
static int compare_units(const struct unit *const *p1,
                         const struct unit *const *q1)
{
  struct unit *p1def = get_defender(*p1, autoattack_target);
  struct unit *q1def = get_defender(*q1, autoattack_target);
  int p1uwc = unit_win_chance(*p1, p1def);
  int q1uwc = unit_win_chance(*q1, q1def);

  if (p1uwc < q1uwc || (*q1)->transported_by > 0) {
    return -1; /* q is better */
  } else if (p1uwc == q1uwc) {
    return 0;
  } else {
    return 1; /* p is better */
  }
}

/*****************************************************************
  Check if unit survives enemy autoattacks. We assume that any
  unit that is adjacent to us can see us.
*****************************************************************/
static bool unit_survive_autoattack(struct unit *punit)
{
  struct unit_list *autoattack = unit_list_new();
  int moves = punit->moves_left;
  int sanity1 = punit->id;

  /* Kludge to prevent attack power from dropping to zero during calc */
  punit->moves_left = MAX(punit->moves_left, 1);

  adjc_iterate(punit->tile, ptile) {
    /* First add all eligible units to a unit list */
    unit_list_iterate(ptile->units, penemy) {
      struct player *enemyplayer = unit_owner(penemy);
      enum diplstate_type ds = 
            pplayer_get_diplstate(unit_owner(punit), enemyplayer)->type;

      if (game.info.autoattack
          && penemy->moves_left > 0
          && ds == DS_WAR
          && can_unit_attack_unit_at_tile(penemy, punit, punit->tile)) {
        unit_list_prepend(autoattack, penemy);
      }
    } unit_list_iterate_end;
  } adjc_iterate_end;

  /* The unit list is now sorted according to win chance against punit */
  autoattack_target = punit->tile; /* global variable */
  if (unit_list_size(autoattack) >= 2) {
    unit_list_sort(autoattack, &compare_units);
  }

  unit_list_iterate_safe(autoattack, penemy) {
    int sanity2 = penemy->id;
    struct unit *enemy_defender = get_defender(punit, penemy->tile);
    struct unit *punit_defender = get_defender(penemy, punit->tile);
    double punitwin = unit_win_chance(punit, enemy_defender);
    double penemywin = unit_win_chance(penemy, punit_defender);
    double threshold = 0.25;
    struct tile *ptile = penemy->tile;

    if (tile_city(ptile) && unit_list_size(ptile->units) == 1) {
      /* Don't leave city defenseless */
      threshold = 0.90;
    }

    if ((penemywin > 1.0 - punitwin
         || unit_has_type_flag(punit, F_DIPLOMAT)
         || get_transporter_capacity(punit) > 0)
        && penemywin > threshold) {
#ifdef REALLY_DEBUG_THIS
      freelog(LOG_TEST, "AA %s -> %s (%d,%d) %.2f > %.2f && > %.2f",
              unit_rule_name(penemy),
              unit_rule_name(punit), 
              TILE_XY(punit->tile),
              penemywin,
              1.0 - punitwin, 
              threshold);
#endif

      unit_activity_handling(penemy, ACTIVITY_IDLE);
      (void) unit_move_handling(penemy, punit->tile, FALSE, FALSE);
    }
#ifdef REALLY_DEBUG_THIS
      else {
      freelog(LOG_TEST, "!AA %s -> %s (%d,%d) %.2f > %.2f && > %.2f",
              unit_rule_name(penemy),
              unit_rule_name(punit), 
              TILE_XY(punit->tile),
              penemywin,
              1.0 - punitwin, 
              threshold);
      continue;
    }
#endif

    if (game_find_unit_by_number(sanity2)) {
      send_unit_info(NULL, penemy);
    }
    if (game_find_unit_by_number(sanity1)) {
      send_unit_info(NULL, punit);
    } else {
      unit_list_free(autoattack);
      return FALSE; /* moving unit dead */
    }
  } unit_list_iterate_safe_end;

  unit_list_free(autoattack);
  if (game_find_unit_by_number(sanity1)) {
    /* We could have lost movement in combat */
    punit->moves_left = MIN(punit->moves_left, moves);
    send_unit_info(NULL, punit);
    return TRUE;
  } else {
    return FALSE;
  }
}

/****************************************************************************
  Cancel orders for the unit.
****************************************************************************/
static void cancel_orders(struct unit *punit, char *dbg_msg)
{
  free_unit_orders(punit);
  send_unit_info(NULL, punit);
  freelog(LOG_DEBUG, "%s", dbg_msg);
}

/*****************************************************************
  Will wake up any neighboring enemy sentry units or patrolling 
  units.
*****************************************************************/
static void wakeup_neighbor_sentries(struct unit *punit)
{
  /* There may be sentried units with a sightrange>3, but we don't
     wake them up if the punit is farther away than 3. */
  square_iterate(punit->tile, 3, ptile) {
    unit_list_iterate(ptile->units, penemy) {
      int radius_sq = get_unit_vision_at(penemy, penemy->tile, V_MAIN);

      if (!pplayers_allied(unit_owner(punit), unit_owner(penemy))
	  && penemy->activity == ACTIVITY_SENTRY
	  && radius_sq >= sq_map_distance(punit->tile, ptile)
	  && can_player_see_unit(unit_owner(penemy), punit)
	  /* on board transport; don't awaken */
	  && can_unit_exist_at_tile(penemy, penemy->tile)) {
	set_unit_activity(penemy, ACTIVITY_IDLE);
	send_unit_info(NULL, penemy);
      }
    } unit_list_iterate_end;
  } square_iterate_end;

  /* Wakeup patrolling units we bump into.
     We do not wakeup units further away than 3 squares... */
  square_iterate(punit->tile, 3, ptile) {
    unit_list_iterate(ptile->units, ppatrol) {
      if (punit != ppatrol
	  && unit_has_orders(ppatrol)
	  && ppatrol->orders.vigilant) {
	if (maybe_cancel_patrol_due_to_enemy(ppatrol)) {
	  cancel_orders(ppatrol, "  stopping because of nearby enemy");
	  notify_player(unit_owner(ppatrol), ppatrol->tile, E_UNIT_ORDERS,
                        FTC_SERVER_INFO, NULL,
			_("Orders for %s aborted after enemy movement was "
			  "spotted."),
			unit_link(ppatrol));
	}
      }
    } unit_list_iterate_end;
  } square_iterate_end;
}

/**************************************************************************
Does: 1) updates the unit's homecity and the city it enters/leaves (the
         city's happiness varies). This also takes into account when the
	 unit enters/leaves a fortress.
      2) process any huts at the unit's destination.
      3) updates adjacent cities' unavailable tiles.

FIXME: Sometimes it is not necessary to send cities because the goverment
       doesn't care whether a unit is away or not.
**************************************************************************/
static void unit_move_consequences(struct unit *punit,
                                   struct tile *src_tile,
                                   struct tile *dst_tile,
                                   bool passenger)
{
  struct city *fromcity = tile_city(src_tile);
  struct city *tocity = tile_city(dst_tile);
  struct city *homecity_start_pos = NULL;
  struct city *homecity_end_pos = NULL;
  int homecity_id_start_pos = punit->homecity;
  int homecity_id_end_pos = punit->homecity;
  struct player *pplayer_start_pos = unit_owner(punit);
  struct player *pplayer_end_pos = pplayer_start_pos;
  struct unit_type *type_start_pos = unit_type(punit);
  struct unit_type *type_end_pos = type_start_pos;
  bool refresh_homecity_start_pos = FALSE;
  bool refresh_homecity_end_pos = FALSE;
  int saved_id = punit->id;
  bool unit_died = FALSE;

  if (tocity) {
    unit_enter_city(punit, tocity, passenger);

    if(!unit_alive(saved_id)) {
      /* Unit died inside unit_enter_city().
       * This means that some cleanup has already taken place when it was
       * removed from game. */
      unit_died = TRUE;
    } else {
      /* In case script has changed something about unit */
      pplayer_end_pos = unit_owner(punit);
      type_end_pos = unit_type(punit);
      homecity_id_end_pos = punit->homecity;
    }
  }

  if (homecity_id_start_pos != 0) {
    homecity_start_pos = game_find_city_by_number(homecity_id_start_pos);
  }
  if (homecity_id_start_pos != homecity_id_end_pos) {
    homecity_end_pos = game_find_city_by_number(homecity_id_end_pos);
  } else {
    homecity_end_pos = homecity_start_pos;
  }

  /* We only do refreshes for non-AI players to now make sure the AI turns
     doesn't take too long. Perhaps we should make a special refresh_city
     functions that only refreshed happines. */

  /* might have changed owners or may be destroyed */
  tocity = tile_city(dst_tile);

  if (tocity) { /* entering a city */
    if (tocity->owner == pplayer_end_pos) {
      if (tocity != homecity_end_pos && !pplayer_end_pos->ai_data.control) {
        city_refresh(tocity);
        send_city_info(pplayer_end_pos, tocity);
      }
    }
    if (homecity_start_pos) {
      refresh_homecity_start_pos = TRUE;
    }
  }

  if (fromcity) { /* leaving a city */
    if (homecity_start_pos) {
      refresh_homecity_start_pos = TRUE;
    }
    if (fromcity != homecity_start_pos
        && fromcity->owner == pplayer_start_pos
        && !pplayer_start_pos->ai_data.control) {
      city_refresh(fromcity);
      send_city_info(pplayer_start_pos, fromcity);
    }
  }

  /* entering/leaving a fortress or friendly territory */
  if (homecity_start_pos || homecity_end_pos) {
    if ((game.info.happyborders > 0 && tile_owner(src_tile) != tile_owner(dst_tile))
        || (tile_has_base_flag_for_unit(dst_tile,
                                        type_end_pos,
                                        BF_NOT_AGGRESSIVE)
            && is_friendly_city_near(pplayer_end_pos, dst_tile))
        || (tile_has_base_flag_for_unit(src_tile,
                                        type_start_pos,
                                        BF_NOT_AGGRESSIVE)
            && is_friendly_city_near(pplayer_start_pos, src_tile))) {
      refresh_homecity_start_pos = TRUE;
      refresh_homecity_end_pos = TRUE;
    }
  }

  if (refresh_homecity_start_pos && !pplayer_start_pos->ai_data.control) {
    city_refresh(homecity_start_pos);
    send_city_info(pplayer_start_pos, homecity_start_pos);
  }
  if (refresh_homecity_end_pos
      && (!refresh_homecity_start_pos
          || homecity_start_pos != homecity_end_pos)
      && !pplayer_end_pos->ai_data.control) {
    city_refresh(homecity_end_pos);
    send_city_info(pplayer_end_pos, homecity_end_pos);
  }

  city_map_update_tile_now(dst_tile);
  sync_cities();
}

/**************************************************************************
Check if the units activity is legal for a move , and reset it if it isn't.
**************************************************************************/
static void check_unit_activity(struct unit *punit)
{
  switch (punit->activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_SENTRY:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_GOTO:
    break;
  case ACTIVITY_POLLUTION:
  case ACTIVITY_ROAD:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_RAILROAD:
  case ACTIVITY_PILLAGE:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_BASE:
  case ACTIVITY_LAST:
    set_unit_activity(punit, ACTIVITY_IDLE);
    break;
  };
}

/**************************************************************************
  Moves a unit. No checks whatsoever! This is meant as a practical 
  function for other functions, like do_airline, which do the checking 
  themselves.

  If you move a unit you should always use this function, as it also sets 
  the transported_by unit field correctly. take_from_land is only relevant 
  if you have set transport_units. Note that the src and dest need not be 
  adjacent.

  Returns TRUE iff unit still alive.
**************************************************************************/
bool move_unit(struct unit *punit, struct tile *pdesttile, int move_cost)
{
  struct player *pplayer = unit_owner(punit);
  struct tile *psrctile = punit->tile;
  struct city *pcity;
  struct unit *ptransporter = NULL;
  struct vision *old_vision = punit->server.vision;
  struct vision *new_vision;
  int saved_id = punit->id;
  bool unit_lives;
    
  conn_list_do_buffer(pplayer->connections);

  /* Transporting units. We first make a list of the units to be moved and
     then insert them again. The way this is done makes sure that the
     units stay in the same order. */
  if (get_transporter_capacity(punit) > 0) {
    struct unit_list *cargo_units = unit_list_new();

    /* First make a list of the units to be moved. */
    unit_list_iterate(psrctile->units, pcargo) {
      if (pcargo->transported_by == punit->id) {
	unit_list_unlink(psrctile->units, pcargo);
	unit_list_prepend(cargo_units, pcargo);
      }
    } unit_list_iterate_end;

    /* Insert them again. */
    unit_list_iterate(cargo_units, pcargo) {
      /* FIXME: Some script may have killed unit while it has been
       *        in cargo_units list, but it has not been unlinked
       *        from this list. pcargo may be invalid pointer. */
      struct vision *old_vision = pcargo->server.vision;
      struct vision *new_vision = vision_new(unit_owner(pcargo), pdesttile);

      pcargo->server.vision = new_vision;
      vision_layer_iterate(v) {
	vision_change_sight(new_vision, v,
			    get_unit_vision_at(pcargo, pdesttile, v));
      } vision_layer_iterate_end;

      ASSERT_VISION(new_vision);

      /* Silently free orders since they won't be applicable anymore. */
      free_unit_orders(pcargo);

      pcargo->tile = pdesttile;

      unit_list_prepend(pdesttile->units, pcargo);
      check_unit_activity(pcargo);
      send_unit_info_to_onlookers(NULL, pcargo, psrctile, FALSE);

      vision_clear_sight(old_vision);
      vision_free(old_vision);

      unit_move_consequences(pcargo, psrctile, pdesttile, TRUE);
    } unit_list_iterate_end;
    unit_list_free(cargo_units);
  }

  unit_lives = unit_alive(saved_id);

  /* We first unfog the destination, then move the unit and send the
     move, and then fog the old territory. This means that the player
     gets a chance to see the newly explored territory while the
     client moves the unit, and both areas are visible during the
     move */

  /* Enhance vision if unit steps into a fortress */
  if (unit_lives) {
    new_vision = vision_new(unit_owner(punit), pdesttile);
    punit->server.vision = new_vision;
    vision_layer_iterate(v) {
      vision_change_sight(new_vision, v,
                          get_unit_vision_at(punit, pdesttile, v));
    } vision_layer_iterate_end;

    ASSERT_VISION(new_vision);

    /* Claim ownership of fortress? */
    if (tile_has_claimable_base(pdesttile, unit_type(punit))
        && (!tile_owner(pdesttile)
            || pplayers_at_war(tile_owner(pdesttile), pplayer))) {
      map_claim_ownership(pdesttile, pplayer, pdesttile);
      map_claim_border(pdesttile, pplayer);
      city_thaw_workers_queue();
      city_refresh_queue_processing();
    }

    unit_list_unlink(psrctile->units, punit);
    punit->tile = pdesttile;
    punit->moved = TRUE;
    if (punit->transported_by != -1) {
      ptransporter = game_find_unit_by_number(punit->transported_by);
      pull_unit_from_transporter(punit, ptransporter);
    }
    punit->moves_left = MAX(0, punit->moves_left - move_cost);
    if (punit->moves_left == 0) {
      punit->done_moving = TRUE;
    }
    unit_list_prepend(pdesttile->units, punit);
    check_unit_activity(punit);
  }

  /*
   * Transporter info should be send first becouse this allow us get right
   * update_menu effect in client side.
   */
  
  /*
   * Send updated information to anyone watching that transporter was unload
   * cargo.
   */
  if (ptransporter) {
    send_unit_info(NULL, ptransporter);
  }

  /* Send updated information to anyone watching.  If the unit moves
   * in or out of a city we update the 'occupied' field.  Note there may
   * be cities at both src and dest under some rulesets.
   *   If unit is about to take over enemy city, unit is seen by
   * those players seeing inside cities of old city owner. After city
   * has been transferred, updated info is sent by unit_enter_city() */
  if (unit_lives) {
    send_unit_info_to_onlookers(NULL, punit, psrctile, FALSE);
    
    /* Special checks for ground units in the ocean. */
    if (!can_unit_survive_at_tile(punit, pdesttile)) {
      ptransporter = find_transporter_for_unit(punit);
      if (ptransporter) {
        put_unit_onto_transporter(punit, ptransporter);
      }

      /* Set activity to sentry if boarding a ship. */
      if (ptransporter && !pplayer->ai_data.control && !unit_has_orders(punit)
          && !can_unit_exist_at_tile(punit, pdesttile)) {
        set_unit_activity(punit, ACTIVITY_SENTRY);
      }

      /*
       * Transporter info should be send first because this allow us get right
       * update_menu effect in client side.
       */
    
      /*
       * Send updated information to anyone watching that transporter has cargo.
       */
      if (ptransporter) {
        send_unit_info(NULL, ptransporter);
      }

      /*
       * Send updated information to anyone watching that unit is on transport.
       * All players without shared vison with owner player get
       * REMOVE_UNIT package.
       */
      send_unit_info_to_onlookers(NULL, punit, punit->tile, TRUE);
    }
  }

  if ((pcity = tile_city(psrctile))) {
    refresh_dumb_city(pcity);
  }
  if ((pcity = tile_city(pdesttile))) {
    refresh_dumb_city(pcity);
  }

  vision_clear_sight(old_vision);
  vision_free(old_vision);

  /* Remove hidden units (like submarines) which aren't seen anymore. */
  square_iterate(psrctile, 1, tile1) {
    players_iterate(pplayer) {
      /* We're only concerned with known, unfogged tiles which may contain
       * hidden units that are no longer visible.  These units will not
       * have been handled by the fog code, above. */
      if (TILE_KNOWN_SEEN == tile_get_known(tile1, pplayer)) {
        unit_list_iterate(tile1->units, punit2) {
          if (punit2 != punit && !can_player_see_unit(pplayer, punit2)) {
	    unit_goes_out_of_sight(pplayer, punit2);
	  }
        } unit_list_iterate_end;
      }
    } players_iterate_end;
  } square_iterate_end;

  if (unit_lives) {
    unit_move_consequences(punit, psrctile, pdesttile, FALSE);

    /* FIXME: Should signal emit be after sentried units have been
     *        waken up in case script causes unit death? */
    script_signal_emit("unit_moved", 3,
                       API_TYPE_UNIT, punit,
                       API_TYPE_TILE, psrctile,
                       API_TYPE_TILE, pdesttile);
    unit_lives = unit_alive(saved_id);

    if (!unit_lives) {
      /* Script caused unit to die */
      return FALSE;
    }
  }

  if (unit_lives) {
    wakeup_neighbor_sentries(punit);
    unit_lives = unit_survive_autoattack(punit);
  }

  if (unit_lives) {
    maybe_make_contact(pdesttile, unit_owner(punit));
  }

  conn_list_do_unbuffer(pplayer->connections);

  if (!unit_lives) {
    return FALSE;
  }

  if (game.info.timeout != 0 && game.server.timeoutaddenemymove > 0) {
    bool new_information_for_enemy = FALSE;

    phase_players_iterate(penemy) {
      /* Increase the timeout if an enemy unit moves and the
       * timeoutaddenemymove setting is in use. */
      if (penemy->is_connected
	  && pplayer != penemy
	  && pplayers_at_war(penemy, pplayer)
	  && can_player_see_unit(penemy, punit)) {
	new_information_for_enemy = TRUE;
	break;
      }
    } phase_players_iterate_end;

    if (new_information_for_enemy) {
      increase_timeout_because_unit_moved();
    }
  }

  /* Note, an individual call to move_unit may leave things in an unstable
   * state (e.g., negative transporter capacity) if more than one unit is
   * being moved at a time (e.g., bounce unit) and they are not done in the
   * right order.  This is probably not a bug. */

  if (tile_has_special(pdesttile, S_HUT)) {
    int saved_id = punit->id;

    unit_enter_hut(punit);

    return unit_alive(saved_id);
  }

  return TRUE;
}

/**************************************************************************
  Maybe cancel the goto if there is an enemy in the way
**************************************************************************/
static bool maybe_cancel_goto_due_to_enemy(struct unit *punit, 
                                           struct tile *ptile)
{
  return (is_non_allied_unit_tile(ptile, unit_owner(punit)) 
	  || is_non_allied_city_tile(ptile, unit_owner(punit)));
}

/**************************************************************************
  Maybe cancel the patrol as there is an enemy near.

  If you modify the wakeup range you should change it in
  wakeup_neighbor_sentries() too.
**************************************************************************/
static bool maybe_cancel_patrol_due_to_enemy(struct unit *punit)
{
  bool cancel = FALSE;
  int radius_sq = get_unit_vision_at(punit, punit->tile, V_MAIN);
  struct player *pplayer = unit_owner(punit);

  circle_iterate(punit->tile, radius_sq, ptile) {
    struct unit *penemy = is_non_allied_unit_tile(ptile, pplayer);

    struct vision_site *pdcity = map_get_player_site(ptile, pplayer);

    if ((penemy && can_player_see_unit(pplayer, penemy))
	|| (pdcity && !pplayers_allied(pplayer, vision_owner(pdcity))
	    && pdcity->occupied)) {
      cancel = TRUE;
      break;
    }
  } circle_iterate_end;

  return cancel;
}

/****************************************************************************
  Executes a unit's orders stored in punit->orders.  The unit is put on idle
  if an action fails or if "patrol" is set and an enemy unit is encountered.

  The return value will be TRUE if the unit lives, FALSE otherwise.  (This
  function used to return a goto_result enumeration, declared in gotohand.h.
  But this enumeration was never checked by the caller and just lead to
  confusion.  All the caller really needs to know is if the unit lived or
  died; everything else is handled internally within execute_orders.)

  If the orders are repeating the loop starts over at the beginning once it
  completes.  To avoid infinite loops on railroad we stop for this
  turn when the unit is back where it started, even if it have moves left.

  A unit will attack under orders only on its final action.
****************************************************************************/
bool execute_orders(struct unit *punit)
{
  struct tile *dst_tile;
  bool res, last_order;
  int unitid = punit->id;
  struct player *pplayer = unit_owner(punit);
  int moves_made = 0;
  enum unit_activity activity;
  Base_type_id base;

  assert(unit_has_orders(punit));

  if (punit->activity != ACTIVITY_IDLE) {
    /* Unit's in the middle of an activity; wait for it to finish. */
    punit->done_moving = TRUE;
    return TRUE;
  }

  freelog(LOG_DEBUG, "Executing orders for %s %d",
	  unit_rule_name(punit),
	  punit->id);   

  /* Any time the orders are canceled we should give the player a message. */

  while (TRUE) {
    struct unit_order order;

    if (punit->moves_left == 0) {
      /* FIXME: this check won't work when actions take 0 MP. */
      freelog(LOG_DEBUG, "  stopping because of no more move points");
      return TRUE;
    }

    if (punit->done_moving) {
      freelog(LOG_DEBUG, "  stopping because we're done this turn");
      return TRUE;
    }

    if (punit->orders.vigilant && maybe_cancel_patrol_due_to_enemy(punit)) {
      /* "Patrol" orders are stopped if an enemy is near. */
      cancel_orders(punit, "  stopping because of nearby enemy");
      notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                    FTC_SERVER_INFO, NULL,
                    _("Orders for %s aborted as there are units nearby."),
                    unit_link(punit));
      return TRUE;
    }

    if (moves_made == punit->orders.length) {
      /* For repeating orders, don't repeat more than once per turn. */
      freelog(LOG_DEBUG, "  stopping because we ran a round");
      punit->done_moving = TRUE;
      send_unit_info(NULL, punit);
      return TRUE;
    }
    moves_made++;

    last_order = (!punit->orders.repeat
		  && punit->orders.index + 1 == punit->orders.length);

    order = punit->orders.list[punit->orders.index];
    if (last_order) {
      /* Clear the orders before we engage in the move.  That way any
       * has_orders checks will yield FALSE and this will be treated as
       * a normal move.  This is important: for instance a caravan goto
       * will popup the caravan dialog on the last move only. */
      free_unit_orders(punit);
    }

    /* Advance the orders one step forward.  This is needed because any
     * updates sent to the client as a result of the action should include
     * the new index value.  Note that we have to send_unit_info somewhere
     * after this point so that the client is properly updated. */
    punit->orders.index++;

    switch (order.order) {
    case ORDER_FULL_MP:
      if (punit->moves_left != unit_move_rate(punit)) {
	/* If the unit doesn't have full MP then it just waits until the
	 * next turn.  We assume that the next turn it will have full MP
	 * (there's no check for that). */
	punit->done_moving = TRUE;
	freelog(LOG_DEBUG, "  waiting this turn");
	send_unit_info(NULL, punit);
      }
      break;
    case ORDER_BUILD_CITY:
      ai_unit_build_city(pplayer, unitid,
			     city_name_suggestion(pplayer, punit->tile));
      freelog(LOG_DEBUG, "  building city");
      if (player_find_unit_by_id(pplayer, unitid)) {
	/* Build failed. */
	cancel_orders(punit, " orders canceled; failed to build city");
	notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Orders for %s aborted because building "
                        "of city failed."),
                      unit_link(punit));
	return TRUE;
      } else {
	/* Build succeeded => unit "died" */
	return FALSE;
      }
    case ORDER_ACTIVITY:
      activity = order.activity;
      base = order.base;
      if ((activity == ACTIVITY_BASE && !can_unit_do_activity_base(punit, base))
          || (activity != ACTIVITY_BASE
              && !can_unit_do_activity(punit, activity))) {
	cancel_orders(punit, "  orders canceled because of failed activity");
	notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Orders for %s aborted since they "
                        "give an invalid activity."),
                      unit_link(punit));
	return TRUE;
      }
      punit->done_moving = TRUE;

      if (activity != ACTIVITY_BASE) {
        set_unit_activity(punit, activity);
      } else {
        set_unit_activity_base(punit, base);
      }
      send_unit_info(NULL, punit);
      break;
    case ORDER_MOVE:
      /* Move unit */
      if (!(dst_tile = mapstep(punit->tile, order.dir))) {
	cancel_orders(punit, "  move order sent us to invalid location");
	/* FIXME: annoys webclient:
               notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Orders for %s aborted since they "
                        "give an invalid location."),
                      unit_link(punit));*/
	return TRUE;
      }

      /* FIXME: the web client want gotos to result in attack.
       if (!last_order
	  && maybe_cancel_goto_due_to_enemy(punit, dst_tile)) {
	cancel_orders(punit, "  orders canceled because of enemy");
	notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Orders for %s aborted as there "
                        "are units in the way."),
                      unit_link(punit));
	return TRUE;
      }*/

      freelog(LOG_DEBUG, "  moving to %d,%d",
	      dst_tile->x, dst_tile->y);
      res = unit_move_handling(punit, dst_tile, FALSE, !last_order);
      if (!player_find_unit_by_id(pplayer, unitid)) {
	freelog(LOG_DEBUG, "  unit died while moving.");
	/* A player notification should already have been sent. */
	return FALSE;
      }

      if (res && !same_pos(dst_tile, punit->tile)) {
	/* Movement succeeded but unit didn't move. */
	freelog(LOG_DEBUG, "  orders resulted in combat.");
	send_unit_info(NULL, punit);
	return TRUE;
      }

      if (!res && punit->moves_left > 0) {
	/* Movement failed (ZOC, etc.) */
	cancel_orders(punit, "  attempt to move failed.");
	/*notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Orders for %s aborted because of failed move."),
                      unit_link(punit));*/
	return TRUE;
      }

      if (!res && punit->moves_left == 0) {
	/* Movement failed (not enough MP).  Keep this move around for
	 * next turn. */
	freelog(LOG_DEBUG, "  orders move failed (out of MP).");
	if (unit_has_orders(punit)) {
	  punit->orders.index--;
	} else {
	  /* FIXME: If this was the last move, the orders will already have
	   * been freed, so we have to add them back on.  This is quite a
	   * hack; one problem is that the no-orders unit has probably
	   * already had its unit info sent out to the client. */
	  punit->has_orders = TRUE;
	  punit->orders.length = 1;
	  punit->orders.index = 0;
	  punit->orders.repeat = FALSE;
	  punit->orders.vigilant = FALSE;
	  punit->orders.list = fc_malloc(sizeof(order));
	  punit->orders.list[0] = order;
	}
	send_unit_info(NULL, punit);
	return TRUE;
      }

      break;
    case ORDER_DISBAND:
      freelog(LOG_DEBUG, "  orders: disbanding");
      handle_unit_disband(pplayer, unitid);
      return FALSE;
    case ORDER_HOMECITY:
      freelog(LOG_DEBUG, "  orders: changing homecity");
      if (tile_city(punit->tile)) {
	handle_unit_change_homecity(pplayer, unitid, tile_city(punit->tile)->id);
      } else {
	cancel_orders(punit, "  no homecity");
	notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Attempt to change homecity for %s failed."),
                      unit_link(punit));
	return TRUE;
      }
      break;
    case ORDER_TRADEROUTE:
      freelog(LOG_DEBUG, "  orders: establishing trade route.");
      handle_unit_establish_trade(pplayer, unitid);
      if (player_find_unit_by_id(pplayer, unitid)) {
	cancel_orders(punit, "  no trade route city");
	notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Attempt to establish trade route for %s failed."),
                      unit_link(punit));
	return TRUE;
      } else {
	return FALSE;
      }
    case ORDER_BUILD_WONDER:
      freelog(LOG_DEBUG, "  orders: building wonder");
      handle_unit_help_build_wonder(pplayer, unitid);
      if (player_find_unit_by_id(pplayer, unitid)) {
	cancel_orders(punit, "  no wonder city");
	notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                      FTC_SERVER_INFO, NULL,
                      _("Attempt to build wonder for %s failed."),
                      unit_link(punit));
	return TRUE;
      } else {
	return FALSE;
      }
    case ORDER_LAST:
      cancel_orders(punit, "  client sent invalid order!");
      notify_player(pplayer, punit->tile, E_UNIT_ORDERS,
                    FTC_SERVER_INFO, NULL,
                    _("Your %s has invalid orders."),
                    unit_link(punit));
      return TRUE;
    }

    if (last_order) {
      assert(punit->has_orders == FALSE);
      freelog(LOG_DEBUG, "  stopping because orders are complete");
      return TRUE;
    }

    if (punit->orders.index == punit->orders.length) {
      assert(punit->orders.repeat);
      /* Start over. */
      freelog(LOG_DEBUG, "  repeating orders.");
      punit->orders.index = 0;
    }
  } /* end while */
}

/****************************************************************************
  Return the vision the unit will have at the given tile.  The base vision
  range may be modified by effects.

  Note that vision MUST be independent of transported_by for this to work
  properly.
****************************************************************************/
int get_unit_vision_at(struct unit *punit, struct tile *ptile,
		       enum vision_layer vlayer)
{
  const int base = (unit_type(punit)->vision_radius_sq
		    + get_unittype_bonus(unit_owner(punit), ptile, unit_type(punit),
					 EFT_UNIT_VISION_RADIUS_SQ));
  switch (vlayer) {
  case V_MAIN:
    return base;
  case V_INVIS:
    return MIN(base, 2);
  case V_COUNT:
    break;
  }

  assert(0);
  return 0;
}

/****************************************************************************
  Refresh the unit's vision.

  This function has very small overhead and can be called any time effects
  may have changed the vision range of the city.
****************************************************************************/
void unit_refresh_vision(struct unit *punit)
{
  struct vision *uvision = punit->server.vision;

  vision_layer_iterate(v) {
    /* This requires two calls to get_unit_vision_at...it could be
     * optimized. */
    vision_change_sight(uvision, v,
			get_unit_vision_at(punit, punit->tile, v));
  } vision_layer_iterate_end;

  ASSERT_VISION(uvision);
}

/****************************************************************************
  Refresh the vision of all units in the list - see unit_refresh_vision.
****************************************************************************/
void unit_list_refresh_vision(struct unit_list *punitlist)
{
  unit_list_iterate(punitlist, punit) {
    unit_refresh_vision(punit);
  } unit_list_iterate_end;
}
