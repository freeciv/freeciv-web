/**********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__AISUPPORT_H
#define FC__AISUPPORT_H

#include "fc_types.h"

struct player *player_leading_spacerace(void);
int player_distance_to_player(struct player *pplayer, struct player *target);
int city_gold_worth(struct city *pcity);

#endif
