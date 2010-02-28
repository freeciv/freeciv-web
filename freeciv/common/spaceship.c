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
  {23, 23, 28},
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

/**********************************************************************
Initialize spaceship struct; could also be used to "cancel" a
spaceship (eg, if/when capital-capture effect implemented).
**********************************************************************/
void spaceship_init(struct player_spaceship *ship)
{
  int i;
  
  ship->structurals = ship->components = ship->modules = 0;
  for(i=0; i<NUM_SS_STRUCTURALS; i++) {
    ship->structure[i] = FALSE;
  }
  ship->fuel = ship->propulsion = 0;
  ship->habitation = ship->life_support = ship->solar_panels = 0;
  ship->state = SSHIP_NONE;
  ship->launch_year = 9999;

  ship->population = ship->mass = 0;
  ship->support_rate = ship->energy_rate =
    ship->success_rate = ship->travel_time = 0.0;
}

/**********************************************************************
Count the number of structurals placed; that is, in ship->structure[]
**********************************************************************/
int num_spaceship_structurals_placed(const struct player_spaceship *ship)
{
  int i, num = 0;
  for(i=0; i<NUM_SS_STRUCTURALS; i++) {
    if (ship->structure[i]) num++;
  }
  return num;
}
