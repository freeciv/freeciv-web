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
#ifndef FC__BUILDINGADV_H
#define FC__BUILDINGADV_H

/* server/advisors */
#include "advchoice.h"

#define FOOD_WEIGHTING 30
#define SHIELD_WEIGHTING 17
#define TRADE_WEIGHTING 18
/* The Trade Weighting has to about as large as the Shield Weighting,
   otherwise the AI will build Barracks to create veterans in cities 
   with only 1 shields production.
    8 is too low
   18 is too high
 */
#define POLLUTION_WEIGHTING 20 /* tentative */
#define WARMING_FACTOR 50
#define COOLING_FACTOR WARMING_FACTOR

struct player;

void building_advisor(struct player *pplayer);

void advisor_choose_build(struct player *pplayer, struct city *pcity);
void building_advisor_choose(struct city *pcity, struct adv_choice *choice);

#endif   /* FC__BUILDINGADV_H */
