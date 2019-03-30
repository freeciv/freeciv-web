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
#ifndef FC__CULTURE_H
#define FC__CULTURE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct city;
struct player;

int city_culture(const struct city *pcity);
int city_history_gain(const struct city *pcity);

int player_culture(const struct player *plr);
int nation_history_gain(const struct player *pplayer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__CULTURE_H */
