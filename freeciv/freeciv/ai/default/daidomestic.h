/***********************************************************************
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
#ifndef FC__DAIDOMESTIC_H
#define FC__DAIDOMESTIC_H

/* common */
#include "fc_types.h"

struct adv_choice *domestic_advisor_choose_build(struct ai_type *ait, struct player *pplayer,
                                                 struct city *pcity);

void dai_wonder_city_distance(struct ai_type *ait, struct player *pplayer, 
                              struct adv_data *adv);

#endif  /* FC__DAIDOMESTIC_H */
