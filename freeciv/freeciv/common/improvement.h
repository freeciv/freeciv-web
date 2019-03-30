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
#ifndef FC__IMPROVEMENT_H
#define FC__IMPROVEMENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* City Improvements, including Wonders.  (Alternatively "Buildings".) */

/* utility */
#include "bitvector.h"
#include "support.h"            /* bool */

/* common */
#include "fc_types.h"
#include "name_translation.h"
#include "requirements.h"

struct strvec;          /* Actually defined in "utility/string_vector.h". */

/* B_LAST is a value that is guaranteed to be larger than all
 * actual Impr_type_id values.  It is used as a flag value; it can
 * also be used for fixed allocations to ensure ability to hold the
 * full number of improvement types.
 *
 * B_NEVER is the pointer equivalent replacement for B_LAST flag value.
 *
 * Used in the network protocol.
 */
#define B_LAST MAX_NUM_BUILDINGS

#define B_NEVER (NULL)

/* Changing these breaks network compatibility. */
#define SPECENUM_NAME impr_flag_id
/* improvement should be visible to others without spying */
#define SPECENUM_VALUE0 IF_VISIBLE_BY_OTHERS
#define SPECENUM_VALUE0NAME "VisibleByOthers"
/* this small wonder is moved to another city if game.savepalace is on. */
#define SPECENUM_VALUE1 IF_SAVE_SMALL_WONDER
#define SPECENUM_VALUE1NAME "SaveSmallWonder"
/* when built, gives gold */
#define SPECENUM_VALUE2 IF_GOLD
#define SPECENUM_VALUE2NAME "Gold"
/* Never destroyed by disasters */
#define SPECENUM_VALUE3 IF_DISASTER_PROOF
#define SPECENUM_VALUE3NAME "DisasterProof"
#define SPECENUM_COUNT IF_COUNT
#define SPECENUM_BITVECTOR bv_impr_flags
#include "specenum_gen.h"

/* Used in the network protocol. */
BV_DEFINE(bv_imprs, B_LAST);

/* Type of improvement. (Read from buildings.ruleset file.) */
struct impr_type {
  Impr_type_id item_number;
  struct name_translation name;
  bool ruledit_disabled;                /* Does not really exist - hole in improvements array */
  char graphic_str[MAX_LEN_NAME];	/* city icon of improv. */
  char graphic_alt[MAX_LEN_NAME];	/* city icon of improv. */
  struct requirement_vector reqs;
  struct requirement_vector obsolete_by;
  int build_cost;			/* Use wrappers to access this. */
  int upkeep;
  int sabotage;		/* Base chance of diplomat sabotage succeeding. */
  enum impr_genus_id genus;		/* genus; e.g. GreatWonder */
  bv_impr_flags flags;
  struct strvec *helptext;
  char soundtag[MAX_LEN_NAME];
  char soundtag_alt[MAX_LEN_NAME];

  /* Cache */
  bool allows_units;
  bool allows_extras;
  bool prevents_disaster;
  bool protects_vs_actions;
  
};


/* General improvement accessor functions. */
Impr_type_id improvement_count(void);
Impr_type_id improvement_index(const struct impr_type *pimprove);
Impr_type_id improvement_number(const struct impr_type *pimprove);

struct impr_type *improvement_by_number(const Impr_type_id id);

struct impr_type *valid_improvement(struct impr_type *pimprove);
struct impr_type *valid_improvement_by_number(const Impr_type_id id);

struct impr_type *improvement_by_rule_name(const char *name);
struct impr_type *improvement_by_translated_name(const char *name);

const char *improvement_rule_name(const struct impr_type *pimprove);
const char *improvement_name_translation(const struct impr_type *pimprove);

/* General improvement flag accessor routines */
bool improvement_has_flag(const struct impr_type *pimprove,
                          enum impr_flag_id flag);

/* Ancillary routines */
int impr_build_shield_cost(const struct city *pcity,
                           const struct impr_type *pimprove);
int impr_buy_gold_cost(const struct city *pcity, const struct impr_type *pimprove,
                       int shields_in_stock);
int impr_sell_gold(const struct impr_type *pimprove);

bool is_improvement_visible(const struct impr_type *pimprove);

bool is_great_wonder(const struct impr_type *pimprove);
bool is_small_wonder(const struct impr_type *pimprove);
bool is_wonder(const struct impr_type *pimprove);
bool is_improvement(const struct impr_type *pimprove);
bool is_special_improvement(const struct impr_type *pimprove);

bool can_improvement_go_obsolete(const struct impr_type *pimprove);

bool can_sell_building(struct impr_type *pimprove);
bool can_city_sell_building(const struct city *pcity,
			    struct impr_type *pimprove);
enum test_result test_player_sell_building_now(struct player *pplayer,
                                               struct city *pcity,
                                               struct impr_type *pimprove);

struct impr_type *improvement_replacement(const struct impr_type *pimprove);

/* Macros for struct packet_game_info::great_wonder_owners[]. */
#define WONDER_DESTROYED (MAX_NUM_PLAYER_SLOTS + 1)  /* Used as player id. */
#define WONDER_NOT_OWNED (MAX_NUM_PLAYER_SLOTS + 2)  /* Used as player id. */
#define WONDER_OWNED(player_id) ((player_id) < MAX_NUM_PLAYER_SLOTS)

/* Macros for struct player::wonders[]. */
#define WONDER_LOST (-1)        /* Used as city id. */
#define WONDER_NOT_BUILT 0      /* Used as city id. */
#define WONDER_BUILT(city_id) ((city_id) > 0)

void wonder_built(const struct city *pcity, const struct impr_type *pimprove);
void wonder_destroyed(const struct city *pcity,
                      const struct impr_type *pimprove);

bool wonder_is_lost(const struct player *pplayer,
                    const struct impr_type *pimprove);
bool wonder_is_built(const struct player *pplayer,
                     const struct impr_type *pimprove);
struct city *city_from_wonder(const struct player *pplayer,
                              const struct impr_type *pimprove);

bool great_wonder_is_built(const struct impr_type *pimprove);
bool great_wonder_is_destroyed(const struct impr_type *pimprove);
bool great_wonder_is_available(const struct impr_type *pimprove);
struct city *city_from_great_wonder(const struct impr_type *pimprove);
struct player *great_wonder_owner(const struct impr_type *pimprove);

bool small_wonder_is_built(const struct player *pplayer,
                           const struct impr_type *pimprove);
struct city *city_from_small_wonder(const struct player *pplayer,
                                    const struct impr_type *pimprove);

/* player related improvement functions */
bool improvement_obsolete(const struct player *pplayer,
			  const struct impr_type *pimprove,
                          const struct city *pcity);
bool is_improvement_productive(const struct city *pcity,
                               struct impr_type *pimprove);
bool is_improvement_redundant(const struct city *pcity,
                              struct impr_type *pimprove);

bool can_player_build_improvement_direct(const struct player *p,
					 struct impr_type *pimprove);
bool can_player_build_improvement_later(const struct player *p,
					struct impr_type *pimprove);
bool can_player_build_improvement_now(const struct player *p,
				      struct impr_type *pimprove);

/* Initialization and iteration */
void improvements_init(void);
void improvements_free(void);

void improvement_feature_cache_init(void);

struct impr_type *improvement_array_first(void);
const struct impr_type *improvement_array_last(void);

#define improvement_iterate(_p)						\
{									\
  struct impr_type *_p = improvement_array_first();			\
  if (NULL != _p) {							\
    for (; _p <= improvement_array_last(); _p++) {

#define improvement_iterate_end						\
    }									\
  }									\
}

#define improvement_re_active_iterate(_p)                               \
  improvement_iterate(_p) {                                             \
    if (!_p->ruledit_disabled) {

#define improvement_re_active_iterate_end                               \
    }                                                                   \
  } improvement_iterate_end;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__IMPROVEMENT_H */
