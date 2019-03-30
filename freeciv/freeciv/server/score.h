/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__SCORE_H
#define FC__SCORE_H

#include "fc_types.h"

void calc_civ_score(struct player *pplayer);

int get_civ_score(const struct player *pplayer);

int total_player_citizens(const struct player *pplayer);

void rank_users(bool);

#endif /* FC__SCORE_H */
