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
#ifndef FC__AITECH_H
#define FC__AITECH_H

#include "fc_types.h"
#include "unittype.h"

void ai_manage_tech(struct player *pplayer); 
void ai_next_tech_goal(struct player *pplayer);
struct unit_type *ai_wants_role_unit(struct player *pplayer,
				     struct city *pcity,
				     int role, int want);

#endif  /* FC__AITECH_H */
