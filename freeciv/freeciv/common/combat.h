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
#ifndef FC__COMBAT_H
#define FC__COMBAT_H

#include "fc_types.h"
#include "unittype.h"

/*
 * attack_strength and defense_strength are multiplied by POWER_FACTOR
 * to yield the base of attack_power and defense_power.
 *
 * The constant may be changed since it isn't externally visible used.
 */
#define POWER_FACTOR	10

bool is_unit_reachable_by_unit(const struct unit *defender,
                               const struct unit *attacker);
bool can_player_attack_tile(const struct player *pplayer,
			    const struct tile *ptile);
bool can_unit_attack_unit_at_tile(const struct unit *punit,
				  const struct unit *pdefender,
				  const struct tile *dest_tile);
bool can_unit_attack_all_at_tile(const struct unit *punit,
				 const struct tile *ptile);
bool can_unit_attack_tile(const struct unit *punit,
			  const struct tile *ptile);

double win_chance(int as, int ahp, int afp, int ds, int dhp, int dfp);

void get_modified_firepower(const struct unit *attacker,
			    const struct unit *defender,
			    int *att_fp, int *def_fp);
double unit_win_chance(const struct unit *attacker,
		       const struct unit *defender);

bool unit_really_ignores_citywalls(const struct unit *punit);
struct city *sdi_try_defend(const struct player *owner,
			       const struct tile *ptile);

int get_attack_power(const struct unit *punit);
int base_get_attack_power(const struct unit_type *punittype,
			  int veteran, int moves_left);
int base_get_defense_power(const struct unit *punit);
int get_defense_power(const struct unit *punit);
int get_total_defense_power(const struct unit *attacker,
			    const struct unit *defender);
int get_virtual_defense_power(const struct unit_type *attacker,
			      const struct unit_type *defender,
			      const struct player *defending_player,
			      const struct tile *ptile,
			      bool fortified, int veteran);
int get_total_attack_power(const struct unit *attacker,
			   const struct unit *defender);

struct unit *get_defender(const struct unit *attacker,
			  const struct tile *ptile);
struct unit *get_attacker(const struct unit *defender,
			  const struct tile *ptile);

bool is_stack_vulnerable(const struct tile *ptile);
#endif /* FC__COMBAT_H */
