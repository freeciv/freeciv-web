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

#include "fc_types.h"

#include "tech.h"

#define MAX_NUM_TEAMS (MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS)

struct team {
  Team_type_id item_number;
  int players; /* # of players on the team */
  
  struct player_research research;
};

/* General team accessor functions. */
Team_type_id team_count(void);
Team_type_id team_index(const struct team *pteam);
Team_type_id team_number(const struct team *pteam);

struct team *team_by_number(const Team_type_id id);
struct team *find_team_by_rule_name(const char *team_name);

const char *team_rule_name(const struct team *pteam);
const char *team_name_translation(struct team *pteam);

/* Ancillary routines */
void team_add_player(struct player *pplayer, struct team *pteam);
void team_remove_player(struct player *pplayer);

struct team *find_empty_team(void);

/* Initialization and iteration */
void teams_init(void);

struct team *team_array_first(void);
const struct team *team_array_last(void);

/* This is different than other iterators.  It always does the entire
 * list, but skips unused entries.
 */
#define team_iterate(_p)						\
{									\
  struct team *_p = team_array_first();					\
  if (NULL != _p) {							\
    for (; _p <= team_array_last(); _p++) {				\
      if (_p->players == 0) {						\
	continue;							\
      }

#define team_iterate_end						\
    }									\
  }									\
}

#endif /* FC__TEAM_H */
