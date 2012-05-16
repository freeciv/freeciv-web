/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__ADVDIPLOMACY_H
#define FC__ADVDIPLOMACY_H

#include "fc_types.h"

#include "ai.h" /* incident_type */

struct Treaty;
struct Clause;
struct ai_data;

enum war_reason {
  WAR_REASON_BEHAVIOUR, WAR_REASON_SPACE, WAR_REASON_EXCUSE,
  WAR_REASON_HATRED, WAR_REASON_ALLIANCE, WAR_REASON_NONE
};

void ai_diplomacy_begin_new_phase(struct player *pplayer,
				  struct ai_data *ai);
void ai_diplomacy_actions(struct player *pplayer);

void ai_treaty_evaluate(struct player *pplayer, struct player *aplayer,
                        struct Treaty *ptreaty);
void ai_treaty_accepted(struct player *pplayer, struct player *aplayer, 
                        struct Treaty *ptreaty);

void ai_incident(enum incident_type type, struct player *violator,
                 struct player *victim);

bool ai_on_war_footing(struct player *pplayer);

void ai_diplomacy_first_contact(struct player *pplayer,
                                struct player *aplayer);

#endif
