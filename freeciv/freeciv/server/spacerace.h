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
#ifndef FC__SPACERACE_H
#define FC__SPACERACE_H

/* common */
#include "fc_types.h"
#include "packets.h"

/* server */
#include "hand_gen.h"

struct player_spaceship;
struct conn_list;

void spaceship_calc_derived(struct player_spaceship *ship);
void send_spaceship_info(struct player *src, struct conn_list *dest);
void spaceship_arrived(struct player *pplayer);
void spaceship_lost(struct player *pplayer);
double spaceship_arrival(const struct player *pplayer);
int rank_spaceship_arrival(struct player **result);

bool do_spaceship_place(struct player *pplayer, enum action_requester from,
                        enum spaceship_place_type type, int num);

#endif /* FC__SPACERACE_H */
