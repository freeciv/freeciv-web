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

#include "fc_types.h"

void dai_manage_diplomat(struct ai_type *ait, struct player *pplayer,
                         struct unit *punit);
void dai_choose_diplomat_defensive(struct ai_type *ait,
                                   struct player *pplayer,
                                   struct city *pcity,
                                   struct adv_choice *choice, int def);
void dai_choose_diplomat_offensive(struct ai_type *ait,
                                   struct player *pplayer,
                                   struct city *pcity,
                                   struct adv_choice *choice);

#endif  /* FC__AIDIPLOMAT_H */
