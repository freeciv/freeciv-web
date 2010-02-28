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
#ifndef FC__IMPROVEMENT_H
#define FC__IMPROVEMENT_H

/* City Improvements, including Wonders.  (Alternatively "Buildings".) */

#include "shared.h"		/* bool */

#include "fc_types.h"

#include "requirements.h"

/* B_LAST is a value that is guaranteed to be larger than all
 * actual Impr_type_id values.  It is used as a flag value; it can
 * also be used for fixed allocations to ensure ability to hold the
 * full number of improvement types.
 *
 * B_NEVER is the pointer equivalent replacement for B_LAST flag value.
 */
#define B_LAST MAX_NUM_ITEMS

#define B_NEVER (NULL)


/* Changing these breaks network compatibility. */
enum impr_flag_id {
  IF_VISIBLE_BY_OTHERS,  /* improvement should be visible to others without spying */
  IF_SAVE_SMALL_WONDER,  /* this small wonder is moved to another city if game.savepalace is on. */
  IF_GOLD,		 /* when built, gives gold */
  IF_LAST
};

enum impr_genus_id {
  IG_GREAT_WONDER,
  IG_SMALL_WONDER,
  IG_IMPROVEMENT,
  IG_SPECIAL,
  IG_LAST
};

BV_DEFINE(bv_imprs, B_LAST);

/* Type of improvement. (Read from buildings.ruleset file.) */
struct impr_type {
  Impr_type_id item_number;
  struct name_translation name;
  char graphic_str[MAX_LEN_NAME];	/* city icon of improv. */
  char graphic_alt[MAX_LEN_NAME];	/* city icon of improv. */
  struct requirement_vector reqs;
  struct advance *obsolete_by;		/* A_NEVER = never obsolete */
  struct impr_type *replaced_by;	/* B_NEVER = never replaced */
  int build_cost;			/* Use wrappers to access this. */
  int upkeep;
  int sabotage;		/* Base chance of diplomat sabotage succeeding. */
  enum impr_genus_id genus;		/* genus; e.g. GreatWonder */
  unsigned int flags;
  char *helptext;
  char soundtag[MAX_LEN_NAME];
  char soundtag_alt[MAX_LEN_NAME];

  bool allows_units;   /* Cache */
};


/* General improvement accessor functions. */
Impr_type_id improvement_count(void);
Impr_type_id improvement_index(const struct impr_type *pimprove);
Impr_type_id improvement_number(const struct impr_type *pimprove);

struct impr_type *improvement_by_number(const Impr_type_id id);

struct impr_type *valid_improvement(struct impr_type *pimprove);
struct impr_type *valid_improvement_by_number(const Impr_type_id id);

struct impr_type *find_improvement_by_rule_name(const char *name);
struct impr_type *find_improvement_by_translated_name(const char *name);

const char *improvement_rule_name(const struct impr_type *pimprove);
const char *improvement_name_translation(struct impr_type *pimprove);

/* General improvement flag accessor routines */
bool improvement_has_flag(const struct impr_type *pimprove,
			  enum impr_flag_id flag);
enum impr_flag_id find_improvement_flag_by_rule_name(const char *s);

/* Ancillary routines */
int impr_build_shield_cost(const struct impr_type *pimprove);
int impr_buy_gold_cost(const struct impr_type *pimprove, int shields_in_stock);
int impr_sell_gold(const struct impr_type *pimprove);

bool is_improvement_visible(const struct impr_type *pimprove);

bool is_great_wonder(const struct impr_type *pimprove);
bool is_small_wonder(const struct impr_type *pimprove);
bool is_wonder(const struct impr_type *pimprove);
bool is_improvement(const struct impr_type *pimprove);
bool is_special_improvement(const struct impr_type *pimprove);

bool can_sell_building(struct impr_type *pimprove);
bool can_city_sell_building(const struct city *pcity,
			    struct impr_type *pimprove);

/* Macros for struct packet_game_info::great_wonder_owners[]. */
#define WONDER_DESTROYED -2     /* Used as player id. */
#define WONDER_NOT_OWNED -1     /* User as player id. */
#define WONDER_OWNED(player_id) ((player_id) >= 0)

/* Macros for struct player::wonders[]. */
#define WONDER_NOT_BUILT 0      /* User as city id. */
#define WONDER_BUILT(city_id) ((city_id) != WONDER_NOT_BUILT)

void wonder_built(const struct city *pcity, const struct impr_type *pimprove);
void wonder_destroyed(const struct city *pcity,
                      const struct impr_type *pimprove);

bool wonder_is_built(const struct player *pplayer,
                     const struct impr_type *pimprove);
struct city *find_city_from_wonder(const struct player *pplayer,
                                   const struct impr_type *pimprove);

bool great_wonder_is_built(const struct impr_type *pimprove);
bool great_wonder_is_destroyed(const struct impr_type *pimprove);
bool great_wonder_is_available(const struct impr_type *pimprove);
struct city *find_city_from_great_wonder(const struct impr_type *pimprove);
struct player *great_wonder_owner(const struct impr_type *pimprove);

bool small_wonder_is_built(const struct player *pplayer,
                           const struct impr_type *pimprove);
struct city *find_city_from_small_wonder(const struct player *pplayer,
                                         const struct impr_type *pimprove);

/* player related improvement functions */
bool improvement_obsolete(const struct player *pplayer,
			  const struct impr_type *pimprove);
bool impr_provides_buildable_units(const struct player *pplayer,
                                   const struct impr_type *pimprove);

bool can_player_build_improvement_direct(const struct player *p,
					 struct impr_type *pimprove);
bool can_player_build_improvement_later(const struct player *p,
					struct impr_type *pimprove);
bool can_player_build_improvement_now(const struct player *p,
				      struct impr_type *pimprove);

/* General genus accessor routines */
enum impr_genus_id find_genus_by_rule_name(const char *s);

/* Initialization and iteration */
void improvements_init(void);
void improvements_free(void);

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
#endif  /* FC__IMPROVEMENT_H */
