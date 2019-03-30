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

/* common */
#include "fc_types.h"
#include "unittype.h"

void dai_manage_tech(struct ai_type *ait, struct player *pplayer);
void dai_clear_tech_wants(struct ai_type *ait, struct player *pplayer);
void dai_next_tech_goal(struct player *pplayer);
struct unit_type *dai_wants_role_unit(struct ai_type *ait, struct player *pplayer,
                                      struct city *pcity, int role, int want);
struct unit_type *dai_wants_defender_against(struct ai_type *ait,
                                             struct player *pplayer,
                                             struct city *pcity,
                                             struct unit_type *att, int want);

#endif  /* FC__AITECH_H */
