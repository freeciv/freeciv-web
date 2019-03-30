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
#ifndef FC__TECH_H
#define FC__TECH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "bitvector.h"
#include "shared.h"

/* common */
#include "fc_types.h"
#include "name_translation.h"
#include "requirements.h"

struct strvec;          /* Actually defined in "utility/string_vector.h". */

/*
  [kept for amusement and posterity]
typedef int Tech_type_id;
  Above typedef replaces old "enum tech_type_id"; see comments about
  Unit_type_id in unit.h, since mainly apply here too, except don't
  use Tech_type_id very widely, and don't use (-1) flag values. (?)
*/
/* [more accurately]
 * Unlike most other indices, the Tech_type_id is widely used, because it 
 * so frequently passed to packet and scripting.  The client menu routines 
 * sometimes add and substract these numbers.
 */
#define A_NONE 0
#define A_FIRST 1
#define A_LAST (MAX_NUM_ADVANCES + 1) /* Used in the network protocol. */
#define A_FUTURE (A_LAST + 1)
#define A_ARRAY_SIZE (A_FUTURE + 1)
#define A_UNSET (A_LAST + 2)
#define A_UNKNOWN (A_LAST + 3)

#define A_NEVER (NULL)

/*
   A_NONE is the root tech. All players always know this tech. It is
   used as a flag in various cases where there is no tech-requirement.

   A_FIRST is the first real tech id value

   A_UNSET indicates that no tech is selected (for research).

   A_FUTURE indicates that the player is researching a future tech.

   A_UNKNOWN may be passed to other players instead of the actual value.

   A_LAST is a value that is guaranteed to be larger than all
   actual Tech_type_id values.  It is used as a flag value; it can
   also be used for fixed allocations to ensure ability to hold the
   full number of techs.

   A_NEVER is the pointer equivalent replacement for A_LAST flag value.
*/

/* Changing these breaks network compatibility. */
/* If a new flag is added techtools.c:research_tech_lost() should be checked */
#define SPECENUM_NAME tech_flag_id
/* player gets extra tech if rearched first */
#define SPECENUM_VALUE0 TF_BONUS_TECH
/* TRANS: this and following strings are 'tech flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("Bonus_Tech")
/* "Settler" unit types can build bridges over rivers */
#define SPECENUM_VALUE1 TF_BRIDGE
#define SPECENUM_VALUE1NAME N_("Bridge")
/* Player can build air units */
#define SPECENUM_VALUE2 TF_BUILD_AIRBORNE
#define SPECENUM_VALUE2NAME N_("Build_Airborne")
/* Player can claim ocean tiles non-adjacent to border source */ 
#define SPECENUM_VALUE3 TF_CLAIM_OCEAN
#define SPECENUM_VALUE3NAME N_("Claim_Ocean")
/* Player can claim ocean tiles non-adjacent to border source as long
 * as source is ocean tile */
#define SPECENUM_VALUE4 TF_CLAIM_OCEAN_LIMITED
#define SPECENUM_VALUE4NAME N_("Claim_Ocean_Limited")
#define SPECENUM_VALUE5 TECH_USER_1
#define SPECENUM_VALUE6 TECH_USER_2
#define SPECENUM_VALUE7 TECH_USER_3
#define SPECENUM_VALUE8 TECH_USER_4
#define SPECENUM_VALUE9 TECH_USER_5
#define SPECENUM_VALUE10 TECH_USER_6
#define SPECENUM_VALUE11 TECH_USER_7
#define SPECENUM_VALUE12 TECH_USER_LAST
/* Keep this last. */
#define SPECENUM_COUNT TF_COUNT
#define SPECENUM_BITVECTOR bv_tech_flags
#define SPECENUM_NAMEOVERRIDE
#include "specenum_gen.h"

#define MAX_NUM_USER_TECH_FLAGS (TECH_USER_LAST - TECH_USER_1 + 1)

enum tech_req {
  AR_ONE = 0,
  AR_TWO = 1,
  AR_ROOT = 2,
  AR_SIZE
};

struct tech_class {
  int idx;
  struct name_translation name;
  bool ruledit_disabled;
  int cost_pct;
};

struct advance {
  Tech_type_id item_number;
  struct name_translation name;
  char graphic_str[MAX_LEN_NAME];	/* which named sprite to use */
  char graphic_alt[MAX_LEN_NAME];	/* alternate icon name */
  struct tech_class *tclass;

  struct advance *require[AR_SIZE];
  bool inherited_root_req;

  /* Required to start researching this tech. For shared research it must
   * be fulfilled for at least one player that shares the research. */
  struct requirement_vector research_reqs;

  bv_tech_flags flags;
  struct strvec *helptext;

  /* 
   * Message displayed to the first player to get a bonus tech 
   */
  char *bonus_message;

  /* Cost of advance in bulbs. It may be specified in ruleset, or
   * calculated in techs_precalc_data(). However, this value wouldn't
   * be right if game.info.tech_cost_style is TECH_COST_CIV1CIV2. */
  double cost;

  /* 
   * Number of requirements this technology has _including_
   * itself. Precalculated at server then send to client.
   */
  int num_reqs;
};

BV_DEFINE(bv_techs, A_LAST);

/* General advance/technology accessor functions. */
Tech_type_id advance_count(void);
Tech_type_id advance_index(const struct advance *padvance);
Tech_type_id advance_number(const struct advance *padvance);

struct advance *advance_by_number(const Tech_type_id atype);

struct advance *valid_advance(struct advance *padvance);
struct advance *valid_advance_by_number(const Tech_type_id atype);

struct advance *advance_by_rule_name(const char *name);
struct advance *advance_by_translated_name(const char *name);

const char *advance_rule_name(const struct advance *padvance);
const char *advance_name_translation(const struct advance *padvance);

void tech_classes_init(void);
struct tech_class *tech_class_by_number(const int idx);
#define tech_class_index(_ptclass_) (_ptclass_)->idx
const char *tech_class_name_translation(const struct tech_class *ptclass);
const char *tech_class_rule_name(const struct tech_class *ptclass);
struct tech_class *tech_class_by_rule_name(const char *name);

#define tech_class_iterate(_p)                                \
{                                                             \
  int _i_##_p;                                                \
  for (_i_##_p = 0; _i_##_p < game.control.num_tech_classes; _i_##_p++) {  \
    struct tech_class *_p = tech_class_by_number(_i_##_p);

#define tech_class_iterate_end                    \
  }                                               \
}

#define tech_class_re_active_iterate(_p)                               \
  tech_class_iterate(_p) {                                             \
    if (!_p->ruledit_disabled) {

#define tech_class_re_active_iterate_end                               \
    }                                                                  \
  } tech_class_iterate_end;

void user_tech_flags_init(void);
void user_tech_flags_free(void);
void set_user_tech_flag_name(enum tech_flag_id id, const char *name, const char *helptxt);
const char *tech_flag_helptxt(enum tech_flag_id id);

/* General advance/technology flag accessor routines */
bool advance_has_flag(Tech_type_id tech, enum tech_flag_id flag);

/* Ancillary routines */
Tech_type_id advance_required(const Tech_type_id tech,
			      enum tech_req require);
struct advance *advance_requires(const struct advance *padvance,
				 enum tech_req require);

bool techs_have_fixed_costs(void);

bool is_future_tech(Tech_type_id tech);

/* Initialization */
void techs_init(void);
void techs_free(void);

void techs_precalc_data(void);

/* Iteration */

/* This iterates over almost all technologies.  It includes non-existent
 * technologies, but not A_FUTURE. */
#define advance_index_iterate(_start, _index)				\
{									\
  Tech_type_id _index = (_start);					\
  for (; _index < advance_count(); _index++) {

#define advance_index_iterate_end					\
  }									\
}

const struct advance *advance_array_last(void);

#define advance_iterate(_start, _p)					\
{									\
  struct advance *_p = advance_by_number(_start);			\
  if (NULL != _p) {							\
    for (; _p <= advance_array_last(); _p++) {

#define advance_iterate_end						\
    }									\
  }									\
}

#define advance_re_active_iterate(_p)                                    \
  advance_iterate(A_FIRST, _p) {                                         \
    if (_p->require[AR_ONE] != A_NEVER) {

#define advance_re_active_iterate_end                                   \
    }                                                                   \
  } advance_iterate_end;


/* Advance requirements iterator.
 * Iterates over 'goal' and all its requirements (including root_reqs),
 * recursively. */
struct advance_req_iter;

size_t advance_req_iter_sizeof(void);
struct iterator *advance_req_iter_init(struct advance_req_iter *it,
                                       const struct advance *goal);

#define advance_req_iterate(_goal, _padvance)                               \
  generic_iterate(struct advance_req_iter, const struct advance *,          \
                  _padvance, advance_req_iter_sizeof, advance_req_iter_init,\
                  _goal)
#define advance_req_iterate_end generic_iterate_end

/* Iterates over all the root requirements of 'goal'.
 * (Not including 'goal' itself, unless it is the special case of a
 * self-root-req technology.) */
struct advance_root_req_iter;

size_t advance_root_req_iter_sizeof(void);
struct iterator *advance_root_req_iter_init(struct advance_root_req_iter *it,
                                            const struct advance *goal);

#define advance_root_req_iterate(_goal, _padvance)                          \
  generic_iterate(struct advance_root_req_iter, const struct advance *,     \
                  _padvance, advance_root_req_iter_sizeof,                  \
                  advance_root_req_iter_init,                               \
                  _goal)
#define advance_root_req_iterate_end generic_iterate_end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__TECH_H */
