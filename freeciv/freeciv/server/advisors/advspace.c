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
#include "government.h"
#include "packets.h"
#include "spaceship.h"

/* server */
#include "spacerace.h"

#include "advspace.h"

/************************************************************************//**
  Place all available spaceship components.

  Returns TRUE iff at least one part was placed.
****************************************************************************/
bool adv_spaceship_autoplace(struct player *pplayer,
                             struct player_spaceship *ship)
{
  struct spaceship_component place;
  bool retval = FALSE;
  bool placed;

  do {
    placed = next_spaceship_component(pplayer, ship, &place);

    if (placed) {
      if (do_spaceship_place(pplayer, ACT_REQ_SS_AGENT,
                             place.type, place.num)) {
        /* A part was placed. It was placed even if the placement of future
         * parts will fail. */
        retval = TRUE;
      } else {
        /* Unable to place this part. Don't try to place it again. */
        break;
      }
    }
  } while (placed);

  return retval;
}
