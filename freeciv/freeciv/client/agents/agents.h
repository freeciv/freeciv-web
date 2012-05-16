/********************************************************************** 
 Freeciv - Copyright (C) 2001 - R. Falke
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__CLIENT_AGENTS_AGENTS_H
#define FC__CLIENT_AGENTS_AGENTS_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

/*
 * Besides callback for convenience client/agents/agents also
 * implements a "flattening" of the call stack i.e. to ensure that
 * every agent is only called once at any time.
 */

/* Don't use the very last level unless you know what you're doing */
#define LAST_AGENT_LEVEL 99

#define MAX_AGENT_NAME_LEN 10

enum callback_type {
  CB_NEW, CB_REMOVE, CB_CHANGE, CB_LAST
};

struct agent {
  char name[MAX_AGENT_NAME_LEN];
  int level;

  void (*turn_start_notify) (void);
  void (*city_callbacks[CB_LAST]) (int);
  void (*unit_callbacks[CB_LAST]) (int);
  void (*tile_callbacks[CB_LAST]) (struct tile *ptile);
};

void agents_init(void);
void agents_free(void);
void register_agent(const struct agent *agent);
bool agents_busy(void);

/* called from client/packhand.c */
void agents_disconnect(void);
void agents_processing_started(void);
void agents_processing_finished(void);
void agents_freeze_hint(void);
void agents_thaw_hint(void);
void agents_game_joined(void);
void agents_game_start(void);
void agents_before_new_turn(void);
void agents_start_turn(void);
void agents_new_turn(void);

void agents_unit_changed(struct unit *punit);
void agents_unit_new(struct unit *punit);
void agents_unit_remove(struct unit *punit);

void agents_city_changed(struct city *pcity);
void agents_city_new(struct city *pcity);
void agents_city_remove(struct city *pcity);

void agents_tile_changed(struct tile *ptile);
void agents_tile_new(struct tile *ptile);
void agents_tile_remove(struct tile *ptile);

/* called from agents */
void cause_a_city_changed_for_agent(const char *name_of_calling_agent,
				    struct city *pcity);
void cause_a_unit_changed_for_agent(const char *name_of_calling_agent,
				    struct unit *punit);
void wait_for_requests(const char *agent_name, int first_request_id,
		       int last_request_id);
#endif
