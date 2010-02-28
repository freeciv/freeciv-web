/********************************************************************** 
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

#include "game.h"
#include "player.h"
#include "team.h"


static struct team teams[MAX_NUM_TEAMS];


/****************************************************************************
  Initializes team structure
****************************************************************************/
void teams_init(void)
{
  Team_type_id i;

  for (i = 0; i < MAX_NUM_TEAMS; i++) {
    /* mark as unused */
    teams[i].item_number = i;

    teams[i].players = 0;
    player_research_init(&(teams[i].research));
  }
}

/**************************************************************************
  Return the first item of teams.
**************************************************************************/
struct team *team_array_first(void)
{
  if (game.info.num_teams > 0) {
    return teams;
  }
  return NULL;
}

/**************************************************************************
  Return the last item of teams array.  Note this is different!
**************************************************************************/
const struct team *team_array_last(void)
{
  if (game.info.num_teams > 0) {
    return &teams[MAX_NUM_TEAMS - 1];
  }
  return NULL;
}

/**************************************************************************
  Return the number of teams.
**************************************************************************/
int team_count(void)
{
  return game.info.num_teams;
}

/****************************************************************************
  Return the team index.

  Currently same as team_number(), paired with team_count()
  indicates use as an array index.
****************************************************************************/
Team_type_id team_index(const struct team *pteam)
{
  assert(pteam);
  return pteam - teams;
}

/****************************************************************************
  Return the team index.
****************************************************************************/
Team_type_id team_number(const struct team *pteam)
{
  assert(pteam);
  return pteam->item_number;
}

/****************************************************************************
  Return the team pointer for the given index
****************************************************************************/
struct team *team_by_number(const Team_type_id id)
{
  if (id < 0 || id >= game.info.num_teams) {
    return NULL;
  }
  return &teams[id];
}

/****************************************************************************
  Set a player to a team.  Removes the previous team affiliation if
  necessary.
****************************************************************************/
void team_add_player(struct player *pplayer, struct team *pteam)
{
  assert(pplayer != NULL);
  assert(pteam != NULL);

  freelog(LOG_DEBUG, "Adding player %d/%s to team %s.",
	  player_number(pplayer), pplayer->username,
	  team_rule_name(pteam));

  /* Remove the player from the old team, if any.  The player's team should
   * only be NULL for a few instants after the player was created; after
   * that they should automatically be put on a team.  So although we
   * check for a NULL case here this is only needed for that one
   * situation. */
  team_remove_player(pplayer);

  /* Put the player on the new team. */
  pplayer->team = pteam;
  
  pteam->players++;
  assert(pteam->players <= MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS);
}

/****************************************************************************
  Remove the player from the team.  This should only be called when deleting
  a player; since every player must always be on a team.

  Note in some very rare cases a player may not be on a team.  It's safe
  to call this function anyway.
****************************************************************************/
void team_remove_player(struct player *pplayer)
{
  if (pplayer->team) {
    freelog(LOG_DEBUG, "Removing player %d/%s from team %s (%d)",
	    player_number(pplayer),
	    pplayer->username,
	    team_rule_name(pplayer->team),
	    pplayer->team->players);
    pplayer->team->players--;
    assert(pplayer->team->players >= 0);
  }
  pplayer->team = NULL;
}

/****************************************************************************
  Return the translated name of the team.
****************************************************************************/
const char *team_name_translation(struct team *pteam)
{
  return _(team_rule_name(pteam));
}

/****************************************************************************
  Return the untranslated name of the team.
****************************************************************************/
const char *team_rule_name(const struct team *pteam)
{
  if (!pteam) {
    /* TRANS: missing value */
    return N_("(none)");
  }
  return game.info.team_names_orig[team_index(pteam)];
}

/****************************************************************************
 Does a linear search of game.info.team_names_orig[]
 Returns NULL when none match.
****************************************************************************/
struct team *find_team_by_rule_name(const char *team_name)
{
  int index;

  assert(team_name != NULL);
  assert(game.info.num_teams <= MAX_NUM_TEAMS);

  /* Can't use team_iterate here since it skips empty teams. */
  for (index = 0; index < game.info.num_teams; index++) {
    struct team *pteam = team_by_number(index);

    if (0 == mystrcasecmp(team_rule_name(pteam), team_name)) {
      return pteam;
    }
  }

  return NULL;
}

/****************************************************************************
  Returns the most empty team available.  This is the team that should be
  assigned to a newly-created player.
****************************************************************************/
struct team *find_empty_team(void)
{
  Team_type_id i;
  struct team *pbest = NULL;

  /* Can't use team_iterate here since it skips empty teams. */
  for (i = 0; i < game.info.num_teams; i++) {
    struct team *pteam = team_by_number(i);

    if (!pbest || pbest->players > pteam->players) {
      pbest = pteam;
    }
    if (pbest->players == 0) {
      /* No need to keep looking. */
      return pbest;
    }
  }

  return pbest;
}
