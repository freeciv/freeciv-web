/***********************************************************************
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
#include <fc_config.h>
#endif

#include <stdlib.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "game.h"
#include "player.h"
#include "team.h"

struct team_slot {
  struct team *team;
  char *defined_name;                   /* Defined by the ruleset. */
  char *rule_name;                      /* Usable untranslated name. */
#ifdef FREECIV_ENABLE_NLS
  char *name_translation;               /* Translated name. */
#endif
};

struct team {
  struct player_list *plrlist;
  struct team_slot *slot;
};

static struct {
  struct team_slot *slots;
  int used_slots;
} team_slots;

/************************************************************************//**
  Initialise all team slots.
****************************************************************************/
void team_slots_init(void)
{
  int i;

  /* Init team slots and names. */
  team_slots.slots = fc_calloc(team_slot_count(), sizeof(*team_slots.slots));
  /* Can't use the defined functions as the needed data will be
   * defined here. */
  for (i = 0; i < team_slot_count(); i++) {
    struct team_slot *tslot = team_slots.slots + i;

    tslot->team = NULL;
    tslot->defined_name = NULL;
    tslot->rule_name = NULL;
#ifdef FREECIV_ENABLE_NLS
    tslot->name_translation = NULL;
#endif
  }
  team_slots.used_slots = 0;
}

/************************************************************************//**
  Returns TRUE if the team slots have been initialized.
****************************************************************************/
bool team_slots_initialised(void)
{
  return (team_slots.slots != NULL);
}

/************************************************************************//**
  Remove all team slots.
****************************************************************************/
void team_slots_free(void)
{
  team_slots_iterate(tslot) {
    if (NULL != tslot->team) {
      team_destroy(tslot->team);
    }
    if (NULL != tslot->defined_name) {
      free(tslot->defined_name);
    }
    if (NULL != tslot->rule_name) {
      free(tslot->rule_name);
    }
#ifdef FREECIV_ENABLE_NLS
    if (NULL != tslot->name_translation) {
      free(tslot->name_translation);
    }
#endif /* FREECIV_ENABLE_NLS */
  } team_slots_iterate_end;
  free(team_slots.slots);
  team_slots.slots = NULL;

  team_slots.used_slots = 0;
}

/************************************************************************//**
  Returns the total number of team slots (including used slots).
****************************************************************************/
int team_slot_count(void)
{
  return (MAX_NUM_TEAM_SLOTS);
}

/************************************************************************//**
  Returns the first team slot.
****************************************************************************/
struct team_slot *team_slot_first(void)
{
  return team_slots.slots;
}

/************************************************************************//**
  Returns the next team slot.
****************************************************************************/
struct team_slot *team_slot_next(struct team_slot *tslot)
{
  tslot++;
  return (tslot < team_slots.slots + team_slot_count() ? tslot : NULL);
}


/************************************************************************//**
  Returns the index of the team slots.
****************************************************************************/
int team_slot_index(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), -1);
  fc_assert_ret_val(tslot != NULL, -1);

  return tslot - team_slots.slots;
}

/************************************************************************//**
  Returns the team corresponding to the slot. If the slot is not used, it
  will return NULL. See also team_slot_is_used().
****************************************************************************/
struct team *team_slot_get_team(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), NULL);
  fc_assert_ret_val(tslot != NULL, NULL);

  return tslot->team;
}

/************************************************************************//**
  Returns TRUE is this slot is "used" i.e. corresponds to a
  valid, initialized team that exists in the game.
****************************************************************************/
bool team_slot_is_used(const struct team_slot *tslot)
{
  /* No team slot available, if the game is not initialised. */
  if (!team_slots_initialised()) {
    return FALSE;
  }

  return NULL != tslot->team;
}

/************************************************************************//**
  Return the possibly unused and uninitialized team slot.
****************************************************************************/
struct team_slot *team_slot_by_number(int team_id)
{
  if (!team_slots_initialised()
      || !(0 <= team_id && team_id < team_slot_count())) {
    return NULL;
  }

  return team_slots.slots + team_id;
}

/************************************************************************//**
  Does a linear search for a (defined) team name. Returns NULL when none
  match.
****************************************************************************/
struct team_slot *team_slot_by_rule_name(const char *team_name)
{
  fc_assert_ret_val(team_name != NULL, NULL);

  team_slots_iterate(tslot) {
    const char *tname = team_slot_rule_name(tslot);

    if (NULL != tname && 0 == fc_strcasecmp(tname, team_name)) {
      return tslot;
    }
  } team_slots_iterate_end;

  return NULL;
}

/************************************************************************//**
  Creates a default name for this team slot.
****************************************************************************/
static inline void team_slot_create_default_name(struct team_slot *tslot)
{
  char buf[MAX_LEN_NAME];

  fc_assert(NULL == tslot->defined_name);
  fc_assert(NULL == tslot->rule_name);
#ifdef FREECIV_ENABLE_NLS
  fc_assert(NULL == tslot->name_translation);
#endif /* FREECIV_ENABLE_NLS */

  fc_snprintf(buf, sizeof(buf), "Team %d", team_slot_index(tslot) + 1);
  tslot->rule_name = fc_strdup(buf);

#ifdef FREECIV_ENABLE_NLS
  fc_snprintf(buf, sizeof(buf), _("Team %d"), team_slot_index(tslot) + 1);
  tslot->name_translation = fc_strdup(buf);
#endif /* FREECIV_ENABLE_NLS */

  log_verbose("No name defined for team %d! Creating a default name: %s.",
              team_slot_index(tslot), tslot->rule_name);
}

/************************************************************************//**
  Returns the name (untranslated) of the slot. Creates a default one if it
  doesn't exist currently.
****************************************************************************/
const char *team_slot_rule_name(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), NULL);
  fc_assert_ret_val(NULL != tslot, NULL);

  if (NULL == tslot->rule_name) {
    /* Get the team slot as changeable (not _const_) struct. */
    struct team_slot *changeable
      = team_slot_by_number(team_slot_index(tslot));
    team_slot_create_default_name(changeable);
    return changeable->rule_name;
  }

  return tslot->rule_name;
}

/************************************************************************//**
  Returns the name (translated) of the slot. Creates a default one if it
  doesn't exist currently.
****************************************************************************/
const char *team_slot_name_translation(const struct team_slot *tslot)
{
#ifdef FREECIV_ENABLE_NLS
  fc_assert_ret_val(team_slots_initialised(), NULL);
  fc_assert_ret_val(NULL != tslot, NULL);

  if (NULL == tslot->name_translation) {
    /* Get the team slot as changeable (not _const_) struct. */
    struct team_slot *changeable
      = team_slot_by_number(team_slot_index(tslot));
    team_slot_create_default_name(changeable);
    return changeable->name_translation;
  }

  return tslot->name_translation;
#else  /* FREECIV_ENABLE_NLS */
  return team_slot_rule_name(tslot);
#endif /* FREECIV_ENABLE_NLS */
}

/************************************************************************//**
  Returns the name defined in the ruleset for this slot. It may return NULL
  if the ruleset didn't defined a such name.
****************************************************************************/
const char *team_slot_defined_name(const struct team_slot *tslot)
{
  fc_assert_ret_val(team_slots_initialised(), NULL);
  fc_assert_ret_val(NULL != tslot, NULL);

  return tslot->defined_name;
}

/************************************************************************//**
  Set the name defined in the ruleset for this slot.
****************************************************************************/
void team_slot_set_defined_name(struct team_slot *tslot,
                                const char *team_name)
{
  fc_assert_ret(team_slots_initialised());
  fc_assert_ret(NULL != tslot);
  fc_assert_ret(NULL != team_name);

  if (NULL != tslot->defined_name) {
    free(tslot->defined_name);
  }
  tslot->defined_name = fc_strdup(team_name);

  if (NULL != tslot->rule_name) {
    free(tslot->rule_name);
  }
  tslot->rule_name = fc_strdup(Qn_(team_name));

#ifdef FREECIV_ENABLE_NLS
  if (NULL != tslot->name_translation) {
    free(tslot->name_translation);
  }
  tslot->name_translation = fc_strdup(Q_(team_name));
#endif /* FREECIV_ENABLE_NLS */
}

/************************************************************************//**
  Creates a new team for the slot. If slot is NULL, it will lookup to a
  free slot. If the slot already used, then just return the team.
****************************************************************************/
struct team *team_new(struct team_slot *tslot)
{
  struct team *pteam;

  fc_assert_ret_val(team_slots_initialised(), NULL);

  if (NULL == tslot) {
    team_slots_iterate(aslot) {
      if (!team_slot_is_used(aslot)) {
        tslot = aslot;
        break;
      }
    } team_slots_iterate_end;

    fc_assert_ret_val(NULL != tslot, NULL);
  } else if (NULL != tslot->team) {
    return tslot->team;
  }

  /* Now create the team. */
  log_debug("Create team for slot %d.", team_slot_index(tslot));
  pteam = fc_calloc(1, sizeof(*pteam));
  pteam->slot = tslot;
  tslot->team = pteam;

  /* Set default values. */
  pteam->plrlist = player_list_new();

  /* Increase number of teams. */
  team_slots.used_slots++;

  return pteam;
}

/************************************************************************//**
  Destroys a team.
****************************************************************************/
void team_destroy(struct team *pteam)
{
  struct team_slot *tslot;

  fc_assert_ret(team_slots_initialised());
  fc_assert_ret(NULL != pteam);
  fc_assert(0 == player_list_size(pteam->plrlist));

  tslot = pteam->slot;
  fc_assert(tslot->team == pteam);

  player_list_destroy(pteam->plrlist);
  free(pteam);
  tslot->team = NULL;
  team_slots.used_slots--;
}

/************************************************************************//**
  Return the current number of teams.
****************************************************************************/
int team_count(void)
{
  return team_slots.used_slots;
}

/************************************************************************//**
  Return the team index.
****************************************************************************/
int team_index(const struct team *pteam)
{
  return team_number(pteam);
}

/************************************************************************//**
  Return the team index/number/id.
****************************************************************************/
int team_number(const struct team *pteam)
{
  fc_assert_ret_val(NULL != pteam, -1);
  return team_slot_index(pteam->slot);
}

/************************************************************************//**
  Return struct team pointer for the given team index.
****************************************************************************/
struct team *team_by_number(const int team_id)
{
  const struct team_slot *tslot = team_slot_by_number(team_id);

  return (NULL != tslot ? team_slot_get_team(tslot) : NULL);
}

/************************************************************************//**
  Returns the name (untranslated) of the team.
****************************************************************************/
const char *team_rule_name(const struct team *pteam)
{
  fc_assert_ret_val(NULL != pteam, NULL);

  return team_slot_rule_name(pteam->slot);
}

/************************************************************************//**
  Returns the name (translated) of the team.
****************************************************************************/
const char *team_name_translation(const struct team *pteam)
{
  fc_assert_ret_val(NULL != pteam, NULL);

  return team_slot_name_translation(pteam->slot);
}

/************************************************************************//**
  Set in 'buf' the name of the team 'pteam' in a format like
  "team <team_name>". To avoid to see "team Team 0", it only prints the
  the team number when the name of this team is not defined in the ruleset.
****************************************************************************/
int team_pretty_name(const struct team *pteam, char *buf, size_t buf_len)
{
  if (NULL != pteam) {
    if (NULL != pteam->slot->defined_name) {
      return fc_snprintf(buf, buf_len, _("team %s"),
                         team_slot_name_translation(pteam->slot));
    } else {
      return fc_snprintf(buf, buf_len, _("team %d"), team_number(pteam));
    }
  }

  /* No need to translate, it's an error. */
  fc_strlcpy(buf, "(null team)", buf_len);
  return -1;
}

/************************************************************************//**
  Returns the member list of the team.
****************************************************************************/
const struct player_list *team_members(const struct team *pteam)
{
  fc_assert_ret_val(NULL != pteam, NULL);

  return pteam->plrlist;
}

/************************************************************************//**
  Set a player to a team.  Removes the previous team affiliation if
  necessary.
****************************************************************************/
void team_add_player(struct player *pplayer, struct team *pteam)
{
  fc_assert_ret(pplayer != NULL);

  if (pteam == NULL) {
    pteam = team_new(NULL);
  }

  fc_assert_ret(pteam != NULL);

  if (pteam == pplayer->team) {
    /* It is the team of the player. */
    return;
  }

  log_debug("Adding player %d/%s to team %s.", player_number(pplayer),
            pplayer->username, team_rule_name(pteam));

  /* Remove the player from the old team, if any. */
  team_remove_player(pplayer);

  /* Put the player on the new team. */
  pplayer->team = pteam;
  player_list_append(pteam->plrlist, pplayer);
}

/************************************************************************//**
  Remove the player from the team.  This should only be called when deleting
  a player; since every player must always be on a team.

  Note in some very rare cases a player may not be on a team.  It's safe
  to call this function anyway.
****************************************************************************/
void team_remove_player(struct player *pplayer)
{
  struct team *pteam;

  if (pplayer->team) {
    pteam = pplayer->team;

    log_debug("Removing player %d/%s from team %s (%d)",
              player_number(pplayer), player_name(pplayer),
              team_rule_name(pteam), player_list_size(pteam->plrlist));
    player_list_remove(pteam->plrlist, pplayer);

    if (player_list_size(pteam->plrlist) == 0) {
      team_destroy(pteam);
    }
  }
  pplayer->team = NULL;
}
