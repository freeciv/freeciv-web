/***********************************************************************
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

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "fc_types.h"

void dai_hunter_choice(struct ai_type *ait, struct player *pplayer,
                       struct city *pcity, struct adv_choice *choice,
                       bool allow_gold_upkeep);
bool dai_hunter_qualify(struct player *pplayer, struct unit *punit);
int dai_hunter_manage(struct ai_type *ait, struct player *pplayer,
                      struct unit *punit);

#endif /* FC__AIHUNT_H */
