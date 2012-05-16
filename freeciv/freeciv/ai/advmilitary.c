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
#include <string.h>

#include "combat.h"
#include "game.h"
#include "government.h"
#include "log.h"
#include "map.h"
#include "movement.h"
#include "unitlist.h"

#include "citytools.h"
#include "cityturn.h"
#include "gotohand.h"		/* warmap has been redeployed */
#include "settlers.h"

#include "aiair.h"
#include "aicity.h"
#include "aidata.h"
#include "aidiplomat.h"
#include "aiferry.h"
#include "aihand.h"
#include "aihunt.h"
#include "ailog.h"
#include "aiparatrooper.h"
#include "aitech.h"
#include "aitools.h"
#include "aiunit.h"

#include "advmilitary.h"

static unsigned int assess_danger(struct city *pcity);

/**************************************************************************
  Choose the best unit the city can build to defend against attacker v.
**************************************************************************/
struct unit_type *ai_choose_defender_versus(struct city *pcity,
					    struct unit *attacker)
{
  struct unit_type *bestunit = NULL;
  double best = 0;
  int best_cost = FC_INFINITY;
  struct player *pplayer = city_owner(pcity);

  simple_ai_unit_type_iterate(punittype) {
    const int move_type = utype_move_type(punittype);

    if (can_city_build_unit_now(pcity, punittype)
	&& (move_type == LAND_MOVING || move_type == SEA_MOVING)) {
      int fpatt, fpdef, defense, attack;
      double want, loss, cost = utype_build_shield_cost(punittype);
      struct unit *defender;
      int veteran = get_unittype_bonus(city_owner(pcity), pcity->tile, punittype,
                                       EFT_VETERAN_BUILD);

      defender = create_unit_virtual(pplayer, pcity, punittype, veteran);
      defense = get_total_defense_power(attacker, defender);
      attack = get_total_attack_power(attacker, defender);
      get_modified_firepower(attacker, defender, &fpatt, &fpdef);

      /* Greg's algorithm. loss is the average number of health lost by
       * defender. If loss > attacker's hp then we should win the fight,
       * which is always a good thing, since we avoid shield loss. */
      loss = (double) defense * punittype->hp * fpdef / (attack * fpatt);
      want = (loss + MAX(0, loss - attacker->hp)) / cost;

#ifdef NEVER
      CITY_LOG(LOG_DEBUG, pcity, "desire for %s against %s(%d,%d) is %.2f",
               unit_name_orig(punittype), unit_name_orig(unit_type(attacker)), 
               TILE_XY(attacker->tile), want);
#endif

      if (want > best || (want == best && cost <= best_cost)) {
        best = want;
        bestunit = punittype;
        best_cost = cost;
      }
      destroy_unit_virtual(defender);
    }
  } simple_ai_unit_type_iterate_end;

  return bestunit;
}

/********************************************************************** 
This function should assign a value to choice and want, where want is a value
between 1 and 100.

If choice is A_UNSET, this advisor doesn't want any particular tech
researched at the moment.
***********************************************************************/
void military_advisor_choose_tech(struct player *pplayer,
				  struct ai_choice *choice)
{
  /* This function hasn't been implemented yet. */
  init_choice(choice);
}

/**************************************************************************
  Choose best attacker based on movement type. It chooses based on unit
  desirability without regard to cost, unless costs are equal. This is
  very wrong. FIXME, use amortize on time to build.
**************************************************************************/
static struct unit_type *ai_choose_attacker(struct city *pcity,
					    enum unit_move_type which)
{
  struct unit_type *bestid = NULL;
  int best = -1;
  int cur;

  simple_ai_unit_type_iterate(punittype) {
    cur = ai_unit_attack_desirability(punittype);
    if (which == utype_move_type(punittype)) {
      if (can_city_build_unit_now(pcity, punittype)
          && (cur > best
              || (cur == best
                  && utype_build_shield_cost(punittype)
                     <= utype_build_shield_cost(bestid)))) {
        best = cur;
        bestid = punittype;
      }
    }
  } simple_ai_unit_type_iterate_end;

  return bestid;
}

/**************************************************************************
  Choose best defender based on movement type. It chooses based on unit
  desirability without regard to cost, unless costs are equal. This is
  very wrong. FIXME, use amortize on time to build.

  We should only be passed with L_DEFEND_GOOD role for now, since this
  is the only role being considered worthy of bodyguarding in findjob.
**************************************************************************/
static struct unit_type *ai_choose_bodyguard(struct city *pcity,
					     enum unit_move_type move_type,
					     enum unit_role_id role)
{
  struct unit_type *bestid = NULL;
  int best = 0;

  simple_ai_unit_type_iterate(punittype) {
    /* Only consider units of given role, or any if L_LAST */
    if (role != L_LAST) {
      if (!utype_has_role(punittype, role)) {
        continue;
      }
    }

    /* Only consider units of same move type */
    if (utype_move_type(punittype) != move_type) {
      continue;
    }

    /* Now find best */
    if (can_city_build_unit_now(pcity, punittype)) {
      const int desire = ai_unit_defence_desirability(punittype);

      if (desire > best
	  || (desire == best && utype_build_shield_cost(punittype) <=
	      utype_build_shield_cost(bestid))) {
        best = desire;
        bestid = punittype;
      }
    }
  } simple_ai_unit_type_iterate_end;

  return bestid;
}

/********************************************************************** 
Helper for assess_defense_quadratic and assess_defense_unit.
***********************************************************************/
static int base_assess_defense_unit(struct city *pcity, struct unit *punit,
                                    bool igwall, bool quadratic,
                                    int wall_value)
{
  int defense;
  bool do_wall = FALSE;

  if (!is_military_unit(punit)) {
    return 0;
  }

  defense = get_defense_power(punit) * punit->hp;
  if (!is_sailing_unit(punit)) {
    defense *= unit_type(punit)->firepower;
    if (is_ground_unit(punit)) {
      if (pcity) {
        /* FIXME: We check if city got defense effect against *some*
         * unit type. Sea unit danger might cause us to build defenses
         * against air units... */
        do_wall = (!igwall && city_got_defense_effect(pcity, NULL));
        defense *= 3;
      }
    }
  }
  defense /= POWER_DIVIDER;

  if (quadratic) {
    defense *= defense;
  }

  if (do_wall) {
    defense *= wall_value;
    defense /= 10;
  }

  return defense;
}

/********************************************************************** 
Need positive feedback in m_a_c_b and bodyguard routines. -- Syela
***********************************************************************/
int assess_defense_quadratic(struct city *pcity)
{
  int defense = 0, walls = 0;
  /* This can be an arg if needed, but we don't need to change it now. */
  const bool igwall = FALSE;

  /* wallvalue = 10, walls = 10,
   * wallvalue = 40, walls = 20,
   * wallvalue = 90, walls = 30 */

  while (walls * walls < pcity->ai->wallvalue * 10) {
    walls++;
  }

  unit_list_iterate(pcity->tile->units, punit) {
    defense += base_assess_defense_unit(pcity, punit, igwall, FALSE,
                                        walls);
  } unit_list_iterate_end;

  if (defense > 1<<12) {
    CITY_LOG(LOG_VERBOSE, pcity, "Overflow danger in assess_defense_quadratic:"
             " %d", defense);
    if (defense > 1<<15) {
      defense = 1<<15; /* more defense than we know what to do with! */
    }
  }

  return defense * defense;
}

/**************************************************************************
One unit only, mostly for findjob; handling boats correctly. 980803 -- Syela
**************************************************************************/
int assess_defense_unit(struct city *pcity, struct unit *punit, bool igwall)
{
  return base_assess_defense_unit(pcity, punit, igwall, TRUE,
				  pcity->ai->wallvalue);
}

/********************************************************************** 
Most of the time we don't need/want positive feedback. -- Syela

It's unclear whether this should treat settlers/caravans as defense. -- Syela
TODO: It looks like this is never used while deciding if we should attack
pcity, if we have pcity defended properly, so I think it should. --pasky
***********************************************************************/
static int assess_defense_backend(struct city *pcity, bool igwall)
{
  /* Estimate of our total city defensive might */
  int defense = 0;

  unit_list_iterate(pcity->tile->units, punit) {
    defense += assess_defense_unit(pcity, punit, igwall);
  } unit_list_iterate_end;

  return defense;
}

/************************************************************************** 
...
**************************************************************************/
int assess_defense(struct city *pcity)
{
  return assess_defense_backend(pcity, FALSE);
}

/************************************************************************** 
...
**************************************************************************/
static int assess_defense_igwall(struct city *pcity)
{
  return assess_defense_backend(pcity, TRUE);
}

/************************************************************************** 
  Compute actual danger depending on move rate of enemy and its distance.
**************************************************************************/
static unsigned int dangerfunct(int danger, int move_rate, int distance)
{
  /* XXX: I don't have a clue about these, it probably has something in common
   * with the way how attack force is computed when attacker already spent some
   * move points..? --pasky */
  unsigned int num = move_rate * 4;
  unsigned int denom = move_rate * 4;

  if (move_rate == 0) {
    return danger;
  }

  danger *= 2;

  /* Turns to reach us.. */
  while (distance >= move_rate) {
    danger /= 2;
    distance -= move_rate;
  }

  /* Moves in the last turn to reach us.. */
  move_rate /= SINGLE_MOVE;
  while (distance > 0 && distance >= move_rate) {
    num *= 4;
    denom *= 5;
    distance -= move_rate;
  }

  /* Partial moves in the last turn.. */
  while (distance > 0) {
    denom += (denom + move_rate * 2) / (move_rate * 4);
    distance--;
  }

  danger = (danger * num + (denom/2)) / denom;

  return danger;
}

/************************************************************************** 
How dangerous a unit is for a city?
**************************************************************************/
static unsigned int assess_danger_unit(struct city *pcity, struct unit *punit)
{
  unsigned int danger;
  bool sailing;
  int mod;

  if (unit_has_type_flag(punit, F_NO_LAND_ATTACK)) return 0;

  sailing = is_sailing_unit(punit);
  if (sailing && !is_ocean_near_tile(pcity->tile)) {
    return 0;
  }

  danger = unit_att_rating(punit);
  mod = 100 + get_unittype_bonus(city_owner(pcity), pcity->tile,
				 unit_type(punit), EFT_DEFEND_BONUS);
  danger = danger * 100 / MAX(mod, 1);

  return danger;
}

/************************************************************************** 
  Assess distance between punit and pcity.
**************************************************************************/
static int assess_distance(struct city *pcity, struct unit *punit,
                           int move_rate)
{
  int distance = 0;
  struct unit *ferry = game_find_unit_by_number(punit->transported_by);

  if (same_pos(punit->tile, pcity->tile)) {
    return 0;
  }

  if (is_tiles_adjacent(punit->tile, pcity->tile)) {
    distance = SINGLE_MOVE;
  } else if (is_sailing_unit(punit)) {
    distance = WARMAP_SEACOST(punit->tile);
  } else if (!is_ground_unit(punit)) {
    distance = real_map_distance(punit->tile, pcity->tile)
               * SINGLE_MOVE;
  } else if (is_ground_unit(punit) && ferry) {
    distance = WARMAP_SEACOST(ferry->tile); /* Sea travellers. */
  } else if (unit_has_type_flag(punit, F_IGTER)) {
    distance = real_map_distance(punit->tile, pcity->tile);
  } else {
    distance = WARMAP_COST(punit->tile);
  }

  /* If distance = 9, a chariot is 1.5 turns away.  NOT 2 turns away. */
  if (distance < SINGLE_MOVE) {
    distance = SINGLE_MOVE;
  }

  return distance;
}

/********************************************************************** 
Call assess_danger() for all cities owned by pplayer.

This is necessary to initialize some ai data before some ai calculations.
***********************************************************************/
void assess_danger_player(struct player *pplayer)
{
  city_list_iterate(pplayer->cities, pcity) {
    assess_danger(pcity);
  } city_list_iterate_end;
}

/********************************************************************** 
  Set (overwrite) our want for a building. Syela tries to explain:
   
    My first attempt to allow ng_wa >= 200 led to stupidity in cities 
    with no defenders and danger = 0 but danger > 0.  Capping ng_wa at 
    100 + urgency led to a failure to buy walls as required.  Allowing 
    want > 100 with !urgency led to the AI spending too much gold and 
    falling behind on science.  I'm trying again, but this will require 
    yet more tedious observation -- Syela
   
  The idea in this horrible function is that there is an enemy nearby
  that can whack us, so let's build something that can defend against
  him. If danger is urgent and overwhelming, danger is 200+, if it is
  only overwhelming, set it depending on danger. If it is underwhelming,
  set it to 100 plus urgency.

  This algorithm is very strange. But I created it by nesting up
  Syela's convoluted if ... else logic, and it seems to work. -- Per
***********************************************************************/
static void ai_reevaluate_building(struct city *pcity, int *value, 
                                   unsigned int urgency, unsigned int danger, 
                                   int defense)
{
  if (*value == 0 || danger <= 0) {
    return;
  }

  *value = MAX(*value, 100 + MAX(0, urgency)); /* default */

  if (urgency > 0 && danger > defense * 2) {
    *value += 100;
  } else if (defense != 0 && danger > defense) {
    *value = MAX(danger * 100 / defense, *value);
  }
}

/********************************************************************** 
  Create cached information about danger, urgency and grave danger to
  our cities. 

  Danger is a weight on how much power enemy units nearby have, which
  is compared to our defence.

  Urgency is the number of hostile units that can attack us in three
  turns.

  Grave danger is number of units that can attack us next turn.

  Generates a warmap around pcity.

  FIXME: We do not consider a paratrooper's mr_req and mr_sub
  fields. Not a big deal, though.

  FIXME: Due to the nature of assess_distance, a city will only be 
  afraid of a boat laden with enemies if it stands on the coast (i.e.
  is directly reachable by this boat).
***********************************************************************/
static unsigned int assess_danger(struct city *pcity)
{
  int i;
  int defender;
  struct player *pplayer = city_owner(pcity);
  bool pikemen = FALSE;
  unsigned int urgency = 0;
  int igwall_threat = 0;
  struct tile *ptile = pcity->tile;
  int defense;
  unsigned int danger_reduced[B_LAST]; /* How much such danger there is that
                                          building would help against */
  int total_danger = 0;

  TIMING_LOG(AIT_DANGER, TIMER_START);

  memset(&danger_reduced, 0, sizeof(danger_reduced));

  generate_warmap(pcity, NULL);	/* generates both land and sea maps */

  if (ai_handicap(pplayer, H_DANGER)) {
    /* Always thinks that city is in grave danger */
    pcity->ai->grave_danger = 1;
  } else {
    pcity->ai->grave_danger = 0;
  }
  pcity->ai->diplomat_threat = FALSE;
  pcity->ai->has_diplomat = FALSE;

  unit_list_iterate(ptile->units, punit) {
    if (unit_has_type_flag(punit, F_DIPLOMAT)) pcity->ai->has_diplomat = TRUE;
    if (unit_has_type_flag(punit, F_PIKEMEN)) pikemen = TRUE;
  } unit_list_iterate_end;

  players_iterate(aplayer) {
    if (!is_player_dangerous(city_owner(pcity), aplayer)) {
      continue;
    }
    /* Note that we still consider the units of players we are not (yet)
     * at war with. */

    /* Look for enemy units */
    unit_list_iterate(aplayer->units, punit) {
      int paramove = 0;
      int move_rate = unit_move_rate(punit);
      unsigned int vulnerability = assess_danger_unit(pcity, punit);
      int dist = assess_distance(pcity, punit, move_rate);
      /* Although enemy units will not be in our cities,
       * we might still consider allies to be dangerous,
       * so dist can be 0. */

      if (unit_has_type_flag(punit, F_PARATROOPERS)) {
        paramove = unit_type(punit)->paratroopers_range;
      }

      if (is_ground_unit(punit) && vulnerability != 0) {
        if (dist <= move_rate * 3 || dist <= paramove + move_rate) {
          urgency++;
        }
        if (dist <= move_rate || dist <= paramove + move_rate) {
          pcity->ai->grave_danger++;
        }
      } else {
        unit_class_iterate(punitclass) {
          if (punitclass->move_type == LAND_MOVING
              && can_unit_type_transport(unit_type(punit), punitclass)) {
            /* It can transport some land moving units! */

            if (dist <= move_rate * 3 || dist <= paramove + move_rate) {
              urgency++;
            }
            if (dist <= move_rate || dist <= paramove + move_rate) {
              pcity->ai->grave_danger++;
            }
            break;
          }
        } unit_class_iterate_end;
      }
      if (paramove > 0 && can_unit_paradrop(punit)) {
        move_rate += paramove; /* gross simplification */
      }

      if (unit_has_type_flag(punit, F_HORSE)) {
	if (pikemen) {
	  vulnerability /= 2;
	} else {
	  (void) ai_wants_role_unit(pplayer, pcity, F_PIKEMEN,
				    vulnerability * move_rate /
				    MAX(dist * 2, 1));
	}
      }

      if (unit_has_type_flag(punit, F_DIPLOMAT) && (dist <= 2 * move_rate)) {
	pcity->ai->diplomat_threat = TRUE;
      }

      vulnerability *= vulnerability; /* positive feedback */

      if (unit_has_type_flag(punit, F_NUCLEAR)) {
        defender = ai_find_source_building(pcity, EFT_NUKE_PROOF,
                                           unit_class(punit), MOVETYPE_LAST);
        if (defender != B_LAST) {
          danger_reduced[defender] += vulnerability * move_rate / MAX(dist, 1);
        }
      } else if (!unit_has_type_flag(punit, F_IGWALL)) {
        defender = ai_find_source_building(pcity, EFT_DEFEND_BONUS,
                                           unit_class(punit), MOVETYPE_LAST);
        if (defender != B_LAST) {
          danger_reduced[defender] += vulnerability * move_rate / MAX(dist, 1);
        }
      } else {
        igwall_threat += vulnerability;
      }

      vulnerability = dangerfunct(vulnerability, move_rate, dist);
      total_danger += vulnerability;

    } unit_list_iterate_end;
  } players_iterate_end;

  if (igwall_threat == 0) {
    pcity->ai->wallvalue = 90;
  } else if (total_danger) {
    pcity->ai->wallvalue = ((total_danger * 9 - igwall_threat * 8) 
                           * 10 / total_danger);
  } else {
    /* No danger.
     * This is half of the wallvalue of what danger 1 would produce. */
    pcity->ai->wallvalue = 5;
  }

  if (pcity->ai->grave_danger != 0) {
    /* really, REALLY urgent to defend */
    urgency += 10;
  }

  /* HACK: This needs changing if multiple improvements provide
   * this effect. */
  /* FIXME: Accept only buildings helping unit classes we actually use.
   *        Now we consider any land mover helper suitable. */
  defense = assess_defense_igwall(pcity);

  for (i = 0; i < B_LAST; i++) {
    if (danger_reduced[i] > 0) {
      ai_reevaluate_building(pcity, &pcity->ai->building_want[i],
                             urgency, danger_reduced[i], defense);
    }
  }

  if (ai_handicap(pplayer, H_DANGER)
      && total_danger == 0) {
    /* Has to have some danger
     * Otherwise grave_danger will be ignored. */
    pcity->ai->danger = 1;
  } else {
    pcity->ai->danger = total_danger;
  }
  pcity->ai->urgency = urgency;

  TIMING_LOG(AIT_DANGER, TIMER_STOP);

  return urgency;
}

/************************************************************************** 
  How much we would want that unit to defend a city? (Do not use this 
  function to find bodyguards for ships or air units.)
**************************************************************************/
int ai_unit_defence_desirability(const struct unit_type *punittype)
{
  int desire = punittype->hp;
  int attack = punittype->attack_strength;
  int defense = punittype->defense_strength;

  /* Sea and helicopters often have their firepower set to 1 when
   * defending. We can't have such units as defenders. */
  if (!utype_has_flag(punittype, F_BADCITYDEFENDER)
      && !utype_has_flag(punittype, F_HELICOPTER)) {
    /* Sea units get 1 firepower in Pearl Harbour,
     * and helicopters very bad against fighters */
    desire *= punittype->firepower;
  }
  desire *= defense;
  desire += punittype->move_rate / SINGLE_MOVE;
  desire += attack;
  if (utype_has_flag(punittype, F_PIKEMEN)) {
    desire += desire / 2;
  }
  if (utype_has_flag(punittype, F_GAMELOSS)) {
    desire /= 10; /* but might actually be worth it */
  }
  return desire;
}

/************************************************************************** 
  How much we would want that unit to attack with?
**************************************************************************/
int ai_unit_attack_desirability(const struct unit_type *punittype)
{
  int desire = punittype->hp;
  int attack = punittype->attack_strength;
  int defense = punittype->defense_strength;

  desire *= punittype->move_rate;
  desire *= punittype->firepower;
  desire *= attack;
  desire += defense;
  if (utype_has_flag(punittype, F_IGTER)) {
    desire += desire / 2;
  }
  if (utype_has_flag(punittype, F_GAMELOSS)) {
    desire /= 10; /* but might actually be worth it */
  }
  if (utype_has_flag(punittype, F_CITYBUSTER)) {
    desire += desire / 2;
  }
  if (utype_has_flag(punittype, F_MARINES)) {
    desire += desire / 4;
  }
  if (utype_has_flag(punittype, F_IGWALL)) {
    desire += desire / 4;
  }
  return desire;
}

/************************************************************************** 
  What would be the best defender for that city? Records the best defender 
  type in choice. Also sets the technology want for the units we can't 
  build yet.
**************************************************************************/
static bool process_defender_want(struct player *pplayer, struct city *pcity,
                                  unsigned int danger, struct ai_choice *choice)
{
  /* FIXME: We check if city got defense effect against *some*
   * unit type. Sea unit danger might cause us to build defenses
   * against air units... */
  bool walls = city_got_defense_effect(pcity, NULL);
  /* Technologies we would like to have. */
  int tech_desire[U_LAST];
  /* Our favourite unit. */
  int best = -1;
  struct unit_type *best_unit_type = NULL;
  int best_unit_cost = 1;

  memset(tech_desire, 0, sizeof(tech_desire));

  simple_ai_unit_type_iterate(punittype) {
    int desire; /* How much we want the unit? */
    int move_type = utype_move_type(punittype);

    /* Only consider proper defenders - otherwise waste CPU and
     * bump tech want needlessly. */
    if (!utype_has_role(punittype, L_DEFEND_GOOD)
	&& !utype_has_role(punittype, L_DEFEND_OK)) {
      continue;
    }

    desire = ai_unit_defence_desirability(punittype);

    if (!utype_has_role(punittype, L_DEFEND_OK)) {
      desire /= 2; /* not good, just ok */
    }

    if (utype_has_flag(punittype, F_FIELDUNIT)) {
      /* Causes unhappiness even when in defense, so not a good
       * idea for a defender, unless it is _really_ good */
      desire /= 2;
    }      

    desire /= POWER_DIVIDER/2; /* Good enough, no rounding errors. */
    desire *= desire;

    if (can_city_build_unit_now(pcity, punittype)) {
      /* We can build the unit now... */

      int build_cost = utype_build_shield_cost(punittype);
      int limit_cost = pcity->shield_stock + 40;

      if (walls && move_type == LAND_MOVING) {
	desire *= pcity->ai->wallvalue;
	/* TODO: More use of POWER_FACTOR ! */
	desire /= POWER_FACTOR;
      }


      if ((best_unit_cost > limit_cost
           && build_cost < best_unit_cost)
          || ((desire > best ||
               (desire == best && build_cost <= best_unit_cost))
              && (best_unit_type == NULL
                  /* In case all units are more expensive than limit_cost */
                  || limit_cost <= pcity->shield_stock + 40))) {
	best = desire;
	best_unit_type = punittype;
	best_unit_cost = build_cost;
      }
    } else if (can_city_build_unit_later(pcity, punittype)) {
      /* We first need to develop the tech required by the unit... */

      /* Cost (shield equivalent) of gaining these techs. */
      /* FIXME? Katvrr advises that this should be weighted more heavily in
       * big danger. */
      int tech_cost = total_bulbs_required_for_goal(pplayer,
			advance_number(punittype->require_advance)) / 4
		      / city_list_size(pplayer->cities);
        
      /* Contrary to the above, we don't care if walls are actually built 
       * - we're looking into the future now. */
      if (move_type == LAND_MOVING) {
	desire *= pcity->ai->wallvalue;
	desire /= POWER_FACTOR;
      }

      /* Yes, there's some similarity with kill_desire(). */
      /* TODO: Explain what shield cost has to do with tech want. */
      tech_desire[utype_index(punittype)] =
        (desire * danger / (utype_build_shield_cost(punittype) + tech_cost));
    }
  } simple_ai_unit_type_iterate_end;

  if (best == -1) {
    CITY_LOG(LOG_DEBUG, pcity, "Ooops - we cannot build any defender!");
  }

  if (best_unit_type) {
    if (!walls && utype_move_type(best_unit_type) == LAND_MOVING) {
      best *= pcity->ai->wallvalue;
      best /= POWER_FACTOR;
    }
  } else {
    best_unit_cost = 100; /* Building impossible is considered costly.
                           * This should increase want for tech providing
                           * first defender type. */
  }

  if (best <= 0) best = 1; /* Avoid division by zero below. */

  /* Update tech_want for appropriate techs for units we want to build. */
  simple_ai_unit_type_iterate(punittype) {
    if (tech_desire[utype_index(punittype)] > 0) {
      /* TODO: Document or fix the algorithm below. I have no idea why
       * it is written this way, and the results seem strange to me. - Per */
      int desire = tech_desire[utype_index(punittype)] * best_unit_cost / best;

      pplayer->ai_data.tech_want[advance_index(punittype->require_advance)]
        += desire;
      TECH_LOG(LOG_DEBUG, pplayer, punittype->require_advance,
               "+ %d for %s to defend %s",
               desire,
               utype_rule_name(punittype),
               city_name(pcity));
    }
  } simple_ai_unit_type_iterate_end;

  if (!best_unit_type) {
    return FALSE;
  }

  choice->value.utype = best_unit_type;
  choice->want = danger;
  choice->type = CT_DEFENDER;
  return TRUE;
}

/************************************************************************** 
  This function decides, what unit would be best for erasing enemy. It is 
  called, when we just want to kill something, we've found it but we don't 
  have the unit for killing that built yet - here we'll choose the type of 
  that unit.

  We will also set increase the technology want to get units which could 
  perform the job better.

  I decided this funct wasn't confusing enough, so I made 
  kill_something_with send it some more variables for it to meddle with. 
  -- Syela

  (x,y) is location of the target.
  best_choice is pre-filled with our current choice, we only 
  consider units of the same move_type as best_choice
**************************************************************************/
static void process_attacker_want(struct city *pcity,
                                  int value,
				  struct unit_type *victim_unit_type,
				  struct player *victim_player,
                                  int veteran, struct tile *ptile,
                                  struct ai_choice *best_choice,
                                  struct unit *boat,
				  struct unit_type *boattype)
{
  struct player *pplayer = city_owner(pcity);
  /* The enemy city.  acity == NULL means stray enemy unit */
  struct city *acity = tile_city(ptile);
  bool shore = is_ocean_near_tile(pcity->tile);
  int orig_move_type = utype_move_type(best_choice->value.utype);
  int victim_count = 1;
  int needferry = 0;
  bool unhap = ai_assess_military_unhappiness(pcity);

  assert(orig_move_type == SEA_MOVING || orig_move_type == LAND_MOVING);

  if (orig_move_type == LAND_MOVING && !boat && boattype) {
    /* cost of ferry */
    needferry = utype_build_shield_cost(boattype);
  }
  
  if (!is_stack_vulnerable(ptile)) {
    /* If it is a city, a fortress or an air base,
     * we may have to whack it many times */
    victim_count += unit_list_size(ptile->units);
  }

  simple_ai_unit_type_iterate(punittype) {
    Tech_type_id tech_req = advance_number(punittype->require_advance);
    int tech_dist = num_unknown_techs_for_goal(pplayer, tech_req);
    int move_type = utype_move_type(punittype);

    if ((move_type == LAND_MOVING || (move_type == SEA_MOVING && shore))
        && (tech_dist > 0 
            || U_NOT_OBSOLETED == punittype->obsoleted_by
            || !can_city_build_unit_direct(pcity, 
                                      punittype->obsoleted_by))
        && punittype->attack_strength > 0 /* or we'll get SIGFPE */
        && move_type == orig_move_type) {
      /* Values to be computed */
      int desire, want;
      int move_time;
      int vuln;

      int will_be_veteran
	= (ai_find_source_building(pcity, EFT_VETERAN_BUILD,
                                   utype_class(punittype), MOVETYPE_LAST) != B_LAST);
      /* Cost (shield equivalent) of gaining these techs. */
      /* FIXME? Katvrr advises that this should be weighted more heavily in big
       * danger. */
      int tech_cost = total_bulbs_required_for_goal(pplayer, tech_req) / 4
                      / city_list_size(pplayer->cities);
      int move_rate = punittype->move_rate;
      int bcost_balanced = build_cost_balanced(punittype);
      /* See description of kill_desire() for info about this variables. */
      int bcost = utype_build_shield_cost(punittype);
      int attack = unittype_att_rating(punittype, will_be_veteran,
                                       SINGLE_MOVE,
                                       punittype->hp);
      
      /* Take into account reinforcements strength */
      if (acity) {
        attack += acity->ai->attack;
      }

      if (attack == 0) {
       /* Yes, it can happen that a military unit has attack=1,
        * for example militia with HP=1 (in civ1 ruleset). */ 
	continue;
      }
      
      attack *= attack;

      if (utype_has_flag(punittype, F_IGTER)) {
        /* TODO: Use something like IGTER_MOVE_COST. -- Raahul */
        move_rate *= SINGLE_MOVE;
      }

      /* Set the move_time appropriatelly. */
      if (acity) {
        move_time = turns_to_enemy_city(punittype, acity, move_rate,
                                        (boattype != NULL), boat, boattype);
      } else {
        /* Target is in the field */
        move_time = turns_to_enemy_unit(punittype, move_rate, ptile,
                                        victim_unit_type);
      }

      /* Estimate strength of the enemy. */

      if (victim_unit_type) {
        vuln = unittype_def_rating_sq(punittype, victim_unit_type,
                                      victim_player,
                                      ptile, FALSE, veteran);
      } else {
        vuln = 0;
      }

      /* Not bothering to s/!vuln/!pdef/ here for the time being. -- Syela
       * (this is noted elsewhere as terrible bug making warships yoyoing) 
       * as the warships will go to enemy cities hoping that the enemy builds 
       * something for them to kill*/
      if (move_type != LAND_MOVING && vuln == 0) {
        desire = 0;
        
      } else if (uclass_has_flag(utype_class(punittype), UCF_CAN_OCCUPY_CITY)
                 && acity
                 && acity->ai->invasion.attack > 0
                 && acity->ai->invasion.occupy == 0) {
        desire = bcost * SHIELD_WEIGHTING;

      } else {
        if (!acity) {
          desire = kill_desire(value, attack, bcost, vuln, victim_count);
        } else {
          int city_attack = acity->ai->attack * acity->ai->attack;

          /* See aiunit.c:find_something_to_kill() for comments. */
          
          desire = kill_desire(value, attack, (bcost + acity->ai->bcost), vuln,
                               victim_count);
          
          if (value * city_attack > acity->ai->bcost * vuln) {
            desire -= kill_desire(value, city_attack, acity->ai->bcost, vuln,
                                  victim_count);
          }
        }
      }
      
      desire -= tech_cost * SHIELD_WEIGHTING;
      /* We can be possibly making some people of our homecity unhappy - then
       * we don't want to travel too far away to our victims. */
      /* TODO: Unify the 'unhap' dependency to some common function. */
      desire -= move_time * (unhap ? SHIELD_WEIGHTING + 2 * TRADE_WEIGHTING
                             : SHIELD_WEIGHTING);

      want = military_amortize(pplayer, pcity, desire, MAX(1, move_time),
                               bcost_balanced + needferry);
      
      if (want > 0) {
        if (tech_dist > 0) {
          /* This is a future unit, tell the scientist how much we need it */
          pplayer->ai_data.tech_want[advance_index(punittype->require_advance)]
            += want;
          TECH_LOG(LOG_DEBUG, pplayer, punittype->require_advance,
                   "+ %d for %s vs %s(%d,%d)",
                   want,
                   utype_rule_name(punittype),
                   (acity ? city_name(acity) : utype_rule_name(victim_unit_type)),
                   TILE_XY(ptile));
        } else if (want > best_choice->want) {
          struct impr_type *impr_req = punittype->need_improvement;

          if (can_city_build_unit_now(pcity, punittype)) {
            /* This is a real unit and we really want it */

            CITY_LOG(LOG_DEBUG, pcity, "overriding %s(%d) with %s(%d)"
                     " [attack=%d,value=%d,move_time=%d,vuln=%d,bcost=%d]",
                     utype_rule_name(best_choice->value.utype),
		     best_choice->want,
                     utype_rule_name(punittype),
                     want,
                     attack, value, move_time, vuln, bcost);

            best_choice->value.utype = punittype;
            best_choice->want = want;
            best_choice->type = CT_ATTACKER;
          } else if (NULL == impr_req) {
            CITY_LOG(LOG_DEBUG, pcity, "cannot build unit %s",
                     utype_rule_name(punittype));
          } else if (can_city_build_improvement_now(pcity, impr_req)) {
	    /* Building this unit requires a specific type of improvement.
	     * So we build this improvement instead.  This may not be the
	     * best behavior. */
            CITY_LOG(LOG_DEBUG, pcity, "building %s to build unit %s",
                     improvement_rule_name(impr_req),
                     utype_rule_name(punittype));
            best_choice->value.building = impr_req;
            best_choice->want = want;
            best_choice->type = CT_BUILDING;
          } else {
	    /* This should never happen? */
            CITY_LOG(LOG_DEBUG, pcity, "cannot build %s or unit %s",
                     improvement_rule_name(impr_req),
                     utype_rule_name(punittype));
	  }
        }
      }
    }
  } simple_ai_unit_type_iterate_end;
}

/************************************************************************** 
This function 
1. receives (in myunit) a first estimate of what we would like to build.
2. finds a potential victim for it.
3. calculates the relevant stats of the victim.
4. finds the best attacker for this type of victim (in process_attacker_want)
5. if we still want to attack, records the best attacker in choice.
If the target is overseas, the function might suggest building a ferry
to carry a land attack unit, instead of the land attack unit itself.
**************************************************************************/
static void kill_something_with(struct player *pplayer, struct city *pcity, 
				struct unit *myunit, struct ai_choice *choice)
{
  struct ai_data *ai = ai_data_get(pplayer);
  /* Our attack rating (with reinforcements) */
  int attack;
  /* Benefit from fighting the target */
  int benefit;
  /* Defender of the target city/tile */
  struct unit *pdef; 
  struct unit_type *def_type;
  struct player *def_owner;
  int def_vet; /* Is the defender veteran? */
  /* Target coordinates */
  struct tile *ptile;
  /* Our transport */
  struct unit *ferryboat = NULL;
  /* Our target */
  struct city *acity;
  /* Type of the boat (real or a future one) */
  struct unit_type *boattype = NULL;
  bool go_by_boat;
  struct ai_choice best_choice;

  init_choice(&best_choice);
  best_choice.value.utype = unit_type(myunit);
  best_choice.type = CT_ATTACKER;
  best_choice.want = choice->want;

  assert(is_military_unit(myunit) && !utype_fuel(unit_type(myunit)));

  if (pcity->ai->danger != 0 && assess_defense(pcity) == 0) {
    /* Defence comes first! */
    return;
  }

  if (!is_ground_unit(myunit) && !is_sailing_unit(myunit)) {
    freelog(LOG_ERROR, "kill_something_with()"
            " attempting to deal with non-trivial unit_type");
    return;
  }

  best_choice.want = find_something_to_kill(pplayer, myunit, &ptile);

  acity = tile_city(ptile);

  if (myunit->id != 0) {
    freelog(LOG_ERROR, "kill_something_with()"
            " non-virtual unit!");
    return;
  }
  
  attack = unit_att_rating(myunit);
  if (acity) {
    attack += acity->ai->attack;
  }
  attack *= attack;
  
  if (acity) {
    /* Our move rate */
    int move_rate = unit_type(myunit)->move_rate;
    /* Distance to target (in turns) */
    int move_time;
    /* Rating of enemy defender */
    int vuln;

    if (unit_has_type_flag(myunit, F_IGTER)) {
      /* See comment in unit_move_turns */
      move_rate *= 3;
    }
    
    if (!HOSTILE_PLAYER(pplayer, ai, city_owner(acity))) {
      /* Not a valid target */
      return;
    }

    if (is_ground_unit(myunit)) {
      int boatid = aiferry_find_boat(myunit, 1, NULL);
      ferryboat = game_find_unit_by_number(boatid);

      if (ferryboat) {
        boattype = unit_type(ferryboat);
      } else {
        boattype = best_role_unit_for_player(pplayer, L_FERRYBOAT);
        if (!boattype) {
          /* We pretend that we can have the simplest boat --
	   * to stimulate tech */
          boattype = get_role_unit(L_FERRYBOAT, 0);
        }
      }
      assert(LAND_MOVING != utype_move_type(boattype));

      go_by_boat = !(WARMAP_COST(ptile) <= (MIN(6, move_rate) * THRESHOLD)
                     && goto_is_sane(myunit, acity->tile, TRUE));
    } else {
      /* else never needs a boat */
      go_by_boat = FALSE;
    }

    move_time = turns_to_enemy_city(unit_type(myunit), acity, move_rate, 
                                    go_by_boat, ferryboat, boattype);

    def_type = ai_choose_defender_versus(acity, myunit);
    def_owner = city_owner(acity);
    if (move_time > 1 && def_type) {
      def_vet = do_make_unit_veteran(acity, def_type);
      vuln = unittype_def_rating_sq(unit_type(myunit), def_type, city_owner(acity),
                                    ptile, FALSE, def_vet);
      benefit = utype_build_shield_cost(def_type);
    } else {
      vuln = 0;
      benefit = 0;
      def_vet = 0;
    }

    pdef = get_defender(myunit, ptile);
    if (pdef) {
      int m = unittype_def_rating_sq(unit_type(myunit), unit_type(pdef), city_owner(acity),
                                     ptile, FALSE, pdef->veteran);
      if (vuln < m) {
        vuln = m;
        benefit = unit_build_shield_cost(pdef);
        def_vet = pdef->veteran;
        def_type = unit_type(pdef); 
	def_owner = unit_owner(pdef);
      }
    }
    if (COULD_OCCUPY(myunit) || acity->ai->invasion.occupy > 0) {
      /* bonus for getting the city */
      benefit += 40;
    }

    /* end dealing with cities */
  } else {

    pdef = get_defender(myunit, ptile);
    if (!pdef) {
      /* Nobody to attack! */
      return;
    }

    benefit = unit_build_shield_cost(pdef);
    go_by_boat = FALSE;

    def_type = unit_type(pdef);
    def_vet = pdef->veteran;
    def_owner = unit_owner(pdef);
    /* end dealing with units */
  }
  
  if (!go_by_boat) {
    process_attacker_want(pcity, benefit, def_type, def_owner,
			  def_vet, ptile, 
                          &best_choice, NULL, NULL);
  } else { 
    /* Attract a boat to our city or retain the one that's already here */
    assert(is_ground_unit(myunit));
    best_choice.need_boat = TRUE;
    process_attacker_want(pcity, benefit, def_type, def_owner,
			  def_vet, ptile, 
                          &best_choice, ferryboat, boattype);
  }

  if (best_choice.want > choice->want) {
    /* We want attacker more than what we have selected before */
    copy_if_better_choice(&best_choice, choice);
    CITY_LOG(LOG_DEBUG, pcity, "kill_something_with()"
	     " %s has chosen attacker, %s, want=%d",
	     city_name(pcity),
	     utype_rule_name(best_choice.value.utype),
	     best_choice.want);

    if (go_by_boat && !ferryboat) { /* need a new ferry */
      /* We might need a new boat even if there are boats free,
       * if they are blockaded or in inland seas*/
      assert(is_ground_unit(myunit));
      ai_choose_role_unit(pplayer, pcity, choice, CT_ATTACKER,
                          L_FERRYBOAT, choice->want, TRUE);
      if (SEA_MOVING == utype_move_type(choice->value.utype)) {
        struct ai_data *ai = ai_data_get(pplayer);

        freelog(LOG_DEBUG, "kill_something_with()"
                " %s has chosen attacker ferry, %s, want=%d, %d of %d free",
                city_name(pcity),
                utype_rule_name(choice->value.utype),
		choice->want,
                ai->stats.available_boats, ai->stats.boats);
      } /* else can not build ferries yet */
    }
  } 
}

/**********************************************************************
... this function should assign a value to choice and want and type, 
    where want is a value between 1 and 100.
    if want is 0 this advisor doesn't want anything
***********************************************************************/
static void ai_unit_consider_bodyguard(struct city *pcity,
                                       struct unit_type *punittype,
                                       struct ai_choice *choice)
{
  struct unit *virtualunit;
  struct player *pplayer = city_owner(pcity);
  struct unit *aunit = NULL;
  struct city *acity = NULL;

  virtualunit = create_unit_virtual(pplayer, pcity, punittype,
                                    do_make_unit_veteran(pcity, punittype));

  if (choice->want < 100) {
    const int want = look_for_charge(pplayer, virtualunit, &aunit, &acity);

    if (want > choice->want) {
      choice->want = want;
      choice->value.utype = punittype;
      choice->type = CT_DEFENDER;
    }
  }
  destroy_unit_virtual(virtualunit);
}

/*********************************************************************
  Before building a military unit, AI builds a barracks/port/airport
  NB: It is assumed this function isn't called in an emergency 
  situation, when we need a defender _now_.
 
  TODO: something more sophisticated, like estimating future demand
  for military units, considering Sun Tzu instead.
*********************************************************************/
static void adjust_ai_unit_choice(struct city *pcity, 
                                  struct ai_choice *choice)
{
  enum unit_move_type move_type;
  Impr_type_id id;

  /* Sanity */
  if (!is_unit_choice_type(choice->type)
      || utype_has_flag(choice->value.utype, F_CIVILIAN)
      || do_make_unit_veteran(pcity, choice->value.utype)) {
    return;
  }

  move_type = utype_move_type(choice->value.utype);

  /*  N.B.: have to check that we haven't already built the building --mck */
  if ((id = ai_find_source_building(pcity, EFT_VETERAN_BUILD,
                                    utype_class(choice->value.utype),
                                    MOVETYPE_LAST)) != B_LAST
       && !city_has_building(pcity, improvement_by_number(id))) {
    choice->value.building = improvement_by_number(id);
    choice->type = CT_BUILDING;
  }
}

/********************************************************************** 
    This function selects either a defender or an attacker to be built.
    It records its choice into ai_choice struct.
    If coice->want is 0 this advisor doesn't want anything.
***********************************************************************/
void military_advisor_choose_build(struct player *pplayer, struct city *pcity,
				   struct ai_choice *choice)
{
  struct ai_data *ai = ai_data_get(pplayer);
  struct unit_type *punittype;
  unsigned int our_def, urgency;
  struct tile *ptile = pcity->tile;
  struct unit *virtualunit;

  init_choice(choice);

  /* Note: assess_danger() creates a warmap for us. */
  urgency = assess_danger(pcity);
  /* Changing to quadratic to stop AI from building piles 
   * of small units -- Syela */
  /* It has to be AFTER assess_danger thanks to wallvalue. */
  our_def = assess_defense_quadratic(pcity); 

  if (pcity->id == ai->wonder_city && pcity->ai->grave_danger == 0) {
    return; /* Other cities can build our defenders, thank you! */
  }

  ai_choose_diplomat_defensive(pplayer, pcity, choice, our_def);

  /* Otherwise no need to defend yet */
  if (pcity->ai->danger != 0) {
    struct impr_type *pimprove;
    int num_defenders = unit_list_size(ptile->units);
    int wall_id, danger;

    /* First determine the danger.  It is measured in percents of our 
     * defensive strength, capped at 200 + urgency */
    if (pcity->ai->danger >= our_def) {
      if (urgency == 0) {
        /* don't waste money */
        danger = 100;
      } else if (our_def == 0) {
        danger = 200 + urgency;
      } else {
        danger = MIN(200, 100 * pcity->ai->danger / our_def) + urgency;
      }
    } else { 
      danger = 100 * pcity->ai->danger / our_def;
    }
    if (pcity->surplus[O_SHIELD] <= 0 && our_def != 0) {
      /* Won't be able to support anything */
      danger = 0;
    }

    CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d urgency=%d danger=%d num_def=%d "
             "our_def=%d", urgency, danger, num_defenders, our_def);

    /* FIXME: 1. Will tend to build walls before coastal irrespectfully what
     * type of danger we are facing */
    /* We will build walls if we can and want and (have "enough" defenders or
     * can just buy the walls straight away) */

    /* HACK: This needs changing if multiple improvements provide
     * this effect. */
    wall_id = ai_find_source_building(pcity, EFT_DEFEND_BONUS, NULL, LAND_MOVING);
    pimprove = improvement_by_number(wall_id);

    if (wall_id != B_LAST
	&& pcity->ai->building_want[wall_id] != 0 && our_def != 0 
        && can_city_build_improvement_now(pcity, pimprove)
        && (danger < 101 || num_defenders > 1
            || (pcity->ai->grave_danger == 0 
                && pplayer->economic.gold > impr_buy_gold_cost(pimprove, pcity->shield_stock)))
        && ai_fuzzy(pplayer, TRUE)) {
      /* NB: great wall is under domestic */
      choice->value.building = pimprove;
      /* building_want is hacked by assess_danger */
      choice->want = pcity->ai->building_want[wall_id];
      if (urgency == 0 && choice->want > 100) {
        choice->want = 100;
      }
      choice->type = CT_BUILDING;
      CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d wants defense building with %d",
               choice->want);
    } else if (danger > 0 && num_defenders <= urgency) {
      /* Consider building defensive units units */
      if (process_defender_want(pplayer, pcity, danger, choice)) {
        /* Potential defender found */
        if (urgency == 0
            && choice->value.utype->defense_strength == 1) {
          /* FIXME: check other reqs (unit class?) */
          if (get_city_bonus(pcity, EFT_HP_REGEN) > 0) {
            /* unlikely */
            choice->want = MIN(49, danger);
          } else {
            choice->want = MIN(25, danger);
          }
        } else {
          choice->want = danger;
        }
        CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d wants %s with desire %d",
                 utype_rule_name(choice->value.utype),
                 choice->want);
      } else {
        CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d cannot select defender");
      }
    } else {
      CITY_LOG(LOG_DEBUG, pcity, "m_a_c_d does not want defenders");
    }
  } /* ok, don't need to defend */

  if (pcity->surplus[O_SHIELD] <= 0 
      || pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL] > pcity->feel[CITIZEN_UNHAPPY][FEELING_EFFECT]
      || pcity->id == ai->wonder_city) {
    /* Things we consider below are not life-saving so we don't want to 
     * build them if our populace doesn't feel like it */
    return;
  }

  /* Consider making a land bodyguard */
  punittype = ai_choose_bodyguard(pcity, LAND_MOVING, L_DEFEND_GOOD);
  if (punittype) {
    ai_unit_consider_bodyguard(pcity, punittype, choice);
  }

  /* If we are in severe danger, don't consider attackers. This is probably
     too general. In many cases we will want to buy attackers to counterattack.
     -- Per */
  if (choice->want > 100 && pcity->ai->grave_danger > 0) {
    CITY_LOG(LOGLEVEL_BUILD, pcity, "severe danger (want %d), force defender",
             choice->want);
    return;
  }

  /* Consider making an offensive diplomat */
  ai_choose_diplomat_offensive(pplayer, pcity, choice);

  /* Consider making a sea bodyguard */
  punittype = ai_choose_bodyguard(pcity, SEA_MOVING, L_DEFEND_GOOD);
  if (punittype) {
    ai_unit_consider_bodyguard(pcity, punittype, choice);
  }

  /* Consider making an airplane */
  (void) ai_choose_attacker_air(pplayer, pcity, choice);

  /* Consider making a paratrooper */
  ai_choose_paratrooper(pplayer, pcity, choice);

  /* Check if we want a sailing attacker. Have to put sailing first
     before we mung the seamap */
  punittype = ai_choose_attacker(pcity, SEA_MOVING);
  if (punittype) {
    virtualunit = create_unit_virtual(pplayer, pcity, punittype,
                                      do_make_unit_veteran(pcity, punittype));
    kill_something_with(pplayer, pcity, virtualunit, choice);
    destroy_unit_virtual(virtualunit);
  }

  /* Consider a land attacker or a ferried land attacker
   * (in which case, we might want a ferry before an attacker)
   */
  punittype = ai_choose_attacker(pcity, LAND_MOVING);
  if (punittype) {
    virtualunit = create_unit_virtual(pplayer, pcity, punittype, 1);
    kill_something_with(pplayer, pcity, virtualunit, choice);
    destroy_unit_virtual(virtualunit);
  }

  /* Consider a hunter */
  ai_hunter_choice(pplayer, pcity, choice);

  /* Consider veteran level enhancing buildings before non-urgent units */
  adjust_ai_unit_choice(pcity, choice);

  if (choice->want <= 0) {
    CITY_LOG(LOGLEVEL_BUILD, pcity, "military advisor has no advice");
  } else {
    CITY_LOG(LOGLEVEL_BUILD, pcity, "military advisor choice: %s (want %d)",
             ai_choice_rule_name(choice),
             choice->want);
  }
}
