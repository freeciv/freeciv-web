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

#include "shared.h"		/* TRUE, FALSE */

#include "spaceship.h"

const struct sship_part_info structurals_info[NUM_SS_STRUCTURALS] = {
  {19, 13, -1},		/* -1 means none required */
  {19, 15,  0},
  {19, 11,  0},
  {19, 17,  1},
  {19,  9,  2},
  {19, 19,  3},
  {17,  9,  4},
  {17, 19,  5},
  {21, 11,  2},
  {21, 17,  3},
  {15,  9,  6},
  {15, 19,  7},
  {23, 11,  8},
  {23, 17,  9},
  {13,  9, 10},
  {13, 19, 11},
  {11,  9, 14},
  {11, 19, 15},
  { 9,  9, 16},
  { 9, 19, 17},
  { 7,  9, 18},
  { 7, 19, 19},
  {19,  7,  4},
  {19, 21,  5},
  {19,  5, 22},
  {19, 23, 23},
  {21,  5, 24},
  {21, 23, 25},
  {23,  5, 26},
  {23, 23, 27},
  { 5,  9, 20},
  { 5, 19, 21}
};

const struct sship_part_info components_info[NUM_SS_COMPONENTS] = {
  {21, 13,  0},
  {24, 13, 12},
  {21, 15,  1},
  {24, 15, 13},
  {21,  9,  8},  /* or 4 */
  {24,  9, 12},
  {21, 19,  9},  /* or 5 */
  {24, 19, 13},
  {21,  7, 22},
  {24,  7, 28},
  {21, 21, 23},
  {24, 21, 29},
  {21,  3, 26},
  {24,  3, 28},
  {21, 25, 27},
  {24, 25, 29}
};

const struct sship_part_info modules_info[NUM_SS_MODULES] = {
  {16, 12,  0},
  {16, 16,  1},
  {14,  6, 10},
  {12, 16, 15},
  {12, 12, 14},
  {14, 22, 11},
  { 8, 12, 18},
  { 8, 16, 19},
  { 6,  6, 20},
  { 4, 16, 31},
  { 4, 12, 30},
  { 6, 22, 21}
};

/*******************************************************************//**
  Initialize spaceship struct; could also be used to "cancel" a
  spaceship (eg, if/when capital-capture effect implemented).
***********************************************************************/
void spaceship_init(struct player_spaceship *ship)
{
  ship->structurals = ship->components = ship->modules = 0;
  BV_CLR_ALL(ship->structure);
  ship->fuel = ship->propulsion = 0;
  ship->habitation = ship->life_support = ship->solar_panels = 0;
  ship->state = SSHIP_NONE;
  ship->launch_year = 9999;

  ship->population = ship->mass = 0;
  ship->support_rate = ship->energy_rate =
    ship->success_rate = ship->travel_time = 0.0;
}

/*******************************************************************//**
  Count the number of structurals placed; that is, in ship->structure[]
***********************************************************************/
int num_spaceship_structurals_placed(const struct player_spaceship *ship)
{
  int i;
  int num = 0;

  for (i = 0; i < NUM_SS_STRUCTURALS; i++) {
    if (BV_ISSET(ship->structure, i)) {
      num++;
    }
  }

  return num;
}

/*******************************************************************//**
  Find (default) place for next spaceship component.
***********************************************************************/
bool next_spaceship_component(struct player *pplayer,
                              struct player_spaceship *ship,
                              struct spaceship_component *fill)
{
  fc_assert_ret_val(fill != NULL, FALSE);
  
  if (ship->modules > (ship->habitation + ship->life_support
		       + ship->solar_panels)) {
    /* "nice" governments prefer to keep success 100%;
     * others build habitation first (for score?)  (Thanks Massimo.)
     */
    fill->type =
      (ship->habitation == 0)   ? SSHIP_PLACE_HABITATION :
      (ship->life_support == 0) ? SSHIP_PLACE_LIFE_SUPPORT :
      (ship->solar_panels == 0) ? SSHIP_PLACE_SOLAR_PANELS :
      ((ship->habitation < ship->life_support)
       && (ship->solar_panels*2 >= ship->habitation + ship->life_support + 1))
                              ? SSHIP_PLACE_HABITATION :
      (ship->solar_panels*2 < ship->habitation + ship->life_support)
                              ? SSHIP_PLACE_SOLAR_PANELS :
      (ship->life_support < ship->habitation)
                              ? SSHIP_PLACE_LIFE_SUPPORT :
      ((ship->life_support <= ship->habitation)
       && (ship->solar_panels * 2 >= ship->habitation + ship->life_support + 1))
                              ? SSHIP_PLACE_LIFE_SUPPORT :
                                SSHIP_PLACE_SOLAR_PANELS;

    if (fill->type == SSHIP_PLACE_HABITATION) {
      fill->num = ship->habitation + 1;
    } else if (fill->type == SSHIP_PLACE_LIFE_SUPPORT) {
      fill->num = ship->life_support + 1;
    } else {
      fill->num = ship->solar_panels + 1;
    }
    fc_assert(fill->num <= NUM_SS_MODULES / 3);

    return TRUE;
  }
  if (ship->components > ship->fuel + ship->propulsion) {
    if (ship->fuel <= ship->propulsion) {
      fill->type = SSHIP_PLACE_FUEL;
      fill->num = ship->fuel + 1;
    } else {
      fill->type = SSHIP_PLACE_PROPULSION;
      fill->num = ship->propulsion + 1;
    }
    return TRUE;
  }
  if (ship->structurals > num_spaceship_structurals_placed(ship)) {
    /* Want to choose which structurals are most important.
       Else we first want to connect one of each type of module,
       then all placed components, pairwise, then any remaining
       modules, or else finally in numerical order.
    */
    int req = -1;
    int i;

    if (!BV_ISSET(ship->structure, 0)) {
      /* if we don't have the first structural, place that! */
      fill->type = SSHIP_PLACE_STRUCTURAL;
      fill->num = 0;
      return TRUE;
    }
    
    if (ship->habitation >= 1
        && !BV_ISSET(ship->structure, modules_info[0].required)) {
      req = modules_info[0].required;
    } else if (ship->life_support >= 1
               && !BV_ISSET(ship->structure, modules_info[1].required)) {
      req = modules_info[1].required;
    } else if (ship->solar_panels >= 1
               && !BV_ISSET(ship->structure, modules_info[2].required)) {
      req = modules_info[2].required;
    } else {
      for (i = 0; i < NUM_SS_COMPONENTS; i++) {
	if ((i % 2 == 0 && ship->fuel > (i/2))
	    || (i % 2 == 1 && ship->propulsion > (i/2))) {
	  if (!BV_ISSET(ship->structure, components_info[i].required)) {
	    req = components_info[i].required;
	    break;
	  }
	}
      }
    }
    if (req == -1) {
      for (i = 0; i < NUM_SS_MODULES; i++) {
	if ((i % 3 == 0 && ship->habitation > (i/3))
	    || (i % 3 == 1 && ship->life_support > (i/3))
	    || (i % 3 == 2 && ship->solar_panels > (i/3))) {
	  if (!BV_ISSET(ship->structure, modules_info[i].required)) {
	    req = modules_info[i].required;
	    break;
	  }
	}
      }
    }
    if (req == -1) {
      for (i = 0; i < NUM_SS_STRUCTURALS; i++) {
        if (!BV_ISSET(ship->structure, i)) {
	  req = i;
	  break;
	}
      }
    }
    /* sanity: */
    fc_assert(req != -1);
    fc_assert(!BV_ISSET(ship->structure, req));

    /* Now we want to find a structural we can build which leads to req.
       This loop should bottom out, because everything leads back to s0,
       and we made sure above that we do s0 first.
     */
    while (!BV_ISSET(ship->structure, structurals_info[req].required)) {
      req = structurals_info[req].required;
    }
    fill->type = SSHIP_PLACE_STRUCTURAL;
    fill->num = req;

    return TRUE;
  }

  return FALSE;
}
