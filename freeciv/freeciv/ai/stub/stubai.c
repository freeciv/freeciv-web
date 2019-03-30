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

/* common */
#include "ai.h"
#include "player.h"

const char *fc_ai_stub_capstr(void);
bool fc_ai_stub_setup(struct ai_type *ai);

/**********************************************************************//**
  Return module capability string
**************************************************************************/
const char *fc_ai_stub_capstr(void)
{
  return FC_AI_MOD_CAPSTR;
}

/**********************************************************************//**
  Set phase done
**************************************************************************/
static void stub_end_turn(struct player *pplayer)
{
  pplayer->ai_phase_done = TRUE;
}

/**********************************************************************//**
  Setup player ai_funcs function pointers.
**************************************************************************/
bool fc_ai_stub_setup(struct ai_type *ai)
{
  strncpy(ai->name, "stub", sizeof(ai->name));

  ai->funcs.first_activities = stub_end_turn;
  ai->funcs.restart_phase    = stub_end_turn;

  return TRUE;
}
