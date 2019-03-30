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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "astring.h"
#include "bitvector.h"
#include "rand.h"

/* common */
#include "player.h"

/* ai */
#include "handicaps.h"

#include "difficulty.h"

static bv_handicap handicap_of_skill_level(enum ai_level level);
static int fuzzy_of_skill_level(enum ai_level level);
static int science_cost_of_skill_level(enum ai_level level);
static int expansionism_of_skill_level(enum ai_level level);

/**********************************************************************//**
  Set an AI level and related quantities, with no feedback.
**************************************************************************/
void set_ai_level_directer(struct player *pplayer, enum ai_level level)
{
  handicaps_set(pplayer, handicap_of_skill_level(level));
  pplayer->ai_common.fuzzy = fuzzy_of_skill_level(level);
  pplayer->ai_common.expand = expansionism_of_skill_level(level);
  pplayer->ai_common.science_cost = science_cost_of_skill_level(level);
  pplayer->ai_common.skill_level = level;
}

/**********************************************************************//**
  Returns handicap bitvector for given AI skill level
**************************************************************************/
static bv_handicap handicap_of_skill_level(enum ai_level level)
{
  bv_handicap handicap;

  fc_assert(ai_level_is_valid(level));

  BV_CLR_ALL(handicap);

  switch (level) {
   case AI_LEVEL_AWAY:
     BV_SET(handicap, H_AWAY);
     BV_SET(handicap, H_FOG);
     BV_SET(handicap, H_MAP);
     BV_SET(handicap, H_RATES);
     BV_SET(handicap, H_TARGETS);
     BV_SET(handicap, H_HUTS);
     BV_SET(handicap, H_REVOLUTION);
#ifdef FREECIV_WEB
     BV_SET(handicap, H_ASSESS_DANGER_LIMITED);
#endif
     BV_SET(handicap, H_PRODCHGPEN);
     break;
   case AI_LEVEL_NOVICE:
   case AI_LEVEL_HANDICAPPED:
     BV_SET(handicap, H_RATES);
     BV_SET(handicap, H_TARGETS);
     BV_SET(handicap, H_HUTS);
     BV_SET(handicap, H_NOPLANES);
     BV_SET(handicap, H_DIPLOMAT);
     BV_SET(handicap, H_LIMITEDHUTS);
     BV_SET(handicap, H_DEFENSIVE);
     BV_SET(handicap, H_DIPLOMACY);
     BV_SET(handicap, H_REVOLUTION);
     BV_SET(handicap, H_EXPANSION);
     BV_SET(handicap, H_DANGER);
     BV_SET(handicap, H_CEASEFIRE);
     BV_SET(handicap, H_NOBRIBE_WF);
#ifdef FREECIV_WEB
     BV_SET(handicap, H_ASSESS_DANGER_LIMITED);
#endif
     BV_SET(handicap, H_PRODCHGPEN);
     break;
   case AI_LEVEL_EASY:
     BV_SET(handicap, H_RATES);
     BV_SET(handicap, H_TARGETS);
     BV_SET(handicap, H_HUTS);
     BV_SET(handicap, H_NOPLANES);
     BV_SET(handicap, H_DIPLOMAT);
     BV_SET(handicap, H_LIMITEDHUTS);
     BV_SET(handicap, H_DEFENSIVE);
     BV_SET(handicap, H_DIPLOMACY);
     BV_SET(handicap, H_REVOLUTION);
     BV_SET(handicap, H_EXPANSION);
     BV_SET(handicap, H_CEASEFIRE);
#ifdef FREECIV_WEB
     BV_SET(handicap, H_ASSESS_DANGER_LIMITED);
#endif
     BV_SET(handicap, H_NOBRIBE_WF);
     break;
   case AI_LEVEL_NORMAL:
     BV_SET(handicap, H_RATES);
     BV_SET(handicap, H_TARGETS);
     BV_SET(handicap, H_HUTS);
     BV_SET(handicap, H_DIPLOMAT);
     BV_SET(handicap, H_CEASEFIRE);
     BV_SET(handicap, H_NOBRIBE_WF);
     break;

#ifdef FREECIV_DEBUG
   case AI_LEVEL_EXPERIMENTAL:
     BV_SET(handicap, H_EXPERIMENTAL);
     break;
#endif /* FREECIV_DEBUG */

   case AI_LEVEL_CHEATING:
     BV_SET(handicap, H_RATES);
     break;
   case AI_LEVEL_HARD:
     /* No handicaps */
     break;
  case AI_LEVEL_COUNT:
    fc_assert(level != AI_LEVEL_COUNT);
    break;
  }

  return handicap;
}

/**********************************************************************//**
  Return the AI fuzziness (0 to 1000) corresponding to a given skill
  level (1 to 10).  See ai_fuzzy() in common/player.c
**************************************************************************/
static int fuzzy_of_skill_level(enum ai_level level)
{
  fc_assert(ai_level_is_valid(level));

  switch(level) {
  case AI_LEVEL_AWAY:
    return 0;
  case AI_LEVEL_HANDICAPPED:
  case AI_LEVEL_NOVICE:
    return 400;
  case AI_LEVEL_EASY:
    return 300;
  case AI_LEVEL_NORMAL:
  case AI_LEVEL_HARD:
  case AI_LEVEL_CHEATING:
#ifdef FREECIV_DEBUG
  case AI_LEVEL_EXPERIMENTAL:
#endif /* FREECIV_DEBUG */
    return 0;
  case AI_LEVEL_COUNT:
    fc_assert(level != AI_LEVEL_COUNT);
    return 0;
  }

  return 0;
}

/**********************************************************************//**
  Return the AI's science development cost; a science development cost of 100
  means that the AI develops science at the same speed as a human; a science
  development cost of 200 means that the AI develops science at half the speed
  of a human, and a science development cost of 50 means that the AI develops
  science twice as fast as the human.
**************************************************************************/
static int science_cost_of_skill_level(enum ai_level level)
{
  fc_assert(ai_level_is_valid(level));

  switch(level) {
  case AI_LEVEL_AWAY:
    return 100;
  case AI_LEVEL_HANDICAPPED:
  case AI_LEVEL_NOVICE:
    return 250;
  case AI_LEVEL_EASY:
  case AI_LEVEL_NORMAL:
  case AI_LEVEL_HARD:
  case AI_LEVEL_CHEATING:
#ifdef FREECIV_DEBUG
  case AI_LEVEL_EXPERIMENTAL:
#endif /* FREECIV_DEBUG */
    return 100;
  case AI_LEVEL_COUNT:
    fc_assert(level != AI_LEVEL_COUNT);
    return 100;
  }

  return 100;
}

/**********************************************************************//**
  Return the AI expansion tendency, a percentage factor to value new cities,
  compared to defaults.  0 means _never_ build new cities, > 100 means to
  (over?)value them even more than the default (already expansionistic) AI.
**************************************************************************/
static int expansionism_of_skill_level(enum ai_level level)
{
  fc_assert(ai_level_is_valid(level));

  switch(level) {
  case AI_LEVEL_AWAY:
    return 0;
  case AI_LEVEL_HANDICAPPED:
  case AI_LEVEL_NOVICE:
  case AI_LEVEL_EASY:
    return 10;
  case AI_LEVEL_NORMAL:
  case AI_LEVEL_HARD:
  case AI_LEVEL_CHEATING:
#ifdef FREECIV_DEBUG
  case AI_LEVEL_EXPERIMENTAL:
#endif /* FREECIV_DEBUG */
    return 100;
  case AI_LEVEL_COUNT:
    fc_assert(level != AI_LEVEL_COUNT);
    return 100;
  }

  return 100;
}

/**********************************************************************//**
  Helper function for skill level command help.
  'cmdname' is a server command name.
  Caller must free returned string.
**************************************************************************/
char *ai_level_help(const char *cmdname)
{
  /* Translate cmdname to AI level. */
  enum ai_level level = ai_level_by_name(cmdname, fc_strcasecmp);
  struct astring help = ASTRING_INIT, features = ASTRING_INIT;
  bv_handicap handicaps;
  enum handicap_type h;

  fc_assert(ai_level_is_valid(level));

  if (level == AI_LEVEL_AWAY) {
    /* Special case */
    astr_add_line(&help,
                  _("Toggles 'away' mode for your nation. In away mode, "
                    "the AI will govern your nation but make only minimal "
                    "changes."));
  } else {
    /* TRANS: %s is a (translated) skill level ('Novice', 'Hard', etc) */
    astr_add_line(&help,
                  _("With no arguments, sets all AI players to skill level "
                    "'%s', and sets the default level for any new AI "
                    "players to '%s'. With an argument, sets the skill "
                    "level for the specified player only."),
                  _(ai_level_name(level)), _(ai_level_name(level)));
  }

  handicaps = handicap_of_skill_level(level);
  for (h = 0; h < H_LAST; h++) {
    bool inverted;
    const char *desc = handicap_desc(h, &inverted);

    if (desc && BV_ISSET(handicaps, h) != inverted) {
      astr_add_line(&features, "%s", desc);
    }
  }

  if (fuzzy_of_skill_level(level) > 0) {
    astr_add_line(&features, _("Has erratic decision-making."));
  }
  {
    int science = science_cost_of_skill_level(level);

    if (science != 100) {
      astr_add_line(&features,
                    _("Research takes %d%% as long as usual."), science);
    }
  }
  if (expansionism_of_skill_level(level) < 100) {
    astr_add_line(&features, _("Has reduced appetite for expansion."));
  } /* no level currently has >100, so no string yet */

  switch (level) {
  case AI_LEVEL_HANDICAPPED:
    /* TRANS: describing an AI skill level */
    astr_add_line(&help,
                  _("\nThis skill level has the same features as 'Novice', "
                    "but may suffer additional ruleset-defined penalties."));
    break;
  case AI_LEVEL_CHEATING:
    /* TRANS: describing an AI skill level */
    astr_add_line(&help,
                  _("\nThis skill level has the same features as 'Hard', "
                    "but may enjoy additional ruleset-defined bonuses."));
    break;
  default:
    /* In principle this text should vary, but all current skill levels
     * have _some_ feature text */
    fc_assert(!astr_empty(&features));
    /* TRANS: describing an AI skill level */
    astr_add_line(&help,
                  _("\nThis skill level's features include the following. "
                    "(Some rulesets may define extra level-specific "
                    "behavior.)"));
    break;
  }

  if (!astr_empty(&features)) {
    astr_add_line(&help, "\n%s", astr_str(&features));
  }

  astr_free(&features);
  return astr_to_str(&help);
}

/**********************************************************************//**
  Return the value normal_decision (a boolean), except if the AI is fuzzy,
  then sometimes flip the value.  The intention of this is that instead of
    if (condition) { action }
  you can use
    if (ai_fuzzy(pplayer, condition)) { action }
  to sometimes flip a decision, to simulate an AI with some confusion,
  indecisiveness, forgetfulness etc. In practice its often safer to use
    if (condition && ai_fuzzy(pplayer,1)) { action }
  for an action which only makes sense if condition holds, but which a
  fuzzy AI can safely "forget".  Note that for a non-fuzzy AI, or for a
  human player being helped by the AI (eg, autosettlers), you can ignore
  the "ai_fuzzy(pplayer," part, and read the previous example as:
    if (condition && 1) { action }
  --dwp
**************************************************************************/
bool ai_fuzzy(const struct player *pplayer, bool normal_decision)
{
  if (!is_ai(pplayer) || pplayer->ai_common.fuzzy == 0) {
    return normal_decision;
  }
  if (fc_rand(1000) >= pplayer->ai_common.fuzzy) {
    return normal_decision;
  }
  return !normal_decision;
}
