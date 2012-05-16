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
#ifndef FC__AIDIPLOMAT_H
#define FC__AIDIPLOMAT_H

#include "shared.h"		/* bool type */
#include "fc_types.h"

void ai_manage_diplomat(struct player *pplayer, struct unit *punit);
void ai_choose_diplomat_defensive(struct player *pplayer,
                                  struct city *pcity,
                                  struct ai_choice *choice, int def);
void ai_choose_diplomat_offensive(struct player *pplayer,
                                  struct city *pcity,
                                  struct ai_choice *choice);

#endif  /* FC__AIDIPLOMAT_H */
