/********************************************************************** 
 Freeciv - Copyright (C) 2001 - R. Falke
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__CLIENT_AGENTS_CITY_MANAGEMENT_H
#define FC__CLIENT_AGENTS_CITY_MANAGEMENT_H

/*
 * CM stands for citizen management.
 *
 * The purpose of this module is to manage the citizens of a city. The
 * caller has to provide a goal (struct cma_parameter) which
 * determines in which way the citizens are allocated and placed. The
 * module will also avoid disorder.
 *
 * The plan defines a minimal surplus. The module will try to get the
 * required surplus. If there are citizens free after allocation of
 * the minimal surplus these citizens will get arranged to maximize
 * the sum over base*factor. The base depends upon the factor_target.
 */

#include "cm.h"

#include "attribute.h"

/*
 * Called once per client start.
 */
void cma_init(void);

/* Change the actual city setting. */
bool cma_apply_result(struct city *pcity,
		      const struct cm_result *const result);

/* Till a call of cma_release_city the city will be managed by the agent. */
void cma_put_city_under_agent(struct city *pcity,
			      const struct cm_parameter *const parameter);

/* Release the city from the agent. */
void cma_release_city(struct city *pcity);

/*
 * Test if the citizen in the given city are managed by the agent. The
 * given parameter is filled if pointer is non-NULL. The parameter is
 * only valid if cma_is_city_under_agent returns true.
 */
bool cma_is_city_under_agent(const struct city *pcity,
			     struct cm_parameter *parameter);

/***************** utility methods *************************************/
bool cma_get_parameter(enum attr_city attr, int city_id,
		       struct cm_parameter *parameter);
void cma_set_parameter(enum attr_city attr, int city_id,
		       const struct cm_parameter *parameter);

#endif
