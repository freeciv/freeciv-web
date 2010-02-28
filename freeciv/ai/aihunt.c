/**********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Team
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

#include "city.h"
#include "combat.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "log.h"
#include "pf_tools.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"

#include "citytools.h"
#include "settlers.h"
#include "unittools.h"

#include "aidata.h"
#include "ailog.h"
#include "aitools.h"
#include "aiunit.h"

#include "aihunt.h"

/**************************************************************************
  We don't need a hunter in this city if we already have one. Return 
  existing hunter if any.
**************************************************************************/
static struct unit *ai_hunter_find(struct player *pplayer, 
                                   struct city *pcity)
{
  unit_list_iterate(pcity->units_supported, punit) {
    if (ai_hunter_qualify(pplayer, punit)) {
      return punit;
    }
  } unit_list_iterate_end;
  unit_list_iterate(pcity->tile->units, punit) {
    if (ai_hunter_qualify(pplayer, punit)) {
      return punit;
    }
  } unit_list_iterate_end;

  return NULL;
}

/**************************************************************************
  Guess best hunter unit type.
**************************************************************************/
static struct unit_type *ai_hunter_guess_best(struct city *pcity,
					      enum unit_move_type umt)
{
  struct unit_type *bestid = NULL;
  int best = 0;

  unit_type_iterate(ut) {
    int desire;

    if (utype_move_type(ut) != umt
     || !can_city_build_unit_now(pcity, ut)
     || ut->attack_strength < ut->transport_capacity) {
      continue;
    }

    desire = (ut->hp
              * ut->attack_strength
              * ut->firepower
              * ut->move_rate
              + ut->defense_strength) / MAX(UNITTYPE_COSTS(ut), 1);

    unit_class_iterate(uclass) {
      if (can_unit_type_transport(ut, uclass)
          && uclass_has_flag(uclass, UCF_MISSILE)) {
        desire += desire / 6;
        break;
      }
    } unit_class_iterate_end;

    if (utype_has_flag(ut, F_IGTER)) {
      desire += desire / 2;
    }
    if (utype_has_flag(ut, F_PARTIAL_INVIS)) {
      desire += desire / 4;
    }
    if (!can_attack_non_native(ut)) {
      desire -= desire / 4; /* less flexibility */
    }
    /* Causes continual unhappiness */
    if (utype_has_flag(ut, F_FIELDUNIT)) {
      desire /= 2;
    }

    desire = amortize(desire,
		      (utype_build_shield_cost(ut)
		       / MAX(pcity->surplus[O_SHIELD], 1)));

    if (desire > best) {
        best = desire;
        bestid = ut;
    }
  } unit_type_iterate_end;

  return bestid;
}

/**************************************************************************
  Check if we want to build a missile for our hunter.
**************************************************************************/
static void ai_hunter_missile_want(struct player *pplayer,
                                   struct city *pcity,
                                   struct ai_choice *choice)
{
  int best = -1;
  struct unit_type *best_unit_type = NULL;
  struct unit *hunter = NULL;

  unit_list_iterate(pcity->tile->units, punit) {
    if (ai_hunter_qualify(pplayer, punit)) {
      unit_class_iterate(uclass) {
        if (can_unit_type_transport(unit_type(punit), uclass)
            && uclass_has_flag(uclass, UCF_MISSILE)) {
          hunter = punit;
          break;
        }
      } unit_class_iterate_end;
      if (hunter) {
        break;
      }
    }
  } unit_list_iterate_end;

  if (!hunter) {
    return;
  }

  unit_type_iterate(ut) {
    int desire;

    if (!uclass_has_flag(utype_class(ut), UCF_MISSILE)
     || !can_city_build_unit_now(pcity, ut)) {
      continue;
    }

    if (!can_unit_type_transport(unit_type(hunter), utype_class(ut))) {
      continue;
    }

    /* FIXME: We need to store some data that can tell us if
     * enemy transports are protected by anti-missile technology. 
     * In this case, want nuclear much more! */
    desire = (ut->hp
              * MIN(ut->attack_strength, 30) /* nuke fix */
              * ut->firepower
              * ut->move_rate) / UNITTYPE_COSTS(ut) + 1;

    /* Causes continual unhappiness */
    if (utype_has_flag(ut, F_FIELDUNIT)) {
      desire /= 2;
    }

    desire = amortize(desire,
		      (utype_build_shield_cost(ut)
		       / MAX(pcity->surplus[O_SHIELD], 1)));

    if (desire > best) {
        best = desire;
        best_unit_type = ut;
    }
  } unit_type_iterate_end;

  if (best > choice->want) {
    CITY_LOG(LOGLEVEL_HUNT, pcity, "pri missile w/ want %d", best);
    choice->value.utype = best_unit_type;
    choice->want = best;
    choice->type = CT_ATTACKER;
    choice->need_boat = FALSE;
  } else if (best != -1) {
    CITY_LOG(LOGLEVEL_HUNT, pcity, "not pri missile w/ want %d"
             "(old want %d)", best, choice->want);
  }
}

/**************************************************************************
  Support function for ai_hunter_choice()
**************************************************************************/
static void eval_hunter_want(struct player *pplayer, struct city *pcity,
                             struct ai_choice *choice,
			     struct unit_type *best_type,
                             int veteran)
{
  struct unit *virtualunit;
  int want = 0;

  virtualunit = create_unit_virtual(pplayer, pcity, best_type, veteran);
  want = ai_hunter_manage(pplayer, virtualunit);
  destroy_unit_virtual(virtualunit);
  if (want > choice->want) {
    CITY_LOG(LOGLEVEL_HUNT, pcity, "pri hunter w/ want %d", want);
    choice->value.utype = best_type;
    choice->want = want;
    choice->type = CT_ATTACKER;
    choice->need_boat = FALSE;
  }
}

/**************************************************************************
  Check if we want to build a hunter.
**************************************************************************/
void ai_hunter_choice(struct player *pplayer, struct city *pcity,
                      struct ai_choice *choice)
{
  struct unit_type *best_land_hunter
    = ai_hunter_guess_best(pcity, LAND_MOVING);
  struct unit_type *best_sea_hunter
    = ai_hunter_guess_best(pcity, SEA_MOVING);
  struct unit *hunter = ai_hunter_find(pplayer, pcity);

  if ((!best_land_hunter && !best_sea_hunter)
      || is_barbarian(pplayer) || !pplayer->is_alive
      || ai_handicap(pplayer, H_TARGETS)) {
    return; /* None available */
  }
  if (hunter) {
    /* Maybe want missiles to go with a hunter instead? */
    ai_hunter_missile_want(pplayer, pcity, choice);
    return;
  }

  if (best_sea_hunter) {
    eval_hunter_want(pplayer, pcity, choice, best_sea_hunter, 
                     do_make_unit_veteran(pcity, best_sea_hunter));
  }
  if (best_land_hunter) {
    eval_hunter_want(pplayer, pcity, choice, best_land_hunter, 
                     do_make_unit_veteran(pcity, best_land_hunter));
  }
}

/**************************************************************************
  Does this unit qualify as a hunter?
**************************************************************************/
bool ai_hunter_qualify(struct player *pplayer, struct unit *punit)
{
  if (is_barbarian(pplayer) || unit_owner(punit) != pplayer) {
    return FALSE;
  }
  if (unit_has_type_role(punit, L_HUNTER)) {
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  Try to shoot our target with a missile. Also shoot down anything that
  might attempt to intercept _us_. We assign missiles to a hunter in
  ai_unit_new_role().
**************************************************************************/
static void ai_hunter_try_launch(struct player *pplayer,
                                 struct unit *punit,
                                 struct unit *target)
{
  int target_sanity = target->id;
  struct pf_parameter parameter;
  struct pf_map *pfm;

  unit_list_iterate(punit->tile->units, missile) {
    struct unit *sucker = NULL;

    if (unit_owner(missile) == pplayer
        && uclass_has_flag(unit_class(missile), UCF_MISSILE)) {
      UNIT_LOG(LOGLEVEL_HUNT, missile, "checking for hunt targets");
      pft_fill_unit_parameter(&parameter, punit);
      pfm = pf_map_new(&parameter);

      pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
        if (move_cost > missile->moves_left / SINGLE_MOVE) {
          break;
        }
        if (tile_city(ptile) || !can_unit_attack_tile(punit, ptile)) {
          continue;
        }
        unit_list_iterate(ptile->units, victim) {
          struct unit_type *ut = unit_type(victim);
          enum diplstate_type ds =
	    pplayer_get_diplstate(pplayer, unit_owner(victim))->type;

          if (ds != DS_WAR) {
            continue;
          }
          if (victim == target) {
            sucker = victim;
            UNIT_LOG(LOGLEVEL_HUNT, missile, "found primary target %d(%d, %d)"
                     " dist %d", victim->id, TILE_XY(victim->tile), 
                     move_cost);
            break; /* Our target! Get him!!! */
          }
          if (ut->move_rate + victim->moves_left > move_cost
              && ATTACK_POWER(victim) > DEFENCE_POWER(punit)
              && (utype_move_type(ut) == SEA_MOVING
                  || utype_move_type(ut) == BOTH_MOVING)) {
            /* Threat to our carrier. Kill it. */
            sucker = victim;
            UNIT_LOG(LOGLEVEL_HUNT, missile, "found aux target %d(%d, %d)",
                     victim->id, TILE_XY(victim->tile));
            break;
          }
        } unit_list_iterate_end;
        if (sucker) {
          break; /* found something - kill it! */
        }
      } pf_map_iterate_move_costs_end;
      pf_map_destroy(pfm);
      if (sucker) {
        if (game_find_unit_by_number(missile->transported_by)) {
          unload_unit_from_transporter(missile);
        }
        ai_unit_goto(missile, sucker->tile);
        sucker = game_find_unit_by_number(target_sanity); /* Sanity */
        if (sucker && is_tiles_adjacent(sucker->tile, missile->tile)) {
          ai_unit_attack(missile, sucker->tile);
        }
        target = game_find_unit_by_number(target_sanity); /* Sanity */
        break; /* try next missile, if any */
      }
    } /* if */
  } unit_list_iterate_end;
}

/**************************************************************************
  Calculate desire to crush this target.
**************************************************************************/
static void ai_hunter_juiciness(struct player *pplayer, struct unit *punit,
                                struct unit *target, int *stackthreat,
                                int *stackcost)
{
  *stackthreat = 0;
  *stackcost = 0;

  unit_list_iterate(target->tile->units, sucker) {
    *stackthreat += ATTACK_POWER(sucker);
    if (unit_has_type_flag(sucker, F_GAMELOSS)) {
      *stackcost += 1000;
      *stackthreat += 5000;
    }
    if (unit_has_type_flag(sucker, F_DIPLOMAT)) {
      *stackthreat += 500; /* extra threatening */
    }
    *stackcost += unit_build_shield_cost(sucker);
  } unit_list_iterate_end;

  *stackthreat *= 9; /* WAG - reduced by distance later */
  *stackthreat += *stackcost;
}

/**************************************************************************
  Manage a (possibly virtual) hunter. Return the want for building a 
  hunter like this. If we return 0, then we have nothing to do with
  the hunter. If we return -1, then we succeeded, and can try again.
  If we return > 0 then we are hunting but ran out of moves (this is
  also used for construction want).

  We try to keep track of our original target, but also opportunistically
  snatch up closer targts if they are better.

  We set punit->ai.target to target's id.
**************************************************************************/
int ai_hunter_manage(struct player *pplayer, struct unit *punit)
{
  bool is_virtual = (punit->id == 0);
  struct pf_parameter parameter;
  struct pf_map *pfm;
  int limit = unit_move_rate(punit) * 6;
  struct unit *original_target = game_find_unit_by_number(punit->ai.target);
  int original_threat = 0, original_cost = 0;

  assert(!is_barbarian(pplayer));
  assert(pplayer->is_alive);

  pft_fill_unit_parameter(&parameter, punit);
  pfm = pf_map_new(&parameter);

  if (original_target) {
    ai_hunter_juiciness(pplayer, punit, original_target, 
                        &original_threat, &original_cost);
  }

  pf_map_iterate_move_costs(pfm, ptile, move_cost, FALSE) {
    /* End faster if we have a target */
    if (move_cost > limit) {
      UNIT_LOG(LOGLEVEL_HUNT, punit, "gave up finding hunt target");
      pf_map_destroy(pfm);
      return 0;
    }
    unit_list_iterate_safe(ptile->units, target) {
      struct player *aplayer = unit_owner(target);
      int dist1, dist2, stackthreat = 0, stackcost = 0;
      int sanity_target = target->id;
      struct pf_path *path;

      /* Note that we need not (yet) be at war with aplayer */
      if (!is_player_dangerous(pplayer, aplayer)) {
        continue;
      }
      if (tile_city(ptile)
          || !can_unit_attack_tile(punit, ptile)
          || TEST_BIT(target->ai.hunted, player_index(pplayer))) {
        /* Can't hunt this one.  The bit is cleared in the beginning
         * of each turn. */
        continue;
      }
      if (!unit_has_type_flag(target, F_DIPLOMAT)
          && get_transporter_capacity(target) == 0
          && !unit_has_type_flag(target, F_GAMELOSS)) {
        /* Won't hunt this one. */
        continue;
      }

      /* Figure out whether unit is coming closer */
      if (target->ai.cur_pos && target->ai.prev_pos) {
        dist1 = real_map_distance(punit->tile, *target->ai.cur_pos);
        dist2 = real_map_distance(punit->tile, *target->ai.prev_pos);
      } else {
        dist1 = dist2 = 0;
      }
      UNIT_LOG(LOGLEVEL_HUNT, punit, "considering chasing %s[%d](%d, %d) "
               "dist1 %d dist2 %d",
	       unit_rule_name(target),
               target->id,
	       TILE_XY(target->tile),
               dist1, dist2);

      /* We can't chase if we aren't faster or on intercept vector */
      if (unit_type(punit)->move_rate < unit_type(target)->move_rate
          && dist1 >= dist2) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "giving up racing %s (%d, %d)->(%d, %d)",
                 unit_rule_name(target),
		 target->ai.prev_pos ? (*target->ai.prev_pos)->x : -1,
                 target->ai.prev_pos ? (*target->ai.prev_pos)->y : -1,
                 TILE_XY(target->tile));
        continue;
      }

      /* Calculate juiciness of target, compare with existing target,
       * if any. */
      ai_hunter_juiciness(pplayer, punit, target, &stackthreat, &stackcost);
      stackcost *= unit_win_chance(punit, get_defender(punit, target->tile));
      if (stackcost < unit_build_shield_cost(punit)) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "%d is too expensive (it %d vs us %d)", 
                 target->id, stackcost,
		 unit_build_shield_cost(punit));
        continue; /* Too expensive */
      }
      stackthreat /= move_cost + 1;
      if (!is_virtual 
          && original_target != target
          && original_threat > stackthreat) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "Unit %d is not worse than %d", 
                 target->id, original_target->id);
        continue; /* The threat we found originally was worse than this! */
      }
      if (stackthreat < unit_build_shield_cost(punit)) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "%d is not worth it", target->id);
        continue; /* Not worth it */
      }

      UNIT_LOG(LOGLEVEL_HUNT, punit, "hunting %s %s[%d](%d,%d) "
               "with want %d, dist1 %d, dist2 %d", 
               nation_rule_name(nation_of_unit(target)),
               unit_rule_name(target), 
               target->id,
               TILE_XY(target->tile),
               stackthreat,
               dist1,
               dist2);
      /* Ok, now we FINALLY have a target worth destroying! */
      punit->ai.target = target->id;
      if (is_virtual) {
        pf_map_destroy(pfm);
        return stackthreat;
      }

      /* This assigns missiles to us */
      ai_unit_new_role(punit, AIUNIT_HUNTER, target->tile);

      /* Check if we can nuke it */
      ai_hunter_try_launch(pplayer, punit, target);

      /* Check if we have nuked it */
      if (target != game_find_unit_by_number(sanity_target)) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "mission accomplished by cargo (pre)");
        ai_unit_new_role(punit, AIUNIT_NONE, NULL);
        pf_map_destroy(pfm);
        return -1; /* try again */
      }

      /* Go towards it. */
      path = pf_map_get_path(pfm, target->tile);
      if (!ai_unit_execute_path(punit, path)) {
        pf_path_destroy(path);
        pf_map_destroy(pfm);
        return 0;
      }
      pf_path_destroy(path);

      if (target != game_find_unit_by_number(sanity_target)) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "mission accomplished");
        ai_unit_new_role(punit, AIUNIT_NONE, NULL);
        pf_map_destroy(pfm);
        return -1; /* try again */
      }

      /* Check if we can nuke it now */
      ai_hunter_try_launch(pplayer, punit, target);
      if (target != game_find_unit_by_number(sanity_target)) {
        UNIT_LOG(LOGLEVEL_HUNT, punit, "mission accomplished by cargo (post)");
        ai_unit_new_role(punit, AIUNIT_NONE, NULL);
        pf_map_destroy(pfm);
        return -1; /* try again */
      }

      pf_map_destroy(pfm);
      punit->ai.done = TRUE;
      return stackthreat; /* still have work to do */
    } unit_list_iterate_safe_end;
  } pf_map_iterate_move_costs_end;

  UNIT_LOG(LOGLEVEL_HUNT, punit, "ran out of map finding hunt target");
  pf_map_destroy(pfm);
  return 0; /* found nothing */
}
