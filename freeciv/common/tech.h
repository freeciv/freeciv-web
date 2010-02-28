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
#ifndef FC__TECH_H
#define FC__TECH_H

#include "shared.h"

#include "fc_types.h"

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
#define A_LAST MAX_NUM_ITEMS
#define A_UNSET (A_LAST-1)
#define A_FUTURE (A_LAST-2)
#define A_UNKNOWN (A_LAST-3)
#define A_LAST_REAL A_UNKNOWN

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
enum tech_flag_id {
  TF_BONUS_TECH, /* player gets extra tech if rearched first */
  TF_BRIDGE,    /* "Settler" unit types can build bridges over rivers */
  TF_RAILROAD,  /* "Settler" unit types can build rail roads */
  TF_POPULATION_POLLUTION_INC,  /* Increase the pollution factor created by population by one */
  TF_FARMLAND,  /* "Settler" unit types can build farmland */
  TF_BUILD_AIRBORNE, /* Player can build air units */
  TF_LAST
};

/* TECH_KNOWN is self-explanatory, TECH_PREREQS_KNOWN are those for which all 
 * requirements are fulfilled; all others (including those which can never 
 * be reached) are TECH_UNKNOWN */
enum tech_state {
  TECH_UNKNOWN = 0,
  TECH_PREREQS_KNOWN = 1,
  TECH_KNOWN = 2,
};

enum tech_req {
  AR_ONE = 0,
  AR_TWO = 1,
  AR_ROOT = 2,
  AR_SIZE
};

struct advance {
  Tech_type_id item_number;
  struct name_translation name;
  char graphic_str[MAX_LEN_NAME];	/* which named sprite to use */
  char graphic_alt[MAX_LEN_NAME];	/* alternate icon name */

  struct advance *require[AR_SIZE];
  unsigned int flags;
  char *helptext;

  /* 
   * Message displayed to the first player to get a bonus tech 
   */
  char *bonus_message;

  /* 
   * Cost of advance in bulbs as specified in ruleset. -1 means that
   * no value was set in ruleset. Server send this to client.
   */
  int preset_cost;

  /* 
   * Number of requirements this technology has _including_
   * itself. Precalculated at server then send to client.
   */
  int num_reqs;
};

BV_DEFINE(tech_vector, A_LAST);

struct player_research {
  /* The number of techs and future techs the player has
   * researched/acquired. */
  int techs_researched, future_tech;

  /* Invention being researched in. Valid values for researching are:
   *  - any existing tech (not A_NONE)
   *  - A_FUTURE
   *  - A_UNSET (indicates need for choosing new research)
   * For enemies, A_UNKNOWN is sent to the client, but not on server.
   *
   * bulbs_researched tracks how many bulbs have been accumulated toward
   * this research target. */
  Tech_type_id researching;
  int bulbs_researched;

  /* If the player changes his research target in a turn, he loses some or
   * all of the bulbs he's accumulated toward that target.  We save the
   * original info from the start of the turn so that if he changes back
   * he will get the bulbs back.
   *
   * Has the same values as researching, plus A_UNKNOWN used between turns
   * (not -1 anymore) for savegames. */
  Tech_type_id researching_saved;
  int bulbs_researching_saved;

  /* If the player completed a research this turn, this value is turned on
   * and changing targets may be done without penalty. */
  bool got_tech;

  struct {
    /* One of TECH_UNKNOWN, TECH_KNOWN or TECH_PREREQS_KNOWN. */
    enum tech_state state;

    /* 
     * required_techs, num_required_techs and bulbs_required are
     * cached values. Updated from build_required_techs (which is
     * called by player_research_update).
     */
    tech_vector required_techs;
    int num_required_techs, bulbs_required;
  } inventions[A_LAST];

  /* Tech goal (similar to worklists; when one tech is researched the next
   * tech toward the goal will be chosen).  May be A_NONE. */
  Tech_type_id tech_goal;

  /*
   * Cached values. Updated by player_research_update.
   */
  int num_known_tech_with_flag[TF_LAST];
};

/* General advance/technology accessor functions. */
Tech_type_id advance_count(void);
Tech_type_id advance_index(const struct advance *padvance);
Tech_type_id advance_number(const struct advance *padvance);

struct advance *advance_by_number(const Tech_type_id atype);

struct advance *valid_advance(struct advance *padvance);
struct advance *valid_advance_by_number(const Tech_type_id atype);

struct advance *find_advance_by_rule_name(const char *name);
struct advance *find_advance_by_translated_name(const char *name);

const char *advance_name_by_player(const struct player *pplayer,
				   Tech_type_id tech);
const char *advance_name_for_player(const struct player *pplayer,
				    Tech_type_id tech);
const char *advance_name_researching(const struct player *pplayer);

const char *advance_rule_name(const struct advance *padvance);
const char *advance_name_translation(struct advance *padvance);

/* General advance/technology flag accessor routines */
bool advance_has_flag(Tech_type_id tech, enum tech_flag_id flag);
enum tech_flag_id find_advance_flag_by_rule_name(const char *s);

/* FIXME: oddball function used in one place */
Tech_type_id find_advance_by_flag(Tech_type_id index, enum tech_flag_id flag);

/* Ancillary routines */
enum tech_state player_invention_state(const struct player *pplayer,
				       Tech_type_id tech);
enum tech_state player_invention_set(struct player *pplayer,
				     Tech_type_id tech,
				     enum tech_state value);
bool player_invention_reachable(const struct player *pplayer,
                                const Tech_type_id tech);

Tech_type_id player_research_step(const struct player *pplayer,
				  Tech_type_id goal);
void player_research_update(struct player *pplayer);

Tech_type_id advance_required(const Tech_type_id tech,
			      enum tech_req require);
struct advance *advance_requires(const struct advance *padvance,
				 enum tech_req require);

int total_bulbs_required(const struct player *pplayer);
int base_total_bulbs_required(const struct player *pplayer,
			      Tech_type_id tech);
bool techs_have_fixed_costs(void);

int num_unknown_techs_for_goal(const struct player *pplayer,
			       Tech_type_id goal);
int total_bulbs_required_for_goal(const struct player *pplayer,
				  Tech_type_id goal);
bool is_tech_a_req_for_goal(const struct player *pplayer,
			    Tech_type_id tech,
			    Tech_type_id goal);
bool is_future_tech(Tech_type_id tech);

void precalc_tech_data(void);

/* Initialization and iteration */
void player_research_init(struct player_research* research);

void techs_init(void);
void techs_free(void);

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
#endif  /* FC__TECH_H */
