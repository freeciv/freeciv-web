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
#ifndef FC__AIAIR_H
#define FC__AIAIR_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

void ai_manage_airunit(struct player *pplayer, struct unit *punit);
bool ai_choose_attacker_air(struct player *pplayer, struct city *pcity, 
			    struct ai_choice *choice);

int ai_evaluate_tile_for_attack(struct unit *punit, struct tile *dst_tile);

#endif /* FC__AIAIR_H */
