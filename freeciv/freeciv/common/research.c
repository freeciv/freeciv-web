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
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "iterator.h"
#include "log.h"
#include "shared.h"
#include "string_vector.h"
#include "support.h"

/* common */
#include "fc_types.h"
#include "game.h"
#include "player.h"
#include "name_translation.h"
#include "team.h"
#include "tech.h"

#include "research.h"


struct research_iter {
  struct iterator vtable;
  int index;
};
#define RESEARCH_ITER(p) ((struct research_iter *) p)

struct research_player_iter {
  struct iterator vtable;
  union {
    struct player *pplayer;
    struct player_list_link *plink;
  };
};
#define RESEARCH_PLAYER_ITER(p) ((struct research_player_iter *) p)

static struct research research_array[MAX_NUM_PLAYER_SLOTS];

static struct name_translation advance_unset_name = NAME_INIT;
static struct name_translation advance_future_name = NAME_INIT;
static struct name_translation advance_unknown_name = NAME_INIT;

static struct strvec *future_rule_name;
static struct strvec *future_name_translation;

/************************************************************************//**
  Initializes all player research structure.
****************************************************************************/
void researches_init(void)
{
  int i;

  /* Ensure we have enough space for players or teams. */
  fc_assert(ARRAY_SIZE(research_array) >= team_slot_count());
  fc_assert(ARRAY_SIZE(research_array) >= player_slot_count());

  memset(research_array, 0, sizeof(research_array));
  for (i = 0; i < ARRAY_SIZE(research_array); i++) {
    research_array[i].tech_goal = A_UNSET;
    research_array[i].researching = A_UNSET;
    research_array[i].researching_saved = A_UNKNOWN;
    research_array[i].future_tech = 0;
    research_array[i].inventions[A_NONE].state = TECH_KNOWN;
    advance_index_iterate(A_FIRST, j) {
      research_array[i].inventions[j].bulbs_researched_saved = 0;
    } advance_index_iterate_end;
  }

  game.info.global_advances[A_NONE] = TRUE;

  /* Set technology names. */
  /* TRANS: "None" tech */
  name_set(&advance_unset_name, NULL, N_("?tech:None"));
  name_set(&advance_future_name, NULL, N_("Future Tech."));
  /* TRANS: "Unknown" advance/technology */
  name_set(&advance_unknown_name, NULL, N_("(Unknown)"));

  future_rule_name = strvec_new();
  future_name_translation = strvec_new();
}

/************************************************************************//**
  Free all resources allocated for the research system
****************************************************************************/
void researches_free(void)
{
  strvec_destroy(future_rule_name);
  strvec_destroy(future_name_translation);
}

/************************************************************************//**
  Returns the index of the research in the array.
****************************************************************************/
int research_number(const struct research *presearch)
{
  fc_assert_ret_val(NULL != presearch, -1);
  return presearch - research_array;
}

/************************************************************************//**
  Returns the research for the given index.
****************************************************************************/
struct research *research_by_number(int number)
{
  fc_assert_ret_val(0 <= number, NULL);
  fc_assert_ret_val(ARRAY_SIZE(research_array) > number, NULL);
  return &research_array[number];
}

/************************************************************************//**
  Returns the research structure associated with the player.
****************************************************************************/
struct research *research_get(const struct player *pplayer)
{
  if (NULL == pplayer) {
    /* Special case used at client side. */
    return NULL;
  } else if (game.info.team_pooled_research) {
    return &research_array[team_number(pplayer->team)];
  } else {
    return &research_array[player_number(pplayer)];
  }
}

/************************************************************************//**
  Returns the name of the research owner: a player name or a team name.
****************************************************************************/
const char *research_rule_name(const struct research *presearch)
{
  if (game.info.team_pooled_research) {
    return team_rule_name(team_by_number(research_number(presearch)));
  } else {
    return player_name(player_by_number(research_number(presearch)));
  }
}

/************************************************************************//**
  Returns the name of the research owner: a player name or a team name.
****************************************************************************/
const char *research_name_translation(const struct research *presearch)
{
  if (game.info.team_pooled_research) {
    return team_name_translation(team_by_number(research_number(presearch)));
  } else {
    return player_name(player_by_number(research_number(presearch)));
  }
}

/************************************************************************//**
  Set in 'buf' the name of the research owner. It may be either a nation
  plural name, or something like "members of team Red".
****************************************************************************/
int research_pretty_name(const struct research *presearch, char *buf,
                         size_t buf_len)
{
  const struct player *pplayer;

  if (game.info.team_pooled_research) {
    const struct team *pteam = team_by_number(research_number(presearch));

    if (1 != player_list_size(team_members(pteam))) {
      char buf2[buf_len];

      team_pretty_name(pteam, buf2, sizeof(buf2));
      /* TRANS: e.g. "members of team 1", or even "members of team Red". */
      return fc_snprintf(buf, buf_len, _("members of %s"), buf2);
    } else {
      pplayer = player_list_front(team_members(pteam));
    }
  } else {
    pplayer = player_by_number(research_number(presearch));
  }

  return fc_strlcpy(buf, nation_plural_for_player(pplayer), buf_len);
}

/************************************************************************//**
  Return the name translation for 'tech'. Utility for
  research_advance_rule_name() and research_advance_translated_name().
****************************************************************************/
static inline const struct name_translation *
research_advance_name(Tech_type_id tech)
{
  if (A_UNSET == tech) {
    return &advance_unset_name;
  } else if (A_FUTURE == tech) {
    return &advance_future_name;
  } else if (A_UNKNOWN == tech) {
    return &advance_unknown_name;
  } else {
    const struct advance *padvance = advance_by_number(tech);

    fc_assert_ret_val(NULL != padvance, NULL);
    return &padvance->name;
  }
}

/************************************************************************//**
  Set a new future tech name in the string vector, and return the string
  duplicate stored inside the vector.
****************************************************************************/
static const char *research_future_set_name(struct strvec *psv, int no,
                                            const char *new_name)
{
  if (strvec_size(psv) <= no) {
    /* Increase the size of the vector if needed. */
    strvec_reserve(psv, no + 1);
  }

  /* Set in vector. */
  strvec_set(psv, no, new_name);

  /* Return duplicate of 'new_name'. */
  return strvec_get(psv, no);
}

/************************************************************************//**
  Store the rule name of the given tech (including A_FUTURE) in 'buf'.
  'presearch' may be NULL.
  We don't return a static buffer because that would break anything that
  needed to work with more than one name at a time.
****************************************************************************/
const char *research_advance_rule_name(const struct research *presearch,
                                       Tech_type_id tech)
{
  if (A_FUTURE == tech && NULL != presearch) {
    const int no = presearch->future_tech;
    char buffer[256];
    const char *name;

    name = strvec_get(future_rule_name, no);
    if (name != NULL) {
      /* Already stored in string vector. */
      return name;
    }

    /* NB: 'presearch->future_tech == 0' means "Future Tech. 1". */
    fc_snprintf(buffer, sizeof(buffer), "%s %d",
                rule_name_get(&advance_future_name),
                no + 1);
    name = research_future_set_name(future_rule_name, no, buffer);
    fc_assert(name != NULL);
    fc_assert(name != buffer);
    return name;
  }

  return rule_name_get(research_advance_name(tech));
}

/************************************************************************//**
  Store the translated name of the given tech (including A_FUTURE) in 'buf'.
  'presearch' may be NULL.
  We don't return a static buffer because that would break anything that
  needed to work with more than one name at a time.
****************************************************************************/
const char *
research_advance_name_translation(const struct research *presearch,
                                  Tech_type_id tech)
{
  if (A_FUTURE == tech && NULL != presearch) {
    const int no = presearch->future_tech;
    char buffer[256];
    const char *name;

    name = strvec_get(future_name_translation, no);
    if (name != NULL) {
      /* Already stored in string vector. */
      return name;
    }

    /* NB: 'presearch->future_tech == 0' means "Future Tech. 1". */
    fc_snprintf(buffer, sizeof(buffer), _("Future Tech. %d"), no + 1);
    name = research_future_set_name(future_name_translation, no, buffer);
    fc_assert(name != NULL);
    fc_assert(name != buffer);
    return name;
  }

  return name_translation_get(research_advance_name(tech));
}

/************************************************************************//**
  Returns TRUE iff the requirement vector may become active against the
  given target.

  If may become active if all unchangeable requirements are active.
****************************************************************************/
static bool reqs_may_activate(const struct player *target_player,
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
                              const enum   req_problem_type prob_type)
{
  requirement_vector_iterate(reqs, preq) {
    if (is_req_unchanging(preq)
        && !is_req_active(target_player, other_player, target_city,
                          target_building, target_tile,
                          target_unit, target_unittype,
                          target_output, target_specialist, target_action,
                          preq, prob_type)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;
  return TRUE;
}

/************************************************************************//**
  Evaluates the legality of starting to research this tech according to
  reqs_eval() and the tech's research_reqs. Returns TRUE iff legal.

  The reqs_eval() argument evaluates the requirements. One variant may
  check the current situation while another may check potential future
  situations.

  Helper for research_update().
****************************************************************************/
static bool
research_allowed(const struct research *presearch,
                 Tech_type_id tech,
                 bool (*reqs_eval)(const struct player *tplr,
                                   const struct player *oplr,
                                   const struct city *tcity,
                                   const struct impr_type *tbld,
                                   const struct tile *ttile,
                                   const struct unit *tunit,
                                   const struct unit_type *tutype,
                                   const struct output_type *top,
                                   const struct specialist *tspe,
                                   const struct action *tact,
                                   const struct requirement_vector *reqs,
                                   const enum   req_problem_type ptype))
{
  struct advance *adv;

  adv = valid_advance_by_number(tech);

  if (adv == NULL) {
    /* Not a valid advance. */
    return FALSE;
  }

  research_players_iterate(presearch, pplayer) {
    if (reqs_eval(pplayer, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                  NULL, NULL, &(adv->research_reqs), RPT_CERTAIN)) {
      /* It is enough that one player that shares research is allowed to
       * research it.
       * Reasoning: Imagine a tech with that requires a nation in the
       * player range. If the requirement applies to all players sharing
       * research it will be illegal in research sharing games. To require
       * that the player that fulfills the requirement must order it to be
       * researched creates unnecessary bureaucracy. */
      return TRUE;
    }
  } research_players_iterate_end;

  return FALSE;
}

/************************************************************************//**
  Returns TRUE iff researching the given tech is allowed according to its
  research_reqs.

  Helper for research_update().
****************************************************************************/
#define research_is_allowed(presearch, tech)                              \
  research_allowed(presearch, tech, are_reqs_active)

/************************************************************************//**
  Returns TRUE iff researching the given tech may become allowed according
  to its research_reqs.

  Helper for research_get_reachable_rreqs().
****************************************************************************/
#define research_may_become_allowed(presearch, tech)                      \
  research_allowed(presearch, tech, reqs_may_activate)

/************************************************************************//**
  Returns TRUE iff the given tech is ever reachable by the players sharing
  the research as far as research_reqs are concerned.

  Helper for research_get_reachable().
****************************************************************************/
static bool research_get_reachable_rreqs(const struct research *presearch,
                                         Tech_type_id tech)
{
  bv_techs done;
  Tech_type_id techs[game.control.num_tech_types];
  enum tech_req req;
  int techs_num;
  int i;

  techs[0] = tech;
  BV_CLR_ALL(done);
  BV_SET(done, A_NONE);
  BV_SET(done, tech);
  techs_num = 1;

  /* Check that all recursive requirements have their research_reqs
   * in order. */
  for (i = 0; i < techs_num; i++) {
    if (presearch->inventions[techs[i]].state == TECH_KNOWN) {
      /* This tech is already reached. What is required to research it and
       * the techs it depends on is therefore irrelevant. */
      continue;
    }

    if (!research_may_become_allowed(presearch, techs[i])) {
      /* It will always be illegal to start researching this tech because
       * of unchanging requirements. Since it isn't already known and can't
       * be researched it must be unreachable. */
      return FALSE;
    }

    /* Check if required techs are research_reqs reachable. */
    for (req = 0; req < AR_SIZE; req++) {
      Tech_type_id req_tech = advance_required(techs[i], req);

      if (valid_advance_by_number(req_tech) == NULL) {
        return FALSE;
      } else if (!BV_ISSET(done, req_tech)) {
        fc_assert(techs_num < ARRAY_SIZE(techs));
        techs[techs_num] = req_tech;
        techs_num++;

        BV_SET(done, req_tech);
      }
    }
  }

  return TRUE;
}

/************************************************************************//**
  Returns TRUE iff the given tech is ever reachable by the players sharing
  the research by checking tech tree limitations.

  Helper for research_update().
****************************************************************************/
static bool research_get_reachable(const struct research *presearch,
                                   Tech_type_id tech)
{
  if (valid_advance_by_number(tech) == NULL) {
    return FALSE;
  } else {
    advance_root_req_iterate(advance_by_number(tech), proot) {
      if (advance_requires(proot, AR_ROOT) == proot) {
        /* This tech requires itself; it can only be reached by special
         * means (init_techs, lua script, ...).
         * If you already know it, you can "reach" it; if not, not. (This
         * case is needed for descendants of this tech.) */
        if (presearch->inventions[advance_number(proot)].state != TECH_KNOWN) {
          return FALSE;
        }
      } else {
        enum tech_req req;

        for (req = 0; req < AR_SIZE; req++) {
          if (valid_advance(advance_requires(proot, req)) == NULL) {
            return FALSE;
          }
        }
      }
    } advance_root_req_iterate_end;
  }

  /* Check research reqs reachability. */
  if (!research_get_reachable_rreqs(presearch, tech)) {
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Returns TRUE iff the players sharing 'presearch' already have got the
  knowledge of all root requirement technologies for 'tech' (without which
  it's impossible to gain 'tech').

  Helper for research_update().
****************************************************************************/
static bool research_get_root_reqs_known(const struct research *presearch,
                                         Tech_type_id tech)
{
  advance_root_req_iterate(advance_by_number(tech), proot) {
    if (presearch->inventions[advance_number(proot)].state != TECH_KNOWN) {
      return FALSE;
    }
  } advance_root_req_iterate_end;

  return TRUE;
}

/************************************************************************//**
  Mark as TECH_PREREQS_KNOWN each tech which is available, not known and
  which has all requirements fullfiled.

  Recalculate presearch->num_known_tech_with_flag
  Should always be called after research_invention_set().
****************************************************************************/
void research_update(struct research *presearch)
{
  enum tech_flag_id flag;
  int techs_researched;

  advance_index_iterate(A_FIRST, i) {
    enum tech_state state = presearch->inventions[i].state;
    bool root_reqs_known = TRUE;
    bool reachable = research_get_reachable(presearch, i);

    /* Finding if the root reqs of an unreachable tech isn't redundant.
     * A tech can be unreachable via research but have known root reqs
     * because of unfilfilled research_reqs. Unfulfilled research_reqs
     * doesn't prevent the player from aquiring the tech by other means. */
    root_reqs_known = research_get_root_reqs_known(presearch, i);

    if (reachable) {
      if (state != TECH_KNOWN) {
        /* Update state. */
        state = (root_reqs_known
                 && (presearch->inventions[advance_required(i, AR_ONE)].state
                     == TECH_KNOWN)
                 && (presearch->inventions[advance_required(i, AR_TWO)].state
                     == TECH_KNOWN)
                 && research_is_allowed(presearch, i)
                 ? TECH_PREREQS_KNOWN : TECH_UNKNOWN);
      }
    } else {
      fc_assert(state == TECH_UNKNOWN);
    }
    presearch->inventions[i].state = state;
    presearch->inventions[i].reachable = reachable;
    presearch->inventions[i].root_reqs_known = root_reqs_known;

    /* Updates required_techs, num_required_techs and bulbs_required. */
    BV_CLR_ALL(presearch->inventions[i].required_techs);
    presearch->inventions[i].num_required_techs = 0;
    presearch->inventions[i].bulbs_required = 0;

    if (!reachable || state == TECH_KNOWN) {
      continue;
    }

    techs_researched = presearch->techs_researched;
    advance_req_iterate(valid_advance_by_number(i), preq) {
      Tech_type_id j = advance_number(preq);

      if (TECH_KNOWN == research_invention_state(presearch, j)) {
        continue;
      }

      BV_SET(presearch->inventions[i].required_techs, j);
      presearch->inventions[i].num_required_techs++;
      presearch->inventions[i].bulbs_required +=
          research_total_bulbs_required(presearch, j, FALSE);
      /* This is needed to get a correct result for the
       * research_total_bulbs_required() call when
       * game.info.game.info.tech_cost_style is TECH_COST_CIV1CIV2. */
      presearch->techs_researched++;
    } advance_req_iterate_end;
    presearch->techs_researched = techs_researched;
  } advance_index_iterate_end;

#ifdef FREECIV_DEBUG
  advance_index_iterate(A_FIRST, i) {
    char buf[advance_count() + 1];

    advance_index_iterate(A_NONE, j) {
      if (BV_ISSET(presearch->inventions[i].required_techs, j)) {
        buf[j] = '1';
      } else {
        buf[j] = '0';
      }
    } advance_index_iterate_end;
    buf[advance_count()] = '\0';

    log_debug("%s: [%3d] %-25s => %s%s%s",
              research_rule_name(presearch),
              i,
              advance_rule_name(advance_by_number(i)),
              tech_state_name(research_invention_state(presearch, i)),
              presearch->inventions[i].reachable ? "" : " [unrechable]",
              presearch->inventions[i].root_reqs_known
              ? "" : " [root reqs aren't known]");
    log_debug("%s: [%3d] %s", research_rule_name(presearch), i, buf);
  } advance_index_iterate_end;
#endif /* FREECIV_DEBUG */

  for (flag = 0; flag <= tech_flag_id_max(); flag++) {
    /* Iterate over all possible tech flags (0..max). */
    presearch->num_known_tech_with_flag[flag] = 0;

    advance_index_iterate(A_NONE, i) {
      if (TECH_KNOWN == research_invention_state(presearch, i)
          && advance_has_flag(i, flag)) {
        presearch->num_known_tech_with_flag[flag]++;
      }
    } advance_index_iterate_end;
  }
}

/************************************************************************//**
  Returns state of the tech for current research.
  This can be: TECH_KNOWN, TECH_UNKNOWN, or TECH_PREREQS_KNOWN
  Should be called with existing techs.

  If 'presearch' is NULL this checks whether any player knows the tech
  (used by the client).
****************************************************************************/
enum tech_state research_invention_state(const struct research *presearch,
                                         Tech_type_id tech)
{
  fc_assert_ret_val(NULL != valid_advance_by_number(tech), -1);

  if (NULL != presearch) {
    return presearch->inventions[tech].state;
  } else if (game.info.global_advances[tech]) {
    return TECH_KNOWN;
  } else {
    return TECH_UNKNOWN;
  }
}

/************************************************************************//**
  Set research knowledge about tech to given state.
****************************************************************************/
enum tech_state research_invention_set(struct research *presearch,
                                       Tech_type_id tech,
                                       enum tech_state value)
{
  enum tech_state old;

  fc_assert_ret_val(NULL != valid_advance_by_number(tech), -1);

  old = presearch->inventions[tech].state;
  if (old == value) {
    return old;
  }
  presearch->inventions[tech].state = value;

  if (value == TECH_KNOWN) {
    if (!game.info.global_advances[tech]) {
      game.info.global_advances[tech] = TRUE;
      game.info.global_advance_count++;
    }
  }

  return old;
}

/************************************************************************//**
  Returns TRUE iff the given tech is ever reachable via research by the
  players sharing the research by checking tech tree limitations.

  'presearch' may be NULL in which case a simplified result is returned
  (used by the client).
****************************************************************************/
bool research_invention_reachable(const struct research *presearch,
                                  const Tech_type_id tech)
{
  if (valid_advance_by_number(tech) == NULL) {
    return FALSE;
  } else if (presearch != NULL) {
    return presearch->inventions[tech].reachable;
  } else {
    researches_iterate(research_iter) {
      if (research_iter->inventions[tech].reachable) {
        return TRUE;
      }
    } researches_iterate_end;

    return FALSE;
  }
}

/************************************************************************//**
  Returns TRUE iff the given tech can be given to the players sharing the
  research immediately.

  If allow_holes is TRUE, any tech with known root reqs is ok. If it's
  FALSE, getting the tech must not leave holes to the known techs tree.
****************************************************************************/
bool research_invention_gettable(const struct research *presearch,
                                 const Tech_type_id tech,
                                 bool allow_holes)
{
  if (valid_advance_by_number(tech) == NULL) {
    return FALSE;
  } else if (presearch != NULL) {
    return (allow_holes
            ? presearch->inventions[tech].root_reqs_known
            : presearch->inventions[tech].state == TECH_PREREQS_KNOWN);
  } else {
    researches_iterate(research_iter) {
      if (allow_holes
          ? research_iter->inventions[tech].root_reqs_known
          : research_iter->inventions[tech].state == TECH_PREREQS_KNOWN) {
        return TRUE;
      }
    } researches_iterate_end;

    return FALSE;
  }
}

/************************************************************************//**
  Return the next tech we should research to advance towards our goal.
  Returns A_UNSET if nothing is available or the goal is already known.
****************************************************************************/
Tech_type_id research_goal_step(const struct research *presearch,
                                Tech_type_id goal)
{
  const struct advance *pgoal = valid_advance_by_number(goal);

  if (NULL == pgoal
      || !research_invention_reachable(presearch, goal)) {
    return A_UNSET;
  }

  advance_req_iterate(pgoal, preq) {
    switch (research_invention_state(presearch, advance_number(preq))) {
    case TECH_PREREQS_KNOWN:
      return advance_number(preq);
    case TECH_KNOWN:
    case TECH_UNKNOWN:
       break;
    };
  } advance_req_iterate_end;
  return A_UNSET;
}

/************************************************************************//**
  Returns the number of technologies the player need to research to get
  the goal technology. This includes the goal technology. Technologies
  are only counted once.

  'presearch' may be NULL in which case it will returns the total number
  of technologies needed for reaching the goal.
****************************************************************************/
int research_goal_unknown_techs(const struct research *presearch,
                                Tech_type_id goal)
{
  const struct advance *pgoal = valid_advance_by_number(goal);

  if (NULL == pgoal) {
    return 0;
  } else if (NULL != presearch) {
    return presearch->inventions[goal].num_required_techs;
  } else {
    return pgoal->num_reqs;
  }
}

/************************************************************************//**
  Function to determine cost (in bulbs) of reaching goal technology.
  These costs _include_ the cost for researching the goal technology
  itself.

  'presearch' may be NULL in which case it will returns the total number
  of bulbs needed for reaching the goal.
****************************************************************************/
int research_goal_bulbs_required(const struct research *presearch,
                                 Tech_type_id goal)
{
  const struct advance *pgoal = valid_advance_by_number(goal);

  if (NULL == pgoal) {
    return 0;
  } else if (NULL != presearch) {
    return presearch->inventions[goal].bulbs_required;
  } else if (game.info.tech_cost_style == TECH_COST_CIV1CIV2) {
     return game.info.base_tech_cost * pgoal->num_reqs
            * (pgoal->num_reqs + 1) / 2;
  } else {
    int bulbs_required = 0;

    advance_req_iterate(pgoal, preq) {
      bulbs_required += preq->cost;
    } advance_req_iterate_end;
    return bulbs_required;
  }
}

/************************************************************************//**
  Returns if the given tech has to be researched to reach the goal. The
  goal itself isn't a requirement of itself.

  'presearch' may be NULL.
****************************************************************************/
bool research_goal_tech_req(const struct research *presearch,
                            Tech_type_id goal, Tech_type_id tech)
{
  const struct advance *pgoal, *ptech;

  if (tech == goal
      || NULL == (pgoal = valid_advance_by_number(goal))
      || NULL == (ptech = valid_advance_by_number(tech))) {
    return FALSE;
  } else if (NULL != presearch) {
    return BV_ISSET(presearch->inventions[goal].required_techs, tech);
  } else {
    advance_req_iterate(pgoal, preq) {
      if (preq == ptech) {
        return TRUE;
      }
    } advance_req_iterate_end;
    return FALSE;
  }
}

/************************************************************************//**
  Function to determine cost for technology.  The equation is determined
  from game.info.tech_cost_style and game.info.tech_leakage.

  tech_cost_style:
  TECH_COST_CIV1CIV2: Civ (I|II) style. Every new tech add base_tech_cost to
                      cost of next tech.
  TECH_COST_CLASSIC: Cost of technology is:
                       base_tech_cost * (1 + reqs) * sqrt(1 + reqs) / 2
                     where reqs == number of requirement for tech, counted
                     recursively.
  TECH_COST_CLASSIC_PRESET: Cost are read from tech.ruleset. Missing costs
                            are generated by style "Classic".
  TECH_COST_EXPERIMENTAL: Cost of technology is:
                            base_tech_cost * (reqs^2
                                              / (1 + sqrt(sqrt(reqs + 1)))
                                              - 0.5)
                          where reqs == number of requirement for tech,
                          counted recursively.
  TECH_COST_EXPERIMENTAL_PRESET: Cost are read from tech.ruleset. Missing
                                 costs are generated by style "Experimental".

  tech_leakage:
  TECH_LEAKAGE_NONE: No reduction of the technology cost.
  TECH_LEAKAGE_EMBASSIES: Technology cost is reduced depending on the number
                          of players which already know the tech and you have
                          an embassy with.
  TECH_LEAKAGE_PLAYERS: Technology cost is reduced depending on the number of
                        all players (human, AI and barbarians) which already
                        know the tech.
  TECH_LEAKAGE_NO_BARBS: Technology cost is reduced depending on the number
                         of normal players (human and AI) which already know
                         the tech.

  At the end we multiply by the sciencebox value, as a percentage.  The
  cost can never be less than 1.

  'presearch' may be NULL in which case a simplified result is returned
  (used by client and manual code).
****************************************************************************/
int research_total_bulbs_required(const struct research *presearch,
                                  Tech_type_id tech, bool loss_value)
{
  enum tech_cost_style tech_cost_style = game.info.tech_cost_style;
  int members;
  double base_cost, total_cost;
  double leak = 0.0;

  if (!loss_value
      && NULL != presearch
      && !is_future_tech(tech)
      && research_invention_state(presearch, tech) == TECH_KNOWN) {
    /* A non-future tech which is already known costs nothing. */
    return 0;
  }

  if (is_future_tech(tech)) {
    /* Future techs use style TECH_COST_CIV1CIV2. */
    tech_cost_style = TECH_COST_CIV1CIV2;
  }

  fc_assert_msg(tech_cost_style_is_valid(tech_cost_style),
                "Invalid tech_cost_style %d", tech_cost_style);
  base_cost = 0.0;
  switch (tech_cost_style) {
  case TECH_COST_CIV1CIV2:
    if (NULL != presearch) {
      base_cost = game.info.base_tech_cost * presearch->techs_researched;
      break;
    }

  case TECH_COST_CLASSIC:
  case TECH_COST_CLASSIC_PRESET:
  case TECH_COST_EXPERIMENTAL:
  case TECH_COST_EXPERIMENTAL_PRESET:
  case TECH_COST_LINEAR:
    {
      const struct advance *padvance = valid_advance_by_number(tech);

      if (NULL != padvance) {
        base_cost = padvance->cost;
      } else {
        fc_assert(NULL != padvance); /* Always fails. */
      }
    }
    break;
  }

  total_cost = 0.0;
  members = 0;
  research_players_iterate(presearch, pplayer) {
    members++;
    total_cost += (base_cost
                   * get_player_bonus(pplayer, EFT_TECH_COST_FACTOR));
  } research_players_iterate_end;
  if (0 == members) {
    /* There is no more alive players for this research, no need to apply
     * complicated modifiers. */
    return base_cost * (double) game.info.sciencebox / 100.0;
  }
  base_cost = total_cost / members;

  fc_assert_msg(tech_leakage_style_is_valid(game.info.tech_leakage),
                "Invalid tech_leakage %d", game.info.tech_leakage);
  switch (game.info.tech_leakage) {
  case TECH_LEAKAGE_NONE:
    /* no change */
    break;

  case TECH_LEAKAGE_EMBASSIES:
    {
      int players = 0, players_with_tech_and_embassy = 0;

      players_iterate_alive(aplayer) {
        const struct research *aresearch = research_get(aplayer);

        players++;
        if (aresearch == presearch
            || (A_FUTURE == tech
                ? aresearch->future_tech <= presearch->future_tech
                : TECH_KNOWN != research_invention_state(aresearch, tech))) {
          continue;
        }

        research_players_iterate(presearch, pplayer) {
          if (player_has_embassy(pplayer, aplayer)) {
            players_with_tech_and_embassy++;
            break;
          }
        } research_players_iterate_end;
      } players_iterate_alive_end;

      fc_assert_ret_val(0 < players, base_cost);
      fc_assert(players >= players_with_tech_and_embassy);
      leak = base_cost * players_with_tech_and_embassy
             * game.info.tech_leak_pct / players / 100;
    }
    break;

  case TECH_LEAKAGE_PLAYERS:
    {
      int players = 0, players_with_tech = 0;

      players_iterate_alive(aplayer) {
        players++;
        if (A_FUTURE == tech
            ? research_get(aplayer)->future_tech > presearch->future_tech
            : TECH_KNOWN == research_invention_state(research_get(aplayer),
                                                     tech)) {
          players_with_tech++;
        }
      } players_iterate_alive_end;

      fc_assert_ret_val(0 < players, base_cost);
      fc_assert(players >= players_with_tech);
      leak = base_cost * players_with_tech * game.info.tech_leak_pct
             / players / 100;
    }
    break;

  case TECH_LEAKAGE_NO_BARBS:
    {
      int players = 0, players_with_tech = 0;

      players_iterate_alive(aplayer) {
        if (is_barbarian(aplayer)) {
          continue;
        }
        players++;
        if (A_FUTURE == tech
            ? research_get(aplayer)->future_tech > presearch->future_tech
            : TECH_KNOWN == research_invention_state(research_get(aplayer),
                                                     tech)) {
          players_with_tech++;
        }
      } players_iterate_alive_end;

      fc_assert_ret_val(0 < players, base_cost);
      fc_assert(players >= players_with_tech);
      leak = base_cost * players_with_tech * game.info.tech_leak_pct
             / players / 100;
    }
    break;
  }

  if (leak > base_cost) {
    base_cost = 0.0;
  } else {
    base_cost -= leak;
  }

  /* Assign a science penalty to the AI at easier skill levels. This code
   * can also be adopted to create an extra-hard AI skill level where the AI
   * gets science benefits. */

  total_cost = 0.0;
  research_players_iterate(presearch, pplayer) {
    if (is_ai(pplayer)) {
      fc_assert(0 < pplayer->ai_common.science_cost);
      total_cost += base_cost * pplayer->ai_common.science_cost / 100.0;
    } else {
      total_cost += base_cost;
    }
  } research_players_iterate_end;
  base_cost = total_cost / members;

  base_cost *= (double) game.info.sciencebox / 100.0;

  return MAX(base_cost, 1);
}


/************************************************************************//**
  Calculate the bulb upkeep needed for all techs of a player. See also
  research_total_bulbs_required().
****************************************************************************/
int player_tech_upkeep(const struct player *pplayer)
{
  const struct research *presearch = research_get(pplayer);
  int f = presearch->future_tech, t = presearch->techs_researched;
  double tech_upkeep = 0.0;
  double total_research_factor;
  int members;

  if (TECH_UPKEEP_NONE == game.info.tech_upkeep_style) {
    return 0;
  }

  total_research_factor = 0.0;
  members = 0;
  research_players_iterate(presearch, contributor) {
    total_research_factor += (get_player_bonus(contributor, EFT_TECH_COST_FACTOR)
                              + (is_ai(contributor)
                                 ? contributor->ai_common.science_cost / 100.0
                                 : 1));
    members++;
  } research_players_iterate_end;
  if (0 == members) {
    /* No player still alive. */
    return 0;
  }

  /* Upkeep cost for 'normal' techs (t). */
  fc_assert_msg(tech_cost_style_is_valid(game.info.tech_cost_style),
                "Invalid tech_cost_style %d", game.info.tech_cost_style);
  switch (game.info.tech_cost_style) {
  case TECH_COST_CIV1CIV2:
    /* sum_1^t x = t * (t + 1) / 2 */
    tech_upkeep += game.info.base_tech_cost * t * (t + 1) / 2;
    break;
  case TECH_COST_CLASSIC:
  case TECH_COST_CLASSIC_PRESET:
  case TECH_COST_EXPERIMENTAL:
  case TECH_COST_EXPERIMENTAL_PRESET:
  case TECH_COST_LINEAR:
    advance_iterate(A_FIRST, padvance) {
      if (TECH_KNOWN == research_invention_state(presearch,
                                                 advance_number(padvance))) {
        tech_upkeep += padvance->cost;
      }
    } advance_iterate_end;
    if (0 < f) {
      /* Upkeep cost for future techs (f) are calculated using style 0:
       * sum_t^(t+f) x = (f * (2 * t + f + 1) + 2 * t) / 2 */
      tech_upkeep += (double) (game.info.base_tech_cost
                               * (f * (2 * t + f + 1) + 2 * t) / 2);
    }
    break;
  }

  tech_upkeep *= total_research_factor / members;
  tech_upkeep *= (double) game.info.sciencebox / 100.0;
  /* We only want to calculate the upkeep part of one player, not the
   * whole team! */
  tech_upkeep /= members;
  tech_upkeep /= game.info.tech_upkeep_divider;

  switch (game.info.tech_upkeep_style) {
  case TECH_UPKEEP_BASIC:
    tech_upkeep -= get_player_bonus(pplayer, EFT_TECH_UPKEEP_FREE);
    break;
  case TECH_UPKEEP_PER_CITY:
    tech_upkeep -= get_player_bonus(pplayer, EFT_TECH_UPKEEP_FREE);
    tech_upkeep *= city_list_size(pplayer->cities);
    break;
  case TECH_UPKEEP_NONE:
    fc_assert(game.info.tech_upkeep_style != TECH_UPKEEP_NONE);
    tech_upkeep = 0.0;
  }

  if (0.0 > tech_upkeep) {
    tech_upkeep = 0.0;
  }

  log_debug("[%s (%d)] tech upkeep: %d", player_name(pplayer),
            player_number(pplayer), (int) tech_upkeep);
  return (int) tech_upkeep;
}


/************************************************************************//**
  Returns the real size of the player research iterator.
****************************************************************************/
size_t research_iter_sizeof(void)
{
  return sizeof(struct research_iter);
}

/************************************************************************//**
  Returns the research structure pointed by the iterator.
****************************************************************************/
static void *research_iter_get(const struct iterator *it)
{
  return &research_array[RESEARCH_ITER(it)->index];
}

/************************************************************************//**
  Jump to next team research structure.
****************************************************************************/
static void research_iter_team_next(struct iterator *it)
{
  struct research_iter *rit = RESEARCH_ITER(it);

  if (team_slots_initialised()) {
    do {
      rit->index++;
    } while (rit->index < ARRAY_SIZE(research_array) && !it->valid(it));
  }
}

/************************************************************************//**
  Returns FALSE if there is no valid team at current index.
****************************************************************************/
static bool research_iter_team_valid(const struct iterator *it)
{
  struct research_iter *rit = RESEARCH_ITER(it);

  return (0 <= rit->index
          && ARRAY_SIZE(research_array) > rit->index
          && NULL != team_by_number(rit->index));
}

/************************************************************************//**
  Jump to next player research structure.
****************************************************************************/
static void research_iter_player_next(struct iterator *it)
{
  struct research_iter *rit = RESEARCH_ITER(it);

  if (player_slots_initialised()) {
    do {
      rit->index++;
    } while (rit->index < ARRAY_SIZE(research_array) && !it->valid(it));
  }
}

/************************************************************************//**
  Returns FALSE if there is no valid player at current index.
****************************************************************************/
static bool research_iter_player_valid(const struct iterator *it)
{
  struct research_iter *rit = RESEARCH_ITER(it);

  return (0 <= rit->index
          && ARRAY_SIZE(research_array) > rit->index
          && NULL != player_by_number(rit->index));
}

/************************************************************************//**
  Initializes a player research iterator.
****************************************************************************/
struct iterator *research_iter_init(struct research_iter *it)
{
  struct iterator *base = ITERATOR(it);

  base->get = research_iter_get;
  it->index = -1;

  if (game.info.team_pooled_research) {
    base->next = research_iter_team_next;
    base->valid = research_iter_team_valid;
  } else {
    base->next = research_iter_player_next;
    base->valid = research_iter_player_valid;
  }

  base->next(base);
  return base;
}

/************************************************************************//**
  Returns the real size of the research player iterator.
****************************************************************************/
size_t research_player_iter_sizeof(void)
{
  return sizeof(struct research_player_iter);
}

/************************************************************************//**
  Returns whether the iterator is currently at a valid state.
****************************************************************************/
static inline bool research_player_iter_valid_state(struct iterator *it)
{
  const struct player *pplayer = iterator_get(it);

  return (NULL == pplayer || pplayer->is_alive);
}

/************************************************************************//**
  Returns player of the iterator.
****************************************************************************/
static void *research_player_iter_pooled_get(const struct iterator *it)
{
  return player_list_link_data(RESEARCH_PLAYER_ITER(it)->plink);
}

/************************************************************************//**
  Returns the next player sharing the research.
****************************************************************************/
static void research_player_iter_pooled_next(struct iterator *it)
{
  struct research_player_iter *rpit = RESEARCH_PLAYER_ITER(it);

  do {
    rpit->plink = player_list_link_next(rpit->plink);
  } while (!research_player_iter_valid_state(it));
}

/************************************************************************//**
  Returns whether the iterate is valid.
****************************************************************************/
static bool research_player_iter_pooled_valid(const struct iterator *it)
{
  return NULL != RESEARCH_PLAYER_ITER(it)->plink;
}

/************************************************************************//**
  Returns player of the iterator.
****************************************************************************/
static void *research_player_iter_not_pooled_get(const struct iterator *it)
{
  return RESEARCH_PLAYER_ITER(it)->pplayer;
}

/************************************************************************//**
  Invalidate the iterator.
****************************************************************************/
static void research_player_iter_not_pooled_next(struct iterator *it)
{
  RESEARCH_PLAYER_ITER(it)->pplayer = NULL;
}

/************************************************************************//**
  Returns whether the iterate is valid.
****************************************************************************/
static bool research_player_iter_not_pooled_valid(const struct iterator *it)
{
  return NULL != RESEARCH_PLAYER_ITER(it)->pplayer;
}

/************************************************************************//**
  Initializes a research player iterator.
****************************************************************************/
struct iterator *research_player_iter_init(struct research_player_iter *it,
                                           const struct research *presearch)
{
  struct iterator *base = ITERATOR(it);

  if (game.info.team_pooled_research && NULL != presearch) {
    base->get = research_player_iter_pooled_get;
    base->next = research_player_iter_pooled_next;
    base->valid = research_player_iter_pooled_valid;
    it->plink = player_list_head(team_members(team_by_number(research_number
                                                             (presearch))));
  } else {
    base->get = research_player_iter_not_pooled_get;
    base->next = research_player_iter_not_pooled_next;
    base->valid = research_player_iter_not_pooled_valid;
    it->pplayer = (NULL != presearch
                   ? player_by_number(research_number(presearch)) : NULL);
  }

  /* Ensure we have consistent data. */
  if (!research_player_iter_valid_state(base)) {
    iterator_next(base);
  }

  return base;
}
