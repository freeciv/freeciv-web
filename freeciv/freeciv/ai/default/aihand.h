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
#ifndef FC__AIHAND_H
#define FC__AIHAND_H

#include "fc_types.h"

void dai_do_first_activities(struct ai_type *ait, struct player *pplayer);
void dai_do_last_activities(struct ai_type *ait, struct player *pplayer);

void dai_calc_data(const struct player *pplayer, int *trade, int *expenses,
                   int *income);

#endif  /* FC__AIHAND_H */
