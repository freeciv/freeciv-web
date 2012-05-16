/********************************************************************** 
 Freeciv - Copyright (C) 2005 The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__RESEARCH_H
#define FC__RESEARCH_H

#include "player.h"
#include "tech.h"

void do_free_cost(struct player *pplayer, Tech_type_id tech);
void do_conquer_cost(struct player *pplayer, Tech_type_id tech);
void do_dipl_cost(struct player *pplayer, Tech_type_id tech);

void do_tech_parasite_effect(struct player *pplayer);
void found_new_tech(struct player *plr, Tech_type_id tech_found,
		    bool was_discovery, bool saving_bulbs);
void update_tech(struct player *plr, int bulbs);
void init_tech(struct player *plr, bool update);
void choose_tech(struct player *plr, Tech_type_id tech);
void choose_random_tech(struct player* plr);
Tech_type_id choose_goal_tech(struct player *plr);
void choose_tech_goal(struct player *plr, Tech_type_id tech);
Tech_type_id steal_a_tech(struct player *pplayer, struct player *target,
                Tech_type_id preferred);

Tech_type_id give_random_free_tech(struct player *pplayer);
Tech_type_id give_immediate_free_tech(struct player *pplayer);
void give_global_initial_techs(struct player *pplayer);
void give_nation_initial_techs(struct player *pplayer);
Tech_type_id give_random_initial_tech(struct player *pplayer);

#endif
