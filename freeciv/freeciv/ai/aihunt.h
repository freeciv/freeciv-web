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
#ifndef FC__AIHUNT_H
#define FC__AIHUNT_H

#include "shared.h"             /* bool type */

#include "fc_types.h"

void ai_hunter_choice(struct player *pplayer, struct city *pcity,
                      struct ai_choice *choice);
bool ai_hunter_qualify(struct player *pplayer, struct unit *punit);
int ai_hunter_manage(struct player *pplayer, struct unit *punit);

#endif
