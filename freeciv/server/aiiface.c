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
#include <config.h>
#endif

/* common */
#include "ai.h"
#include "player.h"

/* server */
#include "aiiface.h"
#include "settlers.h"

/* ai */
#include "aicity.h"
#include "aiexplorer.h"
#include "aihand.h"
#include "aisettler.h"
#include "aitools.h"

/**************************************************************************
  Initialize player ai_funcs function pointers.
**************************************************************************/
void ai_init(void)
{
  struct ai_type *ai = get_ai_type(AI_DEFAULT);

  init_ai(ai);

  ai->funcs.init_city = ai_init_city;
  ai->funcs.close_city = ai_close_city;
  ai->funcs.auto_settlers = auto_settlers_player;
  ai->funcs.building_advisor_init = ai_manage_buildings;
  ai->funcs.building_advisor = ai_advisor_choose_building;
  ai->funcs.auto_explorer = ai_manage_explorer;
  ai->funcs.first_activities = ai_do_first_activities;
  ai->funcs.diplomacy_actions = ai_diplomacy_actions;
  ai->funcs.last_activities = ai_do_last_activities;
  ai->funcs.before_auto_settlers = ai_settler_init;
  ai->funcs.treaty_evaluate = ai_treaty_evaluate;
  ai->funcs.treaty_accepted = ai_treaty_accepted;
  ai->funcs.first_contact = ai_diplomacy_first_contact;
  ai->funcs.incident = ai_incident;
}

/**************************************************************************
  Call incident function of victim, or failing that, incident function
  of violator.
**************************************************************************/
void call_incident(enum incident_type type, struct player *violator,
                   struct player *victim)
{
  if (victim && victim->ai->funcs.incident) {
    victim->ai->funcs.incident(type, violator, victim);
  } else if (violator && violator->ai->funcs.incident) {
    violator->ai->funcs.incident(type, violator, victim);
  }
}
