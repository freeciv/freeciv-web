/********************************************************************** 
 Freeciv - Copyright (C) 2002 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__DAI_EFFECTS_H
#define FC__DAI_EFFECTS_H

adv_want dai_effect_value(struct player *pplayer, struct government *gov,
                          const struct adv_data *ai, const struct city *pcity,
                          const bool capital, int turns,
                          const struct effect *peffect, const int c,
                          const int nplayers);

adv_want dai_content_effect_value(const struct player *pplayer,
                                  const struct city *pcity,
                                  int amount,
                                  int num_cities,
                                  int happiness_step);

bool dai_can_requirement_be_met_in_city(const struct requirement *preq,
                                        const struct player *pplayer,
                                        const struct city *pcity);

#endif /* FC__DAI_EFFECTS_H */
