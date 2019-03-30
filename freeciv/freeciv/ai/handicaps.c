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
#include "shared.h"

/* common */
#include "player.h"

#include "handicaps.h"

/**********************************************************************//**
  Initialize handicaps for player
**************************************************************************/
void handicaps_init(struct player *pplayer)
{
  if (pplayer->ai_common.handicaps != NULL) {
    return;
  }

  pplayer->ai_common.handicaps = fc_malloc(sizeof(bv_handicap));
  BV_CLR_ALL(*((bv_handicap *)pplayer->ai_common.handicaps));
}

/**********************************************************************//**
  Free resources associated with player handicaps.
**************************************************************************/
void handicaps_close(struct player *pplayer)
{
  if (pplayer->ai_common.handicaps == NULL) {
    return;
  }

  free(pplayer->ai_common.handicaps);
  pplayer->ai_common.handicaps = NULL;
}

/**********************************************************************//**
  Set player handicaps
**************************************************************************/
void handicaps_set(struct player *pplayer, bv_handicap handicaps)
{
  *((bv_handicap *)pplayer->ai_common.handicaps) = handicaps;
}

/**********************************************************************//**
  AI players may have handicaps - allowing them to cheat or preventing
  them from using certain algorithms.  This function returns whether the
  player has the given handicap.  Human players are assumed to have no
  handicaps.
**************************************************************************/
bool has_handicap(const struct player *pplayer, enum handicap_type htype)
{
  if (is_human(pplayer)) {
    return TRUE;
  }

  return BV_ISSET(*((bv_handicap *)pplayer->ai_common.handicaps), htype);
}

/**********************************************************************//**
  Return a short (translated) string describing the handicap, for help.
  In some cases it's better to describe what happens if the handicap is
  absent; 'inverted' is set TRUE in these cases.
**************************************************************************/
const char *handicap_desc(enum handicap_type htype, bool *inverted)
{
  *inverted = FALSE;
  switch (htype) {
  case H_DIPLOMAT:
    return _("Doesn't build offensive diplomatic units.");
  case H_AWAY:
    return NULL; /* AI_LEVEL_AWAY has its own description */
  case H_LIMITEDHUTS:
    return _("Gets reduced bonuses from huts.");
  case H_DEFENSIVE:
    return _("Prefers defensive buildings and avoids close diplomatic "
             "relations.");
  case H_EXPERIMENTAL:
    return _("THIS IS ONLY FOR TESTING OF NEW AI FEATURES! For ordinary "
             "servers, this level is no different to 'Hard'.");
  case H_RATES:
    *inverted = TRUE;
    return _("Has no restrictions on tax rates.");
  case H_TARGETS:
    *inverted = TRUE;
    return _("Can target units and cities in unseen or unexplored territory.");
  case H_HUTS:
    *inverted = TRUE;
    return _("Knows the location of huts in unexplored territory.");
  case H_FOG:
    *inverted = TRUE;
    return _("Can see through fog of war.");
  case H_NOPLANES:
    return _("Doesn't build air units.");
  case H_MAP:
    *inverted = TRUE;
    return _("Has complete map knowledge, including unexplored territory.");
  case H_DIPLOMACY:
    return _("Naive at diplomacy.");
  case H_REVOLUTION:
    *inverted = TRUE;
    return _("Can skip anarchy during revolution.");
  case H_EXPANSION:
    return _("Limits growth to match human players.");
  case H_DANGER:
    return _("Believes its cities are always under threat.");
  case H_CEASEFIRE:
    return _("Always offers cease-fire on first contact.");
  case H_NOBRIBE_WF:
    return _("Doesn't bribe worker or city founder units.");
  case H_PRODCHGPEN:
    *inverted = TRUE;
    return _("Can change city production type without penalty.");
#ifdef FREECIV_WEB
  case H_ASSESS_DANGER_LIMITED:
    return _("Limits the distance to search for threatening enemy units.");    
#endif
  case H_LAST:
    break; /* fall through -- should never see this */
  }

  /* Should never reach here */
  fc_assert_ret_val(FALSE, NULL);
}
