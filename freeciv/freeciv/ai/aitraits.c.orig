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

/* utility */
#include "mem.h"
#include "rand.h"

/* common */
#include "game.h"
#include "player.h"
#include "traits.h"

#include "aitraits.h"

/**********************************************************************//**
  Initialize ai traits for player
**************************************************************************/
void ai_traits_init(struct player *pplayer)
{
  enum trait tr;

  pplayer->ai_common.traits = fc_realloc(pplayer->ai_common.traits,
                                         sizeof(struct ai_trait) * TRAIT_COUNT);

  for (tr = trait_begin(); tr != trait_end(); tr = trait_next(tr)) {
    int min = pplayer->nation->server.traits[tr].min;
    int max = pplayer->nation->server.traits[tr].max;

    switch (game.server.trait_dist) {
    case TDM_FIXED:
      pplayer->ai_common.traits[tr].val = pplayer->nation->server.traits[tr].fixed;
      break;
    case TDM_EVEN:
      pplayer->ai_common.traits[tr].val = fc_rand(max + 1 - min) + min;
      break;
    }
    pplayer->ai_common.traits[tr].mod = 0;
  }
}

/**********************************************************************//**
  Free resources associated with player ai traits.
**************************************************************************/
void ai_traits_close(struct player *pplayer)
{
  FC_FREE(pplayer->ai_common.traits);

  pplayer->ai_common.traits = NULL;
}

/**********************************************************************//**
  Get current value of player trait
**************************************************************************/
int ai_trait_get_value(enum trait tr, struct player *pplayer)
{
  int val = pplayer->ai_common.traits[tr].val + pplayer->ai_common.traits[tr].mod;

  /* Clip so that value is at least 1, and maximum is
   * TRAIT_DEFAULT_VALUE as many times as TRAIT_DEFAULT value is
   * minimum value of 1 ->
   * minimum is default / TRAIT_DEFAULT_VALUE,
   * maximum is default * TRAIT_DEFAULT_VALUE */
  val = CLIP(1, val, TRAIT_MAX_VALUE);

  return val;
}
