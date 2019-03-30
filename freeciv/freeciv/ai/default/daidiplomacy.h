/***********************************************************************
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
#ifndef FC__DAIDIPLOMACY_H
#define FC__DAIDIPLOMACY_H

#include "fc_types.h"

#include "ai.h" /* incident_type */

struct Treaty;
struct Clause;
struct ai_data;

void dai_diplomacy_begin_new_phase(struct ai_type *ait, struct player *pplayer);
void dai_diplomacy_actions(struct ai_type *ait, struct player *pplayer);

void dai_treaty_evaluate(struct ai_type *ait, struct player *pplayer,
                         struct player *aplayer, struct Treaty *ptreaty);
void dai_treaty_accepted(struct ai_type *ait, struct player *pplayer,
                         struct player *aplayer, struct Treaty *ptreaty);

void dai_incident(struct ai_type *ait, enum incident_type type,
                  struct player *violator, struct player *victim);

bool dai_on_war_footing(struct ai_type *ait, struct player *pplayer);

void dai_diplomacy_first_contact(struct ai_type *ait, struct player *pplayer,
                                 struct player *aplayer);

#endif /* FC__DAIDIPLOMACY_H */
