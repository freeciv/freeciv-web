/***********************************************************************
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common */
#include "fc_types.h"

/* Range of requirements.
 * Used in the network protocol.
 * Order is important -- wider ranges should come later -- some code
 * assumes a total order, or tests for e.g. >= REQ_RANGE_PLAYER.
 * Ranges of similar types should be supersets, for example:
 *  - the set of Adjacent tiles contains the set of CAdjacent tiles,
 *    and both contain the center Local tile (a requirement on the local
 *    tile is also within Adjacent range);
 *  - World contains Alliance contains Player (a requirement we ourselves
 *    have is also within Alliance range). */
#define SPECENUM_NAME req_range
#define SPECENUM_VALUE0 REQ_RANGE_LOCAL
#define SPECENUM_VALUE0NAME "Local"
#define SPECENUM_VALUE1 REQ_RANGE_CADJACENT
#define SPECENUM_VALUE1NAME "CAdjacent"
#define SPECENUM_VALUE2 REQ_RANGE_ADJACENT
#define SPECENUM_VALUE2NAME "Adjacent"
#define SPECENUM_VALUE3 REQ_RANGE_CITY
#define SPECENUM_VALUE3NAME "City"
#define SPECENUM_VALUE4 REQ_RANGE_TRADEROUTE
#define SPECENUM_VALUE4NAME "Traderoute"
#define SPECENUM_VALUE5 REQ_RANGE_CONTINENT
#define SPECENUM_VALUE5NAME "Continent"
#define SPECENUM_VALUE6 REQ_RANGE_PLAYER
#define SPECENUM_VALUE6NAME "Player"
#define SPECENUM_VALUE7 REQ_RANGE_TEAM
#define SPECENUM_VALUE7NAME "Team"
#define SPECENUM_VALUE8 REQ_RANGE_ALLIANCE
#define SPECENUM_VALUE8NAME "Alliance"
#define SPECENUM_VALUE9 REQ_RANGE_WORLD
#define SPECENUM_VALUE9NAME "World"
#define SPECENUM_COUNT REQ_RANGE_COUNT /* keep this last */
#include "specenum_gen.h"

#define req_range_iterate(_range_) \
  {                                \
    enum req_range _range_;        \
    for (_range_ = REQ_RANGE_LOCAL ; _range_ < REQ_RANGE_COUNT ; \
         _range_ = (enum req_range)(_range_ + 1)) {

#define req_range_iterate_end \
    }                         \
  }

/* A requirement. This requirement is basically a conditional; it may or
 * may not be active on a target.  If it is active then something happens.
 * For instance units and buildings have requirements to be built, techs
 * have requirements to be researched, and effects have requirements to be
 * active.
 * Used in the network protocol. */
struct requirement {
  struct universal source;		/* requirement source */
  enum req_range range;			/* requirement range */
  bool survives; /* set if destroyed sources satisfy the req*/
  bool present;	 /* set if the requirement is to be present */
  bool quiet;    /* do not list this in helptext */
};

#define SPECVEC_TAG requirement
#define SPECVEC_TYPE struct requirement
#include "specvec.h"
#define requirement_vector_iterate(req_vec, preq) \
  TYPED_VECTOR_ITERATE(struct requirement, req_vec, preq)
#define requirement_vector_iterate_end VECTOR_ITERATE_END

/* General requirement functions. */
struct requirement req_from_str(const char *type, const char *range,
                                bool survives, bool present, bool quiet,
                                const char *value);
const char *req_to_fstring(const struct requirement *req);

void req_get_values(const struct requirement *req, int *type,
                    int *range, bool *survives, bool *present, bool *quiet,
                    int *value);
struct requirement req_from_values(int type, int range,
                                   bool survives, bool present, bool quiet,
                                   int value);

bool are_requirements_equal(const struct requirement *req1,
			    const struct requirement *req2);

bool are_requirements_contradictions(const struct requirement *req1,
                                     const struct requirement *req2);

bool does_req_contradicts_reqs(const struct requirement *req,
                               const struct requirement_vector *vec);

void requirement_vector_contradiction_clean(struct requirement_vector *vec);

bool is_req_active(const struct player *target_player,
		   const struct player *other_player,
		   const struct city *target_city,
		   const struct impr_type *target_building,
		   const struct tile *target_tile,
                   const struct unit *target_unit,
                   const struct unit_type *target_unittype,
		   const struct output_type *target_output,
		   const struct specialist *target_specialist,
                   const struct action *target_action,
		   const struct requirement *req,
                   const enum   req_problem_type prob_type);
bool are_reqs_active(const struct player *target_player,
                     const struct player *other_player,
                     const struct city *target_city,
                     const struct impr_type *target_building,
                     const struct tile *target_tile,
                     const struct unit *target_unit,
                     const struct unit_type *target_unittype,
                     const struct output_type *target_output,
                     const struct specialist *target_specialist,
                     const struct action *target_action,
                     const struct requirement_vector *reqs,
                     const enum   req_problem_type prob_type);

bool is_req_unchanging(const struct requirement *req);

bool is_req_in_vec(const struct requirement *req,
                   const struct requirement_vector *vec);

/* General universal functions. */
int universal_number(const struct universal *source);

struct universal universal_by_number(const enum universals_n kind,
                                     const int value);
struct universal universal_by_rule_name(const char *kind,
                                        const char *value);
void universal_value_from_str(struct universal *source, const char *value);
void universal_extraction(const struct universal *source,
                          int *kind, int *value);

bool are_universals_equal(const struct universal *psource1,
                          const struct universal *psource2);

const char *universal_rule_name(const struct universal *psource);
const char *universal_name_translation(const struct universal *psource,
				       char *buf, size_t bufsz);
const char *universal_type_rule_name(const struct universal *psource);

int universal_build_shield_cost(const struct city *pcity,
                                const struct universal *target);

bool universal_replace_in_req_vec(struct requirement_vector *reqs,
                                  const struct universal *to_replace,
                                  const struct universal *replacement);

#define universal_is_mentioned_by_requirement(preq, psource)               \
  are_universals_equal(&preq->source, psource)
bool universal_is_mentioned_by_requirements(
    const struct requirement_vector *reqs,
    const struct universal *psource);

/* An item contradicts, fulfills or is irrelevant to the requirement */
enum req_item_found {ITF_NO, ITF_YES, ITF_NOT_APPLICABLE};

void universal_found_functions_init(void);
enum req_item_found
universal_fulfills_requirement(const struct requirement *preq,
                               const struct universal *source);
bool universal_fulfills_requirements(bool check_necessary,
                                     const struct requirement_vector *reqs,
                                     const struct universal *source);
bool universal_is_relevant_to_requirement(const struct requirement *req,
                                          const struct universal *source);

#define universals_iterate(_univ_) \
  {                                \
    enum universals_n _univ_;      \
    for (_univ_ = VUT_NONE; _univ_ < VUT_COUNT; _univ_ = (enum universals_n)(_univ_ + 1)) {

#define universals_iterate_end \
    }                          \
  }

/* Accessors to determine if a universal fulfills a requirement vector.
 * When adding an additional accessor, be sure to add the appropriate
 * item_found function in universal_found_functions_init(). */
/* XXX Some versions of g++ can't cope with the struct literals */
#define requirement_fulfilled_by_government(_gov_, _rqs_)                  \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
      &(struct universal){.kind = VUT_GOVERNMENT, .value = {.govern = (_gov_)}})
#define requirement_fulfilled_by_nation(_nat_, _rqs_)                      \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
      &(struct universal){.kind = VUT_NATION, .value = {.nation = (_nat_)}})
#define requirement_fulfilled_by_improvement(_imp_, _rqs_)                 \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
    &(struct universal){.kind = VUT_IMPROVEMENT,                           \
                        .value = {.building = (_imp_)}})
#define requirement_fulfilled_by_terrain(_ter_, _rqs_)                 \
  universal_fulfills_requirements(FALSE, (_rqs_),                      \
    &(struct universal){.kind = VUT_TERRAIN,                           \
                        .value = {.terrain = (_ter_)}})
#define requirement_fulfilled_by_unit_class(_uc_, _rqs_)                   \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
      &(struct universal){.kind = VUT_UCLASS, .value = {.uclass = (_uc_)}})
#define requirement_fulfilled_by_unit_type(_ut_, _rqs_)                    \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
      &(struct universal){.kind = VUT_UTYPE, .value = {.utype = (_ut_)}})
#define requirement_fulfilled_by_extra(_ex_, _rqs_)                        \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
      &(struct universal){.kind = VUT_EXTRA, .value = {.extra = (_ex_)}})
#define requirement_fulfilled_by_output_type(_o_, _rqs_)                   \
  universal_fulfills_requirements(FALSE, (_rqs_),                          \
      &(struct universal){.kind = VUT_OTYPE, .value = {.outputtype = (_o_)}})

#define requirement_needs_improvement(_imp_, _rqs_)                        \
  universal_fulfills_requirements(TRUE, (_rqs_),                           \
    &(struct universal){.kind = VUT_IMPROVEMENT,                           \
                        .value = {.building = (_imp_)}})

int requirement_kind_ereq(const int value,
                          const enum req_range range,
                          const bool present,
                          const int max_value);

#define requirement_diplrel_ereq(_id_, _range_, _present_)                \
  requirement_kind_ereq(_id_, _range_, _present_, DRO_LAST)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__REQUIREMENTS_H */
