/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__REQUIREMENTS_H
#define FC__REQUIREMENTS_H

#include "fc_types.h"

#include "tech.h"
#include "terrain.h"
#include "unittype.h"

/* Range of requirements.  This must correspond to req_range_names[]
 * in requirements.c. */
enum req_range {
  REQ_RANGE_LOCAL,
  REQ_RANGE_ADJACENT,
  REQ_RANGE_CITY,
  REQ_RANGE_CONTINENT,
  REQ_RANGE_PLAYER,
  REQ_RANGE_WORLD,
  REQ_RANGE_LAST   /* keep this last */
};

/* A requirement. This requirement is basically a conditional; it may or
 * may not be active on a target.  If it is active then something happens.
 * For instance units and buildings have requirements to be built, techs
 * have requirements to be researched, and effects have requirements to be
 * active. */
struct requirement {
  struct universal source;		/* requirement source */
  enum req_range range;			/* requirement range */
  bool survives; /* set if destroyed sources satisfy the req*/
  bool negated;	 /* set if the requirement is to be negated */
};

#define SPECLIST_TAG requirement
#define SPECLIST_TYPE struct requirement
#include "speclist.h"
#define requirement_list_iterate(req_list, preq) \
  TYPED_LIST_ITERATE(struct requirement, req_list, preq)
#define requirement_list_iterate_end LIST_ITERATE_END

#define SPECVEC_TAG requirement
#define SPECVEC_TYPE struct requirement
#include "specvec.h"
#define requirement_vector_iterate(req_vec, preq) \
  TYPED_VECTOR_ITERATE(struct requirement, req_vec, preq)
#define requirement_vector_iterate_end VECTOR_ITERATE_END

/* General requirement functions. */
enum req_range req_range_from_str(const char *str);
struct requirement req_from_str(const char *type, const char *range,
				bool survives, bool negated,
				const char *value);

void req_get_values(const struct requirement *req, int *type,
		    int *range, bool *survives, bool *negated,
		    int *value);
struct requirement req_from_values(int type, int range,
				   bool survives, bool negated, int value);

bool are_requirements_equal(const struct requirement *req1,
			    const struct requirement *req2);

bool is_req_active(const struct player *target_player,
		   const struct city *target_city,
		   const struct impr_type *target_building,
		   const struct tile *target_tile,
		   const struct unit_type *target_unittype,
		   const struct output_type *target_output,
		   const struct specialist *target_specialist,
		   const struct requirement *req,
                   const enum   req_problem_type prob_type);
bool are_reqs_active(const struct player *target_player,
		     const struct city *target_city,
		     const struct impr_type *target_building,
		     const struct tile *target_tile,
		     const struct unit_type *target_unittype,
		     const struct output_type *target_output,
		     const struct specialist *target_specialist,
		     const struct requirement_vector *reqs,
                     const enum   req_problem_type prob_type);

bool is_req_unchanging(const struct requirement *req);

/* General universal functions. */
int universal_number(const struct universal *source);

struct universal universal_by_number(const enum universals_n kind,
				     const int value);
struct universal universal_by_rule_name(const char *kind,
					const char *value);
void universal_extraction(const struct universal *source,
			  int *kind, int *value);

bool are_universals_equal(const struct universal *psource1,
			  const struct universal *psource2);

const char *universal_kind_name(const enum universals_n kind);
const char *universal_rule_name(const struct universal *psource);
const char *universal_name_translation(const struct universal *psource,
				       char *buf, size_t bufsz);
const char *universal_type_rule_name(const struct universal *psource);

int universal_build_shield_cost(const struct universal *target);

#endif  /* FC__REQUIREMENTS_H */
