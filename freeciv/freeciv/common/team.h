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

#ifndef FC__TEAM_H
#define FC__TEAM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "fc_types.h"

#include "tech.h"

#define MAX_NUM_TEAM_SLOTS MAX_NUM_PLAYER_SLOTS

/* Opaque types. */
struct team;
struct team_slot;

/* General team slot accessor functions. */
void team_slots_init(void);
bool team_slots_initialised(void);
void team_slots_free(void);
int team_slot_count(void);

struct team_slot *team_slot_first(void);
struct team_slot *team_slot_next(struct team_slot *tslot);

/* Team slot accessor functions. */
int team_slot_index(const struct team_slot *tslot);
struct team *team_slot_get_team(const struct team_slot *tslot);
bool team_slot_is_used(const struct team_slot *tslot);
struct team_slot *team_slot_by_number(int team_id);
struct team_slot *team_slot_by_rule_name(const char *team_name);
const char *team_slot_rule_name(const struct team_slot *tslot);
const char *team_slot_name_translation(const struct team_slot *tslot);
const char *team_slot_defined_name(const struct team_slot *tslot);
void team_slot_set_defined_name(struct team_slot *tslot, const char *team_name);

/* Team accessor functions. */
struct team *team_new(struct team_slot *tslot);
void team_destroy(struct team *pteam);
int team_count(void);
int team_index(const struct team *pteam);
int team_number(const struct team *pteam);
struct team *team_by_number(const int team_id);
const char *team_rule_name(const struct team *pteam);
const char *team_name_translation(const struct team *pteam);
int team_pretty_name(const struct team *pteam, char *buf, size_t buf_len);

const struct player_list *team_members(const struct team *pteam);

/* Ancillary routines */
void team_add_player(struct player *pplayer, struct team *pteam);
void team_remove_player(struct player *pplayer);

/* iterate over all team slots */
#define team_slots_iterate(_tslot)                                          \
  if (team_slots_initialised()) {                                           \
    struct team_slot *_tslot = team_slot_first();                           \
    for (; NULL != _tslot; _tslot = team_slot_next(_tslot)) {
#define team_slots_iterate_end                                              \
    }                                                                       \
  }

/* iterate over all teams, which are used at the moment */
#define teams_iterate(_pteam)                                               \
  team_slots_iterate(_tslot) {                                              \
    struct team *_pteam = team_slot_get_team(_tslot);                       \
    if (_pteam != NULL) {
#define teams_iterate_end                                                   \
    }                                                                       \
  } team_slots_iterate_end;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__TEAM_H */
