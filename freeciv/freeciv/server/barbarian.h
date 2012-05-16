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

#ifndef FC__BARBARIAN_H
#define FC__BARBARIAN_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

#define MIN_UNREST_DIST   5
#define MAX_UNREST_DIST   8

#define UPRISE_CIV_SIZE  10
#define UPRISE_CIV_MORE  30
#define UPRISE_CIV_MOST  50

#define MAP_FACTOR     2000  /* adjust this to get a good uprising frequency */

#define BARBARIAN_MIN_LIFESPAN 5

bool unleash_barbarians(struct tile *ptile);
void summon_barbarians(void);
bool is_land_barbarian(struct player *pplayer);
bool is_sea_barbarian(struct player *pplayer);

#endif  /* FC__BARBARIAN_H */
