/********************************************************************** 
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__CM_H
#define FC__CM_H

/*
 * The purpose of this module is to manage the citizens of a city. The
 * caller has to provide a goal (struct cm_parameter) which determines
 * in which way the citizens are allocated and placed. The module will
 * also avoid disorder.
 *
 * The plan defines a minimal surplus. The module will try to get the
 * required surplus. If there are citizens free after allocation of
 * the minimal surplus these citizens will get arranged to maximize
 * the weighted sum over the surplus of each type.
 */

#include "city.h"		/* CITY_MAP_SIZE */
#include "shared.h"		/* bool type */

/* A description of the goal. */
struct cm_parameter {
  int minimal_surplus[O_LAST];
  bool require_happy;
  bool allow_disorder;
  bool allow_specialists;

  int factor[O_LAST];
  int happy_factor;
};

/* A result which can examined. */
struct cm_result {
  bool found_a_valid, disorder, happy;

  int surplus[O_LAST];

  bool worker_positions_used[CITY_MAP_SIZE][CITY_MAP_SIZE];
  int specialists[SP_MAX];
};


void cm_init(void);
void cm_init_citymap(void);

void cm_free(void);

/*
 * Will try to meet the requirements and fill out the result. Caller
 * should test result->found_a_valid. cm_query_result will not change
 * the actual city setting.
 */
void cm_query_result(struct city *pcity,
		     const struct cm_parameter *const parameter,
		     struct cm_result *result);

/*
 * Call this function if the city has changed. To be safe call it
 * everytime before you call cm_query_result.
 */
void cm_clear_cache(struct city *pcity);

/***************** utility methods *************************************/
bool cm_are_parameter_equal(const struct cm_parameter *const p1,
			    const struct cm_parameter *const p2);
void cm_copy_parameter(struct cm_parameter *dest,
		       const struct cm_parameter *const src);
void cm_init_parameter(struct cm_parameter *dest);
void cm_init_emergency_parameter(struct cm_parameter *dest);

void cm_print_city(const struct city *pcity);
void cm_print_result(const struct cm_result *result);

int cm_result_citizens(const struct cm_result *result);
int cm_result_specialists(const struct cm_result *result);
int cm_result_workers(const struct cm_result *result);

void cm_result_from_main_map(struct cm_result *result,
                             const struct city *pcity, bool main_map);
#endif
