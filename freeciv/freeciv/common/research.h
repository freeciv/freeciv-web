/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/
#ifndef FC__RESEARCH_H
#define FC__RESEARCH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "iterator.h"
#include "support.h"

/* common */
#include "fc_types.h"
#include "tech.h"

/* TECH_KNOWN is self-explanatory, TECH_PREREQS_KNOWN are those for which all
 * requirements are fulfilled; all others (including those which can never
 * be reached) are TECH_UNKNOWN. */
#define SPECENUM_NAME tech_state
/* TECH_UNKNOWN must be 0 as the code does no special initialisation after
 * memset(0), See researches_init(). */
#define SPECENUM_VALUE0 TECH_UNKNOWN
#define SPECENUM_VALUE1 TECH_PREREQS_KNOWN
#define SPECENUM_VALUE2 TECH_KNOWN
#include "specenum_gen.h"

struct research {
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
  /* The same as got_tech but flipped back in choose_tech() */
  bool got_tech_multi;

  struct research_invention {
    /* One of TECH_UNKNOWN, TECH_KNOWN or TECH_PREREQS_KNOWN. */
    enum tech_state state;

    /* Following fields are cached values. They are updated by
     * research_update()). */
    bool reachable;
    bool root_reqs_known;
    bv_techs required_techs;
    int num_required_techs, bulbs_required, bulbs_researched_saved;
  } inventions[A_LAST];

  /* Tech goal (similar to worklists; when one tech is researched the next
   * tech toward the goal will be chosen).  May be A_NONE. */
  Tech_type_id tech_goal;

  /*
   * Cached values. Updated by research_update().
   */
  int num_known_tech_with_flag[TF_COUNT];

  union {
    /* Add server side when needed */

    struct {
      /* Only used at the client (the server is omniscient; ./client/). */

      int researching_cost;
      int total_bulbs_prod;
    } client;
  };
};

/* Common functions. */
void researches_init(void);
void researches_free(void);

int research_number(const struct research *presearch);
const char *research_rule_name(const struct research *presearch);
const char *research_name_translation(const struct research *presearch);
int research_pretty_name(const struct research *presearch, char *buf,
                         size_t buf_len);

struct research *research_by_number(int number);
struct research *research_get(const struct player *pplayer);

const char *research_advance_rule_name(const struct research *presearch,
                                       Tech_type_id tech);
const char *
research_advance_name_translation(const struct research *presearch,
                                  Tech_type_id tech);

/* Ancillary routines */
void research_update(struct research *presearch);

enum tech_state research_invention_state(const struct research *presearch,
                                         Tech_type_id tech);
enum tech_state research_invention_set(struct research *presearch,
                                       Tech_type_id tech,
                                       enum tech_state value);
bool research_invention_reachable(const struct research *presearch,
                                  const Tech_type_id tech);
bool research_invention_gettable(const struct research *presearch,
                                 const Tech_type_id tech,
                                 bool allow_holes);

Tech_type_id research_goal_step(const struct research *presearch,
                                Tech_type_id goal);
int research_goal_unknown_techs(const struct research *presearch,
                                Tech_type_id goal);
int research_goal_bulbs_required(const struct research *presearch,
                                 Tech_type_id goal);
bool research_goal_tech_req(const struct research *presearch,
                            Tech_type_id goal, Tech_type_id tech);

int research_total_bulbs_required(const struct research *presearch,
                                  Tech_type_id tech, bool loss_value);

int player_tech_upkeep(const struct player *pplayer);

/* Iterating utilities. */
struct research_iter;

size_t research_iter_sizeof(void);
struct iterator *research_iter_init(struct research_iter *it);

#define researches_iterate(_presearch)                                      \
  generic_iterate(struct research_iter, struct research *,                  \
                  _presearch, research_iter_sizeof, research_iter_init)
#define researches_iterate_end generic_iterate_end

struct research_player_iter;

size_t research_player_iter_sizeof(void);
struct iterator *research_player_iter_init(struct research_player_iter *it,
                                           const struct research *presearch);

#define research_players_iterate(_presearch, _pplayer)                      \
  generic_iterate(struct research_player_iter, struct player *, _pplayer,   \
                  research_player_iter_sizeof, research_player_iter_init,   \
                  _presearch)
#define research_players_iterate_end generic_iterate_end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__RESEARCH_H */
