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
#ifndef FC__AITOOLS_H
#define FC__AITOOLS_H

#include "shared.h"		/* bool type */

#include "fc_types.h"
#include "unit.h"		/* enum ai_unit_task */

struct pf_path;
struct pf_parameter;
struct pft_amphibious;

/*
 * WAGs: how hard to avoid tall stacks of units.
 * Pass as fearfulness values to ai_avoid_risks.
 */
#define NORMAL_STACKING_FEARFULNESS ((double)PF_TURN_FACTOR / 36.0)

#ifdef DEBUG
#define CHECK_UNIT(punit)                                                   \
  (assert(punit != NULL),						    \
   assert(unit_type(punit) != NULL),						    \
   assert(unit_owner(punit) != NULL),					    \
   assert(valid_player_by_number(player_index(unit_owner(punit))) == unit_owner(punit)),	    \
   assert(game_find_unit_by_number(punit->id) != NULL))
#else
#define CHECK_UNIT(punit) assert(TRUE)
#endif

/*
 * Initialise using ai_avoid_risks()
 */
struct ai_risk_cost
{
  double base_value;
  double fearfulness;
  double enemy_zoc_cost;
};

const char *ai_unit_task_rule_name(const enum ai_unit_task task);
const char *ai_choice_rule_name(const struct ai_choice *choice);

int military_amortize(struct player *pplayer, struct city *pcity, 
                      int value, int delay, int build_cost);
int stack_cost(struct unit *pdef);

void ai_avoid_risks(struct pf_parameter *parameter,
		    struct ai_risk_cost *risk_cost,
		    struct unit *punit,
		    const double fearfulness);
void ai_fill_unit_param(struct pf_parameter *parameter,
			struct ai_risk_cost *risk_cost,
			struct unit *punit, struct tile *ptile);
bool ai_unit_execute_path(struct unit *punit, struct pf_path *path);
bool ai_gothere(struct player *pplayer, struct unit *punit, 
                struct tile *dst_tile);
struct tile *immediate_destination(struct unit *punit,
				   struct tile *dest_tile);
void ai_log_path(struct unit *punit,
		 struct pf_path *path, struct pf_parameter *parameter);
bool ai_follow_path(struct unit *punit, struct pf_path *path,
		    struct tile *ptile);
bool ai_unit_goto_constrained(struct unit *punit, struct tile *ptile,
			      struct pf_parameter *parameter);
bool ai_unit_goto(struct unit *punit, struct tile *ptile);

void ai_unit_new_role(struct unit *punit, enum ai_unit_task task, 
                      struct tile *ptile);
bool ai_unit_make_homecity(struct unit *punit, struct city *pcity);
bool ai_unit_attack(struct unit *punit, struct tile *ptile);
bool ai_unit_move(struct unit *punit, struct tile *ptile);

struct city *dist_nearest_city(struct player *pplayer, struct tile *ptile,
                               bool everywhere, bool enemy);

void ai_government_change(struct player *pplayer, struct government *gov);
int ai_gold_reserve(struct player *pplayer);

void init_choice(struct ai_choice *choice);
void adjust_choice(int value, struct ai_choice *choice);
void copy_if_better_choice(struct ai_choice *cur, struct ai_choice *best);

bool is_unit_choice_type(enum choice_type type);

bool ai_choose_role_unit(struct player *pplayer, struct city *pcity,
                         struct ai_choice *choice, enum choice_type type,
                         int role, int want, bool need_boat);
void ai_advisor_choose_building(struct city *pcity, struct ai_choice *choice);
bool ai_assess_military_unhappiness(struct city *pcity);

bool ai_wants_no_science(struct player *pplayer);

bool is_player_dangerous(struct player *pplayer, struct player *aplayer);

void ai_reinit(struct unit *punit);

#endif  /* FC__AITOOLS_H */
