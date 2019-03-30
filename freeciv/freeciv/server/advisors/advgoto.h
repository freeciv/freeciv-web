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
#ifndef FC__ADVGOTO_H
#define FC__ADVGOTO_H

/* common/aicore */
#include "path_finding.h"

/*
 * WAGs: how hard to avoid tall stacks of units.
 * Pass as fearfulness values to adv_avoid_risks.
 */
#define NORMAL_STACKING_FEARFULNESS ((double)PF_TURN_FACTOR / 36.0)

struct adv_risk_cost
{
  double base_value;
  double fearfulness;
  double enemy_zoc_cost;
};

bool adv_follow_path(struct unit *punit, struct pf_path *path,
                     struct tile *ptile);

bool adv_unit_execute_path(struct unit *punit, struct pf_path *path);

int adv_could_unit_move_to_tile(struct unit *punit, struct tile *dst_tile);

bool adv_danger_at(struct unit *punit, struct tile *ptile);

void adv_avoid_risks(struct pf_parameter *parameter,
                     struct adv_risk_cost *risk_cost,
                     struct unit *punit,
                     const double fearfulness);

int adv_unittype_att_rating(const struct unit_type *punittype, int veteran,
                            int moves_left, int hp);
int adv_unit_att_rating(const struct unit *punit);
int adv_unit_def_rating_basic(const struct unit *punit);
int adv_unit_def_rating_basic_squared(const struct unit *punit);

#endif   /* FC__ADVGOTO_H */
