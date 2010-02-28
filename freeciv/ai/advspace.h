/********************************************************************** 
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
#ifndef FC__ADVSPACE_H
#define FC__ADVSPACE_H

#include "shared.h"

#include "fc_types.h"

struct player_spaceship;

bool ai_spaceship_autoplace(struct player *pplayer,
			    struct player_spaceship *ship);

#endif  /* FC__ADVSPACE_H */
