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
#include "fc_types.h"
#include "game.h"
#include "player.h"

#include "mood.h"

/**********************************************************************//**
  What is the player mood?
**************************************************************************/
enum mood_type player_mood(struct player *pplayer)
{
  if (pplayer->last_war_action >= 0 && pplayer->last_war_action + 10 >= game.info.turn) {
    players_iterate(other) {
      struct player_diplstate *us, *them;

      us = player_diplstate_get(pplayer, other);
      them = player_diplstate_get(other, pplayer);

      if (us->type == DS_WAR
          || us->has_reason_to_cancel > 0
          || them->has_reason_to_cancel > 0) {
        return MOOD_COMBAT;
      }
    } players_iterate_end;
  }

  return MOOD_PEACEFUL;
}
